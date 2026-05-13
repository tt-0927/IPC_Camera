/**
 * @FilePath     : mpp_resample.c
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-08-29 10:15:20
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-19 15:21:45
 * @Description  : 海思音频重采样模块封装
 */

#include "mpp_resample.h"
#include "mpi_common.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>

/**
 * @brief       : 音频重采样初始化
 * @param        {HiResample_S} *pHandle 句柄
 * @return       {int} 成功返回0,失败返回-1
 */
static int mppResample_init(HiResample_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    /* 输入采样率 取值范围：8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000 */
    td_s32 in_rate = pHandle->stNeedParam.enInSampleRate;
    /* 输出采样率 取值范围：8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000。 */
    td_s32 out_rate = pHandle->stNeedParam.enOutSampleRate;
    /* 处理声道数 取值范围：1, 2 */
    td_s32 chn_num = pHandle->stNeedParam.nSoundNum;

    /* 创建一个重采样模块 */
    pHandle->pResHandle =  ss_resample_create(in_rate, out_rate, chn_num);
    if(pHandle->pResHandle == NULL)
    {
        mpi_resample_log("创建一个重采样模块失败");
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

/**
 * @brief       : 音频重采样去初始化
 * @param        {HiResample_S} *pHandle 句柄
 * @return       {int} 成功返回0,失败返回-1
 */
static int mppResample_uninit(HiResample_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    
    /* 销毁一个重采样模块实例 */
    ss_resample_destroy(pHandle->pResHandle);

    return TD_SUCCESS;
}

/**
 * @brief   : 处理一帧重采样数据
 * @param    {HiResample_S} *pHandle 句柄
 * @param    {td_s16} *pInBuf 输入数据buf指针
 * @param    {td_s32} nInSamples 输入数据采样点数 取值范围：[0, 2048]
 * @param    {td_s16} *pOutBuf 输出数据buf指针
 * @param    {td_s32} *pOutBufSize 输出数据采样点数
 * @return   {int} 成功返回0,失败返回-1
 */
static int mppResample_process(HiResample_S *pHandle, const td_s16 *pInBuf, td_s32 nInSamples, td_s16 *pOutBuf, td_s32 *pOutBufSize)
{
    if (NULL == pHandle || NULL == pInBuf  || NULL == pOutBuf)
    {
        return TD_FAILURE;
    }

    /* 处理一帧重采样数据 */
    td_s32 nOutBufSize = ss_resample_process(pHandle->pResHandle, pInBuf, nInSamples, pOutBuf);
    if (nOutBufSize <= TD_SUCCESS)
    {
        return TD_FAILURE;
    }
    *pOutBufSize = nOutBufSize;

    return TD_SUCCESS;
}

/**
 * @brief   : 获取最大输出采样点数(每一声道)
 * @param    {HiResample_S} *pHandle 句柄
 * @param    {td_s32} nInSamples 每一声道输入采样点数
 * @return   {int} 成功：非负值,失败：负值
 */
static int mppResample_get_max_output_num(HiResample_S *pHandle, td_s32 nInSamples)
{
    if (NULL == pHandle || nInSamples > MAXFRAMESIZE)
    {
        return TD_FAILURE;
    }

    /* 获取最大输出采样点数(每一声道) */
    return ss_resample_get_max_output_num(pHandle->pResHandle, nInSamples);
}

HiResample_S *mppResample_alloc(HiResampleNeedParam_S stNeedParam)
{
    HiResample_S *pHandle = (HiResample_S *)malloc(sizeof(HiResample_S));
    memset(pHandle, 0, sizeof(HiResample_S));

    //info /**********************必需参数***************************/
    pHandle->stNeedParam.enInSampleRate     = stNeedParam.enInSampleRate;
    pHandle->stNeedParam.enOutSampleRate    = stNeedParam.enOutSampleRate;
    pHandle->stNeedParam.nSoundNum          = stNeedParam.nSoundNum;
    //info /**********************功能参数***************************/

    pHandle->pResHandle = NULL;
    //info /**********************函数列表***************************/
    pHandle->mppResample_init                   = mppResample_init;
    pHandle->mppResample_uninit                 = mppResample_uninit;
    pHandle->mppResample_process                = mppResample_process;
    pHandle->mppResample_get_max_output_num     = mppResample_get_max_output_num;

    return pHandle;
}

void mppResample_release(HiResample_S *pHandle)
{
    if (pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
}