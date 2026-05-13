/**
 * @FilePath     : stream_resample.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-08-29 10:56:28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-29 11:14:13
 * @Description  : stream 音频重采样
 */

#pragma once

#include "audio_define.h"
#include "IpcRet.h"

extern "C"
{
#include "mpp_resample.h"
}

/**
 * @brief       : 音频重采样初始化
 * @param        {AudioConfig_S} stAudioConfig：音频配置信息
 * @return       {HiResample_S*} NULL：失败 非空：句柄
 */
HiResample_S *streamResample_init(Audio_NS::AudioConfig_S stAudioConfig);

/**
 * @brief       : 音频重采样去初始化
 * @param        {HiResample_S} *pHandle：句柄
 * @return       {int} 成功返回0,失败返回-1
 */
int streamResample_uninit(HiResample_S *pHandle);
