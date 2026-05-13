/**
 * @FilePath     : stream_ao.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-31 20:08:23
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-04 14:44:42
 * @Description  : AO 音频流输出
 */

#pragma once

#include <atomic>

#include "audio_define.h"
#include "IpcRet.h"
#include "stream_ai.h"

/*音频输出通道枚举*/
typedef enum
{
    AO_SPEAKER_CHN = 0, //喇叭功放音频输出通道
    AO_UAC_CHN,         //UAC音频输出通道
    AO_MAX_CHN,
} AO_CHN_E;

extern "C"
{
#include "mpp_ao.h"
#include "gpio_utils.h"
}

/**
 * @brief       : 音频输出初始化
 * @author      : zhouzirui
 * @param        {int} nAoDevice：音频采集设备号
 * @param        {AudioConfig_S} stAudioConfig：音频配置信息
 * @return       {HiAo_S*}NULL：失败 非空：句柄
 */
HiAo_S *streamAo_init(int nAoDevice, const Audio_NS::AudioConfig_S &stAudioConfig);

/**
 * @brief       : 音频输出去初始化
 * @author      : zhouzirui
 * @param        {HiAo_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
int streamAo_uninit(HiAo_S *pHandle);

/**
 * @brief   : 音频输出重启
 * @param    {HiAo_S} *pHandle：句柄
 * @param    {int} nAoDevice：音频采集设备号
 * @param    {AudioConfig_S} &stAudioConfig：音频配置信息
 * @return   {int} 0：成功 非零：失败
 */
int streamAo_reboot(HiAo_S *pHandle, int nAoDevice, const Audio_NS::AudioConfig_S &stAudioConfig);


/* 音频流输出管理类 */
class CStreamAo
{
public:
    CStreamAo();
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
     * @brief   : 更新音频配置
     * @param    {AudioConfig_S} &stConfig
     * @return   {int} 0：成功 非零：失败
     */
    int update_audioConfig(const Audio_NS::AudioConfig_S &stConfig);

private:
    /* 扬声器用户空间GPIO ID */
    const int SPEAKER_GPIO = 60;
    /* 线路输出用户空间GPIO ID */
    const int LINEOUT_GPIO = 61;

    /* 初始化标志 */
    std::atomic<bool> m_bInit;
    /* 扬声器GPIO句柄 */
    GpioHandle_S* m_pSpeaker = nullptr;
    /* 线路输出GPIO句柄 */
    GpioHandle_S* m_pLineOut = nullptr;
};
