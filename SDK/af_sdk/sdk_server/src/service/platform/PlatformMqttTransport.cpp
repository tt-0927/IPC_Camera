/**
 * @file PlatformMqttTransport.cpp
 * @author Codex
 * @date 2026-08-22
 * @brief Implements the SDK-owned asynchronous MQTT transport.
 * @change 2026-08-22 Codex Initial implementation for platform migration.
 */

#include "PlatformMqttTransport.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <limits>

#include "NetSdkLog.h"

#if defined(_WIN32)
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

namespace
{
constexpr int PLATFORM_MQTT_INITIAL_RECONNECT_SEC = 5;
constexpr int PLATFORM_MQTT_MAX_RECONNECT_SEC = 300;
constexpr int PLATFORM_MQTT_CONNECT_RESULT_TIMEOUT_SEC = 35;
constexpr int PLATFORM_MQTT_MAX_BACKOFF_EXPONENT = 6;

/**
 * @brief Returns a bounded exponential reconnect delay.
 * @author Codex
 * @param [IN] nAttempt Consecutive failed attempt count.
 * @return Delay in seconds. The first connection attempt has no delay.
 */
static int CalculateReconnectDelay(int nAttempt)
{
    if (nAttempt <= 0)
    {
        return 0;
    }

    const int nExponent = std::min(nAttempt - 1, PLATFORM_MQTT_MAX_BACKOFF_EXPONENT);
    return std::min(PLATFORM_MQTT_MAX_RECONNECT_SEC,
                    PLATFORM_MQTT_INITIAL_RECONNECT_SEC * (1 << nExponent));
}
}

/**
 * @class CPlatformMqttApi
 * @brief Runtime loader for the Paho MQTTAsync C ABI.
 */
class CPlatformMqttApi
{
public:
    using Create_FN = int (*)(MQTTAsync *, const char *, const char *, int, void *);
    using SetCallbacks_FN = int (*)(MQTTAsync,
                                    void *,
                                    MQTTAsync_connectionLost *,
                                    MQTTAsync_messageArrived *,
                                    MQTTAsync_deliveryComplete *);
    using Connect_FN = int (*)(MQTTAsync, const MQTTAsync_connectOptions *);
    using Disconnect_FN = int (*)(MQTTAsync, const MQTTAsync_disconnectOptions *);
    using IsConnected_FN = int (*)(MQTTAsync);
    using Subscribe_FN = int (*)(MQTTAsync, const char *, int, MQTTAsync_responseOptions *);
    using Send_FN = int (*)(MQTTAsync,
                            const char *,
                            int,
                            const void *,
                            int,
                            int,
                            MQTTAsync_responseOptions *);
    using FreeMessage_FN = void (*)(MQTTAsync_message **);
    using Free_FN = void (*)(void *);
    using Destroy_FN = void (*)(MQTTAsync *);

    /**
     * @brief Constructs an unloaded Paho API table.
     * @author Codex
     */
    CPlatformMqttApi() = default;

    /**
     * @brief Unloads the Paho library after all clients are destroyed.
     * @author Codex
     */
    ~CPlatformMqttApi()
    {
        Unload();
    }

    CPlatformMqttApi(const CPlatformMqttApi &) = delete;
    CPlatformMqttApi &operator=(const CPlatformMqttApi &) = delete;

    /**
     * @brief Loads Paho and resolves every function required by the transport.
     * @author Codex
     * @param [IN] strPreferredPath Optional explicit shared-library path.
     * @return True when a complete function table is available.
     */
    bool Load(const std::string &strPreferredPath)
    {
        if (m_pLibrary != nullptr)
        {
            return true;
        }

        std::vector<std::string> aCandidates;
        if (!strPreferredPath.empty())
        {
            aCandidates.push_back(strPreferredPath);
        }

#if defined(_WIN32)
        aCandidates.push_back("paho-mqtt3a.dll");
        aCandidates.push_back("libpaho-mqtt3a.dll");
#else
        aCandidates.push_back("libpaho-mqtt3a.so.1");
        aCandidates.push_back("libpaho-mqtt3a.so");
#endif

        for (const std::string &strCandidate : aCandidates)
        {
            if (OpenLibrary(strCandidate))
            {
                m_strLoadedPath = strCandidate;
                break;
            }
        }

        if (m_pLibrary == nullptr)
        {
            NETSDK_LOG_MESSAGE_ERROR("Platform MQTT could not load the Paho runtime library");
            return false;
        }

        m_fnCreate = Resolve<Create_FN>("MQTTAsync_create");
        m_fnSetCallbacks = Resolve<SetCallbacks_FN>("MQTTAsync_setCallbacks");
        m_fnConnect = Resolve<Connect_FN>("MQTTAsync_connect");
        m_fnDisconnect = Resolve<Disconnect_FN>("MQTTAsync_disconnect");
        m_fnIsConnected = Resolve<IsConnected_FN>("MQTTAsync_isConnected");
        m_fnSubscribe = Resolve<Subscribe_FN>("MQTTAsync_subscribe");
        m_fnSend = Resolve<Send_FN>("MQTTAsync_send");
        m_fnFreeMessage = Resolve<FreeMessage_FN>("MQTTAsync_freeMessage");
        m_fnFree = Resolve<Free_FN>("MQTTAsync_free");
        m_fnDestroy = Resolve<Destroy_FN>("MQTTAsync_destroy");

        if (m_fnCreate == nullptr || m_fnSetCallbacks == nullptr || m_fnConnect == nullptr ||
            m_fnDisconnect == nullptr || m_fnIsConnected == nullptr || m_fnSubscribe == nullptr ||
            m_fnSend == nullptr || m_fnFreeMessage == nullptr || m_fnFree == nullptr ||
            m_fnDestroy == nullptr)
        {
            NETSDK_LOG_MESSAGE_ERROR("Platform MQTT Paho runtime is missing required symbols: %s",
                                     m_strLoadedPath.c_str());
            Unload();
            return false;
        }

        NETSDK_LOG_MESSAGE_INFO("Platform MQTT loaded Paho runtime: %s", m_strLoadedPath.c_str());
        return true;
    }

    /**
     * @brief Creates one Paho MQTTAsync client.
     * @author Codex
     * @return Paho return code.
     */
    int Create(MQTTAsync *pClient,
               const char *pServerUri,
               const char *pClientId,
               int nPersistenceType,
               void *pPersistenceContext) const
    {
        return m_fnCreate(pClient, pServerUri, pClientId, nPersistenceType, pPersistenceContext);
    }

    /**
     * @brief Registers Paho connection and message callbacks.
     * @author Codex
     * @return Paho return code.
     */
    int SetCallbacks(MQTTAsync pClient,
                     void *pContext,
                     MQTTAsync_connectionLost *fnConnectionLost,
                     MQTTAsync_messageArrived *fnMessageArrived) const
    {
        return m_fnSetCallbacks(pClient, pContext, fnConnectionLost, fnMessageArrived, nullptr);
    }

    /**
     * @brief Submits an asynchronous Paho connect request.
     * @author Codex
     * @return Paho return code.
     */
    int Connect(MQTTAsync pClient, const MQTTAsync_connectOptions *pOptions) const
    {
        return m_fnConnect(pClient, pOptions);
    }

    /**
     * @brief Submits a Paho disconnect request.
     * @author Codex
     * @return Paho return code.
     */
    int Disconnect(MQTTAsync pClient, const MQTTAsync_disconnectOptions *pOptions) const
    {
        return m_fnDisconnect(pClient, pOptions);
    }

    /**
     * @brief Queries Paho's current client state.
     * @author Codex
     * @return Nonzero when connected.
     */
    int IsConnected(MQTTAsync pClient) const
    {
        return m_fnIsConnected(pClient);
    }

    /**
     * @brief Submits one Paho subscription.
     * @author Codex
     * @return Paho return code.
     */
    int Subscribe(MQTTAsync pClient, const char *pTopic, int nQos) const
    {
        return m_fnSubscribe(pClient, pTopic, nQos, nullptr);
    }

    /**
     * @brief Submits one Paho publication.
     * @author Codex
     * @return Paho return code.
     */
    int Send(MQTTAsync pClient,
             const char *pTopic,
             int nPayloadLength,
             const void *pPayload,
             int nQos,
             int nRetained) const
    {
        return m_fnSend(pClient,
                        pTopic,
                        nPayloadLength,
                        pPayload,
                        nQos,
                        nRetained,
                        nullptr);
    }

    /**
     * @brief Frees a Paho message received by a callback.
     * @author Codex
     */
    void FreeMessage(MQTTAsync_message **ppMessage) const
    {
        m_fnFreeMessage(ppMessage);
    }

    /**
     * @brief Frees a Paho-owned topic buffer.
     * @author Codex
     */
    void Free(void *pMemory) const
    {
        m_fnFree(pMemory);
    }

    /**
     * @brief Destroys a Paho client handle.
     * @author Codex
     */
    void Destroy(MQTTAsync *pClient) const
    {
        m_fnDestroy(pClient);
    }

private:
    /**
     * @brief Opens one platform shared library.
     * @author Codex
     * @param [IN] strPath Shared-library path or loader name.
     * @return True when the library opens successfully.
     */
    bool OpenLibrary(const std::string &strPath)
    {
#if defined(_WIN32)
        m_pLibrary = static_cast<void *>(LoadLibraryA(strPath.c_str()));
#else
        m_pLibrary = dlopen(strPath.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
        return m_pLibrary != nullptr;
    }

    /**
     * @brief Resolves one typed symbol from the loaded Paho library.
     * @author Codex
     * @return Function pointer or nullptr.
     */
    template <typename FunctionType_T>
    FunctionType_T Resolve(const char *pSymbol) const
    {
#if defined(_WIN32)
        return reinterpret_cast<FunctionType_T>(
            GetProcAddress(static_cast<HMODULE>(m_pLibrary), pSymbol));
#else
        return reinterpret_cast<FunctionType_T>(dlsym(m_pLibrary, pSymbol));
#endif
    }

    /**
     * @brief Clears the function table and unloads the shared library.
     * @author Codex
     * @return No return value.
     */
    void Unload()
    {
        m_fnCreate = nullptr;
        m_fnSetCallbacks = nullptr;
        m_fnConnect = nullptr;
        m_fnDisconnect = nullptr;
        m_fnIsConnected = nullptr;
        m_fnSubscribe = nullptr;
        m_fnSend = nullptr;
        m_fnFreeMessage = nullptr;
        m_fnFree = nullptr;
        m_fnDestroy = nullptr;

        if (m_pLibrary != nullptr)
        {
#if defined(_WIN32)
            FreeLibrary(static_cast<HMODULE>(m_pLibrary));
#else
            dlclose(m_pLibrary);
#endif
            m_pLibrary = nullptr;
        }
        m_strLoadedPath.clear();
    }

    void *m_pLibrary{nullptr};
    std::string m_strLoadedPath;
    Create_FN m_fnCreate{nullptr};
    SetCallbacks_FN m_fnSetCallbacks{nullptr};
    Connect_FN m_fnConnect{nullptr};
    Disconnect_FN m_fnDisconnect{nullptr};
    IsConnected_FN m_fnIsConnected{nullptr};
    Subscribe_FN m_fnSubscribe{nullptr};
    Send_FN m_fnSend{nullptr};
    FreeMessage_FN m_fnFreeMessage{nullptr};
    Free_FN m_fnFree{nullptr};
    Destroy_FN m_fnDestroy{nullptr};
};

CPlatformMqttTransport::CPlatformMqttTransport() = default;

CPlatformMqttTransport::~CPlatformMqttTransport()
{
    Stop();
}

bool CPlatformMqttTransport::Start(const Config_S &stConfig,
                                   MessageCallback_FN fnMessageCallback,
                                   ConnectionCallback_FN fnConnectionCallback)
{
    if (stConfig.strHost.empty() || stConfig.nPort <= 0 || stConfig.nPort > 65535 ||
        stConfig.strClientId.empty() || !fnMessageCallback)
    {
        NETSDK_LOG_MESSAGE_ERROR("Platform MQTT start rejected invalid parameters");
        return false;
    }

    if (m_bRunning.exchange(true))
    {
        NETSDK_LOG_MESSAGE_WARN("Platform MQTT transport is already running");
        return true;
    }

    m_stConfig = stConfig;
    {
        std::lock_guard<std::mutex> stCallbackLock(m_mtxCallbacks);
        m_fnMessageCallback = std::move(fnMessageCallback);
        m_fnConnectionCallback = std::move(fnConnectionCallback);
        m_uActiveMessageCallbacks = 0;
    }
    {
        std::lock_guard<std::mutex> stSubscriptionLock(m_mtxSubscriptions);
        m_aSubscriptions.clear();
    }
    {
        std::lock_guard<std::mutex> stStateLock(m_mtxState);
        m_bConnectResultReady = false;
        m_bConnectResultSuccess = false;
        m_strConnectResultReason.clear();
    }
    m_pApi.reset(new CPlatformMqttApi());
    if (!m_pApi->Load(m_stConfig.strRuntimeLibrary))
    {
        m_pApi.reset();
        m_bRunning.store(false);
        return false;
    }

    {
        std::lock_guard<std::mutex> stApiLock(m_mtxApi);
        if (!CreateClientLocked())
        {
            m_pApi.reset();
            m_bRunning.store(false);
            return false;
        }
    }

    m_bConnected.store(false);
    m_bReconnectRequested.store(true);
    m_bForceRecreate.store(false);
    m_uReconnectCount.store(0);
    m_stReconnectThread = std::thread(&CPlatformMqttTransport::ReconnectLoop, this);
    NETSDK_LOG_MESSAGE_INFO("Platform MQTT lifecycle started: broker=%s:%d client=%s",
                            m_stConfig.strHost.c_str(),
                            m_stConfig.nPort,
                            m_stConfig.strClientId.c_str());
    return true;
}

void CPlatformMqttTransport::Stop()
{
    if (!m_bRunning.exchange(false))
    {
        return;
    }

    m_bReconnectRequested.store(false);
    m_cvState.notify_all();
    if (m_stReconnectThread.joinable())
    {
        m_stReconnectThread.join();
    }

    {
        std::lock_guard<std::mutex> stApiLock(m_mtxApi);
        DestroyClientLocked();
    }

    m_bConnected.store(false);
    {
        std::unique_lock<std::mutex> stCallbackLock(m_mtxCallbacks);
        m_cvCallbacks.wait(stCallbackLock,
                           [this]()
                           {
                               return m_uActiveMessageCallbacks == 0;
                           });
        m_fnMessageCallback = MessageCallback_FN();
        m_fnConnectionCallback = ConnectionCallback_FN();
    }
    {
        std::lock_guard<std::mutex> stSubscriptionLock(m_mtxSubscriptions);
        m_aSubscriptions.clear();
    }
    m_pApi.reset();
    NETSDK_LOG_MESSAGE_INFO("Platform MQTT lifecycle stopped");
}

bool CPlatformMqttTransport::Publish(const std::string &strTopic,
                                     const std::string &strPayload,
                                     int nQos)
{
    if (strTopic.empty() || strPayload.empty() || nQos < 0 || nQos > 2)
    {
        return false;
    }

    if (strPayload.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        NETSDK_LOG_MESSAGE_ERROR("Platform MQTT payload exceeds Paho length range");
        return false;
    }

    std::lock_guard<std::mutex> stApiLock(m_mtxApi);
    if (!m_bConnected.load() || m_pApi == nullptr || m_pClient == nullptr ||
        m_pApi->IsConnected(m_pClient) == 0)
    {
        return false;
    }

    const int nResult = m_pApi->Send(m_pClient,
                                     strTopic.c_str(),
                                     static_cast<int>(strPayload.size()),
                                     strPayload.data(),
                                     nQos,
                                     0);
    if (nResult != MQTTASYNC_SUCCESS)
    {
        NETSDK_LOG_MESSAGE_WARN("Platform MQTT publish submit failed: topic=%s code=%d",
                                strTopic.c_str(),
                                nResult);
        return false;
    }
    return true;
}

bool CPlatformMqttTransport::Subscribe(const std::string &strTopic, int nQos)
{
    if (strTopic.empty() || nQos < 0 || nQos > 2)
    {
        return false;
    }

    {
        std::lock_guard<std::mutex> stSubscriptionLock(m_mtxSubscriptions);
        const auto stFound = std::find_if(
            m_aSubscriptions.begin(),
            m_aSubscriptions.end(),
            [&strTopic](const std::pair<std::string, int> &stItem)
            {
                return stItem.first == strTopic;
            });
        if (stFound == m_aSubscriptions.end())
        {
            m_aSubscriptions.emplace_back(strTopic, nQos);
        }
    }

    if (!m_bConnected.load())
    {
        return true;
    }
    return SubscribeNow(strTopic, nQos);
}

void CPlatformMqttTransport::RequestReconnect()
{
    if (!m_bRunning.load())
    {
        return;
    }

    m_bConnected.store(false);
    m_bForceRecreate.store(true);
    m_bReconnectRequested.store(true);
    m_cvState.notify_all();
}

bool CPlatformMqttTransport::IsConnected() const
{
    return m_bConnected.load();
}

std::uint64_t CPlatformMqttTransport::GetReconnectCount() const
{
    return m_uReconnectCount.load();
}

void CPlatformMqttTransport::OnConnectionLost(void *pContext, char *pCause)
{
    CPlatformMqttTransport *pTransport = static_cast<CPlatformMqttTransport *>(pContext);
    if (pTransport == nullptr || !pTransport->m_bRunning.load())
    {
        return;
    }

    const std::string strReason = pCause != nullptr ? pCause : "connection lost";
    pTransport->m_bConnected.store(false);
    pTransport->m_bReconnectRequested.store(true);
    pTransport->HandleConnectResult(false, strReason);
}

int CPlatformMqttTransport::OnMessageArrived(void *pContext,
                                             char *pTopicName,
                                             int nTopicLength,
                                             MQTTAsync_message *pMessage)
{
    CPlatformMqttTransport *pTransport = static_cast<CPlatformMqttTransport *>(pContext);
    CPlatformMqttApi *pApi = nullptr;
    MessageCallback_FN fnCallback;
    if (pTransport != nullptr)
    {
        std::lock_guard<std::mutex> stCallbackLock(pTransport->m_mtxCallbacks);
        ++pTransport->m_uActiveMessageCallbacks;
        pApi = pTransport->m_pApi.get();
        fnCallback = pTransport->m_fnMessageCallback;
    }

    std::string strTopic;
    std::string strPayload;
    if (pTopicName != nullptr && pMessage != nullptr)
    {
        if (nTopicLength > 0)
        {
            strTopic.assign(pTopicName, static_cast<std::size_t>(nTopicLength));
        }
        else
        {
            strTopic.assign(pTopicName);
        }

        if (pMessage->payload != nullptr && pMessage->payloadlen > 0)
        {
            strPayload.assign(static_cast<const char *>(pMessage->payload),
                              static_cast<std::size_t>(pMessage->payloadlen));
        }
    }

    if (pApi != nullptr)
    {
        if (pMessage != nullptr)
        {
            pApi->FreeMessage(&pMessage);
        }
        if (pTopicName != nullptr)
        {
            pApi->Free(pTopicName);
        }
    }

    if (pTransport != nullptr && pTransport->m_bRunning.load() && fnCallback)
    {
        fnCallback(strTopic, strPayload);
    }

    if (pTransport != nullptr)
    {
        {
            std::lock_guard<std::mutex> stCallbackLock(pTransport->m_mtxCallbacks);
            if (pTransport->m_uActiveMessageCallbacks > 0)
            {
                --pTransport->m_uActiveMessageCallbacks;
            }
        }
        pTransport->m_cvCallbacks.notify_all();
    }
    return 1;
}

void CPlatformMqttTransport::OnConnectSuccess(void *pContext, MQTTAsync_successData *pResponse)
{
    static_cast<void>(pResponse);
    CPlatformMqttTransport *pTransport = static_cast<CPlatformMqttTransport *>(pContext);
    if (pTransport != nullptr && pTransport->m_bRunning.load())
    {
        pTransport->HandleConnectResult(true, "connect");
    }
}

void CPlatformMqttTransport::OnConnectFailure(void *pContext, MQTTAsync_failureData *pResponse)
{
    CPlatformMqttTransport *pTransport = static_cast<CPlatformMqttTransport *>(pContext);
    if (pTransport == nullptr || !pTransport->m_bRunning.load())
    {
        return;
    }

    std::string strReason("connect failure");
    if (pResponse != nullptr && pResponse->message != nullptr)
    {
        strReason.assign(pResponse->message);
    }
    pTransport->HandleConnectResult(false, strReason);
}

void CPlatformMqttTransport::ReconnectLoop()
{
    int nFailedAttempts = 0;
    bool bReportedConnected = false;

    while (m_bRunning.load())
    {
        if (m_bForceRecreate.exchange(false))
        {
            std::lock_guard<std::mutex> stApiLock(m_mtxApi);
            DestroyClientLocked();
            if (!CreateClientLocked())
            {
                ++nFailedAttempts;
                m_uReconnectCount.fetch_add(1);
            }
        }

        if (m_bConnected.load())
        {
            if (!bReportedConnected)
            {
                RestoreSubscriptions();
                bReportedConnected = true;
                ConnectionCallback_FN fnConnectionCallback;
                {
                    std::lock_guard<std::mutex> stCallbackLock(m_mtxCallbacks);
                    fnConnectionCallback = m_fnConnectionCallback;
                }
                if (fnConnectionCallback)
                {
                    fnConnectionCallback(true, "connect");
                }
            }

            std::unique_lock<std::mutex> stStateLock(m_mtxState);
            m_cvState.wait(stStateLock,
                           [this]()
                           {
                               return !m_bRunning.load() || !m_bConnected.load() ||
                                      m_bReconnectRequested.load();
                           });
            continue;
        }

        if (bReportedConnected)
        {
            bReportedConnected = false;
            std::string strDisconnectReason;
            ConnectionCallback_FN fnConnectionCallback;
            {
                std::lock_guard<std::mutex> stStateLock(m_mtxState);
                strDisconnectReason = m_strConnectResultReason.empty()
                                          ? "disconnect"
                                          : m_strConnectResultReason;
            }
            {
                std::lock_guard<std::mutex> stCallbackLock(m_mtxCallbacks);
                fnConnectionCallback = m_fnConnectionCallback;
            }
            if (fnConnectionCallback)
            {
                fnConnectionCallback(false, strDisconnectReason);
            }
        }

        const int nDelaySec = CalculateReconnectDelay(nFailedAttempts);
        if (nDelaySec > 0)
        {
            std::unique_lock<std::mutex> stStateLock(m_mtxState);
            m_cvState.wait_for(stStateLock,
                               std::chrono::seconds(nDelaySec),
                               [this]()
                               {
                                   return !m_bRunning.load() || m_bForceRecreate.load();
                               });
            if (!m_bRunning.load())
            {
                break;
            }
            if (m_bForceRecreate.load())
            {
                continue;
            }
        }

        {
            std::lock_guard<std::mutex> stStateLock(m_mtxState);
            m_bConnectResultReady = false;
            m_bConnectResultSuccess = false;
            m_strConnectResultReason.clear();
        }

        m_bReconnectRequested.store(false);
        if (!ConnectOnce())
        {
            ++nFailedAttempts;
            m_uReconnectCount.fetch_add(1);
            continue;
        }

        std::unique_lock<std::mutex> stStateLock(m_mtxState);
        const bool bResultReady = m_cvState.wait_for(
            stStateLock,
            std::chrono::seconds(PLATFORM_MQTT_CONNECT_RESULT_TIMEOUT_SEC),
            [this]()
            {
                return !m_bRunning.load() || m_bConnectResultReady;
            });

        if (!m_bRunning.load())
        {
            break;
        }

        if (!bResultReady || !m_bConnectResultSuccess)
        {
            ++nFailedAttempts;
            m_uReconnectCount.fetch_add(1);
            m_bConnected.store(false);
            NETSDK_LOG_MESSAGE_WARN("Platform MQTT connect failed: %s",
                                    bResultReady ? m_strConnectResultReason.c_str() : "timeout");
            continue;
        }

        nFailedAttempts = 0;
        m_bConnected.store(true);
        NETSDK_LOG_MESSAGE_INFO("Platform MQTT connected: broker=%s:%d",
                                m_stConfig.strHost.c_str(),
                                m_stConfig.nPort);
    }
}

bool CPlatformMqttTransport::CreateClientLocked()
{
    if (m_pApi == nullptr)
    {
        return false;
    }
    if (m_pClient != nullptr)
    {
        return true;
    }

    const std::string strServerUri = "tcp://" + m_stConfig.strHost + ":" +
                                     std::to_string(m_stConfig.nPort);
    int nResult = m_pApi->Create(&m_pClient,
                                 strServerUri.c_str(),
                                 m_stConfig.strClientId.c_str(),
                                 MQTTCLIENT_PERSISTENCE_NONE,
                                 nullptr);
    if (nResult != MQTTASYNC_SUCCESS || m_pClient == nullptr)
    {
        NETSDK_LOG_MESSAGE_ERROR("Platform MQTT client creation failed: code=%d", nResult);
        m_pClient = nullptr;
        return false;
    }

    nResult = m_pApi->SetCallbacks(m_pClient,
                                   this,
                                   &CPlatformMqttTransport::OnConnectionLost,
                                   &CPlatformMqttTransport::OnMessageArrived);
    if (nResult != MQTTASYNC_SUCCESS)
    {
        NETSDK_LOG_MESSAGE_ERROR("Platform MQTT callback registration failed: code=%d", nResult);
        m_pApi->Destroy(&m_pClient);
        m_pClient = nullptr;
        return false;
    }
    return true;
}

void CPlatformMqttTransport::DestroyClientLocked()
{
    if (m_pApi == nullptr || m_pClient == nullptr)
    {
        m_pClient = nullptr;
        return;
    }

    if (m_pApi->IsConnected(m_pClient) != 0)
    {
        MQTTAsync_disconnectOptions stOptions = MQTTAsync_disconnectOptions_initializer;
        stOptions.timeout = 1000;
        m_pApi->Disconnect(m_pClient, &stOptions);
    }
    m_pApi->Destroy(&m_pClient);
    m_pClient = nullptr;
}

bool CPlatformMqttTransport::ConnectOnce()
{
    std::lock_guard<std::mutex> stApiLock(m_mtxApi);
    if (!m_bRunning.load() || m_pApi == nullptr)
    {
        return false;
    }
    if (m_pClient == nullptr && !CreateClientLocked())
    {
        return false;
    }

    MQTTAsync_willOptions stWillOptions = MQTTAsync_willOptions_initializer;
    if (!m_stConfig.strWillTopic.empty())
    {
        stWillOptions.topicName = m_stConfig.strWillTopic.c_str();
        stWillOptions.message = m_stConfig.strWillPayload.c_str();
        stWillOptions.qos = m_stConfig.nWillQos;
        stWillOptions.retained = m_stConfig.bWillRetain ? 1 : 0;
    }

    MQTTAsync_connectOptions stOptions = MQTTAsync_connectOptions_initializer;
    stOptions.keepAliveInterval = m_stConfig.nKeepAliveSec;
    stOptions.cleansession = 1;
    stOptions.connectTimeout = m_stConfig.nConnectTimeoutSec;
    stOptions.automaticReconnect = 0;
    stOptions.username = m_stConfig.strUser.empty() ? nullptr : m_stConfig.strUser.c_str();
    stOptions.password = m_stConfig.strPassword.empty() ? nullptr : m_stConfig.strPassword.c_str();
    stOptions.will = m_stConfig.strWillTopic.empty() ? nullptr : &stWillOptions;
    stOptions.context = this;
    stOptions.onSuccess = &CPlatformMqttTransport::OnConnectSuccess;
    stOptions.onFailure = &CPlatformMqttTransport::OnConnectFailure;

    const int nResult = m_pApi->Connect(m_pClient, &stOptions);
    if (nResult != MQTTASYNC_SUCCESS)
    {
        NETSDK_LOG_MESSAGE_WARN("Platform MQTT connect submit failed: code=%d", nResult);
        return false;
    }
    return true;
}

void CPlatformMqttTransport::RestoreSubscriptions()
{
    std::vector<std::pair<std::string, int>> aSubscriptions;
    {
        std::lock_guard<std::mutex> stSubscriptionLock(m_mtxSubscriptions);
        aSubscriptions = m_aSubscriptions;
    }

    for (const std::pair<std::string, int> &stSubscription : aSubscriptions)
    {
        if (!SubscribeNow(stSubscription.first, stSubscription.second))
        {
            NETSDK_LOG_MESSAGE_WARN("Platform MQTT subscription recovery failed: topic=%s",
                                    stSubscription.first.c_str());
        }
    }
}

bool CPlatformMqttTransport::SubscribeNow(const std::string &strTopic, int nQos)
{
    std::lock_guard<std::mutex> stApiLock(m_mtxApi);
    if (!m_bConnected.load() || m_pApi == nullptr || m_pClient == nullptr)
    {
        return false;
    }

    const int nResult = m_pApi->Subscribe(m_pClient, strTopic.c_str(), nQos);
    if (nResult != MQTTASYNC_SUCCESS)
    {
        NETSDK_LOG_MESSAGE_WARN("Platform MQTT subscribe submit failed: topic=%s code=%d",
                                strTopic.c_str(),
                                nResult);
        return false;
    }
    return true;
}

void CPlatformMqttTransport::HandleConnectResult(bool bSuccess, const std::string &strReason)
{
    {
        std::lock_guard<std::mutex> stStateLock(m_mtxState);
        m_bConnectResultReady = true;
        m_bConnectResultSuccess = bSuccess;
        m_strConnectResultReason = strReason;
        if (bSuccess)
        {
            m_bConnected.store(true);
            m_bReconnectRequested.store(false);
        }
        else
        {
            m_bConnected.store(false);
            m_bReconnectRequested.store(true);
        }
    }
    m_cvState.notify_all();
}
