/**
 * @FilePath     : stream_vpss.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-03-21 10:29:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-20 10:19:52
 * @Description  : VPSS 视频处理
 */

#include "stream_vpss.h"
#include "dlog.h"

/*二维Vpss句柄数组*/
HiVpss_S *g_pVpssHandle[VPSS_GROUP_SUM];

HiVpss_S **streamVpss_init(HiVpss_S ***pHandle, const std::vector<Video_NS::VideoConfig_S> &vstVideoConfig)
{
    /*开辟vpss组总数*/
    int nVpssGroupSum = VPSS_GROUP_SUM;
    HiVpssChnAttr_S *pVpssChnAttr = NULL;
    HiVpssNeedParam_S stVpssNeedParam;

    int nRet = OK;
    for (int nVpssGrp = 0; nVpssGrp < nVpssGroupSum; nVpssGrp++)
    {
        memset(&stVpssNeedParam, 0, sizeof(HiVpssNeedParam_S));
        if (nVpssGrp == VPSS_MAIN_SUB)
        {
            stVpssNeedParam.nVpssChnSum = VPSS_MAIN_SUB_CHN;
        }

        stVpssNeedParam.nVpssGrp = nVpssGrp;
        stVpssNeedParam.stVpssGrpAttr.enGrpComMode = COMPRESSMODE;
        stVpssNeedParam.stVpssGrpAttr.enGrpPixelFormat = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        stVpssNeedParam.stVpssGrpAttr.nMaxW = PIXEL_WIDTH_2_5K;
        stVpssNeedParam.stVpssGrpAttr.nMaxH = PIXEL_HEIGHT_2_5K;
        stVpssNeedParam.stVpssGrpAttr.nSrcFrameRate = -1;
        stVpssNeedParam.stVpssGrpAttr.nDstFrameRate = -1;

        for (int nVpssChn = 0; nVpssChn < stVpssNeedParam.nVpssChnSum; nVpssChn++)
        {
            pVpssChnAttr = &stVpssNeedParam.astVpssChnAttr[nVpssChn];
            pVpssChnAttr->enChnPixelFormat = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
            pVpssChnAttr->enChnComMode = COMPRESSMODE;
            pVpssChnAttr->enVpssChnMode = OT_VPSS_CHN_MODE_USER;
            pVpssChnAttr->nDepth = 0;
            pVpssChnAttr->nSrcFrameRate = -1;
            pVpssChnAttr->nDstFrameRate = -1;
            pVpssChnAttr->bWrapEnable = TD_FALSE;
            pVpssChnAttr->bLowDelay = TD_FALSE;
            pVpssChnAttr->bOneBufEn = TD_FALSE;
            pVpssChnAttr->bSmallStreamSize = TD_FALSE;
            pVpssChnAttr->nSmallStreamWidth = 0;
            pVpssChnAttr->nSmallStreamHeight = 0;
            if (nVpssGrp == VPSS_MAIN_SUB)
            {
                if (nVpssChn == VPSS_CHANNEL_MAIN) // 主码流
                {
                    pVpssChnAttr->nWidth = vstVideoConfig[nVpssChn].stVideoResolution.nWidth;
                    pVpssChnAttr->nHeight = vstVideoConfig[nVpssChn].stVideoResolution.nHeight;
                    pVpssChnAttr->nMaxWidth = PIXEL_WIDTH_2_5K;
                    pVpssChnAttr->nMaxHeight = PIXEL_HEIGHT_2_5K;
                    pVpssChnAttr->bWrapEnable = TD_TRUE;
                    pVpssChnAttr->enChnComMode = OT_COMPRESS_MODE_SEG_COMPACT;
                    pVpssChnAttr->nSrcFrameRate = 30;
                    pVpssChnAttr->nDstFrameRate = 30;
                    pVpssChnAttr->bSmallStreamSize = TD_TRUE;
                    pVpssChnAttr->nSmallStreamWidth = PIXEL_WIDTH_1680;
                    pVpssChnAttr->nSmallStreamHeight = PIXEL_HEIGHT_954;
                }
                else if (nVpssChn == VPSS_CHANNEL_SUB) // 子码流
                {
                    pVpssChnAttr->nWidth = vstVideoConfig[nVpssChn].stVideoResolution.nWidth;
                    pVpssChnAttr->nHeight = vstVideoConfig[nVpssChn].stVideoResolution.nHeight;
                    pVpssChnAttr->nMaxWidth = PIXEL_WIDTH_704;
                    pVpssChnAttr->nMaxHeight = PIXEL_HEIGHT_576;
                    pVpssChnAttr->bLowDelay = TD_TRUE;
                }
                else if (nVpssChn == VPSS_CHANNEL_AI) // AI 检测
                {
                    pVpssChnAttr->nWidth = PIXEL_WIDTH_1024;
                    pVpssChnAttr->nHeight = PIXEL_HEIGHT_576;
                    pVpssChnAttr->nMaxWidth = PIXEL_WIDTH_1024;
                    pVpssChnAttr->nMaxHeight = PIXEL_HEIGHT_576;
                    pVpssChnAttr->nDepth = 6;
                    pVpssChnAttr->nSrcFrameRate = 30;
                    pVpssChnAttr->nDstFrameRate = 30;
                }
            }
        }
        g_pVpssHandle[nVpssGrp] = mppVpss_alloc(stVpssNeedParam);
        nRet = g_pVpssHandle[nVpssGrp]->mppVpss_init(g_pVpssHandle[nVpssGrp]);
        if (OK != nRet)
        {
            dlog_error("mppVpss_init grp:%d  failed with %#x", nVpssGrp, nRet);
            return NULL;
        }
    }

    dlog_info("Vpss创建成功");
    *pHandle = g_pVpssHandle;
    return g_pVpssHandle;
}

void streamVpss_uninit()
{
    for (int nVpssGrp = 0; nVpssGrp < VPSS_GROUP_SUM; nVpssGrp++)
    {
        if (g_pVpssHandle[nVpssGrp] != NULL)
        {
            int nRet = g_pVpssHandle[nVpssGrp]->mppVpss_uninit(g_pVpssHandle[nVpssGrp]);
            if (OK != nRet)
            {
                dlog_error("mppVpss_uninit grp:%d failed with %#x", nVpssGrp, nRet);
            }
            mppVpss_release(g_pVpssHandle[nVpssGrp]);
            g_pVpssHandle[nVpssGrp] = NULL;
            dlog_info("Vpss释放成功");
        }
    }
}

int streamVpss_set_chnAttr(HiVpss_S *pHandle, int nVpssChn, std::vector<Video_NS::VideoConfig_S> &vstVideoConfig)
{
    HiVpssChnAttr_S stChnAttr;
    if (pHandle->mppVpss_get_chnAttr(pHandle, &stChnAttr, nVpssChn))
    {
        dlog_error("channel:%d 获取通道属性失败", nVpssChn);
        return ERR;
    }

    stChnAttr.nWidth = vstVideoConfig[nVpssChn].stVideoResolution.nWidth;
    stChnAttr.nHeight = vstVideoConfig[nVpssChn].stVideoResolution.nHeight;

    if(pHandle->mppVpss_set_chnAttr(pHandle, &stChnAttr, nVpssChn))
    {
        dlog_error("channel:%d 设置通道属性失败", nVpssChn);
        return ERR;
    }

    return OK;
}

int streamVpss_set_chnAttr(HiVpss_S *pHandle, const Video_NS::VideoConfig_S &stVideoConfig)
{
    HiVpssChnAttr_S stChnAttr;
    int nVpssChn = stVideoConfig.nId;
    if (pHandle->mppVpss_get_chnAttr(pHandle, &stChnAttr, nVpssChn))
    {
        dlog_error("channel:%d 获取通道属性失败", nVpssChn);
        return ERR;
    }

    stChnAttr.nWidth = stVideoConfig.stVideoResolution.nWidth;
    stChnAttr.nHeight = stVideoConfig.stVideoResolution.nHeight;

    if(pHandle->mppVpss_set_chnAttr(pHandle, &stChnAttr, nVpssChn))
    {
        dlog_error("channel:%d 设置通道属性失败", nVpssChn);
        return ERR;
    }

    return OK;
}

int streamVpss_set_chnCrop(HiVpss_S *pHandle, const Video_NS::AreaCrop_S &stAreaCrop)
{
    int nRet = OK;
    /* 设置 VPSS 通道CROP裁剪 */
    ot_vpss_crop_info stCropInfo;
    stCropInfo.enable = (td_bool)stAreaCrop.bEnable;
    stCropInfo.crop_mode = OT_COORD_ABS; // 绝对坐标模式
    // note：转换坐标 插件比例->实际比例
    Common::Rect_S stRect = stAreaCrop.stRect;
    int nId = stAreaCrop.nId;
    dlog_debug("VpssChn:[%d,%d],AreaCrop:[%d,%d][%d,%d]", pHandle->astVpssChnAttr[nId].nWidth,
               pHandle->astVpssChnAttr[nId].nHeight, stRect.nX, stRect.nY, stRect.nWidth,
               stRect.nHeight);
    stRect.ConvertResolution(PLUG_IN_WIDTH_DEFAULT, PLUG_IN_HEIGHT_DEFAULT,
                             pHandle->astVpssChnAttr[nId].nWidth,
                             pHandle->astVpssChnAttr[nId].nHeight);
    /* 裁剪区域起始点坐标和宽高要求2像素对齐 */
    stCropInfo.crop_rect.x = ALIGN_BACK(stRect.nX, 2);
    stCropInfo.crop_rect.y = ALIGN_BACK(stRect.nY, 2);
    // stCropInfo.crop_rect.width = ALIGN_BACK(stRect.nWidth, 2);
    // stCropInfo.crop_rect.height = ALIGN_BACK(stRect.nHeight, 2);
    stCropInfo.crop_rect.width = ALIGN_BACK(stAreaCrop.stResolution.nWidth, 2);
    stCropInfo.crop_rect.height = ALIGN_BACK(stAreaCrop.stResolution.nHeight, 2);
    nRet = pHandle->mppVpss_set_chnCrop(pHandle, &stCropInfo, nId);
    if (TD_SUCCESS != nRet)
    {
        dlog_error("设置Grp:%d 通道:%d 裁剪失败: %#x", pHandle->nVpssGrp, nId, nRet);
        return ERR;
    }
    return OK;
}

int streamVpss_reset_wrap(HiVpss_S *pHandle, const Video_NS::VideoConfig_S &stVideoConfig)
{
    int nRet = OK;
    nRet = pHandle->mppVpss_reset_wrap(pHandle,
                                       stVideoConfig.stVideoResolution.nWidth,
                                       stVideoConfig.stVideoResolution.nHeight);
    if (TD_SUCCESS != nRet)
    {
        dlog_error("重新设置Grp:%d 通道:0 卷绕失败: %#x", pHandle->nVpssGrp, nRet);
        return ERR;
    }
    return OK;
}
