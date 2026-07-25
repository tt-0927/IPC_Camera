/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-27 17:24:46
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-12-10 08:58:47
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/detect_mode/vehicle_detect/vehicle_detect.hpp
 * @Description: 车辆检测
 */

 #pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <sys/time.h>
#include "blocking_queue.hpp"
#include <opencv2/opencv.hpp>
#include "VehicleDetectV2_0.hpp"
#include "event_manager.hpp"
#include "stream_process_ext.hpp"
#include "algorithm.hpp"
#ifdef ENABLE_GAT1400_SRC
#include "gat1400_utils.h"
#endif
class CVehicleDetect : public CAlgorithm
{
public:
    CVehicleDetect();
    ~CVehicleDetect();

    typedef struct _VehicleDetectEventType_
    {
      /* 逆行 */
      bool bDrivingAgainstTraffic = false;
      /* 违规停车 */
      bool bIllegalParking = false;
      /* 违规变道 */
      bool bIllegalLaneChange = false;
      /* 拥堵识别 */
      bool bCongestion = false;
    }VehicleDetectEventType_S;

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
     * @brief   : 更新逆行侦测参数 
     * @param    {DrivingAgainstTrafficDetection_S} &逆行
    */
    void setAlgoParamCfg(const Alarm::DrivingAgainstTrafficDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::REVERSE_DIRECTION);

    /**
     * @brief   : 更新违规变道参数 
     * @param    {IllegalLaneChangeDetection_S} &stAlgoCfg：违规变道
    */
    void setAlgoParamCfg(const Alarm::IllegalLaneChangeDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::ILLEGAL_LANE_CHANGE);

    /**
     * @brief   : 更新违停参数 
     * @param    {IllegalLaneChangeDetection_S} &stAlgoCfg：违停
    */
    void setAlgoParamCfg(const Alarm::ParkingDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::ILLEGAL_LANE_CHANGE);

    /**
     * @brief   : 更新拥堵识别参数 
     * @param    {CongestionDetection_S} &stAlgoCfg：拥堵识别
    */
    void setAlgoParamCfg(const Alarm::CongestionDetection_S &stAlgoCfg, Event::Type_E enType);


    /**
     * @brief 显示出检测区域
     * @param inMat 
     * @param nChnId 
    */
    void drawRulesToImage(cv::Mat& inMat);

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
     * @brief 事件转换函数
     * @param eventFlags 
     * @param stEventStatus 
     * @return int 
    */
    int convertMaskToType(int eventFlags, VehicleDetectEventType_S &stEventStatus);

    /**
     * @brief 灵敏度转置信度（反向映射：灵敏度越高，置信度越低）
     * @param sensitivity 输入灵敏度（范围：1~100，超出会被 clamp 到该范围）
     * @param minConfidence 最低置信度（默认 0.05f，float 类型）
     * @param maxConfidence 最高置信度（默认 0.95f，float 类型）
     * @return float 置信度（范围：minConfidence ~ maxConfidence）
     */
    float sensitivityToConfidence(int sensitivity, float minConfidence = 0.05f, float maxConfidence = 0.95f);

    /**
      * @brief 线程函数
      * @return [*]
      * @note
    */
    void run();

    /**
     * @brief   : 警戒线转换区域坐标并判断是否使能算法
    */
    template<typename T>
    void convertAlertLineToZoneAndIsEnable(T &stConfig, Event::Type_E enType);

    /**
     * @brief   : 警戒区域转换区域坐标并判断是否使能算法
    */
    void convertParkingAndEnable(Alarm::ParkingDetection_S &stAlgoCfg, Event::Type_E enType);

#ifdef ENABLE_GAT1400_SRC
    /**
     * @description  : 上传机动车图片到gat1400平台
     * @param         {Mat} &image
     * @param         {vector<VehicleDetect_NS::Result_S>} &vecResult
     * @return        {*}
     */
    void pushVehiclesToGat1400(const cv::Mat &image, const std::vector<VehicleDetect_NS::Result_S> &vecResult);

    /**
     * @description  : 上传gat1400平台预处理
     * @param         {Mat} &image
     * @param         {vector<VehicleDetect_NS::Result_S>} &vecResult
     * @param         {int} eventFlags
     * @return        {*}
     */    
    void preProcessGat1400(const cv::Mat &image, const std::vector<VehicleDetect_NS::Result_S> &vecResult, int eventFlags);
#endif

    /* 车辆识别后处理 */
    void processVehicleDetect(int eventFlags); 

private:
    /* 句柄 */
    VehicleDetect_NS::CVehicleDetectV2_0* m_pVehicleDetectHandle = nullptr;

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

    /* 逆行侦测配置 */
    Alarm::DrivingAgainstTrafficDetection_S m_stDrivingAgainstTrafficDetectionCfg;
    /* 拥堵配置 */
    Alarm::CongestionDetection_S m_stAlgoCongestionDetectionCfg;
    /* 违规停车配置 */
    Alarm::ParkingDetection_S m_stAlgoParkingDetectionCfg;
    /* 违规变道配置 */
    Alarm::IllegalLaneChangeDetection_S m_stAlgoIllegalLaneChangeDetectionCfg;

    /* 规则配置 */
    typedef struct TrafficRuleInfo
    {
        Alarm::CrossDirection_E enCrossDirection = Alarm::CrossDirection_E::CROSS_DIRECTION_INVALID;
        Event::RuleInfo stRuleInfo;
    }TrafficRuleInfo_S;

    /* 逆行规则 */
    std::vector<TrafficRuleInfo_S> m_vstDrivingAgainstTrafficRule;
    /* 违规变道规则 */
    std::vector<TrafficRuleInfo_S> m_vstIllegalLaneChangeRule;
    /* 违规停车规则 */
    std::vector<Event::RuleInfo> m_vstIllegalParkingRule;

    /* 报警状态管理 */
    CAlarmStateMachine m_DrivingAgainstTrafficStateMachine;  /* 逆行 */
    CAlarmStateMachine m_IllegalLaneChangeStateMachine;      /* 违规变道 */
    CAlarmStateMachine m_IllegalParkingStateMachine;         /* 违规停车 */
    CAlarmStateMachine m_CongestionStateMachine;             /* 拥堵 */
    
#ifdef ENABLE_GAT1400_SRC
    /* 违规停车事件上传管理 */
    CUploadStateMachine m_IllegalParkingUpload;
#endif
    /* 算法默认分辨率 */
    int m_nWidth = 640;
    int m_nHeight = 384;

};