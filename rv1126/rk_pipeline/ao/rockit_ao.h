/**
 * @FilePath     : rockit_ao.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-12-02 09:50:52
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-15 19:23:56
 * @Description  : rockit ao模块封装
 */

#ifndef _ROCKIT_AO_H
#define _ROCKIT_AO_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>

#include "mpi_common.h"

#include "rk_mpi_ao.h"
#include "rk_debug.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_mb.h"
#include "rk_common.h"
#include "rk_comm_aio.h"

typedef struct _RkAoNeedParam_S_
{
    /*声卡名*/
    char aDevName[64];
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

    /*是否重采样*/
    RK_BOOL bResampleEnable;
    /*重采样采样率*/
    AUDIO_SAMPLE_RATE_E enResampleRate;
    /*vqe声音质量增强是否使能*/
    RK_BOOL bVqeEnable;
} RkAoNeedParam_S;

typedef struct _RkAoExParam_S_
{
    /* ai设备号 */
    int nDevId;
    /* 设备播放位数  默认16 */
    AUDIO_BIT_WIDTH_E eDevBitWidth;
    /* 设备声道数  默认2 */
    int nDevChannels;
    /* 设备采样率  默认16K */
    int nDevSampleRate;
    /*ao音量*/
    int nVolume;
    /* 音频设备声道模式类型 */
    AUDIO_TRACK_MODE_E enTrackMode;
    /* AO Vqe */
    /* Vqe每帧处理的音频间隔时间 只能为16ms或者10ms */
    int nVqeGapMs;
    /* Vqe配置文件路径 */
    char aVqeCfgPath[64];
} RkAoExParam_S;

typedef struct _RkAo_S_ RkAo_S;
struct _RkAo_S_
{
    // info /**********************必需参数***************************/
    RkAoNeedParam_S stNeedParam;
    RkAoExParam_S stExParam;

    // info /**********************辅助参数***************************/

    // info /**********************功能列表***************************/
    /*ao初始化*/
    int (*rockitAo_init)(RkAo_S *pHandle);

    /*ao反初始化*/
    int (*rockitAo_uninit)(RkAo_S *pHandle);

    /* 发送pcm数据到ao */
    int (*rockitAo_send_pcmData)(RkAo_S *pHandle, int nChn, uint8_t *pData, int nSize, int nTimeOut);

    /* 清除 AO 通道中当前的音频数据缓存 */
    int (*rockitAo_clean_chnBuffer)(RkAo_S *pHandle, int nChn);

    /* 查询 AO 通道中当前的音频数据缓存状态 */
    int (*rockitAo_get_chnStat)(RkAo_S *pHandle, int nChn, AO_CHN_STATE_S *pstStat);

    /* 设置 AO 设备音量大小 */
    int (*rockitAo_set_volume)(RkAo_S *pHandle, int nVolume);

    /* 设置 AO 设备静音状态 */
    int (*rockitAo_set_mutex)(RkAo_S *pHandle, RK_BOOL bMutex);

    /* 设置AO声道模式 */
    int (*rockitAo_set_track_mode)(RkAo_S *pHandle, AUDIO_TRACK_MODE_E eTrackMode);
};

/**
 * @brief   : 分配ao句柄
 * @param    {RkAoNeedParam_S} stNeedParam ai必须参数
 * @return   {RkAo_S *} 成功返回句柄，失败返回NULL
 */
RkAo_S *rockitAo_alloc(RkAoNeedParam_S stNeedParam);

/**
 * @brief   : 释放ao句柄
 * @param    {RkAo_S} *pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
int rockitAo_release(RkAo_S *pHandle);

#ifdef __cplusplus
}
#endif
#endif
