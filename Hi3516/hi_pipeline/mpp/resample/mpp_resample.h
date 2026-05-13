/**
 * @FilePath     : mpp_resample.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-08-29 10:15:20
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-19 14:02:28
 * @Description  : 海思音频重采样模块封装
 */

#ifndef _MPP_RESAMPLE_H_
#define _MPP_RESAMPLE_H_
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
#include "ss_resample.h"
#include "ot_common_aio.h"

/*海思音频重采样必需参数*/
typedef struct _HiResampleNeedParam_S
{
    /* 输入采样率 */
    ot_audio_sample_rate enInSampleRate;
    /* 输出采样率 */
    ot_audio_sample_rate enOutSampleRate;
    /*声道数*/
    td_s32 nSoundNum;
} HiResampleNeedParam_S;

// typedef struct _HiResampleExParam_S
// {
// } HiResampleExParam_S;

typedef struct _HiResample_S HiResample_S;
struct _HiResample_S
{
    //info /**********************必需参数***************************/
    HiResampleNeedParam_S stNeedParam;
    // HiResampleExParam_S stExParam;
    //info /**********************辅助参数***************************/
    /* 重采样句柄 */
    td_void *pResHandle;

    //info /**********************功能列表***************************/

    /* 初始化音频重采样 */
    int (*mppResample_init)(HiResample_S *pHandle);

    /* 反初始化音频重采样 */
    int (*mppResample_uninit)(HiResample_S *pHandle);

    /* 处理一帧重采样数据 */
    int (*mppResample_process)(HiResample_S *pHandle, const td_s16 *pInBuf, td_s32 nInSamples, td_s16 *pOutBuf, td_s32 *pOutBufSize);

    /* 获取最大输出采样点数(每一声道) */
    int (*mppResample_get_max_output_num)(HiResample_S *pHandle, td_s32 nInSamples);
};

/**
 * @brief       : 分配音频重采样句柄
 * @param        {HiResampleNeedParam_S} stNeedParam：ai必须参数
 * @return       {HiResample_S*} 成功返回句柄，失败返回NULL
 */
HiResample_S *mppResample_alloc(HiResampleNeedParam_S stNeedParam);

/**
 * @brief       : 释放音频重采样句柄
 * @param        {HiResample_S} *pHandle：句柄
 */
void mppResample_release(HiResample_S *pHandle);

#ifdef __cplusplus
}
#endif
#endif
