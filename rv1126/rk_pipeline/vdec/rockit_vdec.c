/*************************************************************************
	> File Name: rockit_vdec.c
	> Author:luoyk 
	> Mail: 
	> Created Time: Fri 06 May 2022 09:37:27 AM CST
 ************************************************************************/
#include "rockit_vdec.h"
#include<stdio.h>


static int rockitVdec_init(RkVdec_S *pHandle);

/*
 * 当pFreeFunCB为空时，解码会进行一次拷贝，用户要自己手动释放data
 * 当pFreeFunCB不为空是，解码仅引用data，内部会自行调用释放回调
 * 为避免出现 RK_ERR_VDEC_BUF_FULL 错误，建议将超时时间设置为通道帧间隔的4倍
 */
static int rockitVdec_send_stream(RkMediaData_S *pMediaData)
{
    MB_EXT_CONFIG_S pstMbExtConfig;
    VDEC_STREAM_S stStream;
    RK_S32 nRet     = RK_SUCCESS;
    int nSendCount  =0;
    MB_BLK buffer   = RK_NULL;

    memset(&pstMbExtConfig, 0, sizeof(MB_EXT_CONFIG_S));
    memset(&stStream, 0, sizeof(VDEC_STREAM_S));

    pstMbExtConfig.pFreeCB      = pMediaData->pFreeFunCB;
    pstMbExtConfig.pOpaque      = pMediaData->pData;
    pstMbExtConfig.pu8VirAddr   = pMediaData->pData;
    pstMbExtConfig.u64Size      = pMediaData->nSize;
    
    RK_MPI_SYS_CreateMB(&buffer, &pstMbExtConfig);
    
    stStream.u64PTS         = pMediaData->nPts;
    stStream.pMbBlk         = buffer;
    stStream.u32Len         = pMediaData->nSize;
    stStream.bEndOfStream   = pMediaData->bEos;
    stStream.bEndOfFrame    = pMediaData->bEos;
    
    if(pMediaData->pFreeFunCB)
    {
        /*直通模式*/
        stStream.bBypassMbBlk = RK_TRUE;
    }
    else
    {
        /*拷贝模式*/
        stStream.bBypassMbBlk = RK_FALSE;
    }

    nRet = RK_MPI_VDEC_SendStream(pMediaData->pHandle->stNeedParam.nChnIndex, &stStream, pMediaData->nTimeMs);
    RK_MPI_MB_ReleaseMB(stStream.pMbBlk);
    return nRet;

}

/*获取解码后的缓存块*/
static int rockitVdec_get_frame(RkVdec_S* pHandle, VIDEO_FRAME_INFO_S *pstFrame, int nTimeout)
{
    RK_S32 nRet = RK_SUCCESS;
    
    nRet = RK_MPI_VDEC_GetFrame(pHandle->stNeedParam.nChnIndex, pstFrame, nTimeout);
    
    if (nRet == 0) 
    {
        if ((pstFrame->stVFrame.u32FrameFlag & FRAME_FLAG_SNAP_END) == FRAME_FLAG_SNAP_END) 
        {
            RK_MPI_VDEC_ReleaseFrame(pHandle->stNeedParam.nChnIndex, pstFrame);
            printf("chn %d reach eos frame.", pHandle->stNeedParam.nChnIndex);
        }   
    }
    return nRet;
}

/*获取缓存块的虚拟地址*/
static int rockitVdec_get_frameVir(VIDEO_FRAME_INFO_S* pstFrame, uint8_t** pData, int nSize)
{
    RK_U32 width    = 0;
    RK_U32 height   = 0;
    RK_U32 h_stride = 0;
    RK_U32 v_stride = 0;
    
    *pData = (RK_U8 *)RK_MPI_MB_Handle2VirAddr(pstFrame->stVFrame.pMbBlk);
    RK_MPI_SYS_MmzFlushCache(pstFrame->stVFrame.pMbBlk, RK_TRUE);
    width = pstFrame->stVFrame.u32Width;
    height = pstFrame->stVFrame.u32Height;
    return 0;
}
/*释放解码后的缓冲块*/
static int rockitVdec_release_frame(RkVdec_S* pHandle, VIDEO_FRAME_INFO_S* pstFrame)
{
    RK_MPI_VDEC_ReleaseFrame(pHandle->stNeedParam.nChnIndex, pstFrame);
    return 0;
}

/* 重置解码器通道队列数据 */
static int rockitVdec_reset_vdecChn(RkVdec_S* pHandle)
{
    RK_MPI_VDEC_ResetChn(pHandle->stNeedParam.nChnIndex);
    return 0;
}

/*获取解码通道属性*/
static int rockitVdec_get_chnAttr( RkVdec_S* pHandle, VDEC_CHN_ATTR_S* pstAttr, VDEC_CHN_PARAM_S *pstParam )
{
    if( pstAttr != NULL )
    {
        RK_MPI_VDEC_GetChnAttr( pHandle->stNeedParam.nChnIndex, pstAttr);
    }
    if( pstParam != NULL )
    {
        RK_MPI_VDEC_GetChnParam( pHandle->stNeedParam.nChnIndex, pstParam);
    }
    return 0;
}

/*改变解码通道属性*/
static int rockitVdec_set_chnAttr( RkVdec_S* pHandle, VDEC_CHN_ATTR_S* pstAttr, VDEC_CHN_PARAM_S *pstParam )
{
    int nRet = RK_SUCCESS;
    
    nRet = RK_MPI_VDEC_StopRecvStream(pHandle->stNeedParam.nChnIndex);
    if (nRet != RK_SUCCESS) 
    {
        printf("stop recv chn %d failed %x! ", pHandle->stNeedParam.nChnIndex, nRet);
        return nRet;
    }
    if( pstAttr != NULL )
    {
        nRet = RK_MPI_VDEC_SetChnAttr( pHandle->stNeedParam.nChnIndex, pstAttr);
        if (nRet != RK_SUCCESS) 
        {
            printf("set recv chn %d attr failed %x! ", pHandle->stNeedParam.nChnIndex, nRet);
            return nRet;
        }
    }
    if( pstParam != NULL )
    {
        nRet = RK_MPI_VDEC_SetChnParam( pHandle->stNeedParam.nChnIndex, pstParam);
        if (nRet != RK_SUCCESS) 
        {
            printf("set recv chn %d param attr failed %x! ", pHandle->stNeedParam.nChnIndex, nRet);
            return nRet;
        }
    }
    RK_MPI_VDEC_ResetChn(pHandle->stNeedParam.nChnIndex);   
    nRet = RK_MPI_VDEC_StartRecvStream(pHandle->stNeedParam.nChnIndex);
    if (nRet != RK_SUCCESS) 
    {
        if(nRet == 0xa0058010)
        {
            nRet = rockitVdec_init(pHandle);
            printf("re init vdec\n");
        }
        if (nRet != RK_SUCCESS) 
            printf("start recv chn %d failed %x! ", pHandle->stNeedParam.nChnIndex, nRet);
        return nRet;
    }
    return nRet;
}

/*改变解码通道宽高*/
static int rockitVdec_change_wh( RkVdec_S* pHandle, unsigned int unWidth, unsigned int unHeight)
{
    int nRet = RK_SUCCESS;
    VDEC_CHN_ATTR_S stAttr;

    memset( &stAttr, 0, sizeof(VDEC_CHN_ATTR_S) );
    
    nRet = RK_MPI_VDEC_StopRecvStream(pHandle->stNeedParam.nChnIndex);
    if (nRet != RK_SUCCESS) 
    {
        printf("stop recv chn %d failed %x! ", pHandle->stNeedParam.nChnIndex, nRet);
        return nRet;
    }
    
    nRet = RK_MPI_VDEC_GetChnAttr( pHandle->stNeedParam.nChnIndex, &stAttr);
    if (nRet != RK_SUCCESS) 
    {
        printf("get recv chn attr %d failed %x! ", pHandle->stNeedParam.nChnIndex, nRet);
        return nRet;
    }
    
    stAttr.u32PicWidth = unWidth; 
    stAttr.u32PicHeight =unHeight;
    
    nRet = RK_MPI_VDEC_SetChnAttr( pHandle->stNeedParam.nChnIndex, &stAttr);
    if (nRet != RK_SUCCESS) 
    {
        printf("set recv chn %d attr failed %x! ", pHandle->stNeedParam.nChnIndex, nRet);
        return nRet;
    }
    
    nRet = RK_MPI_VDEC_StartRecvStream(pHandle->stNeedParam.nChnIndex);
    if (nRet != RK_SUCCESS) 
    {
        printf("start recv chn %d failed %x! ", pHandle->stNeedParam.nChnIndex, nRet);
        return nRet;
    }
    return nRet;
}

/*改变解码通道的解码格式*/
static int rockitVdec_change_codecId( RkVdec_S* pHandle, RK_CODEC_ID_E eCodecId )
{  
    int nRet = RK_SUCCESS;
    VDEC_CHN_ATTR_S stAttr;
    VDEC_CHN_PARAM_S stParam;

    memset( &stAttr, 0, sizeof(VDEC_CHN_ATTR_S) );
    memset( &stParam, 0, sizeof(VDEC_CHN_PARAM_S) );
    
    RK_MPI_VDEC_StopRecvStream(pHandle->stNeedParam.nChnIndex);
    RK_MPI_VDEC_GetChnAttr( pHandle->stNeedParam.nChnIndex, &stAttr);
    RK_MPI_VDEC_GetChnParam(pHandle->stNeedParam.nChnIndex, &stParam);
    
    stAttr.enType = eCodecId; 
    stParam.enType = eCodecId;
    
    RK_MPI_VDEC_SetChnAttr( pHandle->stNeedParam.nChnIndex, &stAttr);
    RK_MPI_VDEC_SetChnParam(pHandle->stNeedParam.nChnIndex, &stParam);
    nRet = RK_MPI_VDEC_StartRecvStream(pHandle->stNeedParam.nChnIndex);
    return nRet;
}

/*设置插入的图片*/
static int rockitVdec_set_userPic( RkVdec_S* pHandle, UserPic_S* pUserPic  )
{
    int nRet = 0;
    PIC_BUF_ATTR_S stPicBufAttr;
    MB_PIC_CAL_S stMbPicCalResult;
    VIDEO_FRAME_INFO_S   stFrame;
    MB_BLK blk;
    uint8_t* pData =NULL;

    memset(&stFrame, 0, sizeof(VIDEO_FRAME_INFO_S));


    /*禁止用户图片*/
    nRet = RK_MPI_VDEC_DisableUserPic( pHandle->stNeedParam.nChnIndex );
    if ( nRet != RK_SUCCESS )
    {
        printf( "vdec disable user pic fail Ox%x\n", nRet );
        return nRet;
    }

    stPicBufAttr.u32Width       = pUserPic->unWidth;
    stPicBufAttr.u32Height      = pUserPic->unHeight;
    stPicBufAttr.enPixelFormat  = pUserPic->ePixFmt;
    stPicBufAttr.enCompMode     = COMPRESS_MODE_NONE;
 
    nRet = RK_MPI_CAL_TDE_GetPicBufferSize(&stPicBufAttr, &stMbPicCalResult);
    if ( nRet != RK_SUCCESS )
    {
        printf( "vdec get pic size  fail Ox%x\n", nRet );
        return nRet;
    }
    
    nRet = RK_MPI_SYS_MmzAlloc( &blk, RK_NULL, RK_NULL, stMbPicCalResult.u32MBSize);
    if ( nRet != RK_SUCCESS )
    {
        printf( "vdec pic malloc pic bufer fail Ox%x\n", nRet );
        return nRet;
    }
    pData = (uint8_t*)RK_MPI_MB_Handle2VirAddr( blk );
    
    /*填充数据*/
    memcpy( pData, pUserPic->pData, stMbPicCalResult.u32MBSize );

	RK_MPI_SYS_MmzFlushCache(blk, RK_FALSE);
    stFrame.stVFrame.pMbBlk = blk;
    stFrame.stVFrame.u32Width = pUserPic->unWidth;
    stFrame.stVFrame.u32Height = pUserPic->unHeight;
    stFrame.stVFrame.u32VirWidth = pUserPic->unWidth;
    stFrame.stVFrame.u32VirHeight = pUserPic->unHeight;
    stFrame.stVFrame.enPixelFormat = pUserPic->ePixFmt;
    stFrame.stVFrame.u32FrameFlag |= 0;
    stFrame.stVFrame.enCompressMode = COMPRESS_MODE_NONE;

    /*设置用户图片*/
    nRet = RK_MPI_VDEC_SetUserPic( pHandle->stNeedParam.nChnIndex, &stFrame);
    if( nRet != RK_SUCCESS )
    {
        RK_MPI_SYS_MmzFree( blk );
    }
    return nRet;
}

/*显示插入用户图片
*inparam pHandle 句柄
*inparam bInstant 1 立即插入， 0 延迟插入
* */
static int rockitVdec_show_userPic( RkVdec_S* pHandle, RK_BOOL bInstant )
{
    RK_MPI_VDEC_StopRecvStream( pHandle->stNeedParam.nChnIndex );
    return RK_MPI_VDEC_EnableUserPic( pHandle->stNeedParam.nChnIndex, bInstant);
}
static int rockitVdec_disShow_userPic( RkVdec_S* pHandle )
{
    int nRet = RK_MPI_VDEC_DisableUserPic( pHandle->stNeedParam.nChnIndex);
    RK_MPI_VDEC_StartRecvStream( pHandle->stNeedParam.nChnIndex );
    return nRet;
}

static int rockitVdec_init(RkVdec_S *pHandle)
{
    RK_S32 nRet = RK_SUCCESS;
    /*解码通道属性*/
    VDEC_CHN_ATTR_S stAttr;
    VDEC_CHN_PARAM_S stVdecParam;
    MB_POOL_CONFIG_S stMbPoolCfg;
    VDEC_PIC_BUF_ATTR_S stVdecPicBufAttr;
    MB_PIC_CAL_S stMbPicCalResult;
    VDEC_MOD_PARAM_S stModParam;
    VIDEO_MODE_E enMode;

    memset(&stAttr, 0, sizeof(VDEC_CHN_ATTR_S));
    memset(&stVdecParam, 0, sizeof(VDEC_CHN_PARAM_S));
    memset(&stMbPoolCfg, 0, sizeof(MB_POOL_CONFIG_S));
    memset(&stVdecPicBufAttr, 0, sizeof(VDEC_PIC_BUF_ATTR_S));
    memset(&stMbPicCalResult, 0, sizeof(MB_PIC_CAL_S));
    memset(&stModParam, 0, sizeof(VDEC_MOD_PARAM_S));
    
    /*jpeg和mjpeg的解码流的输入模式只能按帧发送*/
    if (pHandle->stNeedParam.enCodecId == RK_VIDEO_ID_MJPEG || pHandle->stNeedParam.enCodecId == RK_VIDEO_ID_JPEG) 
    {
        enMode = VIDEO_MODE_FRAME;
    } 
    else 
    {
        enMode = pHandle->stNeedParam.eInputMode;
    }

    /*先判断解码通道内存是不是使用用户模式*/
    if(pHandle->stExParam.bEnableMbPool) 
    {
        stModParam.enVdecMBSource = MB_SOURCE_USER;
    }
    else
    {
        stModParam.enVdecMBSource = MB_SOURCE_PRIVATE;
    }
    nRet = RK_MPI_VDEC_SetModParam(&stModParam);
    if (nRet != RK_SUCCESS) 
    {
        printf("vdec %d SetModParam failed! errcode %x", pHandle->stNeedParam.nChnIndex, nRet);
        return nRet;
    }

    stVdecPicBufAttr.enCodecType                = pHandle->stNeedParam.enCodecId;
    stVdecPicBufAttr.stPicBufAttr.u32Width      = pHandle->stNeedParam.unSrcWidth;
    stVdecPicBufAttr.stPicBufAttr.u32Height     = pHandle->stNeedParam.unSrcHeight;
    stVdecPicBufAttr.stPicBufAttr.enPixelFormat = pHandle->stNeedParam.eOutputPixFmt;
    stVdecPicBufAttr.stPicBufAttr.enPixelFormat = pHandle->stNeedParam.eOutputPixFmt;
    stVdecPicBufAttr.stPicBufAttr.enCompMode    = pHandle->stNeedParam.eCompressMode;
    
    nRet = RK_MPI_CAL_VDEC_GetPicBufferSize(&stVdecPicBufAttr, &stMbPicCalResult);
    
    if (nRet != RK_SUCCESS) 
    {
        printf("get picture buffer size failed. err 0x%x", nRet);
        return nRet;
    }

    stAttr.enMode           = enMode;
    stAttr.enType           = pHandle->stNeedParam.enCodecId;
    stAttr.u32PicWidth      = pHandle->stNeedParam.unSrcWidth;
    stAttr.u32PicHeight     = pHandle->stNeedParam.unSrcHeight;
    stAttr.u32PicVirWidth   = pHandle->stNeedParam.unSrcWidth;
    stAttr.u32PicVirHeight  = pHandle->stNeedParam.unSrcHeight;
    /*解码所需的内存块个数*/
    stAttr.u32FrameBufCnt   = pHandle->stExParam.unFrameBufferCnt;
    /*发送码流缓冲区存储的码流包个数*/
    stAttr.u32StreamBufCnt  = pHandle->stExParam.unSendBufferCnt;
    stAttr.u32FrameBufSize  = stMbPicCalResult.u32MBSize;
    
    if(!pHandle->stExParam.bEnableColmv) 
    {
        stAttr.stVdecVideoAttr.bTemporalMvpEnable = RK_FALSE;
    }
    
    /*创建解码通道*/
    nRet = RK_MPI_VDEC_CreateChn(pHandle->stNeedParam.nChnIndex, &stAttr);
    if (nRet != RK_SUCCESS) 
    {
        printf("create %d vdec failed! ", pHandle->stNeedParam.nChnIndex);
        return nRet;
    }

    stVdecParam.enType = pHandle->stNeedParam.enCodecId;
    /*设置通道参数*/
    if (pHandle->stNeedParam.enCodecId == RK_VIDEO_ID_MJPEG || pHandle->stNeedParam.enCodecId == RK_VIDEO_ID_JPEG)
    {
        stVdecParam.stVdecPictureParam.enPixelFormat = (PIXEL_FORMAT_E)pHandle->stNeedParam.eOutputPixFmt;
    } 
    else 
    {
        stVdecParam.stVdecVideoParam.enCompressMode = (COMPRESS_MODE_E)pHandle->stNeedParam.eCompressMode;
    }

    if (pHandle->stExParam.bEnableDei) 
    {
        stVdecParam.stVdecVideoParam.bDeiEn = RK_TRUE;
    }

    if (!pHandle->stExParam.bEnableColmv) 
    {
        stVdecParam.stVdecVideoParam.enOutputOrder = VIDEO_OUTPUT_ORDER_DEC;
    }

    nRet = RK_MPI_VDEC_SetChnParam(pHandle->stNeedParam.nChnIndex, &stVdecParam);
    if (nRet != RK_SUCCESS) 
    {
        printf("set chn %d param failed %x! ", pHandle->stNeedParam.nChnIndex, nRet);
        return nRet;
    }

    pHandle->nChnFd = RK_MPI_VDEC_GetFd(pHandle->stNeedParam.nChnIndex);
    if (pHandle->nChnFd <= 0) {
            RK_LOGE("get fd chn %d failed %d", pHandle->stNeedParam.nChnIndex, pHandle->nChnFd);
            return nRet;
            
    }   

    if (pHandle->stExParam.bEnableMbPool) 
    {
        /*用户模式创建内存池*/
        memset(&stMbPoolCfg, 0, sizeof(MB_POOL_CONFIG_S));

        stMbPoolCfg.u64MBSize   = stMbPicCalResult.u32MBSize;;
        stMbPoolCfg.u32MBCnt    = 10;
        stMbPoolCfg.enRemapMode = MB_REMAP_MODE_CACHED;
        stMbPoolCfg.bPreAlloc   = RK_TRUE;
        
        pHandle->nPool = RK_MPI_MB_CreatePool(&stMbPoolCfg);
        if (pHandle->nPool == MB_INVALID_POOLID) 
        {
            printf("create pool failed!");
            return nRet;
        }
        
        /*内存池绑定到解码通道*/
        nRet = RK_MPI_VDEC_AttachMbPool(pHandle->stNeedParam.nChnIndex, pHandle->nPool);
        if (nRet != RK_SUCCESS) 
        {
            printf("attatc vdec mb pool %d failed! ", pHandle->stNeedParam.nChnIndex);
            return nRet;
        }
    }
    
    /*开始接受解码码流*/
    nRet = RK_MPI_VDEC_StartRecvStream(pHandle->stNeedParam.nChnIndex);
    if (nRet != RK_SUCCESS) 
    {
        printf("start recv chn %d failed %x! ", pHandle->stNeedParam.nChnIndex, nRet);
        return nRet;
    }
    
    /*设置显示模式，只有作为src bind后才有用*/
    RK_MPI_VDEC_SetDisplayMode(pHandle->stNeedParam.nChnIndex, pHandle->stExParam.eDisPlayMode);
    return RK_SUCCESS;
}

static int rockitVdec_uninit(RkVdec_S *pHandle)
{
    RK_MPI_VDEC_StopRecvStream(pHandle->stNeedParam.nChnIndex);
    RK_MPI_VDEC_ResetChn(pHandle->stNeedParam.nChnIndex);   

    if (pHandle->nChnFd > 0) 
    {
        RK_MPI_VDEC_CloseFd(pHandle->nChnFd);
    }   
    
    if (pHandle->stExParam.bEnableMbPool) 
    {
        RK_MPI_VDEC_DetachMbPool(pHandle->stNeedParam.nChnIndex);
        RK_MPI_VDEC_DestroyChn(pHandle->stNeedParam.nChnIndex);
        RK_MPI_MB_DestroyPool(pHandle->nPool);
    } 
    else 
    {
        RK_MPI_VDEC_DestroyChn(pHandle->stNeedParam.nChnIndex);
    }   
    return RK_SUCCESS;
}

RkVdec_S* rockitVdec_alloc( RkVdecNeedParam_S stParam )
{
    RkVdec_S* pHandle = (RkVdec_S*)malloc(sizeof(RkVdec_S));
    
    memset(pHandle,0,sizeof(RkVdec_S));
    
    /********************必需参数*********************/
    pHandle->stNeedParam.unSrcWidth     = stParam.unSrcWidth;
    pHandle->stNeedParam.unSrcHeight    = stParam.unSrcHeight;
    pHandle->stNeedParam.nChnIndex      = stParam.nChnIndex;
    pHandle->stNeedParam.enCodecId      = stParam.enCodecId;
    pHandle->stNeedParam.eInputMode     = VIDEO_MODE_STREAM;
    pHandle->stNeedParam.eOutputPixFmt  = stParam.eOutputPixFmt;
    pHandle->stNeedParam.eCompressMode  = stParam.eCompressMode;

    /********************功能参数*********************/
    pHandle->stExParam.unFrameBufferCnt = 4;
    pHandle->stExParam.unSendBufferCnt  = 4;
    pHandle->stExParam.bEnableMbPool    = RK_FALSE;
    pHandle->stExParam.bEnableColmv     = RK_FALSE;
    pHandle->stExParam.bEnableDei       = RK_FALSE;
    pHandle->stExParam.eDisPlayMode     = VIDEO_DISPLAY_MODE_PLAYBACK;
    
    /********************函数列表*********************/
    pHandle->rockitVdec_send_stream     = rockitVdec_send_stream;
    pHandle->rockitVdec_get_frame       = rockitVdec_get_frame;
    pHandle->rockitVdec_get_frameVir    = rockitVdec_get_frameVir;
    pHandle->rockitVdec_release_frame   = rockitVdec_release_frame;
    pHandle->rockitVdec_reset_vdecChn   = rockitVdec_reset_vdecChn;
    pHandle->rockitVdec_get_chnAttr     = rockitVdec_get_chnAttr;
    pHandle->rockitVdec_set_chnAttr     = rockitVdec_set_chnAttr;
    pHandle->rockitVdec_change_wh       = rockitVdec_change_wh;
    pHandle->rockitVdec_change_codecId  = rockitVdec_change_codecId;
    pHandle->rockitVdec_set_userPic     = rockitVdec_set_userPic;
    pHandle->rockitVdec_show_userPic    = rockitVdec_show_userPic;
    pHandle->rockitVdec_disShow_userPic    = rockitVdec_disShow_userPic;
    pHandle->rockitVdec_init            = rockitVdec_init;
    pHandle->rockitVdec_uninit          = rockitVdec_uninit;

    return pHandle;
}
void rockitVdec_release(RkVdec_S* pHandle)
{
    if(!pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
}

