/**
 * @FilePath     : mqtt_manager.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-21 10:39:50
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-29 14:24:35
 * @Description  : MQTT 管理器，提供异步双向通信能力
 */

#pragma once

#include "Singleton.h"
#include "IpcRet.h"
#include "network_define.h"
#include "system_define.h"
#include "mqtt_topic_define.h"
#include "mqtt_message.h"

#include <string>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <functional>
#include <utility>

extern "C"
{
#include "bl_mqtt.h"
}

/**
 * @brief 原始消息回调类型
 * @note  通用层回调，将 Topic 和 Payload 透传给业务层
 * @param  {const std::string &} strTopic：消息 Topic
 * @param  {const std::string &} strPayload：消息内容（JSON 字符串）
 */
using MqttRawMessageCallback = std::function<void(const std::string &strTopic, const std::string &strPayload)>;

/**
 * @brief 连接状态变化回调类型
 * @param  {bool} bConnected：true=已连接, false=已断开
 * @param  {const std::string &} strReason：状态变化原因
 */
using MqttConnectionCallback = std::function<void(bool bConnected, const std::string &strReason)>;

/**
 * @brief MQTT 管理器类
 * @note  单例模式，提供异步 MQTT 通信能力
 *        · 内部自动重连，对上层透明
 *        · 所有发布接口均为异步，立即返回不阻塞
 *        · 连接/重连状态通过日志输出
 */
class CMqttManager : public CSingleton<CMqttManager>
{
    CMqttManager() = default;

public:
    ~CMqttManager();
    friend class CSingleton<CMqttManager>;

    /**
     * @brief   : 初始化 MQTT 连接
     * @param    {const std::string &} strBroker：Broker 地址
     * @param    {int} nPort：Broker 端口
     * @param    {const std::string &} strUsername：用户名
     * @param    {const std::string &} strPassword：密码
     * @param    {const std::string &} strClientId：客户端标识（通常为设备SN）
     * @return   {int} OK：初始化成功（异步连接开始），ERR_PARAM_NULL/ERR_PARAM：参数错误
     * @note    :
     *        · 本函数立即返回，MQTT 连接在后台线程异步建立
     *        · 连接成功/失败通过日志输出（dlog_info/dlog_warn/dlog_error）
     *        · 若连接失败，内部自动重连，无需上层干预
     */
    int init(const std::string &strBroker,
             int nPort,
             const std::string &strUsername,
             const std::string &strPassword,
             const std::string &strClientId);

    /**
     * @brief   : 反初始化 MQTT 连接
     * @note    : 断开连接并释放资源，停止重连线程
     */
    void deinit();

    /**
     * @brief   : 异步发布消息到指定 Topic
     * @param    {const std::string &} strTopic：目标 Topic
     * @param    {const std::string &} strPayload：消息内容（JSON 字符串）
     * @param    {int} nQos：消息质量等级（默认 MQTT_QOS_EVENT = 0）
     * @return   {int} OK：已加入发送队列（异步），ERR/ERR_PARAM：失败（未连接/参数错误）
     * @note    :
     *        · 本函数立即返回，不等待 MQTT Broker 确认
     *        · 若当前未连接，返回错误，由上层决定重试策略
     */
    int publish(const std::string &strTopic, const std::string &strPayload, int nQos = MQTT_QOS_EVENT);

    /**
     * @brief   : 订阅指定 Topic
     * @param    {const std::string &} strTopic：要订阅的 Topic
     * @param    {int} nQos：消息质量等级（默认 MQTT_QOS_COMMAND = 1）
     * @return   {int} OK：订阅请求已发送（异步），ERR/ERR_PARAM：失败
     * @note    :
     *        · 需在连接成功后调用，内部自动在重连后重新订阅
     *        · 订阅结果通过日志输出
     */
    int subscribe(const std::string &strTopic, int nQos = MQTT_QOS_COMMAND);

    /**
     * @brief   : 查询当前连接状态
     * @return   {bool} true：已连接，false：未连接/连接中
     */
    bool is_connected() const;

    /**
     * @brief   : 设置原始消息回调
     * @param    {MqttRawMessageCallback} callback：回调函数
     * @note    : 收到消息时，直接将 Topic 和 Payload 透传给回调
     */
    void set_message_callback(MqttRawMessageCallback callback);

    /**
     * @brief   : 设置连接状态变化回调
     * @param    {MqttConnectionCallback} callback：回调函数
     * @note    : 连接成功或断开时触发，用于上层感知连接状态
     */
    void set_connection_callback(MqttConnectionCallback callback);

    /**
     * @brief   : 设置 LWT 遗嘱消息
     * @param    {const std::string &} strWillTopic：遗嘱 Topic
     * @param    {const std::string &} strWillPayload：遗嘱消息内容（JSON）
     * @param    {int} nWillQos：遗嘱 QoS 等级（默认 1）
     * @param    {bool} bWillRetain：遗嘱是否 retain（默认 true）
     * @note    : 需在 init() 之前调用；设置后，设备异常断开时 Broker 自动发布此消息
     */
    void set_will_message(const std::string &strWillTopic,
                          const std::string &strWillPayload,
                          int nWillQos = 1,
                          bool bWillRetain = true);

private:
    /* C 回调包装函数需要访问私有成员 */
    friend int mqtt_callback_wrapper(BlMqttMsg_S stMsg);

    /**
     * @brief   : MQTT 底层消息回调
     * @param    {BlMqttMsg_S} stMsg：MQTT 消息
     * @return   {void}
     * @note    : 由 bl_mqtt 库异步调用；仅更新连接状态、复制消息并唤醒工作线程，禁止执行上层业务
     */
    void on_mqtt_message(BlMqttMsg_S stMsg);

    /**
     * @brief   : 自动重连守护线程
     * @note    :
     *        · 独立线程运行，检测连接状态
     *        · 断开时按指数退避策略重试连接
     *        · 连接成功后恢复订阅并派发上层状态回调
     * @return   {void}
     */
    void reconnect_thread();

    /**
     * @brief   : 处理连接成功事件
     * @return   {void}
     * @note    : 仅记录状态和投递恢复订阅任务，避免 MQTT SDK 回调与连接锁重入
     */
    void handle_connect_success();

    /**
     * @brief   : 处理断开连接事件
     * @param    {const std::string &} strReason：断开原因
     * @return   {void}
     * @note    : 输出 warn 日志，触发重连流程
     */
    void handle_disconnect(const std::string &strReason);

    /**
     * @brief   : 执行 MQTT 连接
     * @return   {bool} true：连接成功，false：连接失败
     * @note    : 内部使用，建立与 MQTT Broker 的连接
     */
    bool do_connect();

    /**
     * @brief   : 恢复所有已订阅的 Topic
     * @return   {void}
     * @note    : 在连接成功后调用，重新订阅之前订阅过的 Topic
     */
    void restore_subscriptions();

    /**
     * @brief   : 在重连线程中派发已缓存的连接状态变化
     * @return   {void}
     * @note    : MQTT SDK 回调线程只更新状态和入队，禁止在回调上下文执行上层发布或业务逻辑
     */
    void dispatch_connection_state_changes();

    /**
     * @brief   : 缓存一条 MQTT 连接状态变化事件
     * @param    {bool} bConnected：true：连接成功，false：连接断开
     * @param    {const std::string &} strReason：状态变化原因
     * @return   {void}
     * @note    : 由 MQTT SDK 回调线程调用，事件将在重连线程中通知上层
     */
    void enqueue_connection_state_change(bool bConnected, const std::string &strReason);

    /**
     * @brief   : 唤醒重连守护线程
     * @return   {void}
     * @note    : 用于连接状态变化、停止和首次初始化，替代固定周期轮询
     */
    void notify_reconnect_thread();

private:
    BlMqtt_S *m_pstMqtt = nullptr;                              /* bl_mqtt 句柄 */
    std::string m_strClientId;                                  /* 客户端标识，用于拼接 Topic */
    std::string m_strBroker;                                    /* Broker 地址 */
    int m_nPort = MQTT_DEFAULT_PORT;                            /* Broker 端口 */
    std::string m_strUsername;                                  /* 用户名 */
    std::string m_strPassword;                                  /* 密码 */
    MqttRawMessageCallback m_fnMessageCallback;                 /* 原始消息回调 */
    MqttConnectionCallback m_fnConnectionCallback;              /* 连接状态回调 */

    /**
     * @brief   : 由 MQTT SDK 回调投递、由重连线程消费的连接状态事件
     * @note    : 事件队列将回调与平台业务隔离；队列满时仅保留较新的状态变化
     */
    struct ConnectionStateEvent_S
    {
        /* true 表示连接建立，false 表示连接丢失或建立失败。 */
        bool bConnected;
        /* 状态变化原因；在线程间传递时由本结构体独占字符串副本。 */
        std::string strReason;
    };

    /* LWT 遗嘱消息配置 */
    std::string m_strWillTopic;   /* LWT Topic（空表示不启用） */
    std::string m_strWillPayload; /* LWT 消息内容 */
    int m_nWillQos = 1;           /* LWT QoS */
    bool m_bWillRetain = true;    /* LWT retain */

    std::thread m_ReconnectThread;               /* 重连守护线程 */
    std::atomic<bool> m_bRunning{ false };       /* 线程运行标志 */
    std::atomic<bool> m_bConnected{ false };     /* 连接状态标志 */
    std::atomic<bool> m_bNeedReconnect{ false }; /* 需要重连标志 */
    std::atomic<bool> m_bConnecting{ false };    /* 连接中标志 */
    /* SDK 回调置位、重连线程 exchange 并清零；避免在回调线程内执行 subscribe。 */
    std::atomic<bool> m_bNeedRestoreSubscriptions{ false };
    std::mutex m_mtxConnect; /* 连接互斥锁 */

    /* 已订阅 Topic 列表（用于重连后恢复订阅） */
    std::vector<std::pair<std::string, int>> m_vecSubscribedTopics;
    std::mutex m_mtxTopics;          /* Topic 列表锁 */
    std::mutex m_mtxMessageCallback; /* 消息回调锁 */
    /* lock: 保护 m_fnConnectionCallback 的注册、替换和派发前快照。 */
    std::mutex m_mtxConnectionCallback;
    /* memory: SDK 回调写入、重连线程取出的有限状态事件；元素包含独立原因字符串副本。 */
    std::deque<ConnectionStateEvent_S> m_deqConnectionStateEvents;
    /* lock: 保护连接状态事件的入队、出队和 deinit 清理，回调函数必须在锁外执行。 */
    std::mutex m_mtxConnectionStateEvents;
    /* lock: 仅配合 m_cvReconnect 等待状态变化；不保护 MQTT 句柄或业务回调。 */
    std::mutex m_mtxReconnectWait;
    /* 连接结果、断连和反初始化均通过此条件变量唤醒重连线程，替代每秒轮询。 */
    std::condition_variable m_cvReconnect;
    /* 未连接时被拒绝的发布累计值；用于低成本汇总日志，不保存消息内容。 */
    std::atomic<uint64_t> m_uPublishRejectCount{ 0 };
    /* 使用 steady_clock 毫秒时间基的最近一次拒绝发布日志时间，CAS 保证并发限频正确。 */
    std::atomic<int64_t> m_nLastPublishRejectLogMs{ 0 };

    /* 重连退避参数 */
    static constexpr int RECONNECT_INITIAL_INTERVAL_SEC = 5; /* 初始重连间隔：5 秒 */
    static constexpr int RECONNECT_MAX_INTERVAL_SEC = 300;   /* 最大重连间隔：5 分钟 */
    /* 单次异步 CONNECT 等待上限，单位秒；超时后进入退避重试。 */
    static constexpr int CONNECT_RESULT_TIMEOUT_SEC = 35;
    /* 连接状态事件最大缓存量；溢出时淘汰最早事件以限制抖动场景下的内存。 */
    static constexpr size_t CONNECTION_EVENT_QUEUE_MAX_SIZE = 8;
    /* 未连接发布拒绝日志最小间隔，单位毫秒。 */
    static constexpr int64_t PUBLISH_REJECT_LOG_INTERVAL_MS = 5000;
    /* SDK 回调和重连线程共享的当前退避次数；必须使用原子读写。 */
    std::atomic<int> m_nReconnectCount{0};
};
