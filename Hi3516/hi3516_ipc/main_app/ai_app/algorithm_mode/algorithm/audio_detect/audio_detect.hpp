/**
 * @FilePath     : audio_detect.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-29 20:10:37
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-12 09:52:47
 * @Description  : 音频异常侦测
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <algorithm>
#include <bitset>
#include <sys/time.h>
#include <unordered_set>

#include "blocking_queue.hpp"
#include "common_process.h"
#include "algorithm.hpp"
#include "share_data.h"

extern "C"
{
}

class CAudioDetect : public CAlgorithm
{
public:

    CAudioDetect();
    ~CAudioDetect();

    /**
     * @brief   : 接收媒体数据
     * @param    {MediaData_S} stMediaData：媒体数据
     */
    void recvMediaData(MediaData_S stMediaData) override;

    /**
     * @brief   : 更新算法配置参数
     * @param    {AlgorithmConfig} &stAlgoConfig：算法配置
     */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;

    /**
     * @brief   : 更新音频异常侦测参数
     * @param    {AudioAnomaly_S} &stAlgoCfg：音频异常侦测配置
     */
    void setAlgoParamCfg(const Alarm::AudioAnomaly_S &stAlgoCfg);

    /**
     * @brief   : 获取当前实时音量
     * @return   {float} 当前音量(dB)
     */
    float getCurrentDb() const override;

private:
    /**
     * @brief   : 初始化
     * @return   {bool} true：成功 false：失败
     */
    bool init();

    /**
     * @brief   : 反初始化
     * @return   {bool} true：成功 false：失败
     */
    bool unInit();

    /**
     * @brief   : 重新启动
     * @return   {bool} true：成功 false：失败
     */
    bool reboot();

    /**
     * @brief   : 线程函数
     */
    void run();

    // info /*----------------------- 算法后处理 -----------------------*/

    /**
     * @brief   : 音频异常侦测处理函数
     * @param    {char} *pData 音频帧数据指针
     * @param    {int} nLength 字节数
     * @param    {SEventProcessContext} &stCtx 事件处理上下文
     */
    void processAudioAnomaly(char *pData, int nLength, const SEventProcessContext &stCtx);

    /**
     * @brief   : 计算历史平均音量
     * @return   {float} 平均音量(dB)
     */
    float calculateAverageDB();

    /**
     * @brief   : 检测声强陡升
     * @param    {float} fCurrentDB：当前音量
     * @param    {float} fAvgDB：平均音量
     * @return   {bool} true：检测到陡升 false：未检测到
     */
    bool detectSuddenRise(float fCurrentDB, float fAvgDB);

    /**
     * @brief   : 检测声强陡降
     * @param    {float} fCurrentDB：当前音量
     * @param    {float} fAvgDB：平均音量
     * @return   {bool} true：检测到陡降 false：未检测到
     */
    bool detectSuddenDrop(float fCurrentDB, float fAvgDB);

private:

    /* 队列 */
    BQ_NS::CBlockingQueue<MediaData_S> m_dateQueue;
    /* 用于控制线程的运行 */
    std::atomic<bool>       m_bRunning;
    /* 互斥锁保护指针访问 */
    std::mutex              m_mutex;
    /* 条件变量 */
    // std::condition_variable m_condition;
    /* 数据获取线程 */
    std::thread             m_thread;
    /* 检测频率控制 */
    EventManager m_RecvManager{20};   /* 20ms处理一次 */
    /* 音频输入异常报警状态机，判断是否进行报警 */
    CAlarmStateMachine m_inputAlarmStateMachine;
    /* 音频声强陡升报警状态机，判断是否进行报警 */
    CAlarmStateMachine m_riseAlarmStateMachine;
    /* 音频声强陡降报警状态机，判断是否进行报警 */
    CAlarmStateMachine m_dropAlarmStateMachine;
    /* 是否初始化 */
    bool m_bInit = false;

    /* 配置参数 */
    /* 音频异常侦测 */
    Alarm::AudioAnomaly_S m_stAudioAnomalyCfg;

    /* 音频参数 */
    int m_nSampleRate = 16000; /* 采样率 */
    int m_nBitsPerSample = 16; /* 采样位数 */

    /* 音量历史缓冲区（滑动窗口） */
    std::deque<float> m_queueDBHistory;
    // note 对于16KHz 1ch 16Bit 音频，一帧为32ms，那么一秒钟大于31帧
    /* 滑动窗口大小(帧数) */
    int m_nWindowSize = 100;

    /* 当前音量 */
    std::atomic<float> m_fCurrentDB{ 0.0f };

    /* 音频输入异常检测 */
    int m_nSilenceFrameCount = 0;                        /* 静音帧计数 */
    static constexpr int SILENCE_THRESHOLD_FRAMES = 150; /* 静音帧阈值 */
};
