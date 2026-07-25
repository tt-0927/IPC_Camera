/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-08 19:22:49
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-01-30 10:21:50
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/detect_mode/group3_detect/group3_detect.hpp
 * @Description: smoke(烟雾)、fire(火焰)、Overflow(垃圾满溢)、expose(垃圾暴露)、Complete(井盖完好)、Damaged(井盖破损)、Lost(井盖丢失)、Uncovered(未盖井盖)、BreakoutOfOuterEdge(井盖外边沿破损)、WaterAccumulation(道路积水)
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
#include "Group3DetectV1_0.hpp"

class CGroup3Detect : public CAlgorithm {
  public:
    CGroup3Detect();
    ~CGroup3Detect();

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
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;
    void setAlgoParamCfg(const Alarm::SmokeFireDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::SMOKE_FIRE);
    void setAlgoParamCfg(const Alarm::OpenFlameDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::OPEN_FLAME);
    void setAlgoParamCfg(const Alarm::GarbageExposureDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::GARBAGE_EXPOSURE);
    void setAlgoParamCfg(const Alarm::GarbageOverflowDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::GARBAGE_OVERFLOW);
    void setAlgoParamCfg(const Alarm::ManholeCoverAbnormalDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::MANHOLE_COVER_ABNORMAL);
    void setAlgoParamCfg(const Alarm::RoadPondingDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::ROAD_PONDING);

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
     * @param minConfidence 最低置信度（默认 0.2f，float 类型）
     * @param maxConfidence 最高置信度（默认 0.5f，float 类型）
     * @return float 置信度（范围：minConfidence ~ maxConfidence）
     */
    float sensitivityToConfidence(int sensitivity, float minConfidence = 0.2f, float maxConfidence = 0.9f);

    /**
     * @brief 将 1-100 的灵敏度转换为触发报警所需的连续帧数
     * @param sensitivity 灵敏度 (1-100)
     * @param minFrames 最高灵敏度 (100) 对应的帧数
     * @param maxFrames 最低灵敏度 (1) 对应的帧数s
     * @return int 触发报警所需的连续帧数
     */
    int sensitivityToFrames(int sensitivity, int minFrames = 1, int maxFrames = 10);
    /**
     * @brief 事件转换函数
     * @param eventFlags
     * @return int
     */
    int convertMaskToType(int eventFlags);
    /**
     * @brief 线程函数
     * @return [*]
     * @note
     */
    void run();

    void processGroup3Detect(const Group3Detect_NS::OutData_S &stOutData);

    /**
     * @brief 动态分析函数
     * @param vecAllResult 模型识别结果
     * @return int
     */
    int dynamicAnalysis(const std::vector<Group3Detect_NS::Result_S> &vecResult);

  private:
    /* 句柄 */
    Group3Detect_NS::CGroup3DetectV1_0 *m_pHandle = nullptr;

    /* 队列 */
    BQ_NS::CBlockingQueue<MediaData_S> m_dateQueue;

    /* 用于控制线程的运行 */
    std::atomic<bool> m_bRunning;
    /* 互斥锁保护指针访问 */
    std::mutex m_mutex;
    /* 条件变量 */
    std::condition_variable m_condition;
    /* 数据获取线程 */
    std::thread m_thread;

    /* 烟雾 识别相关配置 */
    Alarm::SmokeFireDetection_S m_stAlgoSmokeFireCfg;
    /* 火焰 识别相关配置 */
    Alarm::OpenFlameDetection_S m_stAlgoOpenFlameCfg;
    /* 垃圾暴露 识别相关配置 */
    Alarm::GarbageExposureDetection_S m_stAlgoGarbageExposureCfg;
    /* 垃圾满溢 识别相关配置 */
    Alarm::GarbageOverflowDetection_S m_stAlgoGarbageOverflowCfg;
    /* 井盖异常 识别相关配置 */
    Alarm::ManholeCoverAbnormalDetection_S m_stAlgoManholeCoverAbnormalCfg;
    /* 道路积水 识别相关配置 */
    Alarm::RoadPondingDetection_S m_stAlgoRoadPondingCfg;

    /* 检测频率控制 */
    EventManager m_RecvManager{500};

    /* 报警状态管理 */
    CAlarmStateMachine m_SmokeFireStateMachine;            /* 烟雾 */
    CAlarmStateMachine m_OpenFlameStateMachine;            /* 火焰 */
    CAlarmStateMachine m_GarbageExposureStateMachine;      /* 垃圾暴露 */
    CAlarmStateMachine m_GarbageOverStateMachine;          /* 垃圾满溢 */
    CAlarmStateMachine m_ManholeCoverAbnormalStateMachine; /* 井盖异常 */
    CAlarmStateMachine m_RoadPondingStateMachine;          /* 道路积水 */

    /* 算法默认分辨率 */
    int m_nWidth  = 640;
    int m_nHeight = 384;
};