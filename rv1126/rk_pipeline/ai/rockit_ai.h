/**
 * @FilePath     : rockit_ai.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-12-02 09:50:52
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-15 14:46:29
 * @Description  : rockit ai模块封装
 */

#ifndef _ROCKIT_AI_H
#define _ROCKIT_AI_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>

#include "mpi_common.h"

#include "rk_mpi_ai.h"
#include "rk_mpi_amix.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_mb.h"
#include "rk_common.h"
#include "rk_comm_aio.h"
#include "rk_debug.h"

typedef struct _RkAiNeedParam_S_
{
    /*声卡名*/
    char aDevName[64];
    /*输入参数，设置进采集数据*/
    /*采样精度*/
    AUDIO_BIT_WIDTH_E enbitWidth;
    /*采样率*/
    AUDIO_SAMPLE_RATE_E enSampleRate;
    /*声道*/
    AUDIO_SOUND_MODE_E enSoundMode;
    /*缓存帧数目*/
    uint32_t u32FrameNum;
    /*每帧的采样点个数*/
    uint32_t u32PointNumPerFrame;
    /*通道数*/
    int nChnNum;

    /*输出参数*/
    /*是否重采样*/
    RK_BOOL bResampleEnable;
    /*重采样采样率*/
    AUDIO_SAMPLE_RATE_E enResampleRate;
    /*vqe声音质量增强是否使能*/
    RK_BOOL bVqeEnable;
    /* AI声音质量增强是否使能 */
    RK_BOOL bAedEnable;
} RkAiNeedParam_S;

typedef struct _RkAiExParam_S_
{
    /* ai设备号 */
    int nDevId;
    /* 设备播放位数  默认16 */
    AUDIO_BIT_WIDTH_E eDevBitWidth;
    /* 设备采集声道数  默认2 */
    int nDevChannels;
    /* 设备采集采样率  默认16K */
    int nDevSampleRate;
    /*ai音量*/
    int nVolume;
    /* 音频设备声道模式类型 */
    AUDIO_TRACK_MODE_E enTrackMode;
    /* AI Vqe */
    /* 是否使能回声消除 */
    RK_BOOL bEnableAec;
    /* 是否使能噪声消除 */
    RK_BOOL bEnableNr;
    /* Vqe每帧处理的音频间隔时间 只能为16ms或者10ms */
    int nVqeGapMs;
    /* Vqe配置文件路径 */
    char aVqeCfgPath[64];
    /* 回采数据的通道类型 例如：0b00000010 */
    RK_S64 s64RefChannelType;
    /* 录音数据的通道类型 例如：0b00000001 */
    RK_S64 s64RecChannelType;
    /* 输入音频通道类型 例如：0b00000011 */
    RK_S64 s64ChannelLayoutType;
    /* AI声音质量增强 */
    /* 语音信噪比阈值，大于则输出1 */
    float fSnrDB;
    /* 超大声阈值，大于则输出1。最大为0dB */
    float fLsdDB;
    /* 信噪比检测算法灵敏度，取值范围为[0，2]，值越大越灵敏，越容易满足检测阈值。默认取1 */
    int nPolicy;
    // info 启用 AEC 需要AO模块设备号、通道号
    /* ao 设备号 */
    int nAoDev;
    /*ao通道号*/
    int nAoChn;
} RkAiExParam_S;

typedef struct _RkAi_S_ RkAi_S;
struct _RkAi_S_
{
    // info /**********************必需参数***************************/
    RkAiNeedParam_S stNeedParam;
    RkAiExParam_S stExParam;

    // info /**********************辅助参数***************************/

    // info /**********************功能列表***************************/

    /* ai初始化 */
    int (*rockitAi_init)(RkAi_S *pHandle);

    /* ai反初始化 */
    int (*rockitAi_uninit)(RkAi_S *pHandle);

    /* 获取音频帧 */
    int (*rockitAi_get_frame)(RkAi_S *pHandle, int nChn, AUDIO_FRAME_S *pstFrame, AEC_FRAME_S *pstAecFrm, int nTimeoutMs);

    /* 获取ai采集pcm数据帧虚拟地址 */
    uint8_t *(*rockitAi_get_virData)(AUDIO_FRAME_S *pstFrame);

    /* 释放音频帧 */
    int (*rockitAi_release_frame)(RkAi_S *pHandle, int nChn, AUDIO_FRAME_S *pstFrame, AEC_FRAME_S *pstAecFrm);

    /* 设置AI声道模式 */
    int (*rockitAi_set_track_mode)(RkAi_S *pHandle, AUDIO_TRACK_MODE_E enTrackMode);

    /* 是否使能噪声消除 */
    int (*rockitAi_whether_enable_vqe_nr)(RkAi_S *pHandle, int nChn, RK_BOOL bEnableNr);

};

/**
 * @brief   : 分配ai句柄
 * @param    {RkAiNeedParam_S} stNeedParam ai必须参数
 * @return   {RkAi_S *} 成功返回句柄，失败返回NULL
 */
RkAi_S *rockitAi_alloc(RkAiNeedParam_S stNeedParam);

/**
 * @brief   : 释放ai句柄
 * @param    {RkAi_S} *pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
int rockitAi_release(RkAi_S *pHandle);

#ifdef __cplusplus
}
#endif
#endif
