/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-07 11:38:52
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-01-30 09:49:13
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/detect_mode/group1_detect/group1_detect.hpp
 * @Description: notHelmet(未戴安全帽)、helmet(安全帽)、reflective(反光衣)、safetyRope(安全绳)、exposedSoil(泥土裸露) 检测相关
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
#include "Group1DetectV1_0.hpp"

class CGroup1Detect : public CAlgorithm {
  public:
    CGroup1Detect();
    ~CGroup1Detect();

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
    void setAlgoParamCfg(const Alarm::SafetyHelmetDection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::SAFETY_HELMET);
    void setAlgoParamCfg(const Alarm::ReflectiveClothingDection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::REFLECTIVE_CLOTHING);
    void setAlgoParamCfg(const Alarm::HighAltitudeSeatbeltDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::HIGH_ALTITUDE_SEATBELT);
    void setAlgoParamCfg(const Alarm::BareSoiletDection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::BARE_SOIL);
    
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

    void processGroup1Detect(const Group1Detect_NS::OutData_S &stOutData);

    /**
     * @brief 动态分析函数
     * @param vecResult 模型识别结果
     * @return int
     */
    int dynamicAnalysis(const std::vector<Group1Detect_NS::Result_S> &vecResult);

  private:
    /* 句柄 */
    Group1Detect_NS::CGroup1DetectV1_0 *m_pHandle = nullptr;

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

    /* 安全帽识别 */
    Alarm::SafetyHelmetDection_S m_stAlgoSafetyHelmetCfg;
    /* 反光衣识别 */
    Alarm::ReflectiveClothingDection_S m_stAlgoReflectiveClothingCfg;
    /* 高空安全带识别 */
    Alarm::HighAltitudeSeatbeltDetection_S m_stAlgoHighAltitudeSeatbeltCfg;
    /* 泥土裸露 */
    Alarm::BareSoiletDection_S m_stAlgoBareSoiletCfg;

    /* 检测频率控制 */
    EventManager m_RecvManager{500};

    /* 报警状态管理 */
    CAlarmStateMachine m_SafetyHelmetStateMachine;         /* 安全帽识别 */
    CAlarmStateMachine m_ReflectiveClothingStateMachine;   /* 反光衣识别 */
    CAlarmStateMachine m_HighAltitudeSeatbeltStateMachine; /* 高空安全带识别 */
    CAlarmStateMachine m_BareSoilStateMachine;             /* 泥土裸露 */

    /* 算法默认分辨率 */
    int m_nWidth  = 640;
    int m_nHeight = 384;

    int m_nChannelId = 0;
    cv::Mat m_fullRgbMat;
};