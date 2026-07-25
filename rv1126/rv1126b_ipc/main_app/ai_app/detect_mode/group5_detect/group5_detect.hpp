/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-08 19:22:49
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-01-30 10:28:42
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/detect_mode/group3_detect/group5_detect.hpp
 * @Description: metalFence(金属栅栏)、ConeTank(锥形桶)、CrashBarrels(防撞桶)、fence(防护栏)
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
#include "Group5DetectV1_0.hpp"

class CGroup5Detect : public CAlgorithm {
  public:
    CGroup5Detect();
    ~CGroup5Detect();

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
    void setAlgoParamCfg(const Alarm::HoleProtectionBarDection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::HOLE_PROTECTION_BAR);
    void setAlgoParamCfg(const Alarm::ConstructionEncroachmentRoadDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::CONSTRUCTION_OCCUPY_ROAD);

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

    void processGroup5Detect(const Group5Detect_NS::OutData_S &stOutData);

    /**
     * @brief 动态分析函数
     * @param vecAllResult 模型识别结果
     * @return int
     */
    int dynamicAnalysis(const std::vector<Group5Detect_NS::Result_S> &vecResult);

  private:
    /* 句柄 */
    Group5Detect_NS::CGroup5DetectV1_0 *m_pHandle = nullptr;

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

    /* 洞口防护栏识别相关配置 */
    Alarm::HoleProtectionBarDection_S m_stAlgoHoleProtectionBarCfg;
    /* 施工占道识别相关配置 */
    Alarm::ConstructionEncroachmentRoadDetection_S m_stAlgoConstructionEncroachmentRoadCfg;

    /* 检测频率控制 */
    EventManager m_RecvManager{500};

    /* 报警状态管理 */
    CAlarmStateMachine m_HoleProtectionBarStateMachine;            /* 洞口防护栏识别 */
    CAlarmStateMachine m_ConstructionEncroachmentRoadStateMachine; /* 施工占道 */

    /* 算法默认分辨率 */
    int m_nWidth  = 640;
    int m_nHeight = 384;
};