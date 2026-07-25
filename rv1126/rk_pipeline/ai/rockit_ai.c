/**
 * @FilePath     : rockit_ai.c
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-12-02 09:50:52
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-16 16:47:35
 * @Description  : rockit ai模块封装
 */

#include <stdio.h>
#include "rockit_ai.h"

/**
 * @brief   : 声音质量增强配置信息结构体属性填充
 * @param    {RkAi_S} *pHandle 句柄
 * @param    {AI_VQE_CONFIG_S} *pstVqeConfig AI的VQE模块配置结构体指针
 * @param    {AI_VQE_MOD_ENABLE_S} *pstModEnable VQE模块使能结构体指针
 * @param    {RK_BOOL} bEnableNr 是否使能噪声消除
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAi_vqeConfig_fill(RkAi_S *pHandle,
                                   AI_VQE_CONFIG_S *pstVqeConfig,
                                   AI_VQE_MOD_ENABLE_S *pstModEnable,
                                   RK_BOOL bEnableNr)
{
    if (NULL == pHandle)
    {
        return RK_FAILURE;
    }

    /* 判断是否使能降噪 */
    if (bEnableNr == RK_TRUE)
    {
        pstModEnable->bAec = RK_TRUE;        // 回声消除
        pstModEnable->bBf = RK_FALSE;        // 波束形成
        pstModEnable->bFastAec = RK_TRUE;    // 线性残留回声消除
        pstModEnable->bAes = RK_TRUE;        // 非线性残留回声消除
        pstModEnable->bWakeup = RK_FALSE;    // 语音唤醒
        pstModEnable->bGsc = RK_FALSE;       // 开波束后滤波
        pstModEnable->bAgc = RK_TRUE;        // 自动增益控制
        pstModEnable->bAnr = RK_FALSE;       // 噪声抑制
        pstModEnable->bNlp = RK_FALSE;       // 非线性残留回声消除
        pstModEnable->bDereverb = RK_FALSE;  // 去混响
        pstModEnable->bCng = RK_FALSE;       // 舒适度噪声
        pstModEnable->bDtd = RK_FALSE;       // 双讲检测
        pstModEnable->bEq = RK_FALSE;        // 均衡器
        pstModEnable->bHowling = RK_FALSE;   // 啸叫抑制
        pstModEnable->bDoa = RK_FALSE;       // 声源定位
        pstModEnable->bAecDelay = RK_FALSE;  // AEC 延迟估计
        pstModEnable->bGscMethod = RK_FALSE; // 开波束后滤波方法
        pstModEnable->bWind = RK_FALSE;      // 风噪抑制
        pstModEnable->bAinr = RK_TRUE;       // AI 语音降噪
    }
    else if (bEnableNr == RK_FALSE)
    {
        pstModEnable->bAec = RK_TRUE;        // 回声消除
        pstModEnable->bBf = RK_FALSE;        // 波束形成
        pstModEnable->bFastAec = RK_TRUE;    // 线性残留回声消除
        pstModEnable->bAes = RK_TRUE;        // 非线性残留回声消除
        pstModEnable->bWakeup = RK_FALSE;    // 语音唤醒
        pstModEnable->bGsc = RK_FALSE;       // 开波束后滤波
        pstModEnable->bAgc = RK_TRUE;        // 自动增益控制
        pstModEnable->bAnr = RK_FALSE;       // 噪声抑制
        pstModEnable->bNlp = RK_FALSE;       // 非线性残留回声消除
        pstModEnable->bDereverb = RK_FALSE;  // 去混响
        pstModEnable->bCng = RK_FALSE;       // 舒适度噪声
        pstModEnable->bDtd = RK_FALSE;       // 双讲检测
        pstModEnable->bEq = RK_FALSE;        // 均衡器
        pstModEnable->bHowling = RK_FALSE;   // 啸叫抑制
        pstModEnable->bDoa = RK_FALSE;       // 声源定位
        pstModEnable->bAecDelay = RK_FALSE;  // AEC 延迟估计
        pstModEnable->bGscMethod = RK_FALSE; // 开波束后滤波方法
        pstModEnable->bWind = RK_FALSE;      // 风噪抑制
        pstModEnable->bAinr = RK_FALSE;      // AI 语音降噪
    }

    pstVqeConfig->enCfgMode = AIO_VQE_CONFIG_LOAD_FILE;
    pstVqeConfig->s32WorkSampleRate = pHandle->stExParam.nDevSampleRate;
    pstVqeConfig->s32FrameSample = pHandle->stExParam.nDevSampleRate * pHandle->stExParam.nVqeGapMs / 1000;
    // pstVqeConfig->s64RefChannelType = pHandle->stExParam.s64RefChannelType;    // 回采数据的通道类型 0b00000010
    // pstVqeConfig->s64RecChannelType = pHandle->stExParam.s64RecChannelType;    // 录音数据的通道类型 0b00000001
    // for (int i = 0; i < pHandle->stExParam.nDevChannels; i++)
    // {
    //     pstVqeConfig->s64ChannelLayoutType |= (1 << i);
    // }
    // mpi_ai_log("Vqe配置:[%s] pstVqeConfig->s64ChannelLayoutType:[%lld]",pHandle->stExParam.aVqeCfgPath,pstVqeConfig->s64ChannelLayoutType);
    memcpy(pstVqeConfig->aCfgFile, pHandle->stExParam.aVqeCfgPath, strlen(pHandle->stExParam.aVqeCfgPath));

	return RK_SUCCESS;
}

/**
 * @brief   : ai初始化
 * @param    {RkAi_S*} pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAi_init(RkAi_S *pHandle)
{
    /*ai设备号*/
    AUDIO_DEV nAiDev = pHandle->stExParam.nDevId;

    AIO_ATTR_S stAiAttr;
    memset(&stAiAttr, 0, sizeof(stAiAttr));

    /* 设置声卡设备参数 */
    stAiAttr.soundCard.channels = pHandle->stExParam.nDevChannels;
    stAiAttr.soundCard.sampleRate = pHandle->stExParam.nDevSampleRate;
    stAiAttr.soundCard.bitWidth = pHandle->stExParam.eDevBitWidth;
    /* 声卡名字 */
    snprintf((char *) (stAiAttr.u8CardName),
             sizeof(pHandle->stNeedParam.aDevName),
             "%s",
             pHandle->stNeedParam.aDevName);
    /* 设备驱动线程的队列⼤⼩，默认为4 */
    stAiAttr.s32DevQueLen = 4;

    /* 设置AI参数 */
    stAiAttr.enSamplerate = pHandle->stNeedParam.enSampleRate;          // 采样率
    stAiAttr.enBitwidth = pHandle->stNeedParam.enbitWidth;              // 采样精度
    stAiAttr.enSoundmode = pHandle->stNeedParam.enSoundMode;            // 声道模式
    stAiAttr.u32EXFlag = 0;                                             // 设置默认为0
    stAiAttr.u32FrmNum = pHandle->stNeedParam.u32FrameNum;              // 缓冲个数
    stAiAttr.u32PtNumPerFrm = pHandle->stNeedParam.u32PointNumPerFrame; // 应用取帧的长度byte
    stAiAttr.u32ChnCnt = 2;                                             // 设置默认为2

    /* 设置 AI 设备属性 */
    CHECK_API_RETURN(RK_MPI_AI_SetPubAttr(nAiDev, &stAiAttr));

    /* 开启ACodec ADC */
    CHECK_API_RETURN(RK_MPI_AMIX_SetControl(nAiDev, "ACodec ADC Switch", (char *) "1"));
    /* 设置模拟前级增益 16(0->31) */
    CHECK_API_RETURN(RK_MPI_AMIX_SetControl(nAiDev, "ACodec PGA Gain Volume", (char *) "31"));
    CHECK_API_RETURN(RK_MPI_AMIX_SetControl(nAiDev, "ACodec_LP PGA Gain Volume", (char *) "31"));
    /* 设置数字增益 0(0->127) */
    // CHECK_API_RETURN(RK_MPI_AMIX_SetControl(nAiDev, "ACodec Digital Gain Volume", (char *) "100"));
    /* 高通滤波 "3.79Hz" "60Hz" "243Hz" "493Hz"*/
    // CHECK_API_RETURN(RK_MPI_AMIX_SetControl(nAiDev, "ACodec HPF Cutoff", (char *) "243Hz"));
    // CHECK_API_RETURN(RK_MPI_AMIX_SetControl(nAiDev, "ACodec HPF Cutoff", (char *) "1"));

    /* 设置回采设置 */
    // CHECK_API_RETURN(RK_MPI_AMIX_SetControl(nAiDev, "SAI2 SDI0 Loopback Src Select", (char *) "From SDO0"));
    CHECK_API_RETURN(RK_MPI_AMIX_SetControl(nAiDev, "SAI2 SDI0 Loopback I2S LR Switch", (char *) "Enable"));
    CHECK_API_RETURN(RK_MPI_AMIX_SetControl(nAiDev, "SAI2 SDI0 Loopback Switch", (char *) "Enable"));
    CHECK_API_RETURN(RK_MPI_AMIX_SetControl(nAiDev, "SAI2 SDI1 Loopback I2S LR Switch", (char *) "Enable"));
    CHECK_API_RETURN(RK_MPI_AMIX_SetControl(nAiDev, "SAI2 SDI1 Loopback Switch", (char *) "Enable"));
    /* ALC设置 */
    // CHECK_API_RETURN(RK_MPI_AMIX_SetControl(nAiDev, "ADC ALC Left Volume", (char *) "22"));
    // CHECK_API_RETURN(RK_MPI_AMIX_SetControl(nAiDev, "ADC ALC Right Volume", (char *) "22"));

    /* 设置AI声道模式 */
    CHECK_API_RETURN(RK_MPI_AI_SetTrackMode(nAiDev, pHandle->stExParam.enTrackMode));

    /* 启⽤AI设备 */
    CHECK_API_RETURN(RK_MPI_AI_Enable(nAiDev));
    
    for (int nChn = 0; nChn < pHandle->stNeedParam.nChnNum; nChn++)
    {
        AI_CHN_PARAM_S stParams;
        /* 获取AI通道属性 */
        CHECK_API_RETURN(RK_MPI_AI_GetChnParam(nAiDev, nChn, &stParams));
        stParams.s32UsrFrmDepth = pHandle->stNeedParam.u32FrameNum; // 音频帧缓存深度 默认为4个
        /* 设置AI通道属性 */
        CHECK_API_RETURN(RK_MPI_AI_SetChnParam(nAiDev, nChn, &stParams));

        /* 是否启用vqe声音质量增强功能 */
        if (pHandle->stNeedParam.bVqeEnable == RK_TRUE)
        {
            if (pHandle->stExParam.nVqeGapMs != 16 && pHandle->stExParam.nVqeGapMs != 10)
            {
                mpi_ai_log("无效间隔:%d,仅支持 16 毫秒或 10 毫秒的 AI VQE 间隔", pHandle->stExParam.nVqeGapMs);
                return RK_FAILURE;
            }
            /* 判断使用vqe算法类型，设置值 */
            AI_VQE_CONFIG_S stVqeConfig;
            memset(&stVqeConfig, 0, sizeof(AI_VQE_CONFIG_S));
            AI_VQE_MOD_ENABLE_S stModEnable;
            rockitAi_vqeConfig_fill(pHandle, &stVqeConfig, &stModEnable, pHandle->stExParam.bEnableNr);
            /* 启用AI声音质量增强使能模块 */
            // CHECK_API_RETURN(RK_MPI_AI_SetVqeModuleEnable(nAiDev, nChn, &stModEnable));
            /* 设置AI的声音质量增强功能相关属性 */
            CHECK_API_RETURN(RK_MPI_AI_SetVqeAttr(nAiDev,
                                                  nChn,
                                                  pHandle->stExParam.nAoDev,
                                                  pHandle->stExParam.nAoChn,
                                                  &stVqeConfig));
            /* 使能AI的声音质量增强功能 */
            CHECK_API_RETURN(RK_MPI_AI_EnableVqe(nAiDev, nChn));
        }

        /* 是否启用aed声音质量增强功能 */
        if (pHandle->stNeedParam.bAedEnable == RK_TRUE)
        {
            AI_AED_CONFIG_S stAedConfig;
            stAedConfig.fSnrDB = pHandle->stExParam.fSnrDB; /* 语音信噪比阈值，大于则输出1 */
            stAedConfig.fLsdDB = pHandle->stExParam.fLsdDB; /* 超大声阈值，大于则输出1。最大为0dB。 */
            stAedConfig.s32Policy = pHandle->stExParam.nPolicy; /* 信噪比检测算法灵敏度，取值范围为[0，2]，值越大越灵敏，越容易满足检测阈值。默认取1。 */
            /* 设置AI声⾳质量增强相关属性 */
            CHECK_API_RETURN(RK_MPI_AI_SetAedAttr(nAiDev, nChn, &stAedConfig));
            /* 启⽤AI声⾳质量增强相关属性 */
            CHECK_API_RETURN(RK_MPI_AI_EnableAed(nAiDev, nChn));
        }

        /* 启用AI通道 */
        CHECK_API_RETURN(RK_MPI_AI_EnableChn(nAiDev, nChn));

        /* 是否启用重采样功能 */
        if (pHandle->stNeedParam.bResampleEnable)
        {
            /* 启用AI重采样 */
            CHECK_API_RETURN(RK_MPI_AI_EnableReSmp(nAiDev, nChn, pHandle->stNeedParam.enResampleRate));
        }
    }

    /* 设置设备音量 */
    CHECK_API_RETURN(RK_MPI_AI_SetVolume(nAiDev, pHandle->stExParam.nVolume));

#if 0
    /*用来测试ai采集文件保存*/
    AUDIO_SAVE_FILE_INFO_S save;
    save.bCfg = RK_TRUE;
    save.u32FileSize = 1024;
    snprintf(save.aFilePath, sizeof(save.aFilePath), "%s", "/root");
    snprintf(save.aFileName, sizeof(save.aFileName), "%s", "cap.pcm");
    RK_MPI_AI_SaveFile( nAiDev, 0, &save);
#endif
    return RK_SUCCESS;
}

/**
 * @brief   : ai反初始化
 * @param    {RkAi_S*} pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAi_uninit(RkAi_S *pHandle)
{
    if (NULL == pHandle)
    {
        return RK_FAILURE;
    }

    /*ai设备号*/
    AUDIO_DEV nAiDev = pHandle->stExParam.nDevId;

    for (int nChn = 0; nChn < pHandle->stNeedParam.nChnNum; nChn++)
    {
        if (pHandle->stNeedParam.bResampleEnable)
        {
            CHECK_API_RETURN(RK_MPI_AI_DisableReSmp(nAiDev, nChn));
        }
        if (pHandle->stNeedParam.bAedEnable == RK_TRUE)
        {
            /*禁用AI的声音质量增强功能*/
            CHECK_API_RETURN(RK_MPI_AI_DisableAed(nAiDev, nChn));
        }
        if (pHandle->stNeedParam.bVqeEnable == RK_TRUE)
        {
            /*禁用AI的声音质量增强功能*/
            CHECK_API_RETURN(RK_MPI_AI_DisableVqe(nAiDev, nChn));
        }
        /*禁用AI通道*/
        CHECK_API_RETURN(RK_MPI_AI_DisableChn(nAiDev, nChn));
    }

    /* 关闭回采设置 */
    CHECK_API_RETURN(RK_MPI_AMIX_SetControl(nAiDev, "SAI2 SDI0 Loopback I2S LR Switch", (char *) "Disable"));
    CHECK_API_RETURN(RK_MPI_AMIX_SetControl(nAiDev, "SAI2 SDI0 Loopback Switch", (char *) "Disable"));
    CHECK_API_RETURN(RK_MPI_AMIX_SetControl(nAiDev, "SAI2 SDI1 Loopback I2S LR Switch", (char *) "Disable"));
    CHECK_API_RETURN(RK_MPI_AMIX_SetControl(nAiDev, "SAI2 SDI1 Loopback Switch", (char *) "Disable"));

    /* 禁用AI设备 */
    CHECK_API_RETURN(RK_MPI_AI_Disable(nAiDev));

    return RK_SUCCESS;
}

/** 
 * @brief   : 获取音频帧
 * @param    {RkAi_S} *pHandle 句柄
 * @param    {int} nChn 通道号
 * @param    {AUDIO_FRAME_S} *pstFrame 帧数据指针
 * @param    {AEC_FRAME_S} *pstAecFrm 回声抵消参考帧结构体指针
 * @param    {int} nTimeoutMs 等待时间
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAi_get_frame(RkAi_S *pHandle, int nChn, AUDIO_FRAME_S *pstFrame, AEC_FRAME_S *pstAecFrm, int nTimeoutMs)
{
    if (NULL == pHandle || NULL == pstFrame || nTimeoutMs < -1)
    {
        return RK_FAILURE;
    }

    if(pHandle->stExParam.bEnableAec && pstAecFrm == NULL)
    {
        return RK_FAILURE;
    }

    /* 获取音频帧 */
    CHECK_API_RETURN(RK_MPI_AI_GetFrame(pHandle->stExParam.nDevId, nChn, pstFrame, pstAecFrm, nTimeoutMs));

    return RK_SUCCESS;
}

/**
 * @brief   : 获取ai采集pcm数据帧虚拟地址
 * @param    {AUDIO_FRAME_S} *pstFrame 帧数据指针
 * @return   {uint8_t *} RK_NULL:获取虚拟地址失败 非RK_NULL:有效的虚拟地址
 */
static uint8_t *rockitAi_get_virData(AUDIO_FRAME_S *pstFrame)
{
    return (uint8_t *) RK_MPI_MB_Handle2VirAddr(pstFrame->pMbBlk);
}

/**
 * @brief   : 释放音频帧
 * @param    {RkAi_S} *pHandle 句柄
 * @param    {int} nChn 通道号
 * @param    {AUDIO_FRAME_S} *pstFrame 帧数据指针
 * @param    {AEC_FRAME_S} *pstAecFrm 回声抵消参考帧结构体指针
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAi_release_frame(RkAi_S *pHandle, int nChn, AUDIO_FRAME_S *pstFrame, AEC_FRAME_S *pstAecFrm)
{
    if (NULL == pHandle || NULL == pstFrame)
    {
        return RK_FAILURE;
    }

    if (pHandle->stExParam.bEnableAec && pstAecFrm == NULL)
    {
        return RK_FAILURE;
    }

    /* 释放音频帧 */
    CHECK_API_RETURN(RK_MPI_AI_ReleaseFrame(pHandle->stExParam.nDevId, nChn, pstFrame, pstAecFrm));
    return RK_SUCCESS;
}

/**
 * @brief   : 设置AI声道模式
 * @param    {RkAi_S} *pHandle 句柄
 * @param    {AUDIO_TRACK_MODE_E} enTrackMode 声道模式
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAi_set_track_mode(RkAi_S *pHandle, AUDIO_TRACK_MODE_E enTrackMode)
{
    if (NULL == pHandle)
    {
        return RK_FAILURE;
    }

    if (enTrackMode != pHandle->stExParam.enTrackMode)
    {
        /* 设置AI声道模式 */
        CHECK_API_RETURN(RK_MPI_AI_SetTrackMode(pHandle->stExParam.nDevId, enTrackMode));
    }

    pHandle->stExParam.enTrackMode = enTrackMode;

    return RK_SUCCESS;
}

/**
 * @brief   : 是否使能噪声消除
 * @param    {RkAi_S} *pHandle 句柄
 * @param    {int} nChn 通道号
 * @param    {RK_BOOL} bEnableNr 是否使能
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitAi_whether_enable_vqe_nr(RkAi_S *pHandle, int nChn, RK_BOOL bEnableNr)
{
    if (NULL == pHandle || RK_FALSE == pHandle->stNeedParam.bVqeEnable)
    {
        return RK_FAILURE;
    }

    /*ai设备号*/
    AUDIO_DEV nAiDev = pHandle->stExParam.nDevId;
    /* AEC ao 设备号 */
    AUDIO_DEV nAoDev = pHandle->stExParam.nAoDev;
    /* AEC ao 通道号 */
    AO_CHN nAoChn = pHandle->stExParam.nAoChn;

    if (bEnableNr != pHandle->stExParam.bEnableNr)
    {
        /* 禁用AI的声音质量增强功能 */
        CHECK_API_RETURN(RK_MPI_AI_DisableVqe(nAiDev, nChn));
        AI_VQE_CONFIG_S stVqeConfig;
        AI_VQE_MOD_ENABLE_S stModEnable;
        /* 获取AI的声音质量增强功能相关属性 */
        CHECK_API_RETURN(RK_MPI_AI_GetVqeAttr(nAiDev, nChn, &stVqeConfig));
        rockitAi_vqeConfig_fill(pHandle, &stVqeConfig, &stModEnable, pHandle->stExParam.bEnableNr);
        /* 设置AI的声音质量增强功能相关属性 */
        CHECK_API_RETURN(RK_MPI_AI_SetVqeAttr(nAiDev, nChn, nAoDev, nAoChn, &stVqeConfig));
        /* 启用AI声音质量增强使能模块 */
        CHECK_API_RETURN(RK_MPI_AI_SetVqeModuleEnable(nAiDev, nChn, &stModEnable));
        /* 使能AI的声音质量增强功能 */
        CHECK_API_RETURN(RK_MPI_AI_EnableVqe(nAiDev, nChn));
    }

    pHandle->stExParam.bEnableNr = bEnableNr;

    return RK_SUCCESS;
}

RkAi_S* rockitAi_alloc( RkAiNeedParam_S stNeedParam  )
{
    RkAi_S *pHandle = (RkAi_S *) malloc(sizeof(RkAi_S));

    /**************************必需参数*************************/
    pHandle->stNeedParam = stNeedParam;

    /**************************功能参数*************************/
    pHandle->stExParam.nDevId               = 0;
    pHandle->stExParam.eDevBitWidth         = AUDIO_BIT_WIDTH_16;
    pHandle->stExParam.nDevChannels         = 2;
    pHandle->stExParam.nDevSampleRate       = 16000;
    pHandle->stExParam.nVolume              = 100;
    pHandle->stExParam.enTrackMode          = AUDIO_TRACK_NORMAL;
    pHandle->stExParam.bEnableAec           = RK_FALSE;
    pHandle->stExParam.bEnableNr            = RK_FALSE;
    pHandle->stExParam.nVqeGapMs            = 16;
    memset(pHandle->stExParam.aVqeCfgPath, 0, sizeof(pHandle->stExParam.aVqeCfgPath));
    pHandle->stExParam.s64RefChannelType    = 0x2;
    pHandle->stExParam.s64RecChannelType    = 0x1;
    pHandle->stExParam.s64ChannelLayoutType = 0x3;
    pHandle->stExParam.fSnrDB               = 0.5f;
    pHandle->stExParam.fLsdDB               = 0.5f;
    pHandle->stExParam.nPolicy              = 1;
    pHandle->stExParam.nAoDev               = 0;
    pHandle->stExParam.nAoChn               = 0;

    /**************************列表参数*************************/
    pHandle->rockitAi_init                  = rockitAi_init;
    pHandle->rockitAi_uninit                = rockitAi_uninit;
    pHandle->rockitAi_get_frame             = rockitAi_get_frame;
    pHandle->rockitAi_get_virData           = rockitAi_get_virData;
    pHandle->rockitAi_release_frame         = rockitAi_release_frame;
    pHandle->rockitAi_set_track_mode        = rockitAi_set_track_mode;
    pHandle->rockitAi_whether_enable_vqe_nr = rockitAi_whether_enable_vqe_nr;

    return pHandle;
}

int rockitAi_release(RkAi_S *pHandle)
{
    if (pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
    return RK_SUCCESS;
}
