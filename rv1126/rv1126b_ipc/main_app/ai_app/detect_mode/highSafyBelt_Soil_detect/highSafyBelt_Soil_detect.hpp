/**
 * @file highSafyBelt_Soil_detect.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-13
 * 
 * @brief 高空安全带黄土裸露检测
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
#include "SafetyropeAndSoilDetectV1_0.hpp"

class CHighSafyBeltSoilDetect : public CAlgorithm
{
public:

    CHighSafyBeltSoilDetect();
    ~CHighSafyBeltSoilDetect();

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
    void setAlgoParamCfg(const Alarm::HighAltitudeSeatbeltDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::HIGH_ALTITUDE_SEATBELT);
    void setAlgoParamCfg(const Alarm::BareSoiletDection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::BARE_SOIL);

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
     * @param minConfidence 最低置信度（默认 0.3f，float 类型）
     * @param maxConfidence 最高置信度（默认 0.5f，float 类型）
     * @return float 置信度（范围：minConfidence ~ maxConfidence）
     */
    float sensitivityToConfidence(int sensitivity, float minConfidence = 0.3f, float maxConfidence = 0.5f);
    
     /**
     * @brief 将 1-100 的灵敏度转换为触发报警所需的连续帧数
     * @param sensitivity 灵敏度 (1-100)
     * @param minFrames 最高灵敏度 (100) 对应的帧数
     * @param maxFrames 最低灵敏度 (1) 对应的帧数s
     * @return int 触发报警所需的连续帧数
     */
    int sensitivityToFrames(int sensitivity, int minFrames = 2, int maxFrames = 10);
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
    SafetyropeAndSoilDetect_NS::CSafetyropeAndSoilDetectV1_0* m_pHandle = nullptr;

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

    /* 高空安全带检测相关配置 */
    Alarm::HighAltitudeSeatbeltDetection_S m_stHighAltitudeSeatbeltCfg;
    /* 黄土裸露检测相关配置 */
    Alarm::BareSoiletDection_S m_stBareSoiletCfg;

    /* 检测频率控制 */
    EventManager m_RecvManager{500};

    /* 算法默认分辨率 */
    int m_nWidth = 640;
    int m_nHeight = 384;
};