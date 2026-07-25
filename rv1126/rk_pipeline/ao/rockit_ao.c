/**
 * @FilePath     : rockit_ao.c
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-12-10 20:45:23
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-15 19:23:26
 * @Description  : rockit ao模块封装
 */

#include "rockit_ao.h"

/**
 * @brief   : 声音质量增强配置信息结构体属性填充
 * @param    {RkAo_S} *pHandle 句柄
 * @param    {AO_VQE_CONFIG_S} *pstVqeConfig AO的VQE模块配置结构体指针
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAo_vqeConfig_fill(RkAo_S *pHandle, AO_VQE_CONFIG_S *pstVqeConfig)
{
    if (NULL == pHandle)
    {
        return RK_FAILURE;
    }

    pstVqeConfig->enCfgMode = AIO_VQE_CONFIG_LOAD_FILE;
    pstVqeConfig->s32WorkSampleRate = pHandle->stExParam.nDevSampleRate;
    pstVqeConfig->s32FrameSample = pHandle->stExParam.nDevSampleRate * pHandle->stExParam.nVqeGapMs / 1000;

    memcpy(pstVqeConfig->aCfgFile, pHandle->stExParam.aVqeCfgPath, strlen(pHandle->stExParam.aVqeCfgPath));

    return RK_SUCCESS;
}

/**
 * @brief   : ao初始化
 * @param    {RkAo_S*} pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAo_init(RkAo_S *pHandle)
{
    /*ao设备号*/
    AUDIO_DEV nAoDev = pHandle->stExParam.nDevId;

    AIO_ATTR_S stAoAttr;
    memset(&stAoAttr, 0, sizeof(stAoAttr));

    /* 设置声卡设备参数 */
    stAoAttr.soundCard.channels = pHandle->stExParam.nDevChannels;
    stAoAttr.soundCard.sampleRate = pHandle->stExParam.nDevSampleRate;
    stAoAttr.soundCard.bitWidth = pHandle->stExParam.eDevBitWidth;
    /* 声卡名字 */
    snprintf((char *) (stAoAttr.u8CardName),
             sizeof(pHandle->stNeedParam.aDevName),
             "%s",
             pHandle->stNeedParam.aDevName);
    /* 设备驱动线程的队列⼤⼩，默认为4 */
    stAoAttr.s32DevQueLen = 4;

    /* 设置AO参数 */
    stAoAttr.enSamplerate = pHandle->stNeedParam.enSampleRate;          // 采样率
    stAoAttr.enBitwidth = pHandle->stNeedParam.enbitWidth;              // 采样精度
    stAoAttr.enSoundmode = pHandle->stNeedParam.enSoundMode;            // 声道模式
    stAoAttr.u32EXFlag = 0;                                             // 设置默认为0
    stAoAttr.u32FrmNum = pHandle->stNeedParam.u32FrameNum;              // 缓冲个数
    stAoAttr.u32PtNumPerFrm = pHandle->stNeedParam.u32PointNumPerFrame; // 应用取帧的长度byte
    stAoAttr.u32ChnCnt = 2;                                             // 设置默认为2

    /* 设置 AO 设备属性 */
    CHECK_API_RETURN(RK_MPI_AO_SetPubAttr(nAoDev, &stAoAttr));

    /* 启⽤AO设备 */
    CHECK_API_RETURN(RK_MPI_AO_Enable(nAoDev));

    /* 设置AO声道模式 */
    CHECK_API_RETURN(RK_MPI_AO_SetTrackMode(nAoDev, pHandle->stExParam.enTrackMode));

    for (int nChn = 0; nChn < pHandle->stNeedParam.nChnNum; nChn++)
    {
        AO_CHN_PARAM_S stParams;
        /* 获取AO通道属性 */
        CHECK_API_RETURN(RK_MPI_AO_GetChnParams(nAoDev, nChn, &stParams));
        // stParams.enMode = AUDIO_CHN_MODE_LEFT;
        stParams.enLoopbackMode = AUDIO_LOOPBACK_NONE;
        /* 设置AO通道属性 */
        CHECK_API_RETURN(RK_MPI_AO_SetChnParams(nAoDev, nChn, &stParams));

        /* 是否启用vqe声音质量增强功能 */
        if (pHandle->stNeedParam.bVqeEnable == RK_TRUE)
        {
            if (pHandle->stExParam.nVqeGapMs != 16 && pHandle->stExParam.nVqeGapMs != 10)
            {
                mpi_ai_log("无效间隔:%d,仅支持 16 毫秒或 10 毫秒的 AO VQE 间隔", pHandle->stExParam.nVqeGapMs);
                return RK_FAILURE;
            }
            /* 判断使用vqe算法类型，设置值 */
            AO_VQE_CONFIG_S stVqeConfig;
            memset(&stVqeConfig, 0, sizeof(AO_VQE_CONFIG_S));
            rockitAo_vqeConfig_fill(pHandle, &stVqeConfig);
            /* 设置AO的声音质量增强功能相关属性 */
            CHECK_API_RETURN(RK_MPI_AO_SetVqeAttr(nAoDev, nChn, &stVqeConfig));
            /* 使能AO的声音质量增强功能 */
            CHECK_API_RETURN(RK_MPI_AO_EnableVqe(nAoDev, nChn));
        }

        /* 启用AO通道 */
        CHECK_API_RETURN(RK_MPI_AO_EnableChn(nAoDev, nChn));

        /* 是否启用重采样功能 */
        if (pHandle->stNeedParam.bResampleEnable)
        {
            /* 启用AO重采样 */
            CHECK_API_RETURN(RK_MPI_AO_EnableReSmp(nAoDev, nChn, pHandle->stNeedParam.enResampleRate));
        }
    }

    /* 设置设备音量 */
    CHECK_API_RETURN(RK_MPI_AO_SetVolume(nAoDev, pHandle->stExParam.nVolume));

    return RK_SUCCESS;
}

/**
 * @brief   : ao反初始化
 * @param    {RkAo_S*} pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAo_uninit(RkAo_S *pHandle)
{
    if (NULL == pHandle)
    {
        return RK_FAILURE;
    }

    /*ao设备号*/
    AUDIO_DEV nAoDev = pHandle->stExParam.nDevId;

    for (int nChn = 0; nChn < pHandle->stNeedParam.nChnNum; nChn++)
    {
        if (pHandle->stNeedParam.bResampleEnable)
        {
            CHECK_API_RETURN(RK_MPI_AO_DisableReSmp(nAoDev, nChn));
        }
        if (pHandle->stNeedParam.bVqeEnable == RK_TRUE)
        {
            /*禁用AO的声音质量增强功能*/
            CHECK_API_RETURN(RK_MPI_AO_DisableVqe(nAoDev, nChn));
        }
        /*禁用AO通道*/
        CHECK_API_RETURN(RK_MPI_AO_DisableChn(nAoDev, nChn));
    }

    CHECK_API_RETURN(RK_MPI_AO_Disable(nAoDev));

    return RK_SUCCESS;
}

/**
 * @brief   : 发送pcm数据到ao
 * @param    {RkAo_S} *pHandle 句柄
 * @param    {int} nChn 通道号
 * @param    {uint8_t} *pData pcm数据
 * @param    {int} nSize pcm数据大小
 * @param    {int} nTimeOut 超时时间
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAo_send_pcmData(RkAo_S *pHandle, int nChn, uint8_t *pData, int nSize, int nTimeOut)
{
    AUDIO_FRAME_S stFrame;
    MB_EXT_CONFIG_S stExtConfig;
    memset(&stExtConfig, 0, sizeof(stExtConfig));

    stFrame.u32Len = nSize;
    stFrame.u64TimeStamp = 0;
    stFrame.enBitWidth = pHandle->stNeedParam.enbitWidth;
    stFrame.enSoundMode = pHandle->stNeedParam.enSoundMode;
    stFrame.bBypassMbBlk = RK_FALSE;

    stExtConfig.pOpaque = pData;
    stExtConfig.pu8VirAddr = pData;
    stExtConfig.u64Size = nSize;

    /* 创建一个内存缓存块 */
    CHECK_API_RETURN(RK_MPI_SYS_CreateMB(&(stFrame.pMbBlk), &stExtConfig));
    /* 发送AO音频帧 */
    CHECK_API_RETURN(RK_MPI_AO_SendFrame(pHandle->stExParam.nDevId, nChn, &stFrame, nTimeOut));
    /* 释放一个已经获取的缓存块 */
    CHECK_API_RETURN(RK_MPI_MB_ReleaseMB(stFrame.pMbBlk));
    return RK_SUCCESS;
}

/**
 * @brief   : 清除 AO 通道中当前的音频数据缓存
 * @param    {RkAo_S*} pHandle 句柄
 * @param    {int} nChn 通道号
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAo_clean_chnBuffer(RkAo_S *pHandle, int nChn)
{
    CHECK_API_RETURN(RK_MPI_AO_ClearChnBuf(pHandle->stExParam.nDevId, nChn));
    return RK_SUCCESS;
}

/**
 * @brief   : 查询 AO 通道中当前的音频数据缓存状态
 * @param    {RkAo_S} *pHandle 句柄
 * @param    {int} nChn AO 通道 
 * @param    {AO_CHN_STATE_S} *pstStat 数据缓存状态
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAo_get_chnStat(RkAo_S *pHandle, int nChn, AO_CHN_STATE_S *pstStat)
{
    CHECK_API_RETURN(RK_MPI_AO_QueryChnStat(pHandle->stExParam.nDevId, nChn, pstStat));
    return RK_SUCCESS;
}

/**
 * @brief   : 设置 AO 设备音量大小
 * @param    {RkAo_S} *pHandle 句柄
 * @param    {int} nVolume 音量大小
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAo_set_volume(RkAo_S *pHandle, int nVolume)
{
    if (NULL == pHandle) 
    {
        return RK_FAILURE;
    }

    if (nVolume < 0 || nVolume > 100) 
    {
        printf("无效的音量值: %d, 应处于范围 [0, 100]之内", nVolume);
        return RK_FAILURE;
    }

    /*ao设备号*/
    AUDIO_DEV nAoDev = pHandle->stExParam.nDevId;

    pHandle->stExParam.nVolume = nVolume;
    /* 淡入淡出结构体 */
    AUDIO_FADE_S stFade;
    stFade.bFade = RK_TRUE;
    stFade.enFadeOutRate = AUDIO_FADE_RATE_128; // 音频输出设备音量淡入速度
    stFade.enFadeInRate = AUDIO_FADE_RATE_128;  // 音频输出设备音量淡出速度
    if (pHandle->stExParam.nVolume != 0)
    {
        /* 取消AO设备静音 */
        CHECK_API_RETURN(RK_MPI_AO_SetMute(nAoDev, RK_FALSE, &stFade));
        /* 设置AO设备音量大小 */
        CHECK_API_RETURN(RK_MPI_AO_SetVolume(nAoDev, nVolume));
    }
    else
    {
        /* 设置AO设备静音 */
        CHECK_API_RETURN(RK_MPI_AO_SetMute(nAoDev, RK_TRUE, &stFade));
    }
    return RK_SUCCESS;
}

/**
 * @brief   : 设置 AO 设备静音状态
 * @param    {RkAo_S*} pHandle 句柄
 * @param    {RK_BOOL} bMutex 静音状态
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAo_set_mutex(RkAo_S *pHandle, RK_BOOL bMutex)
{
    /* 淡入淡出结构体 */
    AUDIO_FADE_S stFade;
    memset(&stFade, 0, sizeof(AUDIO_FADE_S));

    stFade.bFade = RK_FALSE;
    stFade.enFadeOutRate = AUDIO_FADE_RATE_16; // 音频输出设备音量淡入速度
    stFade.enFadeInRate = AUDIO_FADE_RATE_16;  // 音频输出设备音量淡出速度

    CHECK_API_RETURN(RK_MPI_AO_SetMute(pHandle->stExParam.nDevId, bMutex, &stFade));
    return RK_SUCCESS;
}

/**
 * @brief   : 设置AO声道模式
 * @param    {RkAo_S} *pHandle 句柄
 * @param    {AUDIO_TRACK_MODE_E} enTrackMode 声道模式
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAo_set_track_mode(RkAo_S *pHandle, AUDIO_TRACK_MODE_E enTrackMode)
{
    if (NULL == pHandle)
    {
        return RK_FAILURE;
    }

    if (enTrackMode != pHandle->stExParam.enTrackMode)
    {
        /* 设置AO声道模式 */
        CHECK_API_RETURN(RK_MPI_AO_SetTrackMode(pHandle->stExParam.nDevId, enTrackMode));
    }

    pHandle->stExParam.enTrackMode = enTrackMode;

    return RK_SUCCESS;
}

RkAo_S *rockitAo_alloc(RkAoNeedParam_S stNeedParam)
{
    RkAo_S *pHandle = (RkAo_S *) malloc(sizeof(RkAo_S));

    /**************************必需参数*************************/
    pHandle->stNeedParam = stNeedParam;

    /**************************功能参数*************************/
    pHandle->stExParam.nDevId               = 0;
    pHandle->stExParam.eDevBitWidth         = AUDIO_BIT_WIDTH_16;
    pHandle->stExParam.nDevChannels         = 2;
    pHandle->stExParam.nDevSampleRate       = 16000;
    pHandle->stExParam.nVolume              = 100;
    pHandle->stExParam.enTrackMode          = AUDIO_TRACK_NORMAL;
    pHandle->stExParam.nVqeGapMs            = 16;
    memset(pHandle->stExParam.aVqeCfgPath, 0, sizeof(pHandle->stExParam.aVqeCfgPath));

    /**************************列表参数*************************/
    pHandle->rockitAo_init              = rockitAo_init;
    pHandle->rockitAo_uninit            = rockitAo_uninit;
    pHandle->rockitAo_send_pcmData      = rockitAo_send_pcmData;
    pHandle->rockitAo_clean_chnBuffer   = rockitAo_clean_chnBuffer;
    pHandle->rockitAo_get_chnStat       = rockitAo_get_chnStat;
    pHandle->rockitAo_set_volume        = rockitAo_set_volume;
    pHandle->rockitAo_set_mutex         = rockitAo_set_mutex;
    pHandle->rockitAo_set_track_mode    = rockitAo_set_track_mode;
    return pHandle;
}

int rockitAo_release(RkAo_S *pHandle)
{
    if (pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
    return RK_SUCCESS;
}
