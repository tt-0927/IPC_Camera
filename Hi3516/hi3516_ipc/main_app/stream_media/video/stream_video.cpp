/**
 * @FilePath     : stream_video.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-03-21 10:24:45
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-20 15:57:03
 * @Description  : 流媒体视频模块
 */

#include "stream_video.h"
#include "convert_interface.h"
#include "RtpServer.h"
#include "get_time.h"
#include "isp_runtime_bootstrap.h"
#include "isp_control.h"
#include "stream_server.h"
#include "action_code.h"
#include "capture_ctrl.h"
#include "record_ctrl.h"
#include "system_utils.h"
#if CAP_RTMP_PUSH
#include "push_stream.h"
#endif

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <memory>
#include <new>

namespace
{
    /**
     * @brief   : VENC 码流异常路径释放保护
     * @note    : 成功取流后必须在 release_stream 前保持码流视图有效；下游分发发生异常时，
     *            由该对象兜底归还 VENC 环形码流资源，避免异常路径长期占用编码器缓存。
     */
    class VencStreamReleaseGuard
    {
    public:
        /**
         * @brief   : 构造码流释放保护对象
         * @param   {HiVenc_S*} pHandle：VENC 句柄
         * @param   {ot_venc_stream*} pStream：已成功获取的码流对象
         */
        VencStreamReleaseGuard(HiVenc_S *pHandle, ot_venc_stream *pStream)
            : m_pHandle(pHandle), m_pStream(pStream)
        {
        }

        /**
         * @brief   : 兜底释放 VENC 码流
         */
        ~VencStreamReleaseGuard() noexcept
        {
            if (!m_pHandle || !m_pStream)
            {
                return;
            }

            const int nRet = m_pHandle->mppVenc_release_stream(m_pHandle, m_pStream);
            if (nRet != OK)
            {
                dlog_error("VENC异常路径释放码流失败 ret:%d", nRet);
            }
        }

        /**
         * @brief   : 在正常路径主动释放 VENC 码流
         * @return   {int} MPP 释放结果
         */
        int release() noexcept
        {
            if (!m_pHandle || !m_pStream)
            {
                return OK;
            }

            const int nRet = m_pHandle->mppVenc_release_stream(m_pHandle, m_pStream);
            m_pHandle = nullptr;
            m_pStream = nullptr;
            return nRet;
        }

    private:
        /* memory: 释放保护只保存句柄和栈上码流结构体地址，不持有或复制码流数据。 */
        HiVenc_S *m_pHandle;
        ot_venc_stream *m_pStream;
    };
}

CStreamVideo* CStreamVideo::m_self = NULL;
std::mutex CStreamVideo::m_mutex;

CStreamVideo::CStreamVideo()
    : m_pVpssHandle(nullptr)
{
    for (int i = 0; i < VENC_CHN_MAX; i++)
    {
        m_bVencFlag[i].store(false, std::memory_order_release);
        m_astStreamGeometry[i] = Video_NS::StreamGeometry_S();
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
    m_channelHandlers[VENC_CHN_SUB] = std::make_unique<CSubChannelHandler>(this);
    m_channelHandlers[VENC_CHN_JPEG] = std::make_unique<CJpegChannelHandler>();

    /*初始化NAL解析器策略*/
    m_nalParsers[Video_NS::VideoCodec_E::H264] = std::make_unique<CH264NalParser>();
    m_nalParsers[Video_NS::VideoCodec_E::H265] = std::make_unique<CH265NalParser>();
    m_nalParsers[Video_NS::VideoCodec_E::SVAC3] = std::make_unique<CSvac3NalParser>();
    m_nalParsers[Video_NS::VideoCodec_E::MJPEG] = std::make_unique<CMjpegParser>();

    /*初始化与各模块的回调绑定*/
    initCallbackBinding();

    /* 从配置管理对象获取配置 */
    const auto &videoConfigs = m_configManager.getVideoConfigs();

    /*初始化海思MPI系统/公共视频缓存池*/
    nRet = streamSys_init(videoConfigs);
    if (nRet != OK)
    {
        dlog_error("MPI系统/视频缓存池初始化失败");
        return ERR;
    }
    /*初始化VI*/
    m_viHandle.reset(streamVi_init());
    if (!m_viHandle)
    {
        dlog_error("Vi模块初始化失败");
        return ERR;
    }

    /* isp 图像初始化 */
    nRet = CIspControl::instance()->init();
    if (nRet != OK)
    {
        dlog_error("ISP图像初始化失败: %d", nRet);
        return ERR;
    }

    /* 注册Hi3516 ISP业务服务，供共享ISP薄门面按抽象接口转发。 */
    nRet = CHi3516IspRuntimeBootstrap::instance()->init();
    if (nRet != OK)
    {
        dlog_error("Hi3516 ISP业务服务注册失败: %d", nRet);
        int nDeinitRet = CIspControl::instance()->deinit();
        if (nDeinitRet != OK)
        {
            dlog_warn("Hi3516 ISP业务服务注册失败后释放ISP图像模块失败: %d", nDeinitRet);
        }
        return ERR;
    }

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
    const auto &roiConfigs = m_configManager.getVideoRoiConfigs();
    for (int i = 0; i < VENC_CHN_MAX; i++)
    {
        if (i != VENC_CHN_JPEG)
        {
            m_vencHandles[i].reset(streamVenc_init(videoConfigs[i], roiConfigs[i]));
        }
        else
        {
            m_vencHandles[i].reset(streamVenc_init(stVideoConfigJpeg));
        }
        if (!m_vencHandles[i])
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

    /* 初始化未裁剪时的有效画面几何，OSD 初始化后可直接读取。 */
    update_stream_geometry(VENC_CHN_MAIN, videoConfigs[VENC_CHN_MAIN], nullptr);
    update_stream_geometry(VENC_CHN_SUB, videoConfigs[VENC_CHN_SUB], nullptr);
    update_stream_geometry(VENC_CHN_JPEG, stVideoConfigJpeg, nullptr);

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
    }
    else
    {
        dlog_info("解绑模块成功");
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
        /* RAII自动释放资源 */
        m_vencHandles[i].reset();
    }
    /*去初始化VPSS*/
    streamVpss_uninit();

    /* 清理Hi3516 ISP业务服务，必须早于CIspControl释放。 */
    nRet = CHi3516IspRuntimeBootstrap::instance()->deinit();
    if (nRet != OK)
    {
        dlog_error("Hi3516 ISP业务服务清理失败: %d", nRet);
    }

    /* isp 图像去初始化 */
    nRet = CIspControl::instance()->deinit();
    if (nRet != OK)
    {
        dlog_error("ISP图像去初始化失败: %d", nRet);
    }

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
    if (nChn < 0 || nChn >= VENC_CHN_JPEG)
    {
        dlog_error("重新启动视频流编码通道失败，通道:%d非法", nChn);
        return ERR_PARAM;
    }

    int nRet = OK;
    Video_NS::VideoConfig_S stEffectiveVideoConfig = stVideoConfig;

    /* 停止获取编码通道数据 */
    // m_bVencFlag[nChn].store(false, std::memory_order_release);
    // if(m_getVencThread[nChn].joinable())
    // {
    //     m_getVencThread[nChn].join();
    // }
    /* 解绑定VPSS Chn -> VENC */
    vpssUnbindVencModule(nChn);
    COsdManage::instance()->before_venc_channel_reset(nChn);

    if (m_bInitFlag)
    {
        const auto &roiConfig = m_configManager.getVideoRoiConfigs().at(nChn);
        if (!bUseIncomingAttr)
        {
            stEffectiveVideoConfig = m_configManager.getVideoConfigs().at(nChn);
        }

        const auto &areaCropConfig = m_configManager.getAreaCropConfigRef(nChn);
        if (areaCropConfig.bEnable)
        {
            stEffectiveVideoConfig.stVideoResolution = areaCropConfig.stResolution;
        }

        nRet = streamVenc_reset(m_vencHandles[nChn].get_ref(), stEffectiveVideoConfig, roiConfig);
        if (nRet != OK)
        {
            dlog_error("重新启动视频流模块失败");
            return ERR;
        }
    }

    /* 绑定VPSS Chn -> VENC */
    vpssBindVencModule(nChn);
    ot_vpss_crop_info stAppliedCrop;
    const ot_vpss_crop_info *pstAppliedCrop = nullptr;
    const auto &areaCropConfig = m_configManager.getAreaCropConfigRef(nChn);
    if (areaCropConfig.bEnable &&
        OK == streamVpss_get_chnCrop(m_pVpssHandle[VPSS_MAIN_SUB], nChn, stAppliedCrop) &&
        stAppliedCrop.enable)
    {
        pstAppliedCrop = &stAppliedCrop;
    }
    update_stream_geometry(nChn, stEffectiveVideoConfig, pstAppliedCrop);
    COsdManage::instance()->after_venc_channel_reset(nChn);

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
    if (m_vencHandles[nChannel])
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

int CStreamVideo::get_stream_geometry(int nChn, Video_NS::StreamGeometry_S &stGeometry) const
{
    if (nChn < 0 || nChn >= VENC_CHN_MAX)
    {
        return ERR_PARAM;
    }

    std::lock_guard<std::mutex> lock(m_mutexGeometry);
    stGeometry = m_astStreamGeometry.at(nChn);
    return OK;
}

void CStreamVideo::update_stream_geometry(int nChn,
                                           const Video_NS::VideoConfig_S &stEffectiveVideoConfig,
                                           const ot_vpss_crop_info *pstAppliedCrop)
{
    if (nChn < 0 || nChn >= VENC_CHN_MAX)
    {
        dlog_error("更新码流有效几何失败，通道:%d非法", nChn);
        return;
    }

    Video_NS::StreamGeometry_S stGeometry;
    stGeometry.nSourceWidth = stEffectiveVideoConfig.stVideoResolution.nWidth;
    stGeometry.nSourceHeight = stEffectiveVideoConfig.stVideoResolution.nHeight;

    if (m_pVpssHandle && m_pVpssHandle[VPSS_MAIN_SUB] &&
        nChn < m_pVpssHandle[VPSS_MAIN_SUB]->nVpssChnSum)
    {
        stGeometry.nSourceWidth = m_pVpssHandle[VPSS_MAIN_SUB]->astVpssChnAttr[nChn].nWidth;
        stGeometry.nSourceHeight = m_pVpssHandle[VPSS_MAIN_SUB]->astVpssChnAttr[nChn].nHeight;
    }

    stGeometry.nOutputWidth = stEffectiveVideoConfig.stVideoResolution.nWidth;
    stGeometry.nOutputHeight = stEffectiveVideoConfig.stVideoResolution.nHeight;
    if (m_vencHandles[nChn])
    {
        stGeometry.nOutputWidth = static_cast<int>(m_vencHandles[nChn]->stNeedParam.unWidth);
        stGeometry.nOutputHeight = static_cast<int>(m_vencHandles[nChn]->stNeedParam.unHeight);
    }

    if (pstAppliedCrop && pstAppliedCrop->enable)
    {
        stGeometry.bCropEnable = true;
        stGeometry.nCropX = pstAppliedCrop->crop_rect.x;
        stGeometry.nCropY = pstAppliedCrop->crop_rect.y;
        stGeometry.nCropWidth = static_cast<int>(pstAppliedCrop->crop_rect.width);
        stGeometry.nCropHeight = static_cast<int>(pstAppliedCrop->crop_rect.height);
    }
    else
    {
        stGeometry.nCropWidth = stGeometry.nSourceWidth;
        stGeometry.nCropHeight = stGeometry.nSourceHeight;
    }

    std::lock_guard<std::mutex> lock(m_mutexGeometry);
    stGeometry.unGeneration = m_astStreamGeometry.at(nChn).unGeneration + 1;
    m_astStreamGeometry.at(nChn) = stGeometry;
    dlog_info("通道:%d 有效画面几何更新，源:%dx%d 裁剪:%d[%d,%d,%d,%d] 输出:%dx%d 版本:%llu",
              nChn,
              stGeometry.nSourceWidth,
              stGeometry.nSourceHeight,
              stGeometry.bCropEnable,
              stGeometry.nCropX,
              stGeometry.nCropY,
              stGeometry.nCropWidth,
              stGeometry.nCropHeight,
              stGeometry.nOutputWidth,
              stGeometry.nOutputHeight,
              static_cast<unsigned long long>(stGeometry.unGeneration));
}

int CStreamVideo::setVideoConfig(const Video_NS::VideoConfig_S &stVideoConfig)
{
    bool bIsSetVpss = false;
    bool bIsResetRtsp = false;
    int nId = stVideoConfig.nId;
    int nRet = OK;

    /* 获取当前配置用于比较 */
    const auto currentConfig = m_configManager.getVideoConfigs().at(nId);

    // note: 视频分辨率变更时，需变更 VPSS 通道属性
    if (currentConfig.stVideoResolution.nWidth != stVideoConfig.stVideoResolution.nWidth ||
        currentConfig.stVideoResolution.nHeight != stVideoConfig.stVideoResolution.nHeight)
    {
        bIsSetVpss = true;
    }

    // note: 视频编码格式、视频类型、帧率变更时，需重启 RtspServer
    if (currentConfig.enVideoCodec != stVideoConfig.enVideoCodec || currentConfig.enVideoType != stVideoConfig.enVideoType ||
        currentConfig.enFrameRate != stVideoConfig.enFrameRate)
    {
        bIsResetRtsp = true;
    }

    std::lock_guard<std::mutex> lock(m_mutexCtrl);
    /* 更新视频配置 */
    m_configManager.updateVideoConfig(stVideoConfig);

    CRtspServer::instance()->setVideoConfig(m_configManager.getVideoConfigs());
    if (bIsResetRtsp)
    {
        /* 重新启动RTSP服务器 */
        nRet = CRtspServer::instance()->reboot();
        if (nRet != OK)
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
    COsdManage::instance()->before_venc_channel_reset(nId);

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
            if (nRet != OK)
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

        /* 先清空视频配置应用接口，再更新区域裁剪配置，避免内部同步配置时重复触发业务应用 */
        CAVConfigure::instance()->clearAVVideoConfigApplier(this);
        CAVConfigure::instance()->set_configure(areaCropConfig);
        CAVConfigure::instance()->setAVVideoConfigApplier(this);
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

    ot_vpss_crop_info stAppliedCrop;
    const ot_vpss_crop_info *pstAppliedCrop = nullptr;
    if (areaCropConfig.bEnable &&
        OK == streamVpss_get_chnCrop(m_pVpssHandle[VPSS_MAIN_SUB], nId, stAppliedCrop) &&
        stAppliedCrop.enable)
    {
        pstAppliedCrop = &stAppliedCrop;
    }
    update_stream_geometry(nId, currentVideoConfig, pstAppliedCrop);

    /*
     * info: VENC 已按新参数重建，但新码流尚未开始投递。此时下发录制配置，
     * 可保证录制端收到首个新 I 帧前已经准备好对应的封装参数。
     */
    nRet = sync_record_video_config(currentVideoConfig);
    if (nRet != OK)
    {
        dlog_warn("通道:%d 视频配置已生效，但暂未同步到录制进程，将在录制进程接入后补发", nId);
    }

    /* 启动获取编码通道数据 */
    m_bVencFlag[nId].store(true, std::memory_order_release);
    m_getVencThread[nId] = std::thread(&CStreamVideo::get_vencStream, this, nId);

    dlog_info("重新启动视频流编码通道:%d 成功", nId);

    /* 主动请求 IDR 帧，加速 RTMP 推流的首个 SPS/PPS 到达 */
    request_idr(nId);

#if CAP_RTMP_PUSH
    /* 视频编码配置变更后，主动重启对应通道的 RTMP 推流，避免被动等待 Broken pipe */
    CPushStream::instance()->restart_rtmp_by_channel(nId);
#endif

    /* VENC 重建后必须按有效输出画面重新挂载并绘制 OSD。 */
    COsdManage::instance()->after_venc_channel_reset(nId);

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

    /* VPSS Crop 和 YUV420 VENC 输出均要求偶数尺寸，禁止底层静默向下对齐造成几何不一致。 */
    if (stAreaCrop.bEnable &&
        (stAreaCrop.stResolution.nWidth <= 0 || stAreaCrop.stResolution.nHeight <= 0 ||
         (stAreaCrop.stResolution.nWidth & 1) || (stAreaCrop.stResolution.nHeight & 1)))
    {
        dlog_error("设置区域裁剪失败，输出分辨率必须为正偶数，当前:%dx%d",
                   stAreaCrop.stResolution.nWidth,
                   stAreaCrop.stResolution.nHeight);
        return ERR_PARAM;
    }

    std::lock_guard<std::mutex> lock(m_mutexCtrl);

    int nRet = OK;
    int nId = stAreaCrop.nId;
    auto &currentCropConfig = m_configManager.getAreaCropConfigRef(nId);
    auto currentVideoConfig = m_configManager.getVideoConfigRef(nId);
    ot_vpss_crop_info stAppliedCrop;
    memset(&stAppliedCrop, 0, sizeof(stAppliedCrop));

    /* 视频分辨率和区域裁剪分辨率相同且区域裁剪启用时，不进行区域裁剪 */
    if (stAreaCrop.stResolution == currentVideoConfig.stVideoResolution && stAreaCrop.bEnable)
    {
        dlog_warn("视频分辨率和区域裁剪分辨率相同，不进行区域裁剪");
        return ERR_PARAM;
    }

    // note: 是否需变更 VPSS 通道、VENC 属性
    if (currentCropConfig.stResolution.nWidth != stAreaCrop.stResolution.nWidth ||
        currentCropConfig.stResolution.nHeight != stAreaCrop.stResolution.nHeight ||
        currentCropConfig.stRect != stAreaCrop.stRect || currentCropConfig.bEnable != stAreaCrop.bEnable || bIsMandateSet)
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
        COsdManage::instance()->before_venc_channel_reset(nId);
        /* 重启视频编码 */
        const auto &roiConfig = m_configManager.getVideoRoiConfigs().at(nId);
        nRet = streamVenc_reset(m_vencHandles[nId].get_ref(), currentVideoConfig, roiConfig);
        if (nRet != OK)
        {
            dlog_error("重启视频编码失败");
            return ERR;
        }

        /* 设置 VPSS 对应通道裁剪 */
        nRet = streamVpss_set_chnCrop(m_pVpssHandle[VPSS_MAIN_SUB], stAreaCrop, &stAppliedCrop);
        if (OK != nRet)
        {
            dlog_error("设置 VPSS 通道裁剪失败");
            return ERR;
        }

        /* 绑定VPSS Chn -> VENC */
        vpssBindVencModule(nId);

        update_stream_geometry(nId,
                               currentVideoConfig,
                               stAppliedCrop.enable ? &stAppliedCrop : nullptr);
        COsdManage::instance()->after_venc_channel_reset(nId);

        /* info: 裁剪会改变 VENC 实际输出分辨率，录制端必须同步新参数后再接收新帧。 */
        nRet = sync_record_video_config(currentVideoConfig);
        if (nRet != OK)
        {
            dlog_warn("通道:%d 裁剪配置已生效，但暂未同步到录制进程，将在录制进程接入后补发", nId);
        }

        /* 启动获取编码通道数据 */
        m_bVencFlag[nId].store(true, std::memory_order_release);
        m_getVencThread[nId] = std::thread(&CStreamVideo::get_vencStream, this, nId);

        /* step: 请求新 I 帧，使录制端尽快在新分片写入完整参数集。 */
        request_idr(nId);

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
    if (nVencChn == VENC_CHN_MAIN)
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
    if (nVencChn == VENC_CHN_MAIN)
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

int CStreamVideo::apply_video_config(const Video_NS::VideoConfig_S &stConfig)
{
    return setVideoConfig(stConfig);
}

int CStreamVideo::apply_video_roi_config(const Video_NS::VideoRoiConfig_S &stConfig)
{
    return setVideoRoiConfig(stConfig);
}

int CStreamVideo::apply_area_crop_config(const Video_NS::AreaCrop_S &stConfig)
{
    return setAreaCropConfig(stConfig);
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

    CStreamServer::instance()->set_record_connected_callback(
        [this]()
        {
            std::lock_guard<std::mutex> lock(m_mutexCtrl);

#if CAP_RECORD_USE_MAIN_STREAM
            constexpr int RECORD_VIDEO_CHANNEL = STREAM_MEDIA_MAIN;
#else
            constexpr int RECORD_VIDEO_CHANNEL = STREAM_MEDIA_SUB;
#endif

            /* 当前全部视频配置，用于获取实际录制源通道 */
            const auto &videoConfigs = m_configManager.getVideoConfigs();
            if (RECORD_VIDEO_CHANNEL < 0 || static_cast<size_t>(RECORD_VIDEO_CHANNEL) >= videoConfigs.size())
            {
                dlog_error("录制进程接入后同步视频配置失败，录制通道:%d不存在", RECORD_VIDEO_CHANNEL);
                return;
            }

            /* 录制源通道当前实际生效的视频配置 */
            Video_NS::VideoConfig_S stRecordVideoConfig = videoConfigs.at(RECORD_VIDEO_CHANNEL);
            const auto &areaCropConfig = m_configManager.getAreaCropConfigRef(RECORD_VIDEO_CHANNEL);
            if (areaCropConfig.bEnable)
            {
                stRecordVideoConfig.stVideoResolution = areaCropConfig.stResolution;
            }

            const int nRet = sync_record_video_config(stRecordVideoConfig);
            if (nRet != OK)
            {
                dlog_warn("录制进程接入后同步视频配置失败，通道:%d，错误码:%d", RECORD_VIDEO_CHANNEL, nRet);
            }
        });

    /* 注册视频配置应用接口，由 CAVConfigure 通过抽象接口触发业务侧配置应用 */
    CAVConfigure::instance()->setAVVideoConfigApplier(this);

    /* 获取jpeg编码通道参数回调 */
    CCaptureCtrl::instance()->get_jpegVencParamCallback(
        [this](unsigned int &unWidth, unsigned int &unHeight, unsigned int &nUqFactor) -> int
        {
            return this->getJpegVencParam(unWidth, unHeight, nUqFactor);
        });
}

int CStreamVideo::sync_record_video_config(const Video_NS::VideoConfig_S &stVideoConfig)
{
#if CAP_RECORD_USE_MAIN_STREAM
    constexpr int RECORD_VIDEO_CHANNEL = STREAM_MEDIA_MAIN;
#else
    constexpr int RECORD_VIDEO_CHANNEL = STREAM_MEDIA_SUB;
#endif

    if (stVideoConfig.nId != RECORD_VIDEO_CHANNEL)
    {
        return OK;
    }

    const int nRet = CStreamServer::instance()->sendVideoConfig(stVideoConfig);
    if (nRet != OK)
    {
        return nRet;
    }

    dlog_info("已同步录制视频配置，通道:%d，分辨率:%dx%d，帧率:%d，编码:%d",
              stVideoConfig.nId,
              stVideoConfig.stVideoResolution.nWidth,
              stVideoConfig.stVideoResolution.nHeight,
              stVideoConfig.getFrameRateAsInt(),
              static_cast<int>(stVideoConfig.enVideoCodec));
    return OK;
}

int CStreamVideo::getJpegVencParam(unsigned int &unWidth, unsigned int &unHeight, unsigned int &nUqFactor)
{
    unWidth = m_vencHandles[VENC_CHN_JPEG]->stNeedParam.unWidth;
    unHeight = m_vencHandles[VENC_CHN_JPEG]->stNeedParam.unHeight;
    nUqFactor = m_vencHandles[VENC_CHN_JPEG]->stExParam.uQFactor;

    return 0;
}

// info /*----------------------- 私有线程函数 -----------------------*/

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
        if (nChannel < 0 || nChannel >= VENC_CHN_MAX)
        {
            dlog_error("VENC取流通道越界，chn:%d", nChannel);
            return;
        }
        HiVenc_S *pHandle = m_vencHandles[nChannel].get();
        if (!pHandle)
        {
            dlog_error("VENC取流句柄为空，chn:%d", nChannel);
            return;
        }

        while (true == m_bVencFlag[nChannel].load(std::memory_order_acquire))
        {
            memset(&stFrame, 0, sizeof(ot_venc_stream));
            /*获取编码码流*/
            const auto stGetStreamStart = std::chrono::steady_clock::now();
            const int nGetStreamTimeoutMs =
                (nChannel == VENC_CHN_JPEG) ? TIMEOUT_1500_MS : TIMEOUT_500_MS;
            nRet = pHandle->mppVenc_get_stream(pHandle, &stFrame, nGetStreamTimeoutMs);
            const long long llGetStreamCostMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                     std::chrono::steady_clock::now() - stGetStreamStart)
                                                     .count();

            /*
             * 区分 JPEG 按需抓拍的空闲超时与真实取流失败：JPEG 无抓图请求时编码器
             * 不产出帧，get_stream 阻塞到超时属预期空闲态；真实异常（get_fd/select
             * 失败）耗时约 20ms，与超时耗时（约等于 nGetStreamTimeoutMs）有明确边界。
             */
            const bool bJpegIdleTimeout =
                (nChannel == VENC_CHN_JPEG && nRet != OK &&
                 llGetStreamCostMs >= static_cast<long long>(nGetStreamTimeoutMs) - 50);

            if (nRet != OK)
            {
                if (bJpegIdleTimeout)
                {
                    /* JPEG 空闲超时为正常态，不视为失败，继续等待下一次取流。 */
                }
                else
                {
                    /* 真实取流失败，记录错误并继续。 */
                }
                // dlog_error("mppVenc_get_stream chn: %d error: %x", nChannel, nRet);
                continue;
            }

            /* memory: 任何下游异常都必须归还本轮VENC码流，避免编码器环形缓存泄漏。 */
            VencStreamReleaseGuard stReleaseGuard(pHandle, &stFrame);

            /*
             * perf: 分发帧数据到通道处理器。
             * 每个通道处理器由各自通道的取流线程独占访问，互不共享状态；
             * 下游推送路径（RTSP队列、RTP列表、UDS发送）均有内部锁保护。
             * 此处不再持有全局互斥锁，避免JPEG通道的抓图文件I/O阻塞
             * 主/子码流分发，导致RTSP预览周期性丢帧。
             */
            for (int i = 0; i < (int) stFrame.pack_cnt; i++)
            {
                if (!stFrame.pack || !stFrame.pack[i].addr || stFrame.pack[i].len < stFrame.pack[i].offset)
                {
                    continue;
                }
                pData = stFrame.pack[i].addr + stFrame.pack[i].offset;
                nDataLen = stFrame.pack[i].len - stFrame.pack[i].offset;
                if (nDataLen < 0)
                {
                    throw std::runtime_error("stFrame nDataLen < 0 !");
                }
                // dlog_info("nChannel:%d stFrame.pack_cnt:%d nDataLen:%d", nChannel, (int)stFrame.pack_cnt, nDataLen);

                /* 使用通道处理器策略处理帧数据 */
                if (m_channelHandlers[nChannel])
                {
                    const auto &vVideoConfig = m_configManager.getVideoConfigs();
                    VencFrameView_S stFrameView;
                    stFrameView.pData = pData;
                    stFrameView.nDataLen = nDataLen;

                    /*
                     * perf: 仅解析 NAL 类型，不再为 VideoFrame_S 分配并复制完整编码帧。
                     * 视图只在 handleFrame 同步调用期间有效，下游队列自行完成必要的一次复制。
                     */
                    if (nChannel == VENC_CHN_JPEG)
                    {
                        stFrameView.enVideoCodec = Video_NS::VideoCodec_E::MJPEG;
                        stFrameView.eType = Video_NS::UNKNOWN_TYPE;
                    }
                    else
                    {
                        /*
                         * perf: 主/子码流先拷贝一次到共享 buffer（引用计数），
                         * RTSP/RTMP/录制三个下游零拷贝共享同一份数据，
                         * 避免每个消费者各拷贝一份（原 3 份 -> 1 份）。
                         * JPEG 通道只有抓图一个消费者，不参与共享，保持零拷贝视图。
                         */
                        if (pData == nullptr)
                        {
                            dlog_error("共享帧构造：chn=%d pData 为空，跳过", nChannel);
                        }
                        else
                        {
                            try
                            {
                                /*
                                 * memory: 使用 new[] + shared_ptr 显式删除器，绕开
                                 * make_shared<uint8_t[]> 在 GCC10/musl 工具链上的
                                 * 数组分配缺陷（实测直接崩溃）；数据由取流线程持有，
                                 * 最后一个下游释放后回收。
                                 */
                                std::shared_ptr<uint8_t[]> pShared(new uint8_t[nDataLen], std::default_delete<uint8_t[]>());
                                std::memcpy(pShared.get(), pData, nDataLen);
                                stFrameView.stSharedFrame.pData = std::move(pShared);
                                stFrameView.stSharedFrame.nLen = nDataLen;
                            }
                            catch (const std::bad_alloc &)
                            {
                                /* 共享拷贝失败时退回原拷贝路径（各下游自行复制），不丢帧。 */
                                dlog_warn("共享帧构造 bad_alloc：chn=%d 退回拷贝路径", nChannel);
                            }
                        }
                        if (nChannel < 0 || static_cast<std::size_t>(nChannel) >= vVideoConfig.size())
                        {
                            dlog_error("VENC帧视图无法获取视频配置，chn:%d config_size:%zu",
                                       nChannel,
                                       vVideoConfig.size());
                            continue;
                        }
                        stFrameView.enVideoCodec = vVideoConfig[nChannel].enVideoCodec;
                        const auto it = m_nalParsers.find(stFrameView.enVideoCodec);
                        if (it == m_nalParsers.end() || !it->second)
                        {
                            dlog_error("未找到对应的NAL解析器，编码类型:%d",
                                       static_cast<int>(stFrameView.enVideoCodec));
                            continue;
                        }
                        stFrameView.eType = it->second->parseNalType(pData, nDataLen);
                    }

                    /* 调用通道处理器；处理器不得保存 stFrameView.pData。 */
                    try
                    {
                        m_channelHandlers[nChannel]->handleFrame(stFrameView, m_configManager, nChannel);
                    }
                    catch (const std::bad_alloc &)
                    {
                        dlog_error("VENC下游分发内存申请失败，丢弃当前pack chn:%d len:%d",
                                   nChannel,
                                   nDataLen);
                    }
                    catch (const std::exception &e)
                    {
                        dlog_error("VENC下游分发异常，丢弃当前pack chn:%d len:%d error:%s",
                                   nChannel,
                                   nDataLen,
                                   e.what());
                    }
                    catch (...)
                    {
                        dlog_error("VENC下游分发发生未知异常，丢弃当前pack chn:%d len:%d",
                                   nChannel,
                                   nDataLen);
                    }
                }
            }
            /*释放码流缓存*/
            stReleaseGuard.release();

            /* 主动放弃CPU */
            usleep(1000);
        }
    }
    catch (const std::exception &e)
    {
        dlog_error("%s", e.what());
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
        // int nFrameCount = 0;
        /* 每几帧处理一次 */
        // const int process_every_n_frames = 10;
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

            /* lock: AI帧分发路径内部均为线程安全队列，无需全局串行化 */
            // dlog_info("vpss %d ret %x", nChannel, nRet);

            /* 控制帧数 */
            // if (nFrameCount++ % process_every_n_frames == 0)
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

                // dlog_debug("%dx%d stride: %d %d", stFrameInfo.video_frame.width,
                // stFrameInfo.video_frame.height,stFrameInfo.video_frame.stride[0],stFrameInfo.video_frame.stride[1]);
                // fd.write(static_cast<char *>(stFrameInfo.video_frame.virt_addr[0]), nLen);

                // note memory存储解映射调用位置，移动至CStreamHandler::recvDataProcess的智能指针内
                /* memory存储解映射 */
                ss_mpi_sys_munmap(stFrameInfo.video_frame.virt_addr[0],
                                  stFrameInfo.video_frame.width * stFrameInfo.video_frame.height * 1.5);
            }
            /*释放码流缓存*/
            pHandle->mppVpss_release_chnFrame(pHandle, VPSS_CHANNEL_AI, &stFrameInfo);

            /* 主动放弃CPU */
            usleep(1000);
        }
        // fd.close();
    }
    catch (const std::exception &e)
    {
        dlog_error("%s", e.what());
    }
}
