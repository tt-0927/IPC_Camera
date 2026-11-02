/**
 * @file fire_safety_detect.hpp
 * @author xushou (xushou@kfb.cn)
 * @date 2025-11-12
 * 
 * @brief 消防设施异常检测
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

class CFireEquipmentDetect : public CAlgorithm
{
public:

    CFireEquipmentDetect();
    ~CFireEquipmentDetect();

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
    void setAlgoParamCfg(const Alarm::FireEquipmentAbnormalDetection_S &stAlgoCfg,
                         Event::Type_E enType = Event::Type_E::FIRE_EQUIPMENT_ABNORMAL);
    

    /**
     * @brief 显示出检测区域
     * @param inMat 
     * @param nChnId 
     */
    void drawRulesToImage(cv::Mat& inMat);
     /**
      * @brief 转换区域坐标并判断是否使能算法
      * @tparam T 
      * @param stConfig 
      */
     template<typename T>
     void convertResolutionAndEnable(T &stConfig,Event::Type enType);

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

private:

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
    
    /* 规则区域 */
    Event::RuleInfo m_stRuleInfo;
    /* 区域是否有效 */
    bool m_bRuleValid = false;

    /* 消防设施异常配置 */
    Alarm::FireEquipmentAbnormalDetection_S m_stAlgoFireEquipmentCfg;

    /* 时间阈值(单位：毫秒，硬编码5秒) */
    int m_nLeaveTimeThrd = 5000;

    /* 记录最近一次 S1_rate 为 HIGH 的时间 */
    std::chrono::steady_clock::time_point m_lastHighS1Time;
    /* 记录 S1_rate 持续 HIGH 的起始时间 */
    std::chrono::steady_clock::time_point m_highSustainedStart;
    /* 记录背景冻结的截止时间 */
    std::chrono::steady_clock::time_point m_freezeBgUntil;
    /* 记录 sustained HIGH 期间的 S1_rate 水平 */
    float m_highS1Rate = 0.0f;
    /* 记录事件是否已触发 */
    bool m_bEventTriggered = false;
    /* 记录事件结束时间 */
    std::chrono::steady_clock::time_point m_lastEndTime;
    /* 记录 S1 最后一次跌至 LOW 的时间 */
    std::chrono::steady_clock::time_point m_lastLowTime;
    /* 记录事件触发时间 */
    std::chrono::steady_clock::time_point m_eventTriggerStart;

    /* 检测灵敏度 静态场景 x=5，动态场景 x=10~15 */
    float m_fSensiThrd = 5.0f;

    /* 检测频率控制 */
    EventManager m_RecvManager{500};

    /* 稳定背景帧 */
    cv::Mat m_backgroundFrame;

    /* 记录背景帧最后一次更新时间 */
    std::chrono::steady_clock::time_point m_lastBgUpdateTime;

     /* 算法默认分辨率 */
    int m_nWidth = PIXEL_WIDTH_AI;
    int m_nHeight = PIXEL_HEIGHT_AI;

    int m_nChannelId = 0;
    cv::Mat m_lastRgbFrame;
};