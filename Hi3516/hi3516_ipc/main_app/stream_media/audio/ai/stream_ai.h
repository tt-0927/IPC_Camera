/**
 * @FilePath     : stream_ai.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-31 20:08:09
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-16 13:56:26
 * @Description  : AI 音频流采集输入
 */

#pragma once

#include "audio_define.h"
#include "IpcRet.h"

/*默认缓存帧数目*/
#define FRAME_NUM_DEFAULT   (5)


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
    AI_MIC_CHN = 0, //mic
    AI_LINEIN_CHN,  //linein
    AI_MAX_CHN,
} AI_CHN_E;

extern "C"
{
#include "mpp_ai.h"
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <signal.h>
// #include <fcntl.h>
}

/**
 * @brief       : 音频输入采集初始化
 * @author      : zhouzirui
 * @param        {int} nAiDevice：音频采集设备号
 * @param        {AudioConfig_S} stAudioConfig：音频配置信息
 * @return       {HiAi_S*}NULL：失败 非空：句柄
 */
HiAi_S *streamAi_init(int nAiDevice, Audio_NS::AudioConfig_S stAudioConfig);

/**
 * @brief       : 音频输入采集去初始化
 * @author      : zhouzirui
 * @param        {HiAi_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
int streamAi_uninit(HiAi_S *pHandle);

/**
 * @brief   : 设置录音噪声消除
 * @param    {HiAi_S} *pHandle 句柄
 * @param    {int} nAiDevice 音频采集设备号
 * @param    {bool} bDenoise 是否启用
 * @return   {int} 成功返回0,失败返回-1
 */
int streamAi_set_vqe_rnr(HiAi_S *pHandle, int nAiDevice, bool bDenoise);

/**
 * @brief   : 设置AI声道模式
 * @param    {HiAi_S} *pHandle 句柄
 * @param    {ot_audio_track_mode} enTrackMode 声道模式
 * @return   {int} 成功返回0,失败返回-1
 */
int streamAi_set_track_mode(HiAi_S *pHandle, ot_audio_track_mode enTrackMode);
