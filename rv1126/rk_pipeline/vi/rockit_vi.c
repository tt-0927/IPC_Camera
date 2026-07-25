/*
 * @FilePath     : rockit_vi.c
 * @Author       : luoyk 
 * @Date         : 2022-06-27 14:51:53
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2025-08-25 09:38:31
 * @Description  : 
 */

#include"rockit_vi.h"

/*获取vi通道状态*/
/*获取vi采集数据*/
/*获取vi采集数据虚拟地址*/
/*释放vi采集数据*/
/*通道属性配置*/
static int set_vi_attr( RkVi_S* pHandle)
{
    int nRet;
    VI_CHN_ATTR_S stChnAttr;
    memset( &stChnAttr, 0, sizeof(VI_CHN_ATTR_S) );
    
    stChnAttr.stSize.u32Width               = pHandle->stExParam.nScaWidth;
    stChnAttr.stSize.u32Height              = pHandle->stExParam.nScaHeight;
    stChnAttr.enCompressMode                = pHandle->stNeedParam.enCompressMode;
    stChnAttr.u32Depth                      = pHandle->stExParam.nDepth;
    stChnAttr.enPixelFormat                 = pHandle->stNeedParam.enPixelFormat;
    stChnAttr.stFrameRate.s32SrcFrameRate   = pHandle->stExParam.nSrcFrameRate;
    stChnAttr.stFrameRate.s32DstFrameRate   = pHandle->stExParam.nDstFrameRate; 
    memcpy( stChnAttr.stIspOpt.aEntityName, pHandle->stNeedParam.aEnityName, strlen(pHandle->stNeedParam.aEnityName));
    
    stChnAttr.stIspOpt.enMemoryType     = pHandle->stNeedParam.enMemType;
    // stChnAttr.stIspOpt.enCaptureType    = VI_V4L2_CAPTURE_TYPE_VIDEO_CAPTURE;
    // /******************************************* */
    // if( stChnAttr.stIspOpt.enMemoryType == VI_V4L2_MEMORY_TYPE_MMAP)
    // {
    //     stChnAttr.stIspOpt.bNoUseLibV4L2        = RK_FALSE;
    //     stChnAttr.enAllocBufType = VI_ALLOC_BUF_TYPE_EXTERNAL;
    // }
    // else
    // {
    //     stChnAttr.stIspOpt.bNoUseLibV4L2        = RK_TRUE;
    //     stChnAttr.enAllocBufType = VI_ALLOC_BUF_TYPE_INTERNAL;
    // }
    // stChnAttr.stIspOpt.bNoUseLibV4L2        = RK_FALSE;
    stChnAttr.stIspOpt.u32BufCount          = pHandle->stExParam.nBufCount;
    //stChnAttr.stIspOpt.u32BufSize           = pHandle->stNeedParam.nWidth * pHandle->stNeedParam.nHeight / 2;
    stChnAttr.stIspOpt.stMaxSize.u32Width   = pHandle->stNeedParam.nWidth;
    stChnAttr.stIspOpt.stMaxSize.u32Height  = pHandle->stNeedParam.nHeight;
    
    nRet = RK_MPI_VI_SetChnAttr( pHandle->stExParam.nPipeId, pHandle->stNeedParam.nChannel, &stChnAttr);
    printf("RK_MPI_VI_EnableChn %x %d %d %s\n", pHandle->stNeedParam.nDevId,
           pHandle->stExParam.nPipeId, pHandle->stNeedParam.nChannel, stChnAttr.stIspOpt.aEntityName);
    return nRet;
}

/*获取采集帧*/
static int rockitVi_getChnFrame( RkVi_S* pHandle, int nChn,  VIDEO_FRAME_INFO_S* pFrame, int nTimeoutMs )
{
    return RK_MPI_VI_GetChnFrame(pHandle->stNeedParam.nDevId, nChn, pFrame, nTimeoutMs );
}

/*释放采集帧*/
static int rockitVi_releaseChnFrame( RkVi_S* pHandle, int nChn,  VIDEO_FRAME_INFO_S* pFrame )
{
    return RK_MPI_VI_ReleaseChnFrame(pHandle->stNeedParam.nDevId, nChn, pFrame);
}

/*禁用通道*/
static int rockitVi_disable_chn( RkVi_S* pHandle )
{
    return RK_MPI_VI_DisableChn( pHandle->stNeedParam.nDevId, pHandle->stNeedParam.nChannel );
}

/*启用通道*/
static int rockitVi_enable_chn( RkVi_S* pHandle )
{
    return RK_MPI_VI_EnableChn( pHandle->stNeedParam.nDevId, pHandle->stNeedParam.nChannel );
}

/*暂停通道*/
static int rockitVi_pause_chn( RkVi_S* pHandle )
{
    return RK_MPI_VI_PauseChn( pHandle->stNeedParam.nDevId, pHandle->stNeedParam.nChannel );
}

/*恢复通道*/
static int rockitVi_resume_chn( RkVi_S* pHandle )
{
    return RK_MPI_VI_ResumeChn( pHandle->stNeedParam.nDevId, pHandle->stNeedParam.nChannel );
}

int rockitSet_GDC(RkVi_S* pHandle)
{
// GDC
    RK_S32 nRet = RK_SUCCESS;
	GDC_CHN_ATTR_S stAttr;
	GDC_UPDATE_ATTR_S stUpdateAttr;

    memset( &stAttr, 0, sizeof(GDC_CHN_ATTR_S) );
    memset( &stUpdateAttr, 0, sizeof(GDC_UPDATE_ATTR_S) );

	stAttr.u32MaxInQueue = 3;
	stAttr.u32MaxOutQueue = 3;
	stAttr.s32Depth = 0;
	stAttr.s32DstWidth = pHandle->stNeedParam.nWidth;
	stAttr.s32DstHeight = pHandle->stNeedParam.nHeight;
	stAttr.enDstPixelFormat = RK_FMT_YUV420SP;
	stAttr.enDstCompMode = COMPRESS_RFBC_64x4;

	stAttr.enMode = GDC_CHN_MODE_FEC;
	stAttr.stFecAttr.s32InFourcc = 0;
	stAttr.stFecAttr.s32OutFourcc = 0;
	stAttr.stFecAttr.s32BorderMode = 0;
	stAttr.stFecAttr.s32CrossBufMode = 0;
	stAttr.stFecAttr.stBgVal.s32BgY = 255;
	stAttr.stFecAttr.stBgVal.s32BgU = 100;
	stAttr.stFecAttr.stBgVal.s32BgV = 0;

	const char *fec_ini_file = pHandle->stNeedParam.sGdcFecFile;
	if (fec_ini_file) {
		stUpdateAttr.enMode = GDC_CHN_MODE_FEC;
		RK_MPI_GDC_GetAttrFromFile(&stUpdateAttr, fec_ini_file);
		stAttr.stFecAttr.enFecMode = GDC_FEC_UPDATE_MESH_ONLINE;
		stAttr.stFecAttr.dLightCenter[0] = stUpdateAttr.stFecAttr.stOnlineCfg.dLightCenter[0];
		stAttr.stFecAttr.dLightCenter[1] = stUpdateAttr.stFecAttr.stOnlineCfg.dLightCenter[1];
		for (int i = 0; i < 4; i++) {
			stAttr.stFecAttr.dCoeff[i] = stUpdateAttr.stFecAttr.stOnlineCfg.dCoeff[i];
		}
		// stAttr.stFecAttr.s32CorrectLevel  = stUpdateAttr.stFecAttr.stOnlineCfg.s32CorrectLevel;
		stAttr.stFecAttr.s32CorrectLevel =0 * 2.55;  //等级0
		stAttr.stFecAttr.enStyle = stUpdateAttr.stFecAttr.stOnlineCfg.enStyle;
	} else {
		 printf("fec_ini_file is NULL\n");
		return -1;
	}
	stAttr.stFecAttr.enDirection = GDC_FEC_CORRECT_DIRECTION_XY;
	stAttr.stFecAttr.enStyle = GDC_FEC_KEEP_ASPECT_RATIO_REDUCE_FOV;

	nRet = RK_MPI_GDC_CreateChn(pHandle->stNeedParam.nDevId, &stAttr);
    if ( nRet != RK_SUCCESS ) 
    {
        printf("RK_MPI_GDC_CreateChn %x\n", nRet);
        return -1;
    }
     return nRet;
}



/*初始化vi*/
int rockitVi_init( RkVi_S* pHandle )
{
    VI_DEV_ATTR_S stDevAttr;
    RK_S32 nRet = RK_FAILURE;
    VI_DEV_BIND_PIPE_S stBindPipe;

    memset( &stDevAttr, 0, sizeof(VI_DEV_ATTR_S) );
    memset( &stBindPipe, 0, sizeof(VI_DEV_BIND_PIPE_S) );

    /*通道属性配置*/

    // 0. get dev config status
    nRet = RK_MPI_VI_GetDevAttr( pHandle->stNeedParam.nDevId, &stDevAttr );
    if ( nRet == RK_ERR_VI_NOT_CONFIG) 
    {
        printf("RK_MPI_VI_SetDevAttr begin\n");
        nRet = RK_MPI_VI_SetDevAttr( pHandle->stNeedParam.nDevId, &stDevAttr );
        if ( nRet != RK_SUCCESS ) 
        {
            printf("RK_MPI_VI_SetDevAttr %x\n", nRet);
            return -1;
        }
    }
    else
    {
        printf("RK_MPI_VI_SetDevAttr already\n");
    }

    nRet = RK_MPI_VI_GetDevIsEnable( pHandle->stNeedParam.nDevId );
    
    if ( nRet != RK_SUCCESS ) 
    {
        nRet = RK_MPI_VI_EnableDev( pHandle->stNeedParam.nDevId );
        if ( nRet != RK_SUCCESS ) 
        {
            printf("RK_MPI_VI_EnableDev %x\n", nRet);
            return -1;
        }

        // 1-3.bind dev/pipe
        stBindPipe.u32Num = pHandle->stExParam.nPipeId;
        stBindPipe.PipeId[0] = pHandle->stExParam.nPipeId;
        nRet = RK_MPI_VI_SetDevBindPipe( pHandle->stNeedParam.nDevId, &stBindPipe);
        if ( nRet != RK_SUCCESS) 
        {
            printf("RK_MPI_VI_SetDevBindPipe %x\n", nRet);
            return -1;
        }

    }
    else
    {
        printf("RK_MPI_VI_EnableDev already\n");
    }
    // 2.config channel

    nRet = set_vi_attr(pHandle);
    if ( nRet != RK_SUCCESS ) 
    {
        printf("RK_MPI_VI_SetChnAttr %x\n", nRet);
        return -1;
    }

    //pHandle->nFd = RK_MPI_VI_GetChnFd( pHandle->stExParam.nPipeId, pHandle->stNeedParam.nChannel);

    // 3.enable channel
    nRet = RK_MPI_VI_EnableChn( pHandle->stExParam.nPipeId, pHandle->stNeedParam.nChannel );
    if ( nRet != RK_SUCCESS) 
    {
        printf("RK_MPI_VI_EnableChn %x\n", nRet);
        return -1;
    }

    // 4.save debug file
    #if 0
    nRet = RK_MPI_VI_ChnSaveFile(ctx->pipeId, ctx->channelId, &ctx->stDebugFile);
    #endif

    // nRet = rockitSet_GDC(pHandle);
    // if ( nRet != RK_SUCCESS) 
    // {
    //     printf("rockitSet_GDC fail %x\n", nRet);
    //     return -1;
    // }
	return 0;

}
/*设置vi的属性*/
int rockitVi_set_attr( RkVi_S* pHandle, RkViNeedParam_S stNeedParam, RkViExParam_S stExParam  )
{
    if( pHandle == NULL )
    {
        return -1;
    }
    pHandle->rockitVi_disable_chn( pHandle );
    memcpy( &pHandle->stNeedParam, &stNeedParam, sizeof(RkViNeedParam_S) );    
    memcpy( &pHandle->stExParam, &stExParam, sizeof(RkViExParam_S) );    
    set_vi_attr( pHandle);
    int nRet = pHandle->rockitVi_enable_chn( pHandle );
    if ( nRet != RK_SUCCESS) 
    {
        printf("RK_MPI_VI_EnableChn %x\n", nRet);
        return -1;
    }
    return 0;
}

/*反初始化vi*/
int rockitVi_uninit( RkVi_S* pHandle )
{
    int nRet = 0;
    //nRet = RK_MPI_VI_CloseChnFd( pHandle->stExParam.nPipeId, pHandle->stNeedParam.nChannel );
    // if ( nRet != RK_SUCCESS) 
    //{
        //printf("RK_MPI_VI_closeFd fail %x\n", nRet);
      //  return -1;
    //}
    nRet = RK_MPI_VI_DisableChn( pHandle->stExParam.nPipeId, pHandle->stNeedParam.nChannel );
    if ( nRet != RK_SUCCESS) 
    {
        return -1;
    }
    nRet = RK_MPI_VI_DisableDev( pHandle->stNeedParam.nDevId );
    if ( nRet != RK_SUCCESS) 
    {
        return -1;
    }
    return 0;
}

/*分配vi句柄*/
RkVi_S* rockitVi_alloc( RkViNeedParam_S stNeedParam )
{
    RkVi_S* pHandle = (RkVi_S*) malloc ( sizeof(RkVi_S) );
    memset( pHandle, 0, sizeof( RkVi_S ) );

    /**************必需参数******************/
    pHandle->stNeedParam.nDevId         = stNeedParam.nDevId;
    pHandle->stNeedParam.nChannel       = stNeedParam.nChannel;
    snprintf( (char*)(pHandle->stNeedParam.aEnityName),sizeof(pHandle->stNeedParam.aEnityName), "%s", stNeedParam.aEnityName);
    snprintf( (char*)(pHandle->stNeedParam.sGdcFecFile),sizeof(pHandle->stNeedParam.sGdcFecFile), "%s", stNeedParam.sGdcFecFile);
    pHandle->stNeedParam.nWidth         = stNeedParam.nWidth;
    pHandle->stNeedParam.nHeight        = stNeedParam.nHeight;
    pHandle->stNeedParam.enPixelFormat  = stNeedParam.enPixelFormat;
    pHandle->stNeedParam.enCompressMode = stNeedParam.enCompressMode;
    pHandle->stNeedParam.enMemType      = stNeedParam.enMemType;
    /**************功能参数******************/
    pHandle->stExParam.nPipeId          = stNeedParam.nDevId;
    pHandle->stExParam.nSrcFrameRate    = -1;
    pHandle->stExParam.nDstFrameRate    = -1;
    pHandle->stExParam.nDepth           = 0;	//不通过RK_MPI_VI_GetChnFrame获取图像，u32Depth设置为0。
    pHandle->stExParam.nBufCount        = 3;	
    pHandle->stExParam.nScaWidth        = stNeedParam.nWidth;
    pHandle->stExParam.nScaHeight        = stNeedParam.nHeight;
    /**************函数列表******************/
    pHandle->rockitVi_enable_chn        = rockitVi_enable_chn;
    pHandle->rockitVi_disable_chn       = rockitVi_disable_chn;
    pHandle->rockitVi_pause_chn         = rockitVi_pause_chn;
    pHandle->rockitVi_resume_chn        = rockitVi_resume_chn;
    pHandle->rockitVi_set_attr          = rockitVi_set_attr;
    pHandle->rockitVi_init              = rockitVi_init;
    pHandle->rockitVi_uninit            = rockitVi_uninit;
    pHandle->rockitVi_getChnFrame       = rockitVi_getChnFrame;
    pHandle->rockitVi_releaseChnFrame   = rockitVi_releaseChnFrame;
    
    return pHandle;
}

/*释放vi句柄*/
int rockitVi_release( RkVi_S*pHandle )
{
    if( pHandle )
    {
        free( pHandle );
        pHandle = NULL;
    }
    return 0;
}
