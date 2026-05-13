/**
 * @FilePath     : stream_venc.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-03-21 10:29:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-05 14:07:52
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
int streamVenc_roi_attr_fill(HiVenc_S *pHandle, ot_venc_roi_attr &stRoiAttr,
                             Video_NS::VideoRoi_S &stVideoRoi)
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

    stRoiAttr.idx = stVideoRoi.u32Idx;
    stRoiAttr.enable = static_cast<td_bool>(stVideoRoi.bEnable);
    stRoiAttr.is_abs_qp = TD_FALSE;
    stRoiAttr.qp = ROI_QP_DEFAULT + (stVideoRoi.u32Level * ROI_QP_DEFAULT_FACTOR);
    /* 限制矩形在视频范围内 */
    stRoiAttr.rect.x = std::max(0, ALIGN_UP(stVideoRoi.stRect.nX, 16));
    stRoiAttr.rect.y = std::max(0, ALIGN_UP(stVideoRoi.stRect.nY, 16));
    td_s32 maxWidth = pHandle->stNeedParam.unWidth - stRoiAttr.rect.x;
    td_s32 maxHeight = pHandle->stNeedParam.unHeight - stRoiAttr.rect.y;
    stRoiAttr.rect.width = std::min(ALIGN_UP(stVideoRoi.stRect.nWidth, 16), maxWidth);
    stRoiAttr.rect.height = std::min(ALIGN_UP(stVideoRoi.stRect.nHeight, 16), maxHeight);
    return OK;
}

HiVenc_S *streamVenc_init(const Video_NS::VideoConfig_S &stVideoConfig, const Video_NS::VideoRoiConfig_S &stVideoRoiConfig)
{
    int nChannel = stVideoConfig.nId;
    /* 设置编码通道 */
    if(nChannel >= VENC_CHN_MAX)
    {
        dlog_error("最大venc编码通道数为:%d", VENC_CHN_MAX);
        return NULL;
    }

    HiVenc_S *pHandle = (HiVenc_S *)malloc(sizeof(HiVenc_S));
    memset(pHandle, 0, sizeof(HiVenc_S));

    int nRet = OK;

    HiVencNeedParam_S stVencNeedParam;
    memset(&stVencNeedParam, 0, sizeof(HiVencNeedParam_S));

    /* 编码的必须参数 */
    stVencNeedParam.unWidth         = stVideoConfig.stVideoResolution.nWidth;
    stVencNeedParam.unHeight        = stVideoConfig.stVideoResolution.nHeight;
    stVencNeedParam.unVirWidth      = stVideoConfig.stVideoResolution.nWidth;
    stVencNeedParam.unVirHeight     = stVideoConfig.stVideoResolution.nHeight;
    stVencNeedParam.enPixFormat     = OT_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    stVencNeedParam.nGop            = stVideoConfig.nIFrameInterval;
    stVencNeedParam.enGopMode       = OT_VENC_GOP_MODE_NORMAL_P;
    stVencNeedParam.nChn            = nChannel;
    stVencNeedParam.nInFrameRate    = 30;
    stVencNeedParam.nOutFrameRate   = stVideoConfig.getFrameRateAsInt();
    // stVencNeedParam.enCompressMode  = COMPRESSMODE;
    stVencNeedParam.enCodec         = OT_PT_H264;

    using namespace Video_NS;
    /*编码格式*/
    if( VideoCodec_E::H264 == stVideoConfig.enVideoCodec)
    {
        stVencNeedParam.enCodec = OT_PT_H264;
    }
    else if( VideoCodec_E::H265 == stVideoConfig.enVideoCodec)
    {
        stVencNeedParam.enCodec = OT_PT_H265;
    }
    else if( VideoCodec_E::SVAC3== stVideoConfig.enVideoCodec)
    {
        stVencNeedParam.enCodec = OT_PT_SVAC3;
    }
    else if( VideoCodec_E::JPEG== stVideoConfig.enVideoCodec)
    {
        stVencNeedParam.enCodec = OT_PT_JPEG;
    }
    else if( VideoCodec_E::MJPEG== stVideoConfig.enVideoCodec)
    {
        stVencNeedParam.enCodec = OT_PT_MJPEG;
    }

    /*智能编码*/
    if(stVideoConfig.bSmartEnable == true)
    {
        stVencNeedParam.enGopMode = OT_VENC_GOP_MODE_SMART_P;
    }

    /* 是否开启卷绕,以启用帧节省模式 */
    if(stVideoConfig.nId == VENC_CHN_MAIN)
    {
        stVencNeedParam.bWrapEnable = TD_TRUE;
    }
    pHandle = mppVenc_alloc(stVencNeedParam);

    /*编码等级、码流模式*/
    if( VideoCodec_E::H264 == stVideoConfig.enVideoCodec)
    {
        if(stVideoConfig.enEncodingComplexity == EncodingComplexity_E::Baseline)
        {
            pHandle->stExParam.nProfile = 0;
        }else if(stVideoConfig.enEncodingComplexity == EncodingComplexity_E::Main)
        {
            pHandle->stExParam.nProfile = 1;
        }else if(stVideoConfig.enEncodingComplexity == EncodingComplexity_E::High)
        {
            pHandle->stExParam.nProfile = 2;
        }
        if(stVideoConfig.enBitrateType == BitrateType_E::CBR)
        {
            pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_H264_CBR;
            // pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_H264_CVBR;
        }else if(stVideoConfig.enBitrateType == BitrateType_E::VBR)
        {
            pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_H264_VBR;
            // pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_H264_AVBR;
        }
    }
    else if( VideoCodec_E::H265 == stVideoConfig.enVideoCodec)
    {
        if(stVideoConfig.enBitrateType == BitrateType_E::CBR)
        {
            pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_H265_CBR;
            // pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_H265_CVBR;
        }else if(stVideoConfig.enBitrateType == BitrateType_E::VBR)
        {
            pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_H265_VBR;
            // pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_H265_AVBR;
        }
    }
    else if( VideoCodec_E::SVAC3 == stVideoConfig.enVideoCodec)
    {
        if(stVideoConfig.enBitrateType == BitrateType_E::CBR)
        {
            pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_SVAC3_CBR;
            // pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_SVAC3_CVBR;
        }else if(stVideoConfig.enBitrateType == BitrateType_E::VBR)
        {
            pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_SVAC3_VBR;
            // pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_SVAC3_AVBR;
        }
    }
    else if( VideoCodec_E::MJPEG == stVideoConfig.enVideoCodec)
    {
        if(stVideoConfig.enBitrateType == BitrateType_E::CBR)
        {
            pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_MJPEG_CBR;
        }else if(stVideoConfig.enBitrateType == BitrateType_E::VBR)
        {
            pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_MJPEG_VBR;
        }
    }

    if (VideoCodec_E::MJPEG != stVideoConfig.enVideoCodec && VideoCodec_E::JPEG == stVideoConfig.enVideoCodec)
    {
        if (stVideoConfig.enSvcEnable == Video_NS::SvcMode_E::SVC_MODE_ENABLE ||
            stVideoConfig.enSvcEnable == Video_NS::SvcMode_E::SVC_MODE_AUTO)
        {
            pHandle->stExParam.bSvcEnable = TD_TRUE;
        }
    }

    /*码率大小*/
    pHandle->stExParam.nBitRate = stVideoConfig.nBitrateUpperLimit * 0.75; // 适当降低码率
    pHandle->stExParam.nMaxBitRate = pHandle->stExParam.nBitRate;
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
    nRet = pHandle->mppVenc_init(pHandle);
    if(nRet != OK)
    {
        mppVenc_release(pHandle);
        dlog_error("Venc初始化失败");
        return NULL;
    }

    dlog_info("Venc初始化成功");
    return pHandle;
}

HiVenc_S *streamVenc_init(const Video_NS::VideoConfig_S &stVideoConfig)
{
    Video_NS::VideoRoiConfig_S stVideoRoiConfig;
    return streamVenc_init(stVideoConfig, stVideoRoiConfig);
}

int streamVenc_uninit(HiVenc_S *&pHandle)
{
    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }
    int nRet = OK;
    nRet = pHandle->mppVenc_unInit(pHandle);
    if (nRet != OK)
    {
        dlog_error("streamVenc mppVenc_unInit error");
        return ERR;
    }
    mppVenc_release(pHandle);

    dlog_info("Venc去初始化成功");
    return OK;
}

int streamVenc_reset(HiVenc_S *&pHandle, const Video_NS::VideoConfig_S &stVideoConfig, const Video_NS::VideoRoiConfig_S &stVideoRoiConfig)
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

int streamVenc_reset_attr(HiVenc_S *pHandle, const Video_NS::VideoConfig_S &stVideoConfig, const Video_NS::VideoRoiConfig_S &stVideoRoiConfig)
{
    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }

    int nRet = OK;

    /* 编码的必须参数 */
    pHandle->stNeedParam.nGop            = stVideoConfig.nIFrameInterval;
    pHandle->stNeedParam.nInFrameRate    = 30;
    pHandle->stNeedParam.nOutFrameRate   = stVideoConfig.getFrameRateAsInt();

    using namespace Video_NS;

    /*智能编码*/
    if(stVideoConfig.bSmartEnable == true)
    {
        pHandle->stNeedParam.enGopMode = OT_VENC_GOP_MODE_SMART_P;
    }else{
        pHandle->stNeedParam.enGopMode = OT_VENC_GOP_MODE_NORMAL_P;
    }

    /*编码等级、码流模式*/
    if( VideoCodec_E::H264 == stVideoConfig.enVideoCodec)
    {
        if(stVideoConfig.enEncodingComplexity == EncodingComplexity_E::Baseline)
        {
            pHandle->stExParam.nProfile = 0;
        }else if(stVideoConfig.enEncodingComplexity == EncodingComplexity_E::Main)
        {
            pHandle->stExParam.nProfile = 1;
        }else if(stVideoConfig.enEncodingComplexity == EncodingComplexity_E::High)
        {
            pHandle->stExParam.nProfile = 2;
        }
        if(stVideoConfig.enBitrateType == BitrateType_E::CBR)
        {
            pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_H264_CBR;
            // pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_H264_CVBR;
        }else if(stVideoConfig.enBitrateType == BitrateType_E::VBR)
        {
            pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_H264_VBR;
            // pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_H264_AVBR;
        }
    }
    else if( VideoCodec_E::H265 == stVideoConfig.enVideoCodec)
    {
        if(stVideoConfig.enBitrateType == BitrateType_E::CBR)
        {
            pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_H265_CBR;
            // pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_H265_CVBR;
        }else if(stVideoConfig.enBitrateType == BitrateType_E::VBR)
        {
            pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_H265_VBR;
            // pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_H265_AVBR;
        }
    }
    else if( VideoCodec_E::SVAC3 == stVideoConfig.enVideoCodec)
    {
        if(stVideoConfig.enBitrateType == BitrateType_E::CBR)
        {
            pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_SVAC3_CBR;
            // pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_SVAC3_CVBR;
        }else if(stVideoConfig.enBitrateType == BitrateType_E::VBR)
        {
            pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_SVAC3_VBR;
            // pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_SVAC3_AVBR;
        }
    }
    else if( VideoCodec_E::MJPEG == stVideoConfig.enVideoCodec)
    {
        if(stVideoConfig.enBitrateType == BitrateType_E::CBR)
        {
            pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_MJPEG_CBR;
        }else if(stVideoConfig.enBitrateType == BitrateType_E::VBR)
        {
            pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_MJPEG_VBR;
        }
    }

    if (VideoCodec_E::MJPEG != stVideoConfig.enVideoCodec && VideoCodec_E::JPEG == stVideoConfig.enVideoCodec)
    {
        if (stVideoConfig.enSvcEnable == Video_NS::SvcMode_E::SVC_MODE_ENABLE ||
            stVideoConfig.enSvcEnable == Video_NS::SvcMode_E::SVC_MODE_AUTO)
        {
            pHandle->stExParam.bSvcEnable = TD_TRUE;
        }
    }

    /*码率大小*/
    pHandle->stExParam.nBitRate = stVideoConfig.nBitrateUpperLimit;
    pHandle->stExParam.nMaxBitRate = pHandle->stExParam.nBitRate;
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

    nRet = pHandle->mppVenc_reset_attr(pHandle);
    if (nRet != OK)
    {
        dlog_error("Venc模块重置属性失败");
        return ERR;
    }

    dlog_info("Venc重置属性成功");
    return OK;
}

int streamVenc_set_roi_attr(HiVenc_S *pHandle, const Video_NS::VideoRoiConfig_S &stVideoRoiConfig)
{
    /* 设置编码通道 */
    if (stVideoRoiConfig.nId >= VENC_CHN_JPEG)
    {
        dlog_error("最大可设置venc编码通道数为:%d", VENC_CHN_JPEG);
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
        ot_venc_roi_attr stRoiAttr;
        streamVenc_roi_attr_fill(pHandle, stRoiAttr, stVideoRoi);

        nRet = pHandle->mppVenc_set_roi_attr(pHandle, &stRoiAttr);
        if (nRet != OK)
        {
            dlog_error("设置第%d码流感兴趣编码区域[%d] 属性失败", stVideoRoiConfig.nId + 1, i);
            return ERR;
        }
        dlog_trace("设置第%d码流感兴趣编码区域[%d] 属性成功", stVideoRoiConfig.nId + 1, i);
    }

    return OK;
}

int streamVenc_set_crop(HiVenc_S *pHandle, const Video_NS::AreaCrop_S &stAreaCrop)
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

    if (stAreaCrop.stResolution.nWidth >= (int)pHandle->stNeedParam.unWidth ||
        stAreaCrop.stResolution.nHeight >= (int)pHandle->stNeedParam.unHeight)
    {
        dlog(LOG_ERROR, "参数错误,裁剪宽高大于等于当前编码宽高");
        return ERR_PARAM;
    }

    int nRet = OK;
    ot_crop_info stCropInfo;
    stCropInfo.enable = static_cast<td_bool>(stAreaCrop.bEnable);
    // note：转换坐标 插件比例->实际比例
    Common::Rect_S stRect = stAreaCrop.stRect;
    dlog_debug("VpssChn:[%d,%d],AreaCrop:[%d,%d][%d,%d]",
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
    stCropInfo.rect.x = ALIGN_BACK(stRect.nX, 8);
    stCropInfo.rect.y = ALIGN_BACK(stRect.nY, 2);
    stCropInfo.rect.width = ALIGN_BACK(stAreaCrop.stResolution.nWidth, 2);
    stCropInfo.rect.height = ALIGN_BACK(stAreaCrop.stResolution.nHeight, 2);

    nRet = pHandle->mppVenc_unInit(pHandle);
    if (nRet != OK)
    {
        dlog_error("streamVenc mppVenc_unInit error");
        return ERR;
    }

    nRet = pHandle->mppVenc_set_chn_crop(pHandle, stCropInfo);
    if (nRet != OK)
    {
        dlog_error("设置第%d码流设置通道截取Crop参数失败", stAreaCrop.nId);
        return ERR;
    }

    return OK;
}
