/**
 * @FilePath     : stream_aenc.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-31 20:08:36
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-08 15:49:48
 * @Description  : AENC 音频编码
 */

#pragma once

#include "audio_define.h"
#include "IpcRet.h"
#include "stream_ai.h"

/*音频编码通道枚举*/
typedef enum
{
    AENC_AAC_CHN = 0, // 固定编码AAC送录制进程
    AENC_MIC_CHN,     // 与AI mic连接进行音频编码通道
    AENC_MAX_CHN,
} AENC_CHN_E;

extern "C"
{
#include "mpp_aenc.h"
}

/**
 * @brief       : 音频流编码初始化
 * @author      : zhouzirui
 * @param        {int} nAencChn：编码通道号
 * @param        {AudioConfig_S} stAudioConfig：音频配置信息
 * @return       {HiAenc_S*}NULL：失败 非空：句柄
 */
HiAenc_S *streamAenc_init(int nAencChn, Audio_NS::AudioConfig_S stAudioConfig);

/**
 * @brief       : 音频流编码去初始化
 * @author      : zhouzirui
 * @param        {HiAenc_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
int streamAenc_uninit(HiAenc_S *pHandle);
