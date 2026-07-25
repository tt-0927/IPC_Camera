/*
 * @FilePath     : rockit_vpss.c
 * @Author       : wxz
 * @Date         : 2022-05-06 09:37:27
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2025-11-12 16:13:37
 * @Description  : 
 */

#include "rockit_vpss.h"

/*VPSS组管道号，取值默认为0*/
#define VPSS_GRP_PIPE 0

/*
 * *@description vpss资源初始化
 * *@Author: wxz
 * *@param[in] 
 * *@return 成功返回0,失败返回-1
 * */
int rockit_vpss_init(RkVpss_S *pRkVpssHandle)
{
    /*开辟vpss组总数*/
    int nVpssGrp=0;
    RK_S32 nRet = RK_SUCCESS;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;

    memset(&stVpssGrpAttr, 0, sizeof(VPSS_GRP_ATTR_S));
    memset(&stVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S));

    /*垂直翻转*/
    RK_BOOL nFlip = RK_FALSE;
    /*水平翻转*/
    RK_BOOL nMirror = RK_FALSE;

    /*vpss组参数设置*/
    stVpssGrpAttr.u32MaxW = pRkVpssHandle->stVpssGrpAttr.nMaxW;
    stVpssGrpAttr.u32MaxH = pRkVpssHandle->stVpssGrpAttr.nMaxH;
    /*设置像素格式*/
    stVpssGrpAttr.enPixelFormat = pRkVpssHandle->stVpssGrpAttr.enGrpPixelFormat;
    /*vpss压缩*/
    stVpssGrpAttr.enCompressMode = pRkVpssHandle->stVpssGrpAttr.enGrpComMode;
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate = pRkVpssHandle->stVpssGrpAttr.nSrcFrameRate;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate = pRkVpssHandle->stVpssGrpAttr.nDstFrameRate;
    stVpssGrpAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;//DYNAMIC_RANGE_SDR10
    stVpssGrpAttr.enVProcDev = pRkVpssHandle->enDevType;

    nVpssGrp = pRkVpssHandle->nVpssGrp;

    /*创建vpss通道组*/
    nRet=RK_MPI_VPSS_CreateGrp(nVpssGrp, &stVpssGrpAttr);
    if(nRet != RK_SUCCESS)
    {
        RK_LOGE("RK_MPI_VPSS_CreateGrp(grp:%d) failed with %#x!", nVpssGrp, nRet);
        return nRet;
    }
     RK_LOGE("nVpssGrp:%d enDevType:%d",nVpssGrp,pRkVpssHandle->enDevType);

    for(RK_S32 nChn=0; nChn<pRkVpssHandle->nVpssChnSum; nChn++)
    {
        stVpssChnAttr.enChnMode = pRkVpssHandle->astVpssChnAttr[nChn].enVpssChnMode;
        stVpssChnAttr.enCompressMode = pRkVpssHandle->astVpssChnAttr[nChn].enChnComMode;
        stVpssChnAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
        stVpssChnAttr.enPixelFormat = pRkVpssHandle->astVpssChnAttr[nChn].enChnPixelFormat;
        stVpssChnAttr.stFrameRate.s32SrcFrameRate = pRkVpssHandle->astVpssChnAttr[nChn].nSrcFrameRate;
        stVpssChnAttr.stFrameRate.s32DstFrameRate = pRkVpssHandle->astVpssChnAttr[nChn].nDstFrameRate;
        stVpssChnAttr.u32Width = pRkVpssHandle->astVpssChnAttr[nChn].nWidth;
        stVpssChnAttr.u32Height = pRkVpssHandle->astVpssChnAttr[nChn].nHeight;
        stVpssChnAttr.u32Depth = pRkVpssHandle->astVpssChnAttr[nChn].nDepth;
        stVpssChnAttr.bFlip = nFlip;
        stVpssChnAttr.bMirror = nMirror;
        stVpssChnAttr.u32FrameBufCnt = pRkVpssHandle->astVpssChnAttr[nChn].nFrameBufCnt;
        /*设置通道参数*/
        nRet = RK_MPI_VPSS_SetChnAttr(nVpssGrp, nChn, &stVpssChnAttr);
        if (nRet != RK_SUCCESS) 
        {
            RK_LOGE("RK_MPI_VPSS_SetChnAttr failed with %#x nChn %d", nRet, nChn);
            return nRet;
        }

        /*使能vpss通道*/
        nRet = RK_MPI_VPSS_EnableChn(nVpssGrp, nChn);
        if (nRet != RK_SUCCESS) {
            RK_LOGE("RK_MPI_VPSS_EnableChn failed with %#x", nRet);
            return nRet;
        }
    }

    if(pRkVpssHandle->stVpssGrpAttr.nEnBackup)
    {
        nRet = RK_MPI_VPSS_EnableBackupFrame(nVpssGrp);
        if (RK_SUCCESS != nRet) {
            RK_LOGE("RK_MPI_VPSS_EnableBackupFrame failed with %#x", nRet);
            return nRet;
        }
    }
    else
    {
        nRet = RK_MPI_VPSS_DisableBackupFrame(nVpssGrp);
        if (RK_SUCCESS != nRet) {
            RK_LOGE("RK_MPI_VPSS_DisableBackupFrame failed with %#x", nRet);
            return nRet;
        }
        
    }

    nRet= RK_MPI_VPSS_SetVProcDev( nVpssGrp, pRkVpssHandle->enDevType);
    if(nRet != RK_SUCCESS)
    {
        RK_LOGE("RK_MPI_VPSS_SetVProcDev(grp:%d) failed with %#x!", nVpssGrp, nRet);
        return nRet;
    }

    /*开启vpss*/
    nRet = RK_MPI_VPSS_StartGrp(nVpssGrp);
    if (nRet != RK_SUCCESS) {
        RK_LOGE("RK_MPI_VPSS_StartGrp failed with %#x", nRet);
        return nRet;
    }

    /*复位vpss*/
    // nRet = RK_MPI_VPSS_ResetGrp(nVpssGrp);
    // if (nRet != RK_SUCCESS) {
    //     RK_LOGE("RK_MPI_VPSS_ResetGrp failed with %#x", nRet);
    //     return nRet;
    // }


    return RK_SUCCESS;
}
/*获取vpss组属性*/
int rockitVpss_get_grpAttr( RkVpss_S* pHandle, RkVpssGrpAttr_S* pGrpAttr )
{
    if( !pHandle || !pGrpAttr )
    {
        return -1;
    }
    memcpy( pGrpAttr, &pHandle->stVpssGrpAttr, sizeof(RkVpssGrpAttr_S) );
    return 0;
}

/*设置vpss组属性*/
int rockitVpss_set_grpAttr( RkVpss_S* pHandle, RkVpssGrpAttr_S* pGrpAttr )
{
    VPSS_GRP_ATTR_S stGrpAttr;
    /*vpss组参数设置*/
    stGrpAttr.u32MaxW = pGrpAttr->nMaxW;
    stGrpAttr.u32MaxH = pGrpAttr->nMaxH;
    /*设置像素格式*/
    stGrpAttr.enPixelFormat = pGrpAttr->enGrpPixelFormat;
    /*vpss压缩*/
    stGrpAttr.enCompressMode = pGrpAttr->enGrpComMode;
    stGrpAttr.stFrameRate.s32SrcFrameRate = pGrpAttr->nSrcFrameRate;
    stGrpAttr.stFrameRate.s32DstFrameRate = pGrpAttr->nDstFrameRate;
    stGrpAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
    RK_MPI_VPSS_SetGrpAttr( pHandle->nVpssGrp, &stGrpAttr);
    if( pGrpAttr->nEnBackup != pHandle->stVpssGrpAttr.nEnBackup )
    {
        if( pGrpAttr->nEnBackup )
        {
            RK_MPI_VPSS_EnableBackupFrame(pHandle->nVpssGrp);
        }
        else
        {
            RK_MPI_VPSS_DisableBackupFrame(pHandle->nVpssGrp);
        }
    }
    memcpy( &pHandle->stVpssGrpAttr, pGrpAttr, sizeof(RkVpssGrpAttr_S) );

    return 0;
}




/*
 * *@description 设置通道属性
 * *@Author: wxz
 * *@param[in] pChnAttr:通道参数
 *              nVpssGrp：vpss组
 *              nVpssChn：设置通道
 * *@return 成功返回0,失败返回-1
 * */
int rockitVpss_set_chnAttr(RkVpss_S *pRkVpssHandle, RkVpssChnAttr_S *pChnAttr, 
        int nVpssGrp, int nVpssChn)
{

    if(!pRkVpssHandle)
    {
        RK_LOGE("rockitVpss_set_chnAttr set chn failed\n");
        return RK_FAILURE;
    }
    if(!pChnAttr)
    {
        RK_LOGE("rockitVpss_set_chnAttr set chn failed\n");
        return RK_FAILURE;
    }

    if(pRkVpssHandle->nVpssGrp != nVpssGrp)
    {
        RK_LOGE("rockitVpss_set_chnAttr vpssGrp no support: nVpssGrp:%d, now:%d\n",
                pRkVpssHandle->nVpssGrp, nVpssGrp);
        return RK_FAILURE;
    }

    if(nVpssChn >= pRkVpssHandle->nVpssChnSum)
    {
        RK_LOGE("rockitVpss_set_chnAttr vpssChn more than max chn\n");
        return RK_FAILURE;
    }

    VPSS_CHN_ATTR_S stVpssChnAttr;
    int nRet = RK_SUCCESS;
    memset(&stVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S));

    nRet = RK_MPI_VPSS_GetChnAttr(nVpssGrp, nVpssChn, &stVpssChnAttr);
    if(RK_SUCCESS != nRet)
    {
        RK_LOGE("RK_MPI_VPSS_GetChnAttr failed with %#x", nRet);
        return nRet;
    }

    
    pRkVpssHandle->astVpssChnAttr[nVpssChn].enChnPixelFormat = pChnAttr->enChnPixelFormat;
    pRkVpssHandle->astVpssChnAttr[nVpssChn].enChnComMode = pChnAttr->enChnComMode;
    pRkVpssHandle->astVpssChnAttr[nVpssChn].enVpssChnMode = pChnAttr->enVpssChnMode;
    pRkVpssHandle->astVpssChnAttr[nVpssChn].nWidth = pChnAttr->nWidth;
    pRkVpssHandle->astVpssChnAttr[nVpssChn].nHeight = pChnAttr->nHeight;
    pRkVpssHandle->astVpssChnAttr[nVpssChn].nDepth = pChnAttr->nDepth;
    pRkVpssHandle->astVpssChnAttr[nVpssChn].nFrameBufCnt = pChnAttr->nFrameBufCnt;
    pRkVpssHandle->astVpssChnAttr[nVpssChn].nSrcFrameRate = pChnAttr->nSrcFrameRate;
    pRkVpssHandle->astVpssChnAttr[nVpssChn].nDstFrameRate = pChnAttr->nDstFrameRate;

    stVpssChnAttr.enPixelFormat = pRkVpssHandle->astVpssChnAttr[nVpssChn].enChnPixelFormat;
    stVpssChnAttr.enCompressMode = pRkVpssHandle->astVpssChnAttr[nVpssChn].enChnComMode;
    stVpssChnAttr.enChnMode = pRkVpssHandle->astVpssChnAttr[nVpssChn].enVpssChnMode;
    stVpssChnAttr.u32Width = pRkVpssHandle->astVpssChnAttr[nVpssChn].nWidth;
    stVpssChnAttr.u32Height = pRkVpssHandle->astVpssChnAttr[nVpssChn].nHeight;
    stVpssChnAttr.u32Depth = pRkVpssHandle->astVpssChnAttr[nVpssChn].nDepth;
    stVpssChnAttr.u32FrameBufCnt = pRkVpssHandle->astVpssChnAttr[nVpssChn].nFrameBufCnt;
    stVpssChnAttr.stFrameRate.s32SrcFrameRate = pRkVpssHandle->astVpssChnAttr[nVpssChn].nSrcFrameRate;
    stVpssChnAttr.stFrameRate.s32DstFrameRate = pRkVpssHandle->astVpssChnAttr[nVpssChn].nDstFrameRate;

    /*设置通道参数*/
    nRet = RK_MPI_VPSS_SetChnAttr(nVpssGrp, nVpssChn, &stVpssChnAttr);
    if (RK_SUCCESS != nRet) 
    {
        RK_LOGE("RK_MPI_VPSS_SetChnAttr failed with %#x", nRet);
        return nRet;
    }

    printf("vpssSetChn : grp:%d, chn:%d, w:%d, h:%d\n",nVpssGrp, nVpssChn, pChnAttr->nWidth, pChnAttr->nHeight);

    return nRet;
}

/*
 * *@description 获取通道属性
 * *@Author: wxz
 * *@param[in] pChnAttr:通道参数
 *              nVpssGrp：vpss组
 *              nVpssChn：设置通道
 * *@return 成功返回0,失败返回-1
 * */
int rockitVpss_get_chnAttr(RkVpss_S *pRkVpssHandle, RkVpssChnAttr_S *pChnAttr, 
        int nVpssGrp, int nVpssChn)
{

    if(!pRkVpssHandle)
    {
        RK_LOGE("rockitVpss_set_chnAttr set chn failed\n");
        return RK_FAILURE;
    }
    if(!pChnAttr)
    {
        RK_LOGE("rockitVpss_set_chnAttr set chn failed\n");
        return RK_FAILURE;
    }

    if(pRkVpssHandle->nVpssGrp != nVpssGrp)
    {
        RK_LOGE("rockitVpss_set_chnAttr vpssGrp no support: nVpssGrp:%d, now:%d\n",
                pRkVpssHandle->nVpssGrp, nVpssGrp);
        return RK_FAILURE;
    }

    if(nVpssChn >= pRkVpssHandle->nVpssChnSum)
    {
        RK_LOGE("rockitVpss_set_chnAttr vpssChn more than max chn\n");
        return RK_FAILURE;
    }

    VPSS_CHN_ATTR_S stVpssChnAttr;
    int nRet = RK_SUCCESS;
    memset(&stVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S));

    nRet = RK_MPI_VPSS_GetChnAttr(nVpssGrp, nVpssChn, &stVpssChnAttr);
    if(RK_SUCCESS != nRet)
    {
        RK_LOGE("RK_MPI_VPSS_GetChnAttr failed with %#x", nRet);
        return nRet;
    }

    pChnAttr->enChnComMode = stVpssChnAttr.enCompressMode;
    pChnAttr->enChnPixelFormat = stVpssChnAttr.enPixelFormat;
    pChnAttr->enVpssChnMode = stVpssChnAttr.enChnMode;
    pChnAttr->nDepth = stVpssChnAttr.u32Depth;
    pChnAttr->nFrameBufCnt = stVpssChnAttr.u32FrameBufCnt;
    pChnAttr->nDstFrameRate = stVpssChnAttr.stFrameRate.s32DstFrameRate;
    pChnAttr->nSrcFrameRate = stVpssChnAttr.stFrameRate.s32SrcFrameRate;
    pChnAttr->nWidth = stVpssChnAttr.u32Width;
    pChnAttr->nHeight = stVpssChnAttr.u32Height;

    // printf("vpssGetChn : grp:%d, chn:%d, w:%d, h:%d\n",nVpssGrp, nVpssChn, pChnAttr->nWidth, pChnAttr->nHeight);

    return nRet;
}

/*设置通道电子放大*/
RK_S32 rockitVpss_set_chnSetZoom( RkVpss_S* pRkVpssHandle, int nVpssChn, int nZoom, int x, int y, RK_BOOL bEnable) 
{
    RK_S32 s32Ret = RK_SUCCESS;
    VPSS_CROP_INFO_S stCropInfo;
    
    if( nZoom <=0 || nZoom > 1000 )
    {
        printf("vpssgrp=%d, nChn=%d, zoom=%d, setZoom有误\n", pRkVpssHandle->nVpssGrp, nVpssChn, nZoom);
    }

    s32Ret = RK_MPI_VPSS_GetChnCrop(pRkVpssHandle->nVpssGrp, nVpssChn, &stCropInfo);
    if (s32Ret != RK_SUCCESS) {
        RK_LOGE("RK_MPI_VPSS_GetChnCrop failed with %#x!", s32Ret);
        return s32Ret;
    }

    stCropInfo.bEnable = bEnable;
    stCropInfo.enCropCoordinate = VPSS_CROP_RATIO_COOR;
    stCropInfo.stCropRect.s32X = 500 - nZoom /2 + x ;
    stCropInfo.stCropRect.s32Y = 500 - nZoom /2 + y;
    stCropInfo.stCropRect.u32Width = nZoom;
    stCropInfo.stCropRect.u32Height = nZoom;
    s32Ret = RK_MPI_VPSS_SetChnCrop(pRkVpssHandle->nVpssGrp, nVpssChn, &stCropInfo);
    if (s32Ret != RK_SUCCESS) {
        RK_LOGE("RK_MPI_VPSS_SetChnCrop failed with grp=%d, nChn=%d %#x!", pRkVpssHandle->nVpssGrp, nVpssChn, s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

/**
 * @brief   : 设置通道裁剪
 * @param    {RkVpss_S} *pRkVpssHandle 句柄
 * @param    {VPSS_CROP_INFO_S} *pCropInfo 裁剪功能信息指针
 * @param    {int} nVpssChn 设置的通道
 * @return   {int} 成功返回0,失败返回-1
 */
RK_S32 rockitVpss_set_chnCrop(RkVpss_S *pRkVpssHandle, VPSS_CROP_INFO_S *pCropInfo, int nVpssChn)
{
    if (!pRkVpssHandle || !pCropInfo)
    {
        mpi_vpss_log("指针为空");
        return RK_FAILURE;
    }

    if (nVpssChn >= pRkVpssHandle->nVpssChnSum)
    {
        RK_LOGE("设置的通道数大于最大通道数");
        return RK_FAILURE;
    }

    if (pCropInfo->stCropRect.s32X >= pRkVpssHandle->stVpssGrpAttr.nMaxW
        || pCropInfo->stCropRect.s32Y >= pRkVpssHandle->stVpssGrpAttr.nMaxH
        || pCropInfo->stCropRect.u32Width > (RK_U32) pRkVpssHandle->stVpssGrpAttr.nMaxW
        || pCropInfo->stCropRect.u32Height > (RK_U32) pRkVpssHandle->stVpssGrpAttr.nMaxH
        || pCropInfo->stCropRect.s32X + pCropInfo->stCropRect.u32Width > (RK_U32) pRkVpssHandle->stVpssGrpAttr.nMaxW
        || pCropInfo->stCropRect.s32Y + pCropInfo->stCropRect.u32Height > (RK_U32) pRkVpssHandle->stVpssGrpAttr.nMaxH)
    {
        mpi_vpss_log("设置的区域大小不合法");
        return RK_FAILURE;
    }

    /*组*/
    int nVpssGrp = pRkVpssHandle->nVpssGrp;

    CHECK_API_RETURN(RK_MPI_VPSS_SetChnCrop(nVpssGrp, nVpssChn, pCropInfo));

    mpi_vpss_log("vpssSetChnCrop : grp: %d, chn: %d, enable: %d, crop_mode :%d, crop_rect: [%d,%d][%d,%d]",
                 nVpssGrp,
                 nVpssChn,
                 pCropInfo->bEnable,
                 pCropInfo->enCropCoordinate,
                 pCropInfo->stCropRect.s32X,
                 pCropInfo->stCropRect.s32Y,
                 pCropInfo->stCropRect.u32Width,
                 pCropInfo->stCropRect.u32Height);

    return RK_SUCCESS;
}

/*
 * *@description 手动获取vpss通道数据
 * *@Author: wxz
 * *@param[in]  pRkVpssHandle: Vpss句柄
 *              pstVideFrame:数据帧
 *              nVpssChn:Vpss通道
 *              nMilliSec:  -1为阻塞
 *                          0为非阻塞
 *                          大于0为等待时间，ms
 * *@return 成功返回0,失败返回-1
 * */
int rockitVpss_get_chnFrame(RkVpss_S *pRkVpssHandle, VIDEO_FRAME_INFO_S *pstVideFrame, 
        int nVpssChn, int nMilliSec)
{
    if(!pRkVpssHandle)
    {
        return RK_FAILURE;
    }

    if(nVpssChn >= pRkVpssHandle->nVpssChnSum)
    {
        printf("vpssgrp:%d chn:%d moreThan ChnSum:%d\n",
                pRkVpssHandle->nVpssGrp, nVpssChn, pRkVpssHandle->nVpssChnSum);
        return RK_FAILURE;
    }

    int nRet = RK_SUCCESS;
    nRet = RK_MPI_VPSS_GetChnFrame(pRkVpssHandle->nVpssGrp, nVpssChn, pstVideFrame, nMilliSec);
    if(RK_SUCCESS != nRet)
    {
        RK_LOGE("RK_MPI_VPSS_GetChnFrame grp:%d chn:%d failed with %#x", 
                pRkVpssHandle->nVpssGrp,nVpssChn, nRet);
        return nRet;
    }

    return nRet;
}

/*
 * *@description 通道数据释放
 * *@Author: wxz
 * *@param[in]  pRkVpssHandle: Vpss句柄
 *              pstVideFrame:数据帧
 *              nVpssChn:Vpss通道
 * *@return 成功返回0,失败返回-1
 * */
int rockitVpss_release_chnFrame(RkVpss_S *pRkVpssHandle, VIDEO_FRAME_INFO_S *pstVideFrame, 
        int nVpssChn)
{
    if(NULL == pRkVpssHandle)
    {
        return RK_FAILURE;
    }

    if(NULL == pstVideFrame)
    {
        return RK_FAILURE;
    }

    if(nVpssChn >= pRkVpssHandle->nVpssChnSum)
    {
        printf("vpssgrp:%d chn:%d moreThan ChnSum:%d\n",
                pRkVpssHandle->nVpssGrp, nVpssChn, pRkVpssHandle->nVpssChnSum);
        return RK_FAILURE;
    }

    int nRet = RK_MPI_VPSS_ReleaseChnFrame(pRkVpssHandle->nVpssGrp, nVpssChn, pstVideFrame);
    if(RK_SUCCESS != nRet)
    {
        printf("vpss releaseChnFream error:grp:%d,chn:%d\n",pRkVpssHandle->nVpssGrp, nVpssChn);
        return nRet;
    }

    return nRet;
}


/*
 * *@description 获取vpss通道帧数据缓存区和缓存区大小
 * *@Author: wxz
 * *@param[in]  pRkVpssHandle: Vpss句柄
 *              pstVideFrame:数据帧
 *              data :     缓存区
 *              int size:  缓存区大小
 * *@return 成功返回0,失败返回-1
 * */
int rockitVpss_get_chnFrameData(RkVpss_S *pRkVpssHandle, VIDEO_FRAME_INFO_S *pstVideFrame, 
        unsigned char ** data, int* size)
{
    if(!pRkVpssHandle)
    {
        return RK_FAILURE;
    }
    int s32Ret; 

    PIC_BUF_ATTR_S stPicBufAttr;
	MB_PIC_CAL_S stMbPicCalResult;

    /*获取虚拟地址*/
    *data = RK_MPI_MB_Handle2VirAddr(pstVideFrame->stVFrame.pMbBlk);
    /*获取大小*/
	stPicBufAttr.u32Width = pstVideFrame->stVFrame.u32VirWidth;
	stPicBufAttr.u32Height = pstVideFrame->stVFrame.u32VirHeight;
	stPicBufAttr.enPixelFormat = pstVideFrame->stVFrame.enPixelFormat;
	stPicBufAttr.enCompMode = pstVideFrame->stVFrame.enCompressMode;
	s32Ret = RK_MPI_CAL_VGS_GetPicBufferSize(&stPicBufAttr, &stMbPicCalResult);
	if (s32Ret != RK_SUCCESS) {
		RK_LOGE("RK_MPI_CAL_VGS_GetPicBufferSize failed. err=0x%x", s32Ret);
		return s32Ret;
	}

    RK_MPI_SYS_MmzFlushCache(pstVideFrame->stVFrame.pMbBlk, RK_TRUE);

    // printf("stMbPicCalResult.u32MBSize:%d\n",stMbPicCalResult.u32MBSize);
    *size = stMbPicCalResult.u32MBSize;

    return s32Ret;
}

/*
 * *@description 手动往vpss发送图像
 * *@Author: wxz
 * *@param[in]  pRkVpssHandle: Vpss句柄
 *              pstVideFrame:数据帧
 *              nMilliSec:  -1为阻塞等待
 *                          0为非阻塞
 *                          大于0为等待时间，ms
 * *@return 成功返回0,失败返回-1
 * */
int rockitVpss_send_frame(RkVpss_S *pRkVpssHandle, VIDEO_FRAME_INFO_S *pstVideFrame, int nMilliSec)
{
    if(!pRkVpssHandle)
    {
        return RK_FAILURE;
    }

    if(!pstVideFrame)
    {
        return RK_FAILURE;
    }

    int nRet = RK_SUCCESS;
    nRet = RK_MPI_VPSS_SendFrame(pRkVpssHandle->nVpssGrp, VPSS_GRP_PIPE, pstVideFrame, nMilliSec);
    if(RK_SUCCESS != nRet)
    {
        RK_LOGE("RK_MPI_VPSS_sendFrame %d failed with %#x", pRkVpssHandle->nVpssGrp, nRet);
        return nRet;
    }

    return nRet;
}


/*
     * *@description vpss获取通道号fd
     * *@Author: fhs
     * *@param[in] pRkVpssHandle: Vpss句柄
     *             nVpssGrp：vpss组
     *             nVpssChn：vpss通道
     * *@return 成功返回0,失败返回-1
     * */
int rockit_get_chnfd(RkVpss_S *pRkVpssHandle,int nVpssGrp, int nVpssChn)
{
    if(!pRkVpssHandle)
    {
        return RK_FAILURE;
    }

    int nRet = RK_SUCCESS;
    nRet = RK_MPI_VPSS_GetChnFd(nVpssGrp,nVpssChn);

    return nRet;
}


/*
     * *@description vpss获取通道号fd
     * *@Author: fhs
     * *@param[in] pRkVpssHandle: Vpss句柄
     *             nVpssGrp：vpss组
     *             nVpssChn：vpss通道
     * *@return 成功返回0,失败返回-1
     * */
int rockit_close_chnfd(RkVpss_S *pRkVpssHandle,int nVpssGrp, int nVpssChn)
{
    if(!pRkVpssHandle)
    {
        return RK_FAILURE;
    }

    int nRet = RK_SUCCESS;
    nRet = RK_MPI_VPSS_CloseFd(nVpssGrp,nVpssChn);

    return nRet;
}


/*                                            
 * *@description vpss放初始化                 
 * *@Author: wxz                              
 * *@param[in]                                
 * *@return 成功返回0,失败返回-1
 * */
int rockit_vpss_uninit(RkVpss_S *pRkVpssHandle)
{
    int nRet;
    int nVpssChn;
    for(nVpssChn=0; nVpssChn<pRkVpssHandle->nVpssChnSum; nVpssChn++)
    {
        nRet = RK_MPI_VPSS_DisableChn(pRkVpssHandle->nVpssGrp, nVpssChn);
        if (RK_SUCCESS != nRet) {
            RK_LOGE("RK_MPI_VPSS_DisableChn %d failed with %#x", nVpssChn, nRet);
            return nRet;
        }
    }

    nRet = RK_MPI_VPSS_StopGrp(pRkVpssHandle->nVpssGrp);
    if (RK_SUCCESS != nRet) {
        RK_LOGE("RK_MPI_VPSS_StopGrp %d failed with %#x", pRkVpssHandle->nVpssGrp, nRet);
        return nRet;
    }

    nRet = RK_MPI_VPSS_DestroyGrp(pRkVpssHandle->nVpssGrp);
    if (RK_SUCCESS != nRet) {
        RK_LOGE("RK_MPI_VPSS_DestroyGrp %d failed with %#x", pRkVpssHandle->nVpssGrp, nRet);
        return nRet;
    }


    return RK_SUCCESS;
}


/*
 * *@description 申请RkVpss句柄
 * *@Author: wxz
 * *@param[in] 
 * *@return RkVpss_S
 * */
RkVpss_S *rockit_vpss_alloc(RkVpssNeedParam_S stNeedParam)
{
    RkVpss_S *pVpssHdle = (RkVpss_S*)malloc(sizeof(RkVpss_S));
    RkVpssChnAttr_S *pVpssChnAttr=NULL, *pVpssChnAttrSrc=NULL;

    memset(pVpssHdle, 0, sizeof(RkVpss_S));
    pVpssHdle->nVpssChnSum = stNeedParam.nVpssChnSum;
    pVpssHdle->nVpssGrp = stNeedParam.nVpssGrp;
    pVpssHdle->stVpssGrpAttr.enGrpPixelFormat = stNeedParam.stVpssGrpAttr.enGrpPixelFormat;
    pVpssHdle->stVpssGrpAttr.enGrpComMode = stNeedParam.stVpssGrpAttr.enGrpComMode;
    pVpssHdle->stVpssGrpAttr.nMaxW = stNeedParam.stVpssGrpAttr.nMaxW;
    pVpssHdle->stVpssGrpAttr.nMaxH = stNeedParam.stVpssGrpAttr.nMaxH;
    pVpssHdle->stVpssGrpAttr.nSrcFrameRate = stNeedParam.stVpssGrpAttr.nSrcFrameRate;
    pVpssHdle->stVpssGrpAttr.nDstFrameRate = stNeedParam.stVpssGrpAttr.nDstFrameRate;
    pVpssHdle->stVpssGrpAttr.nEnBackup = stNeedParam.stVpssGrpAttr.nEnBackup;

    for(int nVpssChn=0; nVpssChn<VPSSCHNMAX; nVpssChn++)
    {
        pVpssChnAttr = &pVpssHdle->astVpssChnAttr[nVpssChn];
        pVpssChnAttrSrc = &stNeedParam.astVpssChnAttr[nVpssChn];
        pVpssChnAttr->enChnPixelFormat = pVpssChnAttrSrc->enChnPixelFormat;
        pVpssChnAttr->enVpssChnMode = pVpssChnAttrSrc->enVpssChnMode;
        pVpssChnAttr->enChnComMode = pVpssChnAttrSrc->enChnComMode;
        pVpssChnAttr->nWidth = pVpssChnAttrSrc->nWidth;
        pVpssChnAttr->nHeight = pVpssChnAttrSrc->nHeight;
        pVpssChnAttr->nDepth = pVpssChnAttrSrc->nDepth;
        pVpssChnAttr->nFrameBufCnt = pVpssChnAttrSrc->nFrameBufCnt;
        pVpssChnAttr->nSrcFrameRate = pVpssChnAttrSrc->nSrcFrameRate;
        pVpssChnAttr->nDstFrameRate = pVpssChnAttrSrc->nDstFrameRate;
    }
    //pVpssHdle->enDevType = VIDEO_PROC_DEV_RGA;
    pVpssHdle->enDevType = VIDEO_PROC_DEV_VPSS;
    pVpssHdle->rockit_vpss_init = rockit_vpss_init;
    pVpssHdle->rockitVpss_get_grpAttr = rockitVpss_get_grpAttr;
    pVpssHdle->rockitVpss_set_grpAttr = rockitVpss_set_grpAttr;
    pVpssHdle->rockitVpss_set_chnSetZoom = rockitVpss_set_chnSetZoom;
    pVpssHdle->rockitVpss_set_chnCrop  = rockitVpss_set_chnCrop;
    pVpssHdle->rockitVpss_set_chnAttr = rockitVpss_set_chnAttr;
    pVpssHdle->rockitVpss_get_chnAttr = rockitVpss_get_chnAttr;
    pVpssHdle->rockit_vpss_uninit = rockit_vpss_uninit;
    pVpssHdle->rockitVpss_get_chnFrame = rockitVpss_get_chnFrame;
    pVpssHdle->rockitVpss_get_chnFrameData = rockitVpss_get_chnFrameData;
    pVpssHdle->rockitVpss_release_chnFrame = rockitVpss_release_chnFrame;
    pVpssHdle->rockitVpss_send_frame = rockitVpss_send_frame;
    pVpssHdle->rockit_get_chnfd = rockit_get_chnfd;
    pVpssHdle->rockit_close_chnfd = rockit_close_chnfd;


    return pVpssHdle;
}

/*
 * *@description 注销RkVpss句柄
 * *@Author: wxz
 * *@param[in] 
 * *@return 成功返回0,失败返回-1
 * */
void rockit_vpss_release(RkVpss_S* pVpssHdle)
{
    if(!pVpssHdle)
    {
        free(pVpssHdle);
        pVpssHdle = NULL;
    }
}

