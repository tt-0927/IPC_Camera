/**
 * @file UserSession.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-22
 * 
 * @brief 用户会话类
 */

#include "UserSession.h"
#include "NetTVSDKHttpUrl.h"
#include "NetSdkLog.h"
#include "SDKConvert.h"
#include "NetTVSDKClientInterface.h"
#include "ErrorManage.h"

CUserSession::CUserSession(LPUSER_HANDLE userHand, const std::string& host, int port, 
                           const std::string& user, const std::string& pass,
                           int hbInterval, int maxRetry,
						   int connectTimeout, int receiveTimeout,
						   OnSessionLostCallback callback)
    : userHand_(userHand), host_(host), port_(port), 
      username_(user), password_(pass),
      heartbeatInterval_(hbInterval), maxRetry_(maxRetry),
	  notifyLost_(callback)
{
    cmdClient_ = std::make_unique<httplib::Client>(host_, port_);
    cmdClient_->set_digest_auth(username_.c_str(), password_.c_str());

    // 应用超时配置
    cmdClient_->set_connection_timeout(connectTimeout); // 连接超时
    cmdClient_->set_read_timeout(receiveTimeout);       // 接收(读取)超时
    cmdClient_->set_keep_alive(true);

    sseClient_ = std::make_unique<httplib::Client>(host_, port_);
    sseClient_->set_digest_auth(username_.c_str(), password_.c_str());
    sseClient_->set_keep_alive(true);
	sseClient_->set_read_timeout(std::max(receiveTimeout, 300)); // SSE 建议至少 300s
	sseClient_->set_connection_timeout(connectTimeout);
    
    // Initialize Alarm Manager
    m_alarmMgr = std::make_shared<CClientAlarmManager>(host_, port_, username_, password_);
}
CUserSession::~CUserSession() 
{
    Stop();
}

bool CUserSession::ConnectAndLogin() 
{
    auto res = cmdClient_->Post(TVAPI_PATH_BASIC_LOGIN);
    
    if (res && res->status == HTTP_RESP_CODE_SUCCESS) 
	{        
		NSDK_LOG_DEBUG("登录JSon响应：[%s]",res->body.c_str());
		SeesionMessage_S stSeesionMessage;
		int nRespCode = SDKConvert::get_respCode(res->body);
		if (nRespCode == NET_TV_E_SUCCEED) 
		{
			isOnline_ = true;
			SDKConvert::to_respStruct(res->body.c_str(),stSeesionMessage);
			sessionId_ = stSeesionMessage.SeesionId;

			return true;
		}
    }
    
    NSDK_LOG_ERROR("[User-%p] Login Failed. Status: %d", userHand_, res ? res->status : -1);
    return false;
}

void CUserSession::StartHeartbeat() 
{
    if (isRunning_) return;
    isRunning_ = true;
    // sseThread_ = std::thread(&CUserSession::SseLoop, this);

	heartbeatThread_ = std::thread(&CUserSession::HeartbeatLoop, this);
}

void CUserSession::Stop() 
{
    bool expected = true;
    if (isRunning_.compare_exchange_strong(expected, false)) 
    {
        NSDK_LOG_INFO("[User-%p] Stopping session flag set...", userHand_);
        if (sseClient_) sseClient_->stop();
    }

    if (sseThread_.joinable()) 
    {
        if (std::this_thread::get_id() != sseThread_.get_id()) 
        {
            sseThread_.join();
        } 
        else 
        {
            sseThread_.detach(); // 防止自己 join 自己（虽然这种情况很少见）
        }
    }

    if (heartbeatThread_.joinable()) 
    {
        if (std::this_thread::get_id() != heartbeatThread_.get_id()) 
        {
            heartbeatThread_.join();
        } 
        else 
        {
            heartbeatThread_.detach();
        }
    }

    isOnline_ = false;
    
    if (m_alarmMgr) {
        m_alarmMgr->Stop();
    }
    
    NSDK_LOG_INFO("[User-%p] Stopped and threads joined.", userHand_);
}

void CUserSession::SseLoop() 
{
    int retryCount = 0;
    
    while (isRunning_) 
	{
        // 构造 SSE URL
        std::string url = "/TVAPI/V1.0/Basic/KeepLive?session_id=" + sessionId_;
        
        NSDK_LOG_DEBUG("[User-%p] SSE Connecting...", userHand_);

        // 发起长连接请求
        auto res = sseClient_->Get(url.c_str(), [&](const httplib::Response& response) 
		{
			 NSDK_LOG_DEBUG("[User] SSE response code [%d]", response.status);
            if (response.status != HTTP_RESP_CODE_SUCCESS) return false; 
            
            // 连接成功
            retryCount = 0;
            if (!isOnline_) 
			{
                isOnline_ = true;
                NSDK_LOG_INFO("[User-%p] Online (Heartbeat Restored)", userHand_);
            }
            return true; 
        }, 
        [&](const char* data, size_t len) 
		{
            // [Data 回调]
            if (!isRunning_)
			{
				return false;
			} 

			NSDK_LOG_DEBUG("Received: data[%s] len[%d]",std::string(data, len).c_str(),len);

            return true; 
        });

        // --- 连接断开 ---
 		isOnline_ = false;
        if (!isRunning_) break; // 主动停止

        NSDK_LOG_ERROR("[User-%p] Connection Lost.", userHand_);

        // 重连机制
        if (retryCount < maxRetry_) 
		{
            NSDK_LOG_INFO("[User-%p] Waiting %d sec to retry (%d/%d)...", userHand_, heartbeatInterval_, retryCount + 1, maxRetry_);
            std::this_thread::sleep_for(std::chrono::seconds(heartbeatInterval_));
            retryCount++;
        }
		else
		{
            NSDK_LOG_ERROR("[User-%p] Max retries reached. Stopping heartbeat.", userHand_);
			/* 触发掉线回调 */
			if (notifyLost_) 
			{
                notifyLost_(userHand_);
            }
            break; 
        }
    }
}

void CUserSession::HeartbeatLoop()
{
    int failCount = 0;
    
    while (isRunning_) 
    {
        for (int i = 0; i < heartbeatInterval_; ++i) 
		{
            if (!isRunning_) break;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (!isRunning_) break;

        std::string url = "/TVAPI/V1.0/Basic/KeepLive?session_id=" + sessionId_;
        httplib::Result res;
        
        {
            std::lock_guard<std::mutex> lock(cmdMutex_);
            res = cmdClient_->Get(url.c_str());
        }

        if (res && res->status == HTTP_RESP_CODE_SUCCESS) 
		{
            failCount = 0;
			// NSDK_LOG_DEBUG("[User-%p] This Heartbeat Message.", userHand_);
            if (!isOnline_) 
			{
                isOnline_ = true;
                NSDK_LOG_INFO("[User-%p] Heartbeat recovered.", userHand_);
            }
        } 
        else 
		{
            // 心跳失败处理
            failCount++;                                                                       // 累计失败次数
            NSDK_LOG_WARN("[User-%p] Heartbeat failed (%d/%d). Status: %d",                   // 打印警告日志
                userHand_,                                                                     // 用户会话句柄（内存地址，用于区分不同连接）
                failCount,                                                                     // 当前失败次数
                maxRetry_,                                                                     // 最大允许失败次数
                res ? res->status : -1);                                                      // HTTP响应状态码（-1表示连接完全失败）

            // 判断是否达到最大重试次数
            if (failCount >= maxRetry_) 
			{
                NSDK_LOG_ERROR("[User-%p] Session Lost (Heartbeat timeout).", userHand_);      // 会话超时，断开连接
                isOnline_ = false;                                                             // 标记为离线状态
                
                if (notifyLost_) notifyLost_(userHand_);                                        // 通知上层会话丢失
                
                isRunning_ = false;                                                            // 停止心跳线程
            }
        }
    }
}


bool CUserSession::SendRequest(const CommandRequest& req, std::string& outRespBody) 
{
        if (!isOnline_) 
		{
			CErrorManage::instance()->SetLastError(NET_TV_E_SEND_MSG_ERROR);
            return false;
        }

        std::lock_guard<std::mutex> lock(cmdMutex_); // 串行保护

        // 处理 URL 参数拼接
        std::string finalUrl = req.url;
        if (!req.queryParams.empty()) 
		{
            finalUrl += "?";
            for (const auto& p : req.queryParams) finalUrl += p.first + "=" + p.second + "&";
        }

        httplib::Result res;
        
        // 根据 Method 分发
        if (req.method == "GET") 
		{
            res = cmdClient_->Get(finalUrl.c_str());
        } else if (req.method == "POST") {
            res = cmdClient_->Post(finalUrl.c_str(), req.jsonBody, "application/json");
        } else if (req.method == "PUT") {
            res = cmdClient_->Put(finalUrl.c_str(), req.jsonBody, "application/json");
        }

        if (res && res->status == HTTP_RESP_CODE_SUCCESS) 
		{
            int bizCode = SDKConvert::get_respCode(res->body);
			CErrorManage::instance()->SetLastError(bizCode);
            
            if (bizCode == NET_TV_E_SUCCEED) 
			{
                outRespBody = res->body;
                return true;
            }
            return false;
        }

		CErrorManage::instance()->SetLastError(NET_TV_E_SOCKET_RECV_ERR);
        return false;
    }

void CUserSession::SetAlarmCallback(NET_TV_AlarmCallBack cb, void* userData)
{
    if (m_alarmMgr) 
    {
        m_alarmMgr->SetCallback(cb, userData);
    }
}

void CUserSession::SetChannelStatusCallback(NET_TV_ChannelStatusCallBack cb, void* userData)
{
    if (m_alarmMgr)
    {
        m_alarmMgr->SetChannelStatusCallback(cb, userData);
    }
}

bool CUserSession::StartAlarmListen()
{
    if (!m_alarmMgr)
    {
        return false;
    } 
    
    return m_alarmMgr->StartListen(userHand_, sessionId_);
}
bool CUserSession::StopAlarmListen()
{
    if (m_alarmMgr) 
    {
        m_alarmMgr->Stop();
    }

    return true;
}
