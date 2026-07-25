/**
 * @FilePath     : stream_video.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-03-21 10:24:45
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-12 14:25:09
 * @Description  : 流媒体视频模块
 */

#include "stream_video.h"
#include "convert_interface.h"
#include "RtpServer.h"
#include "get_time.h"
#include "isp_control.h"
#include "stream_server.h"
#include "action_code.h"
#include "capture_ctrl.h"
#include "record_ctrl.h"
#include "system_utils.h"
#if CAP_GARBAGE_STATION_PLATFORM
#include "push_stream.h"
#endif

#include <chrono>

namespace
{
    /* VENC取流诊断限频，避免编码线程逐帧刷日志影响实时性 */
    const long long VENC_RTMP_DIAG_LOG_INTERVAL_MS = 5000;

    long long venc_diag_now_ms()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    bool venc_diag_should_log(long long& llLastLogMs)
    {
        const long long llNowMs = venc_diag_now_ms();
        if (llLastLogMs == 0 || llNowMs - llLastLogMs >= VENC_RTMP_DIAG_LOG_INTERVAL_MS)
        {
            llLastLogMs = llNowMs;
            return true;
        }
        return false;
    }

    unsigned char venc_diag_byte_at(const unsigned char* pData, int nLen, int nIndex)
    {
        if (!pData || nIndex < 0 || nIndex >= nLen)
        {
            return 0;
        }
        return pData[nIndex];
    }
}

CStreamVideo* CStreamVideo::m_self = NULL;
std::mutex CStreamVideo::m_mutex;

CStreamVideo::CStreamVideo()
{
    for (int i = 0; i < VENC_CHN_MAX; i++)
    {
        m_bVencFlag[i].store(false, std::memory_order_release);
    }
    for (int i = 0; i < VPSS_CHANNEL_SUM; i++)
    {
        m_bVpssFlag[i].store(false, std::memory_order_release);
    }
}

CStreamVideo::~CStreamVideo()
{

}

IpcRet_E CStreamVideo::init()
{
    int nRet = OK;

    /*初始化通道处理器策略*/
    m_channelHandlers[VENC_CHN_MAIN] = std::make_unique<CMainChannelHandler>(this);
    m_channelHandlers[VENC_CHN_SUB]  = std::make_unique<CSubChannelHandler>(this);
    m_channelHandlers[VENC_CHN_JPEG] = std::make_unique<CJpegChannelHandler>();

    /*初始化NAL解析器策略*/
    m_nalParsers[Video_NS::VideoCodec_E::H264] = std::make_unique<CH264NalParser>();
    m_nalParsers[Video_NS::VideoCodec_E::H265] = std::make_unique<CH265NalParser>();
    m_nalParsers[Video_NS::VideoCodec_E::SVAC3] = std::make_unique<CSvac3NalParser>();
    m_nalParsers[Video_NS::VideoCodec_E::MJPEG] = std::make_unique<CMjpegParser>();

    /*初始化与各模块的回调绑定*/
    initCallbackBinding();

    /* 从配置管理对象获取配置 */
    const auto& videoConfigs = m_configManager.getVideoConfigs();

    /*初始化海思MPI系统/公共视频缓存池*/
    nRet = streamSys_init(videoConfigs);
    if (nRet != OK)
    {
        dlog_error("MPI系统/视频缓存池初始化失败");
        return ERR;
    }
    /*初始化VI*/
    m_viHandle.reset(streamVi_init());
    if(!m_viHandle)
    {
        dlog_error("Vi模块初始化失败");
        return ERR;
    }

    /* isp 图像初始化 */
    CIspControl::instance()->init();

    /*初始化VPSS*/
    streamVpss_init(&m_pVpssHandle, videoConfigs);

    /*初始化抓图 JPEG 的编码配置*/
    Video_NS::VideoConfig_S stVideoConfigJpeg = videoConfigs[VENC_CHN_SUB];
    stVideoConfigJpeg.nId = VENC_CHN_JPEG;
    stVideoConfigJpeg.enVideoCodec = Video_NS::VideoCodec_E::JPEG;
    stVideoConfigJpeg.stVideoResolution.nWidth = m_pVpssHandle[VPSS_MAIN_SUB]->astVpssChnAttr[VPSS_CHANNEL_AI].nWidth;
    stVideoConfigJpeg.stVideoResolution.nHeight = m_pVpssHandle[VPSS_MAIN_SUB]->astVpssChnAttr[VPSS_CHANNEL_AI].nHeight;
    /* 两帧JPEG组成一张完整图片，帧率设置为2 */
    stVideoConfigJpeg.enFrameRate = Video_NS::FrameRate_E::FRAME_RATE_2;
    m_configManager.updateVideoConfig(stVideoConfigJpeg);

    /*初始化VENC*/
    const auto& roiConfigs = m_configManager.getVideoRoiConfigs();
    for (int i = 0; i < VENC_CHN_MAX; i++)
    {
        if(i != VENC_CHN_JPEG)
        {
            m_vencHandles[i].reset(streamVenc_init(videoConfigs[i], roiConfigs[i]));
        }else{
            m_vencHandles[i].reset(streamVenc_init(stVideoConfigJpeg));
        }
        if(!m_vencHandles[i])
        {
            dlog_error("Venc模块初始化失败 通道:%d", i);
            return ERR;
        }
        m_bVencFlag[i].store(true, std::memory_order_release);
        m_getVencThread[i] = std::thread(&CStreamVideo::get_vencStream, this, i);
        // if (m_getVencThread[i].joinable())
        // {
        //     m_getVencThread[i].detach();
        // }
        /* 设置线程优先级 */
        // setThreadPriority(m_getVencThread[i], SCHED_RR, VIDEO_DATA_THREAD_PRIORITY);
    }

    /* 初始化 AI_APP */
    algo_detect_init();
    /*启动AI送帧线程*/
    m_bVpssFlag[VPSS_CHANNEL_AI].store(true, std::memory_order_release);
    m_getVpssThread[VPSS_CHANNEL_AI] = std::thread(&CStreamVideo::get_vpssStream, this);
    // if (m_getVpssThread[VPSS_CHANNEL_AI].joinable())
    // {
    //     m_getVpssThread[VPSS_CHANNEL_AI].detach();
    // }
    /* 设置线程优先级 */
    // setThreadPriority(m_getVpssThread[VPSS_CHANNEL_AI], SCHED_RR, VIDEO_DATA_THREAD_PRIORITY);

    /* 开启OSD管理模块 */
    COsdManage::instance()->init();
    
    /*绑定模块*/
    nRet = bindModule();
    if (nRet != OK)
    {
        dlog_error("绑定模块失败");
        return ERR;
    }else{
        dlog_info("绑定模块成功");
    }

    m_bInitFlag = true;

    /* 区域裁剪 */
    const auto& areaCropConfigs = m_configManager.getAreaCropConfigs();
    for (const auto &stAreaCrop : areaCropConfigs)
    {
        /* 启用才进行设置 */
        if(stAreaCrop.bEnable)
        {
            setAreaCropConfig(stAreaCrop, true);
        }
    }

    dlog_info("流媒体视频初始化成功");
    return OK;
}

IpcRet_E CStreamVideo::deinit()
{
    int nRet = OK;
    m_bInitFlag = false;

    /*解绑模块*/
    nRet = unbindModule();
    if (nRet != OK)
    {
        dlog_error("解绑模块失败");
        return ERR;
    }else{
        dlog_info("解绑模块成功");
    }

    /*停止AI送帧线程*/ 
    m_bVpssFlag[VPSS_CHANNEL_AI].store(false, std::memory_order_release);
    if(m_getVpssThread[VPSS_CHANNEL_AI].joinable())
    {
        m_getVpssThread[VPSS_CHANNEL_AI].join();
    }
    /* 去初始化 AI_APP */
    algo_detect_deinit();

    /* 关闭OSD管理模块 */
    COsdManage::instance()->deinit();

    /*去初始化VENC*/
    for (int i = 0; i < VENC_CHN_MAX; i++)
    {
        m_bVencFlag[i].store(false, std::memory_order_release);
        if(m_getVencThread[i].joinable())
        {
            m_getVencThread[i].join();
        }
        /* RAII自动释放资源 */
        m_vencHandles[i].reset();
    }
    /*去初始化VPSS*/
    streamVpss_uninit();

    /* isp 图像去初始化 */
    CIspControl::instance()->deinit();

    /*去初始化VI*/
    /* RAII自动释放资源 */
    m_viHandle.reset();
    /*去初始化海思MPI系统/视频缓存池*/
    nRet = streamSys_deinit();
    if (nRet != OK)
    {
        dlog_error("MPI系统/视频缓存池去初始化失败");
        return ERR;
    }

    dlog_info("流媒体视频去初始化成功");
    return OK;
}

IpcRet_E CStreamVideo::reboot()
{
    int nRet = OK;
    std::lock_guard<std::mutex> lock(m_mutexCtrl);

    if (m_bInitFlag)
    {
        nRet = deinit();
        if (nRet < 0)
        {
            dlog_error("反初始化流媒体视频模块失败");
            return ERR;
        }
    }
    nRet = init();
    if (nRet < 0)
    {
        dlog_error("初始化流媒体视频模块失败");
        return ERR;
    }

    return OK;
}

IpcRet_E CStreamVideo::reboot_venc(int nChn, const Video_NS::VideoConfig_S &stVideoConfig, bool bUseIncomingAttr)
{
    int nRet = OK;

    /* 停止获取编码通道数据 */
    // m_bVencFlag[nChn].store(false, std::memory_order_release);
    // if(m_getVencThread[nChn].joinable())
    // {
    //     m_getVencThread[nChn].join();
    // }
    /* 解绑定VPSS Chn -> VENC */
    vpssUnbindVencModule(nChn);

    if (m_bInitFlag)
    {
        const auto& roiConfig = m_configManager.getVideoRoiConfigs().at(nChn);
        if(bUseIncomingAttr)
        {
            nRet = streamVenc_reset(m_vencHandles[nChn].get_ref(), stVideoConfig, roiConfig);
        }
        else
        {
            const auto &videoConfig = m_configManager.getVideoConfigs().at(nChn);
            nRet = streamVenc_reset(m_vencHandles[nChn].get_ref(), videoConfig, roiConfig);
        }
        if (nRet != OK)
        {
            dlog_error("重新启动视频流模块失败");
            return ERR;
        }
    }

    /* 绑定VPSS Chn -> VENC */
    vpssBindVencModule(nChn);
    /* 启动获取编码通道数据 */
    // m_bVencFlag[nChn].store(true, std::memory_order_release);
    // m_getVencThread[nChn] = std::thread(&CStreamVideo::get_vencStream, this, nChn);

    dlog_info("重新启动视频流编码通道:%d 成功", nChn);
    return OK;
}

void CStreamVideo::request_idr(int nChannel)
{
    int nRet = OK;
    td_bool bInstant = TD_TRUE;
    dlog_trace("通道%d 请求I帧", nChannel);
    if(m_vencHandles[nChannel])
    {
        nRet = m_vencHandles[nChannel]->mppVenc_request_idr(m_vencHandles[nChannel].get(), bInstant);
        if (nRet != OK)
        {
            dlog_error("请求I帧失败:%x", nRet);
        }
    }
}

HiVi_S *CStreamVideo::get_viHandle()
{
    return m_viHandle.get();
}

HiVpss_S *CStreamVideo::get_vpssHandle(int nGrp)
{
    if (nGrp < VPSS_MAIN_SUB || nGrp >= VPSS_GROUP_SUM)
    {
        dlog_error("传入的VPSS的组:%d错误", nGrp);
        return nullptr;
    }

    if (!m_bInitFlag)
    {
        dlog_error("流媒体视频未初始化");
        return nullptr;
    }

    return m_pVpssHandle[nGrp];
}

int CStreamVideo::getVideoConfig(std::vector<Video_NS::VideoConfig_S> &vstVideoConfig)
{
    vstVideoConfig = m_configManager.getVideoConfigs();
    return OK;
}

int CStreamVideo::setVideoConfig(const Video_NS::VideoConfig_S &stVideoConfig)
{
    bool bIsSetVpss = false;
    bool bIsResetRtsp = false;
    bool bIsUpdateOsd = false;
    int nId = stVideoConfig.nId;
    int nRet = OK;

    /* 获取当前配置用于比较 */
    const auto currentConfig = m_configManager.getVideoConfigs().at(nId);

    // note: 视频分辨率变更时，需变更 VPSS 通道属性
    if (currentConfig.stVideoResolution.nWidth != stVideoConfig.stVideoResolution.nWidth ||
        currentConfig.stVideoResolution.nHeight != stVideoConfig.stVideoResolution.nHeight)
    {
        bIsSetVpss = true;
        bIsUpdateOsd = true;
    }

    // note: 视频编码格式、视频类型、帧率变更时，需重启 RtspServer
    if (currentConfig.enVideoCodec != stVideoConfig.enVideoCodec ||
        currentConfig.enVideoType != stVideoConfig.enVideoType ||
        currentConfig.enFrameRate != stVideoConfig.enFrameRate)
    {
        bIsResetRtsp = true;
    }

    std::lock_guard<std::mutex> lock(m_mutexCtrl);
    /* 更新视频配置 */
    m_configManager.updateVideoConfig(stVideoConfig);
    /* 同步录制进程 */
#if CAP_RECORD_USE_MAIN_STREAM
    if (nId == STREAM_MEDIA_MAIN)
#else
    if (nId == STREAM_MEDIA_SUB)
#endif
    {
        nRet = CStreamServer::instance()->sendVideoConfig(stVideoConfig);
        if(nRet != OK)
        {
            dlog_error("更新视频配置,同步录制进程失败");
        }
    }

    CRtspServer::instance()->setVideoConfig(m_configManager.getVideoConfigs());
    if (bIsResetRtsp)
    {
        /* 重新启动RTSP服务器 */
        nRet = CRtspServer::instance()->reboot();
        if(nRet != OK)
        {
            dlog_error("重新启动RTSP服务器失败");
            return ERR;
        }
    }

    /* 区域裁剪 */
    auto &areaCropConfig = m_configManager.getAreaCropConfigRef(nId);

    /* 重新启动VENC */
    /* 停止获取编码通道数据 */
    m_bVencFlag[nId].store(false, std::memory_order_release);
    if (m_getVencThread[nId].joinable())
    {
        m_getVencThread[nId].join();
    }
    /* 解绑定VPSS Chn -> VENC */
    vpssUnbindVencModule(nId);

    if (bIsSetVpss)
    {
        /* 如果开启了裁剪，先关闭裁剪 */
        if (areaCropConfig.bEnable)
        {
            Video_NS::AreaCrop_S stAreaCropConfig = areaCropConfig;
            stAreaCropConfig.bEnable = false;
            nRet = streamVpss_set_chnCrop(m_pVpssHandle[VPSS_MAIN_SUB], stAreaCropConfig);
            if (OK != nRet)
            {
                dlog_error("设置 VPSS 通道裁剪失败");
                return ERR;
            }
        }

        /* 重新设置VPSS相关属性 */
        if (nId == VPSS_CHANNEL_MAIN)
        {
            /* 重新设置 VPSS 卷绕 */
            nRet = streamVpss_reset_wrap(m_pVpssHandle[VPSS_MAIN_SUB], stVideoConfig);
            if(nRet != OK)
            {
                dlog_error("重新设置 VPSS 卷绕失败");
                return ERR;
            }
        }
        else if (nId == VPSS_CHANNEL_SUB)
        {
            /* 设置 VPSS 对应通道分辨率 */
            nRet = streamVpss_set_chnAttr(m_pVpssHandle[VPSS_MAIN_SUB], stVideoConfig);
            if (nRet != OK)
            {
                dlog_error("设置 VPSS 对应通道分辨率失败");
                return ERR;
            }
        }

        /* 分辨率更新，更新区域裁剪 */
        /* 转换插件坐标参考系为修改前视频分辨率 */
        areaCropConfig.stRect.ConvertResolution(PLUG_IN_WIDTH_DEFAULT,
                                                PLUG_IN_HEIGHT_DEFAULT,
                                                currentConfig.stVideoResolution.nWidth,
                                                currentConfig.stVideoResolution.nHeight);
        /* 以右下角点为基准，计算左上角点坐标 */
        Common::Pos_S stPos;
        stPos.nX = areaCropConfig.stRect.nX + areaCropConfig.stRect.nWidth;
        stPos.nY = areaCropConfig.stRect.nY + areaCropConfig.stRect.nHeight;
        areaCropConfig.stRect.nX = stPos.nX - areaCropConfig.stResolution.nWidth;
        areaCropConfig.stRect.nY = stPos.nY - areaCropConfig.stResolution.nHeight;
        areaCropConfig.stRect.nWidth = areaCropConfig.stResolution.nWidth;
        areaCropConfig.stRect.nHeight = areaCropConfig.stResolution.nHeight;
        /* 转换插件坐标参考系为插件分辨率 */
        areaCropConfig.stRect.ConvertResolution(stVideoConfig.stVideoResolution.nWidth,
                                                stVideoConfig.stVideoResolution.nHeight,
                                                PLUG_IN_WIDTH_DEFAULT,
                                                PLUG_IN_HEIGHT_DEFAULT);
        /* 修改后的视频分辨率比区域裁剪分辨率小，则强制同步区域裁剪分辨率 */
        if (areaCropConfig.stResolution.nWidth > stVideoConfig.stVideoResolution.nWidth ||
            areaCropConfig.stResolution.nHeight > stVideoConfig.stVideoResolution.nHeight)
        {
            areaCropConfig.stResolution = stVideoConfig.stVideoResolution;
        }
        /* 判断区域裁剪分辨率与视频配置分辨率是否一致，一致则关闭区域裁剪 */
        if (areaCropConfig.bEnable)
        {
            if (areaCropConfig.stResolution != stVideoConfig.stVideoResolution)
            {
                /* 设置 VPSS 对应通道裁剪 */
                nRet = streamVpss_set_chnCrop(m_pVpssHandle[VPSS_MAIN_SUB], areaCropConfig);
                if (OK != nRet)
                {
                    dlog_error("设置 VPSS 通道裁剪失败");
                    return ERR;
                }
            }
            else
            {
                dlog_warn("区域裁剪分辨率与视频配置分辨率一致,关闭区域裁剪");
                areaCropConfig.bEnable = false;
            }
        }

        /* 先清空设置区域裁剪的回调，再更新区域裁剪配置，恢复回调 */
        CAVConfigure::instance()->setAreaCropConfigCallback(SetAreaCropConfigCallback());
        CAVConfigure::instance()->set_configure(areaCropConfig);
        CAVConfigure::instance()->setAreaCropConfigCallback(
            [this](const Video_NS::AreaCrop_S &stConfig) -> int
            {
                return this->setAreaCropConfig(stConfig);
            });
    }

    auto currentVideoConfig = stVideoConfig;
    /* 开启区域裁剪，视频编码分辨率与裁剪配置的分辨率一致 */
    if (areaCropConfig.bEnable)
    {
        currentVideoConfig.stVideoResolution = areaCropConfig.stResolution;
    }

    /* 重启视频编码 */
    const auto &roiConfig = m_configManager.getVideoRoiConfigs().at(nId);
    nRet = streamVenc_reset(m_vencHandles[nId].get_ref(), currentVideoConfig, roiConfig);
    if (nRet != OK)
    {
        dlog_error("重启视频编码失败");
        return ERR;
    }

    /* 绑定VPSS Chn -> VENC */
    vpssBindVencModule(nId);
    /* 启动获取编码通道数据 */
    m_bVencFlag[nId].store(true, std::memory_order_release);
    m_getVencThread[nId] = std::thread(&CStreamVideo::get_vencStream, this, nId);

    dlog_info("重新启动视频流编码通道:%d 成功", nId);

    /* 主动请求 IDR 帧，加速 RTMP 推流的首个 SPS/PPS 到达 */
    request_idr(nId);

#if CAP_GARBAGE_STATION_PLATFORM
    /* 视频编码配置变更后，主动重启对应通道的 RTMP 推流，避免被动等待 Broken pipe */
    CPushStream::instance()->restart_rtmp_by_channel(nId);
#endif

    /* 更新OSD模块 */
    if (bIsUpdateOsd)
    {
        COsdManage::instance()->update_osd_flag();
    }

    dlog_info("设置通道:%d 视频配置成功", nId);
    return OK;
}

int CStreamVideo::setVideoRoiConfig(const Video_NS::VideoRoiConfig_S &stVideoRoiConfig)
{
    std::lock_guard<std::mutex> lock(m_mutexCtrl);
    /*更新视频ROI配置*/
    int nId = stVideoRoiConfig.nId;
    int nRet = streamVenc_set_roi_attr(m_vencHandles[nId].get(), stVideoRoiConfig);
    if (nRet == OK)
    {
        m_configManager.updateVideoRoiConfig(stVideoRoiConfig);
        dlog_info("设置通道:%d 视频ROI配置成功", nId);
    }

    return nRet;
}

int CStreamVideo::setAreaCropConfig(const Video_NS::AreaCrop_S &stAreaCrop, bool bIsMandateSet)
{
    if (!m_bInitFlag)
    {
        dlog_error("设置区域裁剪失败，视频模块未初始化");
        return ERR_UNINIT;
    }

    if (!stAreaCrop.stRect.IsValid() || stAreaCrop.nId > 1 || stAreaCrop.nId < 0)
    {
        dlog_error("设置区域裁剪失败，配置参数不正确");
        return ERR_PARAM;
    }

    std::lock_guard<std::mutex> lock(m_mutexCtrl);

    int nRet = OK;
    int nId = stAreaCrop.nId;
    auto& currentCropConfig = m_configManager.getAreaCropConfigRef(nId);
    auto currentVideoConfig = m_configManager.getVideoConfigRef(nId);

    /* 视频分辨率和区域裁剪分辨率相同且区域裁剪启用时，不进行区域裁剪 */
    if (stAreaCrop.stResolution == currentVideoConfig.stVideoResolution && stAreaCrop.bEnable)
    {
        dlog_warn("视频分辨率和区域裁剪分辨率相同，不进行区域裁剪");
        return ERR_PARAM;
    }

    // note: 是否需变更 VPSS 通道、VENC 属性
    if (currentCropConfig.stResolution.nWidth != stAreaCrop.stResolution.nWidth ||
        currentCropConfig.stResolution.nHeight != stAreaCrop.stResolution.nHeight ||
        currentCropConfig.stRect != stAreaCrop.stRect ||
        currentCropConfig.bEnable != stAreaCrop.bEnable || bIsMandateSet)
    {
        if (stAreaCrop.bEnable)
        {
            /* 开启区域裁剪，VENC分辨率设置为裁剪区域的宽高,仅更改视频流这里的配置 */
            currentVideoConfig.stVideoResolution = stAreaCrop.stResolution;
        }
        /* 重新启动VENC */
        /* 停止获取编码通道数据 */
        m_bVencFlag[nId].store(false, std::memory_order_release);
        if (m_getVencThread[nId].joinable())
        {
            m_getVencThread[nId].join();
        }
        /* 解绑定VPSS Chn -> VENC */
        vpssUnbindVencModule(nId);
        /* 重启视频编码 */
        const auto &roiConfig = m_configManager.getVideoRoiConfigs().at(nId);
        nRet = streamVenc_reset(m_vencHandles[nId].get_ref(), currentVideoConfig, roiConfig);
        if (nRet != OK)
        {
            dlog_error("重启视频编码失败");
            return ERR;
        }

        /* 设置 VPSS 对应通道裁剪 */
        nRet = streamVpss_set_chnCrop(m_pVpssHandle[VPSS_MAIN_SUB], stAreaCrop);
        if (OK != nRet)
        {
            dlog_error("设置 VPSS 通道裁剪失败");
            return ERR;
        }

        /* 绑定VPSS Chn -> VENC */
        vpssBindVencModule(nId);
        /* 启动获取编码通道数据 */
        m_bVencFlag[nId].store(true, std::memory_order_release);
        m_getVencThread[nId] = std::thread(&CStreamVideo::get_vencStream, this, nId);

        dlog_info("重新启动视频流编码通道:%d 成功", nId);
    }

    /*更新区域裁剪配置*/
    m_configManager.updateAreaCropConfig(stAreaCrop);

    dlog_info("设置通道:%d 区域裁剪配置成功", nId);
    return OK;
}

int CStreamVideo::bindModule()
{
    int nRet = OK;
    nRet |= mppVi_bind_vpss(m_viHandle->stNeedParam.nDevId, m_viHandle->stExParam.nPipeId, VPSS_MAIN_SUB, 0);
    nRet |= mppVpss_bind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_MAIN, VENC_CHN_MAIN);
    nRet |= mppVpss_bind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_SUB, VENC_CHN_SUB);
    nRet |= mppVpss_bind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_AI, VENC_CHN_JPEG);
    return nRet;
}

int CStreamVideo::unbindModule()
{
    int nRet = OK;
    nRet |= mppVi_unbind_vpss(m_viHandle->stNeedParam.nDevId, m_viHandle->stExParam.nPipeId, VPSS_MAIN_SUB, 0);
    nRet |= mppVpss_unbind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_MAIN, VENC_CHN_MAIN);
    nRet |= mppVpss_unbind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_SUB, VENC_CHN_SUB);
    nRet |= mppVpss_unbind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_AI, VENC_CHN_JPEG);
    return nRet;
}

int CStreamVideo::vpssBindVencModule(int nVencChn)
{
    int nRet = OK;
    if(nVencChn == VENC_CHN_MAIN)
    {
        nRet |= mppVpss_bind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_MAIN, VENC_CHN_MAIN);
    }
    else if (nVencChn == VENC_CHN_SUB)
    {
        nRet |= mppVpss_bind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_SUB, VENC_CHN_SUB);
    }
    else if (nVencChn == VENC_CHN_JPEG)
    {
        nRet |= mppVpss_bind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_AI, VENC_CHN_JPEG);
    }
    dlog_info("绑定VPSS->VENC VencChn:[%d]", nVencChn);
    return nRet;
}

int CStreamVideo::vpssUnbindVencModule(int nVencChn)
{
    int nRet = OK;
    if(nVencChn == VENC_CHN_MAIN)
    {
        nRet |= mppVpss_unbind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_MAIN, VENC_CHN_MAIN);
    }
    else if (nVencChn == VENC_CHN_SUB)
    {
        nRet |= mppVpss_unbind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_SUB, VENC_CHN_SUB);
    }
    else if (nVencChn == VENC_CHN_JPEG)
    {
        nRet |= mppVpss_unbind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_AI, VENC_CHN_JPEG);
    }
    dlog_info("解绑定VPSS->VENC VencChn:[%d]", nVencChn);
    return nRet;
}

void CStreamVideo::initCallbackBinding()
{
    /*设置请求IDR帧回调*/
    CRtspServer::instance()->setRequestIdrCallback(
        [this](int nChannel, void *pUserData) -> void
        {
            return this->request_idr(nChannel);
        },
        &m_bInitFlag);

    /*绑定视频配置更新回调*/
    CAVConfigure::instance()->setVideoConfigCallback(
        [this](const Video_NS::VideoConfig_S &stVideoConfig) -> int
        {
            return this->setVideoConfig(stVideoConfig);
        });

    /*绑定视频ROI配置更新回调*/
    CAVConfigure::instance()->setVideoRoiConfigCallback(
        [this](const Video_NS::VideoRoiConfig_S &stVideoRoiConfig) -> int
        {
            return this->setVideoRoiConfig(stVideoRoiConfig);
        });

    /*绑定区域裁剪配置更新回调*/
    CAVConfigure::instance()->setAreaCropConfigCallback(
        [this](const Video_NS::AreaCrop_S &stConfig) -> int
        {
            return this->setAreaCropConfig(stConfig);
        });

    /* 获取jpeg编码通道参数回调 */
    CCaptureCtrl::instance()->get_jpegVencParamCallback(
        [this](unsigned int &unWidth, unsigned int &unHeight, unsigned int &nUqFactor) -> int
        {
            return this->getJpegVencParam(unWidth, unHeight, nUqFactor);
        });
}

int CStreamVideo::getJpegVencParam(unsigned int &unWidth, unsigned int &unHeight, unsigned int &nUqFactor)
{
    unWidth = m_vencHandles[VENC_CHN_JPEG]->stNeedParam.unWidth;
    unHeight = m_vencHandles[VENC_CHN_JPEG]->stNeedParam.unHeight;
    nUqFactor = m_vencHandles[VENC_CHN_JPEG]->stExParam.uQFactor;

    return 0;
}

Video_NS::VideoFrame_S *CStreamVideo::createFrame(VENC_CHN_E enChn, uint8_t *pData, int nDataLen)
{
    if (!pData || nDataLen < 6)
    {
        dlog_error("传入参数不正确");
        return nullptr;
    }

    /*分配连续内存：结构体 + 数据*/
    Video_NS::VideoFrame_S *pVideoFrame = (Video_NS::VideoFrame_S*)malloc(sizeof(Video_NS::VideoFrame_S) + nDataLen);
    if (!pVideoFrame)
    {
        dlog_error("内存分配失败");
        return nullptr;
    }

    const auto& videoConfig = m_configManager.getVideoConfigs().at(enChn);

    memcpy(pVideoFrame->pData, pData, nDataLen);
    pVideoFrame->nLen = nDataLen;
    pVideoFrame->enVideoCodec = videoConfig.enVideoCodec;

    /* 使用NAL解析器策略解析NAL类型 */
    auto it = m_nalParsers.find(videoConfig.enVideoCodec);
    if (it != m_nalParsers.end() && it->second)
    {
        pVideoFrame->eType = it->second->parseNalType(pData, nDataLen);
    }
    else
    {
        dlog_error("未找到对应的NAL解析器，编码类型：%d", static_cast<int>(videoConfig.enVideoCodec));
        delete pVideoFrame;
        return nullptr;
    }

    return pVideoFrame;
}

void CStreamVideo::freeFrame(Video_NS::VideoFrame_S *pVideoFrame)
{
    if (pVideoFrame)
    {
        delete pVideoFrame; // 仅释放结构体，不释放 pData
        pVideoFrame = nullptr;
    }
}

//info /*----------------------- 私有线程函数 -----------------------*/

void CStreamVideo::get_vencStream(int param)
{
    pthread_setname_np(pthread_self(), "get_vencStream");
    try
    {
        int nRet = 0;
        ot_venc_stream stFrame;
        unsigned char *pData = NULL;
        int nDataLen = 0;
        int nChannel = param;
        HiVenc_S *pHandle = m_vencHandles[nChannel].get();
        static long long s_anLastVencDiagLogMs[VENC_CHN_MAX] = {0};

        while (true == m_bVencFlag[nChannel].load(std::memory_order_acquire))
        {
            memset(&stFrame, 0, sizeof(ot_venc_stream));
            /*获取编码码流*/
            nRet = pHandle->mppVenc_get_stream(pHandle, &stFrame, TIMEOUT_500_MS);
            if (nRet != OK)
            {
                // dlog_error("mppVenc_get_stream chn: %d error: %x", nChannel, nRet);
                continue;
            }

            if (nChannel >= 0 && nChannel < VENC_CHN_MAX && venc_diag_should_log(s_anLastVencDiagLogMs[nChannel]))
            {
                const int nPackCount = static_cast<int>(stFrame.pack_cnt);
                dlog_info("VENC取流诊断，channel=%d, pack_cnt=%d, pack_ptr=%p",
                          nChannel,
                          nPackCount,
                          static_cast<void*>(stFrame.pack));

                if (!stFrame.pack && nPackCount > 0)
                {
                    dlog_warn("VENC取流诊断异常，channel=%d, pack_cnt=%d, pack_ptr为空", nChannel, nPackCount);
                }
                const int nLogPackCount = (!stFrame.pack) ? 0 : (nPackCount > 4 ? 4 : nPackCount);
                for (int nPackIndex = 0; nPackIndex < nLogPackCount; ++nPackIndex)
                {
                    const int nPackLen = static_cast<int>(stFrame.pack[nPackIndex].len);
                    const int nPackOffset = static_cast<int>(stFrame.pack[nPackIndex].offset);
                    const int nDiagDataLen = nPackLen - nPackOffset;
                    unsigned char* pDiagData = nullptr;
                    if (stFrame.pack[nPackIndex].addr && nDiagDataLen > 0)
                    {
                        pDiagData = stFrame.pack[nPackIndex].addr + stFrame.pack[nPackIndex].offset;
                    }

                    dlog_info("VENC包诊断，channel=%d, pack=%d/%d, len=%d, offset=%d, data_len=%d, pts=%llu, frame_end=%d, first=%02x %02x %02x %02x %02x %02x %02x %02x",
                              nChannel,
                              nPackIndex + 1,
                              nPackCount,
                              nPackLen,
                              nPackOffset,
                              nDiagDataLen,
                              static_cast<unsigned long long>(stFrame.pack[nPackIndex].pts),
                              stFrame.pack[nPackIndex].is_frame_end ? 1 : 0,
                              venc_diag_byte_at(pDiagData, nDiagDataLen, 0),
                              venc_diag_byte_at(pDiagData, nDiagDataLen, 1),
                              venc_diag_byte_at(pDiagData, nDiagDataLen, 2),
                              venc_diag_byte_at(pDiagData, nDiagDataLen, 3),
                              venc_diag_byte_at(pDiagData, nDiagDataLen, 4),
                              venc_diag_byte_at(pDiagData, nDiagDataLen, 5),
                              venc_diag_byte_at(pDiagData, nDiagDataLen, 6),
                              venc_diag_byte_at(pDiagData, nDiagDataLen, 7));
                }
            }

            std::lock_guard<std::mutex> lock(m_mutexSendData);

            for (int i = 0; i < (int)stFrame.pack_cnt; i++)
            {
                pData = stFrame.pack[i].addr + stFrame.pack[i].offset;
                nDataLen = stFrame.pack[i].len - stFrame.pack[i].offset;
                if(nDataLen < 0)
                {
                    throw std::runtime_error("stFrame nDataLen < 0 !");
                }
                // dlog_info("nChannel:%d stFrame.pack_cnt:%d nDataLen:%d", nChannel, (int)stFrame.pack_cnt, nDataLen);

                /* 使用通道处理器策略处理帧数据 */
                if (m_channelHandlers[nChannel])
                {
                    Video_NS::VideoFrame_S* pVideoFrame = nullptr;

                    /* JPEG通道不需要创建VideoFrame */
                    if (nChannel != VENC_CHN_JPEG)
                    {
                        pVideoFrame = createFrame(static_cast<VENC_CHN_E>(nChannel), pData, nDataLen);
                    }

                    /* 调用通道处理器处理帧数据 */
                    m_channelHandlers[nChannel]->handleFrame(pData, nDataLen, pVideoFrame, m_configManager, nChannel);

                    /* 释放帧数据 */
                    if (pVideoFrame)
                    {
                        freeFrame(pVideoFrame);
                    }
                }
            }
            /*释放码流缓存*/
            pHandle->mppVenc_release_stream(pHandle, &stFrame);

            /* 主动放弃CPU */
            usleep(1000);
        }
    }
    catch(const std::exception& e)
    {
        dlog_error("%s",e.what());
    }
}

void CStreamVideo::get_vpssStream()
{
    pthread_setname_np(pthread_self(), "get_vpssStream");
    try
    {
        int nRet = 0;
        ot_video_frame_info stFrameInfo;
        int nChannel = VPSS_CHANNEL_AI;
        HiVpss_S *pHandle = m_pVpssHandle[VPSS_MAIN_SUB];
        ot_aidetect_result_array stResult;
        memset(&stResult, 0, sizeof(ot_aidetect_result_array));
        /* 帧计数器 */
        int nFrameCount = 0;
        /* 每几帧处理一次 */
        const int process_every_n_frames = 10;
        // std::ofstream fd;
        // fd.open("/opt/course/record/1.yuv", std::ios::out | std::ios::trunc);
        while (true == m_bVpssFlag[nChannel].load(std::memory_order_acquire))
        {
            memset(&stFrameInfo, 0, sizeof(ot_video_frame_info));
            /*获取编码码流*/
            nRet = pHandle->mppVpss_get_chnFrame(pHandle, VPSS_CHANNEL_AI, &stFrameInfo, -1);
            if (nRet != OK)
            {
                dlog_info("vpss %d error %x", nChannel, nRet);
                continue;
            }

            std::lock_guard<std::mutex> lock(m_mutexSendData);
            // dlog_info("vpss %d ret %x", nChannel, nRet);

            /* 控制帧数 */
            if (nFrameCount++ % process_every_n_frames == 0)
            {
                // dlog_info("发送数据到 AI_APP");
                /* memory存储映射接口，映射物理内存地址至虚拟内存地址 */
                int nLen = stFrameInfo.video_frame.width * stFrameInfo.video_frame.height * 3 / 2;
                stFrameInfo.video_frame.virt_addr[0] = ss_mpi_sys_mmap(stFrameInfo.video_frame.phys_addr[0], nLen);

                /* 发送数据到 AI_APP */
                algo_send_videoStreamData(stFrameInfo.video_frame.virt_addr[0],
                                          nLen,
                                          stFrameInfo.video_frame.width,
                                          stFrameInfo.video_frame.height);

                // dlog_debug("%dx%d stride: %d %d", stFrameInfo.video_frame.width, stFrameInfo.video_frame.height,stFrameInfo.video_frame.stride[0],stFrameInfo.video_frame.stride[1]);
                // fd.write(static_cast<char *>(stFrameInfo.video_frame.virt_addr[0]), nLen);

                // note memory存储解映射调用位置，移动至CStreamHandler::recvDataProcess的智能指针内
                /* memory存储解映射 */
                ss_mpi_sys_munmap(stFrameInfo.video_frame.virt_addr[0], stFrameInfo.video_frame.width * stFrameInfo.video_frame.height * 1.5);
            }
            /*释放码流缓存*/
            pHandle->mppVpss_release_chnFrame(pHandle, VPSS_CHANNEL_AI, &stFrameInfo);

            /* 主动放弃CPU */
            usleep(1000);
        }
        // fd.close();
    }
    catch(const std::exception& e)
    {
        dlog_error("%s",e.what());
    }
}
