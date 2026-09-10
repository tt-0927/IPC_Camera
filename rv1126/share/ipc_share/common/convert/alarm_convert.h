/**
 * @FilePath     : alarm_convert.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-18 09:54:33
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-20 14:14:56
 * @Description  : 报警配置数据的转换 
 */

#pragma once
#include "alarm_define.h"
#include "common_define.h"
#include "Json.h"
#include <vector>
#include <set>

namespace Convert
{
    /* 联动方式相关 */
    void deal(Json::Object *pRootJson, Alarm::LinkageType_E &enLinkageType, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::LinkageList_S &stLinkageList, bool bOutStruct);

    /**
     * @brief   : 普通事件
     */
    /* 移动侦测相关 */
    void deal(Json::Object *pRootJson, Alarm::MotionRegion_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<Alarm::MotionRegion_S> &vstInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::MotionExpertMode_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::MotionNormalMode_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::MotionDetection_S &stInfo, bool bOutStruct);
    /* 遮挡报警相关 */
    void deal(Json::Object *pRootJson, Alarm::HideAlarm_S &stInfo, bool bOutStruct);
    /* 异常报警相关 */
    void deal(Json::Object* pRootJson, Alarm::AbnormalDetection_S &stAbnormalDetection, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::set<Alarm::AbnormalDetection_S> &abnormalDetection, bool bOutStruct);
    /* 声音报警输出相关 */
    void deal(Json::Object *pRootJson, Alarm::SoundOutputAlarm_S &stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, Alarm::CustomAudio_S &stInfo, bool bOutStruct);  
    void deal(Json::Object* pRootJson, std::vector<Alarm::CustomAudio_S> &stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, Alarm::CustomOperation_S &stInfo, bool bOutStruct); 
    void deal(Json::Object* pRootJson, std::vector<Alarm::CustomOperation_S> &stInfo, bool bOutStruct);
    /* 报警输入相关 */
    void deal(Json::Object* pRootJson, Alarm::IoInputInfo_S &stIOInputInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::set<Alarm::IoInputInfo_S> &stIOInputInfo, bool bOutStruct);
    /* 报警输出相关 */
    void deal(Json::Object* pRootJson, Alarm::IoOutputInfo_S &stIOOutputInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::set<Alarm::IoOutputInfo_S> &stIOOutputInfo, bool bOutStruct);
    /* 闪光报警相关 */
    void deal(Json::Object *pRootJson, Alarm::FlashInfo_S &stInfo, bool bOutStruct);
    /* PIR报警相关 */
    void deal(Json::Object *pRootJson, Alarm::PirAlarmInfo_S &stInfo, bool bOutStruct);

    /**
     * @brief   : 周界事件
     */

    /* 越界侦测的区域信息转换 */
     void deal(Json::Object *pRootJson, Alarm::BoundaryPlane_S &stInfo, bool bOutStruct);
     void deal(Json::Object *pRootJson, std::vector<Alarm::BoundaryPlane_S> &stInfo, bool bOutStruct);    
 
     /*  越界侦测配置信息转换 */
     void deal(Json::Object *pRootJson, Alarm::BoundaryDetection_S &stInfo, bool bOutStruct);
     
    /* 区域入侵的规则配置信息 */
     void deal(Json::Object *pRootJson, Alarm::Intrusion_S &stInfo, bool bOutStruct);
     void deal(Json::Object *pRootJson, std::vector<Alarm::Intrusion_S> &stInfo, bool bOutStruct);

    /* 进入/离开区域的规则配置信息 */
    void deal(Json::Object *pRootJson, Alarm::EnterExitIntrusion_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<Alarm::EnterExitIntrusion_S> &stInfo, bool bOutStruct);
 
    /* 区域入侵侦测相关 */
     void deal(Json::Object *pRootJson, Alarm::FieldDetection_S &stInfo, bool bOutStruct);

    /* 进入区域侦测相关 */
    void deal(Json::Object *pRootJson, Alarm::EntranceDetection_S &stInfo, bool bOutStruct);

    /* 离开区域侦测相关 */
    void deal(Json::Object *pRootJson, Alarm::ExitingDetection_S &stInfo, bool bOutStruct);

    /**
     * @brief   : smart事件
     */
    /* 音频异常侦测相关 */
    void deal(Json::Object *pRootJson, Alarm::AudioAnomaly_S &stInfo, bool bOutStruct);
    /* 场景变更侦测相关 */
    void deal(Json::Object *pRootJson, Alarm::SceneChange_S &stInfo, bool bOutStruct);
    /* 人脸侦测相关 */
    void deal(Json::Object *pRootJson, Alarm::FaceDetection_S &stInfo, bool bOutStruct);
    /* 徘徊侦测相关 */
    void deal(Json::Object *pRootJson, Alarm::LoiteringRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::LoiteringDetection_S &stInfo, bool bOutStruct);
    /* 人员聚集侦测相关 */
    void deal(Json::Object *pRootJson, Alarm::CrowdGatheringRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::CrowdGathering_S &stInfo, bool bOutStruct);
    /* 停车侦测相关 */
    void deal(Json::Object *pRootJson, Alarm::ParkingRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::ParkingDetection_S &stInfo, bool bOutStruct);
    /* 物品遗留侦测相关 */
    void deal(Json::Object *pRootJson, Alarm::UnattendedObjectRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::UnattendedObject_S &stInfo, bool bOutStruct);
    /* 物品拾取侦测相关 */
    void deal(Json::Object *pRootJson, Alarm::ObjectRemovalRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::ObjectRemoval_S &stInfo, bool bOutStruct);
    /* 宠物识别相关 */
    void deal(Json::Object *pRootJson, Alarm::PetRecognition_S &stInfo, bool bOutStruct);
    /*人脸比对*/
    void deal(Json::Object *pRootJson, Alarm::FaceCompare_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::TargetLibInfos_S &TargetLibInfos, bool bOutStruct);
    /* 人脸抓拍相关 */
    void deal(Json::Object *pRootJson, Alarm::FaceCaptureRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::FaceCapture_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::OverlayInfo_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::FaceAlarmAttribute_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::FaceAlarmInfo_S &stInfo, bool bOutStruct);
#ifdef SCENE_INTELLIGENT_ANALYSIS
    /**
    * @brief   : 场景智能分析
    */
    /*场景智能分析控制*/
    void deal(Json::Object *pRootJson, Alarm::LLMAISceneAnalysis_S &stInfo, bool bOutStruct);
    
    /* 画面分析相关 */
    void deal(Json::Object *pRootJson, Alarm::LLMImageAnalysis_S &stInfo, bool bOutStruct);
    /* 间隔分析 */
    void deal(Json::Object *pRootJson, Alarm::IntervalAnalysisConfig_S &stInfo, bool bOutStruct);
    /* 重复分析 */
    void deal(Json::Object *pRootJson, Alarm::RepeatedAnalysisConfig_S &stInfo, bool bOutStruct);
    /*画面分析记录*/
    void deal(Json::Object *pRootJson, Alarm::AnalysisRecords_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::AnalysisRecordIndexItem_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::AnalysisAllRecordIndexItem_S &stInfo, bool bOutStruct);

    /* 文字预设任务相关 */
    void deal(Json::Object *pRootJson, Alarm::TextPreset_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<Alarm::TextPreset_S> &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::TextPresetTaskManager_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::TextPresetQueryFilter_S &stInfo, bool bOutStruct);

    /* 实时预警推送相关 */
    void deal(Json::Object *pRootJson, Alarm::RealAlarmPushTime_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::RealAlarmProcessRecord_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<Alarm::RealAlarmProcessRecord_S> &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::RealAlarmPushRecord_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<Alarm::RealAlarmPushRecord_S> &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::RealAlarmPushQueryFilter_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::RealAlarmPushManager_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::RealAlarmPushBatchRequest_S &stInfo, bool bOutStruct);
    /* 实时预警推送相关 */
#endif

#ifdef SCENE_INTELLIGENCE
    /**
    * @brief   : 场景智能
    */
    /* 翻越围栏侦测相关 */
    void deal(Json::Object *pRootJson, Alarm::FenceClimbingRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::FenceClimbingDetection_S &stInfo, bool bOutStruct);
    /* 离岗相关 */
    void deal(Json::Object *pRootJson, Alarm::LeavePostRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::LeavePostDetection_S &stInfo, bool bOutStruct);
    /* 行人闯入相关 */
    void deal(Json::Object *pRootJson, Alarm::PedestrianIntrusionRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::PedestrianIntrusionDetection_S &stInfo, bool bOutStruct);
    /* 烟火检测相关 */
    void deal(Json::Object *pRootJson, Alarm::SmokeFireRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::SmokeFireDetection_S &stInfo, bool bOutStruct);
    /* 明火检测相关 */
    void deal(Json::Object *pRootJson, Alarm::OpenFlameRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::OpenFlameDetection_S &stInfo, bool bOutStruct);
    /* 道路积水检测相关 */
    void deal(Json::Object *pRootJson, Alarm::RoadPondingRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::RoadPondingDetection_S &stInfo, bool bOutStruct);
    /* 井盖异常检测相关 */
    void deal(Json::Object *pRootJson, Alarm::ManholeCoverAbnormalRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::ManholeCoverAbnormalDetection_S &stInfo, bool bOutStruct);
    /* 睡岗识别相关 */
    void deal(Json::Object *pRootJson, Alarm::SleepOnDutyRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::SleepOnDutyDetection_S &stInfo, bool bOutStruct);
    /* 摔倒识别相关 */
    void deal(Json::Object *pRootJson, Alarm::TripRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::TripDetection_S &stInfo, bool bOutStruct);
    /* 玩手机识别相关 */
    void deal(Json::Object *pRootJson, Alarm::PhoneUsageRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::PhoneUsageDetection_S &stInfo, bool bOutStruct);
    /* 倒地识别相关 */
    void deal(Json::Object *pRootJson, Alarm::PersonFallDownRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::PersonFallDownDetection_S &stInfo, bool bOutStruct);
    /* 高空安全带检测相关 */
    void deal(Json::Object *pRootJson, Alarm::HighAltitudeSeatbeltRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::HighAltitudeSeatbeltDetection_S &stInfo, bool bOutStruct);
    /* 黄土裸露检测相关 */
    void deal(Json::Object *pRootJson, Alarm::BareSoilRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::BareSoiletDection_S &stInfo, bool bOutStruct);
     /* 安全帽检测相关 */
    void deal(Json::Object *pRootJson, Alarm::SafetyHelmetRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::SafetyHelmetDection_S &stInfo, bool bOutStruct);
     /* 洞口防护栏检测相关 */
    void deal(Json::Object *pRootJson, Alarm::HoleProtectionBarRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::HoleProtectionBarDection_S &stInfo, bool bOutStruct);
     /* 反光衣检测相关 */
    void deal(Json::Object *pRootJson, Alarm::ReflectiveClothingRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::ReflectiveClothingDection_S &stInfo, bool bOutStruct);
     /* 抽烟识别相关 */
    void deal(Json::Object *pRootJson, Alarm::SmokingRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::SmokingDection_S &stInfo, bool bOutStruct);
    /* 其他smart事件通用检测 */
    void deal(Json::Object *pRootJson, Alarm::OtherSmartRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::OtherSmartDection_S &stInfo, bool bOutStruct);
    /* 施工占道检测相关 */
    void deal(Json::Object *pRootJson, Alarm::ConstructionEncroachmentRoadRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::ConstructionEncroachmentRoadDetection_S &stInfo, bool bOutStruct);
    /* 电瓶车检测相关 */
    void deal(Json::Object *pRootJson, Alarm::ElectricScooterRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::ElectricScooterDetection_S &stInfo, bool bOutStruct);
    /* 车牌识别检测相关 */
    void deal(Json::Object *pRootJson, Alarm::LicensePlateCognitionRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::LicensePlateCognitionDetection_S &stInfo, bool bOutStruct);
    /* 逆行识别检测相关 */
    void deal(Json::Object *pRootJson, Alarm::DrivingAgainstTrafficRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<Alarm::DrivingAgainstTrafficRule_S> &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::DrivingAgainstTrafficDetection_S &stInfo, bool bOutStruct);
    /* 拥堵检测相关 */
    void deal(Json::Object *pRootJson, Alarm::CongestionRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::CongestionDetection_S &stInfo, bool bOutStruct);
    /* 违规变道检测相关 */
    void deal(Json::Object *pRootJson, Alarm::IllegalLaneChangeRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<Alarm::IllegalLaneChangeRule_S> &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::IllegalLaneChangeDetection_S &stInfo, bool bOutStruct);
    /* 应急车道占用侦测相关 */
    void deal(Json::Object *pRootJson, Alarm::EmergencyLaneOccupancyRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::EmergencyLaneOccupancyDetection_S &stInfo, bool bOutStruct);
    /* 非机动车闯入侦测相关 */ 
    void deal(Json::Object *pRootJson, Alarm::NonMotorVehicleIntrusionRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::NonMotorVehicleIntrusionDetection_S &stInfo, bool bOutStruct);

    /* 行人属性信息推送相关 */
    void deal(Json::Object *pRootJson, Alarm::PersonAlarmAttribute_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::PersonAlarmInfo_S &stInfo, bool bOutStruct);

    /* 机动车属性信息推送相关 */
    void deal(Json::Object *pRootJson, Alarm::MotorvehicleAlarmAttribute_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::MotorvehicleAlarmInfo_S &stInfo, bool bOutStruct);

    /* 非机动车属性信息推送相关 */
    void deal(Json::Object *pRootJson, Alarm::NonMotorvehicleAlarmAttribute_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::NonMotorvehicleAlarmInfo_S &stInfo, bool bOutStruct);

    /* 属性识别开关 */
    void deal(Json::Object *pRootJson, Alarm::AttributeDetectSwitch_S &stInfo, bool bOutStruct);

#endif

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
    /* 垃圾暴露检测相关 */
    void deal(Json::Object *pRootJson, Alarm::GarbageExposureRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::GarbageExposureDetection_S &stInfo, bool bOutStruct);
    /* 垃圾满溢检测相关 */
    void deal(Json::Object *pRootJson, Alarm::GarbageOverflowRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::GarbageOverflowDetection_S &stInfo, bool bOutStruct);
#endif

    /* 人数统计相关 */
#if CAP_AI_PEOPLE_STATISTICS
    void deal(Json::Object *pRootJson, Alarm::PeopleFlowRuleLine_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::PopulationAlarmRule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::PopulationAlarmConfig_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::StatisticsResetConfig_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::PeopleFlowStatistics_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Alarm::PeopleDensityDetection_S &stInfo, bool bOutStruct);
#endif

    /**
     * @brief  转换布防时间数据结构的日期信息
     * @param  [Object] *pRootJson
     * @param  [vector<std::vector<Common::SchedTime_S>>] &vecInfo
     * @param  [bool] bOutStruct
     * @return [*]
     * @author EasonLu
     * @note
     */
    void deal(Json::Object *pRootJson, std::vector<std::vector<Common::SchedTime_S>> &vecInfo, bool bOutStruct);

    /**
     * @brief  转换多边形的区域信息
     * @param  [Object] *pRootJson
     * @param  [Region_S] &stInfo
     * @param  [bool] bOutStruct
     * @return [*]
     * @author EasonLu
     * @note
     */
    void deal(Json::Object *pRootJson, Alarm::Region_S &stInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, Alarm::EventSchedule_S &stSchedule, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::set<Alarm::EventSchedule_S> &stSchedule, bool bOutStruct);
    void deal(Json::Object* pRootJson, Alarm::VideoLostDetection_S &stVideoLostDetection, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::set<Alarm::VideoLostDetection_S> &videoLostDetection, bool bOutStruct);
    
}; // namespace Convert
