/**
 * @FilePath     : stream_ao.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-31 20:08:23
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-27 10:20:24
 * @Description  : AO 音频流输出
 */

#pragma once

#include <atomic>
#include <mutex>

#include "share_data.h"
#include "audio_define.h"
#include "stream_ai.h"

/*输出声卡*/
#define SOUND_CARD_SPEAK    "hw:0,0"

/*音频输出通道枚举*/
typedef enum
{
    AO_SPEAKER_CHN = 0, // 喇叭功放音频输出通道
    AO_MAX_CHN,
} AO_CHN_E;

extern "C"
{
#include "rockit_ao.h"
#include "gpio_utils.h"
}

/**
 * @brief   : 音频输出初始化
 * @param    {int} enAoDevice 音频输出通道号
 * @param    {AudioConfig_S} stAudioConfig 音频配置信息
 * @return   {int} 0：成功 非零：失败
 */
RkAo_S *streamAo_init(int enAoDevice, Audio_NS::AudioConfig_S stAudioConfig);

/**
 * @brief   : 音频输出去初始化
 * @param    {RkAo_S} *pHandle 句柄
 * @return   {int} 0：成功 非零：失败
 */
int streamAo_uninit(RkAo_S *pHandle);

/**
 * @brief   : 音频输出重启
 * @param    {RkAo_S} *pHandle：句柄
 * @param    {int} nAoDevice：音频采集设备号
 * @param    {AudioConfig_S} &stAudioConfig：音频配置信息
 * @return   {int} 0：成功 非零：失败
 */
int streamAo_reboot(RkAo_S *pHandle, int nAoDevice, const Audio_NS::AudioConfig_S &stAudioConfig);

/**
 * @brief   : 设置音频输出音量
 * @param    {RkAo_S} *pHandle 句柄
 * @param    {int} nVolume：输出音量
 * @return   {int} 0：成功 非零：失败
 */
int streamAo_setVolume(RkAo_S *pHandle, int nVolume);

/* 音频流输出管理类 */
class CStreamAo
{
private:
    CStreamAo();
    static CStreamAo *m_self;
    static std::mutex m_mutex;

public:
    static CStreamAo *instance()
    {

         if (m_self == nullptr) // 第一层检查
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_self == nullptr) // 第二层检查
            {
                m_self = new CStreamAo();
            }
        }
        return m_self;
    }

    ~CStreamAo();

    /**
     * @brief   : 初始化
     * @param    {AudioConfig_S} &stConfig
     * @return   {int} 0：成功 非零：失败
     */
    int init(Audio_NS::AudioConfig_S &stConfig);

    /**
     * @brief   : 去初始化
     * @return   {int} 0：成功 非零：失败
     */
    int uninit();

    /**
     * @brief   : 更新音频输出类型
     * @param    {AudioOutputType_E} &enType
     * @return   {int} 0：成功 非零：失败
     */
    int update_audioOutputType(const Audio_NS::AudioOutputType_E &enType);

private:
    /* 扬声器用户空间GPIO ID */
    const int SPEAKER_GPIO = 4;
    /* 线路输出用户空间GPIO ID */
    const int LINEOUT_GPIO = 199;

    /* 初始化标志 */
    std::atomic<bool> m_bInit;
    /* 扬声器GPIO句柄 */
    GpioHandle_S* m_pSpeaker = nullptr;
    /* 线路输出GPIO句柄 */
    GpioHandle_S* m_pLineOut = nullptr;
};
