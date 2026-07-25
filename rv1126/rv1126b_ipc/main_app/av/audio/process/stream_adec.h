/*** 
 * @FilePath     : stream_adec.h
 * @Author       : cyc
 * @Date         : 2025-04-03 14:34:41
 * @LastEditors  : cyc
 * @LastEditTime : 2025-04-17 15:31:00
 * @Description  : 音频解码模块
 */

#pragma once

#include "audio_define.h"
#include "rockit_adec.h"
#include "share_data.h"

/*音频解码通道枚举*/
typedef enum
{
    ADEC_SPEAK_CHN = 0, /* 对讲对接进行音频解码通道 */
    ADEC_MAX_CHN,
} ADEC_CHN_E;

/**
 * @brief       : 音频流解码初始化
 * @param        {int} nAdecChn：解码通道号
 * @param        {AudioConfig_S} stAudioConfig：音频配置信息
 * @return       {pRkAenc*} NULL：失败 非空：句柄
 */
RkAdec_S *streamAdec_init(int nAdecChn, Audio_NS::AudioConfig_S stAudioConfig);

/**
 * @brief       : 音频解码去初始化
 * @author      : cyc
 * @param        {RkAdec_S} *pHandle 音频解码模块句柄
 * @return       {*}
 */
void streamAdec_uninit(RkAdec_S *pHandle);
