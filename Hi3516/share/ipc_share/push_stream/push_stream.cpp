/**
 * @FilePath     : push_stream.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-03-27 17:42:05
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-20 16:00:01
 * @Description  : 推流模块
 */

#include "push_stream.h"

#include <set>

#include "av_configure.h"
#include "path_define.h"
#include "platform_manager.h"
#include "system_manage.h"

namespace
{
#if CAP_RTMP_PUSH
/**
 * @brief 判断视频配置是否可用于RTMP推流
 * @param stVideoConfig 视频配置
 * @return true：支持，false：不支持
 */
bool is_rtmp_video_config_supported(const Video_NS::VideoConfig_S &stVideoConfig)
{
    /* 只允许 nId 为 0/1 且编码为 H264/H265，直接排除 2 的 JPEG 抓图通道 */
    if (stVideoConfig.nId < 0 || stVideoConfig.nId >= 2)
    {
        return false;
    }

    return stVideoConfig.enVideoCodec == Video_NS::VideoCodec_E::H264 ||
           stVideoConfig.enVideoCodec == Video_NS::VideoCodec_E::H265;
}

/**
 * @brief 构造RTMP推流地址
 * @param stPlatformInfo 平台配置
 * @param nChannel 码流通道号
 * @param strDeviceSN 设备序列号
 * @param strToken 访问 token
 * @return RTMP推流地址（格式：rtmp://<IP>:4920/live/<device_sn>-<stream_id>?token=<token>）
 * @note   : stream_id 根据通道号确定：0->main, 1->sub1, 2->sub2, 3->sub3
 */
std::string build_rtmp_url(const Network::Platform_Info_t &stPlatformInfo,
                           int nChannel,
                           const std::string &strDeviceSN,
                           const std::string &strToken)
{
    /* RTMP 推流端口，优先使用平台配置中的 rtmp_port，无效时使用默认值 */
    int nPort = stPlatformInfo.rtmp_port > 0 && stPlatformInfo.rtmp_port <= 65535 ? stPlatformInfo.rtmp_port : 4920;

    /* 根据通道号确定 stream_id */
    std::string strStreamId;
    switch (nChannel)
    {
    case 0:
        strStreamId = "main";
        break;
    case 1:
        strStreamId = "sub1";
        break;
    case 2:
        strStreamId = "sub2";
        break;
    case 3:
        strStreamId = "sub3";
        break;
    default:
        strStreamId = "main";
        break;
    }

    /* 构建 URL: rtmp://<IP>:4920/live/<device_sn>-<stream_id>?token=<token> */
    // std::string strUrl = "rtmp://" + stPlatformInfo.server_ip + ":" + std::to_string(nPort) + "/live/" + strDeviceSN + "-" +
    //                      strStreamId + "?token=" + strToken;
    std::string strUrl = "rtmp://" + stPlatformInfo.server_ip + ":" + std::to_string(nPort) + "/live/" + strDeviceSN + "-" +
                         strStreamId;

    dlog_info("RTMP推流地址: server_ip=%s, port=%d, device_sn=%s, stream_id=%s, token=%s",
              stPlatformInfo.server_ip.c_str(),
              nPort,
              strDeviceSN.c_str(),
              strStreamId.c_str(),
              strToken.c_str());

    return strUrl;
    /* 测试使用 */
    // return "rtmp://183.129.224.253:4920/live/21a46a5500032541-main?token=test_token";
}

/**
 * @brief 获取当前平台推流配置
 * @param stPlatformInfo 平台配置
 * @return true：平台推流已开启且地址有效，false：未开启或地址无效
 */
bool get_rtmp_platform_info(Network::Platform_Info_t &stPlatformInfo)
{
    // Network::LoginInfo stLoginInfo;
    // CPlatformManager::instance()->getlogininfo(stLoginInfo);

    // stPlatformInfo.server_ip = stLoginInfo.host;
    // stPlatformInfo.server_port = stLoginInfo.port;
    // stPlatformInfo.enable = stLoginInfo.enable;
    // stPlatformInfo.Custom = stLoginInfo.Custom;

    /* 读取完整平台配置，确保启动及按通道重推时使用网页保存的 RTMP 端口。 */
    CPlatformManager::instance()->getplatforminfo(stPlatformInfo);

    if (!stPlatformInfo.enable)
    {
        dlog_info("RTMP推流未开启，跳过初始化");
        return false;
    }
    if (stPlatformInfo.server_ip.empty())
    {
        dlog_error("RTMP推流地址为空，跳过初始化");
        return false;
    }

    return true;
}
#endif
}

CPushStream::CPushStream() : m_strHttpsConfigFile(HTTPS_CONFIG_FILE)
{
}

IpcRet_E CPushStream::init()
{
    Network::HttpsConfigInfo_S stInfo;
    if (Convert::read_file(m_strHttpsConfigFile, stInfo))
    {
        Convert::write_file(m_strHttpsConfigFile, stInfo);
    }
    if (stInfo.bEnRtsp && !CRtspServer::instance()->isInit())
    {
        CRtspServer::instance()->init();
    }

    /* 初始化RTMP推流（按平台配置开关控制） */
#if CAP_RTMP_PUSH
    {
        Network::Platform_Info_t stPlatformInfo;
        if (get_rtmp_platform_info(stPlatformInfo))
        {
            if (!CRtmpPusher::instance()->is_init())
            {
                CRtmpPusher::instance()->init();
            }

            /* 获取设备序列号 */
            System::DeviceInfo_S stDeviceInfo;
            SystemManage::instance()->get_device_info(stDeviceInfo);
            std::string strDeviceSN = stDeviceInfo.serialNumber;

            /* 获取访问 token */
            std::string strToken = CPlatformManager::instance()->get_access_token();

            Audio_NS::AudioConfig_S stAudioConfig;
            CAVConfigure::instance()->get_configure(stAudioConfig);

            std::set<Video_NS::VideoConfig_S> setVideoConfig;
            CAVConfigure::instance()->get_configure(setVideoConfig);
            for (const auto &stVideoConfig : setVideoConfig)
            {
                if (!is_rtmp_video_config_supported(stVideoConfig))
                {
                    continue;
                }
                std::string strRtmpUrl = build_rtmp_url(stPlatformInfo, stVideoConfig.nId, strDeviceSN, strToken);
                dlog_info("RTMP推流地址生成成功，通道=%d, URL=%s", stVideoConfig.nId, strRtmpUrl.c_str());
                CRtmpPusher::instance()->start_push(stVideoConfig.nId, strRtmpUrl, stVideoConfig, stAudioConfig);
            }
        }
    }
#endif
    m_bInitFlag = true;

    return OK;
}

IpcRet_E CPushStream::deinit()
{
    if (CRtspServer::instance()->isInit())
    {
        CRtspServer::instance()->deinit();
    }
#if CAP_RTMP_PUSH
    if (CRtmpPusher::instance()->is_init())
    {
        CRtmpPusher::instance()->deinit();
    }
#endif
    m_bInitFlag = false;

    return OK;
}

int CPushStream::sendVideoData(Video_NS::VideoFrame_S *pVideoFrame, bool bIsMain, bool bIsRtsp)
{
    if (!pVideoFrame)
    {
        dlog_error("指针为空");
        return ERR_PTR_NULL;
    }

    return sendVideoData(pVideoFrame->pData,
                         pVideoFrame->nLen,
                         pVideoFrame->enVideoCodec,
                         pVideoFrame->eType,
                         bIsMain,
                         bIsRtsp);
}

int CPushStream::sendVideoData(const uint8_t *pData,
                               int nDataLen,
                               Video_NS::VideoCodec_E enVideoCodec,
                               Video_NS::NalType_E eType,
                               bool bIsMain,
                               bool bIsRtsp)
{
    if (!pData || nDataLen <= 0)
    {
        return ERR_PARAM;
    }

    int nRet = OK;
    if (m_bInitFlag == true)
    {
        if (bIsRtsp == true)
        {
            /* 发送到RTSP */
            if (bIsMain == true)
            {
                nRet = CRtspServer::instance()->sendVideoData(
                    RTSP_CHN_MAIN, pData, nDataLen, enVideoCodec, eType);
            }
            else
            {
                nRet = CRtspServer::instance()->sendVideoData(
                    RTSP_CHN_SUB, pData, nDataLen, enVideoCodec, eType);
            }

            /* 同时发送到RTMP（如果RTMP已初始化） */
#if CAP_RTMP_PUSH
            if (CRtmpPusher::instance()->is_init())
            {
                int nChannel = bIsMain ? 0 : 1;
                CRtmpPusher::instance()->send_video_data(nChannel, pData, nDataLen, eType);
            }
#endif
        }
    }

    return nRet;
}

int CPushStream::sendVideoData(const Video_NS::SharedMediaFrame_S &stSharedFrame,
                               Video_NS::VideoCodec_E enVideoCodec,
                               Video_NS::NalType_E eType,
                               bool bIsMain,
                               bool bIsRtsp)
{
    if (!stSharedFrame.pData || stSharedFrame.nLen <= 0)
    {
        return ERR_PARAM;
    }

    int nRet = OK;
    if (m_bInitFlag == true)
    {
        if (bIsRtsp == true)
        {
            /* 发送到RTSP（零拷贝，共享引用） */
            if (bIsMain == true)
            {
                nRet = CRtspServer::instance()->sendVideoData(RTSP_CHN_MAIN, stSharedFrame, enVideoCodec, eType);
            }
            else
            {
                nRet = CRtspServer::instance()->sendVideoData(RTSP_CHN_SUB, stSharedFrame, enVideoCodec, eType);
            }

            /* 同时发送到RTMP（如果RTMP已初始化），同样零拷贝共享引用 */
#if CAP_RTMP_PUSH
            if (CRtmpPusher::instance()->is_init())
            {
                int nChannel = bIsMain ? 0 : 1;
                CRtmpPusher::instance()->send_video_data(nChannel, stSharedFrame, eType);
            }
#endif
        }
    }

    return nRet;
}

#if CAP_RTMP_PUSH
int CPushStream::sendVideoData(Video_NS::VideoFrame_S *pVideoFrame, bool bIsMain, PushStreamProtocol_E enProtocol)
{
    if (!pVideoFrame)
    {
        dlog_error("指针为空");
        return ERR_PTR_NULL;
    }

    int nRet = OK;
    if (m_bInitFlag == true)
    {
        switch (enProtocol)
        {
        case PROTOCOL_RTSP:
            if (bIsMain == true)
            {
                nRet = CRtspServer::instance()->sendVideoData(RTSP_CHN_MAIN, pVideoFrame);
            }
            else
            {
                nRet = CRtspServer::instance()->sendVideoData(RTSP_CHN_SUB, pVideoFrame);
            }
            break;
        case PROTOCOL_RTMP:
        {
            int nChannel = bIsMain ? 0 : 1;
            nRet = CRtmpPusher::instance()->send_video_data(nChannel, pVideoFrame);
        }
        break;
        default:
            dlog_error("未知的推流协议: %d", enProtocol);
            break;
        }
    }

    return nRet;
}
#endif

int CPushStream::sendAudioData(Audio_NS::AudioFrame_S *pAudioFrame, bool bIsMain, bool bIsRtsp)
{
    if (!pAudioFrame)
    {
        dlog_error("指针为空");
        return ERR_PTR_NULL;
    }

    int nRet = OK;
    if (m_bInitFlag == true)
    {
        if (bIsRtsp == true)
        {
            /* 发送到RTSP */
            if (bIsMain == true)
            {
                nRet = CRtspServer::instance()->sendAudioData(RTSP_CHN_MAIN, pAudioFrame);
            }
            else
            {
                nRet = CRtspServer::instance()->sendAudioData(RTSP_CHN_SUB, pAudioFrame);
            }

            /* 同时发送到RTMP（如果RTMP已初始化） */
#if CAP_RTMP_PUSH
            if (CRtmpPusher::instance()->is_init())
            {
                int nChannel = bIsMain ? 0 : 1;
                /* 音频帧转发为高频路径，默认关闭逐帧日志，排查音频转发时再临时打开。 */
                /* dlog_debug("PushStream转发音频到RTMP，通道=%d, 长度=%d, 格式=%d",
                             nChannel, pAudioFrame->nLen, static_cast<int>(pAudioFrame->enFormat)); */
                CRtmpPusher::instance()->send_audio_data(nChannel, pAudioFrame);
            }
#endif
        }
    }

    return nRet;
}

#if CAP_RTMP_PUSH
int CPushStream::sendAudioData(Audio_NS::AudioFrame_S *pAudioFrame, bool bIsMain, PushStreamProtocol_E enProtocol)
{
    if (!pAudioFrame)
    {
        dlog_error("指针为空");
        return ERR_PTR_NULL;
    }

    int nRet = OK;
    if (m_bInitFlag == true)
    {
        switch (enProtocol)
        {
        case PROTOCOL_RTSP:
            if (bIsMain == true)
            {
                nRet = CRtspServer::instance()->sendAudioData(RTSP_CHN_MAIN, pAudioFrame);
            }
            else
            {
                nRet = CRtspServer::instance()->sendAudioData(RTSP_CHN_SUB, pAudioFrame);
            }
            break;
        case PROTOCOL_RTMP:
        {
            int nChannel = bIsMain ? 0 : 1;
            nRet = CRtmpPusher::instance()->send_audio_data(nChannel, pAudioFrame);
        }
        break;
        default:
            dlog_error("未知的推流协议: %d", enProtocol);
            break;
        }
    }

    return nRet;
}
#endif

int CPushStream::sendAudioData(Audio_NS::AudioFrame_S *pAudioFrame, bool bIsRtsp)
{
    if (!pAudioFrame)
    {
        dlog_error("指针为空");
        return ERR_PTR_NULL;
    }

    int nRet = OK;
    if (m_bInitFlag == true)
    {
        if (bIsRtsp == true)
        {
            /* 发送到RTSP */
            nRet = CRtspServer::instance()->sendAudioData(RTSP_CHN_MAIN, pAudioFrame);
            nRet = CRtspServer::instance()->sendAudioData(RTSP_CHN_SUB, pAudioFrame);

            /* 同时发送到RTMP（如果RTMP已初始化） */
#if CAP_RTMP_PUSH
            if (CRtmpPusher::instance()->is_init())
            {
                /* 音频广播为高频路径，默认关闭逐帧日志，排查多码流音频时再临时打开。 */
                /* dlog_debug("PushStream广播音频到所有RTMP通道，长度=%d, 格式=%d",
                             pAudioFrame->nLen, static_cast<int>(pAudioFrame->enFormat)); */
                CRtmpPusher::instance()->send_audio_data(pAudioFrame);
            }
#endif
        }
    }

    return nRet;
}

#if CAP_RTMP_PUSH
int CPushStream::sendAudioData(Audio_NS::AudioFrame_S *pAudioFrame, PushStreamProtocol_E enProtocol)
{
    if (!pAudioFrame)
    {
        dlog_error("指针为空");
        return ERR_PTR_NULL;
    }

    int nRet = OK;
    if (m_bInitFlag == true)
    {
        switch (enProtocol)
        {
        case PROTOCOL_RTSP:
            nRet = CRtspServer::instance()->sendAudioData(RTSP_CHN_MAIN, pAudioFrame);
            nRet = CRtspServer::instance()->sendAudioData(RTSP_CHN_SUB, pAudioFrame);
            break;
        case PROTOCOL_RTMP:
            nRet = CRtmpPusher::instance()->send_audio_data(pAudioFrame);
            break;
        default:
            dlog_error("未知的推流协议: %d", enProtocol);
            break;
        }
    }

    return nRet;
}
#endif

#if CAP_RTMP_PUSH
int CPushStream::restart_rtmp_stream(const Network::Platform_Info_t &stPlatformInfo)
{
    if (!stPlatformInfo.enable)
    {
        dlog_info("平台接入未启用，停止 RTMP 推流");
        if (CRtmpPusher::instance()->is_init())
        {
            CRtmpPusher::instance()->deinit();
        }
        return 0;
    }

    if (stPlatformInfo.server_ip.empty())
    {
        dlog_error("平台服务器地址为空，无法更新 RTMP 推流地址");
        return -1;
    }

    dlog_info("更新 RTMP 推流地址: server_ip=%s, port=%d",
              stPlatformInfo.server_ip.c_str(), stPlatformInfo.server_port);

    /* 获取设备序列号 */
    System::DeviceInfo_S stDeviceInfo;
    SystemManage::instance()->get_device_info(stDeviceInfo);
    std::string strDeviceSN = stDeviceInfo.serialNumber;

    /* 获取访问 token */
    std::string strToken = CPlatformManager::instance()->get_access_token();

    /* 确保 RTMP 推流管理器已初始化 */
    if (!CRtmpPusher::instance()->is_init())
    {
        //CRtmpPusher::instance()->init();
        const int nInitRet = CRtmpPusher::instance()->init();
        if (nInitRet != OK)
        {
            dlog_error("RTMP推流管理器初始化失败: %d", nInitRet);
            return nInitRet;
        }
    }

    /* 获取音视频配置 */
    Audio_NS::AudioConfig_S stAudioConfig;
    CAVConfigure::instance()->get_configure(stAudioConfig);

    std::set<Video_NS::VideoConfig_S> setVideoConfig;
    CAVConfigure::instance()->get_configure(setVideoConfig);

    /* 遍历所有视频通道，热更新推流地址 */
    int nRestartRet = OK;
    for (const auto &stVideoConfig : setVideoConfig)
    {
        if (!is_rtmp_video_config_supported(stVideoConfig))
        {
            continue;
        }

        /* 先停止该通道的推流 */
        CRtmpPusher::instance()->stop_push(stVideoConfig.nId);

        /* 用新地址重新启动 */
        std::string strRtmpUrl = build_rtmp_url(stPlatformInfo, stVideoConfig.nId, strDeviceSN, strToken);
        dlog_info("RTMP推流地址重新生成，通道=%d, URL=%s", stVideoConfig.nId, strRtmpUrl.c_str());
        int nRet = CRtmpPusher::instance()->start_push(stVideoConfig.nId, strRtmpUrl, stVideoConfig, stAudioConfig);
        if (nRet != OK)
        {
            dlog_error("通道%d RTMP 推流重启失败: %d", stVideoConfig.nId, nRet);
            /* 保留失败结果，不能让平台层将本次网络切换误判为完全成功。 */
            if (nRestartRet == OK)
            {
                nRestartRet = nRet;
            }
        }
    }

    if (nRestartRet != OK)
    {
        /*
         * 网络切换后首次连接可能处于路由或旧连接释放窗口。
         * 失败会话已保存在推流管理器中，立即请求其重连线程再次尝试。
         */
        CRtmpPusher::instance()->trigger_reconnect();
        dlog_error("RTMP 推流地址更新失败: %d，已请求立即重连", nRestartRet);
        return nRestartRet;
    }

    dlog_info("RTMP 推流地址更新完成");
    return OK;
    //return 0;
}

int CPushStream::restart_rtmp_by_channel(int nChannel)
{
    Network::Platform_Info_t stPlatformInfo;
    if (!get_rtmp_platform_info(stPlatformInfo))
    {
        dlog_warn("RTMP推流未启用或平台配置无效，通道=%d", nChannel);
        return 0;
    }

    if (!CRtmpPusher::instance()->is_init())
    {
        dlog_info("RTMP推流管理器未初始化，跳过通道%d重启", nChannel);
        return 0;
    }

    /* 获取设备序列号 */
    System::DeviceInfo_S stDeviceInfo;
    SystemManage::instance()->get_device_info(stDeviceInfo);
    std::string strDeviceSN = stDeviceInfo.serialNumber;

    /* 获取访问 token */
    std::string strToken = CPlatformManager::instance()->get_access_token();

    /* 获取音频配置 */
    Audio_NS::AudioConfig_S stAudioConfig;
    CAVConfigure::instance()->get_configure(stAudioConfig);

    /* 获取指定通道的最新视频配置 */
    Video_NS::VideoConfig_S stTargetConfig;
    stTargetConfig.nId = nChannel;
    CAVConfigure::instance()->get_configure(stTargetConfig);

    if (!is_rtmp_video_config_supported(stTargetConfig))
    {
        dlog_warn("通道%d视频配置不支持RTMP推流，跳过重启", nChannel);
        return 0;
    }

    dlog_info("视频配置变更，主动重启RTMP推流，通道=%d", nChannel);

    /* 先停止该通道的推流 */
    CRtmpPusher::instance()->stop_push(nChannel);

    /* 用最新配置重新启动 */
    std::string strRtmpUrl = build_rtmp_url(stPlatformInfo, nChannel, strDeviceSN, strToken);
    dlog_info("RTMP推流地址重新生成，通道=%d, URL=%s", nChannel, strRtmpUrl.c_str());

    int nRet = CRtmpPusher::instance()->start_push(nChannel, strRtmpUrl, stTargetConfig, stAudioConfig);
    if (nRet != OK)
    {
        dlog_error("通道%d RTMP推流重启失败: %d，触发重连线程立即重试", nChannel, nRet);
        CRtmpPusher::instance()->trigger_reconnect();
        return nRet;
    }

    dlog_info("通道%d RTMP推流重启成功", nChannel);
    return OK;
}
#endif
