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
#include <cctype>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

namespace
{
std::string TrimAscii(const std::string& value)
{
    size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])))
    {
        ++begin;
    }

    size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])))
    {
        --end;
    }

    return value.substr(begin, end - begin);
}

bool EqualsNoCase(const std::string& lhs, const std::string& rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }

    for (size_t i = 0; i < lhs.size(); ++i)
    {
        if (std::tolower(static_cast<unsigned char>(lhs[i])) !=
            std::tolower(static_cast<unsigned char>(rhs[i])))
        {
            return false;
        }
    }

    return true;
}

bool ContainsNoCase(const std::string& haystack, const std::string& needle)
{
    if (needle.empty())
    {
        return true;
    }

    auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
                          [](char lhs, char rhs) {
                              return std::tolower(static_cast<unsigned char>(lhs)) ==
                                     std::tolower(static_cast<unsigned char>(rhs));
                          });
    return it != haystack.end();
}

bool GetMultipartHeaderValue(const std::string& headers,
                             const std::string& headerName,
                             std::string& value)
{
    size_t pos = 0;
    while (pos <= headers.size())
    {
        size_t lineEnd = headers.find("\r\n", pos);
        if (lineEnd == std::string::npos)
        {
            lineEnd = headers.size();
        }

        std::string line = headers.substr(pos, lineEnd - pos);
        size_t colon = line.find(':');
        if (colon != std::string::npos &&
            EqualsNoCase(TrimAscii(line.substr(0, colon)), headerName))
        {
            value = TrimAscii(line.substr(colon + 1));
            return true;
        }

        if (lineEnd == headers.size())
        {
            break;
        }
        pos = lineEnd + 2;
    }

    return false;
}

bool ParseContentLength(const std::string& headers, size_t& contentLength)
{
    std::string value;
    if (!GetMultipartHeaderValue(headers, "Content-Length", value) || value.empty())
    {
        return false;
    }

    size_t result = 0;
    for (char ch : value)
    {
        if (!std::isdigit(static_cast<unsigned char>(ch)))
        {
            return false;
        }

        size_t digit = static_cast<size_t>(ch - '0');
        if (result > (std::numeric_limits<size_t>::max() - digit) / 10)
        {
            return false;
        }
        result = result * 10 + digit;
    }

    contentLength = result;
    return true;
}

void TrimTrailingCrlf(std::string& value)
{
    if (value.length() >= 2 && value.substr(value.length() - 2) == "\r\n")
    {
        value.resize(value.length() - 2);
    }
}

bool IsCompleteJsonBody(const std::string& body)
{
    Json::Object* root = Json::init(body);
    if (!root)
    {
        return false;
    }

    Json::deinit(root);
    return true;
}
}

/**
 * @brief 构造函数
 * @param [IN] host 设备IP地址
 * @param [IN] port 设备端口号
 * @param [IN] user 用户名
 * @param [IN] pass 密码
 */
CClientAlarmManager::CClientAlarmManager(const std::string& host, int port, const std::string& user, const std::string& pass)
    : host_(host), port_(port), username_(user), password_(pass)
{
    
}

/**
 * @brief 析构函数
 * @details 调用Stop()停止监听，释放所有资源
 */
CClientAlarmManager::~CClientAlarmManager()
{
    Stop();
    NSDK_LOG_INFO("[DIAG-ALARM] CClientAlarmManager destroyed, totalReconnects=%d", m_reconnectCount.load());
}

/**
 * @brief 开始监听报警消息
 * @param [IN] userHandle 用户登录句柄
 * @param [IN] sessionId 会话ID
 * @return 成功返回true，失败返回false
 * @details 启动报警监听线程和健康监控线程，建立长连接接收报警消息
 */
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


/**
 * @brief 停止监听报警消息
 * @details 停止健康监控线程和报警监听线程，关闭客户端连接，释放资源
 */
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

/**
 * @brief 报警监听循环线程
 * @details 建立长连接接收报警消息，解析multipart格式数据，分发报警回调，
 *          处理心跳检测和session过期（401），断线后自动重连
 */
void CClientAlarmManager::AlarmLoop()
{
    std::string boundary;
    std::string buffer;

    // 返回 false 表示当前 AlarmListen 应断开（例如客户端停止）。
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
            NET_ChannelInfo_S info = {0};
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

        NET_Alarmer_S alarmer = {0};
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
            std::unique_ptr<NET_AlarmFaceCompareInfo_S> info(new NET_AlarmFaceCompareInfo_S());
            SDKConvert::deal(alarmInfoObj, *info, true);
            INT32 len = (INT32)sizeof(*info);
            alarmCb_(lCommand, &alarmer, (CHAR*)info.get(), &len, alarmUserData_);
        }else if (alarmBase == NET_TV_ALARM_BASE_BASIC)
        {
            NET_AlarmBasicInfo_S info = {0};
            SDKConvert::deal(alarmInfoObj, info, true);
            NSDK_LOG_INFO("[DIAG-ALARM] User-%p basic parsed: cmd=0x%llX, alarmType=0x%X, timestamp=%lld, panoramaLen=%u",
                          userHandle_,
                          lCommand,
                          info.uAlarmType,
                          (long long)info.llTimestampMs,
                          info.uPanoramaImgLen);
            INT32 len = (INT32)sizeof(info);
            alarmCb_(lCommand, &alarmer, (CHAR*)&info, &len, alarmUserData_);
        }
        else if (alarmBase == NET_TV_ALARM_BASE_RULE)
        {
            auto info = std::make_unique<NET_AlarmRuleInfo_S>();
            SDKConvert::deal(alarmInfoObj, *info, true);
            NSDK_LOG_INFO("[DIAG-ALARM] User-%p rule parsed: cmd=0x%llX, alarmType=0x%X, channel=%u, rule=%u, "
                          "target=%u, objType=%u, timestamp=%lld, rect=[%d,%d,%d,%d], panoramaLen=%u, targetLen=%u, "
                          "jsonHasPanoramaB64=%d, jsonHasTargetB64=%d",
                          userHandle_,
                          lCommand,
                          info->uAlarmType,
                          info->uChannel,
                          info->uRuleID,
                          info->uTargetID,
                          info->uObjectType,
                          (long long)info->llTimestampMs,
                          info->nLeft,
                          info->nTop,
                          info->nRight,
                          info->nBottom,
                          info->uPanoramaImgLen,
                          info->uTargetImgLen,
                          jsonBody.find("PanoramaImgBase64") != std::string::npos ? 1 : 0,
                          jsonBody.find("TargetImgBase64") != std::string::npos ? 1 : 0);
            INT32 len = (INT32)sizeof(*info);
            alarmCb_(lCommand, &alarmer, (CHAR*)info.get(), &len, alarmUserData_);
        }
        else if (alarmBase == NET_TV_ALARM_BASE_AI)
        {
            auto info = std::make_unique<NET_AlarmAiObjectInfo_S>();
            SDKConvert::deal(alarmInfoObj, *info, true);
            NSDK_LOG_INFO("[DIAG-ALARM] User-%p ai object parsed: cmd=0x%llX, alarmType=0x%X, channel=%u, object=%s, "
                          "objType=%u, timestamp=%lld, rect=[%d,%d,%d,%d], panoramaLen=%u, imgLen=%u, "
                          "jsonHasPanoramaB64=%d, jsonHasImgDataB64=%d",
                          userHandle_,
                          lCommand,
                          info->uAlarmType,
                          info->uChannel,
                          info->strObjectID,
                          info->uObjectType,
                          (long long)info->llTimestampMs,
                          info->nLeft,
                          info->nTop,
                          info->nRight,
                          info->nBottom,
                          info->uPanoramaImgLen,
                          info->uImgLen,
                          jsonBody.find("PanoramaImgBase64") != std::string::npos ? 1 : 0,
                          jsonBody.find("ImgDataBase64") != std::string::npos ? 1 : 0);
            INT32 len = (INT32)sizeof(*info);
            alarmCb_(lCommand, &alarmer, (CHAR*)info.get(), &len, alarmUserData_);
        }
        else if (alarmBase == NET_TV_ALARM_BASE_TRAFFIC)
        {
            NET_AlarmPlateInfo_S info = {0};
            SDKConvert::deal(alarmInfoObj, info, true);
            INT32 len = (INT32)sizeof(info);
            alarmCb_(lCommand, &alarmer, (CHAR*)&info, &len, alarmUserData_);
        }
        else if (alarmBase == NET_TV_ALARM_BASE_EXCEPTION)
        {
            NET_AlarmExceptionInfo_S info = {0};
            SDKConvert::deal(alarmInfoObj, info, true);
            INT32 len = (INT32)sizeof(info);
            alarmCb_(lCommand, &alarmer, (CHAR*)&info, &len, alarmUserData_);
        }
        else if (alarmBase == NET_TV_ALARM_BASE_STATISTICS)
        {
            auto info = std::make_unique<NET_AlarmStatisticsInfo_S>();
            SDKConvert::deal(alarmInfoObj, *info, true);
            INT32 len = (INT32)sizeof(*info);
            alarmCb_(lCommand, &alarmer, (CHAR*)info.get(), &len, alarmUserData_);
        }
        else if (lCommand == NET_TV_NOTICE_DOWNLOAD_RECORD_PROGRESS)
        {
            NET_RecordDownloadProgress_S info = {0};
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
            client_->set_read_timeout(75);
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

        bool bGot401 = false;  // 标记是否收到 401 响应
        auto res = localClient->Get(url.c_str(), [&](const httplib::Response& response)
        {
            NSDK_LOG_INFO("[DIAG-ALARM] User-%p [RESPONSE] status=%d, ct=%s",
                          userHandle_, response.status,
                          response.get_header_value("Content-Type").c_str());
            if (response.status == 401) {
                bGot401 = true;  // 记录 401，后续将触发重新登录
                return false;    // 不读取响应体，httplib 会设置 err=Canceled(7)
            }
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
            while (true)
            {
                size_t boundPos = buffer.find(boundary);
                if (boundPos == std::string::npos)
                {
                    if (buffer.size() > boundary.size())
                    {
                        buffer.erase(0, buffer.size() - boundary.size());
                    }
                    break;
                }

                if (boundPos > 0)
                {
                    buffer.erase(0, boundPos);
                    continue;
                }

                if (buffer.size() < boundary.size() + 2)
                {
                    break;
                }

                if (buffer.compare(boundary.size(), 2, "--") == 0)
                {
                    buffer.erase(0, boundary.size() + 2);
                    break;
                }

                if (buffer.compare(boundary.size(), 2, "\r\n") != 0)
                {
                    buffer.erase(0, boundary.size());
                    continue;
                }

                const size_t headersStart = boundary.size() + 2;
                const size_t headerEnd = buffer.find("\r\n\r\n", headersStart);
                if (headerEnd == std::string::npos)
                {
                    break;
                }

                std::string headers = buffer.substr(headersStart, headerEnd - headersStart);
                const size_t bodyStart = headerEnd + 4;
                size_t contentLength = 0;
                size_t consumeEnd = std::string::npos;
                std::string body;

                if (ParseContentLength(headers, contentLength))
                {
                    if (contentLength > buffer.size() - bodyStart)
                    {
                        break;
                    }

                    body = buffer.substr(bodyStart, contentLength);
                    consumeEnd = bodyStart + contentLength;
                    if (buffer.size() >= consumeEnd + 2 && buffer.compare(consumeEnd, 2, "\r\n") == 0)
                    {
                        consumeEnd += 2;
                    }
                }
                else
                {
                    size_t nextBoundPos = buffer.find(boundary, bodyStart);
                    if (nextBoundPos == std::string::npos)
                    {
                        if (!ContainsNoCase(headers, "application/json"))
                        {
                            break;
                        }

                        body = buffer.substr(bodyStart);
                        TrimTrailingCrlf(body);
                        if (!IsCompleteJsonBody(body))
                        {
                            break;
                        }
                        consumeEnd = buffer.size();
                    }
                    else
                    {
                        body = buffer.substr(bodyStart, nextBoundPos - bodyStart);
                        TrimTrailingCrlf(body);
                        consumeEnd = nextBoundPos;
                    }
                }

             /* JSON数据 */
                if (ContainsNoCase(headers, "application/json"))
                {
                    if (!dispatch_alarm(body)) {
                        // 回调要求当前 AlarmListen 断开。
                        return false;
                    }
                }
                /* 图片数据 */
                else if (ContainsNoCase(headers, "image"))
                {
                    // 忽略：不依赖 multipart 附件
                }

                if (consumeEnd == std::string::npos)
                {
                    break;
                }
                buffer.erase(0, consumeEnd);
            }
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
            
            // 读取当前 session 信息
            std::string currentSessionIdLog;
            {
                std::lock_guard<std::mutex> lk(sessionIdMutex_);
                currentSessionIdLog = sessionId_;
            }

            // err=7(Canceled) 且之前收到过 401，识别为 session 过期，触发重新登录
            if (bGot401) {
                NSDK_LOG_ERROR("[DIAG-ALARM] User-%p [DISCONNECT] 401 Unauthorized (Canceled): "
                              "conn=%llds, hb=%d, alarms=%d, session=%s, triggering re-login",
                              userHandle_, (long long)connDuration,
                              heartbeatRecvCount_.load(), alarmRecvCount_.load(), currentSessionIdLog.c_str());
                if (sessionExpiredCb_) sessionExpiredCb_();
                break; // 退出循环，由上层重新登录后重启 AlarmListen
            }

            NSDK_LOG_WARN("[DIAG-ALARM] User-%p [DISCONNECT] No response (err=%d): "
                          "conn=%llds, total=%llds, hb=%d, alarms=%d, session=%s, reconnecting...",
                          userHandle_, (int)err, (long long)connDuration, (long long)totalElapsed,
                          heartbeatRecvCount_.load(), alarmRecvCount_.load(), currentSessionIdLog.c_str());
        }

        // 清空缓冲区，避免脏数据
        buffer.clear();
        boundary.clear();

        // 统一等 0.5 秒再重连
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    m_alarmLoopExited = true;
    NSDK_LOG_INFO("[DIAG-ALARM] User-%p AlarmLoop EXIT: totalLoops=%d, totalReconnects=%d",
                  userHandle_, loopCount, m_reconnectCount.load());
}

/**
 * @brief 健康监控线程
 * @details 定时检查AlarmLoop是否收到数据，检测连接假死（read_timeout失效导致永久阻塞），
 *          超过阈值时强制中断并通知上层触发重新登录
 */
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
                          "Requesting AlarmListen reconnect with the current session.",
                          userHandle_, (long long)elapsedSec, (long long)timeoutSec,
                          (int)m_firstDataReceived.load());

            // 仅中断当前 AlarmListen。AlarmLoop 的外层循环会使用同一 sessionId_
            // 重建长连接；无数据不等于登录 session 已失效，不能触发 Basic/Login。
            {
                std::lock_guard<std::mutex> lk(clientMutex_);
                if (client_) client_->stop();
            }

            // 防止当前 Get() 尚未退出时每个监控周期重复触发恢复。
            // 若 stop() 生效，AlarmLoop 很快会进入下一轮连接并刷新该时间戳。
            m_lastDataTimeMs.store(nowMs);
        }
    }
    NSDK_LOG_INFO("[DIAG-ALARM] User-%p HealthMonitor EXIT", userHandle_);
}
