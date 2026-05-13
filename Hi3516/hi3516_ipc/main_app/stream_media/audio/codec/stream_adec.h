/**
 * @FilePath     : stream_adec.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-31 20:08:36
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-05-13 15:53:30
 * @Description  : AENC 音频解码
 */

#pragma once

#include "audio_define.h"
#include "IpcRet.h"
#include "stream_ai.h"

/*音频解码通道枚举*/
typedef enum
{
    SPEAK_CHN = 0, /* 对讲对接进行音频解码通道 */
    ADEC_MAX_CHN,
} ADEC_CHN_E;

extern "C"
{
#include "mpp_adec.h"
}

/**
 * @brief       : 音频流解码初始化
 * @author      : zhouzirui
 * @param        {int} nAdecChn：解码通道号
 * @param        {AudioConfig_S} stAudioConfig：音频配置信息
 * @return       {HiAdec_S*}NULL：失败 非空：句柄
 */
HiAdec_S *streamAdec_init(int nAdecChn, Audio_NS::AudioConfig_S stAudioConfig);

/**
 * @brief       : 音频流解码去初始化
 * @author      : zhouzirui
 * @param        {HiAdec_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
int streamAdec_uninit(HiAdec_S *pHandle);
