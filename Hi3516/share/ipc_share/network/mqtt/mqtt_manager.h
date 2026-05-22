/**
 * @FilePath     : mqtt_manager.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-21 10:39:50
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-21 14:24:40
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
#include <unordered_map>
#include <vector>
#include <mutex>
#include <functional>

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

private:
    /* C 回调包装函数需要访问私有成员 */
    friend int mqtt_callback_wrapper(BlMqttMsg_S stMsg);

    /**
     * @brief   : MQTT 底层消息回调
     * @param    {BlMqttMsg_S} stMsg：MQTT 消息
     * @note    : 由 bl_mqtt 库异步调用，处理连接/断开/消息到达事件
     */
    void on_mqtt_message(BlMqttMsg_S stMsg);

    /**
     * @brief   : 自动重连守护线程
     * @note    :
     *        · 独立线程运行，检测连接状态
     *        · 断开时按指数退避策略重试连接
     *        · 连接成功后自动恢复订阅
     */
    void reconnect_thread();

    /**
     * @brief   : 处理连接成功事件
     * @note    : 输出 info 日志，恢复订阅
     */
    void handle_connect_success();

    /**
     * @brief   : 处理断开连接事件
     * @param    {const std::string &} strReason：断开原因
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
     * @note    : 在连接成功后调用，重新订阅之前订阅过的 Topic
     */
    void restore_subscriptions();

private:
    BlMqtt_S *m_pstMqtt = nullptr;                              /* bl_mqtt 句柄 */
    std::string m_strClientId;                                  /* 客户端标识，用于拼接 Topic */
    std::string m_strBroker;                                    /* Broker 地址 */
    int m_nPort = MQTT_DEFAULT_PORT;                            /* Broker 端口 */
    std::string m_strUsername;                                  /* 用户名 */
    std::string m_strPassword;                                  /* 密码 */
    MqttRawMessageCallback m_fnMessageCallback;                 /* 原始消息回调 */

    std::thread m_ReconnectThread;                              /* 重连守护线程 */
    std::atomic<bool> m_bRunning{false};                        /* 线程运行标志 */
    std::atomic<bool> m_bConnected{false};                      /* 连接状态标志 */
    std::atomic<bool> m_bNeedReconnect{false};                  /* 需要重连标志 */
    std::atomic<bool> m_bConnecting{false};                     /* 连接中标志 */
    std::mutex  m_mtxConnect;                                   /* 连接互斥锁 */

    /* 已订阅 Topic 列表（用于重连后恢复订阅） */
    std::vector<std::pair<std::string, int>> m_vecSubscribedTopics;
    std::mutex m_mtxTopics;                                     /* Topic 列表锁 */

    /* 重连退避参数 */
    static constexpr int RECONNECT_INITIAL_INTERVAL_SEC = 5;    /* 初始重连间隔：5 秒 */
    static constexpr int RECONNECT_MAX_INTERVAL_SEC = 300;      /* 最大重连间隔：5 分钟 */
    int m_nReconnectCount = 0;                                  /* 当前重连次数 */
};
