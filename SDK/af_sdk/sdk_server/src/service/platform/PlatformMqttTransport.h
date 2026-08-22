/**
 * @file PlatformMqttTransport.h
 * @author Codex
 * @date 2026-08-22
 * @brief Declares the SDK-owned asynchronous MQTT transport.
 * @details The transport dynamically loads Eclipse Paho, owns reconnect and
 * subscription recovery, and isolates Paho callbacks from platform business logic.
 * @change 2026-08-22 Codex Initial implementation for platform migration.
 */
#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "MQTTAsync.h"

class CPlatformMqttApi;

/**
 * @class CPlatformMqttTransport
 * @brief Owns one Paho MQTTAsync client and its reconnect lifecycle.
 */
class CPlatformMqttTransport
{
public:
    /**
     * @struct Config_S
     * @brief Immutable connection parameters copied by Start.
     */
    struct Config_S
    {
        std::string strHost;
        int nPort{0};
        std::string strUser;
        std::string strPassword;
        std::string strClientId;
        std::string strRuntimeLibrary;
        std::string strWillTopic;
        std::string strWillPayload;
        int nWillQos{1};
        bool bWillRetain{false};
        int nKeepAliveSec{20};
        int nConnectTimeoutSec{30};
    };

    using MessageCallback_FN = std::function<void(const std::string &, const std::string &)>;
    using ConnectionCallback_FN = std::function<void(bool, const std::string &)>;

    /**
     * @brief Constructs an inactive MQTT transport.
     * @author Codex
     */
    CPlatformMqttTransport();

    /**
     * @brief Stops the transport and releases the dynamically loaded library.
     * @author Codex
     */
    ~CPlatformMqttTransport();

    CPlatformMqttTransport(const CPlatformMqttTransport &) = delete;
    CPlatformMqttTransport &operator=(const CPlatformMqttTransport &) = delete;

    /**
     * @brief Starts the reconnect worker and creates the Paho client.
     * @author Codex
     * @param [IN] stConfig Connection configuration.
     * @param [IN] fnMessageCallback Callback receiving deep-copied MQTT messages.
     * @param [IN] fnConnectionCallback Callback receiving stable connection transitions.
     * @return True when the asynchronous connection lifecycle starts successfully.
     */
    bool Start(const Config_S &stConfig,
               MessageCallback_FN fnMessageCallback,
               ConnectionCallback_FN fnConnectionCallback);

    /**
     * @brief Stops reconnect processing and destroys the Paho client.
     * @author Codex
     * @return No return value.
     */
    void Stop();

    /**
     * @brief Publishes one payload through the active MQTT connection.
     * @author Codex
     * @param [IN] strTopic Destination topic.
     * @param [IN] strPayload Payload bytes.
     * @param [IN] nQos MQTT quality of service level.
     * @return True when Paho accepts the asynchronous send request.
     */
    bool Publish(const std::string &strTopic, const std::string &strPayload, int nQos);

    /**
     * @brief Records and applies one MQTT subscription.
     * @author Codex
     * @param [IN] strTopic Topic filter.
     * @param [IN] nQos Requested quality of service level.
     * @return True when the subscription is recorded and, if connected, submitted.
     */
    bool Subscribe(const std::string &strTopic, int nQos);

    /**
     * @brief Requests client recreation after a network route or broker change.
     * @author Codex
     * @return No return value.
     */
    void RequestReconnect();

    /**
     * @brief Returns the connection state confirmed by Paho callbacks.
     * @author Codex
     * @return True only while the MQTT session is connected.
     */
    bool IsConnected() const;

    /**
     * @brief Returns the cumulative number of reconnect attempts.
     * @author Codex
     * @return Reconnect attempt counter.
     */
    std::uint64_t GetReconnectCount() const;

private:
    /**
     * @brief Paho callback invoked when an established connection is lost.
     * @author Codex
     * @param [IN] pContext Transport instance.
     * @param [IN] pCause Paho-owned diagnostic text.
     * @return No return value.
     */
    static void OnConnectionLost(void *pContext, char *pCause);

    /**
     * @brief Paho callback invoked for a subscribed message.
     * @author Codex
     * @param [IN] pContext Transport instance.
     * @param [IN] pTopicName Paho-owned topic buffer.
     * @param [IN] nTopicLength Topic length or zero for a terminated string.
     * @param [IN] pMessage Paho-owned message object.
     * @return One after the SDK consumes and frees the Paho buffers.
     */
    static int OnMessageArrived(void *pContext,
                                char *pTopicName,
                                int nTopicLength,
                                MQTTAsync_message *pMessage);

    /**
     * @brief Paho callback invoked after a successful connect request.
     * @author Codex
     * @param [IN] pContext Transport instance.
     * @param [IN] pResponse Optional Paho response.
     * @return No return value.
     */
    static void OnConnectSuccess(void *pContext, MQTTAsync_successData *pResponse);

    /**
     * @brief Paho callback invoked after a failed connect request.
     * @author Codex
     * @param [IN] pContext Transport instance.
     * @param [IN] pResponse Optional Paho failure response.
     * @return No return value.
     */
    static void OnConnectFailure(void *pContext, MQTTAsync_failureData *pResponse);

    /**
     * @brief Reconnect worker entry point.
     * @author Codex
     * @return No return value.
     */
    void ReconnectLoop();

    /**
     * @brief Creates and configures a Paho client while m_mtxApi is held.
     * @author Codex
     * @return True when the client handle is ready.
     */
    bool CreateClientLocked();

    /**
     * @brief Disconnects and destroys the Paho client while m_mtxApi is held.
     * @author Codex
     * @return No return value.
     */
    void DestroyClientLocked();

    /**
     * @brief Submits one asynchronous MQTT CONNECT request.
     * @author Codex
     * @return True when Paho accepts the request.
     */
    bool ConnectOnce();

    /**
     * @brief Restores all recorded subscriptions after reconnect.
     * @author Codex
     * @return No return value.
     */
    void RestoreSubscriptions();

    /**
     * @brief Submits one subscription without changing the subscription list.
     * @author Codex
     * @param [IN] strTopic Topic filter.
     * @param [IN] nQos Requested quality of service level.
     * @return True when Paho accepts the request.
     */
    bool SubscribeNow(const std::string &strTopic, int nQos);

    /**
     * @brief Records a connect result and wakes the reconnect worker.
     * @author Codex
     * @param [IN] bSuccess Whether the connect request succeeded.
     * @param [IN] strReason Stable diagnostic text.
     * @return No return value.
     */
    void HandleConnectResult(bool bSuccess, const std::string &strReason);

    Config_S m_stConfig;
    std::unique_ptr<CPlatformMqttApi> m_pApi;
    MQTTAsync m_pClient{nullptr};
    MessageCallback_FN m_fnMessageCallback;
    ConnectionCallback_FN m_fnConnectionCallback;
    std::thread m_stReconnectThread;
    std::atomic<bool> m_bRunning{false};
    std::atomic<bool> m_bConnected{false};
    std::atomic<bool> m_bReconnectRequested{false};
    std::atomic<bool> m_bForceRecreate{false};
    std::atomic<std::uint64_t> m_uReconnectCount{0};
    bool m_bConnectResultReady{false};
    bool m_bConnectResultSuccess{false};
    std::string m_strConnectResultReason;
    mutable std::mutex m_mtxApi;
    std::mutex m_mtxState;
    std::condition_variable m_cvState;
    std::mutex m_mtxCallbacks;
    std::condition_variable m_cvCallbacks;
    std::size_t m_uActiveMessageCallbacks{0};
    std::mutex m_mtxSubscriptions;
    std::vector<std::pair<std::string, int>> m_aSubscriptions;
};
