/**
 * @file VoiceComHub.cpp
 * @brief 语音对讲汇聚管理实现 - 启动/发送/停止语音对讲，含音频参数校验
 */

#include "VoiceComHub.h"

/* ========================================================================== */
/*  NormalizeAudioParam                                                        */
/* ========================================================================== */

bool CVoiceComHub::NormalizeAudioParam(NET_VoiceComAudioParam_S& audioParam)
{
    if (audioParam.uChannels != 1) {
        return false;
    }

    int bytesPerSample = 0;
    switch (audioParam.enFormat) {
        case NET_AUDIO_FORMAT_PCM:
        {
            if (audioParam.uBitDepth <= 0) {
                audioParam.uBitDepth = 16;
            }
            if (audioParam.uBitDepth != 16) {
                return false;
            }
            switch (audioParam.uSampleRate) {
                case NET_AUDIO_SAMPRATE_8000:
                case NET_AUDIO_SAMPRATE_16000:
                    break;
                default:
                    return false;
            }
            bytesPerSample = audioParam.uBitDepth / 8;
            break;
        }
        case NET_AUDIO_FORMAT_AAC:
        {
            if (audioParam.uSampleRate <= 0) {
                audioParam.uSampleRate = NET_AUDIO_SAMPRATE_16000;
            }
            if (audioParam.uBitDepth <= 0) {
                audioParam.uBitDepth = 16;
            }
            if (audioParam.uFrameIntervalMs <= 0) {
                audioParam.uFrameIntervalMs = 64;
            }
            if (audioParam.uFrameBytes <= 0) {
                audioParam.uFrameBytes = NET_LEN_4096;
            }
            if (audioParam.uFrameBytes > NET_LEN_4096) {
                return false;
            }
            if (audioParam.uBitRate <= 0) {
                audioParam.uBitRate = 48000;
            }
            audioParam.bLittleEndian = TRUE;
            return true;
        }
        case NET_AUDIO_FORMAT_G711A:
        case NET_AUDIO_FORMAT_G711U:
        {
            if (audioParam.uSampleRate != NET_AUDIO_SAMPRATE_8000) {
                return false;
            }
            if (audioParam.uBitDepth <= 0) {
                audioParam.uBitDepth = 8;
            }
            if (audioParam.uBitDepth != 8) {
                return false;
            }
            bytesPerSample = 1;
            break;
        }
        default:
            return false;
    }

    if (audioParam.uFrameIntervalMs <= 0) {
        audioParam.uFrameIntervalMs = 20;
    }
    if (audioParam.uFrameIntervalMs < 10 || audioParam.uFrameIntervalMs > 1000) {
        return false;
    }

    const int frameBytes = audioParam.uSampleRate * audioParam.uChannels *
                           bytesPerSample * audioParam.uFrameIntervalMs / 1000;
    if (frameBytes <= 0 || frameBytes > NET_LEN_4096) {
        return false;
    }

    if (audioParam.uFrameBytes <= 0) {
        audioParam.uFrameBytes = frameBytes;
    }
    if (audioParam.uFrameBytes != frameBytes) {
        return false;
    }

    audioParam.uBitRate = audioParam.uSampleRate * audioParam.uChannels * audioParam.uBitDepth;
    audioParam.bLittleEndian = TRUE;
    return true;
}

/* ========================================================================== */
/*  Start                                                                      */
/* ========================================================================== */

BOOL CVoiceComHub::Start(LPVOID lpUserID,
                              pNET_VoiceComStartInfo_S pstStartInfo,
                              NET_VoiceComCallBack cbVoiceCom,
                              LPVOID lpUserData)
{
    if (!lpUserID || !pstStartInfo || pstStartInfo->uAudioPort == 0) {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    NET_VoiceComAudioParam_S audioParam = pstStartInfo->stAudioParam;
    if (!NormalizeAudioParam(audioParam)) {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    auto session = CSessionManager::instance()->GetSession(lpUserID);
    if (!session) {
        CErrorManage::instance()->SetLastError(NET_E_NO_USER);
        return FALSE;
    }

    const std::string host = session->GetHost();
    if (host.empty()) {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    auto client = std::make_shared<tvsdk::CVoiceComStream>();

    /* 绑定 C 回调到 C++ callback */
    tvsdk::VoiceComCallback cb = [cbVoiceCom, lpUserData](const char* data, size_t size) {
        if (cbVoiceCom) {
            cbVoiceCom(data, static_cast<unsigned int>(size), lpUserData);
        }
    };

    if (!client->start(host, static_cast<int>(pstStartInfo->uAudioPort), audioParam, std::move(cb))) {
        CErrorManage::instance()->SetLastError(NET_E_SYSCALL_FALIED);
        return FALSE;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto old = m_comMap.find(lpUserID);
        if (old != m_comMap.end()) {
            old->second->stop();
            m_comMap.erase(old);
        }
        m_comMap[lpUserID] = client;
    }

    return TRUE;
}

/* ========================================================================== */
/*  SendData                                                                   */
/* ========================================================================== */

BOOL CVoiceComHub::SendData(LPVOID lpUserID, const CHAR* pData, UINT32 dwSize)
{
    if (!lpUserID || !pData || dwSize == 0) {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_comMap.find(lpUserID);
    if (it == m_comMap.end() || !it->second->is_running()) {
        CErrorManage::instance()->SetLastError(NET_E_AUDIO_NO_EXISTED);
        return FALSE;
    }

    if (!it->second->send(pData, static_cast<size_t>(dwSize))) {
        CErrorManage::instance()->SetLastError(NET_E_AUDIO_FAILED);
        return FALSE;
    }
    return TRUE;
}

/* ========================================================================== */
/*  Stop                                                                       */
/* ========================================================================== */

BOOL CVoiceComHub::Stop(LPVOID lpUserID)
{
    if (!lpUserID) {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_comMap.find(lpUserID);
    if (it == m_comMap.end()) {
        CErrorManage::instance()->SetLastError(NET_E_AUDIO_NO_EXISTED);
        return FALSE;
    }

    it->second->stop();
    m_comMap.erase(it);
    return TRUE;
}

/* ========================================================================== */
/*  Cleanup                                                                    */
/* ========================================================================== */

void CVoiceComHub::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_comMap)
    {
        pair.second->stop();
    }
    m_comMap.clear();
}
