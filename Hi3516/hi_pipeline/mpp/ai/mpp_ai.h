/**
 * @FilePath     : mpp_ai.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-28 14:24:42
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-11 21:10:51
 * @Description  : 海思ai模块封装
 */

#ifndef _MPP_AI_H_
#define _MPP_AI_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "ss_mpi_audio.h"
#include "ot_common_aio.h"
#include "ot_acodec.h"
#include "securec.h"

/*vqe声音质量增强算法类型*/
typedef enum AudioVqeType
{
    AUDIO_VQE_TYPE_RECORD = 0,  /*主要用于Mobile Camera录音场景，适用于高音质语音处理场景,其支持16/48kHz采样率，16bit位宽，单/双声道*/
    AUDIO_VQE_TYPE_TALK,        /*用于录像机语音对讲场景，适用于低音质语音处理场景（8/16kHz采样率，16bit位宽，单声道），支持双向对讲场景*/
    AUDIO_VQE_TYPE_TALKV2,      /*TalkVQE的升级版本，其支持16kHz采样率，16bit位宽，单/双声道。对于支持TalkVQEV2和其他方式的解决方案来说，录像机场景推荐优先使用TalkVQEV2*/
    AUDIO_VQE_TYPE_MAX,
} AudioVqeType_E;

/*海思 ai必需参数*/
typedef struct _HiAiNeedParam_S
{
    /*ai通道号*/
    int nChn;
    /*输入参数，设置进采集数据*/
    /*采样精度*/
    ot_audio_bit_width enBitWidth;
    /*采样率*/
    ot_audio_sample_rate enSampleRate;
    /*声道*/
    ot_audio_snd_mode enSoundMode;
    /*缓存帧数目*/
    uint32_t u32FrameNum;
    /*每帧的采样点个数*/
    uint32_t u32PointNumPerFrame;
    /*通道数*/
    int nChnNum;
    /*输出参数*/
    /*是否重采样*/
    int nResampleEnable;
    /*重采样采样率*/
    ot_audio_sample_rate enResampleRate;
    /*vqe声音质量增强是否使能*/
    int nVqeEnable;
    /*vqe声音质量增强算法类型*/
    AudioVqeType_E enVqeType;
    // info 启用 AEC 需要AO模块设备号、通道号
    /* ao 设备号 */
    int nAoDev;
    /*ao通道号*/
    int nAoChn;
} HiAiNeedParam_S;

typedef struct _HiAiExParam_S
{
    /*ai设备号*/
    int nDevId;
    /* 是否使能噪声消除 */
    td_bool bEnableNr;
    /* 音频设备声道模式类型 */
    ot_audio_track_mode enTrackMode;
} HiAiExParam_S;

typedef struct _HiAi_S HiAi_S;
struct _HiAi_S
{
    //info /**********************必需参数***************************/
    HiAiNeedParam_S stNeedParam;
    HiAiExParam_S stExParam;
    //info /**********************辅助参数***************************/
    // int nFd;
    
    //info /**********************功能列表***************************/
    /*获取通道采集帧*/
    int (*mppAi_getFrame)(HiAi_S *pHandle, int nChn, ot_audio_frame *pFrame, ot_aec_frame *pAecFrame, int nTimeoutMs);

    /*释放通道采集帧*/
    int (*mppAi_releaseFrame)(HiAi_S *pHandle, int nChn, ot_audio_frame *pFrame, ot_aec_frame *pAecFrame);

    /* 获取采集音频原始帧 */
    int (*mppAi_getRawFrame)(HiAi_S *pHandle, int nChn, ot_audio_frame *pFrame, int nTimeoutMs);

    /* 释放采集音频原始帧 */
    int (*mppAi_releaseRawFrame)(HiAi_S *pHandle, int nChn, ot_audio_frame *pFrame);

    /*初始化ai*/
    int (*mppAi_init)(HiAi_S *pHandle);

    /*反初始化ai*/
    int (*mppAi_uninit)(HiAi_S *pHandle);

    /* 是否使能vqe声音质量增强 */
    int (*mppAi_whether_enable_vqe)(HiAi_S *pHandle, int nChn, td_bool nVqeEnable);

    /* 是否使能录音噪声消除 */
    int (*mppAi_whether_enable_vqe_rnr)(HiAi_S *pHandle, int nChn, td_bool nEnableRnr);

    /* 设置AI声道模式 */
    int (*mppAi_set_track_mode)(HiAi_S *pHandle, ot_audio_track_mode enTrackMode);

};

/**
 * @brief       : 分配ai句柄
 * @author      : zhouzirui
 * @param        {HiAiNeedParam_S} stNeedParam：ai必须参数
 * @return       {*}成功返回句柄，失败返回NULL
 */
HiAi_S *mppAi_alloc(HiAiNeedParam_S stNeedParam);

/**
 * @brief       : 释放ai句柄
 * @author      : zhouzirui
 * @param        {HiAi_S} *pHandle：句柄
 * @return       {*}
 */
void mppAi_release(HiAi_S *pHandle);

#ifdef __cplusplus
}
#endif
#endif
