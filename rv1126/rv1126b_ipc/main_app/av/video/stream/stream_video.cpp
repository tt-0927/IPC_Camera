/**
 * @FilePath     : stream_video.cpp
 * @Author       : huangjunda
 * @Date         : 2025-03-17 15:37:06
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-12 14:21:19
 * @Description  : RV1126B视频流生命周期管理
 */

#include "stream_video.h"
#include "isp_runtime_bootstrap.h"
#include "RtpServer.h"
#include "push_stream.h"
#include "IpcRet.h"
#include "RtpServer.h"
#include "stream_server.h"
#include "record_ctrl.h"
#include "capture_ctrl.h"
#include "isp_control.h"
#include "isp_manage.h"
#include "algo_detect.h"

#ifdef DEVICE_TV_3882TI
#include "event_configure.h"
#endif

CStreamVideo* CStreamVideo::m_self = NULL;
std::mutex CStreamVideo::m_mutex;

/*NALU的起始码长度偏移*/
static int find_nalu_offset(const uint8_t *p, int len)
{
    for (int i = 0; i + 4 < len; ++i)
    {
        if (p[i] == 0 && p[i+1] == 0)
        {
            if (p[i+2] == 1)            // 00 00 01
                return i + 3;
            if (p[i+2] == 0 && p[i+3] == 1)  // 00 00 00 01
                return i + 4;
        }
    }
    return -1;
}

/* 送帧数据到抓图模块 */
static int sendFrameData(unsigned char *pData, int nDataLen)
{
    if(!pData || nDataLen <= 0)
    {
        return -1;
    }

    unsigned char *FrameData = NULL; // 保存一帧数据
   
    FrameData = (unsigned char *)malloc(nDataLen);
    if (!FrameData) 
    {
        dlog_error("Memory allocation failed for frame!");
        return -1;
    }

    memcpy(FrameData, pData, nDataLen);

     /* 送编码好的jpeg图片数据至抓图模块 */
    CCaptureCtrl::instance()->send_frameData(FrameData, nDataLen);

    // 释放帧内存
    if(FrameData)
    {
        free(FrameData);
        FrameData = NULL;
    }
   
    return OK;
}

CStreamVideo::CStreamVideo() :
    m_strConfigPath(VIDEO_CONFIG_FILE),
    m_strRoiConfigPath(ROI_CONFIG_FILE)
{
    for (int i = 0; i < VENC_CHN_MAX; i++)
    {
        m_bVencFlag[i].store(false, std::memory_order_release);
    }
    for (int i = 0; i < VPSS_CHANNEL_SUM; i++)
    {
        m_bVpssFlag[i].store(false, std::memory_order_release);
    }

    m_bInitFlag = false;
}

CStreamVideo::~CStreamVideo()
{
}

int CStreamVideo::init()
{
    int nRet = OK;

    /*初始化与各模块的回调绑定*/
    initCallbackBinding();

    /* 图像初始化 note:要在流初始化前加载，不然isp会失效 */
    nRet = CIspControl::instance()->init();
    if(nRet != OK)
    {
        dlog_error("图像模块初始化失败：%d", nRet);
        return ERR;
    }

    /* step: RK AIQ就绪后注册共享ISP服务，后续所有图像配置均由CIspManage统一事务下发。 */
    nRet = CRv1126bIspRuntimeBootstrap::instance()->init();
    if (nRet != OK)
    {
        dlog_error("RV1126B共享ISP服务注册失败：%d", nRet);
        /* memory: 只有共享service已完整注销后才能释放其借用的RK AIQ上下文。 */
        const int nBootstrapDeinitRet = CRv1126bIspRuntimeBootstrap::instance()->deinit();
        if (nBootstrapDeinitRet != OK)
        {
            dlog_error("RV1126B共享ISP服务注册失败后仍有资源未释放：%d", nBootstrapDeinitRet);
            return ERR;
        }
        const int nIspDeinitRet = CIspControl::instance()->deinit();
        if (nIspDeinitRet != OK)
        {
            dlog_warn("RV1126B共享ISP服务注册失败后释放RK AIQ失败：%d", nIspDeinitRet);
        }
        return ERR;
    }

    /*初始化stream 编码*/
    nRet = initStream();
    if(nRet != OK)
    {
        dlog_error("视频模块初始化失败：%d", nRet);

        /* step: 共享ISP服务持有RK AIQ借用引用；流初始化失败时必须按正常反序回滚，禁止遗留回调。 */
        const int nBootstrapRet = CRv1126bIspRuntimeBootstrap::instance()->deinit();
        if (nBootstrapRet != OK)
        {
            /* ! service仍可能借用AIQ，不能继续释放上下文；后续需重试bootstrap deinit。 */
            dlog_error("视频模块初始化失败后仍有共享ISP资源未释放：%d", nBootstrapRet);
            return ERR;
        }

        const int nIspDeinitRet = CIspControl::instance()->deinit();
        if (nIspDeinitRet != OK)
        {
            dlog_warn("视频模块初始化失败后释放RK AIQ失败：%d", nIspDeinitRet);
        }
        return ERR;
    }

    /*
     * step: VI和传感器stream-on可能重新下发传感器默认寄存器，覆盖共享ISP启动阶段的镜像设置。
     * 通过CIspManage转发给同一个共享reconciler，等待全部已保存配置完成后再结束初始化；
     * ! 禁止在这里直接调用CIspControl或RKISP镜像ioctl，避免绕过共享参数重放顺序和串行执行器。
     */
    nRet = CIspManage::instance()->reconcile_all();
    if (nRet != OK)
    {
        dlog_error("VI初始化完成后强制重放ISP配置失败：%d", nRet);

        /*
         * ! stream_main在init()失败后会统一调用deinit()。此处不能提前释放VI和RK AIQ，
         * 否则调用方的失败清理会再次进入deinitStream()，并可能重复操作Rockit句柄。
         * reboot()失败时则保留已完成的流和共享服务，调用方可再次执行完整deinit/reboot。
         */
        return ERR;
    }

    dlog_info("VI初始化完成后共享ISP配置已同步重放");

    return OK;
}

int CStreamVideo::deinit()
{
    int nRet = OK;
    int nResult = OK;
    /*去初始化视频流*/
    nRet = deinitStream();
    if(nRet != OK)
    {
        dlog_error("视频模块去初始化失败：%d", nRet);
        nResult = ERR;
    }

    /* 图像去初始化 */
    /* memory: 共享服务必须先停止，避免SmartIR回调或补光worker继续借用即将销毁的RK AIQ上下文。 */
    nRet = CRv1126bIspRuntimeBootstrap::instance()->deinit();
    if(nRet != OK)
    {
        dlog_error("RV1126B共享ISP服务去初始化失败：%d", nRet);
        /* ! CIspManage仍可能持有共享对象时，不能释放被SmartIR借用的RK AIQ。 */
        return ERR;
    }

    nRet = CIspControl::instance()->deinit();
    if(nRet != OK)
    {
        dlog_error("图像模块去初始化失败：%d", nRet);
        nResult = ERR;
    }
    
    return nResult;
}

int CStreamVideo::initStream()
{ 
    int nRet = OK;
   
/*大模型开启推理，系统负载高，限制高分辨率和高画质，防止花屏*/
#ifdef DEVICE_TV_3882TI

    /* 从配置管理对象获取配置 */
    auto& videoConfigs = m_configManager.getVideoConfigs();

    Alarm::LLMAISceneAnalysis_S stAISceneAnalysisCfg;
    if (CEventConfigure::instance()->get_configure(stAISceneAnalysisCfg) == 0)
    {
        if(stAISceneAnalysisCfg.bEnable)
        {
            if(((videoConfigs.at(VENC_CHN_MAIN).stVideoResolution.nWidth >= PIXEL_WIDTH_2_5K) && (videoConfigs.at(VENC_CHN_MAIN).stVideoResolution.nHeight >= PIXEL_HEIGHT_2_5K))
                || videoConfigs.at(VENC_CHN_MAIN).enImageQuality >= Video_NS::ImageQuality_E::MEDIUM)
            {
                videoConfigs.at(VENC_CHN_MAIN).stVideoResolution.nWidth  = PIXEL_WIDTH_2_5K;
                videoConfigs.at(VENC_CHN_MAIN).stVideoResolution.nHeight = PIXEL_HEIGHT_2_5K;
                videoConfigs.at(VENC_CHN_MAIN).enImageQuality = Video_NS::ImageQuality_E::MEDIUM;
                if(videoConfigs.at(VENC_CHN_MAIN).nBitrateUpperLimit > 4096)
                {
                    videoConfigs.at(VENC_CHN_MAIN).nBitrateUpperLimit = 4096;
                }

                std::lock_guard<std::mutex> lock(m_mutexCtrl);
                /* 更新视频配置 */
                m_configManager.updateVideoConfig(videoConfigs.at(VENC_CHN_MAIN));
                CAVConfigure::instance()->update_configure(videoConfigs.at(VENC_CHN_MAIN));
            }
        }
    }

#else
   
    /* 从配置管理对象获取配置 */
    const auto& videoConfigs = m_configManager.getVideoConfigs();

#endif

    /* 初始化RK MPI系统 */
    nRet = RK_MPI_SYS_Init();
    if (nRet != RK_SUCCESS)
    {
        dlog(LOG_ERROR, "RK MPI系统初始化失败 错误码为:%x",nRet);
        return ERR;
    }

    m_pViHandle = streamVi_init();
    if(m_pViHandle == nullptr)
    {
        dlog_error("Vi模块初始化失败");
        return ERR;
    }

    /* vpss视频处理子系统模块初始化创建 */
    streamVpss_init(&m_pVpssHandle, videoConfigs);

    /*初始化抓图 JPEG 的编码配置*/
    Video_NS::VideoConfig_S stVideoConfigJpeg = videoConfigs[VENC_CHN_SUB];
    stVideoConfigJpeg.nId = VENC_CHN_JPEG;
    stVideoConfigJpeg.enVideoCodec = Video_NS::VideoCodec_E::JPEG;
    stVideoConfigJpeg.stVideoResolution.nWidth = m_pVpssHandle[VPSS_MAIN_SUB]->astVpssChnAttr[VPSS_CHANNEL_AI].nWidth;
    stVideoConfigJpeg.stVideoResolution.nHeight = m_pVpssHandle[VPSS_MAIN_SUB]->astVpssChnAttr[VPSS_CHANNEL_AI].nHeight;
    /* 一帧JPEG即一张完整图片，帧率设置为1 */
    stVideoConfigJpeg.enFrameRate = Video_NS::FrameRate_E::FRAME_RATE_1;
    m_configManager.updateVideoConfig(stVideoConfigJpeg);

    /*初始化VENC*/
    const auto& roiConfigs = m_configManager.getVideoRoiConfigs();
    for (int i = 0; i < VENC_CHN_MAX; i++)
    {
        if(i != VENC_CHN_JPEG)
        {
            m_pVencHandle[i] = streamVenc_init(videoConfigs[i], roiConfigs[i]);
        }
        else
        {
            m_pVencHandle[i] = streamVenc_init(stVideoConfigJpeg);
        }

        if (m_pVencHandle[i] == nullptr)
        {
            dlog_error("Venc模块初始化失败 通道:%d", i);
            return ERR;
        }
        m_bVencFlag[i].store(true, std::memory_order_release);
        m_getVencThread[i] = std::thread(&CStreamVideo::get_vencStream, this, i);
    }

    /* 初始化 AI_APP */
    algo_detect_init();
    /*启动AI送帧线程*/
    m_bVpssFlag[VPSS_CHANNEL_AI].store(true, std::memory_order_release);
    m_getVpssThread[VPSS_CHANNEL_AI] = std::thread(&CStreamVideo::get_vpssStream, this);

    /* 开启OSD管理模块 */
    COsdManage::instance()->init();

    /*绑定模块*/
    nRet = bindModule();
    if (nRet != OK)
    {
        dlog_error("绑定模块失败");
        return ERR;
    }
    else
    {
        dlog_info("绑定模块成功");
    }

    m_bInitFlag = true;

    /* 区域裁剪 */
    const auto &areaCropConfigs = m_configManager.getAreaCropConfigs();
    for (const auto &stAreaCrop : areaCropConfigs)
    {
        /* 启用才进行设置 */
        if (stAreaCrop.bEnable)
        {
            setAreaCropConfig(stAreaCrop, true);
        }
    }

    dlog_info("流媒体视频初始化成功");
    return OK;
}

int CStreamVideo::deinitStream()
{
    int nRet = OK;
    m_bInitFlag = false;

    /*解绑模块*/
    nRet = unbindModule();
    if(nRet != 0)
    {
        dlog(LOG_ERROR,"解绑rockit各模块失败");
        return ERR;
    }

    /*停止AI送帧线程*/
    m_bVpssFlag[VPSS_CHANNEL_AI].store(false, std::memory_order_release);
    if (m_getVpssThread[VPSS_CHANNEL_AI].joinable())
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
        if (m_getVencThread[i].joinable())
        {
            m_getVencThread[i].join();
        }
        nRet = streamVenc_uninit(m_pVencHandle[i]);
        if (nRet != OK)
        {
            dlog_error("Venc模块去初始化失败");
            return ERR;
        }
    }
    /*去初始化VPSS*/
    streamVpss_uninit();

    /*去初始化VI*/
    streamVi_uninit(m_pViHandle);

    /* 反初始化RK MPI系统*/
    RK_MPI_SYS_Exit();

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
    //std::lock_guard<std::mutex> lock(m_mutexCtrl);

    /* 停止获取编码通道数据 */
    m_bVencFlag[nChn].store(false, std::memory_order_release);
    if(m_getVencThread[nChn].joinable())
    {
        m_getVencThread[nChn].join();
    }
    /* 解绑定VPSS Chn -> VENC */
    vpssUnbindVencModule(nChn);

    if (m_bInitFlag)
    {
        const auto& roiConfig = m_configManager.getVideoRoiConfigs().at(nChn);
        if(bUseIncomingAttr)
        {
            nRet = streamVenc_reset(m_pVencHandle[nChn], stVideoConfig, roiConfig);
        }
        else
        {
            const auto &videoConfig = m_configManager.getVideoConfigs().at(nChn);
            nRet = streamVenc_reset(m_pVencHandle[nChn], videoConfig, roiConfig);
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
    m_bVencFlag[nChn].store(true, std::memory_order_release);
    m_getVencThread[nChn] = std::thread(&CStreamVideo::get_vencStream, this, nChn);

    dlog_info("重新启动视频流编码通道:%d 成功", nChn);
    return OK;
}

void CStreamVideo::request_idr(int nChannel)
{
    int nRet = OK;
    dlog_trace("通道%d 请求I帧", nChannel);
    if(m_pVencHandle[nChannel])
    {
        // note 当前调用接口请求出的是I SLICE
        nRet = m_pVencHandle[nChannel]->rockitVenc_request_idr(m_pVencHandle[nChannel]);
        if (nRet != OK)
        {
            dlog_error("请求I帧失败:%x", nRet);
        }
    }
}

int CStreamVideo::getVideoConfig(std::vector<Video_NS::VideoConfig_S> &vstVideoConfig)
{
    vstVideoConfig = m_configManager.getVideoConfigs();
    return OK;
}

int CStreamVideo::setVideoConfig(Video_NS::VideoConfig_S &stVideoConfig)
{
    bool bIsSetVpss = false;
    bool bIsResetRtsp = false;
    bool bIsUpdateOsd = false;
    int nId = stVideoConfig.nId;
    int nRet = OK;

    /* 获取当前配置用于比较 */
    const auto &currentConfig = m_configManager.getVideoConfigs().at(nId);

    if(currentConfig == stVideoConfig)
    {
        dlog_info("通道:%d 视频配置不变", nId);
        return OK;
    }

/*大模型开启推理，系统负载高，限制高分辨率和高画质，防止花屏*/
#ifdef DEVICE_TV_3882TI
    Alarm::LLMAISceneAnalysis_S stAISceneAnalysisCfg;
    if (CEventConfigure::instance()->get_configure(stAISceneAnalysisCfg) == 0)
    {
        if(stAISceneAnalysisCfg.bEnable)
        {
            if((stVideoConfig.stVideoResolution.nWidth >= PIXEL_WIDTH_2_5K) && (stVideoConfig.stVideoResolution.nHeight >= PIXEL_HEIGHT_2_5K))
            {
                stVideoConfig.stVideoResolution.nWidth  = PIXEL_WIDTH_2_5K;
                stVideoConfig.stVideoResolution.nHeight = PIXEL_HEIGHT_2_5K;
            }

            if(stVideoConfig.enImageQuality >= Video_NS::ImageQuality_E::MEDIUM)
            {
                stVideoConfig.enImageQuality = Video_NS::ImageQuality_E::MEDIUM;
            }
        }
    }
#endif

    // note: 视频分辨率变更时，需变更 VPSS 通道属性
    if (currentConfig.stVideoResolution.nWidth != stVideoConfig.stVideoResolution.nWidth ||
        currentConfig.stVideoResolution.nHeight != stVideoConfig.stVideoResolution.nHeight)
    {
        bIsSetVpss = true;
        bIsUpdateOsd = true;
        // note: 视频分辨率变更时，需重启 RtspServer
        bIsResetRtsp = true;
    }

    // note: 视频编码格式、视频类型变更时，需重启 RtspServer
    if (currentConfig.enVideoCodec != stVideoConfig.enVideoCodec
        || currentConfig.enVideoType != stVideoConfig.enVideoType
        || currentConfig.enFrameRate != stVideoConfig.enFrameRate)
    {
        bIsResetRtsp = true;
    }

    std::lock_guard<std::mutex> lock(m_mutexCtrl);

    nRet = COsdManage::instance()->deinit();
    if(nRet != OK)
    {
        dlog_error("osd去初始化失败");
        return nRet;
    }

    /* 更新视频配置 */
    m_configManager.updateVideoConfig(stVideoConfig);
    /* 同步录制进程 */
#if CAP_RECORD_USE_MAIN_STREAM
    if (nId == VENC_CHN_MAIN)
#else
    if (nId == VENC_CHN_SUB)
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
        /* 设置 VPSS 对应通道分辨率 */
        nRet = streamVpss_set_chnAttr(m_pVpssHandle[VPSS_MAIN_SUB], stVideoConfig);
        if (nRet != OK)
        {
            dlog_error("设置 VPSS 对应通道分辨率失败");
            return ERR;
        }

        /* 分辨率更新，更新区域裁剪 */
        /* 转换插件坐标参考系为需要修改的视频分辨率 */
        areaCropConfig.stRect.ConvertResolution(PLUG_IN_WIDTH_DEFAULT,
                                                PLUG_IN_HEIGHT_DEFAULT,
                                                stVideoConfig.stVideoResolution.nWidth,
                                                stVideoConfig.stVideoResolution.nHeight);

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

        /* 设置 VPSS 对应通道分辨率 */
        nRet = streamVpss_set_chnAttr(m_pVpssHandle[VPSS_MAIN_SUB], currentVideoConfig);
        if (nRet != OK)
        {
            dlog_error("设置 VPSS 对应通道分辨率与裁剪分辨率一致失败");
            return ERR;
        }
    }

    /* 重启视频编码 */
    const auto &roiConfig = m_configManager.getVideoRoiConfigs().at(nId);
    nRet = streamVenc_reset(m_pVencHandle[nId], currentVideoConfig, roiConfig);
    if (nRet != OK)
    {
        dlog_error("重启视频编码失败");
        return ERR;
    }

    /* 绑定VPSS Chn -> VENC */
    vpssBindVencModule(nId);


    dlog_info("重新启动视频流编码通道:%d 成功", nId);

    // /* 更新OSD模块 */
    // // if (bIsUpdateOsd)
    // {
    //     COsdManage::instance()->reset_osd_status(nId);
    // }
    
    /* 启动获取编码通道数据 */
    m_bVencFlag[nId].store(true, std::memory_order_release);
    m_getVencThread[nId] = std::thread(&CStreamVideo::get_vencStream, this, nId);

    nRet = COsdManage::instance()->init();
    if(nRet != OK)
    {
        dlog_error("osd初始化失败");
        return nRet;
    }

    dlog_info("设置通道:%d 视频配置成功", nId);
    return OK;

}

int CStreamVideo::setVideoRoiConfig(const Video_NS::VideoRoiConfig_S &stVideoRoiConfig)
{
    std::lock_guard<std::mutex> lock(m_mutexCtrl);
    /*更新视频ROI配置*/
    int nId = stVideoRoiConfig.nId;
    int nRet = streamVenc_set_roi_attr(m_pVencHandle[nId], stVideoRoiConfig);
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

    /* 视频分辨率和区域裁剪分辨率相同，不进行区域裁剪 */
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
        nRet = COsdManage::instance()->deinit();
        if(nRet != OK)
        {
            dlog_error("osd去初始化失败");
            return nRet;
        }

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

        /* 先设置裁剪，再设置属性 */
        nRet = streamVpss_set_chnCrop(m_pVpssHandle[VPSS_MAIN_SUB], stAreaCrop);
        if (OK != nRet)
        {
            dlog_error("设置 VPSS 通道裁剪失败");
            return ERR;
        }

        /* 设置 VPSS 对应通道属性，同步分辨率 */
        nRet = streamVpss_set_chnAttr(m_pVpssHandle[VPSS_MAIN_SUB], currentVideoConfig);
        if (nRet != OK)
        {
            dlog_error("VPSS设置属性失败");
            return ERR;
        }

        /* 重启视频编码 */
        const auto &roiConfig = m_configManager.getVideoRoiConfigs().at(nId);
        nRet = streamVenc_reset(m_pVencHandle[nId], currentVideoConfig, roiConfig);
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

        nRet = COsdManage::instance()->init();
        if(nRet != OK)
        {
            dlog_error("osd初始化失败");
            return nRet;
        }

        dlog_info("重新启动视频流编码通道:%d 成功", nId);
    }

    /*更新区域裁剪配置*/
    m_configManager.updateAreaCropConfig(stAreaCrop);

    dlog_info("设置通道:%d 区域裁剪配置成功", nId);
    return OK;
}

int CStreamVideo::getJpegVencParam(unsigned int &unWidth, unsigned int &unHeight, unsigned int &nUqFactor)
{
    unWidth = m_pVencHandle[VENC_CHN_JPEG]->stNeedParam.unWidth;
    unHeight = m_pVencHandle[VENC_CHN_JPEG]->stNeedParam.unHeight;
    nUqFactor = m_pVencHandle[VENC_CHN_JPEG]->stExParam.u32Qfactor;
    return OK;
}

int CStreamVideo::bindModule()
{
    int nRet = OK;
    nRet |= rockitVi_bind_vpss(m_pViHandle->stNeedParam.nDevId, m_pViHandle->stNeedParam.nChannel, VPSS_MAIN_SUB, 0);
    nRet |= rockitVpss_bind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_MAIN, VENC_CHN_MAIN);
    nRet |= rockitVpss_bind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_SUB, VENC_CHN_SUB);
    nRet |= rockitVpss_bind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_AI, VENC_CHN_JPEG);
    return nRet;
}

int CStreamVideo::unbindModule()
{
    int nRet = OK;
    nRet |= rockitVi_unbind_vpss(m_pViHandle->stNeedParam.nDevId, m_pViHandle->stNeedParam.nChannel, VPSS_MAIN_SUB, 0);
    nRet |= rockitVpss_unbind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_MAIN, VENC_CHN_MAIN);
    nRet |= rockitVpss_unbind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_SUB, VENC_CHN_SUB);
    nRet |= rockitVpss_unbind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_AI, VENC_CHN_JPEG);
    return nRet;
}

int CStreamVideo::vpssBindVencModule(int nVencChn)
{
    int nRet = OK;
    if (nVencChn == VENC_CHN_MAIN)
    {
        nRet |= rockitVpss_bind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_MAIN, VENC_CHN_MAIN);
    }
    else if (nVencChn == VENC_CHN_SUB)
    {
        nRet |= rockitVpss_bind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_SUB, VENC_CHN_SUB);
    }
    else if (nVencChn == VENC_CHN_JPEG)
    {
        nRet |= rockitVpss_bind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_AI, VENC_CHN_JPEG);
    }

    return nRet;
}

int CStreamVideo::vpssUnbindVencModule(int nVencChn)
{
    int nRet = OK;
    if (nVencChn == VENC_CHN_MAIN)
    {
        nRet |= rockitVpss_unbind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_MAIN, VENC_CHN_MAIN);
    }
    else if (nVencChn == VENC_CHN_SUB)
    {
        nRet |= rockitVpss_unbind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_SUB, VENC_CHN_SUB);
    }
    else if (nVencChn == VENC_CHN_JPEG)
    {
        nRet |= rockitVpss_unbind_venc(m_pVpssHandle[VPSS_MAIN_SUB]->nVpssGrp, VPSS_CHANNEL_AI, VENC_CHN_JPEG);
    }
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
            Video_NS::VideoConfig_S configCopy = stVideoConfig;
            return this->setVideoConfig(configCopy);
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

Video_NS::VideoFrame_S * CStreamVideo::createFrame(Video_NS::VideoCodec_E enCodec,uint8_t *pData, int nDataLen)
{
    if (!pData || nDataLen <= 0)
    {
        dlog_error("传入参数不正确");
        return nullptr;
    }

    /*分配连续内存：结构体 + 数据*/
    Video_NS::VideoFrame_S *pVideoFrame = (Video_NS::VideoFrame_S *) malloc(sizeof(Video_NS::VideoFrame_S) + nDataLen);
    if (!pVideoFrame)
    {
        dlog_error("内存分配失败");
        return nullptr;
    }

    memcpy(pVideoFrame->pData, pData, nDataLen);
    pVideoFrame->nLen = nDataLen;
    pVideoFrame->enVideoCodec = enCodec;

    int off = find_nalu_offset(pData, nDataLen);
    if (off < 0)
    {
        return nullptr;
    }

    uint8_t nalType = 0;
    /*根据编码格式解析 NAL 单元类型*/
    switch (pVideoFrame->enVideoCodec)
    {
    case Video_NS::VideoCodec_E::H264:
        // dlog_debug("H264 nalType:%d nDataLen:%d", nalType, nDataLen);
        nalType = pData[off] & 0x1F;
        switch (nalType)
        {
        case 1:
            pVideoFrame->eType = Video_NS::NalType_E::H264_TYPE_SLICE;
            break;
        case 5:
            pVideoFrame->eType = Video_NS::NalType_E::H264_TYPE_IDR;
            break;
        case 6:
            pVideoFrame->eType = Video_NS::NalType_E::H264_TYPE_SEI;
            break;
        case 7:
            pVideoFrame->eType = Video_NS::NalType_E::H264_TYPE_SPS;
            break;
        case 8:
            pVideoFrame->eType = Video_NS::NalType_E::H264_TYPE_PPS;
            break;
        case 9:
            pVideoFrame->eType = Video_NS::NalType_E::H264_TYPE_AUD;
            break;
        case 10:
            pVideoFrame->eType = Video_NS::NalType_E::H264_TYPE_EOSEQ;
            break;
        case 11:
            pVideoFrame->eType = Video_NS::NalType_E::H264_TYPE_EOSTREAM;
            break;
        case 12:
            pVideoFrame->eType = Video_NS::NalType_E::H264_TYPE_FILLER;
            break;
        case 2:
            pVideoFrame->eType = Video_NS::NalType_E::H264_TYPE_DPA;
            break;
        case 3:
            pVideoFrame->eType = Video_NS::NalType_E::H264_TYPE_DPB;
            break;
        case 4:
            pVideoFrame->eType = Video_NS::NalType_E::H264_TYPE_DPC;
            break;
        default:
            pVideoFrame->eType = Video_NS::NalType_E::H264_TYPE_SLICE;
            break;
        }
        break;
    case Video_NS::VideoCodec_E::H265:
        nalType = (pData[off] >> 1) & 0x3F; 
        //dlog_debug("H265 nalType:%d nDataLen:%d", nalType, nDataLen);
        switch (nalType)
        {
        case 0:
            pVideoFrame->eType = Video_NS::NalType_E::H265_TYPE_TRAIL_N;
            break;
        case 1:
            pVideoFrame->eType = Video_NS::NalType_E::H265_TYPE_TRAIL_R;
            break;
        case 8:
            pVideoFrame->eType = Video_NS::NalType_E::H265_TYPE_RASL_N;
            break;
        case 9:
            pVideoFrame->eType = Video_NS::NalType_E::H265_TYPE_RASL_R;
            break;
        case 10:
            pVideoFrame->eType = Video_NS::NalType_E::H265_TYPE_RADL_N;
            break;
        case 11:
            pVideoFrame->eType = Video_NS::NalType_E::H265_TYPE_RADL_R;
            break;
        case 19:
            pVideoFrame->eType = Video_NS::NalType_E::H265_TYPE_IDR_W_RADL;
            break;
        case 20:
            pVideoFrame->eType = Video_NS::NalType_E::H265_TYPE_IDR_N_LP;
            break;
        case 21:
            pVideoFrame->eType = Video_NS::NalType_E::H265_TYPE_CRA;
            break;
        case 32:
            pVideoFrame->eType = Video_NS::NalType_E::H265_TYPE_VPS;
            break;
        case 33:
            pVideoFrame->eType = Video_NS::NalType_E::H265_TYPE_SPS;
            break;
        case 34:
            pVideoFrame->eType = Video_NS::NalType_E::H265_TYPE_PPS;
            break;
        case 35:
            pVideoFrame->eType = Video_NS::NalType_E::H265_TYPE_AUD;
            break;
        case 36:
            pVideoFrame->eType = Video_NS::NalType_E::H265_TYPE_EOS;
            break;
        case 37:
            pVideoFrame->eType = Video_NS::NalType_E::H265_TYPE_EOB;
            break;
        case 38:
            pVideoFrame->eType = Video_NS::NalType_E::H265_TYPE_FILLER;
            break;
        case 39:
            pVideoFrame->eType = Video_NS::NalType_E::H265_TYPE_SEI;
            break;
        case 40:
            pVideoFrame->eType = Video_NS::NalType_E::H265_TYPE_SEI_SUFFIX;
            break;
        default:
            pVideoFrame->eType = Video_NS::NalType_E::H265_TYPE_TRAIL_N;
            break;
        }
        break;
    case Video_NS::VideoCodec_E::SVAC3:
        nalType = pData[5];
        // dlog_debug("SVAC3 nalType:%d nDataLen:%d", nalType, nDataLen);
        switch (nalType)
        {
        case 0:
            pVideoFrame->eType = Video_NS::NalType_E::SVAC3_TYPE_RESERVED_0;
            break;
        case 1:
            pVideoFrame->eType = Video_NS::NalType_E::SVAC3_TYPE_NON_IDR_SLICE;
            break;
        case 2:
            pVideoFrame->eType = Video_NS::NalType_E::SVAC3_TYPE_IDR_SLICE;
            break;
        case 3:
            pVideoFrame->eType = Video_NS::NalType_E::SVAC3_TYPE_NON_IDR_SVC_SLICE;
            break;
        case 4:
            pVideoFrame->eType = Video_NS::NalType_E::SVAC3_TYPE_IDR_SVC_SLICE;
            break;
        case 5:
            pVideoFrame->eType = Video_NS::NalType_E::SVAC3_TYPE_SURVEILLANCE_EXTENSION;
            break;
        case 6:
            pVideoFrame->eType = Video_NS::NalType_E::SVAC3_TYPE_SUPPLEMENTAL_INFO;
            break;
        case 7:
            pVideoFrame->eType = Video_NS::NalType_E::SVAC3_TYPE_SEQUENCE_PARAM_SET;
            break;
        case 8:
            pVideoFrame->eType = Video_NS::NalType_E::SVAC3_TYPE_PICTURE_PARAM_SET;
            break;
        case 9:
            pVideoFrame->eType = Video_NS::NalType_E::SVAC3_TYPE_SECURITY_PARAM_SET;
            break;
        case 10:
            pVideoFrame->eType = Video_NS::NalType_E::SVAC3_TYPE_AUTHENTICATION_DATA;
            break;
        case 11:
            pVideoFrame->eType = Video_NS::NalType_E::SVAC3_TYPE_END_OF_STREAM;
            break;
        case 12:
            pVideoFrame->eType = Video_NS::NalType_E::SVAC3_TYPE_RESERVED_12;
            break;
        case 13:
            pVideoFrame->eType = Video_NS::NalType_E::SVAC3_TYPE_RESERVED_13;
            break;
        case 14:
            pVideoFrame->eType = Video_NS::NalType_E::SVAC3_TYPE_RESERVED_14;
            break;
        case 15:
            pVideoFrame->eType = Video_NS::NalType_E::SVAC3_TYPE_SVC_PIC_PARAM_SET;
            break;
        default:
            pVideoFrame->eType = Video_NS::NalType_E::SVAC3_TYPE_NON_IDR_SLICE;
            break;
        }
        break;
    case Video_NS::VideoCodec_E::MJPEG:
        pVideoFrame->eType = Video_NS::NalType_E::UNKNOWN_TYPE;
        break;
    default:
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

void CStreamVideo::get_vencStream(int param)
{
    pthread_setname_np(pthread_self(), "get_vencStream");

    struct sched_param sched_param;
    /* 使用 Round-Robin 调度*/
    int policy = SCHED_RR; 
    /*优先级范围 1-99，越高越优先*/
    sched_param.sched_priority = 90; 
    pthread_setschedparam(pthread_self(), policy, &sched_param);

    try
    {
        int nRet = 0;
        VENC_STREAM_S stFrame;
        int nChannel = param;
        RkVenc_S *pHandle = m_pVencHandle[nChannel];
        VENC_PACK_S stPacks[MAX_VENC_PACK_COUNT]; 

        auto videoConfig = m_configManager.getVideoConfigs();
        int nTimeOutMs = -1;
        Video_NS::VideoCodec_E enVideoCodec = videoConfig[nChannel].enVideoCodec;
        if (nChannel == VENC_CHN_JPEG)
        {
            nTimeOutMs = 3000;
        }
        else
        {
            nTimeOutMs = 100;
        }

        while (true == m_bVencFlag[nChannel].load(std::memory_order_acquire))
        {
            memset(&stFrame, 0, sizeof(VENC_STREAM_S));
            /*获取编码码流*/
            nRet = pHandle->rockitVenc_get_stream(pHandle, &stFrame, stPacks, MAX_VENC_PACK_COUNT, nTimeOutMs);
            if(nRet != RK_SUCCESS)
            {
                //dlog(LOG_INFO,"venc %d error %x", nChannel, nRet);
                continue;
            }

            /* 计算全帧总长度 (SPS + PPS + IDR)*/
            uint32_t nTotalLen = 0;
            for (uint32_t i = 0; i < stFrame.u32PackCount; i++) 
            {
                nTotalLen += stFrame.pstPack[i].u32Len;
            }

            if(nTotalLen > 0)
            {
                /* 检查是否为IDR帧（H.264/H.265） */
                // if (enVideoCodec == Video_NS::VideoCodec_E::H264)
                // {
                //     dlog_debug("stFrame.pstPack->DataType.enH264EType:%d pPack->u32Len:[%d]", stFrame.pstPack->DataType.enH264EType,pPack->u32Len);
                // }

                /* 获取这一帧起始位置的虚拟地址 */
                uint8_t *pData = pHandle->rockitVenc_get_streamVirdata(&stFrame.pstPack[0]);
                if (pData != NULL)
                {
                    if (VENC_CHN_JPEG == nChannel) /* 抓图 */
                    {
                        sendFrameData(pData, nTotalLen);
                    }
                    else
                    {
                        Video_NS::VideoFrame_S *pVideoFrame = createFrame(enVideoCodec, pData, nTotalLen);
                        if (pVideoFrame) 
                        {
                            /*送编码视频数据*/
                            if (VENC_CHN_MAIN == nChannel) /* 第一码流 */
                            {
                                /* 发送到 RTSP 推流模块 */
                                CPushStream::instance()->sendVideoData(pVideoFrame, true, true);
                                /* 发送到 GB28181 */
                                SIP::CRtpServer::instance()->sendVideoData(pVideoFrame);
#if CAP_RECORD_USE_MAIN_STREAM
                                /* 录制模块 */
                                if (CRecordCtrl::instance()->get_record_status() == Record_NS::Status_E::RECORD_OPERATION)
                                {
                                    CStreamServer::instance()->sendVideoData(pVideoFrame);
                                }
#endif                                
                            }
                            else if (VENC_CHN_SUB == nChannel) /* 第二码流 */
                            {
                                /* 发送到rtsp */
                                CPushStream::instance()->sendVideoData(pVideoFrame, false, true);
#if !CAP_RECORD_USE_MAIN_STREAM
                                /* 录制模块 */
                                if (CRecordCtrl::instance()->get_record_status() == Record_NS::Status_E::RECORD_OPERATION)
                                {
                                    CStreamServer::instance()->sendVideoData(pVideoFrame);
                                }
#endif    
                            }
                            freeFrame(pVideoFrame);
                        }  
                    }
                }
            }
            /*释放码流缓存*/
            pHandle->rockitVenc_release_stream(pHandle, &stFrame);         
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
        StreamVpssFrame_t stVpssFrame;
        int nChannel = VPSS_CHANNEL_AI;
        RkVpss_S *pHandle = m_pVpssHandle[VPSS_MAIN_SUB];
        /* 帧计数器 */
        int nFrameCount = 0; 
        /* 每几帧处理一次 */
        const int process_every_n_frames = 3;

        while (true == m_bVpssFlag[nChannel].load(std::memory_order_acquire))
        {
            memset(&stVpssFrame, 0, sizeof(StreamVpssFrame_t));
            stVpssFrame.channel = VPSS_CHANNEL_AI;
            /*获取编码码流*/
            nRet = streamVpss_get_chnFrame(pHandle, &stVpssFrame);
            if (nRet != OK)
            {
                dlog_info("vpss %d error %x", nChannel, nRet);
                continue;
            }

            std::lock_guard<std::mutex> lock(m_mutexSendData);

            /* 控制帧数 */
            if (nFrameCount++ % process_every_n_frames == 0)
            {
                /* 发送数据到 AI_APP */
                algo_send_streamData(stVpssFrame.framedata, stVpssFrame.framesize, stVpssFrame.pstVideoFrame.stVFrame.u32Height, stVpssFrame.pstVideoFrame.stVFrame.u32Width);
            }

            /*释放码流缓存*/
            streamVpss_release_chnFrame(pHandle, &stVpssFrame);

            /* 主动放弃CPU */
            usleep(1000);
        }
    }
    catch(const std::exception& e)
    {
        dlog_error("%s",e.what());
    }
}
