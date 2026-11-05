#include "VoiceComServer.h"

#include <cstring>
#include <utility>
#include <vector>
#include <chrono>

#include "NetSdkLog.h"
#include "PlatformCompat.h"

namespace tvsdk {
namespace {

constexpr uint32_t kVoiceComParamMagic = 0x56435031;

struct VoiceComParamFrame {
    uint32_t magic;
    NET_VoiceComAudioParam_S audio_param;
};

bool recv_exact(socket_fd_t nFd, char* pstrData, size_t uSize) {
    size_t uReceived = 0;
    while (uReceived < uSize) {
        ssize_t lRet = recv(nFd, pstrData + uReceived, static_cast<int>(uSize - uReceived), 0);
        if (lRet <= 0) {
            return false;
        }
        uReceived += static_cast<size_t>(lRet);
    }
    return true;
}

void fill_default_voicecom_audio_param(NET_VoiceComAudioParam_S& stAudioParam) {
    std::memset(&stAudioParam, 0, sizeof(stAudioParam));
    stAudioParam.enFormat = NET_AUDIO_FORMAT_PCM;
    stAudioParam.uSampleRate = NET_AUDIO_SAMPRATE_16000;
    stAudioParam.uBitDepth = 16;
    stAudioParam.uChannels = 1;
    stAudioParam.uFrameIntervalMs = 20;
    stAudioParam.uFrameBytes = 640;
    stAudioParam.uBitRate = 256000;
    stAudioParam.bLittleEndian = TRUE;
}

bool normalize_voicecom_audio_param(NET_VoiceComAudioParam_S& stAudioParam) {
    if (stAudioParam.uChannels != 1) {
        return false;
    }

    int nBytesPerSample = 0;
    switch (stAudioParam.enFormat) {
        case NET_AUDIO_FORMAT_PCM:
            if (stAudioParam.uBitDepth <= 0) {
                stAudioParam.uBitDepth = 16;
            }
            if (stAudioParam.uBitDepth != 16) {
                return false;
            }
            switch (stAudioParam.uSampleRate) {
                case NET_AUDIO_SAMPRATE_8000:
                case NET_AUDIO_SAMPRATE_16000:
                    break;
                default:
                    return false;
            }
            nBytesPerSample = stAudioParam.uBitDepth / 8;
            break;
        case NET_AUDIO_FORMAT_G711A:
        case NET_AUDIO_FORMAT_G711U:
            if (stAudioParam.uSampleRate != NET_AUDIO_SAMPRATE_8000) {
                return false;
            }
            if (stAudioParam.uBitDepth <= 0) {
                stAudioParam.uBitDepth = 8;
            }
            if (stAudioParam.uBitDepth != 8) {
                return false;
            }
            nBytesPerSample = 1;
            break;
        default:
            return false;
    }

    if (stAudioParam.uFrameIntervalMs <= 0) {
        stAudioParam.uFrameIntervalMs = 20;
    }
    if (stAudioParam.uFrameIntervalMs < 10 || stAudioParam.uFrameIntervalMs > 1000) {
        return false;
    }

    const int nFrameBytes = stAudioParam.uSampleRate * stAudioParam.uChannels *
                            nBytesPerSample * stAudioParam.uFrameIntervalMs / 1000;
    if (nFrameBytes <= 0 || nFrameBytes > NET_LEN_4096) {
        return false;
    }

    if (stAudioParam.uFrameBytes <= 0) {
        stAudioParam.uFrameBytes = nFrameBytes;
    }
    if (stAudioParam.uFrameBytes != nFrameBytes) {
        return false;
    }

    stAudioParam.uBitRate = stAudioParam.uSampleRate * stAudioParam.uChannels * stAudioParam.uBitDepth;
    stAudioParam.bLittleEndian = TRUE;
    return true;
}

int normalize_frame_interval_ms(int nFrameIntervalMs) {
    if (nFrameIntervalMs <= 0) {
        return 20;
    }
    if (nFrameIntervalMs < 10) {
        return 10;
    }
    if (nFrameIntervalMs > 1000) {
        return 1000;
    }
    return nFrameIntervalMs;
}

bool is_voicecom_param_frame(const char* pstrData, size_t uSize, NET_VoiceComAudioParam_S& stAudioParam) {
    if (!pstrData || uSize != sizeof(VoiceComParamFrame)) {
        return false;
    }

    VoiceComParamFrame stFrame{};
    std::memcpy(&stFrame, pstrData, sizeof(stFrame));
    if (ntohl(stFrame.magic) != kVoiceComParamMagic) {
        return false;
    }

    stAudioParam = stFrame.audio_param;
    return normalize_voicecom_audio_param(stAudioParam);
}

bool has_voicecom_param_magic(const char* pstrData, size_t uSize) {
    if (!pstrData || uSize != sizeof(VoiceComParamFrame)) {
        return false;
    }

    uint32_t uMagic = 0;
    std::memcpy(&uMagic, pstrData, sizeof(uMagic));
    return ntohl(uMagic) == kVoiceComParamMagic;
}

} // namespace

CVoiceComServer* CVoiceComServer::instance() {
    static CVoiceComServer s_instance;
    return &s_instance;
}

CVoiceComServer::CVoiceComServer() {
    fill_default_voicecom_audio_param(m_stAudioParameter);
}

CVoiceComServer::~CVoiceComServer() { stop(); }

bool CVoiceComServer::start(int nPort) {
    if (m_bRunning) return true;

    m_nListenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_nListenFd == INVALID_SOCKET_FD) {
        NETSDK_LOG_MESSAGE_ERROR("CVoiceComServer: socket failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
        return false;
    }

    int nOpt = 1;
    setsockopt(m_nListenFd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&nOpt), sizeof(nOpt));

    sockaddr_in stAddr{};
    stAddr.sin_family = AF_INET;
    stAddr.sin_addr.s_addr = INADDR_ANY;
    stAddr.sin_port = htons(static_cast<uint16_t>(nPort));

    if (bind(m_nListenFd, reinterpret_cast<sockaddr*>(&stAddr), sizeof(stAddr)) != 0) {
        NETSDK_LOG_MESSAGE_ERROR("CVoiceComServer: bind port %d failed, errno=%d", nPort, NETSDK_SOCKET_GET_ERROR());
        NETSDK_SOCKET_CLOSE(m_nListenFd);
        m_nListenFd = INVALID_SOCKET_FD;
        return false;
    }

    if (listen(m_nListenFd, 1) != 0) {
        NETSDK_LOG_MESSAGE_ERROR("CVoiceComServer: listen failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
        NETSDK_SOCKET_CLOSE(m_nListenFd);
        m_nListenFd = INVALID_SOCKET_FD;
        return false;
    }

    m_bRunning = true;
    m_stAcceptThread = std::thread(&CVoiceComServer::accept_loop, this);
    m_stCaptureThread = std::thread(&CVoiceComServer::capture_loop, this);

    NETSDK_LOG_MESSAGE_INFO("CVoiceComServer: listening on port %d", nPort);
    return true;
}

void CVoiceComServer::stop() {
    m_bRunning = false;
    m_stParameterCondition.notify_all();

    if (m_nListenFd != INVALID_SOCKET_FD) {
        shutdown(m_nListenFd, SHUT_RDWR);
        NETSDK_SOCKET_CLOSE(m_nListenFd);
        m_nListenFd = INVALID_SOCKET_FD;
    }
    {
        std::lock_guard<std::mutex> lock(m_stSendMutex);
        if (m_nClientFd != INVALID_SOCKET_FD) {
            shutdown(m_nClientFd, SHUT_RDWR);
            m_nClientFd = INVALID_SOCKET_FD;
        }
    }
    {
        std::lock_guard<std::mutex> lock(m_stParameterMutex);
        fill_default_voicecom_audio_param(m_stAudioParameter);
        m_bHasAudioParameter = false;
    }
    m_stParameterCondition.notify_all();

    if (m_stAcceptThread.joinable()) m_stAcceptThread.join();
    if (m_stReceiveThread.joinable()) m_stReceiveThread.join();
    if (m_stCaptureThread.joinable()) m_stCaptureThread.join();

    NETSDK_LOG_MESSAGE_INFO("CVoiceComServer: stopped");
}

bool CVoiceComServer::send_to_client(const char* pstrData, size_t uSize) {
    if (pstrData == nullptr || uSize == 0 || uSize > 0xFFFFu) {
        return false;
    }

    // 帧格式: [2B长度 大端][音频帧], 音频格式由首帧参数协商。
    uint16_t uLenBe = htons(static_cast<uint16_t>(uSize));
    std::vector<char> vecFrame(sizeof(uLenBe) + uSize);
    std::memcpy(vecFrame.data(), &uLenBe, sizeof(uLenBe));
    std::memcpy(vecFrame.data() + sizeof(uLenBe), pstrData, uSize);

#ifdef _WIN32
    int nSendFlags = 0;
#else
    int nSendFlags = MSG_NOSIGNAL;
#endif

    std::lock_guard<std::mutex> lock(m_stSendMutex);
    const socket_fd_t nFd = m_nClientFd;
    if (nFd == INVALID_SOCKET_FD) {
        return false;
    }

    size_t uSent = 0;
    while (uSent < vecFrame.size()) {
        ssize_t lRet = ::send(nFd, vecFrame.data() + uSent, static_cast<int>(vecFrame.size() - uSent), nSendFlags);
        if (lRet <= 0) return false;
        uSent += static_cast<size_t>(lRet);
    }
    return true;
}

void CVoiceComServer::set_play_callback(VoiceComPlayCallback cb) {
    std::lock_guard<std::mutex> lock(m_stCallbackMutex);
    m_fnPlayCallback = std::move(cb);
}

void CVoiceComServer::set_capture_callback(VoiceComCaptureCallback cb) {
    {
        std::lock_guard<std::mutex> lock(m_stCallbackMutex);
        m_fnCaptureCallback = std::move(cb);
    }
    m_stParameterCondition.notify_all();
}

bool CVoiceComServer::get_audio_param(NET_VoiceComAudioParam_S& stAudioParam) const {
    return snapshot_audio_param(stAudioParam);
}

bool CVoiceComServer::snapshot_audio_param(NET_VoiceComAudioParam_S& stAudioParam) const {
    std::lock_guard<std::mutex> lock(m_stParameterMutex);
    if (!m_bHasAudioParameter) {
        return false;
    }

    stAudioParam = m_stAudioParameter;
    return true;
}

void CVoiceComServer::notify_audio_param_ready() {
    m_stParameterCondition.notify_all();
}

void CVoiceComServer::capture_loop() {
    bool bLoggedWaitSource = false;
    bool bLoggedSendFailed = false;
    uint64_t ulFrameCount = 0;

    /*
     * VoiceCom协议层只负责会话参数、拉帧节拍和网络发送。
     * 实际音频采集由业务注册的采集回调提供，避免SDK强依赖具体芯片或音频驱动。
     */
    while (m_bRunning) {
        NET_VoiceComAudioParam_S stAudioParam{};
        {
            std::unique_lock<std::mutex> lock(m_stParameterMutex);
            m_stParameterCondition.wait_for(lock, std::chrono::milliseconds(100), [this] {
                return !m_bRunning || m_bHasAudioParameter;
            });

            if (!m_bRunning) {
                break;
            }
            if (!m_bHasAudioParameter) {
                continue;
            }
            stAudioParam = m_stAudioParameter;
        }

        VoiceComCaptureCallback captureCb;
        {
            std::lock_guard<std::mutex> lock(m_stCallbackMutex);
            captureCb = m_fnCaptureCallback;
        }
        if (!captureCb) {
            if (!bLoggedWaitSource) {
                NETSDK_LOG_MESSAGE_INFO("CVoiceComServer: waiting voice capture callback");
                bLoggedWaitSource = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        bLoggedWaitSource = false;

        const size_t uFrameBytes = static_cast<size_t>(stAudioParam.uFrameBytes);
        if (uFrameBytes == 0 || uFrameBytes > NET_LEN_4096) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        std::vector<char> vecFrame(uFrameBytes);
        auto stNextSendTime = std::chrono::steady_clock::now();

        while (m_bRunning) {
            {
                std::lock_guard<std::mutex> lock(m_stCallbackMutex);
                captureCb = m_fnCaptureCallback;
            }
            if (!captureCb) {
                break;
            }

            NET_VoiceComAudioParam_S stCurrentParam{};
            if (!snapshot_audio_param(stCurrentParam) ||
                stCurrentParam.enFormat != stAudioParam.enFormat ||
                stCurrentParam.uSampleRate != stAudioParam.uSampleRate ||
                stCurrentParam.uBitDepth != stAudioParam.uBitDepth ||
                stCurrentParam.uChannels != stAudioParam.uChannels ||
                stCurrentParam.uFrameBytes != stAudioParam.uFrameBytes ||
                stCurrentParam.uFrameIntervalMs != stAudioParam.uFrameIntervalMs) {
                break;
            }

            int nReadBytes = captureCb(stCurrentParam, vecFrame.data(), vecFrame.size());
            if (nReadBytes <= 0) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(normalize_frame_interval_ms(stCurrentParam.uFrameIntervalMs)));
                stNextSendTime = std::chrono::steady_clock::now();
                continue;
            }

            if (static_cast<size_t>(nReadBytes) > vecFrame.size()) {
                NETSDK_LOG_MESSAGE_WARN("CVoiceComServer: capture callback returned oversized frame, bytes=%d, max=%zu",
                              nReadBytes, vecFrame.size());
                nReadBytes = static_cast<int>(vecFrame.size());
            }

            std::this_thread::sleep_until(stNextSendTime);
            stNextSendTime += std::chrono::milliseconds(
                normalize_frame_interval_ms(stCurrentParam.uFrameIntervalMs));

            if (!send_to_client(vecFrame.data(), static_cast<size_t>(nReadBytes))) {
                if (!bLoggedSendFailed) {
                    NETSDK_LOG_MESSAGE_WARN("CVoiceComServer: send captured voice frame failed");
                    bLoggedSendFailed = true;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                stNextSendTime = std::chrono::steady_clock::now();
                continue;
            }

            bLoggedSendFailed = false;
            ++ulFrameCount;
            if (ulFrameCount <= 5 || (ulFrameCount % 100) == 0) {
                NETSDK_LOG_MESSAGE_DEBUG("CVoiceComServer: sent captured voice frame=%llu bytes=%d",
                               static_cast<unsigned long long>(ulFrameCount),
                               nReadBytes);
            }
        }
    }
}

void CVoiceComServer::accept_loop() {
    while (m_bRunning) {
        sockaddr_in stClientAddr{};
        socklen_t uAddrLen = sizeof(stClientAddr);

        socket_fd_t nFd = accept(m_nListenFd, reinterpret_cast<sockaddr*>(&stClientAddr), &uAddrLen);
        if (nFd == INVALID_SOCKET_FD) {
            if (m_bRunning) {
                NETSDK_LOG_MESSAGE_WARN("CVoiceComServer: accept failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
            }
            continue;
        }

        int nTcpNoDelay = 1;
        if (setsockopt(nFd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&nTcpNoDelay), sizeof(nTcpNoDelay)) != 0) {
            NETSDK_LOG_MESSAGE_WARN("CVoiceComServer: set TCP_NODELAY failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
        }

        {
            std::lock_guard<std::mutex> lock(m_stSendMutex);
            if (m_nClientFd != INVALID_SOCKET_FD) {
                // 关闭旧连接，唤醒旧接收线程从 recv_exact() 中退出。
                shutdown(m_nClientFd, SHUT_RDWR);
            }
        }

        // std::thread 对象即使线程函数已经返回，在 join 前仍然是 joinable。
        // 必须在创建新接收线程前回收旧线程，否则重新赋值会触发 std::terminate。
        if (m_stReceiveThread.joinable()) m_stReceiveThread.join();

        {
            std::lock_guard<std::mutex> lock(m_stParameterMutex);
            fill_default_voicecom_audio_param(m_stAudioParameter);
            m_bHasAudioParameter = false;
        }
        {
            std::lock_guard<std::mutex> lock(m_stSendMutex);
            m_nClientFd = nFd;
        }
        m_stReceiveThread = std::thread(&CVoiceComServer::client_recv_loop, this, nFd);

        char strIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &stClientAddr.sin_addr, strIp, sizeof(strIp));
        NETSDK_LOG_MESSAGE_INFO("CVoiceComServer: client connected from %s:%d",
                      strIp, ntohs(stClientAddr.sin_port));
    }
}

void CVoiceComServer::client_recv_loop(socket_fd_t nClientFd) {
    char strHeader[2];
    char strBuffer[4096];

    while (m_bRunning && nClientFd != INVALID_SOCKET_FD) {
        if (!recv_exact(nClientFd, strHeader, sizeof(strHeader))) break;

        uint16_t uLenBe;
        std::memcpy(&uLenBe, strHeader, sizeof(uLenBe));
        size_t uFrameLen = ntohs(uLenBe);

        if (uFrameLen == 0 || uFrameLen > sizeof(strBuffer)) continue;

        if (!recv_exact(nClientFd, strBuffer, uFrameLen)) {
            break;
        }

        NET_VoiceComAudioParam_S stAudioParam{};
        if (is_voicecom_param_frame(strBuffer, uFrameLen, stAudioParam)) {
            {
                std::lock_guard<std::mutex> lock(m_stParameterMutex);
                m_stAudioParameter = stAudioParam;
                m_bHasAudioParameter = true;
            }
            NETSDK_LOG_MESSAGE_INFO("CVoiceComServer: audio param format=%d sampleRate=%d bitDepth=%d channels=%d frameMs=%d frameBytes=%d",
                          stAudioParam.enFormat,
                          stAudioParam.uSampleRate,
                          stAudioParam.uBitDepth,
                          stAudioParam.uChannels,
                          stAudioParam.uFrameIntervalMs,
                          stAudioParam.uFrameBytes);
            notify_audio_param_ready();
            continue;
        }
        if (has_voicecom_param_magic(strBuffer, uFrameLen)) {
            NETSDK_LOG_MESSAGE_WARN("CVoiceComServer: invalid audio param frame");
            break;
        }

        VoiceComPlayCallback playCb;
        {
            std::lock_guard<std::mutex> lock(m_stCallbackMutex);
            playCb = m_fnPlayCallback;
        }
        if (playCb) {
            playCb(strBuffer, uFrameLen);
        }
    }

    NETSDK_LOG_MESSAGE_INFO("CVoiceComServer: client disconnected");
    if (nClientFd != INVALID_SOCKET_FD) {
        NETSDK_SOCKET_CLOSE(nClientFd);
    }

    bool bClearParam = false;
    {
        std::lock_guard<std::mutex> lock(m_stSendMutex);
        if (m_nClientFd == nClientFd) {
            m_nClientFd = INVALID_SOCKET_FD;
            bClearParam = true;
        }
    }
    if (bClearParam) {
        std::lock_guard<std::mutex> lock(m_stParameterMutex);
        fill_default_voicecom_audio_param(m_stAudioParameter);
        m_bHasAudioParameter = false;
    }
    if (bClearParam) {
        notify_audio_param_ready();
    }
}

}  // namespace tvsdk
