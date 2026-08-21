/**
 * @FilePath     : rtsp_server.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-03-29 10:05:15
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-20 15:59:01
 * @Description  : RTSP服务器
 */

#include "rtsp_server.h"

#include <cstdio>
#include <cstring>

#include "convert_interface.h"
#include "time_utils.h"

/* 丢帧期间日志打印最小间隔（毫秒），避免持续丢帧时刷屏 */
#define QUEUE_DROP_LOG_INTERVAL_MS (1000)

namespace
{
/**
 * @brief   : 取得主码流用于限流判断的码率
 * @param   {const std::vector<Video_NS::VideoConfig_S>&} configs：当前视频配置
 * @return  {int} 码率，单位 kbps；配置缺失或非法时返回 0
 * @note    : 使用 nBitrateUpperLimit 的最坏情况预算，不用较低的平均码率放宽限制。
 */
int get_main_bitrate_kbps(const std::vector<Video_NS::VideoConfig_S>& configs)
{
    if (configs.size() <= static_cast<std::size_t>(RTSP_CHN_MAIN))
    {
        return 0;
    }

    const Video_NS::VideoConfig_S& config = configs[RTSP_CHN_MAIN];
    return config.nBitrateUpperLimit > 0 ? config.nBitrateUpperLimit : 0;
}

/**
 * @brief   : 根据主码流码率计算主码流会话上限
 * @param   {int} nBitrateKbps：主码流码率，单位 kbps
 * @return  {int} 当前设备主码流会话上限
 */
int get_main_client_limit(const int nBitrateKbps)
{
    int nLimit = RTSP_MAIN_CLIENT_LIMIT_DEFAULT;
    if (nBitrateKbps >= RTSP_MAIN_BITRATE_THRESHOLD_16M)
    {
        nLimit = RTSP_MAIN_CLIENT_LIMIT_16M;
    }
    else if (nBitrateKbps >= RTSP_MAIN_BITRATE_THRESHOLD_8M)
    {
        nLimit = RTSP_MAIN_CLIENT_LIMIT_8M;
    }
    return nLimit * RTSP_CLIENT_CAPACITY_MULTIPLIER;
}

/**
 * @brief   : 读取一个 RTSP 会话当前的 live555 引用计数
 * @param   {RtSpServerHandle_t} pServerHandle：RTSP服务句柄
 * @param   {const char*} pStreamName：媒体会话名称
 * @param   {int&} nClientCount：输出当前引用计数
 * @return  {bool} 是否读取成功
 */
bool read_rtsp_client_count(RtSpServerHandle_t pServerHandle, const char* pStreamName, int& nClientCount)
{
    if (pServerHandle == nullptr || pStreamName == nullptr)
    {
        return false;
    }

    Rtsp_Client_Info_t stClientInfo{};
    if (rtsp_getclient_info(pServerHandle, pStreamName, &stClientInfo) != OK)
    {
        return false;
    }

    nClientCount = stClientInfo.nNumClient < 0 ? 0 : stClientInfo.nNumClient;
    return true;
}
}

void CRtspServer::report_queue_drop(QueueDiag_S &stDiag, const char *strType, int nChannel)
{
    const long long llNow = TimeUtils_NS::get_currentTimestampMs();

    if (!stDiag.bDropping)
    {
        /* 丢帧开始：记录起始时间并打印首条警告 */
        stDiag.bDropping = true;
        stDiag.nDropCount = 1;
        stDiag.llFirstDropMs = llNow;
        stDiag.llLastDropLogMs = llNow;
        dlog_warn("RTSP%s队列已满开始丢帧 chn:%d 距上次成功入队:%lldms",
                  strType,
                  nChannel,
                  stDiag.llLastPushOkMs > 0 ? (llNow - stDiag.llLastPushOkMs) : 0);
        return;
    }

    stDiag.nDropCount++;

    /* 持续丢帧期间每秒汇总打印一次 */
    if (llNow - stDiag.llLastDropLogMs >= QUEUE_DROP_LOG_INTERVAL_MS)
    {
        stDiag.llLastDropLogMs = llNow;
        dlog_warn("RTSP%s队列持续已满 chn:%d 累计丢帧:%d 已持续:%lldms",
                  strType,
                  nChannel,
                  stDiag.nDropCount,
                  llNow - stDiag.llFirstDropMs);
    }
}

void CRtspServer::report_queue_recover(QueueDiag_S &stDiag, const char *strType, int nChannel)
{
    const long long llNow = TimeUtils_NS::get_currentTimestampMs();

    stDiag.llLastPushOkMs = llNow;

    if (stDiag.bDropping)
    {
        /*
         * 丢帧结束：打印本轮丢帧总数与持续时长。
         * 丢帧持续时长表示连续入队失败窗口，不能单独等同于socket阻塞；
         * 需要结合RTSP消费线程和系统I/O诊断判断卡顿位置。
         */
        dlog_warn("RTSP%s队列恢复 chn:%d 本轮丢帧:%d 持续:%lldms",
                  strType,
                  nChannel,
                  stDiag.nDropCount,
                  llNow - stDiag.llFirstDropMs);
        stDiag.bDropping = false;
        stDiag.nDropCount = 0;
        stDiag.llFirstDropMs = 0;
    }
}

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
    // dlog_warn("RTSP取帧 waitKey:%d queue:%zu",
    //     pStreamInfo->requestIFrame,
    //     pStreamInfo->videoQueue->size());
    return OK;
}

/**
 * @brief RTSP客户端状态回调函数
 * @param {Rtsp_ClientStream_State_t*} param：live555传入的状态和流上下文
 * @return {int} 0：处理成功，非0：参数错误或超过策略上限
 * @note
 *   旧版 live555 在 DESCRIBE/SETUP/PLAY 的多个阶段都会触发 START，音频和视频
 *   也可能各触发一次。因此这里不按回调次数累加，而是读取媒体会话当前的
 *   referenceCount 作为连接数；DESCRIBE 的临时引用可能短暂计入，但不会把附加
 *   Track 的回调次数重复累加，也不会因 SDP 探测把队列误清空。
 */
int rtspStateCallback(Rtsp_ClientStream_State_t* param)
{
    if (param == nullptr || param->param == nullptr)
    {
        dlog_error("live555状态回调参数为空");
        return ERR_PARAM_NULL;
    }

    CRtspServer* pRtspServer = CRtspServer::instance();
    if (pRtspServer->m_pLiveInfo == nullptr || pRtspServer->m_pLiveInfo->pServerHandle == nullptr)
    {
        dlog_error("RTSP状态回调时服务器未初始化");
        return ERR_UNINIT;
    }

    Live_Stream_Info_t* pStreamInfo = static_cast<Live_Stream_Info_t*>(param->param);
    int nChannel = -1;
    for (int i = 0; i < RTSP_CHN_MAX; ++i)
    {
        if (pRtspServer->m_pLiveInfo->listLive[i] == pStreamInfo)
        {
            nChannel = i;
            break;
        }
    }
    if (nChannel < RTSP_CHN_MAIN || nChannel >= RTSP_CHN_MAX)
    {
        dlog_error("RTSP状态回调找不到流上下文");
        return ERR_PARAM;
    }

    const int nMainBitrateKbps = get_main_bitrate_kbps(pRtspServer->getVideoConfig());
    const int nChannelLimit = nChannel == RTSP_CHN_MAIN
                                  ? get_main_client_limit(nMainBitrateKbps)
                                  : RTSP_GLOBAL_MAX_CLIENT;
    const int nGlobalLimit = RTSP_GLOBAL_MAX_CLIENT;

    int nChannelCount = 0;
    int nGlobalCount = 0;
    bool bChannelCountValid = read_rtsp_client_count(pRtspServer->m_pLiveInfo->pServerHandle,
                                                     pStreamInfo->streamName,
                                                     nChannelCount);
    bool bGlobalCountValid = bChannelCountValid;
    for (int i = 0; i < RTSP_CHN_MAX; ++i)
    {
        if (i == nChannel)
        {
            nGlobalCount += nChannelCount;
            continue;
        }

        if (pRtspServer->m_pLiveInfo->listLive[i] == nullptr)
        {
            bGlobalCountValid = false;
            continue;
        }

        int nOtherCount = 0;
        if (!read_rtsp_client_count(pRtspServer->m_pLiveInfo->pServerHandle,
                                     pRtspServer->m_pLiveInfo->listLive[i]->streamName,
                                     nOtherCount))
        {
            bGlobalCountValid = false;
            continue;
        }
        nGlobalCount += nOtherCount;
    }

    if (param->status == RTSPCLIENT_ADMISSION)
    {
        /*
         * 首个 SETUP 尚未增加 live555 引用计数，此处使用 >= 判断：当前值达到任一
         * 上限时拒绝候选连接。准入回调不修改 request/关键帧状态，避免被拒绝的
         * 客户端改变已有连接的供帧状态；被拒连接不会增加引用计数，库侧回复453。
         */
        if (!bChannelCountValid || !bGlobalCountValid)
        {
            dlog_error("RTSP准入计数读取失败 stream:%s chn:%d channel:%d/%d total:%d/%d",
                       pStreamInfo->streamName,
                       nChannel,
                       nChannelCount,
                       nChannelLimit,
                       nGlobalCount,
                       nGlobalLimit);
            return ERR;
        }

        const bool bGlobalLimitReached = nGlobalCount >= nGlobalLimit;
        const bool bChannelLimitReached = nChannelCount >= nChannelLimit;
        if (bGlobalLimitReached || bChannelLimitReached)
        {
            const char* strReason = bGlobalLimitReached ? "global_limit" : "channel_limit";
            dlog_warn("RTSP客户端拒绝 stream:%s chn:%d bitrate:%dkbps "
                      "channel:%d/%d total:%d/%d capability:x%d reason:%s",
                      pStreamInfo->streamName,
                      nChannel,
                      nMainBitrateKbps,
                      nChannelCount,
                      nChannelLimit,
                      nGlobalCount,
                      nGlobalLimit,
                      RTSP_CLIENT_CAPACITY_MULTIPLIER,
                      strReason);
            return ERR;
        }

        return OK;
    }

    if (param->status == RTSPCLIENT_START)
    {
        /*
         * 先维持原有供帧逻辑，再按当前连接数判断是否超限。即使这是 DESCRIBE
         * 阶段的临时 START，也必须打开 request，否则 H264/H265 无法拿到
         * SPS/PPS/VPS 生成 SDP。
         */
        const bool bNeedIdr = pStreamInfo->request == 0;
        pStreamInfo->request = 1;
        pStreamInfo->requestIFrame = 1;

        if (bChannelCountValid && bGlobalCountValid &&
            (nChannelCount > nChannelLimit || nGlobalCount > nGlobalLimit))
        {
            /*
             * ! 这是旧版 Track START 回调的兼容判断：正式 SETUP 已由库侧
             * RTSPCLIENT_ADMISSION 在引用计数递增前拦截；若混用旧库，回调返回值
             * 仍可能被忽略，因此这里只记录超限候选并保留已有连接。
             */
            dlog_warn("RTSP新连接超过上限，保留已有连接 stream:%s chn:%d bitrate:%dkbps "
                      "channel:%d/%d total:%d/%d capability:x%d",
                      pStreamInfo->streamName,
                      nChannel,
                      nMainBitrateKbps,
                      nChannelCount,
                      nChannelLimit,
                      nGlobalCount,
                      nGlobalLimit,
                      RTSP_CLIENT_CAPACITY_MULTIPLIER);
            if (bNeedIdr)
            {
                pRtspServer->triggerRequestIdr(nChannel);
            }
            return ERR;
        }

        dlog_info("RTSP客户端连接 stream:%s chn:%d bitrate:%dkbps "
                  "channel:%d/%d total:%d/%d capability:x%d%s",
                  pStreamInfo->streamName,
                  nChannel,
                  nMainBitrateKbps,
                  nChannelCount,
                  nChannelLimit,
                  nGlobalCount,
                  nGlobalLimit,
                  RTSP_CLIENT_CAPACITY_MULTIPLIER,
                  bChannelCountValid && bGlobalCountValid ? "" : " count_unavailable");
        if (bNeedIdr)
        {
            const int nIdrRet = pRtspServer->triggerRequestIdr(nChannel);
            if (nIdrRet != OK)
            {
                dlog_warn("RTSP客户端连接后请求IDR失败 chn:%d ret:%d", nChannel, nIdrRet);
            }
        }
        return OK;
    }

    if (param->status == RTSPCLIENT_STOP)
    {
        /*
         * deleteStream/source 析构回调在 live555 引用计数递减前后都可能触发，
         * 因此按“观察值减一、不低于0”估算断开后的连接数；重复 STOP 只会再次
         * 得到0，不会出现负数，也不会误清理其他码流的计数。
         */
        const bool bHadRequestedStream = pStreamInfo->request == 1;
        const int nChannelAfterStop = nChannelCount > 0 ? nChannelCount - 1 : 0;
        const int nGlobalAfterStop = nGlobalCount > 0 ? nGlobalCount - 1 : 0;
        if (!bHadRequestedStream && nChannelAfterStop == 0)
        {
            /* 音视频源析构可能分别回调 STOP；首个 STOP 已清空供帧状态，后续只保留调试痕迹。 */
            dlog_debug("RTSP重复断开回调已忽略 stream:%s chn:%d", pStreamInfo->streamName, nChannel);
            return OK;
        }
        if (nChannelAfterStop == 0)
        {
            pStreamInfo->request = 0;
            pStreamInfo->requestIFrame = 0;
        }

        dlog_info("RTSP客户端断开 stream:%s chn:%d bitrate:%dkbps "
                  "channel:%d/%d total:%d/%d capability:x%d%s",
                  pStreamInfo->streamName,
                  nChannel,
                  nMainBitrateKbps,
                  nChannelAfterStop,
                  nChannelLimit,
                  nGlobalAfterStop,
                  nGlobalLimit,
                  RTSP_CLIENT_CAPACITY_MULTIPLIER,
                  bChannelCountValid && bGlobalCountValid ? "" : " count_unavailable");
        return OK;
    }

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
    /* 两路流的运行时对象都会按通道索引访问配置，缺少通道时先拒绝初始化，避免越界。 */
    if (m_vstVideoConfig.size() < static_cast<std::size_t>(RTSP_CHN_MAX))
    {
        dlog_error("RTSP视频配置缺少通道，无法初始化 size:%zu", m_vstVideoConfig.size());
        return ERR_PARAM;
    }
    if (get_main_bitrate_kbps(m_vstVideoConfig) <= 0)
    {
        /* 主码率非法时沿用通用默认限额，但保留配置供后续业务修正。 */
        dlog_warn("RTSP主码流码率非法，采用默认连接限额");
    }

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
    m_strLastIp = stNetInfo.stIp.ipv4Ip;
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

        /*
         * 根据码率动态计算该通道的队列字节预算与单帧上限：
         * 码率上限类型使用码率上限、变码率类型使用平均码率估算，
         * 单帧上限取1秒码量，队列总字节取2倍单帧上限且不小于原有固定上限，
         * 避免高码率大I帧击穿固定上限导致丢帧。
         */
        const std::size_t unMaxFrameBytes = Video_NS::calcMaxFrameBytes(m_vstVideoConfig[i]);
        m_unVideoMaxFrameBytes[i] = unMaxFrameBytes;
        const std::size_t unQueueBytes = unMaxFrameBytes * 2U;
        m_unVideoQueueMaxBytes[i] = unQueueBytes > RTSP_VIDEO_QUEUE_MAX_BYTES ? unQueueBytes : RTSP_VIDEO_QUEUE_MAX_BYTES;

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
        m_pLiveInfo->listLive[i]->videoQueue = std::make_unique<CThreadSafeFrameQueue>(RTSP_VIDEO_QUEUE_DEPTH,
                                                                                       m_unVideoQueueMaxBytes[i]);
        m_pLiveInfo->listLive[i]->audioQueue = std::make_unique<CThreadSafeFrameQueue>(RTSP_AUDIO_QUEUE_DEPTH,
                                                                                       RTSP_AUDIO_QUEUE_MAX_BYTES);

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

        /*
         * 拓展参数保留给库层最大连接数；封装回调会再次按实际码率和能力宏校验。
         * 主码流在8 Mbps/16 Mbps档位分别收紧为2/1路，高配设备整体翻倍。
         */
        const int nMainBitrateKbps = get_main_bitrate_kbps(m_vstVideoConfig);
        const int nStreamClientLimit = i == RTSP_CHN_MAIN
                                           ? get_main_client_limit(nMainBitrateKbps)
                                           : RTSP_GLOBAL_MAX_CLIENT;
        m_stClientInfo[i].param1 = nStreamClientLimit;
        /* memory: OutPacketBuffer按主/子码流分别配置，使用应用层定义的设备相关缓存大小 */
        m_stClientInfo[i].outPacketBufferSize = i == RTSP_CHN_MAIN ? RTSP_APP_MAIN_OUT_PACKET_BUFFER_SIZE
                                                                   : RTSP_APP_SUB_OUT_PACKET_BUFFER_SIZE;
        m_stClientInfo[i].audioOutPacketBufferSize = RTSP_APP_AUDIO_OUT_PACKET_BUFFER_SIZE;
        dlog_info("RTSP实例缓存配置 chn:%d video_buf:%u audio_buf:%u",
                  i,
                  m_stClientInfo[i].outPacketBufferSize,
                  m_stClientInfo[i].audioOutPacketBufferSize);
        strcpy(m_stClientInfo[i].streamName, m_pLiveInfo->listLive[i]->streamName);

        const int nCreateRet = rtsp_server_create(m_pLiveInfo->pServerHandle, &m_stClientInfo[i]);
        if (nCreateRet != OK)
        {
            dlog_error("RTSP流会话创建失败 chn:%d ret:%d", i, nCreateRet);
            return ERR;
        }

        /*
         * rtsp_server_create 内部设置上限早于 ServerMediaSession 创建，旧库中该次
         * 设置可能无效；创建完成后再设置一次，兼容实现了 fReferenceMax 的库。
         */
        if (rtsp_setclient_maxNum(m_pLiveInfo->pServerHandle,
                                  m_stClientInfo[i].streamName,
                                  nStreamClientLimit) != OK)
        {
            dlog_warn("RTSP流上限未能下发到库 chn:%d limit:%d，封装回调仍会记录超限", i, nStreamClientLimit);
        }
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
    if (!pVideoFrame)
    {
        return ERR;
    }

    return sendVideoData(nChannel, pVideoFrame->pData, pVideoFrame->nLen, pVideoFrame->enVideoCodec, pVideoFrame->eType);
}

int CRtspServer::sendVideoData(int nChannel,
                               const uint8_t *pData,
                               int nDataLen,
                               Video_NS::VideoCodec_E enVideoCodec,
                               Video_NS::NalType_E eType)
{
    if (!m_bInitFlag.load() || nChannel < 0 || nChannel >= RTSP_CHN_MAX || !pData || nDataLen <= 0)
    {
        return ERR;
    }

    int nRet = OK;
    Live_Stream_Info_t* pStreamInfo = m_pLiveInfo->listLive[nChannel];

    if (pStreamInfo->requestIFrame == 1 && nDataLen >= 6)
    {
        dlog_warn("RTSP等待关键帧 chn:%d codec:%d nal:%d len:%d "
                  "head:%02x %02x %02x %02x %02x %02x",
                  nChannel,
                  static_cast<int>(enVideoCodec),
                  static_cast<int>(eType),
                  nDataLen,
                  pData[0],
                  pData[1],
                  pData[2],
                  pData[3],
                  pData[4],
                  pData[5]);
    }

    if (pStreamInfo->request == 1)
    {
        if (static_cast<std::size_t>(nDataLen) > m_unVideoMaxFrameBytes[nChannel])
        {
            /* memory: 先拒绝超限帧再申请副本，避免异常I帧造成一次性大内存峰值。 */
            report_queue_drop(m_stVideoQueueDiag[nChannel], "视频", nChannel);
            return ERR;
        }

        /* 创建帧数据（使用智能指针管理内存） */
        auto frameData = std::make_unique<FrameData>();
        /*
         * memory: 使用 new[] + shared_ptr 显式删除器，绕开 make_shared<unsigned char[]>
         * 在 GCC10/musl 工具链上的数组分配缺陷（实测崩溃），与共享帧构造保持一致。
         */
        frameData->data = std::shared_ptr<unsigned char[]>(new unsigned char[nDataLen], std::default_delete<unsigned char[]>());
        frameData->type = VIDEO_TYPE;
        frameData->frameSize = nDataLen;

        /* memory: 只在RTSP有界队列入队前复制一次，不保存VENC原始指针。 */
        memcpy(frameData->data.get(), pData, nDataLen);

        /* 判断是否为I帧 */
        if (enVideoCodec == Video_NS::VideoCodec_E::H264)
        {
            frameData->iFrame = (eType == Video_NS::H264_TYPE_SPS) ? 1 : 0;
        }
        else if (enVideoCodec == Video_NS::VideoCodec_E::H265)
        {
            frameData->iFrame = (eType == Video_NS::H265_TYPE_VPS) ? 1 : 0;
        }
        else if (enVideoCodec == Video_NS::VideoCodec_E::MJPEG)
        {
            frameData->iFrame = 1;
        }

        /* 入队（队列满时丢帧，智能指针自动释放） */
        if (!pStreamInfo->videoQueue->push(std::move(frameData)))
        {
            /* 队列已满，frameData 在此自动释放（丢帧策略），并输出丢帧时长诊断 */
            report_queue_drop(m_stVideoQueueDiag[nChannel], "视频", nChannel);
        }
        else
        {
            report_queue_recover(m_stVideoQueueDiag[nChannel], "视频", nChannel);
        }
    }
    else
    {
        /* 客户端未请求时，清空队列（智能指针自动释放内存） */
        pStreamInfo->videoQueue->clear();
        m_stVideoQueueDiag[nChannel] = QueueDiag_S{};
    }

    return nRet;
}

int CRtspServer::sendVideoData(int nChannel,
                               const Video_NS::SharedMediaFrame_S &stSharedFrame,
                               Video_NS::VideoCodec_E enVideoCodec,
                               Video_NS::NalType_E eType)
{
    if (!m_bInitFlag.load() || nChannel < 0 || nChannel >= RTSP_CHN_MAX || !stSharedFrame.pData || stSharedFrame.nLen <= 0)
    {
        return ERR;
    }

    const uint8_t *pData = stSharedFrame.pData.get();
    const int nDataLen = stSharedFrame.nLen;

    int nRet = OK;
    Live_Stream_Info_t *pStreamInfo = m_pLiveInfo->listLive[nChannel];

    if (pStreamInfo->requestIFrame == 1 && nDataLen >= 6)
    {
        dlog_warn("RTSP等待关键帧 chn:%d codec:%d nal:%d len:%d "
                  "head:%02x %02x %02x %02x %02x %02x",
                  nChannel,
                  static_cast<int>(enVideoCodec),
                  static_cast<int>(eType),
                  nDataLen,
                  pData[0],
                  pData[1],
                  pData[2],
                  pData[3],
                  pData[4],
                  pData[5]);
    }

    if (pStreamInfo->request == 1)
    {
        if (static_cast<std::size_t>(nDataLen) > m_unVideoMaxFrameBytes[nChannel])
        {
            /* memory: 超限帧直接拒绝，不增加共享引用计数。 */
            report_queue_drop(m_stVideoQueueDiag[nChannel], "视频", nChannel);
            return ERR;
        }

        /* 创建帧数据，data 直接持有共享帧的引用（零拷贝，引用计数+1） */
        auto frameData = std::make_unique<FrameData>();
        frameData->data = stSharedFrame.pData;
        frameData->type = VIDEO_TYPE;
        frameData->frameSize = nDataLen;

        /* 判断是否为I帧 */
        if (enVideoCodec == Video_NS::VideoCodec_E::H264)
        {
            frameData->iFrame = (eType == Video_NS::H264_TYPE_SPS) ? 1 : 0;
        }
        else if (enVideoCodec == Video_NS::VideoCodec_E::H265)
        {
            frameData->iFrame = (eType == Video_NS::H265_TYPE_VPS) ? 1 : 0;
        }
        else if (enVideoCodec == Video_NS::VideoCodec_E::MJPEG)
        {
            frameData->iFrame = 1;
        }

        /* 入队（队列满时丢帧，智能指针自动释放） */
        if (!pStreamInfo->videoQueue->push(std::move(frameData)))
        {
            report_queue_drop(m_stVideoQueueDiag[nChannel], "视频", nChannel);
        }
        else
        {
            report_queue_recover(m_stVideoQueueDiag[nChannel], "视频", nChannel);
        }
    }
    else
    {
        /* 客户端未请求时，清空队列（智能指针自动释放内存） */
        pStreamInfo->videoQueue->clear();
        m_stVideoQueueDiag[nChannel] = QueueDiag_S{};
    }

    return nRet;
}

int CRtspServer::sendAudioData(int nChannel, Audio_NS::AudioFrame_S* pAudioFrame)
{
    if (!m_bInitFlag.load() || nChannel < 0 || nChannel >= RTSP_CHN_MAX ||
        !pAudioFrame || !pAudioFrame->pData || pAudioFrame->nLen <= 0 ||
        m_stClientInfo[nChannel].Audioindex == nullptr) // 通道无音频需求时，送音频数据直接返回
    {
        return ERR;
    }

    int nRet = OK;
    Live_Stream_Info_t* pStreamInfo = m_pLiveInfo->listLive[nChannel];

    if (pStreamInfo->request == 1)
    {
        if (static_cast<std::size_t>(pAudioFrame->nLen) > RTSP_AUDIO_QUEUE_MAX_BYTES)
        {
            /* memory: 音频异常大包直接丢弃，避免占满整个音频队列。 */
            report_queue_drop(m_stAudioQueueDiag[nChannel], "音频", nChannel);
            return ERR;
        }

        /* 创建帧数据（使用智能指针管理内存） */
        auto frameData = std::make_unique<FrameData>();
        /* memory: new[] + shared_ptr 显式删除器，绕开 make_shared 数组缺陷（同视频路径） */
        frameData->data = std::shared_ptr<unsigned char[]>(new unsigned char[pAudioFrame->nLen], std::default_delete<unsigned char[]>());
        frameData->type = AUDIO_TYPE;
        frameData->frameSize = pAudioFrame->nLen;

        /* 一次性拷贝数据 */
        memcpy(frameData->data.get(), pAudioFrame->pData, pAudioFrame->nLen);

        /* 入队（队列满时丢帧，智能指针自动释放） */
        if (!pStreamInfo->audioQueue->push(std::move(frameData)))
        {
            /* 队列已满，frameData 在此自动释放（丢帧策略），并输出丢帧时长诊断 */
            report_queue_drop(m_stAudioQueueDiag[nChannel], "音频", nChannel);
        }
        else
        {
            report_queue_recover(m_stAudioQueueDiag[nChannel], "音频", nChannel);
        }
    }
    else
    {
        /* 客户端未请求时，清空队列（智能指针自动释放内存） */
        pStreamInfo->audioQueue->clear();
        m_stAudioQueueDiag[nChannel] = QueueDiag_S{};
    }

    return nRet;
}

int CRtspServer::setVideoConfig(const std::vector<Video_NS::VideoConfig_S>& vstVideoConfig)
{
    /*
     * 运行时限额依赖主/子码流配置；异常输入必须保留上一份有效配置，不能让
     * 缺失或零码率回退到低码率档而意外放宽已有的连接上限。
     */
    if (vstVideoConfig.size() < static_cast<std::size_t>(RTSP_CHN_MAX) ||
        get_main_bitrate_kbps(vstVideoConfig) <= 0)
    {
        dlog_error("RTSP视频配置无效，保留现有限额 size:%zu main_bitrate:%d",
                   vstVideoConfig.size(),
                   get_main_bitrate_kbps(vstVideoConfig));
        return ERR_PARAM;
    }

    const int nOldMainBitrateKbps = get_main_bitrate_kbps(m_vstVideoConfig);
    const int nOldMainClientLimit = get_main_client_limit(nOldMainBitrateKbps);
    m_vstVideoConfig = vstVideoConfig;

    /*
     * 码率运行时升档只收紧新连接，不主动断开已有连接。封装回调每次 START
     * 都重新读取 live555 referenceCount，因此无需重启 RTSP 服务器即可生效。
     */
    if (m_vstVideoConfig.size() > static_cast<std::size_t>(RTSP_CHN_MAIN))
    {
        const int nMainBitrateKbps = get_main_bitrate_kbps(m_vstVideoConfig);
        const int nMainClientLimit = get_main_client_limit(nMainBitrateKbps);
        m_stClientInfo[RTSP_CHN_MAIN].param1 = nMainClientLimit;
        m_stClientInfo[RTSP_CHN_SUB].param1 = RTSP_GLOBAL_MAX_CLIENT;

        int nCurrentMainCount = 0;
        int nCurrentSubCount = 0;
        bool bCurrentCountValid = false;
        if (m_bInitFlag.load() && m_pLiveInfo != nullptr && m_pLiveInfo->pServerHandle != nullptr)
        {
            bCurrentCountValid = read_rtsp_client_count(m_pLiveInfo->pServerHandle,
                                                        m_stClientInfo[RTSP_CHN_MAIN].streamName,
                                                        nCurrentMainCount) &&
                                 read_rtsp_client_count(m_pLiveInfo->pServerHandle,
                                                        m_stClientInfo[RTSP_CHN_SUB].streamName,
                                                        nCurrentSubCount);
            if (rtsp_setclient_maxNum(m_pLiveInfo->pServerHandle,
                                      m_stClientInfo[RTSP_CHN_MAIN].streamName,
                                      nMainClientLimit) != OK)
            {
                dlog_warn("主码流运行时上限下发失败 limit:%d，封装回调仍按新上限检查", nMainClientLimit);
            }
            if (rtsp_setclient_maxNum(m_pLiveInfo->pServerHandle,
                                      m_stClientInfo[RTSP_CHN_SUB].streamName,
                                      RTSP_GLOBAL_MAX_CLIENT) != OK)
            {
                dlog_warn("子码流运行时上限下发失败 limit:%d，封装回调仍按新上限检查", RTSP_GLOBAL_MAX_CLIENT);
            }
        }

        dlog_info("RTSP连接上限更新 bitrate:%d->%dkbps main:%d->%d total:%d capability:x%d "
                  "policy:keep_existing",
                  nOldMainBitrateKbps,
                  nMainBitrateKbps,
                  nOldMainClientLimit,
                  nMainClientLimit,
                  RTSP_GLOBAL_MAX_CLIENT,
                  RTSP_CLIENT_CAPACITY_MULTIPLIER);
        dlog_info("RTSP连接上限当前计数 main:%d/%d total:%d/%d%s",
                  nCurrentMainCount,
                  nMainClientLimit,
                  nCurrentMainCount + nCurrentSubCount,
                  RTSP_GLOBAL_MAX_CLIENT,
                  bCurrentCountValid ? "" : " count_unavailable");
    }
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

    /* 检查IP是否有变化，有则更新URL */
    {
        Network::Info_S stNetInfo;
        CNetworkManage::instance()->get_system_networkInfo(stNetInfo);
        if (!stNetInfo.stIp.ipv4Ip.empty() && stNetInfo.stIp.ipv4Ip != "0.0.0.0"
            && stNetInfo.stIp.ipv4Ip != m_strLastIp)
        {
            m_strLastIp = stNetInfo.stIp.ipv4Ip;
            updateNetworkConfig(stNetInfo);
        }
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
