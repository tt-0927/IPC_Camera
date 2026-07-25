/**
 * @FilePath     : stream_audio.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-12-02 09:50:53
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-15 15:26:36
 * @Description  : 流媒体音频模块头文件
 */

#pragma once

#include <iostream>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include <stdint.h>
#include <string.h>

#include "dlog.h"
#include "IpcRet.h"

#include "stream_ai.h"
#include "stream_aenc.h"
#include "stream_ao.h"
#include "stream_adec.h"
#include "audio_enc.h"
#include "audio_define.h"
#include "push_stream.h"
#include "av_configure.h"

extern "C"
{
#include "codec_g711.h"
}

class CStreamAudio : public CSingleton<CStreamAudio>
{
private:
    CStreamAudio();

public:
    virtual ~CStreamAudio();
    /* 允许 Singleton 访问私有构造函数 */
    friend class CSingleton<CStreamAudio>;

    /**
     * @brief   : 初始化音频流模块
     * @return   {int} 0：成功，非0：失败
     */
    int init();

    /**
     * @brief   : 去初始化音频流模块
     * @return   {int} 0：成功，非0：失败
     */
    int deinit();

    /**
     * @brief   : 重新启动音频流模块
     * @return   {int} 0：成功，非0：失败
     */
    int reboot();

    /**
     * @brief   : 初始化音频流采集处理模块
     * @author  : zhouzirui
     * @param    {Stream_Audio_S} stAudio 音频配置信息
     * @return   {int} 0：成功，非0：失败
     */
    int initStreamAudio(Audio_NS::AudioConfig_S stAudioConfig);

    /**
     * @brief   : 去初始化音频流采集处理模块
     * @author  : zhouzirui
     * @return   {int} 0：成功，非0：失败
     */
    int deinitStreamAudio();

    /**
     * @brief   : 初始化ffmpeg音频编码
     * @author  : zhouzirui
     * @param    {Audio_NS::AudioConfig_S} stuAudionNeedAttr
     * @return   {int} 0：成功，非0：失败
     */
    int init_ff_encode(const Audio_NS::AudioConfig_S stAudioConfig);

    /**
     * @brief   : 去初始化ffmpeg音频编码
     * @return   {int} 0：成功，非0：失败
     */
    int deinit_ff_encode();

    /**
     * @brief   : 送音频数据至ADEC解码后送AO播放
     * @param    {AoInfo_S} stAoInfo ao音频信息
     * @return   {int} 0：成功，非0：失败
     */
    int sendAudio_to_Adec(Audio_NS::AoInfo_S stAoInfo);

    /**
     * @brief   : 获取音频配置
     * @param    {Audio_NS::AudioConfig_S} &stAudioConfig：音频配置
     * @return   {int} 0：成功，非0：失败
     */
    int getAudioConfig(Audio_NS::AudioConfig_S &stAudioConfig);

    /**
     * @brief   : 设置音频配置
     * @param    {Audio_NS::AudioConfig_S} &stAudioConfig：视频配置
     * @return   {int} 0：成功，非0：失败
     */
    int setAudioConfig(const Audio_NS::AudioConfig_S &stAudioConfig);

    /**
     * @brief   : 设置音频输出采样率
     * @note    : 和当前ao采样率不一致时，进行重启ao
     * @param    {Audio_NS::AudioSamprate_E} enSampRate：音频采样率
     * @return   {int} 0：成功，非0：失败
     */
    int setAoSampleRate(const Audio_NS::AudioSamprate_E enSampRate);

    /**
     * @brief   : 立即静音功放（关闭 GPIO），用于播放结束时提前关闭功放消除结尾噗声
     */
    void mutePa();

private:
    /**
     * @brief   : 获取当前单调时间戳（毫秒）
     * @return   {int} 0：成功，非0：失败
     */
    uint64_t getSteadyTimeMS();
    /**
     * @brief   : 绑定模块信息
     * @return   {int} 0：成功，非0：失败
     */
    int bindModule();

    /**
     * @brief   : 解绑定模块信息
     * @return   {int} 0：成功，非0：失败
     */
    int unbindModule();

    /**
     * @brief   : 初始化回调绑定
     */
    void initCallbackBinding();

    /**
     * @brief   : 创建音频帧包
     * @param    {uint8_t} *pData 音频数据
     * @param    {int} nDataLen 音频数据长度
     * @return   {Audio_NS::AudioFrame_S *} NULL：失败 非NULL：成功
     */
    Audio_NS::AudioFrame_S *createFrame(uint8_t *pData, int nDataLen);

    /**
     * @brief   : 销毁音频帧包（不释放内部 pData）
     * @param    {AudioFrame_S} *pAudioFrame 待释放的音频帧数据指针
     */
    void freeFrame(Audio_NS::AudioFrame_S *pAudioFrame);

    /**
     * @brief   : 音频流数据左右声道音量调整
     * @param    {int8_t} *pData pcm数据
     * @param    {int} nSize 音频长度
     * @return   {int} 0：成功，非0：失败
     */
    int volume_adjust(int8_t *pData, int nSize);

    /**
     * @brief   : 编码g711格式
     * @param    {uint8_t} *pData：待编码 PCM 数据
     * @param    {int} nDataLen：数据长度
     * @return   {AudioFrame_S} 音频帧：成功 NULL：失败
     */
    Audio_NS::AudioFrame_S *encode_g711(uint8_t *pData, int nDataLen);

    // info /*----------------------- 私有线程函数 -----------------------*/

    /**
     * @brief   : 音频流采集处理线程
     * @param    {int} param
     */
    void deal_aiFrame_thr(int param);

    /**
     * @brief   : 获取音频编码输出线程
     * @param    {int} param
     */
    // void deal_aencFrame_thr(int param);

    /**
     * @brief   : 获取ffmpeg音频编码数据线程
     */
    void deal_ffAencFrame_thr();

    /**
     * @brief   : 监测AO对应输出模块GPIO状态线程
     */
    void paMonitorTimer_thr();

private:
    // info /*----------------------- 模块句柄 -----------------------*/
    /* ai句柄 */
    RkAi_S *m_pAiHandle[AI_MAX_CHN];
    /* ao句柄 */
    RkAo_S *m_pAoHandle[AO_MAX_CHN];
    /* 音频流输出管理类 */
    CStreamAo* m_streamAO;
    /* aenc句柄 */
    // RkAenc_S *m_pAencHandle[AENC_MAX_CHN];
    /* adec句柄 */
    // RkAdec_S *m_pAdecHandle[ADEC_MAX_CHN];
    /* ffmpeg 音频编码句柄*/
    ff_AudioEnc_S *m_pFfAencHandle = NULL;

    /* ai 线程句柄 */
    std::thread aiThread[AI_MAX_CHN];
    /* aenc 线程句柄 */
    std::thread aencThread[AENC_MAX_CHN];
    /* ffmpeg aenc 线程句柄 */
    std::thread ffAencThread;
    /*监测AO对应输出模块GPIO状态线程对象*/
    std::thread           m_monitorThread; 

    // info /*----------------------- 参数变量 -----------------------*/
    /*音频配置文件路径*/
    std::string m_strConfigPath;
    /* 是否初始化 */
    bool m_bInitFlag = false;
    /*获取 ai数据-标志*/
    std::atomic_bool m_bGetAiFlag[AI_MAX_CHN];
    /*获取 aenc数据-标志*/
    std::atomic_bool m_bGetAencFlag[AENC_MAX_CHN];
    /*获取 ffmpeg aenc数据-标志*/
    std::atomic_bool m_bGetFfAencFlag;
    /*监测AO对应输出模块GPIO状态线程停止标志*/
    std::atomic<bool>     m_bStopThread{false}; 
    /*音频配置*/
    Audio_NS::AudioConfig_S m_stAudioConfig;
    /*左通道音量*/
    std::atomic<int> m_atVolumL;
    /*右通道音量*/
    std::atomic<int> m_atVolumR;
    /*对讲解码数据*/
    std::vector<uint8_t> m_bytesTalkbackData;
    /*控制操作互斥锁*/
    std::mutex m_mutexCtrl;
    /*送编码数据互斥锁*/
    std::mutex m_mutexSendData;
    /*音频编码是否为 AAC*/
    bool m_bIsAac{false};

    /*最后音频数据送AO的时间*/
    uint64_t m_lastSendTime = 0;
    /*AO对应输出模块GPIO是否使能*/
    bool m_isPaEnabled = false;
    /*最后音频数据送AO的时间*/
    std::atomic<uint64_t> m_lastAudioTime{0};
    /*AO对应输出模块GPIO是否使能*/
    std::atomic<bool>     m_paEnable{false};

    // info /*----------------------- 功放延时开启 -----------------------*/
    /*功放启动延时常量（毫秒）：等待 Codec 偏置电压稳定后再开启功放，消除开头噗声*/
    static constexpr uint64_t PA_DELAY_ALARM_MS    = 300;  /* 声音联动报警音延时 */
    static constexpr uint64_t PA_DELAY_REALTIME_MS = 500;  /* 对讲/广播实时流延时 */
    /*当前使用的延时值（根据音频来源动态切换）*/
    uint64_t              m_paDelayMs{PA_DELAY_ALARM_MS};
    /*是否处于功放启动延时期间*/
    std::atomic<bool>     m_bPaDelayActive{false};
    /*功放延时开始时间戳*/
    uint64_t              m_paDelayStartTime{0};

};
