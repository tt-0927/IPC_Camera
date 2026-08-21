/**
 * @FilePath     : stream_audio.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-31 15:31:49
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 08:58:38
 * @Description  : 流媒体音频模块头文件
 */

#pragma once

#include <iostream>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <condition_variable>
#include <stdexcept>
#include "dlog.h"
#include "IpcRet.h"
#include "audio_define.h"
#include "stream_audio_sys.h"
#include "stream_ai.h"
#include "stream_ao.h"
#include "stream_aenc.h"
#include "stream_adec.h"
#include "stream_resample.h"
#include "push_stream.h"
#include "av_configure.h"
#include "algo_detect.h"

extern "C"
{
#include "codec_g711.h"
#include "mpp_sys.h"
#include "mpp_bind.h"
#include "audio_processing.h"
}

class CStreamAudio : public IAVAudioConfigApplier
{
private:
    CStreamAudio();
    static CStreamAudio* m_self;
    static std::mutex m_mutex;
public:
    static CStreamAudio* instance()
    {
        if (m_self == nullptr) // 第一层检查
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_self == nullptr) // 第二层检查
            {
                m_self = new CStreamAudio();
            }
        }
        return m_self;
    }

    ~CStreamAudio();

    /**
     * @brief       : 初始化音频流模块
     * @author      : zhouzirui
     * @return       {*} RETURN_ERROR：失败
     */
    IpcRet_E init();

    /**
     * @brief       : 反初始化音频流模块
     * @author      : zhouzirui
     * @return       {*} RETURN_ERROR：失败
     */
    IpcRet_E deinit();

    /**
     * @brief   : 重新启动音频流模块
     * @return   {IpcRet_E}0：成功，非0：失败
     */
    IpcRet_E reboot();

    /**
     * @brief       : 送音频数据至ADEC解码后送AO播放
     * @author      : zhouzirui
     * @param        {AoInfo_S} &stAoInfo ao音频信息
     * @return       {*}0：成功 -1：失败
     */
    int sendAudio_to_Adec(const Audio_NS::AoInfo_S &stAoInfo);

    /**
     * @brief   : 获取音频配置
     * @param    {Audio_NS::AudioConfig_S} &stAudioConfig：音频配置
     * @return   {int}0：成功，非0：失败
     */
    int getAudioConfig(Audio_NS::AudioConfig_S &stAudioConfig);

    /**
     * @brief   : 设置音频配置
     * @param    {Audio_NS::AudioConfig_S} &stAudioConfig：音频配置
     * @return   {int}0：成功，非0：失败
     */
    int setAudioConfig(const Audio_NS::AudioConfig_S &stAudioConfig);

    /**
     * @brief   : 应用音频配置
     * @param    {Audio_NS::AudioConfig_S} &stConfig：音频配置
     * @return   {int} 0：成功，非0：失败
     */
    int apply_audio_config(const Audio_NS::AudioConfig_S &stConfig) override;

    /**
     * @brief   : 设置音频输出采样率
     * @param    {Audio_NS::AudioSamprate_E} enSampRate：音频采样率
     * @return   {int} 0：成功，非0：失败
     * @note    : 和当前ao采样率不一致时，进行重启ao
     */
    int setAoSampleRate(const Audio_NS::AudioSamprate_E enSampRate);

    /**
     * @brief   : 等待 AO 通道硬件缓冲区完全排空
     * @param    {int} nChn：通道号
     * @param    {int} nTimeoutMs：最大等待时间（ms），-1 表示无限等待
     * @return   {int} 0：已完全排空，非0：超时或错误
     */
    int waitAoDrained(int nChn, int nTimeoutMs);

    /**
     * @brief   : 获取AI句柄
     * @param    {int} nChn AI通道号
     * @return   {HiAi_S *} 非空：成功 空：失败
     */
    HiAi_S *get_aiHandle(int nChn);

private:
    /**
     * @brief       : 绑定模块
     * @author      : zhouzirui
     * @return       {*}RETURN_ERROR：失败
     */
    int bindModule();

    /**
     * @brief       : 解绑模块
     * @author      : zhouzirui
     * @return       {*}RETURN_ERROR：失败
     */
    int unbindModule();

    /**
     * @brief       : 音频流左右声道音量设置
     * @author      : zhouzirui
     * @param        {int} nVolumeL 左声道音量
     * @param        {int} nVolumeR 右声道音量
     * @return       {*}0：成功 -1：失败
     */
    int audioMange_set_volume(int nVolumeL,  int nVolumeR);

    /**
     * @brief   : 初始化回调绑定
     */
    void initCallbackBinding();

    /**
     * @brief       : 创建音频帧包
     * @author      : zhouzirui
     * @param        {uint8_t} *pData：音频数据
     * @param        {int} nDataLen：音频数据长度
     * @return       {*}NULL：失败 非NULL：成功
     */
    Audio_NS::AudioFrame_S *createFrame(uint8_t *pData, int nDataLen);

    /**
     * @brief       : 销毁由 createFrame 创建的连续音频帧包
     * @author      : zhouzirui
     * @param        {AudioFrame_S} *pAudioFrame：待释放的音频帧数据指针
     * @return       {void}
     */
    void freeFrame(Audio_NS::AudioFrame_S *pAudioFrame);

    /**
     * @brief       : 音频流数据左右声道音量调整
     * @author      : zhouzirui
     * @param        {int8_t} *pData    pcm数据
     * @param        {int} nSize        音频长度
     * @return       {*}0：成功 -1：失败
     */
    int volume_adjust(int8_t *pData, int nSize);

    /**
     * @brief   : 编码g711格式
     * @param    {uint8_t} *pData：待编码 PCM 数据
     * @param    {int} nDataLen：数据长度
     * @return   {AudioFrame_S} 音频帧：成功 NULL：失败
     */
    Audio_NS::AudioFrame_S *encode_g711(uint8_t *pData, int nDataLen);

    //info /*----------------------- 私有线程函数 -----------------------*/
    /**
     * @brief       : 获取音频输入线程
     * @author      : zhouzirui
     * @param        {void} *param：通道号
     * @return       {*}
     */
    void deal_aiFrame_thr(int param);

    /**
     * @brief   : 获取音频输入原始帧线程
     * @param    {int} param 通道号
     */
    // void deal_aiRawFrame_thr(int param);

    /**
     * @brief       : 获取音频编码输出线程
     * @author      : zhouzirui
     * @param        {void} *param：通道号
     * @return       {*}
     */
    void deal_aencFrame_thr(int param);

    /**
     * @brief       : 获取音频解码输出线程
     * @author      : zhouzirui
     * @param        {int} param：通道号
     * @return       {*}
     */
    // void deal_adecFrame_thr(int param);

    // 异常处理
    void HandleThreadException(const std::string& thread_name) {
        std::lock_guard<std::mutex> lock(exception_mutex_);
        last_error_ = thread_name + " crashed";
        // RestartThread(thread_name); // 线程重启逻辑
    }
private:
    //info /*----------------------- 模块句柄 -----------------------*/
    /* ai句柄 */
    HiAi_S *m_pAiHandle[AI_MAX_CHN];
    /* ao句柄 */
    HiAo_S *m_pAoHandle[AO_MAX_CHN];
    /* 音频流输出管理类 */
    CStreamAo m_streamAO;
    /* aenc句柄 */
    HiAenc_S *m_pAencHandle[OT_AENC_MAX_CHN_NUM];
    /* adec句柄 */
    HiAdec_S *m_pAdecHandle[OT_ADEC_MAX_CHN_NUM];
    /* 音频重采样句柄 */
    HiResample_S *m_pResampleHandle;
    /*送编码数据互斥锁*/
    std::mutex m_mutexSendData;
    /*ai 线程句柄*/
    std::thread aiThread[AI_MAX_CHN];
    /*aiRaw 线程句柄*/
    // std::thread m_aiRawThread;
    /*aenc 线程句柄*/
    std::thread aencThread[AENC_MAX_CHN];
    /*adec 线程句柄*/
    // std::thread adecThread[ADEC_MAX_CHN];
    //info /*----------------------- 参数变量 -----------------------*/
    /*音频配置文件路径*/
	std::string m_strConfigPath;
    /* 是否初始化 */
    bool m_bInitFlag = false;
    /*获取 ai数据-标志*/
    std::atomic_bool m_bGetAiFlag[AI_MAX_CHN];
    /*获取 aenc数据-标志*/
    std::atomic_bool m_bGetAencFlag[AENC_MAX_CHN];
    /*获取 adec数据-标志*/
    // std::atomic_bool m_bGetAdecFlag[ADEC_MAX_CHN];
    /*音频配置*/
    Audio_NS::AudioConfig_S m_stAudioConfig;
    /*左通道音量*/
    std::atomic<int> m_atVolumL;
    /*右通道音量*/
    std::atomic<int> m_atVolumR;
    /*线程相关*/
    std::atomic_bool m_bAiFlag;
    /*线程异常处理互斥锁*/
    std::mutex exception_mutex_;
    /*线程异常处理的线程名称*/
    std::string last_error_;
    /*对讲解码数据*/
    std::vector<uint8_t> m_bytesTalkbackData;
    /*控制操作互斥锁*/
    std::mutex m_mutexCtrl;
    /*音频编码是否为 AAC*/
    bool m_bIsAac{false};
};
