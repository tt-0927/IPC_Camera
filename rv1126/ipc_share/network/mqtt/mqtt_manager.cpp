/**
 * @FilePath     : mqtt_manager.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-21 10:39:50
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-29 14:24:27
 * @Description  : MQTT 管理器实现
 */

#include "mqtt_manager.h"
#include "dlog.h"
#include "IpcRet.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <utility>

/* lock: C 回调与反初始化可能并发发生，使用原子指针避免读写全局实例指针的数据竞争。 */
static std::atomic<CMqttManager *> g_pstMqttManagerInstance{ nullptr };

/**
 * @brief   : 判断限频日志是否允许在当前时刻输出
 * @param    {std::atomic<int64_t> &} nLastLogMs：最近一次日志时间，使用 steady_clock 毫秒时间基
 * @param    {int64_t} nIntervalMs：两条日志之间允许的最小间隔，单位毫秒
 * @param    {int64_t} nNowMs：本次判断的 steady_clock 毫秒时间
 * @return   {bool} true：当前线程获得输出资格，false：仍处于限频窗口
 * @note   : 使用 CAS 保证多个发布线程同时失败时只产生一条汇总日志
 */
static bool should_log_rate_limited(std::atomic<int64_t> &nLastLogMs, int64_t nIntervalMs, int64_t nNowMs)
{
    int64_t nLastMs = nLastLogMs.load();
    while (nLastMs == 0 || nNowMs - nLastMs >= nIntervalMs)
    {
        if (nLastLogMs.compare_exchange_weak(nLastMs, nNowMs))
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief   : 将 bl_mqtt 的 C 回调转发至 MQTT 管理器
 * @param    {BlMqttMsg_S} stMsg：底层 MQTT 事件，消息缓冲区只在本次回调期间有效
 * @return   {int} 0：事件已转发或管理器已退出
 * @note    : 先原子读取管理器指针；反初始化会先置空，阻止释放后的新回调进入 C++ 对象
 */
int mqtt_callback_wrapper(BlMqttMsg_S stMsg)
{
    CMqttManager *pMqttManager = g_pstMqttManagerInstance.load();
    if (pMqttManager != nullptr)
    {
        pMqttManager->on_mqtt_message(stMsg);
    }
    return 0;
}

CMqttManager::~CMqttManager()
{
    deinit();
}

int CMqttManager::init(const std::string &strBroker,
                       int nPort,
                       const std::string &strUsername,
                       const std::string &strPassword,
                       const std::string &strClientId)
{
    /* 参数校验 */
    if (strBroker.empty())
    {
        dlog_error("MQTT 初始化失败：Broker 地址为空");
        return ERR_PARAM_NULL;
    }

    if (strClientId.empty())
    {
        dlog_error("MQTT 初始化失败：客户端标识为空");
        return ERR_PARAM_NULL;
    }

    if (m_bRunning.exchange(true))
    {
        dlog_warn("MQTT 管理器已初始化，忽略重复初始化请求");
        return OK;
    }

    /* 保存配置信息 */
    m_strBroker = strBroker;
    m_nPort = nPort;
    m_strUsername = strUsername;
    m_strPassword = strPassword;
    m_strClientId = strClientId;

    {
        /* lock: 初始化阶段清空上一轮订阅记录，避免重新配置后恢复到旧 Broker 的 Topic。 */
        std::lock_guard<std::mutex> lock(m_mtxTopics);
        m_vecSubscribedTopics.clear();
    }

    /* 设置全局实例指针，供 C 回调使用 */
    g_pstMqttManagerInstance.store(this);

    /* 启动重连守护线程 */
    m_bConnected.store(false);
    m_bNeedReconnect.store(true);
    m_bConnecting.store(false);
    m_bNeedRestoreSubscriptions.store(false);
    m_nReconnectCount.store(0);
    m_uPublishRejectCount.store(0);
    m_nLastPublishRejectLogMs.store(0);
    {
        /* lock: 清除上一生命周期未派发的状态事件，避免新连接收到旧断连通知。 */
        std::lock_guard<std::mutex> lock(m_mtxConnectionStateEvents);
        m_deqConnectionStateEvents.clear();
    }
    m_ReconnectThread = std::thread(&CMqttManager::reconnect_thread, this);

    dlog_info("MQTT 管理器初始化完成，ClientID[%s]，Broker[%s:%d]", m_strClientId.c_str(), m_strBroker.c_str(), m_nPort);

    return OK;
}

void CMqttManager::deinit()
{
    /* 停止重连线程 */
    m_bRunning.store(false);
    m_bNeedReconnect.store(false);
    m_bConnecting.store(false);
    m_bNeedRestoreSubscriptions.store(false);
    notify_reconnect_thread();

    if (m_ReconnectThread.joinable())
    {
        m_ReconnectThread.join();
    }

    /* lock: 先阻止新的 C 回调进入，再销毁底层句柄，避免释放后继续访问管理器状态。 */
    g_pstMqttManagerInstance.store(nullptr);

    /* lock: 与 publish()/do_connect() 共用句柄锁，避免释放期间仍有异步发送。 */
    {
        std::lock_guard<std::mutex> lock(m_mtxConnect);
        m_bConnected.store(false);
        if (m_pstMqtt != nullptr)
        {
            m_pstMqtt->uninit(m_pstMqtt);
            bl_mqtt_release(m_pstMqtt);
            m_pstMqtt = nullptr;
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_mtxTopics);
        m_vecSubscribedTopics.clear();
    }
    {
        /* lock: deinit 期间不允许残留连接事件在下一次 init 后被派发。 */
        std::lock_guard<std::mutex> lock(m_mtxConnectionStateEvents);
        m_deqConnectionStateEvents.clear();
    }
    dlog_info("MQTT 管理器已反初始化");
}

int CMqttManager::publish(const std::string &strTopic, const std::string &strPayload, int nQos)
{
    /* 参数校验不依赖底层句柄，可在加锁前完成。 */
    if (strTopic.empty() || strPayload.empty())
    {
        dlog_error("MQTT 发布失败：Topic 或 Payload 为空");
        return ERR_PARAM_NULL;
    }

    /* 发送与重连、反初始化互斥，防止使用已释放的 m_pstMqtt。 */
    std::lock_guard<std::mutex> lock(m_mtxConnect);

    /* 检查连接状态 */
    if (!m_bConnected.load() || m_pstMqtt == nullptr)
    {
        /* 仅保留计数，不缓存离线期间的 Payload；避免断网时无界占用内存和发送过期事件。 */
        const uint64_t uRejectCount = m_uPublishRejectCount.fetch_add(1) + 1;
        const int64_t nNowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now().time_since_epoch())
                                   .count();
        if (should_log_rate_limited(m_nLastPublishRejectLogMs, PUBLISH_REJECT_LOG_INTERVAL_MS, nNowMs))
        {
            const uint64_t uRejectedSinceLastLog = m_uPublishRejectCount.exchange(0);
            dlog_warn("MQTT 未连接，拒绝发布消息，最近%llu条（本次序号=%llu）",
                      static_cast<unsigned long long>(uRejectedSinceLastLog),
                      static_cast<unsigned long long>(uRejectCount));
        }
        return ERR_UNINIT;
    }

    /* 异步发布消息 */
    char *pTopic = const_cast<char *>(strTopic.c_str());
    char *pPayload = const_cast<char *>(strPayload.c_str());

    int nRet = m_pstMqtt->publish(m_pstMqtt, pTopic, pPayload, static_cast<int>(strPayload.length()), nQos);

    if (nRet != 0)
    {
        dlog_error("MQTT 发布失败，Topic[%s]，返回值[%d]", strTopic.c_str(), nRet);
    }

    return nRet;
}

int CMqttManager::subscribe(const std::string &strTopic, int nQos)
{
    /* 参数校验 */
    if (strTopic.empty())
    {
        dlog_error("MQTT 订阅失败：Topic 为空");
        return ERR_PARAM_NULL;
    }

    if (strTopic.find("/faces") != std::string::npos)
    {
        dlog_warn("MQTT 忽略废弃订阅Topic[%s]", strTopic.c_str());
        return OK;
    }

    /* 无论当前是否已连接，均先登记订阅，保证重连后可恢复。 */
    {
        std::lock_guard<std::mutex> lock(m_mtxTopics);
        if (std::find_if(m_vecSubscribedTopics.begin(), m_vecSubscribedTopics.end(),
                         [&strTopic](const std::pair<std::string, int> &item) {
                             return item.first == strTopic;
                         }) == m_vecSubscribedTopics.end())
        {
            m_vecSubscribedTopics.emplace_back(strTopic, nQos);
        }
    }

    /* lock: 与do_connect()/deinit()串行，防止订阅时使用已释放的底层句柄。 */
    std::lock_guard<std::mutex> lockConnect(m_mtxConnect);
    if (!m_bConnected.load() || m_pstMqtt == nullptr)
    {
        dlog_warn("MQTT 订阅延后：未连接，Topic[%s] 将在连接后自动订阅", strTopic.c_str());
        return ERR_UNINIT;
    }

    /* 执行订阅 */
    char *pTopic = const_cast<char *>(strTopic.c_str());
    int nRet = m_pstMqtt->subscribe(m_pstMqtt, pTopic, nQos);

    if (nRet == 0)
    {
        dlog_info("MQTT 订阅成功，Topic[%s]，QoS[%d]", strTopic.c_str(), nQos);

    }
    else
    {
        dlog_error("MQTT 订阅失败，Topic[%s]，返回值[%d]", strTopic.c_str(), nRet);
    }

    return nRet;
}

bool CMqttManager::is_connected() const
{
    return m_bConnected.load();
}

void CMqttManager::set_message_callback(MqttRawMessageCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mtxMessageCallback);
    m_fnMessageCallback = callback;
    dlog_info("MQTT 消息回调已设置");
}

void CMqttManager::set_connection_callback(MqttConnectionCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mtxConnectionCallback);
    m_fnConnectionCallback = callback;
    dlog_info("MQTT 连接状态回调已设置");
}

void CMqttManager::set_will_message(const std::string &strWillTopic,
                                    const std::string &strWillPayload,
                                    int nWillQos,
                                    bool bWillRetain)
{
    m_strWillTopic = strWillTopic;
    m_strWillPayload = strWillPayload;
    m_nWillQos = nWillQos;
    m_bWillRetain = bWillRetain;
    dlog_info("MQTT LWT 遗嘱已设置，Topic[%s]，QoS[%d]，Retain[%d]",
              strWillTopic.c_str(), nWillQos, bWillRetain ? 1 : 0);
}

void CMqttManager::on_mqtt_message(BlMqttMsg_S stMsg)
{
    switch (stMsg.enMsgType)
    {
    case BL_MQTT_MSG_CONNECT_SUCCESS:
    {
        handle_connect_success();
        break;
    }
    case BL_MQTT_MSG_CONNECT_FAILURE:
    {
        handle_disconnect("连接失败");
        break;
    }
    case BL_MQTT_MSG_DISCONNECT_SUCCESS:
    {
        handle_disconnect("连接断开");
        break;
    }
    case BL_MQTT_MSG_DISCONNECT_FAILURE:
    {
        handle_disconnect("断开失败");
        break;
    }
    case BL_MQTT_MSG_TOPIC:
    {
        /* 收到订阅主题的消息 */
        if (stMsg.pTopicName != nullptr)
        {
            /* memory: bl_mqtt 在本函数返回后释放 pTopicName/pMsg，必须先复制到 C++ 字符串。 */
            std::string strTopic(stMsg.pTopicName ? stMsg.pTopicName : "");
            std::string strPayload;
            if (stMsg.pMsg != nullptr && stMsg.nMsgLen > 0)
            {
                strPayload.assign(stMsg.pMsg, stMsg.nMsgLen);
            }

            /* lock: 回调在锁外执行，业务层可安全注册/替换回调或继续调用 MQTT 接口。 */
            MqttRawMessageCallback fnCallback;
            {
                std::lock_guard<std::mutex> lock(m_mtxMessageCallback);
                fnCallback = m_fnMessageCallback;
            }
            if (fnCallback)
            {
                fnCallback(strTopic, strPayload);
            }
        }
        break;
    }
    case BL_MQTT_MSG_PUBLISH_SUCCESS:
    case BL_MQTT_MSG_SUBSCRIBE_SUCCESS:
    case BL_MQTT_MSG_PUBLISH_FAILURE:
    case BL_MQTT_MSG_SUBSCRIBE_FAILURE:
    {
        /* perf: 底层已记录发布/订阅的详细结果；管理层不重复逐条输出，避免异常时日志放大。 */
        break;
    }
    default:
    {
        dlog_debug("MQTT 收到未处理的消息类型[%d]", stMsg.enMsgType);
        break;
    }
    }
}

void CMqttManager::reconnect_thread()
{
    dlog_info("MQTT 重连守护线程已启动");

    while (m_bRunning.load())
    {
        if (m_bConnected.load() && m_bNeedRestoreSubscriptions.exchange(false))
        {
            /* lock: 此处位于重连线程，do_connect() 已返回；订阅操作可安全获取m_mtxConnect。 */
            restore_subscriptions();
        }
        dispatch_connection_state_changes();

        if (m_bConnected.load())
        {
            /* perf: 已连接状态不再每秒唤醒，直到断连、停止或显式状态变化才检查。 */
            std::unique_lock<std::mutex> lock(m_mtxReconnectWait);
            m_cvReconnect.wait(lock,
                               [this]()
                               {
                                   return !m_bRunning.load() || !m_bConnected.load();
                               });
            continue;
        }

        if ((m_bNeedReconnect.load() || m_pstMqtt == nullptr) && !m_bConnecting.load())
        {
            /* 读取本轮退避快照，回调线程可并发重置计数但不影响正在执行的重连等待。 */
            const int nReconnectCount = m_nReconnectCount.load();
            const bool bFirstConnect = (nReconnectCount == 0 && m_pstMqtt == nullptr);
            const int nMaxExponent = 6;
            const int nExponent = std::min(nReconnectCount, nMaxExponent);
            const int nInterval = bFirstConnect
                                      ? 0
                                      : std::min(RECONNECT_MAX_INTERVAL_SEC, RECONNECT_INITIAL_INTERVAL_SEC * (1 << nExponent));

            if (nInterval > 0)
            {
                dlog_info("MQTT 第[%d]次重连，等待[%d]秒", nReconnectCount + 1, nInterval);
                std::unique_lock<std::mutex> lock(m_mtxReconnectWait);
                m_cvReconnect.wait_for(lock,
                                       std::chrono::seconds(nInterval),
                                       [this]()
                                       {
                                           return !m_bRunning.load() || m_bConnected.load();
                                       });
                if (!m_bRunning.load() || m_bConnected.load())
                {
                    continue;
                }
            }

            dlog_info("MQTT 第[%d]次%s尝试", nReconnectCount + 1, bFirstConnect ? "连接" : "重连");
            m_bConnecting.store(true);
            if (do_connect())
            {
                /* 异步连接结果由回调唤醒；超时后才允许下一次退避重试。 */
                std::unique_lock<std::mutex> lock(m_mtxReconnectWait);
                m_cvReconnect.wait_for(lock,
                                       std::chrono::seconds(CONNECT_RESULT_TIMEOUT_SEC),
                                       [this]()
                                       {
                                           return !m_bRunning.load() || m_bConnected.load() || !m_bConnecting.load();
                                       });
                if (m_bRunning.load() && !m_bConnected.load() && m_bConnecting.load())
                {
                    m_bConnecting.store(false);
                    m_bNeedReconnect.store(true);
                    m_nReconnectCount.fetch_add(1);
                    dlog_warn("MQTT 连接等待结果超时，将继续退避重试");
                }
            }
            else
            {
                m_bConnecting.store(false);
                m_bNeedReconnect.store(true);
                m_nReconnectCount.fetch_add(1);
                dlog_warn("MQTT 连接请求失败，将进入退避重试");
            }
        }
        else
        {
            /* perf: 所有状态改变都会主动 notify，异常状态下也不再固定每秒轮询。 */
            std::unique_lock<std::mutex> lock(m_mtxReconnectWait);
            m_cvReconnect.wait(lock,
                               [this]()
                               {
                                   return !m_bRunning.load() || m_bConnected.load() ||
                                          (m_bNeedReconnect.load() && !m_bConnecting.load());
                               });
        }
    }

    dlog_info("MQTT 重连守护线程已退出");
}

void CMqttManager::handle_connect_success()
{
    m_bConnecting.store(false);
    m_bConnected.store(true);
    m_nReconnectCount.store(0);
    m_bNeedReconnect.store(false);
    m_bNeedRestoreSubscriptions.store(true);
    dlog_info("MQTT 连接成功");

    /* perf: SDK 回调线程不得直接恢复订阅或调用上层，避免与连接锁重入及业务阻塞。 */
    enqueue_connection_state_change(true, "connect");
    notify_reconnect_thread();
}

void CMqttManager::handle_disconnect(const std::string &strReason)
{
    m_bConnecting.store(false);
    m_bConnected.store(false);
    dlog_warn("MQTT 连接断开：%s", strReason.c_str());

    /* 标记需要重连 */
    m_bNeedReconnect.store(true);
    /* perf: 上层连接回调可能发布状态或切换业务，必须由重连线程执行。 */
    enqueue_connection_state_change(false, strReason);
    notify_reconnect_thread();
}

void CMqttManager::enqueue_connection_state_change(bool bConnected, const std::string &strReason)
{
    /* lock: SDK 回调仅在此锁内移动事件；不得持锁执行平台连接状态回调。 */
    std::lock_guard<std::mutex> lock(m_mtxConnectionStateEvents);
    if (m_deqConnectionStateEvents.size() >= CONNECTION_EVENT_QUEUE_MAX_SIZE)
    {
        /* warn: 仅在异常状态频繁抖动时触发；保留最新状态比保留过期状态更重要。 */
        m_deqConnectionStateEvents.pop_front();
        dlog_warn("MQTT 连接状态事件队列已满，丢弃最早事件");
    }

    /* memory: 原因字符串复制进事件，调用方返回后不依赖底层 MQTT 的错误文本。 */
    ConnectionStateEvent_S stEvent;
    stEvent.bConnected = bConnected;
    stEvent.strReason = strReason;
    m_deqConnectionStateEvents.push_back(std::move(stEvent));
}

void CMqttManager::dispatch_connection_state_changes()
{
    while (true)
    {
        ConnectionStateEvent_S stEvent;
        {
            /* lock: 只在锁内弹出事件，随后立即释放，防止平台回调反调 MQTT 时锁递归。 */
            std::lock_guard<std::mutex> lock(m_mtxConnectionStateEvents);
            if (m_deqConnectionStateEvents.empty())
            {
                return;
            }
            stEvent = std::move(m_deqConnectionStateEvents.front());
            m_deqConnectionStateEvents.pop_front();
        }

        MqttConnectionCallback fnCallback;
        {
            /* lock: 快照连接回调后在锁外调用，注册/注销回调不会被业务处理阻塞。 */
            std::lock_guard<std::mutex> lock(m_mtxConnectionCallback);
            fnCallback = m_fnConnectionCallback;
        }
        if (fnCallback)
        {
            fnCallback(stEvent.bConnected, stEvent.strReason);
        }
    }
}

void CMqttManager::notify_reconnect_thread()
{
    m_cvReconnect.notify_all();
}

bool CMqttManager::do_connect()
{
    /* 互斥锁保护，防止并发调用 */
    std::lock_guard<std::mutex> lock(m_mtxConnect);

    /* 如果已有连接，先释放 */
    if (m_pstMqtt != nullptr)
    {
        m_pstMqtt->uninit(m_pstMqtt);
        bl_mqtt_release(m_pstMqtt);
        m_pstMqtt = nullptr;
    }

    /* 配置 MQTT 参数 */
    BlMqttNeedParam_S stNeedParam;
    memset(&stNeedParam, 0, sizeof(BlMqttNeedParam_S));
    stNeedParam.pfnCallback = mqtt_callback_wrapper;

    BlMqttExParam_S stMqttInfo;
    memset(&stMqttInfo, 0, sizeof(BlMqttExParam_S));

    /* 设置客户端 ID */
    snprintf(stMqttInfo.achClientID, sizeof(stMqttInfo.achClientID), "%s", m_strClientId.c_str());

    /* 设置服务器地址和端口 */
    snprintf(stMqttInfo.achURL, sizeof(stMqttInfo.achURL), "%s", m_strBroker.c_str());
    stMqttInfo.nPort = m_nPort;

    /* 设置用户名密码 */
    snprintf(stMqttInfo.achUserName, sizeof(stMqttInfo.achUserName), "%s", m_strUsername.c_str());
    snprintf(stMqttInfo.achPassword, sizeof(stMqttInfo.achPassword), "%s", m_strPassword.c_str());

    /* 设置心跳和超时 */
    stMqttInfo.unKeepAlive = 20;      /* 20 秒心跳 */
    stMqttInfo.unConnectTimeout = 30; /* 30 秒连接超时 */
    stMqttInfo.bAutoReconnect = 0;    /* 关闭库自动重连，使用自定义重连逻辑 */

    /* 将上层设置的 LWT 写入 CONNECT 参数，异常断电/断网后由 Broker 发布离线状态。 */
    if (!m_strWillTopic.empty())
    {
        snprintf(stMqttInfo.achWillTopic, sizeof(stMqttInfo.achWillTopic), "%s", m_strWillTopic.c_str());
        snprintf(stMqttInfo.achWillMessage, sizeof(stMqttInfo.achWillMessage), "%s", m_strWillPayload.c_str());
        stMqttInfo.nWillQos = m_nWillQos;
        stMqttInfo.bWillRetain = m_bWillRetain ? 1 : 0;
    }

    /* 分配 MQTT 句柄 */
    m_pstMqtt = bl_mqtt_alloc(&stNeedParam, &stMqttInfo);
    if (m_pstMqtt == nullptr)
    {
        dlog_error("MQTT 分配句柄失败");
        return false;
    }

    /* 初始化连接 */
    int nRet = m_pstMqtt->init(m_pstMqtt);
    if (nRet != 0)
    {
        dlog_error("MQTT 初始化连接失败，返回值[%d]", nRet);
        bl_mqtt_release(m_pstMqtt);
        m_pstMqtt = nullptr;
        return false;
    }

    /* 等待连接回调确认（非阻塞，由回调更新状态） */
    dlog_debug("MQTT 连接请求已发送，等待确认...");
    return true;
}

void CMqttManager::restore_subscriptions()
{
    std::vector<std::pair<std::string, int>> vTopics;
    {
        std::lock_guard<std::mutex> lock(m_mtxTopics);
        vTopics = m_vecSubscribedTopics;
    }
    if (vTopics.empty())
    {
        return;
    }

    dlog_info("MQTT 恢复[%zu]个订阅", vTopics.size());

    for (const auto &stPair : vTopics)
    {
        const int nRet = subscribe(stPair.first, stPair.second);
        if (nRet != OK)
        {
            dlog_error("MQTT 恢复订阅失败，Topic[%s]，返回值[%d]", stPair.first.c_str(), nRet);
        }
    }
}
