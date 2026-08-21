/**
 * @FilePath     : mpp_ao.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-31 17:31:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-23 11:31:22
 * @Description  : 海思ao模块封装
 */

#ifndef _MPP_AO_H_
#define _MPP_AO_H_
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
#include "securec.h"

/*海思 ao必需参数*/
typedef struct _HiAoNeedParam_S
{
    /*ao设备号*/
    int nDevId;
    /*ao通道号*/
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
} HiAoNeedParam_S;

typedef struct _HiAoExParam_S
{
    /* 音量大小 [0,100] */
    int nVolume;
    /* ao 输出音量最小值 */
    int nMinVolume;
    /* ao 输出音量最大值 */
    int nMaxVolume;
    /* 指数音量曲线 BASE，取值大于 1 时低中音量段衰减更明显 */
    double dVolumeCurveBase;
} HiAoExParam_S;

typedef struct _HiAo_S HiAo_S;
struct _HiAo_S
{
    //info /**********************必需参数***************************/
    HiAoNeedParam_S stNeedParam;
    HiAoExParam_S stExParam;
    //info /**********************辅助参数***************************/
    // int nFd;
    
    //info /**********************功能列表***************************/
    /*发送数据帧*/
    int (*mppAo_sendFrame)(HiAo_S *pHandle, int nChn, ot_audio_frame *pFrame, int nTimeoutMs);

    /*初始化ao*/
    int (*mppAo_init)(HiAo_S *pHandle);

    /*反初始化ao*/
    int (*mppAo_uninit)(HiAo_S *pHandle);

    /* 设置ao音量 */
    int (*mppAo_setVolume)(HiAo_S *pHandle,int nVolume);

    /**
     * @brief   : 等待 AO 通道缓冲区完全排空（chn_busy_num == 0）
     * @param    {HiAo_S} *pHandle：句柄
     * @param    {int} nChn：通道号
     * @param    {int} nTimeoutMs：最大等待时间（ms），-1 表示无限等待
     * @return   {int} 0：成功排空，-1：超时或错误
     */
    int (*mppAo_waitDrained)(HiAo_S *pHandle, int nChn, int nTimeoutMs);
};

/**
 * @brief       : 分配ao句柄
 * @author      : zhouzirui
 * @param        {HiAoNeedParam_S} stNeedParam：ao必须参数
 * @return       {*}成功返回句柄，失败返回NULL
 */
HiAo_S *mppAo_alloc(HiAoNeedParam_S stNeedParam);

/**
 * @brief       : 释放ao句柄
 * @author      : zhouzirui
 * @param        {HiAo_S} *pHandle：句柄
 * @return       {*}
 */
void mppAo_release(HiAo_S *pHandle);

#ifdef __cplusplus
}
#endif
#endif
