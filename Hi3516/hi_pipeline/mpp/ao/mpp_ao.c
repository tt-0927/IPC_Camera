/**
 * @FilePath     : mpp_ao.c
 * @Author       : zhouzirui
 * @Date         : 2025-03-31 17:30:59
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-04 15:06:39
 * @Description  : 海思ao模块封装
 */

#include "mpp_ao.h"
#include "mpi_common.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>

int convert_volume(int nVolume, int nMinVolume, int nMaxVolume)
{
    /* 参数验证 */
    if (nVolume < 0)
    {
        nVolume = 0;
    }
    else if (nVolume > 100)
    {
        nVolume = 100;
    }
    /* 将0-100映射到-121到6的范围 */
    int converted_volume = nMinVolume + (nVolume * (nMaxVolume - nMinVolume)) / 100;
    /* 确保结果在有效范围内 */
    if (converted_volume < nMinVolume)
    {
        converted_volume = nMinVolume;
    }
    else if (converted_volume > nMaxVolume)
    {
        converted_volume = nMaxVolume;
    }
    return converted_volume;
}

/**
 * @brief       : ao初始化
 * @author      : zhouzirui
 * @param        {HiAo_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int mppAo_init(HiAo_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    /*ao设备号*/
    ot_audio_dev nAoDev = pHandle->stNeedParam.nDevId;
    /*ao通道号*/
    // ot_ao_chn nAoChn = pHandle->stNeedParam.nChn;

    ot_aio_attr stAioAttr = {
        .sample_rate = pHandle->stNeedParam.enSampleRate,
        .bit_width = pHandle->stNeedParam.enBitWidth,
        .work_mode = OT_AIO_MODE_I2S_MASTER,
        .snd_mode = pHandle->stNeedParam.enSoundMode,
        .expand_flag = 0,
        .frame_num = pHandle->stNeedParam.u32FrameNum,
        .point_num_per_frame = pHandle->stNeedParam.u32PointNumPerFrame,
        .chn_cnt = (pHandle->stNeedParam.enSoundMode == OT_AUDIO_SOUND_MODE_MONO)     ? (td_u32) 1
                   : (pHandle->stNeedParam.enSoundMode == OT_AUDIO_SOUND_MODE_STEREO) ? 2
                                                                                      : 1,
        .clk_share = 1,
        .i2s_type = OT_AIO_I2STYPE_INNERCODEC,
    };
    /*设置AO设备属性*/
    CHECK_API_RETURN(ss_mpi_ao_set_pub_attr(nAoDev,&stAioAttr));
    /*启用AO设备*/
    CHECK_API_RETURN(ss_mpi_ao_enable(nAoDev));
    for (int nChn = 0; nChn < pHandle->stNeedParam.nChnNum; nChn++)
    {
        /*启用AO通道*/
        mpi_ao_log("nChn:%d stAioAttr.chn_cnt:%d", nChn, stAioAttr.chn_cnt);
        CHECK_API_RETURN(ss_mpi_ao_enable_chn(nAoDev, nChn));

        /*是否启用重采样功能*/
        if(pHandle->stNeedParam.nResampleEnable)
        {
            /*启用AO重采样*/
            CHECK_API_RETURN(ss_mpi_ao_enable_resample(nAoDev,nChn,pHandle->stNeedParam.enResampleRate));
        }
        /*是否启用vqe声音质量增强功能*/
        if (pHandle->stNeedParam.nVqeEnable == TD_TRUE)
        {
            ot_ao_vqe_cfg stAoVqeCfg;
            stAoVqeCfg.open_mask = OT_AO_VQE_MASK_HPF | OT_AO_VQE_MASK_ANR | OT_AO_VQE_MASK_AGC | OT_AO_VQE_MASK_EQ;
            stAoVqeCfg.work_sample_rate = pHandle->stNeedParam.enSampleRate;
            stAoVqeCfg.frame_sample = pHandle->stNeedParam.u32PointNumPerFrame;
            stAoVqeCfg.work_state = OT_VQE_WORK_STATE_NOISY;
            stAoVqeCfg.hpf_cfg.usr_mode = TD_FALSE;
            stAoVqeCfg.anr_cfg.usr_mode = TD_FALSE;
            stAoVqeCfg.agc_cfg.usr_mode = TD_FALSE;
            td_s8 gain_db[OT_VQE_EQ_BAND_NUM] = {
                -6, // 100Hz     - 稍微衰减，避免低频轰鸣
                -2, // 200Hz     - 微升，增加人声厚度
                0,  // 250Hz     - 保持或微升
                -2, // 350Hz     - 防止浑浊
                3,  // 500Hz     - 提升人声主体
                3,  // 800Hz     - 微升，增强中频
                4,  // 1.2kHz    - 增加清晰度
                6,  // 2.5kHz    - 重点提升辅音清晰度
                4,  // 4kHz      - 提升明亮度
                2,  // 8kHz      - 若采样率支持可+2，否则保持0（如非16kHz+可设为0）
            };
            memcpy_s(stAoVqeCfg.eq_cfg.gain_db, sizeof(stAoVqeCfg.eq_cfg.gain_db), gain_db, sizeof(gain_db));
            /*设置AO的声音质量增强功能相关属性*/
            CHECK_API_RETURN(ss_mpi_ao_set_vqe_attr(nAoDev, nChn, &stAoVqeCfg));
            /*使能AO的声音质量增强功能*/
            CHECK_API_RETURN(ss_mpi_ao_enable_vqe(nAoDev, nChn));
        }
    }

    /* 淡入淡出结构体 */
    ot_audio_fade stFade;
    stFade.fade = TD_TRUE;
    stFade.fade_in_rate = OT_AUDIO_FADE_RATE_64;
    stFade.fade_out_rate = OT_AUDIO_FADE_RATE_64;
    if (pHandle->stExParam.nVolume != 0)
    {
        /* 设置AO设备静音 */
        CHECK_API_RETURN(ss_mpi_ao_set_mute(nAoDev, TD_FALSE, &stFade));
        int nConvertVolume = convert_volume(pHandle->stExParam.nVolume, pHandle->stExParam.nMinVolume, pHandle->stExParam.nMaxVolume);
        /*设置AO设备音量大小 音频设备音量大小（以dB为单位）取值范围：[-121, 6]*/
        CHECK_API_RETURN(ss_mpi_ao_set_volume(nAoDev, nConvertVolume));
    }
    else
    {
        /* 设置AO设备静音 */
        CHECK_API_RETURN(ss_mpi_ao_set_mute(nAoDev, TD_TRUE, &stFade));
    }

    return TD_SUCCESS;
}

/**
 * @brief       : ao去初始化
 * @author      : zhouzirui
 * @param        {HiAo_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int mppAo_uninit(HiAo_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    /*ao设备号*/
    ot_audio_dev nAoDev = pHandle->stNeedParam.nDevId;
    /*ao通道号*/
    // ot_ao_chn nAoChn = pHandle->stNeedParam.nChn;

    for (int nChn = 0; nChn < pHandle->stNeedParam.nChnNum; nChn++)
    {
        if (pHandle->stNeedParam.nVqeEnable == TD_TRUE)
        {
            /*禁用AO的声音质量增强功能*/
            CHECK_API_RETURN(ss_mpi_ao_disable_vqe(nAoDev,nChn));
        }
        if(pHandle->stNeedParam.nResampleEnable)
        {
            /*禁用AO重采样*/
            CHECK_API_RETURN(ss_mpi_ao_disable_resample(nAoDev,nChn));
        }
        /*禁用AO通道*/
        CHECK_API_RETURN(ss_mpi_ao_disable_chn(nAoDev,nChn));
    }
    /*禁用AO设备*/
    CHECK_API_RETURN(ss_mpi_ao_disable(nAoDev));

    return TD_SUCCESS;
}

/**
 * @brief       : 获取采集帧
 * @author      : zhouzirui
 * @param        {HiAo_S} *pHandle：句柄
 * @param        {int} nChn：通道号
 * @param        {ot_aodeo_frame_info} *pFrame：帧数据信息指针
 * @param        {int} nTimeoutMs：等待时间
 * @return       {*}成功返回0,失败返回-1
 */
static int mppAo_sendFrame(HiAo_S *pHandle, int nChn, ot_audio_frame *pFrame, int nTimeoutMs)
{
    if (NULL == pHandle || NULL == pFrame || nTimeoutMs < -1)
    {
        return TD_FAILURE;
    }
    CHECK_API_RETURN(ss_mpi_ao_send_frame(pHandle->stNeedParam.nDevId, nChn, pFrame, nTimeoutMs));
    return TD_SUCCESS;
}

static int mppAo_setVolume(HiAo_S *pHandle, int nVolume) 
{
    if (NULL == pHandle) 
    {
        return TD_FAILURE;
    }

    if (nVolume < 0 || nVolume > 100) 
    {
        printf("Invalid volume: %d, should be in range [0, 100]\n", nVolume);
        return TD_FAILURE;
    }

    /*ao设备号*/
    ot_audio_dev nAoDev = pHandle->stNeedParam.nDevId;

    pHandle->stExParam.nVolume = nVolume;
    /* 淡入淡出结构体 */
    ot_audio_fade stFade;
    stFade.fade = TD_TRUE;
    stFade.fade_in_rate = OT_AUDIO_FADE_RATE_64;
    stFade.fade_out_rate = OT_AUDIO_FADE_RATE_64;
    if (pHandle->stExParam.nVolume != 0)
    {
        /* 设置AO设备静音 */
        CHECK_API_RETURN(ss_mpi_ao_set_mute(nAoDev, TD_FALSE, &stFade));
        int nConvertVolume = convert_volume(pHandle->stExParam.nVolume, pHandle->stExParam.nMinVolume, pHandle->stExParam.nMaxVolume);
        /* 设置AO设备音量大小 音频设备音量大小（以dB为单位）取值范围：[-121, 6] */
        CHECK_API_RETURN(ss_mpi_ao_set_volume(nAoDev, nConvertVolume));
    }
    else
    {
        /* 设置AO设备静音 */
        CHECK_API_RETURN(ss_mpi_ao_set_mute(nAoDev, TD_TRUE, &stFade));
    }

    return TD_SUCCESS;
}

HiAo_S *mppAo_alloc(HiAoNeedParam_S stNeedParam)
{
    HiAo_S *pHandle = (HiAo_S *)malloc(sizeof(HiAo_S));
    memset(pHandle, 0, sizeof(HiAo_S));

    //info /**********************必需参数***************************/
    pHandle->stNeedParam.nDevId                 = stNeedParam.nDevId;
    pHandle->stNeedParam.nChn                   = stNeedParam.nChn;
    pHandle->stNeedParam.enBitWidth             = stNeedParam.enBitWidth;
    pHandle->stNeedParam.enSampleRate           = stNeedParam.enSampleRate;
    pHandle->stNeedParam.enSoundMode            = stNeedParam.enSoundMode;
    pHandle->stNeedParam.u32FrameNum            = stNeedParam.u32FrameNum;
    pHandle->stNeedParam.u32PointNumPerFrame    = stNeedParam.u32PointNumPerFrame;
    pHandle->stNeedParam.nChnNum                = stNeedParam.nChnNum;
    pHandle->stNeedParam.nResampleEnable        = stNeedParam.nResampleEnable;
    pHandle->stNeedParam.enResampleRate         = stNeedParam.enResampleRate;
    pHandle->stNeedParam.nVqeEnable             = stNeedParam.nVqeEnable;
    //info /**********************功能参数***************************/
    pHandle->stExParam.nVolume                  = 100;
    pHandle->stExParam.nMinVolume               = -121;
    pHandle->stExParam.nMaxVolume               = 6;

    //info /**********************函数列表***************************/
    pHandle->mppAo_init                 = mppAo_init;
    pHandle->mppAo_uninit               = mppAo_uninit;
    pHandle->mppAo_sendFrame            = mppAo_sendFrame;
    pHandle->mppAo_setVolume            = mppAo_setVolume;
    return pHandle;
}

void mppAo_release(HiAo_S *pHandle)
{
    if (pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
}