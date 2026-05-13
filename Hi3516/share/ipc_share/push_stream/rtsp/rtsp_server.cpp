/**
 * @FilePath     : rtsp_server.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-03-29 10:05:15
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-28 14:38:21
 * @Description  : RTSP服务器
 */

#include "rtsp_server.h"
#include "convert_interface.h"
#include "time_utils.h"

/**
 * @brief RTSP帧回调函数（重构版本，使用C++容器）
 * @note 保持I帧优先处理逻辑
 */
int rtspFrameCall(Fream_Info_t* frame)
{
    if (frame == nullptr || frame->param == nullptr || frame->data == nullptr)
    {
        dlog_error("参数为空");
        return ERR_PARAM_NULL;
    }

    Live_Stream_Info_t* pStreamInfo = static_cast<Live_Stream_Info_t*>(frame->param);

    if (frame->type == AUDIO_TYPE)
    {
        /* 音频帧处理 */
        auto audioFrame = pStreamInfo->audioQueue->pop();
        if (audioFrame)
        {
            memcpy(frame->data, audioFrame->data.get(), audioFrame->frameSize);
            frame->frameSize = audioFrame->frameSize;
            /* audioFrame 在作用域结束时自动释放 */
        }
        else
        {
            frame->frameSize = 0;
        }
    }
    else
    {
        /* 视频帧处理 */
        if (pStreamInfo->requestIFrame == 1)
        {
            /* 当前处于"必须发送I帧"的模式 */
            frame->frameSize = 0;
            frame->iFrame = 0;

            /* 循环查找I帧 */
            while (!pStreamInfo->videoQueue->empty())
            {
                auto videoFrame = pStreamInfo->videoQueue->pop();
                if (videoFrame)
                {
                    if (videoFrame->iFrame == 1)
                    {
                        /* 找到了I帧，发送它并退出特殊模式 */
                        memcpy(frame->data, videoFrame->data.get(), videoFrame->frameSize);
                        frame->frameSize = videoFrame->frameSize;
                        frame->iFrame = videoFrame->iFrame;

                        /* 重置I帧请求标志，恢复正常发送模式 */
                        pStreamInfo->requestIFrame = 0;
                        dlog_debug("I帧已找到并发送给新客户端.");
                        break;
                    }
                    else
                    {
                        /* 这不是I帧，丢弃它（自动释放） */
                        dlog_debug("在等待I帧期间丢弃非I帧");
                    }
                }
            }
            /* 如果循环结束时仍未找到I帧，本次调用返回空帧，等待下一次被调用 */
        }
        else
        {
            /* 正常模式：发送队列中的任何帧 */
            auto videoFrame = pStreamInfo->videoQueue->pop();
            if (videoFrame)
            {
                memcpy(frame->data, videoFrame->data.get(), videoFrame->frameSize);
                frame->frameSize = videoFrame->frameSize;
                frame->iFrame = videoFrame->iFrame;
                /* videoFrame 在作用域结束时自动释放 */
            }
            else
            {
                frame->iFrame = 0;
                frame->frameSize = 0;
            }
        }
    }

    frame->videolistsize = static_cast<int>(pStreamInfo->videoQueue->size());
    frame->audiolistsize = static_cast<int>(pStreamInfo->audioQueue->size());
    frame->fFps = pStreamInfo->fFps;

    return OK;
}

/**
 * @brief RTSP客户端状态回调函数
 */
int rtspStateCallback(Rtsp_ClientStream_State_t* param)
{
    if (param == nullptr || param->param == nullptr)
    {
        dlog_error("live555 状态回调出错");
        return ERR;
    }
    Live_Stream_Info_t* pStreamInfo = static_cast<Live_Stream_Info_t*>(param->param);

    if (param->status == RTSPCLIENT_STOP)
    {
        pStreamInfo->request = 0;
        pStreamInfo->requestIFrame = 0; /* 客户端停止时，也清除标志 */
    }
    else if (param->status == RTSPCLIENT_START)
    {
        pStreamInfo->request = 1;
        /* 设置I帧请求标志，通知rtspFrameCall优先处理I帧 */
        pStreamInfo->requestIFrame = 1;
        /* 通过通道号确定是哪个通道 */
        int nChannel = 0;
        /*比较pStreamInfo与m_pLiveInfo->listLive[i]来确定通道号*/
        for (int i = 0; i < RTSP_CHN_MAX; i++)
        {
            if (CRtspServer::instance()->m_pLiveInfo &&
                CRtspServer::instance()->m_pLiveInfo->listLive[i] == pStreamInfo)
            {
                nChannel = i;
                break;
            }
        }
        /* 调用回调函数请求I帧 */
        CRtspServer::instance()->triggerRequestIdr(nChannel);
    }

    dlog_debug("%s", param->status == RTSPCLIENT_START ? "RTSP客户端启动" : "RTSP客户端停止");
    return OK;
}

/**
 * @brief   : 握手认证回调函数
 * @note    : 暂时未做相关逻辑
 * @param    {char} *pClientIP 握手连接成功的客户端IP
 * @return   {int} 0：成功 非0：失败
 */
int handshakeAuth_callback(char* pClientIP)
{
    if (pClientIP == NULL)
    {
        dlog_error("客户端IP参数为空");
        return ERR;
    }

    dlog_info("客户端[%s]已连接", pClientIP);
    return OK;
}

CRtspServer::CRtspServer()
    : m_strVideoConfigPath(VIDEO_CONFIG_FILE), m_strAudioConfigPath(AUDIO_CONFIG_FILE), m_strPortPath(PORT_CONFIG_FILE)
{
    m_requestIdrCallback = nullptr;
    m_pCallbackUserData = nullptr;

    /* 读取视频配置文件 */
    Convert::read_file(m_strVideoConfigPath, m_vstVideoConfig);
    /* 读取音频配置文件 */
    Convert::read_file(m_strAudioConfigPath, m_stAudioConfig);

    /* 端口配置 */
    Network::PortConfig_S stPortConfig;
    /*读取端口配置文件*/
    if (Convert::read_file(m_strPortPath, stPortConfig))
    {
        /*无端口配置文件，进行生成*/
        Convert::write_file(m_strPortPath, stPortConfig);
    }
    /* 设置RTSP端口 */
    m_nRtspPort = stPortConfig.nRtspPort;

    /* 设置QoS策略 */
    m_nMediaDscp = QOS_DSCP_MIN;
    CQosManage::instance()->get_qos_config(m_stQosConfigInfo);
    if (QOS_DSCP_MIN <= m_stQosConfigInfo.nMediaDscp && QOS_DSCP_MAX >= m_stQosConfigInfo.nMediaDscp)
    {
        m_nMediaDscp = m_stQosConfigInfo.nMediaDscp;
    }

    /* 默认加密 */
    m_bAuthentication = true;

    // 获取认证方式
    updateRtspDigestAlgorithm();
}

CRtspServer::~CRtspServer()
{
}

IpcRet_E CRtspServer::init()
{
    m_pLiveInfo = (LIVE_RTSP_S*) calloc(1, sizeof(LIVE_RTSP_S));
    if (m_pLiveInfo == NULL)
    {
        dlog_error("rtsp服务器初始化失败,m_pLiveInfo分配内存失败");
        return ERR_PARAM_NULL;
    }

    m_pLiveInfo->nPort = m_nRtspPort;
    m_pLiveInfo->pServerHandle = rtsp_server_init(m_nRtspPort,
                                                  m_bAuthentication,
                                                  m_strUser.c_str(),
                                                  m_strPwd.c_str(),
                                                  m_nRtspDigestAlgorithm,
                                                  m_nMediaDscp);
    if (m_pLiveInfo->pServerHandle == NULL)
    {
        dlog_error("rtsp服务器初始化失败,端口:%d", m_nRtspPort);
        return ERR_PARAM_NULL;
    }

    /*获取网络信息*/
    Network::Info_S stNetInfo;
    CNetworkManage::instance()->get_system_networkInfo(stNetInfo);

    /* 设置握手认证回调函数 */
    set_handshakeAuth_callback(m_pLiveInfo->pServerHandle, handshakeAuth_callback);

    // 两路
    for (int i = 0; i < RTSP_CHN_MAX; i++)
    {
        m_pLiveInfo->listLive[i] = (Live_Stream_Info_t*) calloc(1, sizeof(Live_Stream_Info_t));
        if (!m_pLiveInfo->listLive[i])
        {
            return ERR_PARAM_NULL;
        }
        m_pLiveInfo->listLive[i]->requestIFrame = 0;
        m_pLiveInfo->listLive[i]->fFps = m_vstVideoConfig[i].getFrameRateAsFloat();

        memset(&m_stClientInfo[i], 0, sizeof(Rtsp_Create_Info_t));

        m_pLiveInfo->listLive[i]->nPort = m_nRtspPort;
        snprintf(m_pLiveInfo->listLive[i]->ip,
                 sizeof(m_pLiveInfo->listLive[i]->ip),
                 "%s",
                 stNetInfo.stIp.ipv4Ip.c_str());
        snprintf(m_pLiveInfo->listLive[i]->achUrl,
                 sizeof(m_pLiveInfo->listLive[i]->achUrl),
                 RTSP_URL_DEFAULT,
                 m_pLiveInfo->listLive[i]->ip,
                 m_pLiveInfo->listLive[i]->nPort,
                 i + 101);

        /* 判断是否需要鉴权 */
        if (m_bAuthentication)
        {
            auto& Url = m_rtspUrlMap[i];
            Url.resize(128);
            snprintf(Url.data(),
                     Url.size(),
                     RTSP_URL_AUTHENTICATION_DEFAULT,
                     m_strUser.c_str(),
                     m_strPwd.c_str(),
                     m_pLiveInfo->listLive[i]->ip,
                     m_pLiveInfo->listLive[i]->nPort,
                     i + 101);
        }
        else
        {
            auto& Url = m_rtspUrlMap[i];
            Url = m_pLiveInfo->listLive[i]->achUrl;
        }

        /* URL格式  第一码流： rtsp://admin:zfrl@168@ip:port/Streaming/Channels/101 */
        /* 以此类推 第二码流： rtsp://admin:zfrl@168@ip:port/Streaming/Channels/102 */
        // dlog_info("Rtsp url: %s", m_pLiveInfo->listLive[i]->achUrl);
        // dlog_info("Rtsp url: %s", m_rtspUrlMap[i].c_str());
        if (m_vstVideoConfig[i].enVideoCodec == Video_NS::VideoCodec_E::H264)
        {
            m_stClientInfo[i].nProtolType = RTSP_FRAMEPROTOL_H264;
        }
        else if (m_vstVideoConfig[i].enVideoCodec == Video_NS::VideoCodec_E::H265)
        {
            m_stClientInfo[i].nProtolType = RTSP_FRAMEPROTOL_H265;
        }
        else if (m_vstVideoConfig[i].enVideoCodec == Video_NS::VideoCodec_E::MJPEG)
        {
            m_stClientInfo[i].nProtolType = RTSP_FRAMEPROTOL_MJPEG;
        }

        strcpy(m_pLiveInfo->listLive[i]->ip, "127.0.0.1");
        /* 创建线程安全的帧队列 */
        m_pLiveInfo->listLive[i]->videoQueue = std::make_unique<CThreadSafeFrameQueue>(MAX_VIDEO_FRAME);
        m_pLiveInfo->listLive[i]->audioQueue = std::make_unique<CThreadSafeFrameQueue>(MAX_AUDIO_FRAME);

        sprintf(m_pLiveInfo->listLive[i]->streamName, "Streaming/Channels/%d", i + 101);
        m_stClientInfo[i].Videoindex = m_pLiveInfo->listLive[i];
        m_stClientInfo[i].clientFun = rtspStateCallback;
        m_stClientInfo[i].dataGetfun = rtspFrameCall;

        /*控制流是否包含音频*/
        if (m_vstVideoConfig[i].enVideoType == Video_NS::VideoType_E::COMPOSITE_STREAM)
        {
            m_stClientInfo[i].Audioindex = m_pLiveInfo->listLive[i];
        }
        else if (m_vstVideoConfig[i].enVideoType == Video_NS::VideoType_E::VIDEO_STREAM)
        {
            m_stClientInfo[i].Audioindex = NULL;
        }

        /*音频*/
        switch (m_stAudioConfig.enFormat) // 0:aac 1:g711a 2:g711u
        {
        case Audio_NS::AudioFormat_E::G711U:
            m_stClientInfo[i].nAudioType = 1;
            break;
        case Audio_NS::AudioFormat_E::G711A:
            m_stClientInfo[i].nAudioType = 2;
            break;
        case Audio_NS::AudioFormat_E::G726:
            m_stClientInfo[i].nAudioType = 3;
            m_stClientInfo[i].nAudioBitWidth = 4; // 默认G.726-32的位深度为4位/样本，对应32kbps
            // note:当前仅支持默认G.726-32即可
            //  m_stClientInfo[i].nAudioBitWidth = (int)(m_stAudioConfig.enBitRate) / 8000; //码率转采样位深
            break;
        case Audio_NS::AudioFormat_E::AAC:
            m_stClientInfo[i].nAudioType = 0;
            m_stClientInfo[i].nAudioChannel = 1; // 默认单声道
            /*设置AAC 输出采样率的ADTS头*/
            switch (m_stAudioConfig.enSampRate)
            {
            case Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_8000:
                m_stClientInfo[i].nAuidoSamplingFreqIndex = 11;
                break;
            case Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_16000:
                m_stClientInfo[i].nAuidoSamplingFreqIndex = 8;
                break;
            case Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_32000:
                m_stClientInfo[i].nAuidoSamplingFreqIndex = 5;
                break;
            case Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_48000:
                m_stClientInfo[i].nAuidoSamplingFreqIndex = 3;
                break;
            default:
                dlog_error("不支持的AAC采样率");
                m_stClientInfo[i].Audioindex = NULL;
                break;
            }
            break;
        default:
            dlog_error("不支持的音频格式");
            m_stClientInfo[i].Audioindex = NULL;
            break;
        }

        /* 拓展参数为最大client个数 */
        m_stClientInfo[i].param1 = MAX_CLIENT_NUM;
        strcpy(m_stClientInfo[i].streamName, m_pLiveInfo->listLive[i]->streamName);

        rtsp_server_create(m_pLiveInfo->pServerHandle, &m_stClientInfo[i]);
    }

    m_bInitFlag.store(true);
    dlog_info("rtsp服务器初始化成功");
    return OK;
}

IpcRet_E CRtspServer::deinit()
{
    m_bInitFlag.store(false);
    for (int i = 0; i < RTSP_CHN_MAX; i++)
    {
        /* 清空队列（智能指针会自动释放内存） */
        if (m_pLiveInfo->listLive[i]->videoQueue)
        {
            m_pLiveInfo->listLive[i]->videoQueue->clear();
        }
        if (m_pLiveInfo->listLive[i]->audioQueue)
        {
            m_pLiveInfo->listLive[i]->audioQueue->clear();
        }
        rtsp_server_destory(m_pLiveInfo->pServerHandle, m_stClientInfo[i].streamName);
        /* 释放 Live_Stream_Info_t（队列的智能指针会自动释放） */
        free(m_pLiveInfo->listLive[i]);
        m_pLiveInfo->listLive[i] = nullptr;
    }

    rtsp_server_unInit(m_pLiveInfo->pServerHandle);
    free(m_pLiveInfo);
    m_pLiveInfo = nullptr;

    dlog_info("rtsp服务器去初始化成功");
    return OK;
}

bool CRtspServer::isInit()
{
    return m_bInitFlag.load();
}

IpcRet_E CRtspServer::reboot()
{
    int nRet = OK;
    std::lock_guard<std::mutex> lock(m_mutexCtrl);

    if (m_bInitFlag.load())
    {
        nRet = deinit();
        if (nRet < 0)
        {
            dlog_error("反初始化rtsp服务器失败");
            return ERR;
        }
    }
    nRet = init();
    if (nRet < 0)
    {
        dlog_error("初始化rtsp服务器失败");
        return ERR;
    }

    return OK;
}

int CRtspServer::sendVideoData(int nChannel, Video_NS::VideoFrame_S* pVideoFrame)
{
    if (!m_bInitFlag.load())
    {
        return ERR;
    }

    int nRet = OK;
    Live_Stream_Info_t* pStreamInfo = m_pLiveInfo->listLive[nChannel];

    if (pStreamInfo->request == 1)
    {
        /* 创建帧数据（使用智能指针管理内存） */
        auto frameData = std::make_unique<FrameData>();
        frameData->data = std::make_unique<unsigned char[]>(pVideoFrame->nLen);
        frameData->type = VIDEO_TYPE;
        frameData->frameSize = pVideoFrame->nLen;

        /* 一次性拷贝数据 */
        memcpy(frameData->data.get(), pVideoFrame->pData, pVideoFrame->nLen);

        /* 判断是否为I帧 */
        if (pVideoFrame->enVideoCodec == Video_NS::VideoCodec_E::H264)
        {
            frameData->iFrame = (pVideoFrame->eType == Video_NS::H264_TYPE_SPS) ? 1 : 0;
        }
        else if (pVideoFrame->enVideoCodec == Video_NS::VideoCodec_E::H265)
        {
            frameData->iFrame = (pVideoFrame->eType == Video_NS::H265_TYPE_VPS) ? 1 : 0;
        }
        else if (pVideoFrame->enVideoCodec == Video_NS::VideoCodec_E::MJPEG)
        {
            frameData->iFrame = 1;
        }

        /* 入队（队列满时丢帧，智能指针自动释放） */
        if (!pStreamInfo->videoQueue->push(std::move(frameData)))
        {
            /* 队列已满，frameData 在此自动释放（丢帧策略） */
            dlog_debug("视频队列已满，丢弃帧");
        }
    }
    else
    {
        /* 客户端未请求时，清空队列（智能指针自动释放内存） */
        pStreamInfo->videoQueue->clear();
    }

    return nRet;
}

int CRtspServer::sendAudioData(int nChannel, Audio_NS::AudioFrame_S* pAudioFrame)
{
    if (!m_bInitFlag.load() || m_stClientInfo[nChannel].Audioindex == nullptr) // 通道无音频需求时，送音频数据直接返回
    {
        return ERR;
    }

    int nRet = OK;
    Live_Stream_Info_t* pStreamInfo = m_pLiveInfo->listLive[nChannel];

    if (pStreamInfo->request == 1)
    {
        /* 创建帧数据（使用智能指针管理内存） */
        auto frameData = std::make_unique<FrameData>();
        frameData->data = std::make_unique<unsigned char[]>(pAudioFrame->nLen);
        frameData->type = AUDIO_TYPE;
        frameData->frameSize = pAudioFrame->nLen;

        /* 一次性拷贝数据 */
        memcpy(frameData->data.get(), pAudioFrame->pData, pAudioFrame->nLen);

        /* 入队（队列满时丢帧，智能指针自动释放） */
        if (!pStreamInfo->audioQueue->push(std::move(frameData)))
        {
            /* 队列已满，frameData 在此自动释放（丢帧策略） */
            dlog_debug("音频队列已满，丢弃帧");
        }
    }
    else
    {
        /* 客户端未请求时，清空队列（智能指针自动释放内存） */
        pStreamInfo->audioQueue->clear();
    }

    return nRet;
}

int CRtspServer::setVideoConfig(const std::vector<Video_NS::VideoConfig_S>& vstVideoConfig)
{
    m_vstVideoConfig = vstVideoConfig;
    return OK;
}

int CRtspServer::setAudioConfig(const Audio_NS::AudioConfig_S& stAudioConfig)
{
    m_stAudioConfig = stAudioConfig;
    return OK;
}

int CRtspServer::updateNetworkConfig(const Network::Info_S& stInfo)
{
    for (int i = 0; i < RTSP_CHN_MAX; i++)
    {
        snprintf(m_pLiveInfo->listLive[i]->ip, sizeof(m_pLiveInfo->listLive[i]->ip), "%s", stInfo.stIp.ipv4Ip.c_str());
        snprintf(m_pLiveInfo->listLive[i]->achUrl,
                 sizeof(m_pLiveInfo->listLive[i]->achUrl),
                 RTSP_URL_DEFAULT,
                 m_pLiveInfo->listLive[i]->ip,
                 m_pLiveInfo->listLive[i]->nPort,
                 i + 101);

        /* 判断是否需要鉴权 */
        if (m_bAuthentication)
        {
            auto& Url = m_rtspUrlMap[i];
            Url.resize(128);
            snprintf(Url.data(),
                     Url.size(),
                     RTSP_URL_AUTHENTICATION_DEFAULT,
                     m_strUser.c_str(),
                     m_strPwd.c_str(),
                     m_pLiveInfo->listLive[i]->ip,
                     m_pLiveInfo->listLive[i]->nPort,
                     i + 101);
        }
        else
        {
            auto& Url = m_rtspUrlMap[i];
            Url = m_pLiveInfo->listLive[i]->achUrl;
        }

        /* URL格式  第一码流： rtsp://admin:zfrl@168@ip:port/Streaming/Channels/101 */
        /* 以此类推 第二码流： rtsp://admin:zfrl@168@ip:port/Streaming/Channels/102 */
        dlog_info("Rtsp url: %s", m_pLiveInfo->listLive[i]->achUrl);
        dlog_info("Rtsp url: %s", m_rtspUrlMap[i].c_str());
    }
    return OK;
}

int CRtspServer::setPort(const int& nPort)
{
    m_nRtspPort = nPort;
    reboot();
    return OK;
}

int CRtspServer::setQosDscp(const int& nDscp)
{
    if (QOS_DSCP_MIN <= nDscp && QOS_DSCP_MAX >= nDscp)
    {
        m_nMediaDscp = nDscp;
        reboot();
        return OK;
    }
    return ERR_PARAM;
}

char* CRtspServer::getRtspUrl(int nChn, bool bAuth)
{
    if (nChn < RTSP_CHN_MAIN || nChn >= RTSP_CHN_MAX)
    {
        dlog_error("Rtsp通道号错误");
        return nullptr;
    }

    if (!m_bInitFlag.load())
    {
        dlog_error("Rtsp未初始化");
        return nullptr;
    }

    if (bAuth)
    {
        return m_rtspUrlMap[nChn].data();
    }
    else
    {
        switch (nChn)
        {
        default:
        case RTSP_CHN_MAIN:
            return m_pLiveInfo->listLive[RTSP_CHN_MAIN]->achUrl;
        case RTSP_CHN_SUB:
            return m_pLiveInfo->listLive[RTSP_CHN_SUB]->achUrl;
        }
    }
}

int CRtspServer::setRequestIdrCallback(const RequestIdrCallback& callback, void* pUserData)
{
    if (!callback)
    {
        dlog_error("无效的回调函数 (nullptr或未绑定)");
        return ERR;
    }

    m_requestIdrCallback = callback;
    m_pCallbackUserData = pUserData;

    dlog_info("RtspServer 回调设置成功");
    return OK;
}

int CRtspServer::triggerRequestIdr(int nChannel)
{
    /* MJPEG不支持请求IDR帧 */
    if (m_vstVideoConfig[nChannel].enVideoCodec == Video_NS::VideoCodec_E::MJPEG)
    {
        return OK;
    }

    std::lock_guard<std::mutex> lock(m_mutexCtrl);

    /* 获取当前时间 */
    auto now = TimeUtils_NS::get_currentTimestampMs();
    /* 查找该通道上次触发时间（如果不存在，会自动创建并初始化为默认值——零时间点） */
    auto& lastTime = m_lastIdrRequestTimeMap[nChannel];
    /* 计算时间间隔 */
    auto elapsedMs = now - lastTime;
    /* 如果间隔太短，拒绝执行 */
    if (elapsedMs < m_minIdrInterval)
    {
        dlog_warn("通道 %d IDR 请求过于频繁，已忽略。距上次请求仅 %lld 毫秒", nChannel, elapsedMs);
        return OK;
    }

    if (m_requestIdrCallback)
    {
        /* 判断 StreamVideo 是否初始化完毕，否则调用接口会出现崩溃 */
        if (*static_cast<bool*>(m_pCallbackUserData) == true)
        {
            m_requestIdrCallback(nChannel, m_pCallbackUserData);

            /* 更新该通道的最后触发时间 */
            lastTime = now;
        }
        else
        {
            dlog_warn("StreamVideo 未初始化完毕");
        }
    }
    else
    {
        dlog_warn("RtspServer 未设置回调函数");
        return ERR;
    }
    return OK;
}

int CRtspServer::reset_lastIdrRequestTime()
{
    dlog_trace("重置RtspServer上次请求IDR帧时间");
    std::lock_guard<std::mutex> lock(m_mutexCtrl);
    m_lastIdrRequestTimeMap.clear();
    return OK;
}

int CRtspServer::update_userInfo(std::string strUser, std::string strPwd, bool bReboot)
{
    if (strUser.empty() || strPwd.empty())
    {
        return ERR_PARAM_NULL;
    }

    m_strUser = strUser;
    m_strPwd = strPwd;

    if (bReboot)
    {
        reboot();
    }

    return OK;
}

int CRtspServer::updateRtspDigestAlgorithm()
{
    System::SecurityCert_S stSecurityCert;
    Convert::read_file(SECURITY_CERT_CONFIG_FILE, stSecurityCert);

    if ((int) stSecurityCert.enRtspDigestAlgorithm != m_nRtspDigestAlgorithm)
    {
        m_nRtspDigestAlgorithm = int(stSecurityCert.enRtspDigestAlgorithm);
        dlog_debug("rtsp 摘要算法设置 %d", m_nRtspDigestAlgorithm);
    }
    return OK;
}
