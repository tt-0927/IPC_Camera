/*** 
 * @FilePath     : stream_ai.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-21 10:31:58
 * @LastEditors  : cyc
 * @LastEditTime : 2025-04-17 15:51:03
 * @Description  : 
 */

#pragma once

#include "audio_define.h"
#include "data_length.h"
#include "share_data.h"
#include "dlog.h"
#include "IpcRet.h"

extern "C"
{
#include "rockit_bind.h"
#include "rockit_ai.h"
}

/*默认缓存帧数目*/
#define FRAME_NUM_DEFAULT   (4)

/* AAC-LD (Low Delay) 每帧采样点个数 适用于低延迟场景，帧长度较短 */
#define AACLD_SAMPLES_PER_FRAME         (512)   // AAC-LD: 512 samples/frame @48kHz = 10.67ms
/* AAC-LC (Low Complexity) 每帧采样点个数 标准AAC配置，平衡编码效率与复杂度 */
#define AACLC_SAMPLES_PER_FRAME         (1024)    // AAC-LC: 1024 samples/frame @48kHz = 21.33ms
/* AAC-HE (High Efficiency, aka AAC+) 每帧采样点个数 采用SBR技术，提高高频编码效率 */
#define AACPLUS_SAMPLES_PER_FRAME       (2048)    // AAC+: 2048 samples/frame @48kHz = 42.67ms
/* MP3 (MPEG-1 Layer III) 每帧采样点个数 兼容MP3标准帧结构 */
#define MP3_SAMPLES_PER_FRAME           (1152)    // MP3: 1152 samples/frame @44.1kHz = 26.12ms
/* 通用音频帧采样点基准值 常用于PCM处理或作为音频处理单元基准 */
#define AUDIO_SAMPLES_PER_FRAME         (480)     // 480 samples/frame @48kHz = 10ms
/* 语音编码常用帧长 16kHz*/
#define AMR_SAMPLES_PER_FRAME           (640)     // 640 samples @16kHz = 20ms
/* 语音编码常用帧长 16kHz*/
#define EVS_SAMPLES_PER_FRAME           (256)     // EVS: 256 samples @16kHz = 16ms

/*音频采集设备枚举*/
typedef enum
{
    AI_MIC_CHN = 0, //mic 麦克风
    AI_MAX_CHN,
} AI_CHN_E;

/**
 * @brief   : 音频输入采集初始化
 * @param    {int} nAiDevice 音频采集设备号
 * @param    {AudioConfig_S} stAudioConfig 音频配置信息
 * @return   {HiAi_S*} NULL：失败 非空：句柄
 */
RkAi_S *streamAi_init(int nAiDevice, Audio_NS::AudioConfig_S stAudioConfig);

/**
 * @brief   : 音频输入采集去初始化
 * @param    {RkAi_S} *pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
int streamAi_uninit(RkAi_S *pHandle);

/**
 * @brief   : 设置AI声道模式
 * @param    {RkAi_S} *pHandle 句柄
 * @param    {ot_audio_track_mode} enTrackMode 声道模式
 * @return   {int} 成功返回0,失败返回-1
 */
int streamAi_set_track_mode(RkAi_S *pHandle, AUDIO_TRACK_MODE_E enTrackMode);

/**
 * @brief   : 设置噪声消除使能
 * @param    {RkAi_S} *pHandle 句柄
 * @param    {int} nAiChn 音频采集通道号
 * @param    {bool} bDenoise 是否启用
 * @return   {int} 成功返回0,失败返回-1
 */
int streamAi_set_vqe_nr(RkAi_S *pHandle, int nAiChn, bool bDenoise);
