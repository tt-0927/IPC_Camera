/**
 * @FilePath     : rockit_adec.c
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-12-02 09:50:52
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-12 08:57:42
 * @Description  : RK ADEC 音频解码
 */

#include"rockit_adec.h"

/**
 * @brief   : adec初始化
 * @param    {RkAdec_S} *pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAdec_init(RkAdec_S *pHandle)
{
    if (NULL == pHandle)
    {
        return RK_FAILURE;
    }

    ADEC_CHN_ATTR_S stChnAttr;
    memset(&stChnAttr, 0, sizeof(ADEC_CHN_ATTR_S));

    stChnAttr.enType = pHandle->stNeedParam.enType;
    stChnAttr.enMode = pHandle->stNeedParam.enMode;
    stChnAttr.u32BufCount = pHandle->stExParam.nBuffCount;
    stChnAttr.u32BufSize = pHandle->stExParam.nBuffSize;

    stChnAttr.stCodecAttr.enType = pHandle->stNeedParam.enType;
    stChnAttr.stCodecAttr.u32Channels = pHandle->stNeedParam.nChannels;
    stChnAttr.stCodecAttr.u32SampleRate = pHandle->stNeedParam.nSampleRate;
    stChnAttr.stCodecAttr.u32BitPerCodedSample = pHandle->stNeedParam.nBitPerCodedSample;

    /* 创建音频解码通道 */
    CHECK_API_RETURN(RK_MPI_ADEC_CreateChn(pHandle->stNeedParam.nChn, &stChnAttr));
    return RK_SUCCESS;
}

/**
 * @brief   : adec去初始化
 * @param    {RkAdec_S*} pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAdec_uninit(RkAdec_S *pHandle)
{
    /* 销毁音频解码通道 */
    CHECK_API_RETURN(RK_MPI_ADEC_DestroyChn(pHandle->stNeedParam.nChn));
    return RK_SUCCESS;
}

/**
 * @brief   : 发送码流结束标识符
 * @param    {RkAdec_S} *pHandle 句柄
 * @param    {RK_BOOL} bInstant 是否立刻清除解码器内部的缓存数据
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAdec_send_endOfStream(RkAdec_S *pHandle, RK_BOOL bInstant)
{
    /* 向解码器发送码流结束标识符 */
    CHECK_API_RETURN(RK_MPI_ADEC_SendEndOfStream(pHandle->stNeedParam.nChn, bInstant));
    return RK_SUCCESS;
}

/**
 * @brief   : 送数据到解码通道
 * @param    {RkAdec_S} *pHandle 句柄
 * @param    {AudioFrameInfo_S} *pstFrameInfo 音频码流结构体指针
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAdec_send_data(RkAdec_S *pHandle, AudioFrameInfo_S *pstFrameInfo)
{
    if (NULL == pHandle || NULL == pstFrameInfo->pData)
    {
        return RK_FAILURE;
    }

    AUDIO_STREAM_S stAudioFrm;
    MB_EXT_CONFIG_S extConfig = { 0 };
    stAudioFrm.u32Len = pstFrameInfo->nSize;
    stAudioFrm.u64TimeStamp = pstFrameInfo->unTimeStamp;
    stAudioFrm.u32Seq = pstFrameInfo->unSeq;
    stAudioFrm.bBypassMbBlk = RK_TRUE;

    extConfig.pFreeCB = pstFrameInfo->pFreeCB;
    extConfig.pOpaque = pstFrameInfo->pData;
    extConfig.pu8VirAddr = pstFrameInfo->pData;
    extConfig.u64Size = pstFrameInfo->nSize;

    /* 创建一个内存缓存块 */
    CHECK_API_RETURN(RK_MPI_SYS_CreateMB(&(stAudioFrm.pMbBlk), &extConfig));

    /* 发送音频码流到音频解码通道 */
    CHECK_API_RETURN(RK_MPI_ADEC_SendStream(pHandle->stNeedParam.nChn, &stAudioFrm, pstFrameInfo->bBlock));

    /* 释放一个已经获取的缓存块 */
    CHECK_API_RETURN(RK_MPI_MB_ReleaseMB(stAudioFrm.pMbBlk));

    return RK_SUCCESS;
}

/**
 * @brief   : 清空解码通道缓存
 * @param    {RkAdec_S} *pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAdec_clear_chnBuf(RkAdec_S *pHandle)
{
    if (NULL == pHandle)
    {
        return RK_FAILURE;
    }

    /* 清除ADEC通道中当前的音频数据缓存 */
    CHECK_API_RETURN(RK_MPI_ADEC_ClearChnBuf(pHandle->stNeedParam.nChn));
    return RK_SUCCESS;
}

/**
 * @brief   : 查询解码通道状态
 * @param    {RkAdec_S*} pHandle 句柄
 * @param    {ADEC_CHN_STATE_S*} pstStat 音频解码通道的数据缓存状态结构体指针
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAdec_query_chnStat(RkAdec_S *pHandle, ADEC_CHN_STATE_S *pstStat)
{
    if (NULL == pHandle || NULL == pstStat)
    {
        return RK_FAILURE;
    }

    /* 查询ADEC通道中当前的音频数据缓存状态 */
    CHECK_API_RETURN(RK_MPI_ADEC_QueryChnStat( pHandle->stNeedParam.nChn, pstStat));
    return RK_SUCCESS;
}

/**
 * @brief   : 获取解码帧
 * @param    {RkAdec_S} *pHandle 句柄
 * @param    {AUDIO_FRAME_INFO_S} *pstStream 音频码流结构体指针
 * @param    {RK_BOOL} bBlock 是否以阻塞方式获取
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAdec_get_frame(RkAdec_S *pHandle, AUDIO_FRAME_INFO_S *pstStream, RK_BOOL bBlock)
{
    if (NULL == pHandle || NULL == pstStream)
    {
        return RK_FAILURE;
    }

    /* 获取音频解码帧数据 */
    CHECK_API_RETURN(RK_MPI_ADEC_GetFrame(pHandle->stNeedParam.nChn, pstStream, bBlock));
    return RK_SUCCESS;
}

/**
 * @brief   : 获取解码音频帧信息的虚拟地址
 * @note    : 获取一个内存缓存池中的缓存块的用户态虚拟地址
 * @param    {VENC_PACK_S*} pPack 帧码流包结构指针
 * @return   {uint8_t *} 码流的虚拟地址:成功, NULL:失败
 */
static uint8_t *rockitAdec_get_virData(AUDIO_FRAME_INFO_S *pstStream)
{
    if (NULL == pstStream)
    {
        return NULL;
    }

    /* 获取一个内存缓存池中的缓存块的用户态虚拟地址 */
    return (uint8_t *) RK_MPI_MB_Handle2VirAddr(pstStream->pstFrame->pMbBlk);
}

/**
 * @brief   : 释放音频解码帧数据
 * @param    {RkAdec_S} *pHandle 句柄
 * @param    {AUDIO_FRAME_INFO_S} *pstStream 音频码流结构体指针
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAdec_release_frame(RkAdec_S *pHandle, AUDIO_FRAME_INFO_S *pstStream)
{
    if (NULL == pHandle || NULL == pstStream)
    {
        return RK_FAILURE;
    }

    /* 释放音频解码帧数据 */
    CHECK_API_RETURN(RK_MPI_ADEC_ReleaseFrame(pHandle->stNeedParam.nChn, pstStream));
    return RK_SUCCESS;
}

RkAdec_S *rockitAdec_alloc(RkAdecNeedParam_S stNeedParam)
{
    RkAdec_S *pHandle = (RkAdec_S *) malloc(sizeof(RkAdec_S));

    /*****************必需参数***************************/
    pHandle->stNeedParam = stNeedParam;

    /*****************功能参数***************************/
    pHandle->stExParam.nBuffCount       = 4;
    pHandle->stExParam.nBuffSize        = 4096;

    /*****************函数列表***************************/
    pHandle->rockitAdec_init             = rockitAdec_init;
    pHandle->rockitAdec_uninit           = rockitAdec_uninit;
    pHandle->rockitAdec_send_endOfStream = rockitAdec_send_endOfStream;
    pHandle->rockitAdec_send_data        = rockitAdec_send_data;
    pHandle->rockitAdec_clear_chnBuf     = rockitAdec_clear_chnBuf;
    pHandle->rockitAdec_query_chnStat    = rockitAdec_query_chnStat;
    pHandle->rockitAdec_get_frame        = rockitAdec_get_frame;
    pHandle->rockitAdec_get_virData      = rockitAdec_get_virData;
    pHandle->rockitAdec_release_frame    = rockitAdec_release_frame;

    return pHandle;
}

int rockitAdec_release(RkAdec_S *pHandle)
{
    if (pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
    return 0;
}
