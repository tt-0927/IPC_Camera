/*************************************************************************
	> File Name: rockit_aenc.c
	> Author:luoyk 
	> Mail: 
	> Created Time: 2022年06月23日 星期四 10时23分30秒
 ************************************************************************/
#include<stdio.h>
#include"rockit_aenc.h"
static int rockitAenc_init( RkAenc_S* pHandle )
{
    int nRet;
    AENC_CHN_ATTR_S stChnAttr;
    memset( &stChnAttr, 0, sizeof(AENC_CHN_ATTR_S) );
    
    stChnAttr.enType = pHandle->stNeedParam.enType;
    stChnAttr.u32BufCount = pHandle->stExParam.nBuffCount; 

    stChnAttr.stCodecAttr.enType            = pHandle->stNeedParam.enType;
    stChnAttr.stCodecAttr.u32Channels       = pHandle->stNeedParam.nChannels;
    stChnAttr.stCodecAttr.u32SampleRate     = pHandle->stNeedParam.nSampleRate;
    stChnAttr.stCodecAttr.enBitwidth        = pHandle->stNeedParam.enBitWidth;
    stChnAttr.stCodecAttr.u32Bitrate        = pHandle->stNeedParam.nBitrate;
    stChnAttr.stCodecAttr.pstResv           = RK_NULL;
    nRet = RK_MPI_AENC_CreateChn( pHandle->stNeedParam.nChn, &stChnAttr );
    if (nRet != RK_SUCCESS) 
    {
        printf("create aenc chn %d err:0x%x\n", pHandle->stNeedParam.nChn, nRet);
        return RK_FAILURE;
    }
    
    pHandle->nFd = RK_MPI_AENC_GetFd( pHandle->stNeedParam.nChn );
    return 0;
}
static int rockitAenc_uninit( RkAenc_S* pHandle )
{
    RK_MPI_AENC_DestroyChn( pHandle->stNeedParam.nChn );
    return 0;
}

/*送数据到编码通道*/
static int rockitAenc_send_data( RkAenc_S* pHandle, PcmInfo_S* stPcmInfo)
{
    int nRet;
	AUDIO_FRAME_S stAudioFrm;
    MB_EXT_CONFIG_S extConfig = {0};
    if(NULL == pHandle)
    {
        printf("send enc phandle NULL\n");
        return -1;
    }

    if(NULL == stPcmInfo->pData)
    {
        printf("send enc data NULL\n");
        return -1;
    }
	stAudioFrm.u32Len = stPcmInfo->nSize;
    stAudioFrm.u64TimeStamp = stPcmInfo->unTimeStamp;
    stAudioFrm.u32Seq = stPcmInfo->unSeq;
    stAudioFrm.bBypassMbBlk = RK_TRUE;

    extConfig.pFreeCB = stPcmInfo->pFreeCB;
    extConfig.pOpaque = stPcmInfo->pData;
    extConfig.pu8VirAddr = stPcmInfo->pData;
    extConfig.u64Size    = stPcmInfo->nSize;
    
    RK_MPI_SYS_CreateMB(&(stAudioFrm.pMbBlk), &extConfig);
    nRet = RK_MPI_AENC_SendFrame( pHandle->stNeedParam.nChn, &stAudioFrm, RK_NULL, stPcmInfo->nTimeOut);
    if ( nRet != RK_SUCCESS) 
    {
        printf("fail to send aenc stream 0x%x\n",nRet);
    }
    RK_MPI_MB_ReleaseMB(stAudioFrm.pMbBlk);    
    return nRet;
}

/*获取编码帧*/
static int rockitAenc_get_frame( RkAenc_S* pHandle, AUDIO_STREAM_S* pstStream, int nTimeOut)
{
    int nRet = 0;
    nRet = RK_MPI_AENC_GetStream( pHandle->stNeedParam.nChn, pstStream, nTimeOut);
    if ( nRet != RK_SUCCESS) 
    {
        printf("fail to get aenc chn %d stream  0x%x\n", pHandle->stNeedParam.nChn, nRet);
    }
    return nRet;
}

/*获取编码帧的虚拟地址*/
static uint8_t* rockitAenc_get_virData( AUDIO_STREAM_S* pstStream )
{
    return (uint8_t*)RK_MPI_MB_Handle2VirAddr( pstStream->pMbBlk );
}

/*释放编码帧*/
static int rockitAenc_release_frame( RkAenc_S* pHandle, AUDIO_STREAM_S* pstStream )
{
    return RK_MPI_AENC_ReleaseStream( pHandle->stNeedParam.nChn, pstStream);
}


/*分配一个音频编码句柄*/
RkAenc_S* rockitAenc_alloc( RkAencNeedParam_S stNeedParam)
{
    RkAenc_S* pHandle = (RkAenc_S*) malloc ( sizeof(RkAenc_S) );
    
    /*****************必需参数***************************/
    pHandle->stNeedParam.nChn           = stNeedParam.nChn;
    pHandle->stNeedParam.enType         = stNeedParam.enType;
    pHandle->stNeedParam.enBitWidth     = stNeedParam.enBitWidth;
    pHandle->stNeedParam.nChannels      = stNeedParam.nChannels;
    pHandle->stNeedParam.nSampleRate    = stNeedParam.nSampleRate;
    pHandle->stNeedParam.nBitrate       = stNeedParam.nBitrate;

    /*****************功能参数***************************/
    pHandle->stExParam.nBuffCount = 4;
    
    /*****************函数列表***************************/
    pHandle->rockitAenc_init            = rockitAenc_init;
    pHandle->rockitAenc_uninit          = rockitAenc_uninit;
    pHandle->rockitAenc_send_data       = rockitAenc_send_data;
    pHandle->rockitAenc_get_frame       = rockitAenc_get_frame;
    pHandle->rockitAenc_get_virData     = rockitAenc_get_virData;
    pHandle->rockitAenc_release_frame   = rockitAenc_release_frame;

    return pHandle;
}

/*释放编码句柄*/
int rockitAenc_release( RkAenc_S* pHandle )
{
    if( pHandle )
    {
        free(pHandle);
        pHandle = NULL;
    }
    return 0;
}
