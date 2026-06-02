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
#include <memory>
#include <vector>

CClientAlarmManager::CClientAlarmManager(const std::string& host, int port, const std::string& user, const std::string& pass)
    : host_(host), port_(port), username_(user), password_(pass)
{
    
}

CClientAlarmManager::~CClientAlarmManager()
{
    Stop();
    NSDK_LOG_INFO("[DIAG-ALARM] CClientAlarmManager destroyed, totalReconnects=%d", m_reconnectCount.load());
}

bool CClientAlarmManager::StartListen(void* userHandle, const std::string& sessionId)
{
    // 确保旧线程完全停止再启动
    Stop();

    userHandle_ = userHandle;
    sessionId_ = sessionId;
    isRunning_ = true;

    // 赋值前 thread_ 已经被 Stop() 中 join/detach 处理完毕，安全赋值
    thread_ = std::thread(&CClientAlarmManager::AlarmLoop, this);

    // 启动健康监控线程：当 read_timeout 失效导致 AlarmLoop 阻塞时，强制中断恢复
    m_lastDataTimeMs.store(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    if (!m_healthMonitorRunning.exchange(true)) {
        if (m_healthMonitorThread.joinable()) m_healthMonitorThread.join();
        m_healthMonitorThread = std::thread(&CClientAlarmManager::HealthMonitorLoop, this);
    }

    NSDK_LOG_INFO("[DIAG-ALARM] User-%p StartListen: session=%s, starting AlarmLoop+HealthMonitor",
                  userHandle_, sessionId_.c_str());
    return true;
}


void CClientAlarmManager::Stop()
{
    isRunning_ = false;

    // 先停健康监控线程
    if (m_healthMonitorRunning.exchange(false)) {
        if (m_healthMonitorThread.joinable()) {
            if (std::this_thread::get_id() != m_healthMonitorThread.get_id())
                m_healthMonitorThread.join();
            else
                m_healthMonitorThread.detach();
        }
    }

    // 在锁内调用 client_->stop() 并释放成员引用，防止与 AlarmLoop 重建 client_ 并发
    {
        std::lock_guard<std::mutex> lk(clientMutex_);
        if (client_) client_->stop();
        client_.reset();  // 释放成员引用，旧 AlarmLoop 持有的 localClient 仍保持对象存活
    }

    // 使用 m_alarmLoopExited 标志 + 超时来安全关闭 AlarmLoop 线程
    // 在此平台 read_timeout 和 client_->stop() 均无法中断阻塞的 recv()，
    // 如果 AlarmLoop 卡在 recv() 中，直接 join 会永久阻塞
    if (thread_.joinable())
    {
        if (std::this_thread::get_id() == thread_.get_id())
        {
            thread_.detach();
        }
        else
        {
            // 等待 AlarmLoop 自然退出（最多 3 秒）
            bool exited = false;
            for (int i = 0; i < 30; ++i) {
                if (m_alarmLoopExited.load()) {
                    exited = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            if (exited) {
                thread_.join();
                NSDK_LOG_INFO("[DIAG-ALARM] User-%p Stopped: AlarmLoop joined normally, totalReconnects=%d",
                              userHandle_, m_reconnectCount.load());
            } else {
                // AlarmLoop 卡在 recv() 中无法退出，只能 detach
                thread_.detach();
                NSDK_LOG_WARN("[DIAG-ALARM] User-%p Stopped: AlarmLoop STUCK in recv(), detached. "
                              "totalReconnects=%d",
                              userHandle_, m_reconnectCount.load());
            }
        }
    }
}

void CClientAlarmManager::AlarmLoop()
{
    std::string boundary;
    std::string buffer;
    std::string pendingJson;

    // 返回 false 表示需要断开连接（僵尸检测或用户停止）
    auto dispatch_alarm = [&](const std::string& jsonBody) -> bool {
        Json::Object* root = Json::init(jsonBody);
        if (!root) {
            NSDK_LOG_WARN("[DIAG-ALARM] User-%p JSON parse failed, len=%zu", userHandle_, jsonBody.size());
            return true;  // JSON 解析失败，不是致命错误，继续处理
        }

        long long lCommand = 0;
        Json::get(root, "Command", lCommand);

        std::string eventType;
        Json::get(root, "Event", eventType);

        // 过滤服务端心跳包，不传递给上层
        std::string msgType;
        Json::get(root, "type", msgType);
        if (msgType == "heartbeat") {
            heartbeatRecvCount_++;
            // 僵尸连接检测：连接存活(心跳在走)但长时间没有报警数据
            // 正常情况摄像机每10秒推一次统计报警，5分钟没有 = 一定有问题
            if (alarmRecvCount_ > 0) {
                auto noAlarmSec = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - m_lastAlarmTime).count();
                if (noAlarmSec >= 300) {
                    NSDK_LOG_WARN("[DIAG-ALARM] User-%p ZOMBIE: no alarm for %llds (hb=%d, alarms=%d), force reconnect",
                                  userHandle_, (long long)noAlarmSec,
                                  heartbeatRecvCount_.load(), alarmRecvCount_.load());
                    Json::deinit(root);
                    return false;  // 让 content_receiver 也返回 false，强制断连重连
                }
            }
            // 心跳日志降级为 DEBUG，避免刷屏；每 30 条打印一次 INFO
            int hb = heartbeatRecvCount_.load();
            if (hb % 30 == 0) {
                NSDK_LOG_INFO("[DIAG-ALARM] User-%p heartbeat #%d, alarms=%d, conn_alive",
                              userHandle_, hb, alarmRecvCount_.load());
            }
            Json::deinit(root);
            return true;
        }
        alarmRecvCount_++;
        m_lastAlarmTime = std::chrono::steady_clock::now();

        // [DIAG] 提取入队时间戳，计算端到端延迟
        long long enqueueTs = 0;
        Json::get(root, "enqueue_ts", enqueueTs);
        if (enqueueTs > 0) {
            auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            NSDK_LOG_INFO("[DIAG-ALARM] User-%p alarm received: cmd=0x%llX, event=%s, e2e_delay_ms=%lld",
                          userHandle_, lCommand, eventType.c_str(), nowMs - enqueueTs);
        } else {
            NSDK_LOG_INFO("[DIAG-ALARM] User-%p alarm received: cmd=0x%llX, event=%s",
                          userHandle_, lCommand, eventType.c_str());
        }

        if (eventType == "ChannelStatus" || lCommand == NET_TV_NOTIFY_CHANNEL_STATUS)
        {
            NET_TV_CHANNEL_INFO_S info = {0};
            bool parseSuccess = false;

            if (auto* channelObj = Json::get(root, "ChannelInfo"))
            {
                SDKConvert::deal(channelObj, info, true);
                parseSuccess = true;
            }

            if (parseSuccess && channelStatusCb_)
            {
                channelStatusCb_(&info, channelStatusUserData_);
            }

            Json::deinit(root);
            return true;
        }

        if (!alarmCb_)
        {
            Json::deinit(root);
            return true;
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
            return true;
        }

        if (lCommand == NET_TV_ALARM_FACE_COMPARE)
        {
            std::unique_ptr<NET_TV_ALARM_FACE_COMPARE_INFO_S> info(new NET_TV_ALARM_FACE_COMPARE_INFO_S());
            SDKConvert::deal(alarmInfoObj, *info, true);
            INT32 len = (INT32)sizeof(*info);
            alarmCb_(lCommand, &alarmer, (CHAR*)info.get(), &len, alarmUserData_);
        }else if (alarmBase == NET_TV_ALARM_BASE_BASIC)
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
        return true;
    };

    int loopCount = 0;
    m_alarmLoopExited = false;
    while(isRunning_)
    {
        loopCount++;
        auto tp_before_connect = std::chrono::steady_clock::now();

        // 在锁内重建 client_，防止与 Stop()/HealthMonitor 并发访问
        // 使用 shared_ptr：HealthMonitor 重建 client_ 后，旧 AlarmLoop 持有的
        // localClient 仍能保持旧 Client 存活，避免 use-after-free
        std::shared_ptr<httplib::Client> localClient;
        {
            std::lock_guard<std::mutex> lk(clientMutex_);
            client_ = std::make_shared<httplib::Client>(host_, port_);
            client_->set_digest_auth(username_.c_str(), password_.c_str());
            client_->set_read_timeout(15);
            client_->set_keep_alive(true);
            localClient = client_;  // 保持本地引用，防止 HealthMonitor 替换 client_ 后析构
        }

        // 重置连接健康计数器
        heartbeatRecvCount_ = 0;
        alarmRecvCount_ = 0;
        m_firstDataReceived = false;
        m_lastStatTime = std::chrono::steady_clock::time_point{};
        m_connStartTime = std::chrono::steady_clock::now();
        m_lastAlarmTime = std::chrono::steady_clock::now();
        // 更新时间戳，防止健康监控线程误判为假死
        m_lastDataTimeMs.store(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());

        // 每次重连时读取最新的 sessionId（ReconnectLoop 登录后会通过 UpdateSessionId 更新）
        std::string currentSessionId;
        {
            std::lock_guard<std::mutex> lk(sessionIdMutex_);
            currentSessionId = sessionId_;
        }

        m_reconnectCount++;
        std::string url = std::string(TVAPI_PATH_ALARMEVENT_LISTEN) + "?session_id=" + currentSessionId;
        NSDK_LOG_INFO("[DIAG-ALARM] User-%p [CONNECT #%d] url=%s, host=%s:%d",
                      userHandle_, m_reconnectCount.load(), url.c_str(), host_.c_str(), port_);

        auto res = localClient->Get(url.c_str(), [&](const httplib::Response& response)
        {
            NSDK_LOG_INFO("[DIAG-ALARM] User-%p [RESPONSE] status=%d, ct=%s",
                          userHandle_, response.status,
                          response.get_header_value("Content-Type").c_str());
            if (response.status != 200) return false;
            // HTTP 200 响应到达说明连接已建立，更新活跃时间戳防止 HealthMonitor 误判
            m_lastDataTimeMs.store(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count());
            std::string ct = response.get_header_value("Content-Type");
            auto pos = ct.find("boundary=");
            if (pos != std::string::npos) {
                boundary = "--" + ct.substr(pos + 9);
            }
            return true;
        }, [&](const char* data, size_t len)
        {
            if (!isRunning_) return false;

            auto tp_now = std::chrono::steady_clock::now();
            m_lastDataTimeMs.store(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    tp_now.time_since_epoch()).count());
            m_firstDataReceived = true;  // 收到任何数据都标记连接正常

            buffer.append(data, len);

            // 每 60 秒输出一次数据接收统计（防止日志刷屏）
            auto statElapsed = std::chrono::duration_cast<std::chrono::seconds>(
                tp_now - m_lastStatTime).count();
            if (m_lastStatTime == std::chrono::steady_clock::time_point{} || statElapsed >= 60) {
                NSDK_LOG_INFO("[DIAG-ALARM] User-%p [DATA] buffer=%zu bytes, hb=%d, alarms=%d",
                              userHandle_, buffer.size(),
                              heartbeatRecvCount_.load(), alarmRecvCount_.load());
                m_lastStatTime = tp_now;
            }

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
                            if (!dispatch_alarm(pendingJson)) {
                                // 僵尸检测触发，断开连接
                                return false;
                            }
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

        // 长连接断开，记录详细状态
        auto connDuration = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - m_connStartTime).count();
        auto totalElapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - tp_before_connect).count();

        if (res) {
            if (res->status == HTTP_RESP_CODE_UNAUTHORIZED || res->status == 401)
            {
                NSDK_LOG_ERROR("[DIAG-ALARM] User-%p [DISCONNECT] 401 Unauthorized: conn=%llds, "
                              "hb=%d, alarms=%d, triggering re-login",
                              userHandle_, (long long)connDuration,
                              heartbeatRecvCount_.load(), alarmRecvCount_.load());
                if (sessionExpiredCb_) sessionExpiredCb_();
                break; // 退出循环，由上层重新登录后重启 AlarmListen
            }
            // 其他 HTTP 错误（500/503 等）
            NSDK_LOG_WARN("[DIAG-ALARM] User-%p [DISCONNECT] HTTP status=%d: conn=%llds, "
                          "hb=%d, alarms=%d",
                          userHandle_, res->status, (long long)connDuration,
                          heartbeatRecvCount_.load(), alarmRecvCount_.load());
        } else {
            // no response: 服务端关闭了 TCP 连接、网络中断、或 read_timeout 超时
            auto err = res.error();
            NSDK_LOG_WARN("[DIAG-ALARM] User-%p [DISCONNECT] No response (err=%d): "
                          "conn=%llds, total=%llds, hb=%d, alarms=%d, reconnecting...",
                          userHandle_, (int)err, (long long)connDuration, (long long)totalElapsed,
                          heartbeatRecvCount_.load(), alarmRecvCount_.load());
        }

        // 清空缓冲区，避免脏数据
        buffer.clear();
        boundary.clear();
        pendingJson.clear();

        // 统一等 0.5 秒再重连
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    m_alarmLoopExited = true;
    NSDK_LOG_INFO("[DIAG-ALARM] User-%p AlarmLoop EXIT: totalLoops=%d, totalReconnects=%d",
                  userHandle_, loopCount, m_reconnectCount.load());
}

void CClientAlarmManager::HealthMonitorLoop()
{
    NSDK_LOG_INFO("[DIAG-ALARM] User-%p HealthMonitor started: checkInterval=5s, "
                  "timeoutInitial=30s, timeoutDataLoss=60s",
                  userHandle_);

    while (m_healthMonitorRunning.load()) {
        for (int i = 0; i < 5 && m_healthMonitorRunning.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (!m_healthMonitorRunning.load()) break;

        auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        auto lastMs = m_lastDataTimeMs.load();
        auto elapsedSec = (lastMs > 0) ? (nowMs - lastMs) / 1000 : 0;

        // 分级超时：未收过数据 30s，数据中断 60s
        int64_t timeoutSec = m_firstDataReceived.load() ? 60 : 30;

        if (lastMs > 0 && elapsedSec > timeoutSec) {
            NSDK_LOG_ERROR("[DIAG-ALARM] User-%p [HEALTH] NO DATA for %llds (threshold=%llds, "
                          "hadData=%d). Connection is STUCK in recv(). "
                          "Stopping AlarmLoop and delegating recovery to upper layer.",
                          userHandle_, (long long)elapsedSec, (long long)timeoutSec,
                          (int)m_firstDataReceived.load());

            // 1. 通知 AlarmLoop 退出（设标志后它醒来/超时后会自然 break）
            isRunning_ = false;

            // 2. 尝试中断旧 client（best-effort，此平台大概率无效）
            {
                std::lock_guard<std::mutex> lk(clientMutex_);
                if (client_) client_->stop();
                client_.reset();
            }

            // 3. 异步通知上层触发完整重连流程
            //    !! 必须异步 !!：若同步调用，ReconnectLoop 成功后会调 Stop()，
            //    Stop() 会 join(m_healthMonitorThread)，而此时正是 HealthMonitor 自己
            //    在等待，导致线程等待自身——死锁。
            if (sessionExpiredCb_) {
                NSDK_LOG_INFO("[DIAG-ALARM] User-%p [HEALTH] Dispatching sessionExpiredCb_ async",
                              userHandle_);
                SessionExpiredCallback cb = sessionExpiredCb_; // 值拷贝，防止 this 提前析构
                std::thread([cb]() { cb(); }).detach();
            }

            // 4. HealthMonitor 退出，由上层 StartListen 重新启动新的 HealthMonitor
            break;
        }
    }
    NSDK_LOG_INFO("[DIAG-ALARM] User-%p HealthMonitor EXIT", userHandle_);
}
