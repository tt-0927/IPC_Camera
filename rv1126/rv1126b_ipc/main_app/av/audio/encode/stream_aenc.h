/* 
 * @FilePath     : stream_aenc.h
 * @Author       : zhouzirui
 * @Date         : 2024-11-26 17:06:27
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2024-11-27 09:14:07
 * @Description  : 音频编码模块头文件
 */
#pragma once

#include "audio_define.h"
#include "stream_ai.h"

/*音频编码通道枚举*/
typedef enum
{
    AENC_MIC_CHN = 0, // 与AI mic连接进行音频编码通道
    AENC_MAX_CHN,
} AENC_CHN_E;

extern "C"
{
#include "rockit_aenc.h"
}

/**
 * @brief       : 音频编码初始化
 * @author      : zhouzirui
 * @param        {Stream_Audio_S} stAudioConfig 音频配置信息
 * @return       {*}
 */
RkAenc_S *streamAenc_init(Audio_NS::AudioConfig_S stAudioConfig);

/**
 * @brief       : 音频编码去初始化
 * @author      : zhouzirui
 * @param        {RkAenc_S} *pRkAenc 音频编码模块句柄
 * @return       {*}
 */
void streamAenc_uninit(RkAenc_S *pRkAenc);
