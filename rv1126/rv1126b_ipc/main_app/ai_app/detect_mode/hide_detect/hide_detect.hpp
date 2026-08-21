/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-05 10:38:00
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2026-04-30 11:28:50
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/algorithm_mode/algorithm/motion_detect/hide_detect.hpp
 * @Description: 遮挡侦测
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
#include "CameraObstructionV1_0.hpp"

enum class EOcclusionSens : int
{
    OFF = 0,
    LOW = 1,
    MID = 2,
    HIGH = 3,
    COUNT = 4 /* 边界检查用 */
};

/* 遮挡报警阈值 */
static const float nOcclusionThreshold[static_cast<int>(EOcclusionSens::COUNT)] = {
1.0,  /* OFF */ 
0.75,  /* LOW*/ 
0.5,  /* MID */ 
0.25   /* HIGH */ 
};

class CHideDetect : public CAlgorithm
{
public:
    CHideDetect();
    ~CHideDetect();

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
     * @brief   : 更新遮挡侦测参数 
     * @param    {HideAlarm_S} &stAlgoCfg：遮挡侦测
     */
     void setAlgoParamCfg(const Alarm::HideAlarm_S &stAlgoCfg);

private:

     /**
      * @brief 初始化
      * @return [*]
      * @note
      */
     bool init();
 
     /**
      * @brief 反初始化
      * @return [*]
      * @note
      */
     bool unInit();
     /**
      * @brief 线程函数
      * @return [*]
      * @note
      */
     void run();

    /**
     * @brief   : 遮挡侦测后处理函数
     * @param    {bool} nHideResult 遮挡检测结果
     * @return   {bool} 是否有报警
     */
    bool processHideDetect(bool bIsAlarm, const MediaData_S &stMediaData = MediaData_S());

     /**
     * @brief   : 转换区域坐标并判断是否使能算法
     */
    template<typename T>
    void convertResolutionAndEnable(T &stConfig);
 
private:

    /* 遮挡侦测句柄 */
    CameraObstruction_NS::CCameraObstructionV1_0 *m_pHideDetHandle = nullptr;
    /* 队列 */
    BQ_NS::CBlockingQueue<MediaData_S> m_dateQueue;
    /* 用于控制线程的运行 */
    std::atomic<bool>       m_bRunning;
    /* 互斥锁保护指针访问 */
    std::mutex              m_mutex;
    /* 条件变量 */
    std::condition_variable m_condition;
    /* 数据获取线程 */
    std::thread             m_thread;
    /* 检测频率控制 */
    EventManager m_RecvManager{500};
    /* 遮挡侦测报警状态机,判断是否进行报警 */
    CAlarmStateMachine m_hideAlarmStateMachine;
    /* 配置参数 */
    /* 遮挡侦测 */
    Alarm::HideAlarm_S m_stAlgoHideDetCfg;
    /* 侦测区域 */
    Common::Rect_S m_stRect;
    /*RGA复用内存*/
    cv::Mat m_fullRgbMat;
    /* 算法默认分辨率 */
    int m_nWidth = PIXEL_WIDTH_1280;
    int m_nHeight = PIXEL_HEIGHT_720;
    /* 是否需要重新初始化算法（宽高变化） */
    std::atomic<bool> m_bNeedReInit{false};
    /* 是否需要裁剪源视频 */
    bool m_bIsCrop = false;
};