/**
 * @file rubish_detect.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-13
 * 
 * @brief 垃圾检测相关
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
#include "RubishDetectV1_0.hpp"

class CRubishDetect : public CAlgorithm
{
public:

    CRubishDetect();
    ~CRubishDetect();

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
    void setAlgoParamCfg(const Alarm::GarbageExposureDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::GARBAGE_EXPOSURE);
    void setAlgoParamCfg(const Alarm::GarbageOverflowDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::GARBAGE_OVERFLOW);

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
     * @brief 灵敏度转置信度（反向映射：灵敏度越高，置信度越低）
     * @param sensitivity 输入灵敏度（范围：1~100，超出会被 clamp 到该范围）
     * @param minConfidence 最低置信度（默认 0.5f，float 类型）
     * @param maxConfidence 最高置信度（默认 0.9f，float 类型）
     * @return float 置信度（范围：minConfidence ~ maxConfidence）
     */
    float sensitivityToConfidence(int sensitivity, float minConfidence = 0.5f, float maxConfidence = 0.9f);
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
    RubishDetect_NS::CRubishDetectV1_0* m_pHandle = nullptr;

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

    /* 垃圾暴露检测配置 */
    Alarm::GarbageExposureDetection_S m_stAlgoGarbageExposureCfg;
    /* 垃圾满溢检测配置 */
    Alarm::GarbageOverflowDetection_S m_stAlgoGarbageOverflowCfg;

    /* 检测频率控制 */
    EventManager m_RecvManager{500};

    /* 算法默认分辨率 */
    int m_nWidth = 640;
    int m_nHeight = 384;
};