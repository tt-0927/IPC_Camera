/**
 * @FilePath     : rtsp_server.cpp
 * @Author       : zhouzr@kfb.cn
 * @Description  : RTSP服务器薄封装（smolrtsp/ipc_rtspserver 实现）
 *
 * 本文件只做四件事：
 *   1. 读取产品配置（视频/音频/端口/QoS/网络），换算为 ipc_rtsp::ServerConfig；
 *   2. 把 Video_NS / Audio_NS 的帧结构映射为库的帧视图；
 *   3. 维护对外展示 URL 缓存，保持 getRtspUrl 的 char* 返回语义。
 *
 * 其中枚举/PT/码率分档映射收纳在 codec_map.{h,cpp}，库日志到 dlog 的转发在
 * log_sink.{h,cpp}；协议实现、连接/会话/鉴权/背压全部在 ipc_rtspserver 内，
 * 这里不做任何媒体缓冲。
 */
#include "rtsp_server.h"

#include <chrono>
#include <cstdio>
#include <utility>

#include "codec_map.h"
#include "convert_interface.h"
#include "ipc_rtsp/log.h"
#include "ipc_rtsp/server.h"
#include "log_sink.h"
#include "network_manage.h"
#include "path_define.h"
#include "qos_manage.h"
#include "system_define.h"

/** 封装实现体：只保存在头文件里前向声明的库对象。 */
struct CRtspServer::Impl
{
    /* 服务实例 */
    std::unique_ptr<ipc_rtsp::Server> server;
    /* 每码流配置（用于按码率更新上限） */
    std::vector<ipc_rtsp::StreamConfig> streams;
    /* 上一次对外展示的 IP */
    std::string advertised_ip;
};

IpcRet_E CRtspServer::buildStreamConfigs(std::vector<ipc_rtsp::StreamConfig> &vStreams, ipc_rtsp::ServerConfig &stServerConfig)
{
    vStreams.assign(static_cast<std::size_t>(RTSP_CHN_MAX), ipc_rtsp::StreamConfig{});
    for (int i = 0; i < RTSP_CHN_MAX; ++i)
    {
        const Video_NS::VideoConfig_S &stVideoConfig = m_vstVideoConfig[static_cast<std::size_t>(i)];
        ipc_rtsp::StreamConfig &stStream = vStreams[static_cast<std::size_t>(i)];

        stStream.id = (i == RTSP_CHN_MAIN) ? ipc_rtsp::StreamId::Main : ipc_rtsp::StreamId::Sub;
        stStream.path = (i == RTSP_CHN_MAIN) ? "Streaming/Channels/101" : "Streaming/Channels/102";
        stStream.codec = rtsp_smol::to_codec(stVideoConfig.enVideoCodec);
        stStream.payload_type = rtsp_smol::kVideoPayloadType;
        stStream.clock_rate = 90000;
        stStream.fps = static_cast<int>(stVideoConfig.getFrameRateAsFloat());
        if (stStream.fps <= 0)
        {
            stStream.fps = 25;
        }

        /*
         * 单帧上限按“1 秒码量、保底 512KiB”估算，与 live555 实现的 calcMaxFrameBytes
         * 保持同一口径；队列字节预算取单帧上限的 2 倍且不低于固定基线。
         */
        const std::size_t unMaxFrameBytes = Video_NS::calcMaxFrameBytes(stVideoConfig);
        stStream.max_frame_bytes = unMaxFrameBytes;
        m_unVideoMaxFrameBytes[i] = unMaxFrameBytes;
        const std::size_t unQueueBytes = unMaxFrameBytes * 2U;
        m_unVideoQueueMaxBytes[i] = unQueueBytes > RTSP_VIDEO_QUEUE_MAX_BYTES ? unQueueBytes : RTSP_VIDEO_QUEUE_MAX_BYTES;
        stServerConfig.backpressure.hub_max_video_bytes = m_unVideoQueueMaxBytes[i] > stServerConfig.backpressure.hub_max_video_bytes
                                                              ? m_unVideoQueueMaxBytes[i]
                                                              : stServerConfig.backpressure.hub_max_video_bytes;

        /* 流级上限主/子码流统一，防单条流被打满；总额由服务级上限约束。 */
        stStream.max_playing_clients = RTSP_STREAM_MAX_CLIENT;

        /*
         * GOP cache（25 号文档 25.4.2 产品初值）：主 4MiB / 子 1MiB，
         * 帧数 max(2×fps, 60) 且不超过实现硬上限 300，时长 3s。
         * 三项全 0 可整体关闭（回退 WaitStart + IDR 起播）。
         */
        stStream.gop_cache.max_bytes = (i == RTSP_CHN_MAIN) ? (4u * 1024u * 1024u) : (1u * 1024u * 1024u);
        {
            const int fps_for_gop = stStream.fps;
            int max_frames = fps_for_gop * 2 > 60 ? fps_for_gop * 2 : 60;
            if (max_frames > 300)
            {
                max_frames = 300;
            }
            stStream.gop_cache.max_frames = static_cast<std::size_t>(max_frames);
        }
        stStream.gop_cache.max_duration_ms = 3000;

        if (stStream.codec == ipc_rtsp::Codec::Unknown)
        {
            dlog_error("RTSP通道编码格式未知 chn:%d，初始化失败", i);
            return ERR_PARAM;
        }

        /* 音频：仅复合流挂音频轨，与旧实现一致。 */
        const ipc_rtsp::AudioCodec enAudioCodec = rtsp_smol::to_audio_codec(m_stAudioConfig.enFormat);
        const bool bComposite = (stVideoConfig.enVideoType == Video_NS::VideoType_E::COMPOSITE_STREAM);
        if (bComposite && m_stAudioConfig.bAudioSwitch && enAudioCodec != ipc_rtsp::AudioCodec::None)
        {
            stStream.audio_enabled = true;
            stStream.audio_codec = enAudioCodec;
            stStream.audio_clock_rate = static_cast<std::uint32_t>(m_stAudioConfig.enSampRate);
            stStream.audio_payload_type = rtsp_smol::audio_payload_type(enAudioCodec);
            stStream.audio_channels = 1;
            /* AAC 入口在业务侧已剥离 ADTS 头，这里按裸 AAC AU 处理。 */
            stStream.audio_stripped_adts = true;
        }

        dlog_info("RTSP码流配置 chn:%d path:%s codec:%s fps:%d 单帧上限:%zu 队列上限:%zu 连接上限:%d 音频:%d",
                  i,
                  stStream.path.c_str(),
                  ipc_rtsp::CodecToString(stStream.codec),
                  stStream.fps,
                  stStream.max_frame_bytes,
                  m_unVideoQueueMaxBytes[i],
                  stStream.max_playing_clients,
                  stStream.audio_enabled ? 1 : 0);
    }
    return OK;
}

CRtspServer::CRtspServer()
    : m_strVideoConfigPath(VIDEO_CONFIG_FILE), m_strAudioConfigPath(AUDIO_CONFIG_FILE), m_strPortPath(PORT_CONFIG_FILE), m_pImpl(new Impl())
{
    m_requestIdrCallback = nullptr;

    /* 读取视频/音频配置：与旧实现一致，文件缺失时写回默认配置。 */
    if (Convert::read_file(m_strVideoConfigPath, m_vstVideoConfig))
    {
        Convert::write_file(m_strVideoConfigPath, m_vstVideoConfig);
    }
    if (Convert::read_file(m_strAudioConfigPath, m_stAudioConfig))
    {
        Convert::write_file(m_strAudioConfigPath, m_stAudioConfig);
    }

    /* 端口配置 */
    Network::PortConfig_S stPortConfig;
    if (Convert::read_file(m_strPortPath, stPortConfig))
    {
        Convert::write_file(m_strPortPath, stPortConfig);
    }
    m_nRtspPort = stPortConfig.nRtspPort;

    /* QoS：默认使用最低 DSCP，配置合法时采用配置值。 */
    m_nMediaDscp = QOS_DSCP_MIN;
    Network::QosConfigInfo_S stQosConfigInfo;
    CQosManage::instance()->get_qos_config(stQosConfigInfo);
    if (stQosConfigInfo.nMediaDscp >= QOS_DSCP_MIN && stQosConfigInfo.nMediaDscp <= QOS_DSCP_MAX)
    {
        m_nMediaDscp = stQosConfigInfo.nMediaDscp;
    }

    /* 默认开启鉴权（与旧实现一致）。 */
    m_bAuthentication = true;
    updateRtspDigestAlgorithm();
}

CRtspServer::~CRtspServer()
{
    deinit();
}

int CRtspServer::stopIngest()
{
    if (m_pImpl == nullptr || m_pImpl->server == nullptr)
    {
        return ERR_UNINIT;
    }
    const ipc_rtsp::Result result = m_pImpl->server->StopIngest();
    return result.ok() ? OK : ERR;
}

IpcRet_E CRtspServer::init()
{
    if (m_bInitFlag.load())
    {
        dlog_warn("RTSP已初始化，忽略重复init");
        return OK;
    }

    /* 两路码流都会按通道索引访问配置，缺通道时先拒绝初始化，避免越界。 */
    if (m_vstVideoConfig.size() < static_cast<std::size_t>(RTSP_CHN_MAX))
    {
        dlog_error("RTSP视频配置缺少通道，无法初始化 size:%zu", m_vstVideoConfig.size());
        return ERR_PARAM;
    }

    /* 网络信息：用于对外展示地址与 SDP 的 c= 行。 */
    CNetworkManage::instance()->get_system_networkInfo(m_stNetInfo);
    m_strAdvertisedIp = m_stNetInfo.stIp.ipv4Ip;

    /* ---- 服务配置 ---- */
    ipc_rtsp::ServerConfig stServerConfig;
    stServerConfig.port = static_cast<std::uint16_t>(m_nRtspPort);
    stServerConfig.bind_address = "0.0.0.0";
    stServerConfig.advertised_ip = m_strAdvertisedIp;
    stServerConfig.max_connections = RTSP_GLOBAL_MAX_CLIENT * 2; /* 允许未播放/未认证连接 */
    stServerConfig.max_playing_clients = RTSP_GLOBAL_MAX_CLIENT;
    stServerConfig.dscp = m_nMediaDscp;
    stServerConfig.server_name = "IPC RTSP Server";

    if (m_bAuthentication)
    {
        stServerConfig.auth.mode = ipc_rtsp::AuthMode::Digest;
        stServerConfig.auth.user = m_strUser;
        stServerConfig.auth.password = m_strPwd;
    }
    else
    {
        stServerConfig.auth.mode = ipc_rtsp::AuthMode::None;
    }

    /* 背压预算：时长制判定 + 内存护栏（见 21_资源预算与背压设计.md）。 */
    stServerConfig.backpressure.hub_max_video_frames = RTSP_VIDEO_QUEUE_DEPTH;
    stServerConfig.backpressure.hub_max_video_bytes = RTSP_VIDEO_QUEUE_MAX_BYTES;
    stServerConfig.backpressure.hub_max_audio_frames = RTSP_AUDIO_QUEUE_DEPTH;
    stServerConfig.backpressure.hub_max_audio_bytes = RTSP_AUDIO_QUEUE_MAX_BYTES;
    stServerConfig.backpressure.client_soft_backlog_ms = RTSP_APP_CLIENT_SOFT_BACKLOG_MS;
    stServerConfig.backpressure.client_hard_backlog_ms = RTSP_APP_CLIENT_HARD_BACKLOG_MS;
    stServerConfig.backpressure.client_output_guard_bytes = RTSP_APP_CLIENT_OUTPUT_GUARD_BYTES;

    /* ---- 码流配置：枚举映射、单帧/队列预算与连接上限（同时抬高视频队列字节上限） ---- */
    std::vector<ipc_rtsp::StreamConfig> vStreams(static_cast<std::size_t>(RTSP_CHN_MAX));
    const IpcRet_E enBuildRet = buildStreamConfigs(vStreams, stServerConfig);
    if (enBuildRet != OK)
    {
        return enBuildRet;
    }

    /* ---- 创建并启动服务 ---- */
    ipc_rtsp::SetLogSink(&rtsp_smol::log_sink, nullptr);
    ipc_rtsp::SetLogLevel(ipc_rtsp::LogLevel::Info);

    ipc_rtsp::Result result = ipc_rtsp::Server::Create(stServerConfig, vStreams, &m_pImpl->server);
    if (!result.ok())
    {
        dlog_error("RTSP服务创建失败 status:%s", ipc_rtsp::StatusToString(result.code));
        m_pImpl->server.reset();
        return ERR;
    }
    m_pImpl->streams = vStreams;
    m_pImpl->advertised_ip = m_strAdvertisedIp;

    m_pImpl->server->SetIdrRequestCallback(
        [this](ipc_rtsp::StreamId id)
        {
            /* 库在 I/O 线程内回调：只做转发，不做阻塞动作。 */
            RequestIdrCallback callback = m_requestIdrCallback;
            if (callback)
            {
                callback(static_cast<int>(id), m_pCallbackUserData);
            }
        });

    result = m_pImpl->server->Start();
    if (!result.ok())
    {
        dlog_error("RTSP服务启动失败 status:%s errno:%d port:%d", ipc_rtsp::StatusToString(result.code), result.sys_errno, m_nRtspPort);
        m_pImpl->server.reset();
        return ERR;
    }

    /* URL 缓存：普通地址与带凭据地址各一份，保持 char* 语义。 */
    for (int i = 0; i < RTSP_CHN_MAX; ++i)
    {
        char achUrl[128] = { 0 };
        snprintf(achUrl, sizeof achUrl, RTSP_URL_DEFAULT, m_strAdvertisedIp.c_str(), m_nRtspPort, i + 101);
        m_rtspUrlMap[i] = achUrl;
    }

    m_bInitFlag.store(true);
    dlog_info("rtsp服务器初始化成功 port:%d 主码流URL:%s", m_nRtspPort, m_rtspUrlMap[RTSP_CHN_MAIN].c_str());
    return OK;
}

IpcRet_E CRtspServer::deinit()
{
    m_bInitFlag.store(false);
    if (m_pImpl != nullptr && m_pImpl->server != nullptr)
    {
        const ipc_rtsp::Result result = m_pImpl->server->Shutdown(std::chrono::milliseconds(2000));
        if (!result.ok())
        {
            dlog_warn("RTSP服务关停返回 status:%s", ipc_rtsp::StatusToString(result.code));
        }
        m_pImpl->server.reset();
    }
    dlog_info("rtsp服务器去初始化成功");
    return OK;
}

bool CRtspServer::isInit()
{
    return m_bInitFlag.load();
}

IpcRet_E CRtspServer::reboot()
{
    std::lock_guard<std::mutex> lock(m_mutexCtrl);

    if (m_bInitFlag.load())
    {
        const IpcRet_E nRet = deinit();
        if (nRet != OK)
        {
            dlog_error("反初始化rtsp服务器失败");
            return ERR;
        }
    }
    return init();
}

int CRtspServer::sendVideoData(int nChannel, Video_NS::VideoFrame_S *pVideoFrame)
{
    if (pVideoFrame == nullptr)
    {
        return ERR_PTR_NULL;
    }
    return sendVideoData(nChannel, pVideoFrame->pData, pVideoFrame->nLen, pVideoFrame->enVideoCodec, pVideoFrame->eType);
}

int CRtspServer::sendVideoData(int nChannel,
                               const uint8_t *pData,
                               int nDataLen,
                               Video_NS::VideoCodec_E enVideoCodec,
                               Video_NS::NalType_E eType)
{
    if (!m_bInitFlag.load() || m_pImpl == nullptr || m_pImpl->server == nullptr)
    {
        return ERR_UNINIT;
    }
    if (nChannel < 0 || nChannel >= RTSP_CHN_MAX || pData == nullptr || nDataLen <= 0)
    {
        return ERR_PARAM;
    }

    ipc_rtsp::VideoFrameView stFrame;
    stFrame.data = pData;
    stFrame.size = static_cast<std::size_t>(nDataLen);
    stFrame.codec = rtsp_smol::to_codec(enVideoCodec);
    stFrame.first_nal = rtsp_smol::to_nal_type(enVideoCodec, eType);
    /* 产品当前不向 RTSP 传 PTS：库内用单调时钟推进时间戳（与旧实现口径一致）。 */
    stFrame.has_pts = false;
    stFrame.key = (stFrame.first_nal == ipc_rtsp::NalUnitType::IdrSlice) || (stFrame.first_nal == ipc_rtsp::NalUnitType::CraSlice);

    const ipc_rtsp::StreamId id = (nChannel == RTSP_CHN_MAIN) ? ipc_rtsp::StreamId::Main : ipc_rtsp::StreamId::Sub;
    const ipc_rtsp::Result result = m_pImpl->server->PushVideo(id, stFrame);

    /* 队列满（NoCapacity）按旧语义不算失败：丢弃由库内计数与日志体现。 */
    if (result.ok() || result.code == ipc_rtsp::Status::NoCapacity)
    {
        return OK;
    }
    return ERR;
}

int CRtspServer::sendVideoData(int nChannel,
                               const Video_NS::SharedMediaFrame_S &stSharedFrame,
                               Video_NS::VideoCodec_E enVideoCodec,
                               Video_NS::NalType_E eType)
{
    if (!m_bInitFlag.load() || m_pImpl == nullptr || m_pImpl->server == nullptr)
    {
        return ERR_UNINIT;
    }
    if (nChannel < 0 || nChannel >= RTSP_CHN_MAX || !stSharedFrame.pData || stSharedFrame.nLen <= 0)
    {
        return ERR_PARAM;
    }

    ipc_rtsp::SharedVideoFrame stFrame;
    /* shared_ptr<uint8_t> -> shared_ptr<const uint8_t>：别名构造共享同一控制块
     * （引用计数与数组释放策略不变），持有指针退化为首元素指针。 */
    stFrame.data = std::shared_ptr<const std::uint8_t>(stSharedFrame.pData, stSharedFrame.pData.get());
    stFrame.size = static_cast<std::size_t>(stSharedFrame.nLen);

    ipc_rtsp::VideoFrameMeta stMeta;
    stMeta.codec = rtsp_smol::to_codec(enVideoCodec);
    stMeta.first_nal = rtsp_smol::to_nal_type(enVideoCodec, eType);
    stMeta.has_pts = false;
    stMeta.key = (stMeta.first_nal == ipc_rtsp::NalUnitType::IdrSlice) || (stMeta.first_nal == ipc_rtsp::NalUnitType::CraSlice);

    const ipc_rtsp::StreamId id = (nChannel == RTSP_CHN_MAIN) ? ipc_rtsp::StreamId::Main : ipc_rtsp::StreamId::Sub;
    const ipc_rtsp::Result result = m_pImpl->server->PushVideo(id, stFrame, stMeta);
    if (result.ok() || result.code == ipc_rtsp::Status::NoCapacity)
    {
        return OK;
    }
    return ERR;
}

int CRtspServer::sendAudioData(int nChannel, Audio_NS::AudioFrame_S *pAudioFrame)
{
    if (!m_bInitFlag.load() || m_pImpl == nullptr || m_pImpl->server == nullptr)
    {
        return ERR_UNINIT;
    }
    if (nChannel < 0 || nChannel >= RTSP_CHN_MAX || pAudioFrame == nullptr || pAudioFrame->nLen <= 0)
    {
        return ERR_PARAM;
    }

    ipc_rtsp::AudioFrameView stFrame;
    stFrame.data = pAudioFrame->pData;
    stFrame.size = static_cast<std::size_t>(pAudioFrame->nLen);
    stFrame.codec = rtsp_smol::to_audio_codec(pAudioFrame->enFormat);
    stFrame.has_pts = false;

    const ipc_rtsp::StreamId id = (nChannel == RTSP_CHN_MAIN) ? ipc_rtsp::StreamId::Main : ipc_rtsp::StreamId::Sub;
    const ipc_rtsp::Result result = m_pImpl->server->PushAudio(id, stFrame);
    if (result.ok() || result.code == ipc_rtsp::Status::NoCapacity || result.code == ipc_rtsp::Status::InvalidArgument)
    {
        return OK;
    }
    return ERR;
}

int CRtspServer::setVideoConfig(const std::vector<Video_NS::VideoConfig_S> &vstVideoConfig)
{
    std::lock_guard<std::mutex> lock(m_mutexCtrl);
    if (vstVideoConfig.size() < static_cast<std::size_t>(RTSP_CHN_MAX))
    {
        dlog_error("RTSP视频配置缺少通道 size:%zu", vstVideoConfig.size());
        return ERR_PARAM;
    }
    m_vstVideoConfig = vstVideoConfig;

    if (!m_bInitFlag.load() || m_pImpl == nullptr || m_pImpl->server == nullptr)
    {
        /* 未运行时只保存配置，等 init 生效。 */
        return OK;
    }

    /* 运行中：只更新流级上限与单帧/队列预算，不重建会话（不踢已有客户端）。 */
    std::vector<ipc_rtsp::StreamConfig> vStreams = m_pImpl->streams;
    for (int i = 0; i < RTSP_CHN_MAX; ++i)
    {
        const std::size_t unMaxFrameBytes = Video_NS::calcMaxFrameBytes(m_vstVideoConfig[static_cast<std::size_t>(i)]);
        vStreams[static_cast<std::size_t>(i)].max_frame_bytes = unMaxFrameBytes;
        vStreams[static_cast<std::size_t>(i)].max_playing_clients = RTSP_STREAM_MAX_CLIENT;
        vStreams[static_cast<std::size_t>(i)].fps = static_cast<int>(m_vstVideoConfig[static_cast<std::size_t>(i)].getFrameRateAsFloat());
        if (vStreams[static_cast<std::size_t>(i)].fps <= 0)
        {
            vStreams[static_cast<std::size_t>(i)].fps = 25;
        }
    }

    const ipc_rtsp::Result result = m_pImpl->server->UpdateStreamConfig(vStreams);
    if (!result.ok())
    {
        dlog_error("RTSP更新码流配置失败 status:%s", ipc_rtsp::StatusToString(result.code));
        return ERR;
    }
    m_pImpl->streams = vStreams;
    dlog_info("RTSP更新视频配置完成 流级连接上限:%d 服务级总额:%d", RTSP_STREAM_MAX_CLIENT, RTSP_GLOBAL_MAX_CLIENT);
    return OK;
}

int CRtspServer::setAudioConfig(const Audio_NS::AudioConfig_S &stAudioConfig)
{
    std::lock_guard<std::mutex> lock(m_mutexCtrl);
    m_stAudioConfig = stAudioConfig;
    dlog_info("RTSP保存音频配置完成（音轨变更需调用方reboot生效）");
    return OK;
}

int CRtspServer::updateNetworkConfig(const Network::Info_S &stInfo)
{
    m_stNetInfo = stInfo;
    m_strAdvertisedIp = stInfo.stIp.ipv4Ip;

    if (m_pImpl != nullptr && m_pImpl->server != nullptr)
    {
        (void) m_pImpl->server->UpdateAdvertisedIp(m_strAdvertisedIp);
    }

    /* 更新展示 URL（日志只打印脱敏地址，不含账号密码）。 */
    for (int i = 0; i < RTSP_CHN_MAX; ++i)
    {
        char achUrl[128] = { 0 };
        snprintf(achUrl, sizeof achUrl, RTSP_URL_DEFAULT, m_strAdvertisedIp.c_str(), m_nRtspPort, i + 101);
        m_rtspUrlMap[i] = achUrl;
    }
    dlog_info("RTSP更新对外地址完成 ip:%s", m_strAdvertisedIp.c_str());
    return OK;
}

int CRtspServer::setPort(const int &nPort)
{
    std::lock_guard<std::mutex> lock(m_mutexCtrl);
    if (nPort <= 0 || nPort > 65535)
    {
        dlog_error("RTSP端口非法 port:%d", nPort);
        return ERR_PARAM;
    }
    m_nRtspPort = nPort;
    dlog_info("RTSP端口已更新为 %d（reboot 后生效）", nPort);
    return OK;
}

int CRtspServer::setQosDscp(const int &nDscp)
{
    std::lock_guard<std::mutex> lock(m_mutexCtrl);
    if (nDscp < QOS_DSCP_MIN || nDscp > QOS_DSCP_MAX)
    {
        dlog_error("RTSP DSCP非法 dscp:%d", nDscp);
        return ERR_PARAM;
    }
    m_nMediaDscp = nDscp;
    dlog_info("RTSP媒体DSCP已更新为 %d（reboot 后生效）", nDscp);
    return OK;
}

char *CRtspServer::getRtspUrl(int nChn, bool bAuth)
{
    if (nChn < 0 || nChn >= RTSP_CHN_MAX)
    {
        return nullptr;
    }

    const auto it = m_rtspUrlMap.find(nChn);
    if (it == m_rtspUrlMap.end())
    {
        return nullptr;
    }

    if (bAuth && m_bAuthentication)
    {
        char achUrl[160] = { 0 };
        snprintf(achUrl,
                 sizeof achUrl,
                 RTSP_URL_AUTHENTICATION_DEFAULT,
                 m_strUser.c_str(),
                 m_strPwd.c_str(),
                 m_strAdvertisedIp.c_str(),
                 m_nRtspPort,
                 nChn + 101);
        m_rtspUrlAuthMap[nChn] = achUrl;
        return const_cast<char *>(m_rtspUrlAuthMap[nChn].c_str());
    }

    return const_cast<char *>(it->second.c_str());
}

int CRtspServer::setRequestIdrCallback(const RequestIdrCallback &callback, void *pUserData)
{
    std::lock_guard<std::mutex> lock(m_mutexCtrl);
    m_requestIdrCallback = callback;
    m_pCallbackUserData = pUserData;
    return OK;
}

int CRtspServer::triggerRequestIdr(int nChannel)
{
    if (nChannel < 0 || nChannel >= RTSP_CHN_MAX)
    {
        return ERR_PARAM;
    }
    if (m_pImpl == nullptr || m_pImpl->server == nullptr)
    {
        return ERR_UNINIT;
    }

    /* MJPEG 每帧独立可解码，海思 VENC 对 MJPEG 通道请求 IDR 返回 NOT_SUPPORT；
     * 旧 live555 封装（triggerRequestIdr）入口同样直接返回成功。 */
    if (m_pImpl->streams[static_cast<std::size_t>(nChannel)].codec == ipc_rtsp::Codec::MJPEG)
    {
        return OK;
    }

    /* 库内已做全局限频；这里只转发回调，保持旧接口语义。 */
    RequestIdrCallback callback = m_requestIdrCallback;
    if (callback)
    {
        callback(nChannel, m_pCallbackUserData);
        return OK;
    }
    return ERR_UNINIT;
}

int CRtspServer::reset_lastIdrRequestTime()
{
    /*
     * 新实现用单调时钟做限频，系统时间回拨不再影响 IDR 请求节流，
     * 因此这里保留接口兼容语义，不再需要重置动作。
     */
    dlog_info("RTSP IDR节流基于单调时钟，无需重置");
    return OK;
}

int CRtspServer::update_userInfo(std::string strUser, std::string strPwd, bool bReboot)
{
    if (strUser.empty() || strPwd.empty())
    {
        return ERR_PARAM_NULL;
    }

    std::lock_guard<std::mutex> lock(m_mutexCtrl);
    const bool bServerRunning = m_pImpl != nullptr && m_pImpl->server != nullptr;
    if (bServerRunning)
    {
        const ipc_rtsp::Result result = m_pImpl->server->UpdateCredential(strUser, strPwd);
        if (!result.ok())
        {
            dlog_error("RTSP更新用户失败 status:%s", ipc_rtsp::StatusToString(result.code));
            return ERR;
        }
    }

    m_strUser = std::move(strUser);
    m_strPwd = std::move(strPwd);

    /*
     * bReboot 是 live555 后端遗留的兼容参数。smolrtsp 的凭据由鉴权控制面热更新，
     * 服务运行时无需关闭监听器；未运行时缓存值会在下次 init() 写入 ServerConfig。
     */
    dlog_info("RTSP用户凭据已更新：%s（服务不重启）；兼容参数bReboot=%d已忽略",
              bServerRunning ? "已热生效" : "服务未启动，缓存待下次启动生效",
              bReboot ? 1 : 0);
    return OK;
}

int CRtspServer::updateRtspDigestAlgorithm()
{
    /* 与旧实现一致：从安全配置读取摘要算法；读不到时保持 MD5。 */
    System::SecurityCert_S stSecurityCert;
    (void) Convert::read_file(SECURITY_CERT_CONFIG_FILE, stSecurityCert);

    const int nAlgorithm = static_cast<int>(stSecurityCert.enRtspDigestAlgorithm);
    if (nAlgorithm != m_nRtspDigestAlgorithm)
    {
        m_nRtspDigestAlgorithm = nAlgorithm;
        dlog_debug("rtsp 摘要算法设置 %d", m_nRtspDigestAlgorithm);
    }

    if (m_pImpl != nullptr && m_pImpl->server != nullptr)
    {
        /* v1 库只实现 MD5；传 SHA256 时库内会降级并告警。 */
        const ipc_rtsp::DigestAlgorithm enAlgorithm = (m_nRtspDigestAlgorithm == 1) ? ipc_rtsp::DigestAlgorithm::Sha256
                                                                                    : ipc_rtsp::DigestAlgorithm::Md5;
        (void) m_pImpl->server->UpdateDigestAlgorithm(enAlgorithm);
    }
    return OK;
}

void CRtspServer::dumpRuntimeStats()
{
    if (m_pImpl == nullptr || m_pImpl->server == nullptr)
    {
        dlog_warn("RTSP未初始化，无运行指标");
        return;
    }

    const ipc_rtsp::MetricsSnapshot stSnapshot = m_pImpl->server->Snapshot();
    dlog_info("RTSP指标 连接:%llu/%llu 拒绝:%llu 会话:%llu 播放:%llu TCP:%llu UDP:%llu RTP:%llu RTCP:%llu 发送字节:%llu 认证失败:%llu "
              "解析失败:%llu 拥塞断开:%llu",
              static_cast<unsigned long long>(stSnapshot.connections_active),
              static_cast<unsigned long long>(stSnapshot.connections_accepted),
              static_cast<unsigned long long>(stSnapshot.connections_rejected),
              static_cast<unsigned long long>(stSnapshot.sessions_active),
              static_cast<unsigned long long>(stSnapshot.playing_clients),
              static_cast<unsigned long long>(stSnapshot.tcp_clients),
              static_cast<unsigned long long>(stSnapshot.udp_clients),
              static_cast<unsigned long long>(stSnapshot.rtp_packets),
              static_cast<unsigned long long>(stSnapshot.rtcp_packets),
              static_cast<unsigned long long>(stSnapshot.tx_bytes),
              static_cast<unsigned long long>(stSnapshot.auth_failures),
              static_cast<unsigned long long>(stSnapshot.parse_failures),
              static_cast<unsigned long long>(stSnapshot.congestion_disconnects));

    for (int i = 0; i < RTSP_CHN_MAX; ++i)
    {
        const ipc_rtsp::StreamStats &stStream = stSnapshot.streams[i];
        dlog_info("RTSP码流指标 chn:%d 收帧:%llu 收字节:%llu 丢帧:%llu 丢关键帧:%llu 订阅者:%llu 队列峰值帧:%llu 队列峰值字节:%llu "
                  "参数集更新:%llu",
                  i,
                  static_cast<unsigned long long>(stStream.pushed_frames),
                  static_cast<unsigned long long>(stStream.pushed_bytes),
                  static_cast<unsigned long long>(stStream.dropped_frames),
                  static_cast<unsigned long long>(stStream.dropped_key_frames),
                  static_cast<unsigned long long>(stStream.subscribers),
                  static_cast<unsigned long long>(stStream.queue_high_water_frames),
                  static_cast<unsigned long long>(stStream.queue_high_water_bytes),
                  static_cast<unsigned long long>(stStream.parameter_set_updates));
        dlog_info("RTSP码流GOP指标 chn:%d 开:%llu 帧:%llu 字节:%llu 代:%llu 建:%llu 失效:%llu 回放:%llu 中止:%llu IDR请求:%llu",
                  i,
                  static_cast<unsigned long long>(stStream.gop_cache_enabled),
                  static_cast<unsigned long long>(stStream.gop_cache_frames),
                  static_cast<unsigned long long>(stStream.gop_cache_bytes),
                  static_cast<unsigned long long>(stStream.gop_epoch),
                  static_cast<unsigned long long>(stStream.gop_started),
                  static_cast<unsigned long long>(stStream.gop_invalidated),
                  static_cast<unsigned long long>(stStream.gop_replays),
                  static_cast<unsigned long long>(stStream.gop_replay_aborts),
                  static_cast<unsigned long long>(stStream.idr_requests));
    }
}
