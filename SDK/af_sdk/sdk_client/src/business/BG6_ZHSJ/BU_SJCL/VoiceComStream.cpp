/**
 * @file VoiceComStream.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CVoiceComStream 模块实现
 * 功能说明：
 * 1. 实现 CVoiceComStream 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : CVoiceComStream.cpp
 * @Description  : 语音对讲客户端实现，负责与设备建立TCP音频通道、收发音频数据
 */

#include "VoiceComStream.h"

#include <cstring>
#include <utility>
#include <vector>
#include <cstdio>

#include "NetSdkLog.h"
#include "PlatformCompat.h"

namespace tvsdk {
namespace {

constexpr uint32_t kVoiceComParamMagic = 0x56435031;  /*/< 音频参数帧魔数（VCP1） */

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 音频参数帧结构体
 * @details 首帧发送的音频参数协商帧，包含魔数和音频参数
 */
struct VoiceComParamFrame_S {
    uint32_t magic;                            /*/< 魔数，固定为0x56435031（VCP1） */
    NET_VoiceComAudioParam_S audio_param; /*/< 音频参数（采样率、通道数、编码格式等） */
};

#ifdef VOICECOM_DEBUG_PRINT

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 检查音频数据是否包含ADTS头
 * @details ADTS头以0xFF 0xF0开头，用于识别AAC音频帧
 * @param pData 音频数据指针
 * @param uSize 音频数据大小（字节）
 * @return true表示包含ADTS头，false表示不包含
 */
static bool has_adts_header(const void* pData, size_t uSize) {
    if (pData == nullptr || uSize < 2) {
        return false;
    }

    const auto* p = static_cast<const unsigned char*>(pData);
    return p[0] == 0xFF && (p[1] & 0xF0) == 0xF0;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 格式化数据头部为十六进制字符串
 * @details 最多显示前16字节的十六进制值，用于调试打印
 * @param pData 数据指针
 * @param uSize 数据大小（字节）
 * @param pstrOutput 输出缓冲区
 * @param uOutputSize 输出缓冲区大小
 */
static void format_head_hex(const void* pData, size_t uSize, char* pstrOutput, size_t uOutputSize) {
    if (pstrOutput == nullptr || uOutputSize == 0) {
        return;
    }

    pstrOutput[0] = '\0';
    if (pData == nullptr || uSize == 0) {
        return;
    }

    const auto* p = static_cast<const unsigned char*>(pData);
    const size_t uDumpSize = uSize < 16 ? uSize : 16;
    size_t uUsed = 0;
    for (size_t i = 0; i < uDumpSize && uUsed < uOutputSize; ++i) {
        int nWritten = std::snprintf(pstrOutput + uUsed, uOutputSize - uUsed,
                                    (i == 0) ? "%02X" : " %02X", p[i]);
        if (nWritten <= 0) {
            break;
        }
        uUsed += static_cast<size_t>(nWritten);
        if (uUsed >= uOutputSize) {
            break;
        }
    }
}

#endif /* VOICECOM_DEBUG_PRINT */

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 精确接收指定长度的数据
 * @details 循环调用recv直到接收完指定长度或出错
 * @param fd socket文件描述符
 * @param pstrData 接收缓冲区指针
 * @param uSize 需要接收的字节数
 * @return true表示成功接收全部数据，false表示失败
 */
static bool recv_exact(socket_fd_t fd, char* pstrData, size_t uSize) {
    size_t uReceived = 0;
    while (uReceived < uSize) {
        ssize_t n = recv(fd, pstrData + uReceived, static_cast<int>(uSize - uReceived), 0);
        if (n <= 0) {
            return false;
        }
        uReceived += static_cast<size_t>(n);
    }
    return true;
}

} /* namespace */

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 构造函数
 */
CVoiceComStream::CVoiceComStream() = default;

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 析构函数
 * @details 自动停止对讲并释放资源
 */
CVoiceComStream::~CVoiceComStream() { stop(); }

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 连接设备音频端口并启动收发
 * @details 完整流程：
 *          1. 创建TCP socket
 *          2. 设置超时时间（5秒）
 *          3. 连接到设备音频端口
 *          4. 启用TCP_NODELAY（禁用Nagle算法）
 *          5. 发送音频参数协商帧（VCP1）
 *          6. 启动接收线程
 * @param host 设备IP地址
 * @param nPort 设备音频端口号
 * @param stAudioParam 音频参数（采样率、通道数、编码格式等）
 * @param callback 音频数据回调函数，用于接收设备发送的音频数据
 * @return true表示成功，false表示失败
 */
bool CVoiceComStream::start(const std::string& host,
                           int nPort,
                           const NET_VoiceComAudioParam_S& stAudioParam,
                           VoiceComCallback callback) {
    if (m_bRunning) {
        NETSDK_LOG_MESSAGE_WARN("CVoiceComStream: already running");
        return false;
    }

    m_nSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_nSocket == INVALID_SOCKET_FD) {
        NETSDK_LOG_MESSAGE_ERROR("CVoiceComStream: socket failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
        return false;
    }

    /* 设置5秒超时 */
    timeval tv{};
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(m_nSocket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
    setsockopt(m_nSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(nPort));
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        NETSDK_LOG_MESSAGE_ERROR("CVoiceComStream: invalid host %s", host.c_str());
        NETSDK_SOCKET_CLOSE(m_nSocket);
        m_nSocket = INVALID_SOCKET_FD;
        return false;
    }

    if (connect(m_nSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        NETSDK_LOG_MESSAGE_ERROR("CVoiceComStream: connect %s:%d failed, errno=%d",
                       host.c_str(), nPort, NETSDK_SOCKET_GET_ERROR());
        NETSDK_SOCKET_CLOSE(m_nSocket);
        m_nSocket = INVALID_SOCKET_FD;
        return false;
    }

    int nTcpNoDelay = 1;
    if (setsockopt(m_nSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&nTcpNoDelay), sizeof(nTcpNoDelay)) != 0) {
        NETSDK_LOG_MESSAGE_WARN("CVoiceComStream: set TCP_NODELAY failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
    }

    if (!send_audio_param(stAudioParam)) {
        NETSDK_LOG_MESSAGE_ERROR("CVoiceComStream: send audio param failed");
        NETSDK_SOCKET_CLOSE(m_nSocket);
        m_nSocket = INVALID_SOCKET_FD;
        return false;
    }

    m_fnCallback = std::move(callback);
    m_bRunning = true;
    m_stReceiveThread = std::thread(&CVoiceComStream::recv_loop, this);

    NETSDK_LOG_MESSAGE_INFO("CVoiceComStream: connected to %s:%d", host.c_str(), nPort);
    return true;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 发送音频参数协商帧
 * @details 将音频参数封装为VCP1帧格式，包含魔数和参数数据
 * @param stAudioParam 音频参数结构体
 * @return true表示成功，false表示失败
 */
bool CVoiceComStream::send_audio_param(const NET_VoiceComAudioParam_S& stAudioParam) {
    VoiceComParamFrame_S frame{};
    frame.magic = htonl(kVoiceComParamMagic);
    frame.audio_param = stAudioParam;
    return send_frame(reinterpret_cast<const char*>(&frame), sizeof(frame));
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 发送一帧数据（带长度头）
 * @details 帧格式：[2字节长度（大端）][音频负载]，首帧负载为VCP1参数帧
 * @param pstrData 帧数据指针
 * @param uSize 帧数据大小（字节）
 * @return true表示成功，false表示失败
 */
bool CVoiceComStream::send_frame(const char* pstrData, size_t uSize) {
    if (m_nSocket == INVALID_SOCKET_FD || pstrData == nullptr || uSize == 0 || uSize > 0xFFFFu) {
        return false;
    }

    uint16_t uLenBe = htons(static_cast<uint16_t>(uSize));
    std::vector<char> frame(sizeof(uLenBe) + uSize);
    std::memcpy(frame.data(), &uLenBe, sizeof(uLenBe));
    std::memcpy(frame.data() + sizeof(uLenBe), pstrData, uSize);

#ifdef _WIN32
    int nSendFlags = 0;
#else
    int nSendFlags = MSG_NOSIGNAL;
#endif

    std::lock_guard<std::mutex> lock(m_stSendMutex);
    size_t uSent = 0;
    while (uSent < frame.size()) {
        ssize_t n = ::send(m_nSocket, frame.data() + uSent, static_cast<int>(frame.size() - uSent), nSendFlags);
        if (n <= 0) return false;
        uSent += static_cast<size_t>(n);
    }
    return true;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 发送音频数据到设备
 * @param pstrData 音频数据指针
 * @param uSize 音频数据大小（字节）
 * @return true表示成功，false表示失败
 */
bool CVoiceComStream::send(const char* pstrData, size_t uSize) {
    if (!m_bRunning || pstrData == nullptr || uSize == 0) {
        return false;
    }

#ifdef VOICECOM_DEBUG_PRINT
    const uint64_t uFrameCount = ++m_uSentFrameCount;
    if (uFrameCount <= 10 || (uFrameCount % 100) == 0) {
        char strHeadHex[64];
        format_head_hex(pstrData, uSize, strHeadHex, sizeof(strHeadHex));
        NETSDK_LOG_MESSAGE_INFO("CVoiceComStream: send audio frame=%llu bytes=%zu has_adts=%d head=%s",
                      static_cast<unsigned long long>(uFrameCount),
                      uSize,
                      has_adts_header(pstrData, uSize) ? 1 : 0,
                      strHeadHex);
    }
#endif

    return send_frame(pstrData, uSize);
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 停止并关闭连接
 * @details 完整流程：
 *          1. 设置运行标志为false
 *          2. 关闭socket连接
 *          3. 等待接收线程退出
 */
void CVoiceComStream::stop() {
    if (!m_bRunning) return;

    m_bRunning = false;
    if (m_nSocket != INVALID_SOCKET_FD) {
        shutdown(m_nSocket, SHUT_RDWR);
        NETSDK_SOCKET_CLOSE(m_nSocket);
        m_nSocket = INVALID_SOCKET_FD;
    }
    if (m_stReceiveThread.joinable()) {
        m_stReceiveThread.join();
    }
    NETSDK_LOG_MESSAGE_INFO("CVoiceComStream: stopped");
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 接收数据循环
 * @details 持续从socket读取音频帧，解析后通过回调转发给上层：
 *          1. 读取2字节帧头（大端长度）
 *          2. 根据长度读取帧数据
 *          3. 通过回调函数转发给上层
 */
void CVoiceComStream::recv_loop() {
    char strHeader[2];
    char strBuffer[4096];

    while (m_bRunning) {
        /* 读帧头: 2字节长度（大端） */
        if (!recv_exact(m_nSocket, strHeader, sizeof(strHeader))) {
            if (m_bRunning) {
                NETSDK_LOG_MESSAGE_WARN("CVoiceComStream: recv header failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
            }
            break;
        }

        uint16_t uLenBe;
        std::memcpy(&uLenBe, strHeader, sizeof(uLenBe));
        size_t uFrameLen = ntohs(uLenBe);

        if (uFrameLen == 0 || uFrameLen > sizeof(strBuffer)) {
            NETSDK_LOG_MESSAGE_WARN("CVoiceComStream: invalid frame len %zu", uFrameLen);
            continue;
        }

        /* 读帧数据 */
        if (recv_exact(m_nSocket, strBuffer, uFrameLen) && m_fnCallback) {
#ifdef VOICECOM_DEBUG_PRINT
            const uint64_t uFrameCount = ++m_uReceivedFrameCount;
            if (uFrameCount <= 10 || (uFrameCount % 100) == 0) {
                char strHeadHex[64];
                format_head_hex(strBuffer, uFrameLen, strHeadHex, sizeof(strHeadHex));
                NETSDK_LOG_MESSAGE_INFO("CVoiceComStream: recv audio frame=%llu bytes=%zu has_adts=%d head=%s",
                              static_cast<unsigned long long>(uFrameCount),
                              uFrameLen,
                              has_adts_header(strBuffer, uFrameLen) ? 1 : 0,
                              strHeadHex);
            }
#endif
            m_fnCallback(strBuffer, uFrameLen);
        }
    }
}

}  /* namespace tvsdk */
