/**
 * @FilePath     : mqtt_manager.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-21 10:39:50
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-21 17:18:18
 * @Description  : MQTT 管理器实现
 */

#include "mqtt_manager.h"
#include "dlog.h"
#include "IpcRet.h"
#include <cstring>

/* 静态全局实例指针，用于 C 回调转 C++ 成员函数 */
static CMqttManager *g_pstMqttManagerInstance = nullptr;

/* C 风格回调函数，转发到 CMqttManager 成员函数 */
int mqtt_callback_wrapper(BlMqttMsg_S stMsg)
{
    if (g_pstMqttManagerInstance != nullptr)
    {
        g_pstMqttManagerInstance->on_mqtt_message(stMsg);
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

    /* 保存配置信息 */
    m_strBroker = strBroker;
    m_nPort = nPort;
    m_strUsername = strUsername;
    m_strPassword = strPassword;
    m_strClientId = strClientId;

    /* 设置全局实例指针，供 C 回调使用 */
    g_pstMqttManagerInstance = this;

    /* 启动重连守护线程 */
    m_bRunning.store(true);
    m_ReconnectThread = std::thread(&CMqttManager::reconnect_thread, this);

    dlog_info("MQTT 管理器初始化完成，ClientID[%s]，Broker[%s:%d]", m_strClientId.c_str(), m_strBroker.c_str(), m_nPort);

    return OK;
}

void CMqttManager::deinit()
{
    /* 停止重连线程 */
    m_bRunning.store(false);
    m_bNeedReconnect.store(false);

    if (m_ReconnectThread.joinable())
    {
        m_ReconnectThread.join();
    }

    /* 断开 MQTT 连接 */
    if (m_pstMqtt != nullptr)
    {
        if (m_pstMqtt->bConnected == 1)
        {
            m_pstMqtt->uninit(m_pstMqtt);
        }
        bl_mqtt_release(m_pstMqtt);
        m_pstMqtt = nullptr;
    }

    m_bConnected.store(false);
    g_pstMqttManagerInstance = nullptr;

    dlog_info("MQTT 管理器已反初始化");
}

int CMqttManager::publish(const std::string &strTopic, const std::string &strPayload, int nQos)
{
    /* 检查连接状态 */
    if (!m_bConnected.load() || m_pstMqtt == nullptr)
    {
        dlog_warn("MQTT 发布失败：未连接");
        return ERR_UNINIT;
    }

    /* 参数校验 */
    if (strTopic.empty() || strPayload.empty())
    {
        dlog_error("MQTT 发布失败：Topic 或 Payload 为空");
        return ERR_PARAM_NULL;
    }

    /* 异步发布消息 */
    char *pTopic = const_cast<char *>(strTopic.c_str());
    char *pPayload = const_cast<char *>(strPayload.c_str());

    int nRet = m_pstMqtt->publish(m_pstMqtt, pTopic, pPayload, static_cast<int>(strPayload.length()), nQos);

    if (nRet == 0)
    {
        dlog_debug("MQTT 发布成功，Topic[%s]，QoS[%d]，长度[%zu]", strTopic.c_str(), nQos, strPayload.length());
    }
    else
    {
        dlog_error("MQTT 发布失败，Topic[%s]，返回值[%d]", strTopic.c_str(), nRet);
    }

    return nRet;
}

int CMqttManager::subscribe(const std::string &strTopic, int nQos)
{
    /* 检查连接状态 */
    if (!m_bConnected.load() || m_pstMqtt == nullptr)
    {
        dlog_warn("MQTT 订阅失败：未连接，Topic[%s] 将在连接后自动订阅", strTopic.c_str());
        /* 记录到待订阅列表，连接成功后自动订阅 */
        std::lock_guard<std::mutex> lock(m_mtxTopics);
        m_vecSubscribedTopics.emplace_back(strTopic, nQos);
        return ERR_UNINIT;
    }

    /* 参数校验 */
    if (strTopic.empty())
    {
        dlog_error("MQTT 订阅失败：Topic 为空");
        return ERR_PARAM_NULL;
    }

    /* 执行订阅 */
    char *pTopic = const_cast<char *>(strTopic.c_str());
    int nRet = m_pstMqtt->subscribe(m_pstMqtt, pTopic, nQos);

    if (nRet == 0)
    {
        dlog_info("MQTT 订阅成功，Topic[%s]，QoS[%d]", strTopic.c_str(), nQos);

        /* 记录到已订阅列表 */
        std::lock_guard<std::mutex> lock(m_mtxTopics);
        m_vecSubscribedTopics.emplace_back(strTopic, nQos);
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
    m_fnMessageCallback = callback;
    dlog_info("MQTT 消息回调已设置");
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
        if (stMsg.pMsg != nullptr && stMsg.nMsgLen > 0)
        {
            std::string strTopic(stMsg.pTopicName ? stMsg.pTopicName : "");
            std::string strPayload(stMsg.pMsg, stMsg.nMsgLen);

            dlog_debug("MQTT 收到消息，Topic[%s]，长度[%d]", strTopic.c_str(), stMsg.nMsgLen);

            /* 透传给业务层回调 */
            if (m_fnMessageCallback)
            {
                m_fnMessageCallback(strTopic, strPayload);
            }
        }
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
        /* 如果已连接，等待一段时间后检查 */
        if (m_bConnected.load())
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        /* 如果需要重连或首次连接（且当前未在连接中） */
        if ((m_bNeedReconnect.load() || m_pstMqtt == nullptr) && !m_bConnecting.load())
        {
            /* 计算退避时间 */
            int nInterval = RECONNECT_INITIAL_INTERVAL_SEC * (1 << m_nReconnectCount);
            if (nInterval > RECONNECT_MAX_INTERVAL_SEC)
            {
                nInterval = RECONNECT_MAX_INTERVAL_SEC;
            }

            dlog_info("MQTT 第[%d]次重连尝试，等待[%d]秒", m_nReconnectCount + 1, nInterval);

            std::this_thread::sleep_for(std::chrono::seconds(nInterval));

            if (!m_bRunning.load())
            {
                break;
            }

            /* 执行连接 */
            m_bConnecting.store(true);
            if (do_connect())
            {
                /* 连接请求已发送，等待回调确认 */
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
            else
            {
                /* 连接失败，增加重连计数 */
                m_bConnecting.store(false);
                m_nReconnectCount++;
                dlog_warn("MQTT 连接失败，将在[%d]秒后重试", RECONNECT_INITIAL_INTERVAL_SEC * (1 << m_nReconnectCount));
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    dlog_info("MQTT 重连守护线程已退出");
}

void CMqttManager::handle_connect_success()
{
    m_bConnecting.store(false);
    m_bConnected.store(true);
    m_nReconnectCount = 0;
    m_bNeedReconnect.store(false);
    dlog_info("MQTT 连接成功");

    /* 恢复订阅 */
    restore_subscriptions();
}

void CMqttManager::handle_disconnect(const std::string &strReason)
{
    m_bConnecting.store(false);
    m_bConnected.store(false);
    dlog_warn("MQTT 连接断开：%s", strReason.c_str());

    /* 标记需要重连 */
    m_bNeedReconnect.store(true);
}

bool CMqttManager::do_connect()
{
    /* 互斥锁保护，防止并发调用 */
    std::lock_guard<std::mutex> lock(m_mtxConnect);

    /* 如果已有连接，先释放 */
    if (m_pstMqtt != nullptr)
    {
        if (m_pstMqtt->bConnected == 1)
        {
            m_pstMqtt->uninit(m_pstMqtt);
        }
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
    std::lock_guard<std::mutex> lock(m_mtxTopics);

    if (m_vecSubscribedTopics.empty())
    {
        return;
    }

    dlog_info("MQTT 恢复[%zu]个订阅", m_vecSubscribedTopics.size());

    for (const auto &stPair : m_vecSubscribedTopics)
    {
        const std::string &strTopic = stPair.first;
        int nQos = stPair.second;

        if (m_pstMqtt != nullptr && m_pstMqtt->bConnected == 1)
        {
            char *pTopic = const_cast<char *>(strTopic.c_str());
            int nRet = m_pstMqtt->subscribe(m_pstMqtt, pTopic, nQos);

            if (nRet == 0)
            {
                dlog_info("MQTT 恢复订阅成功，Topic[%s]，QoS[%d]", strTopic.c_str(), nQos);
            }
            else
            {
                dlog_error("MQTT 恢复订阅失败，Topic[%s]，返回值[%d]", strTopic.c_str(), nRet);
            }
        }
    }
}
