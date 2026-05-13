/**
 * @file ClientAlarmManager.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-25
 * 
 * @brief 客户端告警管理类实现
 */

#include "ClientAlarmManager.h"
#include "NetSdkLog.h"
#include "NetTVSDKHttpUrl.h"
#include "Json.h"
#include "AlarmInfoConvert.h"
#include "DeviceInfoConvert.h"

#include <algorithm>
#include <cstring>
#include <vector>

CClientAlarmManager::CClientAlarmManager(const std::string& host, int port, const std::string& user, const std::string& pass)
    : host_(host), port_(port), username_(user), password_(pass)
{
    
}

CClientAlarmManager::~CClientAlarmManager()
{
    Stop();
}

bool CClientAlarmManager::StartListen(void* userHandle, const std::string& sessionId)
{
    if (isRunning_) return true;
    
    userHandle_ = userHandle;
    sessionId_ = sessionId;
    isRunning_ = true;
    thread_ = std::thread(&CClientAlarmManager::AlarmLoop, this);
    
    NSDK_LOG_INFO("[ClientAlarmManager] Started for User-%p session=%s", userHandle_, sessionId_.c_str());
    return true;
}


void CClientAlarmManager::Stop()
{
    isRunning_ = false;
    
    if (client_) client_->stop();
    
    if (thread_.joinable()) 
    {
        if (std::this_thread::get_id() != thread_.get_id()) 
        {
            thread_.join();
        } 
        else 
        {
            thread_.detach();
        }
    }
    NSDK_LOG_INFO("[ClientAlarmManager] Stopped.");
}

void CClientAlarmManager::AlarmLoop()
{
    client_ = std::make_unique<httplib::Client>(host_, port_);
    client_->set_digest_auth(username_.c_str(), password_.c_str());
    client_->set_read_timeout(300); // 5 minutes timeout
    client_->set_keep_alive(true);

    std::string boundary;
    std::string buffer;
    std::string pendingJson;



    auto dispatch_alarm = [&](const std::string& jsonBody) {
        Json::Object* root = Json::init(jsonBody);
        if (!root) return;

        long long lCommand = 0;
        Json::get(root, "Command", lCommand);

        std::string eventType;
        Json::get(root, "Event", eventType);
        if (eventType == "ChannelStatus" || lCommand == NET_TV_NOTIFY_CHANNEL_STATUS)
        {
            if (channelStatusCb_)
            {
                NET_TV_CHANNEL_INFO_S info = {0};
                if (auto* channelObj = Json::get(root, "ChannelInfo"))
                {
                    SDKConvert::deal(channelObj, info, true);
                    channelStatusCb_(&info, channelStatusUserData_);
                }
            }
            Json::deinit(root);
            return;
        }

        if (!alarmCb_)
        {
            Json::deinit(root);
            return;
        }

        INT32 alarmBase = ((INT32)lCommand) & 0xF000;

        NET_TV_ALARMER_S alarmer = {0};
        if (auto* alarmerObj = Json::get(root, "Alarmer"))
        {
            SDKConvert::deal(alarmerObj, alarmer, true);
        }

        Json::Object* alarmInfoObj = Json::get(root, "AlarmInfo");
        if (!alarmInfoObj)
        {
            std::vector<char> tmp(jsonBody.begin(), jsonBody.end());
            tmp.push_back('\0'); // ensure C-string
            INT32 len = (INT32)(tmp.size() - 1);
            alarmCb_(lCommand, &alarmer, tmp.data(), &len, alarmUserData_);
            Json::deinit(root);
            return;
        }

        if (lCommand == NET_TV_ALARM_FACE_COMPARE)
        {
            NET_TV_ALARM_FACE_COMPARE_INFO_S info = {0};
            SDKConvert::deal(alarmInfoObj, info, true);
            INT32 len = (INT32)sizeof(info);
            alarmCb_(lCommand, &alarmer, (CHAR*)&info, &len, alarmUserData_);
        }
        else if (alarmBase == NET_TV_ALARM_BASE_BASIC)
        {
            NET_TV_ALARM_BASIC_INFO_S info = {0};
            SDKConvert::deal(alarmInfoObj, info, true);
            INT32 len = (INT32)sizeof(info);
            alarmCb_(lCommand, &alarmer, (CHAR*)&info, &len, alarmUserData_);
        }
        else if (alarmBase == NET_TV_ALARM_BASE_RULE)
        {
            NET_TV_ALARM_RULE_INFO_S info = {0};
            SDKConvert::deal(alarmInfoObj, info, true);
            INT32 len = (INT32)sizeof(info);
            alarmCb_(lCommand, &alarmer, (CHAR*)&info, &len, alarmUserData_);
        }
        else if (alarmBase == NET_TV_ALARM_BASE_AI)
        {
            NET_TV_ALARM_AI_OBJECT_INFO_S info = {0};
            SDKConvert::deal(alarmInfoObj, info, true);
            INT32 len = (INT32)sizeof(info);
            alarmCb_(lCommand, &alarmer, (CHAR*)&info, &len, alarmUserData_);
        }
        else if (alarmBase == NET_TV_ALARM_BASE_TRAFFIC)
        {
            NET_TV_ALARM_PLATE_INFO_S info = {0};
            SDKConvert::deal(alarmInfoObj, info, true);
            INT32 len = (INT32)sizeof(info);
            alarmCb_(lCommand, &alarmer, (CHAR*)&info, &len, alarmUserData_);
        }
        else if (alarmBase == NET_TV_ALARM_BASE_EXCEPTION)
        {
            NET_TV_ALARM_EXCEPTION_INFO_S info = {0};
            SDKConvert::deal(alarmInfoObj, info, true);
            INT32 len = (INT32)sizeof(info);
            alarmCb_(lCommand, &alarmer, (CHAR*)&info, &len, alarmUserData_);
        }
        else if (alarmBase == NET_TV_ALARM_BASE_STATISTICS)
        {
            NET_TV_ALARM_STATISTICS_INFO_S info = {0};
            SDKConvert::deal(alarmInfoObj, info, true);
            INT32 len = (INT32)sizeof(info);
            alarmCb_(lCommand, &alarmer, (CHAR*)&info, &len, alarmUserData_);
        }
        else if (lCommand == NET_TV_NOTICE_DOWNLOAD_RECORD_PROGRESS)
        {
            NET_TV_RECORD_DOWNLOAD_PROGRESS_S info = {0};
            SDKConvert::deal(alarmInfoObj, info, true);
            INT32 len = (INT32)sizeof(info);
            alarmCb_(lCommand, &alarmer, (CHAR*)&info, &len, alarmUserData_);
        }
        else
        {
            std::vector<char> tmp(jsonBody.begin(), jsonBody.end());
            tmp.push_back('\0'); // ensure C-string
            INT32 len = (INT32)(tmp.size() - 1);
            alarmCb_(lCommand, &alarmer, tmp.data(), &len, alarmUserData_);
        }

        Json::deinit(root);
    };
    
    while(isRunning_)
    {
        std::string url = std::string(TVAPI_PATH_ALARMEVENT_LISTEN) + "?session_id=" + sessionId_;
        auto res = client_->Get(url.c_str(), [&](const httplib::Response& response) 
        {
            if (response.status != 200) return false;
            std::string ct = response.get_header_value("Content-Type");
            auto pos = ct.find("boundary=");
            if (pos != std::string::npos) {
                boundary = "--" + ct.substr(pos + 9);
            }
            return true;
        }, [&](const char* data, size_t len) 
        {
            if (!isRunning_) return false;
            buffer.append(data, len);
            
            if (boundary.empty()) return true;
            size_t pos = 0;
            while (true) 
            {
                size_t boundPos = buffer.find(boundary, pos);
                if (boundPos == std::string::npos) {
                     if (buffer.length() > pos + boundary.length()) 
                     {
                          buffer.erase(0, pos);
                          pos = 0;
                     }
                     break;
                }
                
                if (boundPos > pos) {
                    std::string part = buffer.substr(pos, boundPos - pos);
                    size_t headerEnd = part.find("\r\n\r\n");
                    if (headerEnd != std::string::npos) 
                    {
                        std::string headers = part.substr(0, headerEnd);
                        std::string body = part.substr(headerEnd + 4);
                        if (body.length() >= 2 && body.substr(body.length()-2) == "\r\n") 
                        {
                            body.resize(body.length()-2);
                        }
                        
                        /* JSON数据 */
                        if (headers.find("application/json") != std::string::npos) 
                        {
                            pendingJson = body;
                            dispatch_alarm(pendingJson);
                            pendingJson.clear();
                        }
                        /* 图片数据 */
                        else if (headers.find("image") != std::string::npos) 
                        {
                            // 忽略：不依赖 multipart 附件
                        }
                    }
                }
                pos = boundPos + boundary.length();
            }
            if (pos > 0) buffer.erase(0, pos);
            return true;
        });

        if (!isRunning_) break;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
