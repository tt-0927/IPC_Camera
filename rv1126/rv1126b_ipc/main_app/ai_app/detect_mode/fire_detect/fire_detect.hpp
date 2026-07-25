/**
 * @file fire_detect.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-13
 * 
 * @brief 火焰检测相关
 */

 #pragma once

 #include <atomic>
 #include <chrono>
 #include <condition_variable>
 #include <mutex>
 #include <thread>
 #include <algorithm>
 #include <sys/time.h>
 #include "common_process.h"
 #include "blocking_queue.hpp"
 #include "stream_video.h"
 #include "stream_vpss.h"
 #include <opencv2/opencv.hpp>
 #include "event_manager.hpp"
 #include "stream_process_ext.hpp"
#include "algorithm.hpp"
#include "FireDetectV1_0.hpp"

class CFireDetect : public CAlgorithm
{
public:

    CFireDetect();
    ~CFireDetect();

    /**
     * @brief 接受媒体数据
     * @param [MediaData_S] stMediaData:
     * @return [*]
     * @note
     */
    void recvMediaData(MediaData_S stMediaData) override;
    
    /**
     * @brief 更新算法配置参数
     * @param stAlgoConfig 
     */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig);
    void setAlgoParamCfg(const Alarm::SmokeFireDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::SMOKE_FIRE);
    void setAlgoParamCfg(const Alarm::OpenFlameDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::OPEN_FLAME);

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
     * @brief 将 1-100 的灵敏度转换为触发报警所需的连续帧数
     * @param sensitivity 灵敏度 (1-100)
     * @param minFrames 最高灵敏度 (100) 对应的帧数
     * @param maxFrames 最低灵敏度 (1) 对应的帧数
     * @return int 触发报警所需的连续帧数
     */
    int sensitivityToFrames(int sensitivity, int minFrames = 1, int maxFrames = 10);
    /**
     * @brief 事件转换函数
     * @param eventFlags 
     * @return int 
     */
    int convertMaskToType( int eventFlags);
    /**
     * @brief 线程函数
     * @return [*]
     * @note
     */
    void run();

private:
    /* 句柄 */
    FireDetect_NS::CFireDetectV1_0* m_pHandle = nullptr;

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

    /* 烟火检测配置 */
    Alarm::SmokeFireDetection_S m_stAlgoSmokeFireCfg;
    /* 明火检测配置 */
    Alarm::OpenFlameDetection_S m_stAlgoOpenFlameCfg;

    /* 检测频率控制 */
    EventManager m_RecvManager{500};

    /* 算法默认分辨率 */
    int m_nWidth = 640;
    int m_nHeight = 384;
};