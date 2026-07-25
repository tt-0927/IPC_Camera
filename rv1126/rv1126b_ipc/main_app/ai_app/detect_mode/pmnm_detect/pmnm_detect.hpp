/**
 * @file pmnm_detect.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-05
 * 
 * @brief 机动车、行人、非机动车检测相关
 */
#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <sys/time.h>
#include "blocking_queue.hpp"
#include "algorithm.hpp"
#include <opencv2/opencv.hpp>
#include "PMNMDetectV2_0.hpp"
#ifdef ENABLE_GAT1400_SRC
#include "gat1400_utils.h"
#endif
class CPMNMDetect : public CAlgorithm
{
public:
    CPMNMDetect();
    ~CPMNMDetect();

    typedef struct PMNMDetectRuleInfo
    {
        // std::vector<PMNMDetect_NS::DetectionTargetType_E> veDetectionTargets;
        std::vector<int> veDetectionTargets; /* 0-人 1-机动车 2-非机动车 */
        Event::RuleInfo stRuleInfo;
    }PMNMDetectRuleInfo_S;

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
     * @brief   : 更新越界侦测参数 
     * @param    {BoundaryDetection_S} &stAlgoCfg：越界侦测
    */
    void setAlgoParamCfg(const Alarm::BoundaryDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::LINE_CROSSING);

     /**
     * @brief   : 更新区域入侵参数 
     * @param    {FieldDetection_S} &stAlgoCfg：区域入侵
    */
    void setAlgoParamCfg(const Alarm::FieldDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::INTRUSION);

    /**
     * @brief   : 更新 进入区域参数 
     * @param    {EntranceDetection_S} & 进入区域
    */
    void setAlgoParamCfg(const Alarm::EntranceDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::ENTER_REGION);

    /**
     * @brief   : 更新离开区域参数 
     * @param    {ExitingDetection_S} &离开区域
    */
    void setAlgoParamCfg(const Alarm::ExitingDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::LEAVE_REGION);

    /**
     * @brief 更新应急车道占用检测区域参数
     * @param stRuleInfo 
    */
    void setAlgoParamCfg(const Alarm::EmergencyLaneOccupancyDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::EMERGENCY_LANE_OCCUPANCY);

    /**
     * @brief 更新非机动车闯入识别区域参数
     * @param stRuleInfo 
    */
    void setAlgoParamCfg(const Alarm::NonMotorVehicleIntrusionDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::NON_MOTOR_VEHICLE_INTRUSION);

    /**
     * @brief 灵敏度转置信度（反向映射：灵敏度越高，置信度越低）
     * @param sensitivity 输入灵敏度（范围：1~100，超出会被 clamp 到该范围）
     * @param minConfidence 最低置信度（默认 0.1f，float 类型）
     * @param maxConfidence 最高置信度（默认 0.9f，float 类型）
     * @return float 置信度（范围：minConfidence ~ maxConfidence）
     */
    float sensitivityToConfidence(int sensitivity, float minConfidence = 0.1f, float maxConfidence = 0.9f);

    /**
     * @brief 将 1-100 的灵敏度转换为触发报警所需的连续帧数
     * @param sensitivity 灵敏度 (1-100)
     * @param minFrames 最高灵敏度 (100) 对应的帧数
     * @param maxFrames 最低灵敏度 (1) 对应的帧数s
     * @return int 触发报警所需的连续帧数
     */
    int sensitivityToFrames(int sensitivity, int minFrames = 1, int maxFrames = 10);
    
    /**
     * @brief 更新边界检测划线数据
     * @param stRuleInfo 
    */
    void changeRuleInfos(const Event::RuleInfo &stRuleInfo, std::vector<int> veDetectionTargets, bool bNeedClear);


    /**
     * @brief 显示出检测区域
     * @param inMat 
     * @param nChnId 
    */
    void drawRulesToImage(cv::Mat& inMat);

    /**
     * @brief 事件转换函数
     * @param eventFlags 
     * @return int 
     */
    int convertMaskToType( int eventFlags);

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
     * @brief   : 越界侦测转换区域坐标并判断是否使能算法
     */
     void convertBoundaryAndEnable(Alarm::BoundaryDetection_S &stConfig);

     /**
      * @brief 转换区域坐标并判断是否使能算法
      * @tparam T 
      * @param stConfig 
      */
     template<typename T>
     void convertResolutionAndEnable(T &stConfig,Event::Type enType);

#ifdef ENABLE_GAT1400_SRC
    void pushImageToGat1400(const cv::Mat &image, const std::vector<PMNMDetect_NS::Result_S> &vecResult);
#endif

private:
     /* 句柄 */
    PMNMDetect_NS::CPMNMDetectV2_0* m_pHandle = nullptr;

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
    std::vector<PMNMDetectRuleInfo_S> m_vstRuleInfo;
    /* 检测频率控制 */
    EventManager m_RecvManager{500};
    // /* 事件管理器, 过滤重复事件 */
    // EventManager m_EventManager{20 * 1000};
    /* 框的置信度阈值 */
    float m_fBoxThreshold = 0.25;

    /* 越界侦测配置 */
    Alarm::BoundaryDetection_S m_stAlgoCrossCfg;
    /* 区域入侵配置 */
    Alarm::FieldDetection_S m_stAlgoIntruCfg;
    /* 进入区域配置 */
    Alarm::EntranceDetection_S m_stAlgoEntryCfg;
    /* 离开区域配置 */
    Alarm::ExitingDetection_S m_stAlgoExitCfg;
    /* 应急车道占用配置 */
    Alarm::EmergencyLaneOccupancyDetection_S m_stAlgoEmergencyLaneOccupancyCfg;
    /* 非机动车闯入配置 */
    Alarm::NonMotorVehicleIntrusionDetection_S m_stAlgoNonMotorVehicleIntrusionCfg;

    /* 规则配置 */
    std::vector<PMNMDetectRuleInfo_S> m_vstCrossRule;
    std::vector<PMNMDetectRuleInfo_S> m_vstIntruRule;
    std::vector<PMNMDetectRuleInfo_S> m_vstEntryRule;
    std::vector<PMNMDetectRuleInfo_S> m_vstExitRule;
    std::vector<PMNMDetectRuleInfo_S> m_vstEmergencyLaneOccupancyRule;
    std::vector<PMNMDetectRuleInfo_S> m_vstNonMotorVehicleIntrusionRule;

    /* 报警状态管理 */
    CAlarmStateMachine m_CrossAlarmStateMachine;
    CAlarmStateMachine m_IntruAlarmStateMachine;
    CAlarmStateMachine m_EntryAlarmStateMachine;
    CAlarmStateMachine m_ExitAlarmStateMachine;

#ifdef ENABLE_GAT1400_SRC
    /* 侵入事件上传gat1400平台间隔管理 */
    CUploadInterval m_IntruUploadInterval;
#endif
    /* 算法默认分辨率 */
    int m_nWidth = 640;
    int m_nHeight = 640;

};