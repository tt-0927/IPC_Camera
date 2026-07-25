/**
 * @FilePath     : stream_venc.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2024-09-26 13:52:07
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-09 19:36:49
 * @Description  : VENC 视频编码
 */

#include <stdio.h>
#include <math.h>

#include "stream_venc.h"
#include "dlog.h"
#include "IpcRet.h"

/*感兴趣编码区域QP默认值*/
#define ROI_QP_DEFAULT (-8)
/*感兴趣编码区域QP转换因子默认值*/
#define ROI_QP_DEFAULT_FACTOR  (-2)

/**
 * @brief   : 编码器ROI属性填充
 * @return   {int} 0：成功 非零：失败
 */
static int streamVenc_roi_attr_fill(RkVenc_S *pHandle, VENC_ROI_ATTR_S &stRoiAttr, Video_NS::VideoRoi_S &stVideoRoi)
{
    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }

    /* 区域是否正确设置矩形坐标 */
    if (!stVideoRoi.stRect.IsValid())
    {
        /* 未正确设置矩形坐标 */
        stVideoRoi.bEnable = false;
        stVideoRoi.stRect = Common::Rect_S();
    }
    /* 区域为空，不使能该区域 */
    else if (!(stVideoRoi.stRect != Common::Rect_S()))
    {
        stVideoRoi.bEnable = false;
    }

    // note：转换坐标（不在stream_video处转换，与区域裁剪有交叉）
    stVideoRoi.stRect.ConvertResolution(PLUG_IN_WIDTH_DEFAULT,
                                        PLUG_IN_HEIGHT_DEFAULT,
                                        pHandle->stNeedParam.unWidth,
                                        pHandle->stNeedParam.unHeight);

    stRoiAttr.u32Index = stVideoRoi.u32Idx;
    stRoiAttr.bEnable = static_cast<RK_BOOL>(stVideoRoi.bEnable);
    stRoiAttr.bAbsQp = RK_FALSE;
    /* QP 值。
    当bAbsQp模式为相对QP，s32Qp 为 QP 相对于该帧qp值的偏移，s32Qp 范围[-51,51]；
    当bAbsQp模式为绝对QP，s32Qp 为宏块 QP 值，s32Qp 范围[1,51]。 */
    stRoiAttr.s32Qp = ROI_QP_DEFAULT + (stVideoRoi.u32Level * ROI_QP_DEFAULT_FACTOR);
    stRoiAttr.bIntra = RK_FALSE;
    /* 限制矩形在视频范围内 */
    stRoiAttr.stRect.s32X = std::max(0, ALIGN_UP(stVideoRoi.stRect.nX, 16));
    stRoiAttr.stRect.s32Y = std::max(0, ALIGN_UP(stVideoRoi.stRect.nY, 16));
    RK_S32 maxWidth = pHandle->stNeedParam.unWidth - stRoiAttr.stRect.s32X;
    RK_S32 maxHeight = pHandle->stNeedParam.unHeight - stRoiAttr.stRect.s32Y;
    stRoiAttr.stRect.u32Width = std::min(ALIGN_UP(stVideoRoi.stRect.nWidth, 16), maxWidth);
    stRoiAttr.stRect.u32Height = std::min(ALIGN_UP(stVideoRoi.stRect.nHeight, 16), maxHeight);
    return OK;
}

RkVenc_S* streamVenc_init(const Video_NS::VideoConfig_S &stVideoConfig,const Video_NS::VideoRoiConfig_S &stVideoRoiConfig)
{
    int nChannel = stVideoConfig.nId;
    /* 设置编码通道 */
    if(nChannel >= VENC_CHN_MAX)
    {
        dlog_error("最大venc编码通道数为:%d", VENC_CHN_MAX);
        return NULL;
    }

    RkVenc_S *pHandle = (RkVenc_S *) malloc(sizeof(RkVenc_S));
    memset(pHandle, 0, sizeof(RkVenc_S));

    int nRet = OK;

    dlog(LOG_INFO, "create venc");
    
    RkVencNeedParam_S stVencNeedParam;
    memset(&stVencNeedParam, 0, sizeof(RkVencNeedParam_S));

    /* 编码的必须参数 */
    stVencNeedParam.unWidth         = stVideoConfig.stVideoResolution.nWidth;
    stVencNeedParam.unHeight        = stVideoConfig.stVideoResolution.nHeight;
    stVencNeedParam.unVirWidth      = stVideoConfig.stVideoResolution.nWidth;
    stVencNeedParam.unVirHeight     = stVideoConfig.stVideoResolution.nHeight;
    stVencNeedParam.enPixFormat     = RK_FMT_YUV420SP;
    stVencNeedParam.nGop            = stVideoConfig.nIFrameInterval;
    stVencNeedParam.enGopMode       = VENC_GOPMODE_NORMALP;
    stVencNeedParam.nChn            = nChannel;
    stVencNeedParam.nInFrameRate    = 30;
    stVencNeedParam.nOutFrameRate   = stVideoConfig.getFrameRateAsInt();
    stVencNeedParam.enCompressMode  = COMPRESS_MODE_NONE;
    stVencNeedParam.enCodec         = RK_VIDEO_ID_AVC;

    using namespace Video_NS;

    /*编码格式*/
    if( VideoCodec_E::H264 == stVideoConfig.enVideoCodec)
    {
        stVencNeedParam.enCodec = RK_VIDEO_ID_AVC;
        stVencNeedParam.enCompressMode = COMPRESS_AFBC_16x16;
    }
    else if( VideoCodec_E::H265 == stVideoConfig.enVideoCodec)
    {
        stVencNeedParam.enCodec = RK_VIDEO_ID_HEVC;
        stVencNeedParam.enCompressMode = COMPRESS_AFBC_16x16;
    }
    else if( VideoCodec_E::MJPEG== stVideoConfig.enVideoCodec)
    {
        stVencNeedParam.enCodec = RK_VIDEO_ID_MJPEG;
        stVencNeedParam.enCompressMode = COMPRESS_MODE_NONE;
    }
    else if( VideoCodec_E::JPEG== stVideoConfig.enVideoCodec)
    {
        stVencNeedParam.enCodec = RK_VIDEO_ID_JPEG;
        stVencNeedParam.enCompressMode = COMPRESS_MODE_NONE;
    }

    /*智能编码*/
    if(stVideoConfig.bSmartEnable == true)
    {
        stVencNeedParam.enGopMode = VENC_GOPMODE_SMARTP;
    }

    pHandle = rockitVenc_alloc(stVencNeedParam);

    /*编码等级、码流模式*/
    if( VideoCodec_E::H264 == stVideoConfig.enVideoCodec)
    {
        if (stVideoConfig.enEncodingComplexity == EncodingComplexity_E::Baseline)
        {
            pHandle->stExParam.nProfile = H264E_PROFILE_BASELINE;
        }
        else if (stVideoConfig.enEncodingComplexity == EncodingComplexity_E::Main)
        {
            pHandle->stExParam.nProfile = H264E_PROFILE_MAIN;
        }
        else if (stVideoConfig.enEncodingComplexity == EncodingComplexity_E::High)
        {
            pHandle->stExParam.nProfile = H264E_PROFILE_HIGH;
        }

        if (stVideoConfig.enBitrateType == BitrateType_E::CBR)
        {
            pHandle->stExParam.enRcMode = VENC_RC_MODE_H264CBR;
        }
        else if (stVideoConfig.enBitrateType == BitrateType_E::VBR)
        {
            pHandle->stExParam.enRcMode = VENC_RC_MODE_H264VBR;
        }
    }
    else if( VideoCodec_E::H265 == stVideoConfig.enVideoCodec)
    {
        pHandle->stExParam.nProfile = H265E_PROFILE_MAIN;
        if (stVideoConfig.enBitrateType == BitrateType_E::CBR)
        {
            pHandle->stExParam.enRcMode = VENC_RC_MODE_H265CBR;
        }
        else if (stVideoConfig.enBitrateType == BitrateType_E::VBR)
        {
            pHandle->stExParam.enRcMode = VENC_RC_MODE_H265VBR;
        }
    }
    else if( VideoCodec_E::MJPEG == stVideoConfig.enVideoCodec)
    {
        pHandle->stExParam.nProfile = 0;
        if (stVideoConfig.enBitrateType == BitrateType_E::CBR)
        {
            pHandle->stExParam.enRcMode = VENC_RC_MODE_MJPEGCBR;
        }
        else if (stVideoConfig.enBitrateType == BitrateType_E::VBR)
        {
            pHandle->stExParam.enRcMode = VENC_RC_MODE_MJPEGVBR;
        }

        pHandle->stExParam.u32Qfactor = 70;
        pHandle->stExParam.u32MaxQfactor = 99;
        pHandle->stExParam.u32MinQfactor = 1;
    }
    else if(VideoCodec_E::JPEG== stVideoConfig.enVideoCodec)
    {
        pHandle->stExParam.unMbCnt = 2;
        pHandle->stExParam.unBufferSize = 2097152 ; //2048KB;
    }

    if (VideoCodec_E::MJPEG != stVideoConfig.enVideoCodec && VideoCodec_E::JPEG == stVideoConfig.enVideoCodec)
    {
        if (stVideoConfig.enSvcEnable == Video_NS::SvcMode_E::SVC_MODE_ENABLE ||
            stVideoConfig.enSvcEnable == Video_NS::SvcMode_E::SVC_MODE_AUTO)
        {
            pHandle->stExParam.bSvcEnable = RK_TRUE;
        }
    }

    /*码率大小*/
    pHandle->stExParam.nBitRate = stVideoConfig.nBitrateUpperLimit * 0.85; // 适当降低码率
    pHandle->stExParam.nMaxBitRate = pHandle->stExParam.nBitRate + 1024;
    pHandle->stExParam.nMinBitRate = pHandle->stExParam.nBitRate - 2000 > 256 ? pHandle->stExParam.nBitRate - 2000 : 256;
    /*平均码率*/
    pHandle->stExParam.nAverageBitrate = stVideoConfig.nAverageBitrate;
    /*码流平滑 [ 清晰<->平滑 ] min="1" max="100"*/
    pHandle->stExParam.nBitrateSmoothing = stVideoConfig.nBitrateSmoothing;
    /*图像质量*/
    pHandle->stExParam.nImageQuality = static_cast<int>(stVideoConfig.enImageQuality);

    /*ROI*/
    if (VideoCodec_E::MJPEG != stVideoConfig.enVideoCodec && VideoCodec_E::JPEG == stVideoConfig.enVideoCodec)
    {
        for (size_t i = 0; i < stVideoRoiConfig.vstVideoRoi.size(); i++)
        {
            // note：不要引用
            auto stVideoRoi = stVideoRoiConfig.vstVideoRoi[i];
            streamVenc_roi_attr_fill(pHandle, pHandle->stExParam.astRoiAttr[i], stVideoRoi);
        }
    }

    /*编码初始化*/
    nRet = pHandle->rockitVenc_init(pHandle);
    if (nRet != OK)
    {
        rockitVenc_release(pHandle);
        dlog_error("Venc初始化失败");
        return NULL;
    }

    dlog_info("Venc初始化成功");
    return pHandle;
}

RkVenc_S *streamVenc_init(const Video_NS::VideoConfig_S &stVideoConfig)
{
    Video_NS::VideoRoiConfig_S stVideoRoiConfig;
    return streamVenc_init(stVideoConfig, stVideoRoiConfig);
}

int streamVenc_uninit(RkVenc_S *pHandle)
{
    if (pHandle == NULL)
    {
        dlog_error("句柄为空");
        return ERR_PTR_NULL;
    }

    int nRet = OK;
    nRet = pHandle->rockitVenc_unInit(pHandle);
    if (nRet != OK)
    {
        dlog_error("streamVenc mppVenc_unInit error");
        return ERR;
    }
    rockitVenc_release(pHandle);

    dlog_info("Venc去初始化成功");
    return OK;
}

int streamVenc_reset(RkVenc_S *pHandle, const Video_NS::VideoConfig_S &stVideoConfig, const Video_NS::VideoRoiConfig_S &stVideoRoiConfig)
{
    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }

    int nRet = OK;
    nRet = streamVenc_uninit(pHandle);
    if (nRet != OK)
    {
        dlog_error("Venc模块去初始化失败");
        return ERR;
    }

    pHandle = streamVenc_init(stVideoConfig, stVideoRoiConfig);
    if (!pHandle)
    {
        dlog_error("Venc模块初始化失败");
        return ERR;
    }

    dlog_info("Venc重置成功");
    return OK;
}

int streamVenc_set_roi_attr(RkVenc_S *pHandle, const Video_NS::VideoRoiConfig_S &stVideoRoiConfig)
{
    /* 设置编码通道 */
    if(stVideoRoiConfig.nId >= VENC_CHN_MAX)
    {
        dlog_error("最大venc编码通道数为:%d", VENC_CHN_MAX);
        return ERR_PARAM;
    }

    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }

    int nRet = OK;
    for (size_t i = 0; i < stVideoRoiConfig.vstVideoRoi.size(); i++)
    {
        // note：不要引用
        auto stVideoRoi = stVideoRoiConfig.vstVideoRoi[i];
        VENC_ROI_ATTR_S stRoiAttr;
        streamVenc_roi_attr_fill(pHandle, stRoiAttr, stVideoRoi);

        nRet = pHandle->rockitVenc_set_roiAttr(pHandle, &stRoiAttr);
        if (nRet != OK)
        {
            dlog_error("设置第%d码流感兴趣编码区域[%d] 属性失败", stVideoRoiConfig.nId + 1, i);
            return ERR;
        }
        dlog_trace("设置第%d码流感兴趣编码区域[%d] 属性成功", stVideoRoiConfig.nId + 1, i);
    }

    return OK;
}

int streamVenc_set_chnCrop(RkVenc_S *pHandle, const Video_NS::AreaCrop_S &stAreaCrop)
{
    /* 设置编码通道 */
    if (stAreaCrop.nId >= VENC_CHN_JPEG)
    {
        dlog_error("最大可设置venc编码通道数为:%d", VENC_CHN_JPEG);
        return ERR_PARAM;
    }

    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }

    VENC_CROP_TYPE_E  enCropType = VENC_CROP_NONE;
    if (stAreaCrop.bEnable)
    {
        enCropType = VENC_CROP_ONLY;
    }

    RECT_S stCropRect;
    // note：转换坐标 插件比例->实际比例
    Common::Rect_S stRect = stAreaCrop.stRect;
    dlog_debug("VencChn:[%d,%d],AreaCrop:[%d,%d][%d,%d]",
               pHandle->stNeedParam.unWidth,
               pHandle->stNeedParam.unHeight,
               stRect.nX,
               stRect.nY,
               stRect.nWidth,
               stRect.nHeight);
    stRect.ConvertResolution(PLUG_IN_WIDTH_DEFAULT,
                             PLUG_IN_HEIGHT_DEFAULT,
                             pHandle->stNeedParam.unWidth,
                             pHandle->stNeedParam.unHeight);
    /* 裁剪区域起始点坐标和宽高要求2像素对齐 */
    stCropRect.s32X = ALIGN_BACK(stRect.nX, 2);
    stCropRect.s32Y = ALIGN_BACK(stRect.nY, 2);
    stCropRect.u32Width = ALIGN_BACK(stRect.nWidth, 2);
    stCropRect.u32Height = ALIGN_BACK(stRect.nHeight, 2);

    int nRet = pHandle->rockitVenc_set_corpOrScale(pHandle, enCropType, &stCropRect, NULL);
    if (nRet != RK_SUCCESS)
    {
        dlog_error("设置第%d码流设置通道裁剪缩放失败", stAreaCrop.nId);
        return ERR;
    }

    return OK;
}

int streamVenc_set_chnCropScale(RkVenc_S *pHandle, const Video_NS::AreaCrop_S &stAreaCrop)
{
    /* 设置编码通道 */
    if (stAreaCrop.nId >= VENC_CHN_JPEG)
    {
        dlog_error("最大可设置venc编码通道数为:%d", VENC_CHN_JPEG);
        return ERR_PARAM;
    }

    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }

    if (stAreaCrop.stResolution.nWidth >= (int) pHandle->stNeedParam.unWidth
        || stAreaCrop.stResolution.nHeight >= (int) pHandle->stNeedParam.unHeight)
    {
        dlog(LOG_ERROR, "参数错误,裁剪宽高大于等于当前编码宽高");
        return ERR_PARAM;
    }

    VENC_CROP_TYPE_E enCropType = VENC_CROP_NONE;
    if (stAreaCrop.bEnable)
    {
        enCropType = VENC_CROP_SCALE;
    }

    VENC_SCALE_RECT_S stScaleRect;
    // note：转换坐标 插件比例->实际比例
    Common::Rect_S stRect = stAreaCrop.stRect;
    dlog_debug("VencChn:[%d,%d],AreaCrop:[%d,%d][%d,%d],Resolution:[%d,%d]",
               pHandle->stNeedParam.unWidth,
               pHandle->stNeedParam.unHeight,
               stRect.nX,
               stRect.nY,
               stRect.nWidth,
               stRect.nHeight,
               stAreaCrop.stResolution.nWidth,
               stAreaCrop.stResolution.nHeight);
    stRect.ConvertResolution(PLUG_IN_WIDTH_DEFAULT,
                             PLUG_IN_HEIGHT_DEFAULT,
                             pHandle->stNeedParam.unWidth,
                             pHandle->stNeedParam.unHeight);
    /* 裁剪区域起始点坐标和宽高要求2像素对齐 */
    stScaleRect.stSrc.s32X = ALIGN_BACK(stRect.nX, 2);
    stScaleRect.stSrc.s32Y = ALIGN_BACK(stRect.nY, 2);
    stScaleRect.stSrc.u32Width = ALIGN_BACK(stRect.nWidth, 2);
    stScaleRect.stSrc.u32Height = ALIGN_BACK(stRect.nHeight, 2);
    /* 裁剪后，缩放到指定分辨率 */
    stScaleRect.stDst.s32X = ALIGN_BACK(0, 2);
    stScaleRect.stDst.s32Y = ALIGN_BACK(0, 2);
    stScaleRect.stDst.u32Width = ALIGN_BACK(stAreaCrop.stResolution.nWidth, 2);
    stScaleRect.stDst.u32Height = ALIGN_BACK(stAreaCrop.stResolution.nHeight, 2);

    int nRet = pHandle->rockitVenc_set_corpOrScale(pHandle, enCropType, NULL, &stScaleRect);
    if (nRet != RK_SUCCESS)
    {
        dlog_error("设置第%d码流设置通道裁剪缩放失败", stAreaCrop.nId);
        return ERR;
    }

    return OK;
}

int streamVenc_send_frame(RkVenc_S *pHandle, VIDEO_FRAME_INFO_S *pFrame, int nTimeOutMs)
{
    if (pHandle != NULL)
    {
        pHandle->rockitVenc_send_VFrame(pHandle, pFrame, 0);
    }
    return OK;
}
