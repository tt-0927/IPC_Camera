/**
 * @file object_detect.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-12
 * 
 * @brief 物品检测
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

class CObjectDetect : public CAlgorithm
{
public:

    CObjectDetect();
    ~CObjectDetect();

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
    void setAlgoParamCfg(const Alarm::UnattendedObject_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::UNATTENDED_OBJECT);
    void setAlgoParamCfg(const Alarm::ObjectRemoval_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::OBJECT_REMOVAL);
    

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
    std::vector<Event::RuleInfo> m_vstRuleInfo;

    /* 物品遗留配置 */
    Alarm::UnattendedObject_S m_stAlgoUnattendedObjectCfg;
    /* 物品拿取配置 */
    Alarm::ObjectRemoval_S m_stAlgoObjectRemovalCfg;

    /* 物品遗留时间阈值(单位：毫秒，从配置读取) */
    int m_nLeaveTimeThrd = 5000;

    /* 物品拿取时间阈值(单位：毫秒，从配置读取) */
    int m_nPickupTimeThrd = 5000;

    /* 记录每个区域最近一次 S1_rate 为 HIGH 的时间（用于物品拿取方向判断） */
    std::map<int, std::chrono::steady_clock::time_point> m_lastHighS1TimeMap;
    /* 记录每个区域 S1_rate 持续 HIGH(>5%) 的起始时间（用于排除短暂手部晃动） */
    std::map<int, std::chrono::steady_clock::time_point> m_highSustainedStartMap;
    /* 记录每个区域背景冻结的截止时间（ObjectRemoval 短暂冻结防止背景吸收） */
    std::map<int, std::chrono::steady_clock::time_point> m_freezeBgUntilMap;
    /* 记录每个区域 sustained HIGH 期间的 S1_rate 水平（用于相对阈值计算 isLow） */
    std::map<int, float> m_highS1RateMap;
    /* 记录每个区域事件是否已触发（用于事件结束判断） */
    std::map<int, bool> m_eventTriggeredMap;
    /* 记录每个区域事件结束时间（用于冷却期，防止冷却期内重复触发） */
    std::map<int, std::chrono::steady_clock::time_point> m_lastEndTimeMap;
    /* 记录每个区域 S1 最后一次跌至 LOW 的时间（用于冷却期内判断新的放置事件） */
    std::map<int, std::chrono::steady_clock::time_point> m_lastLowTimeMap;
    /* 记录每个区域事件触发时间（用于强制超时结束） */
    std::map<int, std::chrono::steady_clock::time_point> m_eventTriggerStartMap;
    /* 记录每个区域连续 HIGH 的起始时间（用于启动时物体已存在的拿取检测） */
    std::map<int, std::chrono::steady_clock::time_point> m_removalHighStartMap;
    /* 记录每个区域 OBJECT_REMOVAL HIGH 跌落时间（用于区分首次移除 vs 后续放置） */
    std::map<int, std::chrono::steady_clock::time_point> m_removalHighDropTime;
    /* 鬼影检测相关（仅bOnlyRemoval模式） */
    std::map<int, std::chrono::steady_clock::time_point> m_ghostStartMap;
    std::map<int, bool> m_ghostUsedMap;

    /* 物品检测灵敏度 静态场景 x=5，动态场景 x=10~15 */
    float m_fSensiThrd = 5.0f;

    /* 检测频率控制 */
    EventManager m_RecvManager{500};

    /* 稳定背景帧 */
    cv::Mat m_backgroundFrame;

    /* 记录背景帧最后一次更新时间 */
    std::chrono::steady_clock::time_point m_lastBgUpdateTime;

    bool m_bOnlyRemoval = false; // 单独开启物品拿取
    /* 有限状态机: 记录物品状态 */
    enum ItemState { NONE, LEFT, PICKED }; ItemState m_lastItemState = NONE;

     /* 算法默认分辨率 */
    int m_nWidth = 1280;
    int m_nHeight = 720;

    int m_nChannelId = 0;
    cv::Mat m_lastRgbFrame;
};