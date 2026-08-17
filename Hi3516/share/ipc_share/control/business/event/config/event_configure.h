/**
 * @FilePath     : event_configure.h
* @Author       : zhangjc (zhangjc@kfb.cn)
* @Date         : 2024-12-14
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-20 14:38:35
* @Descripttion : 事件配置
*/

#pragma once

#include "alarm_define.h"
#include "event_define.h"
#include "Singleton.h"
#include "alarm_convert.h"
#include "event_convert.h"
#include "convert_interface.h"
#include "path_define.h"
#include "config_storage.h"


class CEventConfigure : public CSingleton<CEventConfigure>
{
    CEventConfigure();
public:
    ~CEventConfigure();    
    friend class CSingleton<CEventConfigure>;

    /* 联动 */
    template <typename T>
    int set_linkageList(const T &stInfo)
    {
        T stDbInfo;
        stDbInfo.nChnId = stInfo.nChnId;
        get_configure(stDbInfo);
        stDbInfo.stLinkageList = stInfo.stLinkageList;
        return set_configure(stDbInfo);
    }
    template <typename T>
    int get_linkageList(T &stInfo) const
    {
        T stDbInfo;
        stDbInfo.nChnId = stInfo.nChnId;
        int nRet = get_configure(stDbInfo);
        stInfo.stLinkageList = stDbInfo.stLinkageList;
        return nRet;
    }

    /* 算法配置 */
    int set_configure(const Event::AlgorithmConfig_S &alarm);
    int get_configure(Event::AlgorithmConfig_S &alarm) const;

    /* 所有算法事件计划 */
    int set_configure(const Alarm::EventSchedule_S &alarm);
    int get_configure(Alarm::EventSchedule_S &alarm) const;
    int get_configure(std::set<Alarm::EventSchedule_S> &alarm) const;

    /* 智能资源分配-智能事件启用情况 */
    int set_configure(const Event::SmartEventEnableStatus_S &alarm);
    int get_configure(Event::SmartEventEnableStatus_S &alarm) const;

    /* Metadata配置 */
    int set_configure(const Event::MetadataConfig_S &alarm);
    int get_configure(Event::MetadataConfig_S &alarm) const;

    /**
     * @brief   : 普通事件
     */
    /* 移动侦测配置 */
    int set_configure(const Alarm::MotionDetection_S &stMotionAlarm);
    int get_configure(Alarm::MotionDetection_S &stMotionAlarm) const;
    /* 遮挡报警配置 */
    int set_configure(const Alarm::HideAlarm_S &stHideAlarm);
    int get_configure(Alarm::HideAlarm_S &stHideAlarm) const;
    /* 异常报警配置 */
    int set_configure(const Alarm::AbnormalDetection_S &stAbnormalAlarm);
	int get_configure(Alarm::AbnormalDetection_S &stAbnormalAlarm) const;
	int get_configure(std::set<Alarm::AbnormalDetection_S> &stAbnormalAlarm) const;
    /* 声音报警输出配置 */
    int set_configure(const Alarm::SoundOutputAlarm_S &stSoundOutAlarm);
    int get_configure(Alarm::SoundOutputAlarm_S &stSoundOutAlarm) const;
    /* 报警输入配置 */
    int set_configure(const Alarm::IoInputInfo_S &stIoInputAlarm);
    int get_configure(Alarm::IoInputInfo_S &stIoInputAlarm) const;
    int get_configure(std::set<Alarm::IoInputInfo_S> &ioInputInfos) const;
    /* 报警输出配置 */
    int set_configure(const Alarm::IoOutputInfo_S &stIoOutputAlarm);
    int get_configure(Alarm::IoOutputInfo_S &stIoOutputAlarm) const;
    int get_configure(std::set<Alarm::IoOutputInfo_S> &ioOutputInfos) const;
    /* 闪光报警配置 */
    int set_configure(const Alarm::FlashInfo_S &stFlashAlarm);
    int get_configure(Alarm::FlashInfo_S &stFlashAlarm) const;
    /* 手动声光报警联动配置 */
    int set_configure(const Alarm::LinkageList_S &stManualSoundLightAlarm);
    int get_configure(Alarm::LinkageList_S &stManualSoundLightAlarm) const;
    /* Pir报警配置 */
    int set_configure(const Alarm::PirAlarmInfo_S &stPirAlarm);
    int get_configure(Alarm::PirAlarmInfo_S &stPirAlarm) const;

    /**
     * @brief   : 周界事件
     */

    /* 越界侦测配置 */
    int set_configure(const Alarm::BoundaryDetection_S &alarm);
    int get_configure(Alarm::BoundaryDetection_S &alarm) const;

    /* 区域入侵侦测配置 */
    int set_configure(const Alarm::FieldDetection_S &alarm);
    int get_configure(Alarm::FieldDetection_S &alarm) const;
    /* 区域进入侦测配置 */
    int set_configure(const Alarm::EntranceDetection_S &alarm);
    int get_configure(Alarm::EntranceDetection_S &alarm) const;
    /* 区域离开侦测配置 */
    int set_configure(const Alarm::ExitingDetection_S &alarm);
    int get_configure(Alarm::ExitingDetection_S &alarm) const;

    /**
     * @brief   : smart事件
     */
    /* 音频异常侦测配置 */
    int set_configure(const Alarm::AudioAnomaly_S &alarm);
    int get_configure(Alarm::AudioAnomaly_S &alarm) const;
    /* 场景变更侦测配置 */
    int set_configure(const Alarm::SceneChange_S &alarm);
    int get_configure(Alarm::SceneChange_S &alarm) const;
    /* 人脸侦测配置 */
    int set_configure(const Alarm::FaceDetection_S &alarm);
    int get_configure(Alarm::FaceDetection_S &alarm) const;
    /* 徘徊侦测配置 */
    int set_configure(const Alarm::LoiteringDetection_S &alarm);
    int get_configure(Alarm::LoiteringDetection_S &alarm) const;
    /* 人员聚集侦测配置 */
    int set_configure(const Alarm::CrowdGathering_S &alarm);
    int get_configure(Alarm::CrowdGathering_S &alarm) const;
    /* 停车侦测配置 */
    int set_configure(const Alarm::ParkingDetection_S &alarm);
    int get_configure(Alarm::ParkingDetection_S &alarm) const;
    /* 物品遗留侦测配置 */
    int set_configure(const Alarm::UnattendedObject_S &alarm);
    int get_configure(Alarm::UnattendedObject_S &alarm) const;
    /* 物品拿取侦测配置 */
    int set_configure(const Alarm::ObjectRemoval_S &alarm);
    int get_configure(Alarm::ObjectRemoval_S &alarm) const;
    /* 宠物识别配置 */
    int set_configure(const Alarm::PetRecognition_S &alarm);
    int get_configure(Alarm::PetRecognition_S &alarm) const;
    /* 人脸抓拍配置 */
    int set_configure(const Alarm::FaceCapture_S &alarm);
    int get_configure(Alarm::FaceCapture_S &alarm) const;
    /* 人脸抓拍叠加信息 */
    int set_configure(const Alarm::OverlayInfo_S &alarm);
    int get_configure(Alarm::OverlayInfo_S &alarm) const;

    /* 人脸比对配置 */
    int set_configure(const Alarm::FaceCompare_S &alarm);
    int get_configure(Alarm::FaceCompare_S &alarm) const;

#ifdef SCENE_INTELLIGENT_ANALYSIS
    /**
     *  @brief   :  场景智能分析事件
     */
    /* 智能AI场景智能分析控制信息 */
    int set_configure(const Alarm::LLMAISceneAnalysis_S &alarm);
    int get_configure(Alarm::LLMAISceneAnalysis_S &alarm) const;
    /* 智能AI画面分析信息 */
    int set_configure(const Alarm::LLMImageAnalysis_S &alarm);
    int get_configure(Alarm::LLMImageAnalysis_S &alarm) const;
    /* 智能AI画面分析记录信息 */
    int set_configure(const Alarm::AnalysisAllRecordIndexItem_S &alarm);
    int get_configure(Alarm::AnalysisAllRecordIndexItem_S &alarm) const;
    /* 文字预设任务 */
    int set_configure(const Alarm::TextPreset_S &alarm);
    int get_configure(Alarm::TextPreset_S &alarm) const;
    int set_configure(const Alarm::TextPresetTaskManager_S &alarm);
    int get_configure(Alarm::TextPresetTaskManager_S &alarm) const;
    /* 实时预警管理 */
    int set_configure(const Alarm::RealAlarmPushManager_S &alarm);
    int get_configure(Alarm::RealAlarmPushManager_S &alarm) const;
#endif

#ifdef SCENE_INTELLIGENCE
    /**
     *  @brief   :  场景智能
     */
    int set_configure(const Alarm::FenceClimbingDetection_S &alarm);
    int get_configure(Alarm::FenceClimbingDetection_S &alarm) const;

    int set_configure(const Alarm::LeavePostDetection_S &alarm);
    int get_configure(Alarm::LeavePostDetection_S &alarm) const;

    int set_configure(const Alarm::PedestrianIntrusionDetection_S &alarm);
    int get_configure(Alarm::PedestrianIntrusionDetection_S &alarm) const;

    int set_configure(const Alarm::SmokeFireDetection_S &alarm);
    int get_configure(Alarm::SmokeFireDetection_S &alarm) const;

    int set_configure(const Alarm::OpenFlameDetection_S &alarm);
    int get_configure(Alarm::OpenFlameDetection_S &alarm) const;

    int set_configure(const Alarm::RoadPondingDetection_S &alarm);
    int get_configure(Alarm::RoadPondingDetection_S &alarm) const;

    int set_configure(const Alarm::ManholeCoverAbnormalDetection_S &alarm);
    int get_configure(Alarm::ManholeCoverAbnormalDetection_S &alarm) const;

    int set_configure(const Alarm::SleepOnDutyDetection_S &alarm);
    int get_configure(Alarm::SleepOnDutyDetection_S &alarm) const;

    int set_configure(const Alarm::TripDetection_S &alarm);
    int get_configure(Alarm::TripDetection_S &alarm) const;

    int set_configure(const Alarm::PhoneUsageDetection_S &alarm);
    int get_configure(Alarm::PhoneUsageDetection_S &alarm) const;

    int set_configure(const Alarm::PersonFallDownDetection_S &alarm);
    int get_configure(Alarm::PersonFallDownDetection_S &alarm) const;

    int set_configure(const Alarm::HighAltitudeSeatbeltDetection_S &alarm);
    int get_configure(Alarm::HighAltitudeSeatbeltDetection_S &alarm) const;

    int set_configure(const Alarm::BareSoiletDection_S &alarm);
    int get_configure(Alarm::BareSoiletDection_S &alarm) const;

    int set_configure(const Alarm::SafetyHelmetDection_S &alarm);
    int get_configure(Alarm::SafetyHelmetDection_S &alarm) const;

    int set_configure(const Alarm::HoleProtectionBarDection_S &alarm);
    int get_configure(Alarm::HoleProtectionBarDection_S &alarm) const;

    int set_configure(const Alarm::ReflectiveClothingDection_S &alarm);
    int get_configure(Alarm::ReflectiveClothingDection_S &alarm) const;

    int set_configure(const Alarm::SmokingDection_S &alarm);
    int get_configure(Alarm::SmokingDection_S &alarm) const;
    
    int set_configure(const Alarm::ConstructionEncroachmentRoadDetection_S &alarm);
    int get_configure(Alarm::ConstructionEncroachmentRoadDetection_S &alarm) const;

    int set_configure(const Alarm::ElectricScooterDetection_S &alarm);
    int get_configure(Alarm::ElectricScooterDetection_S &alarm) const;
    
    int set_configure(const Alarm::LicensePlateCognitionDetection_S &alarm);
    int get_configure(Alarm::LicensePlateCognitionDetection_S &alarm) const;

    int set_configure(const Alarm::DrivingAgainstTrafficDetection_S &alarm);
    int get_configure(Alarm::DrivingAgainstTrafficDetection_S &alarm) const;
    
    int set_configure(const Alarm::IllegalLaneChangeDetection_S &alarm);
    int get_configure(Alarm::IllegalLaneChangeDetection_S &alarm) const;

    int set_configure(const Alarm::CongestionDetection_S &alarm);
    int get_configure(Alarm::CongestionDetection_S &alarm) const;

    int set_configure(const Alarm::EmergencyLaneOccupancyDetection_S &alarm);
    int get_configure(Alarm::EmergencyLaneOccupancyDetection_S &alarm) const;

    int set_configure(const Alarm::NonMotorVehicleIntrusionDetection_S &alarm);
    int get_configure(Alarm::NonMotorVehicleIntrusionDetection_S &alarm) const;

    int set_configure(const Alarm::AttributeDetectSwitch_S &alarm);
    int get_configure(Alarm::AttributeDetectSwitch_S &alarm) const;

#endif

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
    int set_configure(const Alarm::GarbageExposureDetection_S &alarm);
    int get_configure(Alarm::GarbageExposureDetection_S &alarm) const;
    int set_configure(const Alarm::GarbageOverflowDetection_S &alarm);
    int get_configure(Alarm::GarbageOverflowDetection_S &alarm) const;
#endif

#if CAP_AI_PEOPLE_STATISTICS
    /* 人流统计配置 */
    int set_configure(const Alarm::PeopleFlowStatistics_S &alarm);
    int get_configure(Alarm::PeopleFlowStatistics_S &alarm) const;
    /* 人员密度检测配置 */
    int set_configure(const Alarm::PeopleDensityDetection_S &alarm);
    int get_configure(Alarm::PeopleDensityDetection_S &alarm) const;
#endif

private:
    /* 算法配置 */
    ConfigStorage<Event::AlgorithmConfig_S, StorageType_E::Single> m_algorithmConfig;
    /* 所有算法事件计划 */
    ConfigStorage<Alarm::EventSchedule_S> m_eventSchedule;
    /* 智能资源分配-智能事件启用情况 */
    ConfigStorage<Event::SmartEventEnableStatus_S, StorageType_E::Single> m_smartEventEnableStatus;
    /* Metadata配置 */
    ConfigStorage<Event::MetadataConfig_S, StorageType_E::Single> m_metadataConfig;

    /**
     * @brief   : 普通事件
     */
    /* 移动侦测配置 */
    ConfigStorage<Alarm::MotionDetection_S, StorageType_E::Single> m_motionAlarm;
    /* 遮挡报警配置 */
    ConfigStorage<Alarm::HideAlarm_S, StorageType_E::Single> m_HideAlarm;   
    /* 异常报警配置 */
	ConfigStorage<Alarm::AbnormalDetection_S> m_AbnormalAlarm;
    /* 声音报警输出配置 */
    ConfigStorage<Alarm::SoundOutputAlarm_S, StorageType_E::Single> m_SoundOutAlarm; 
    /* 报警输入配置 */
    ConfigStorage<Alarm::IoInputInfo_S> m_IoInputAlarm; 
    /* 报警输出配置 */
    ConfigStorage<Alarm::IoOutputInfo_S> m_IoOutputAlarm; 
    /* 闪光报警配置 */
    ConfigStorage<Alarm::FlashInfo_S, StorageType_E::Single> m_FlashAlarm; 
    /* 手动声光报警联动配置 */
    ConfigStorage<Alarm::LinkageList_S, StorageType_E::Single> m_ManualSoundLightAlarm;
    /* Pir报警配置 */
    ConfigStorage<Alarm::PirAlarmInfo_S, StorageType_E::Single> m_PirAlarm; 
    

    /**
     * @brief   : 周界事件
     */

    /* 越界侦测配置 */
    ConfigStorage<Alarm::BoundaryDetection_S, StorageType_E::Single> m_BoundaryDetection;
    /* 区域入侵配置 */
    ConfigStorage<Alarm::FieldDetection_S, StorageType_E::Single> m_FieldDetection;
    /* 进入区域配置 */
    ConfigStorage<Alarm::EntranceDetection_S, StorageType_E::Single> m_EntranceDetection;
    /* 离开区域配置 */
    ConfigStorage<Alarm::ExitingDetection_S, StorageType_E::Single> m_ExitDetection;

    /**
     * @brief   : smart事件
     */
    /* 音频异常侦测配置 */
    ConfigStorage<Alarm::AudioAnomaly_S, StorageType_E::Single> m_audioAnomaly;
    /* 场景变更侦测配置 */
    ConfigStorage<Alarm::SceneChange_S, StorageType_E::Single> m_sceneChange;
    /* 人脸侦测配置 */
    ConfigStorage<Alarm::FaceDetection_S, StorageType_E::Single> m_faceDetection;
    /* 徘徊侦测配置  */
    ConfigStorage<Alarm::LoiteringDetection_S, StorageType_E::Single> m_loiteringDetection;
    /* 人员聚集侦测配置  */
    ConfigStorage<Alarm::CrowdGathering_S, StorageType_E::Single> m_crowdGathering;
    /* 停车侦测配置 */
    ConfigStorage<Alarm::ParkingDetection_S, StorageType_E::Single> m_parkingDetection;
    /* 物品遗留侦测配置 */
    ConfigStorage<Alarm::UnattendedObject_S, StorageType_E::Single> m_unattendedObject;
    /* 物品拿取侦测配置 */
    ConfigStorage<Alarm::ObjectRemoval_S, StorageType_E::Single> m_objectRemoval;
    /* 宠物识别配置 */
    ConfigStorage<Alarm::PetRecognition_S, StorageType_E::Single> m_petRecognition;
    /* 人脸抓拍配置 */
    ConfigStorage<Alarm::FaceCapture_S, StorageType_E::Single> m_faceCapture;
    /* 人脸比对配置 */
    ConfigStorage<Alarm::FaceCompare_S, StorageType_E::Single> m_faceCompare;
    /* 人脸抓拍叠加信息 */
    ConfigStorage<Alarm::OverlayInfo_S, StorageType_E::Single> m_overlayInfo;

#ifdef SCENE_INTELLIGENT_ANALYSIS
     /**
     * @brief   :  场景智能分析事件
     */
    /* 智能AI场景智能分析控制配置 */
    ConfigStorage<Alarm::LLMAISceneAnalysis_S, StorageType_E::Single> m_AiSceneAnalysis;
    /* 画面智能分析配置 */
    ConfigStorage<Alarm::LLMImageAnalysis_S, StorageType_E::Single> m_imageAnalysis;
     /* 画面智能分析记录 */
    ConfigStorage<Alarm::AnalysisAllRecordIndexItem_S, StorageType_E::Single> m_imageAnalysisRecord;

    /* 文字预设任务 */
    ConfigStorage<Alarm::TextPreset_S, StorageType_E::Single> m_TextPresetAnalysis;  
    ConfigStorage<Alarm::TextPresetTaskManager_S, StorageType_E::Single> m_TextPresetTaskManager;
    /* 实时预警管理 */
    ConfigStorage<Alarm::RealAlarmPushManager_S, StorageType_E::Single> m_RealPushAlarmManager;
#endif

#ifdef SCENE_INTELLIGENCE
    /**
     *  @brief   :  场景智能
     */
     /* 翻越围栏 */
    ConfigStorage<Alarm::FenceClimbingDetection_S, StorageType_E::Single> m_fenceClimbing;
    /* 离岗识别 */
    ConfigStorage<Alarm::LeavePostDetection_S, StorageType_E::Single> m_leavePost;
    /* 行人闯入 */
    ConfigStorage<Alarm::PedestrianIntrusionDetection_S, StorageType_E::Single> m_pedestrianIntrusion;
    /* 烟火识别 */
    ConfigStorage<Alarm::SmokeFireDetection_S, StorageType_E::Single> m_smokeFire;
    /* 明火识别 */
    ConfigStorage<Alarm::OpenFlameDetection_S, StorageType_E::Single> m_openFlame;
    /* 道路积水检测 */
    ConfigStorage<Alarm::RoadPondingDetection_S, StorageType_E::Single> m_roadPonding;
    /* 井盖异常检测 */
    ConfigStorage<Alarm::ManholeCoverAbnormalDetection_S, StorageType_E::Single> m_manholeCoverAbnormal;
    /* 睡岗识别 */
    ConfigStorage<Alarm::SleepOnDutyDetection_S, StorageType_E::Single> m_SleepOnDuty;
    /* 摔倒识别 */
    ConfigStorage<Alarm::TripDetection_S, StorageType_E::Single> m_trip;
    /* 倒地识别 */
    ConfigStorage<Alarm::PersonFallDownDetection_S, StorageType_E::Single> m_personFallDown;
    /* 玩手机识别 */
    ConfigStorage<Alarm::PhoneUsageDetection_S, StorageType_E::Single> m_phoneUsage;
    /* 高空安全带检测 */
    ConfigStorage<Alarm::HighAltitudeSeatbeltDetection_S, StorageType_E::Single> m_highAltitudeSeatbelt;
    /* 黄土裸露检测 */
    ConfigStorage<Alarm::BareSoiletDection_S, StorageType_E::Single> m_bareSoilet;
    /* 安全帽检测 */
    ConfigStorage<Alarm::SafetyHelmetDection_S, StorageType_E::Single> m_safetyHelmet;
    /* 洞口防护栏检测 */
    ConfigStorage<Alarm::HoleProtectionBarDection_S, StorageType_E::Single> m_holeProtectionBar;
    /* 反光衣检测 */
    ConfigStorage<Alarm::ReflectiveClothingDection_S, StorageType_E::Single> m_reflectiveClothing;
    /* 抽烟识别 */
    ConfigStorage<Alarm::SmokingDection_S, StorageType_E::Single> m_smoking;
    /* 施工占道检测 */
    ConfigStorage<Alarm::ConstructionEncroachmentRoadDetection_S, StorageType_E::Single> m_constructionEncroachmentRoad;
    /* 电瓶车检测 */
    ConfigStorage<Alarm::ElectricScooterDetection_S, StorageType_E::Single> m_ElectricScooter;
    /* 车牌识别检测 */
    ConfigStorage<Alarm::LicensePlateCognitionDetection_S, StorageType_E::Single> m_licensePlateCognition;
    /* 逆行识别检测 */
    ConfigStorage<Alarm::DrivingAgainstTrafficDetection_S, StorageType_E::Single> m_drivingAgainstTrafficDetection;
    /* 违规变道识别检测 */
    ConfigStorage<Alarm::IllegalLaneChangeDetection_S, StorageType_E::Single> m_illegalLaneChangeDetection;
    /* 拥堵识别检测 */
    ConfigStorage<Alarm::CongestionDetection_S, StorageType_E::Single> m_congestionDetection;
    /* 应急车道占用识别检测 */
    ConfigStorage<Alarm::EmergencyLaneOccupancyDetection_S, StorageType_E::Single> m_emergencyLaneOccupancyDetection;
    /* 非机动车闯入识别检测 */
    ConfigStorage<Alarm::NonMotorVehicleIntrusionDetection_S, StorageType_E::Single> m_nonMotorVehicleIntrusionDetection;
    
    /* 属性识别检测 */
    ConfigStorage<Alarm::AttributeDetectSwitch_S, StorageType_E::Single> m_AttributeDetectSwitch;

#endif

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
    /* 垃圾暴露检测 */
    ConfigStorage<Alarm::GarbageExposureDetection_S, StorageType_E::Single> m_garbageExposure;
    /* 垃圾满溢检测 */
    ConfigStorage<Alarm::GarbageOverflowDetection_S, StorageType_E::Single> m_garbageOverflow;
#endif

#if CAP_AI_PEOPLE_STATISTICS
    /* 人流统计配置 */
    ConfigStorage<Alarm::PeopleFlowStatistics_S, StorageType_E::Single> m_peopleFlowStatistics;
    /* 人员密度检测配置 */
    ConfigStorage<Alarm::PeopleDensityDetection_S, StorageType_E::Single> m_peopleDensityDetection;
#endif

};
