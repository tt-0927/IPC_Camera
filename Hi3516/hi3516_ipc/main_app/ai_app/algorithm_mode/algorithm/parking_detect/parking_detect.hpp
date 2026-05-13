/**
 * @FilePath     : parking_detect.hpp
 * @Author       : zhouzirui
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-29 17:02:54
 * @Description  : 停车侦测
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <algorithm>
#include <sys/time.h>

#include "blocking_queue.hpp"
#include "common_process.h"
#include "algorithm.hpp"
#include "algo_control_deal.h"
#include "algo_stream_deal.h"

class CParkingDetect : public CAlgorithm
{
public:

    CParkingDetect();
    ~CParkingDetect();

    /**
     * @brief   : 接受媒体数据
     * @param    {MediaData_S} stMediaData：媒体数据
     */
    void recvMediaData(MediaData_S stMediaData) override;

    /**
     * @brief   : 更新算法配置参数
     * @param    {AlgorithmConfig} &stAlgoConfig：算法配置
     */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;

    /**
     * @brief   : 更新停车侦测参数 
     * @param    {ParkingDetection_S} &stAlgoCfg：停车侦测
     */
    void setAlgoParamCfg(const Alarm::ParkingDetection_S &stAlgoCfg);

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
     * @brief   : 线程函数
     */
    void run();

private:

    /* 停车侦测句柄 */
    Inference_NS::CYoloUltralytics *m_pParkDetHandle = nullptr;
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
    /* 事件管理器, 过滤重复事件 */
    EventManager m_EventManager{5 * 1000};
    /* 检测频率控制 */
    EventManager m_RecvManager{500};
    /* 目标上报控制 */
    EventManager m_SendManager{500};
    /* 配置参数 */
    /* 停车侦测 */
    Alarm::ParkingDetection_S m_stAlgoParkDetCfg;

    /* 算法默认分辨率 */
    int m_nWidth = PIXEL_WIDTH_640;
    int m_nHeight = PIXEL_HEIGHT_384;
};
