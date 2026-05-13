/*** 
 * @FilePath     : hide_detect.hpp
 * @Author       : cyc
 * @Date         : 2025-07-21 15:47:09
 * @LastEditors  : cyc
 * @LastEditTime : 2025-09-02 15:39:38
 * @Description  : 遮挡检测
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

extern "C"
{
     #include "mpp_vgs.h"
     #include "svp_od.h"
}

enum class EOcclusionSens : int
{
    OFF = 0,
    LOW = 1,
    MID = 2,
    HIGH = 3,
    COUNT = 4 /* 边界检查用 */
};

/* 遮挡报警阈值 */
static const int nOcclusionThreshold[static_cast<int>(EOcclusionSens::COUNT)] = {
64,  /* OFF */ 
20,  /* LOW*/ 
17,  /* MID */ 
15   /* HIGH */ 
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
     * @param    {int} nHideResult 遮挡检测结果值
     * @param    {SEventProcessContext} &stCtx：事件处理上下文
     * @return   {bool} 是否有报警
     */
    bool processHideDetect(int nHideResult, const SEventProcessContext &stCtx);

     /**
     * @brief   : 转换区域坐标并判断是否使能算法
     */
    template<typename T>
    void convertResolutionAndEnable(T &stConfig);
 
private:

     /* 遮挡侦测句柄 */
     HiOd_S *m_pHideDetHandle = nullptr;
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
     EventManager m_RecvManager{500};
     /* 遮挡侦测报警状态机,判断是否进行报警 */
     CAlarmStateMachine m_hideAlarmStateMachine;
     /* 配置参数 */
     /* 遮挡侦测 */
     Alarm::HideAlarm_S m_stAlgoHideDetCfg;
     /* 侦测区域 */
     Common::Rect_S m_stRect;
     /* 算法默认分辨率 */
     int m_nWidth = PIXEL_WIDTH_1024;
     int m_nHeight = PIXEL_HEIGHT_576;
     /* 是否需要重新初始化算法（宽高变化） */
     std::atomic<bool> m_bNeedReInit{false};
     /* 是否需要裁剪源视频 */
     bool m_bIsCrop = false;
     /* 目标视频帧 */
     ot_video_frame_info m_stDstFrameInfo;
};