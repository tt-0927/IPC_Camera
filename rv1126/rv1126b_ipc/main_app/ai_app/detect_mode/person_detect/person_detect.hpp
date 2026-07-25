/**
 * @file person_detect.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-05
 * 
 * @brief 行人检测相关
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
#include "HumanAreaDetectV3_0.hpp"
 #include "event_manager.hpp"
 #include "stream_process_ext.hpp"
#include "task_publish.h"
#include "algorithm.hpp"
#include "PlayPhoneDetectV1_0.hpp"
 
class CPersonDetect : public CAlgorithm
{
public:
    CPersonDetect();
    ~CPersonDetect();

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
#if 0
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
#endif
    /**
     * @brief   : 更新徘徊侦测区域参数 
     * @param    {LoiteringDetection_S} &徘徊侦测区域
     */
     void setAlgoParamCfg(const Alarm::LoiteringDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::LOITERING_DETECT);
     /**
     * @brief   : 更新翻越围栏参数 
     * @param    {FenceClimbingDetection_S} &翻越围栏区域
     */
     void setAlgoParamCfg(const Alarm::FenceClimbingDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::FENCE_CLIMBING);
     /**
     * @brief   : 更新离岗识别参数 
     * @param    {LeavePostDetection_S} &离岗区域
     */
     void setAlgoParamCfg(const Alarm::LeavePostDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::LEAVE_POST);
     /**
     * @brief   : 更新行人闯入参数 
     * @param    {PedestrianIntrusionDetection_S} &行人闯入区域
     */
     void setAlgoParamCfg(const Alarm::PedestrianIntrusionDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::PEDESTRIAN_INTRUSION);
     /**
     * @brief   : 更新人员聚集参数 
     * @param    {CrowdGathering_S} &人员聚集
     */
    void setAlgoParamCfg(const Alarm::CrowdGathering_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::CROWD_GATHERING);
     /**
     * @brief   : 更新人员倒地参数 
     * @param    {CrowdGathering_S} &人员聚集
     */
    void setAlgoParamCfg(const Alarm::PersonFallDownDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::PERSON_FALL_DOWN);
    
    /**
     * @brief   : 更新玩手机参数 
     * @param    {CrowdGathering_S} &人员聚集
     */
    void setAlgoParamCfg(const Alarm::PhoneUsageDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::PHONE_USAGE);

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
    void changeRuleInfos(const Event::RuleInfo &stRuleInfo, bool bNeedClear);

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
      * @brief 初始化行人检测模型
      * @return [*]
      * @note
      */
     bool init();

    /**
      * @brief 初始化玩手机检测模型
      * @return [*]
      * @note
    */ 
     bool initPlayPhoneDetect();
 
     /**
      * @brief 反初始化行人检测模型
      * @return [*]
      * @note
      */
     bool unInit();

     /**
      * @brief 反初始化玩手机检测模型
      * @return [*]
      * @note
      */
     bool unInitPlayPhoneDetect();
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
     * @brief   : 人员聚集转换区域坐标并判断是否使能算法
     */
    void convertCrowdGatherAndEnable(Alarm::CrowdGathering_S &stAlgoCfg);
     /**
      * @brief 转换区域坐标并判断是否使能算法
      * @tparam T 
      * @param stConfig 
      */
     template<typename T>
     void convertResolutionAndEnable(T &stConfig,Event::Type enType);

#ifdef ENABLE_GAT1400_SRC
    /**
     * @description  : 上传人员图片到gat1400平台
     * @param         {Mat} &image
     * @param         {vector<HumanAreaDetect_NS::Result_S>} &vecResult
     * @return        {*}
     */    
    void pushPersonImageToGat1400(const cv::Mat &image, const std::vector<HumanAreaDetect_NS::Result_S> &vecResult);
#endif

    /**
     * @description  : 检测玩手机
     * @param         {Mat} &srcDate 图片数据
     * @param         {vector<HumanAreaDetect_NS::Result_S>} &vecResult 人形所在矩形信息
     * @return        {*}
     */
    void detectPlayPhone(cv::Mat &srcDate, std::vector<HumanAreaDetect_NS::Result_S> &vstModelDetectResult);

private:
     /* 句柄 */
     HumanAreaDetect_NS::CHumanAreaDetectV3_0* m_pHumanAreaDetectHandle = nullptr;
     PlayPhoneDetect_NS::CPlayPhoneDetectV1_0* m_pPlayPhoneDetectHandle = nullptr;


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
     /* 检测频率控制 */
     EventManager m_RecvManager{500};
     /* 事件管理器, 过滤重复事件 */
     EventManager m_EventManager{20 * 1000};
     /* 框的置信度阈值 */
     float m_fBoxThreshold = 0.25;

    //  /* 越界侦测配置 */
    //  Alarm::BoundaryDetection_S m_stAlgoCrossCfg;
    //  /* 区域入侵配置 */
    //  Alarm::FieldDetection_S m_stAlgoIntruCfg;
    //  /* 进入区域配置 */
    //  Alarm::EntranceDetection_S m_stAlgoEntryCfg;
    //  /* 离开区域配置 */
    //  Alarm::ExitingDetection_S m_stAlgoExitCfg;
     /* 徘徊侦测相关配置 */
     Alarm::LoiteringDetection_S m_stLoiteringCfg;
    /* 人员倒地配置 */
     Alarm::PersonFallDownDetection_S m_stPersonFallDownCfg;
     /* 翻越围栏配置 */
     Alarm::FenceClimbingDetection_S m_stFenceClimbingCfg;
     /* 离岗配置 */
     Alarm::LeavePostDetection_S m_stLeavePostCfg;
     /* 行人闯入配置 */
     Alarm::PedestrianIntrusionDetection_S m_stPedestrianIntrusionCfg;
    /* 人员聚集配置 */
     Alarm::CrowdGathering_S m_stCrowdGatheringDetCfg; 
     /* 玩手机识别识别相关配置 */
     Alarm::PhoneUsageDetection_S m_stPhoneUsageCfg;

     /* 规则配置 */
     std::vector<Event::RuleInfo> m_vstCrossRule;
     std::vector<Event::RuleInfo> m_vstIntruRule;
     std::vector<Event::RuleInfo> m_vstEntryRule;
     std::vector<Event::RuleInfo> m_vstExitRule;
     std::vector<Event::RuleInfo> m_vstLoiteringRule;
     std::vector<Event::RuleInfo> m_vstFenceClimbingRule;
     std::vector<Event::RuleInfo> m_vstLeavePostRule;
     std::vector<Event::RuleInfo> m_vstPedestrianIntrusionRule;
     std::vector<Event::RuleInfo> m_vstCrowdGatheringDetRule;

     /* 报警状态管理 */
     CAlarmStateMachine m_CrossAlarmStateMachine;
     CAlarmStateMachine m_IntruAlarmStateMachine;
     CAlarmStateMachine m_EntryAlarmStateMachine;
     CAlarmStateMachine m_ExitAlarmStateMachine;

     /* 算法默认分辨率 */
    int m_nWidth = 640;
    int m_nHeight = 384;

};