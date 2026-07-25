/*
 * @FilePath     : stream_vpss.c
 * @Author       : zhouzirui
 * @Date         : 2024-09-26 11:23:29
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2025-11-17 20:39:37
 * @Description  : VPSS 视频处理
 */

#include "stream_vpss.h"
#include "dlog.h"


/*二维Vpss句柄数组*/
RkVpss_S *g_pVpssHandle[VPSS_GROUP_SUM];

RkVpss_S **streamVpss_init(RkVpss_S ***pHandle, const std::vector<Video_NS::VideoConfig_S> &vstVideoConfig)
{
    /*开辟vpss组总数*/
    int nVpssGroupSum = VPSS_GROUP_SUM;
    RK_S32 nRet = RK_SUCCESS;
    RkVpssChnAttr_S *pVpssChnAttr = NULL;
    RkVpssNeedParam_S stVpssNeedParam;

    for (int nVpssGrp = 0; nVpssGrp < nVpssGroupSum; nVpssGrp++)
    {
        memset(&stVpssNeedParam, 0, sizeof(RkVpssNeedParam_S));
        stVpssNeedParam.nVpssGrp = nVpssGrp;
        stVpssNeedParam.stVpssGrpAttr.enGrpComMode = COMPRESSMODE;
        stVpssNeedParam.stVpssGrpAttr.enGrpPixelFormat = RK_FMT_YUV420SP;
        stVpssNeedParam.stVpssGrpAttr.nMaxW = PIXEL_WIDTH_4K;
        stVpssNeedParam.stVpssGrpAttr.nMaxH = PIXEL_HEIGHT_4K;
        stVpssNeedParam.stVpssGrpAttr.nSrcFrameRate = -1;
        stVpssNeedParam.stVpssGrpAttr.nDstFrameRate = -1;
        stVpssNeedParam.stVpssGrpAttr.nEnBackup = 1;
        stVpssNeedParam.nVpssChnSum = 3;
        if (nVpssGrp == VPSS_MAIN_SUB)
        {
            stVpssNeedParam.nVpssChnSum = VPSS_MAIN_SUB_CHN;
        }

        for (int nVpssChn = 0; nVpssChn < stVpssNeedParam.nVpssChnSum; nVpssChn++)
        {
            pVpssChnAttr = &stVpssNeedParam.astVpssChnAttr[nVpssChn];
            pVpssChnAttr->enChnPixelFormat = RK_FMT_YUV420SP;
            pVpssChnAttr->enChnComMode = COMPRESS_MODE_NONE;
            pVpssChnAttr->enVpssChnMode = VPSS_CHN_MODE_USER; // 如需裁剪生效需设置为VPSS_CHN_MODE_PASSTHROUGH
            pVpssChnAttr->nDepth = 0;
            pVpssChnAttr->nFrameBufCnt = 2;
            pVpssChnAttr->nSrcFrameRate = -1;
            pVpssChnAttr->nDstFrameRate = -1;
            if(nVpssChn == VPSS_CHANNEL_MAIN) // 第一码流
            {
                    pVpssChnAttr->nWidth = PIXEL_WIDTH_4K;
                    pVpssChnAttr->nHeight = PIXEL_HEIGHT_4K;
                    pVpssChnAttr->nSrcFrameRate = -1;
                    pVpssChnAttr->nDstFrameRate = -1;
                    pVpssChnAttr->enChnComMode  = COMPRESSMODE;

            }
            else if (nVpssChn == VPSS_CHANNEL_SUB) // 第二码流
            {
                    pVpssChnAttr->nWidth = PIXEL_WIDTH_1920;
                    pVpssChnAttr->nHeight = PIXEL_HEIGHT_1080;
                    pVpssChnAttr->nSrcFrameRate = -1;
                    pVpssChnAttr->nDstFrameRate = -1;
            }
            else if (nVpssChn == VPSS_CHANNEL_AI) // AI 检测
            {
                    pVpssChnAttr->nWidth = PIXEL_WIDTH_1280;
                    pVpssChnAttr->nHeight = PIXEL_HEIGHT_720;
                    pVpssChnAttr->nDepth = 3;
                    pVpssChnAttr->nFrameBufCnt = 5;   //buf cnt 至少要比depth 大2
                    pVpssChnAttr->nSrcFrameRate = -1;
                    pVpssChnAttr->nDstFrameRate = -1;
            }
        }
        nVpssGrp = stVpssNeedParam.nVpssGrp;
        g_pVpssHandle[nVpssGrp] = rockit_vpss_alloc(stVpssNeedParam);
        nRet = g_pVpssHandle[nVpssGrp]->rockit_vpss_init(g_pVpssHandle[nVpssGrp]);
        if (RK_SUCCESS != nRet)
        {
            dlog(LOG_ERROR, "RK_MPI_VPSS_INIT grp:%d  failed with %#x", nVpssGrp, nRet);
            return NULL;
        }
    }


    for (int nVpssChn = 0; nVpssChn <= VPSS_CHANNEL_SUB; nVpssChn++)
    {
        /* 设置 VPSS 对应通道分辨率 */
         nRet = streamVpss_set_chnAttr(g_pVpssHandle[stVpssNeedParam.nVpssGrp],vstVideoConfig.at(nVpssChn));
         if (0 != nRet)
         {
             dlog(LOG_ERROR, "streamVpss_set_chnAttr grp:%d  failed with %#x", stVpssNeedParam.nVpssGrp, nRet);
             return NULL;
         }    
    }
    
    dlog(LOG_DEBUG, "Vpss创建成功");
    *pHandle = g_pVpssHandle;
    return g_pVpssHandle;
}

void streamVpss_uninit()
{
    for (int nVpssGrp = 0; nVpssGrp < VPSS_GROUP_SUM; nVpssGrp++)
    {
        if (g_pVpssHandle[nVpssGrp] != NULL)
        {
            RK_S32 nRet = g_pVpssHandle[nVpssGrp]->rockit_vpss_uninit(g_pVpssHandle[nVpssGrp]);
            if (RK_SUCCESS != nRet)
            {
                dlog(LOG_ERROR, "RK_MPI_VPSS_DEINIT grp:%d failed with %#x", nVpssGrp, nRet);
            }
            rockit_vpss_release(g_pVpssHandle[nVpssGrp]);
            g_pVpssHandle[nVpssGrp] = NULL; // 释放后置为NULL
            dlog(LOG_DEBUG, "Vpss释放成功");
        }
    }
}


int streamVpss_set_chnAttr(RkVpss_S *pHandle, int nVpssChn, std::vector<Video_NS::VideoConfig_S> &vstVideoConfig)
{
    RkVpssChnAttr_S pVpssChnAttr ;
    if (pHandle->rockitVpss_get_chnAttr(pHandle,&pVpssChnAttr,pHandle->nVpssGrp,nVpssChn))
    {
         dlog_error("channel:%d 获取通道属性失败", nVpssChn);
         return ERR;
    }

    if (nVpssChn == VPSS_CHANNEL_MAIN) // 主码流
    {
         pVpssChnAttr.nWidth = vstVideoConfig[nVpssChn].stVideoResolution.nWidth;
         pVpssChnAttr.nHeight = vstVideoConfig[nVpssChn].stVideoResolution.nHeight;
         pVpssChnAttr.nSrcFrameRate = vstVideoConfig[nVpssChn].enFrameRate;
         pVpssChnAttr.nDstFrameRate = vstVideoConfig[nVpssChn].enFrameRate;
    }
    else if (nVpssChn == VPSS_CHANNEL_SUB) // 子码流
     {
         pVpssChnAttr.nWidth = vstVideoConfig[nVpssChn].stVideoResolution.nWidth;
         pVpssChnAttr.nHeight = vstVideoConfig[nVpssChn].stVideoResolution.nHeight;
         pVpssChnAttr.nSrcFrameRate = vstVideoConfig[nVpssChn].enFrameRate;
         pVpssChnAttr.nDstFrameRate = vstVideoConfig[nVpssChn].enFrameRate;
    }

    if(pHandle->rockitVpss_set_chnAttr(pHandle, &pVpssChnAttr,pHandle->nVpssGrp,nVpssChn))
    {
        dlog_error("channel:%d 设置通道属性失败", nVpssChn);
        return ERR;
    }

    return OK;
}

int streamVpss_set_chnAttr(RkVpss_S *pHandle, const Video_NS::VideoConfig_S &stVideoConfig)
{
    RkVpssChnAttr_S pVpssChnAttr;
    int nVpssChn = stVideoConfig.nId;
    if (pHandle->rockitVpss_get_chnAttr(pHandle, &pVpssChnAttr, pHandle->nVpssGrp, nVpssChn))
    {
        dlog_error("channel:%d 获取通道属性失败", nVpssChn);
        return ERR;
    }

    pVpssChnAttr.nWidth = stVideoConfig.stVideoResolution.nWidth;
    pVpssChnAttr.nHeight = stVideoConfig.stVideoResolution.nHeight;

    if (pHandle->rockitVpss_set_chnAttr(pHandle, &pVpssChnAttr, pHandle->nVpssGrp, nVpssChn))
    {
        dlog_error("channel:%d 设置通道属性失败", nVpssChn);
        return ERR;
    }

    return OK;
}

int streamVpss_get_chnFrame(RkVpss_S *pHandle,StreamVpssFrame_t* stVpssFrame)
{

    if (pHandle->rockitVpss_get_chnFrame(pHandle,&stVpssFrame->pstVideoFrame,stVpssFrame->channel,1000))
    {
        dlog_error("channel:%d 获取通道帧失败", stVpssFrame->channel);
        return ERR;
    }

    if (pHandle->rockitVpss_get_chnFrameData(pHandle, &stVpssFrame->pstVideoFrame, &stVpssFrame->framedata, &stVpssFrame->framesize))
    {
        dlog_error("channel:%d 获取通道帧数据大小失败", stVpssFrame->channel);
        return ERR;
    }

    return OK;
}

int streamVpss_release_chnFrame(RkVpss_S *pHandle,StreamVpssFrame_t* stVpssFrame)
{
    if (pHandle->rockitVpss_release_chnFrame(pHandle,&stVpssFrame->pstVideoFrame,stVpssFrame->channel))
    {
        dlog_error("channel:%d 释放通道帧失败", stVpssFrame->channel);
        return ERR;
    }

    return OK;
}

int streamVpss_set_chnCrop(RkVpss_S *pHandle, const Video_NS::AreaCrop_S &stAreaCrop)
{
    int nRet = OK;
    /* 设置 VPSS 通道CROP裁剪 */
    VPSS_CROP_INFO_S stCropInfo;
    // note RK 必须强制设置enable为RK_TRUE，否则无法关闭crop
    stCropInfo.bEnable = RK_TRUE;
    stCropInfo.enCropCoordinate = VPSS_CROP_ABS_COOR; // 绝对坐标模式
    // note：转换坐标 插件比例->实际比例
    Common::Rect_S stRect = stAreaCrop.stRect;
    int nId = stAreaCrop.nId;
    if (!stAreaCrop.bEnable)
    {
        stRect.nX = 0;
        stRect.nY = 0;
        stRect.nWidth = pHandle->stVpssGrpAttr.nMaxW;
        stRect.nHeight = pHandle->stVpssGrpAttr.nMaxH;
    }
    else
    {
        dlog_debug("AreaCrop:[%d,%d][%d,%d]", stRect.nX, stRect.nY, stRect.nWidth, stRect.nHeight);
        stRect.ConvertResolution(PLUG_IN_WIDTH_DEFAULT,
                                 PLUG_IN_HEIGHT_DEFAULT,
                                 pHandle->stVpssGrpAttr.nMaxW,
                                 pHandle->stVpssGrpAttr.nMaxH);
    }

    /* 裁剪区域起始点坐标和宽高要求2像素对齐 */
    stCropInfo.stCropRect.s32X = ALIGN_BACK(stRect.nX, 2);
    stCropInfo.stCropRect.s32Y = ALIGN_BACK(stRect.nY, 2);
    stCropInfo.stCropRect.u32Width = ALIGN_BACK(stRect.nWidth, 2);
    stCropInfo.stCropRect.u32Height = ALIGN_BACK(stRect.nHeight, 2);
    nRet = pHandle->rockitVpss_set_chnCrop(pHandle, &stCropInfo, nId);
    if (RK_SUCCESS != nRet)
    {
        dlog_error("设置Grp:%d 通道:%d 裁剪失败: %#x", pHandle->nVpssGrp, nId, nRet);
        return ERR;
    }

    dlog_trace("设置Grp:%d 通道:%d 裁剪成功", pHandle->nVpssGrp, nId);

    return OK;
}
