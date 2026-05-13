/**
 * @FilePath     : event_task.cpp
 * @Author       : huangjunda
 * @Date         : 2025-04-29 09:54:27
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-20 14:59:52
 * @Description  : 事件任务
 */

#include "event_task.h"
#include "convert_interface.h"
#include "path_define.h"
#include "event_define.h"
#include "event_manage.h"
#include "event_alarm.h"
#include "IpcRet.h"
#include "event_resource.h"
#include "event_configure.h"
#ifdef SCENE_INTELLIGENT_ANALYSIS
#include "event_vlm_manage.hpp"
#endif

#define ERR_EVENT_RESOURCE_CONFLICT -305
/**
 * @brief   : 辅助函数：将AlgorithmConfig转换为SmartEventEnableStatus用于资源检查
 */
static void helper_convert_to_status(const Event::AlgorithmConfig_S& algo, Event::SmartEventEnableStatus_S& status) {
    status.bLineCrossing = algo.nEnLineCrossing;
    status.bIntrusion = algo.nEnIntrusion;
    status.bEnterRegion = algo.nEnEnterRegion;
    status.bLeaveRegion = algo.nEnLeaveRegion;
    status.bLoiteringDetect = algo.nEnLoiteringDetect;
    status.bCrowdGathering = algo.nEnCrowdGathering;
    status.bParkingDetect = algo.nEnParkingDetect;
    status.bAudioAnomaly = algo.nEnAudioAnomaly;
    status.bSceneChange = algo.nEnSceneChange;
    status.bUnattendedObject = algo.nEnUnattendedObject;
    status.bObjectRemoval = algo.nEnObjectRemoval;
    status.bFaceDetect = algo.nEnFaceDetect;
    status.bPetRecognition = algo.nEnPetRecognition;
    status.bFaceCapture = algo.nEnFaceCapture;
    status.bFaceCompare = algo.nEnFaceCompare;
    #ifdef SCENE_INTELLIGENCE
    status.bSleepOnDuty = algo.nEnSleepOnDuty;
    status.bLeavePost = algo.nEnLeavePost;
    status.bElectricVehicleInElevator = algo.nEnElectricVehicleInElevator;
    status.bPersonFallDown = algo.nEnPersonFallDown;
    status.bFenceClimbing = algo.nEnFenceClimbing;
    status.bTrip = algo.nEnTrip;
    status.bSmoking = algo.nEnSmoking;
    status.bPhoneUsage = algo.nEnPhoneUsage;
    status.bSmokeFire = algo.nEnSmokeFire;
    status.bOpenFlame = algo.nEnOpenFlame;
    status.bManholeCoverAbnormal = algo.nEnManholeCoverAbnormal;
    status.bBareSoil = algo.nEnBareSoil;
    status.bHoleProtectionBar = algo.nEnHoleProtectionBar;
    status.bPedestrianIntrusion = algo.nEnPedestrianIntrusion;
    status.bSafetyHelmet = algo.nEnSafetyHelmet;
    status.bReflectiveClothing = algo.nEnReflectiveClothing;
    status.bHighAltitudeSeatbelt = algo.nEnHighAltitudeSeatbelt;
    status.bConstructionOccupyRoad = algo.nEnConstructionOccupyRoad;
    status.bEmergencyLaneOccupancy = algo.nEnEmergencyLaneOccupancy;
    status.bReverseDirection = algo.nEnReverseDirection;
    status.bNonMotorVehicleIntrusion = algo.nEnNonMotorVehicleIntrusion;
    status.bRoadPonding = algo.nEnRoadPonding;
    status.bCongestion = algo.nEnCongestion;
    status.bIllegalParking = algo.nEnIllegalParking;
    status.bIllegalLaneChange = algo.nEnIllegalLaneChange;
    status.bPlateNumber = algo.nPlateNumber;
    #endif

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
    status.bGarbageExposure = algo.nEnGarbageExposure;
    status.bGarbageOverflow = algo.nEnGarbageOverflow;
#endif

#if CAP_AI_PEOPLE_STATISTICS
    status.bPeopleFlowStatistics = algo.nEnPeopleFlowStatistics;
    status.bPeopleDensityDetection = algo.nEnPeopleDensityDetection;
#endif
}

/**
 * @brief   : 获取事件类型名称（用于调试打印）
 */
static const char* get_event_type_name(Event::Type_E type) {
    switch (type) {
        case Event::Type::LINE_CROSSING: return "LINE_CROSSING";
        case Event::Type::INTRUSION: return "INTRUSION";
        case Event::Type::ENTER_REGION: return "ENTER_REGION";
        case Event::Type::LEAVE_REGION: return "LEAVE_REGION";
        case Event::Type::LOITERING_DETECT: return "LOITERING_DETECT";
        case Event::Type::CROWD_GATHERING: return "CROWD_GATHERING";
        case Event::Type::PARKING_DETECT: return "PARKING_DETECT";
        case Event::Type::AUDIO_ANOMALY: return "AUDIO_ANOMALY";
        case Event::Type::SCENE_CHANGE: return "SCENE_CHANGE";
        case Event::Type::UNATTENDED_OBJECT: return "UNATTENDED_OBJECT";
        case Event::Type::OBJECT_REMOVAL: return "OBJECT_REMOVAL";
        case Event::Type::FACE_DETECT: return "FACE_DETECT";
        case Event::Type::PET_RECOGNITION: return "PET_RECOGNITION";
        case Event::Type::FACE_LIB: return "FACE_LIB";
        case Event::Type::FACE_CAPTURE: return "FACE_CAPTURE";

#if CAP_AI_PEOPLE_STATISTICS
        case Event::Type::PEOPLE_FLOW_STATISTICS: return "PEOPLE_FLOW_STATISTICS";
        case Event::Type::PEOPLE_DENSITY_DETECTION: return "PEOPLE_DENSITY_DETECTION";
#endif

        case Event::Type::FACE_COMPARE: return "FACE_COMPARE";

        default: return "UNKNOWN";
    }
}

#if CAP_AI_PEOPLE_STATISTICS
/**
 * @brief   : 检查三级人数告警阈值配置
 * @param    {PopulationAlarmConfig_S} &stAlarmCfg：三级人数告警配置
 * @return   {bool} true：合法 false：非法
 */
static bool check_population_alarm_thresholds(const Alarm::PopulationAlarmConfig_S &stAlarmCfg)
{
    if (stAlarmCfg.stNormal.nThreshold == 0 ||
        stAlarmCfg.stMedium.nThreshold == 0 ||
        stAlarmCfg.stSevere.nThreshold == 0)
    {
        return false;
    }

    if (stAlarmCfg.stNormal.nThreshold > stAlarmCfg.stMedium.nThreshold ||
        stAlarmCfg.stMedium.nThreshold > stAlarmCfg.stSevere.nThreshold)
    {
        return false;
    }

    return true;
}
#endif

/**
 * @brief   : 检查智能事件资源冲突
 * @param    {Event::Type_E} enable_type 要启用的事件类型
 * @param    {bool} bEnable 是否启用
 * @return   {int} 0：成功 非0：失败（资源冲突）
 */
static int check_analytics_resource(const Event::Type_E enable_type, const bool bEnable) {
    dlog_debug("[check_analytics_resource] 开始检查事件类型: %s (%d), 启用状态: %s", 
               get_event_type_name(enable_type), (int)enable_type, bEnable ? "启用" : "禁用");

    // 使用与 GetSmartEventEnableStatus 相同的方式获取智能事件启用状态
    Event::SmartEventEnableStatus_S oldStatus;
    CEventConfigure::instance()->get_configure(oldStatus);
    
    dlog_debug("[check_analytics_resource] 当前智能事件启用状态 - 越界侦测:%d, 区域入侵:%d, 进入区域:%d, 离开区域:%d, "
               "徘徊侦测:%d, 人员聚集:%d, 停车侦测:%d, 音频异常:%d, 场景变更:%d, "
               "物品遗留:%d, 物品拿取:%d, 人脸侦测:%d, 宠物识别:%d, 人脸抓拍:%d,人脸比对:%d",
               oldStatus.bLineCrossing, oldStatus.bIntrusion, oldStatus.bEnterRegion, 
               oldStatus.bLeaveRegion, oldStatus.bLoiteringDetect, oldStatus.bCrowdGathering,
               oldStatus.bParkingDetect, oldStatus.bAudioAnomaly, oldStatus.bSceneChange,
               oldStatus.bUnattendedObject, oldStatus.bObjectRemoval, oldStatus.bFaceDetect,
               oldStatus.bPetRecognition, oldStatus.bFaceCapture,oldStatus.bFaceCompare);
    
    // 检查是否已经处于目标状态
    bool already_in_target_state = false;
    switch (enable_type) {
        case Event::Type::LINE_CROSSING: already_in_target_state = (oldStatus.bLineCrossing == bEnable); break;
        case Event::Type::INTRUSION: already_in_target_state = (oldStatus.bIntrusion == bEnable); break;
        case Event::Type::ENTER_REGION: already_in_target_state = (oldStatus.bEnterRegion == bEnable); break;
        case Event::Type::LEAVE_REGION: already_in_target_state = (oldStatus.bLeaveRegion == bEnable); break;
        case Event::Type::LOITERING_DETECT: already_in_target_state = (oldStatus.bLoiteringDetect == bEnable); break;
        case Event::Type::CROWD_GATHERING: already_in_target_state = (oldStatus.bCrowdGathering == bEnable); break;
        case Event::Type::PARKING_DETECT: already_in_target_state = (oldStatus.bParkingDetect == bEnable); break;
        case Event::Type::AUDIO_ANOMALY: already_in_target_state = (oldStatus.bAudioAnomaly == bEnable); break;
        case Event::Type::SCENE_CHANGE: already_in_target_state = (oldStatus.bSceneChange == bEnable); break;
        case Event::Type::UNATTENDED_OBJECT: already_in_target_state = (oldStatus.bUnattendedObject == bEnable); break;
        case Event::Type::OBJECT_REMOVAL: already_in_target_state = (oldStatus.bObjectRemoval == bEnable); break;
        case Event::Type::FACE_DETECT: already_in_target_state = (oldStatus.bFaceDetect == bEnable); break;
        case Event::Type::PET_RECOGNITION: already_in_target_state = (oldStatus.bPetRecognition == bEnable); break;
        case Event::Type::FACE_LIB: already_in_target_state = (oldStatus.bFaceLib == bEnable); break;
        case Event::Type::FACE_CAPTURE: already_in_target_state = (oldStatus.bFaceCapture == bEnable); break;
#if CAP_AI_PEOPLE_STATISTICS
        case Event::Type::PEOPLE_FLOW_STATISTICS: already_in_target_state = (oldStatus.bPeopleFlowStatistics == bEnable); break;
        case Event::Type::PEOPLE_DENSITY_DETECTION: already_in_target_state = (oldStatus.bPeopleDensityDetection == bEnable); break;
#endif

        case Event::Type::FACE_COMPARE: already_in_target_state = (oldStatus.bFaceCompare == bEnable); break;

        default: break; 
    }
    
    if (already_in_target_state) {
        dlog_debug("[check_analytics_resource] 事件 %s 已经处于目标状态 (%s)，无需更新", 
                   get_event_type_name(enable_type), bEnable ? "启用" : "禁用");
        return 0;
    }

    // 如果是启用事件，需要检查资源冲突
    if (bEnable) {
        // 使用与 GetSmartEventEnableStatus 相同的方式获取可启用事件列表
        std::vector<Event::Type_E> aCanEnableEvent;
        int ret = CEventResource::instance()->get_canEventResource_rules(oldStatus, aCanEnableEvent);
        
        dlog_debug("[check_analytics_resource] 资源规则查询返回: %d, 可启用事件数量: %zu", ret, aCanEnableEvent.size());
        if (aCanEnableEvent.size() > 0) {
            dlog_debug("[check_analytics_resource] 可启用的事件列表:");
            for (size_t i = 0; i < aCanEnableEvent.size(); i++) {
                dlog_debug("[check_analytics_resource]   [%zu] %s (%d)", i, get_event_type_name(aCanEnableEvent[i]), (int)aCanEnableEvent[i]);
            }
        }
        
        bool can_enable = false;
        for (const auto& ev : aCanEnableEvent) {
            if (ev == enable_type) {
                can_enable = true;
                break;
            }
        }
        
        if (!can_enable) {
            dlog_error("[check_analytics_resource] 资源冲突！事件类型: %s (%d) 无法启用，当前已启用的事件与目标事件存在资源冲突", 
                       get_event_type_name(enable_type), (int)enable_type);
            return ERR_EVENT_RESOURCE_CONFLICT; // 返回参数错误，表示资源冲突
        }
        
        dlog_debug("[check_analytics_resource] 事件 %s 在可启用列表中，资源检查通过", get_event_type_name(enable_type));
    } else {
        dlog_debug("[check_analytics_resource] 禁用事件，直接允许");
    }

    // 创建新状态并更新对应事件的状态
    Event::SmartEventEnableStatus_S newStatus = oldStatus;
    switch (enable_type) {
        case Event::Type::LINE_CROSSING: newStatus.bLineCrossing = bEnable; break;
        case Event::Type::INTRUSION: newStatus.bIntrusion = bEnable; break;
        case Event::Type::ENTER_REGION: newStatus.bEnterRegion = bEnable; break;
        case Event::Type::LEAVE_REGION: newStatus.bLeaveRegion = bEnable; break;
        case Event::Type::LOITERING_DETECT: newStatus.bLoiteringDetect = bEnable; break;
        case Event::Type::CROWD_GATHERING: newStatus.bCrowdGathering = bEnable; break;
        case Event::Type::PARKING_DETECT: newStatus.bParkingDetect = bEnable; break;
        case Event::Type::AUDIO_ANOMALY: newStatus.bAudioAnomaly = bEnable; break;
        case Event::Type::SCENE_CHANGE: newStatus.bSceneChange = bEnable; break;
        case Event::Type::UNATTENDED_OBJECT: newStatus.bUnattendedObject = bEnable; break;
        case Event::Type::OBJECT_REMOVAL: newStatus.bObjectRemoval = bEnable; break;
        case Event::Type::FACE_DETECT: newStatus.bFaceDetect = bEnable; break;
        case Event::Type::PET_RECOGNITION: newStatus.bPetRecognition = bEnable; break;
        case Event::Type::FACE_LIB: newStatus.bFaceLib = bEnable; break;
        case Event::Type::FACE_CAPTURE: newStatus.bFaceCapture = bEnable; break;

#if CAP_AI_PEOPLE_STATISTICS
        case Event::Type::PEOPLE_FLOW_STATISTICS: newStatus.bPeopleFlowStatistics = bEnable; break;
        case Event::Type::PEOPLE_DENSITY_DETECTION: newStatus.bPeopleDensityDetection = bEnable; break;
#endif

        case Event::Type::FACE_COMPARE: newStatus.bFaceCompare = bEnable; break;
        default: break; 
    }

    // 保存新的智能事件启用状态配置
    int ret = CEventConfigure::instance()->set_configure(newStatus);
    if (ret != 0) {
        dlog_error("[check_analytics_resource] 保存智能事件启用状态失败，错误码: %d", ret);
        return ret;
    }
    
    dlog_debug("[check_analytics_resource] 已同步更新事件资源，事件 %s 状态设置为: %s", 
               get_event_type_name(enable_type), bEnable ? "启用" : "禁用");

    // 检查是否有事件被禁用，并更新其具体配置（参考 SetSmartEventEnableStatus 的逻辑）
    if (!bEnable) {
        CEventResource::instance()->update_event_configurations_on_disable(oldStatus, newStatus);
        dlog_debug("[check_analytics_resource] 已调用 update_event_configurations_on_disable 更新禁用事件的配置");
    }

    return 0;
}


/**
 * @brief   : 普通事件
 */
/* 获取移动侦测信息 */
void Task::Event::GetMotionDetectionInfo::handle()
{
    Alarm::MotionDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置移动侦测信息 */
void Task::Event::SetMotionDetectionInfo::handle()
{
    Alarm::MotionDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 参数有效性判断 */
    if (stInfo.stMotionNormalMode.nSensitivity < 0 || stInfo.stMotionNormalMode.nSensitivity > 100)
    {
        dlog_error("设置移动侦测信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }

    /* 普通模式区域参数有效性判断 */
    auto &grid = std::get<Alarm::MotionNormalMode_S::AreaGrid>(stInfo.stMotionNormalMode.varRegion);
    if (grid.empty())
    {
        dlog_error("设置移动侦测信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
    else
    {
        for (auto &AreaGrid : grid)
        {
            if (AreaGrid.empty())
            {
                dlog_error("设置移动侦测信息参数错误");
                result(ERR_WEB_PARAM);
                return;
            }
        }
    }

    for (auto &region : stInfo.stMotionExpertMode.vstMotionRegion)
    {
        if (region.nAreaNo < 1 || region.nAreaNo > MOTION_EXPERT_AREA_MAX ||
            region.nCloseSensitivity < 1 || region.nCloseSensitivity > 100 ||
            region.nDaytimeSensitivity < 1 || region.nDaytimeSensitivity > 100 ||
            region.nNightSensitivity < 1 || region.nNightSensitivity > 100)
        {
            dlog_error("设置移动侦测信息参数错误");
            result(ERR_WEB_PARAM);
            return;
        }
        /* 坐标有效性判断 */
        if (!region.stRect.IsValid())
        {
            dlog_error("设置移动侦测信息区域绘制异常");
            result(ERR_WEB_REGION);
            return;
        }
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::MOTION_DETECT;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

/* 获取遮挡报警参数 */
void Task::Event::GetHideAlarmInfo::handle()
{
    Alarm::HideAlarm_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置遮挡报警参数 */
void Task::Event::SetHideAlarmInfo::handle()
{
    Alarm::HideAlarm_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 参数有效性判断 */
    if (stInfo.nSensitivity < 0 || stInfo.nSensitivity > 3)
    {
        dlog_error("设置遮挡报警参数错误");
        result(ERR_WEB_PARAM);
    }
    /* 坐标有效性判断 */
    if (!stInfo.stRect.IsValid())
    {
        dlog_error("设置遮挡报警参数的区域绘制异常");
        result(ERR_WEB_REGION);
        return;
    }

    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::OCCLUSION_DETECT;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

/* 获取异常报警参数 */
void Task::Event::GetAbnormalAlarmInfo::handle()
{
	Alarm::AbnormalDetection_S stInfo;
	Convert::to_struct(m_taskData, stInfo);
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置异常报警参数 */
void Task::Event::SetAbnormalAlarmInfo::handle()
{
    ::Alarm::AbnormalDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    int nRet = CEventConfigure::instance()->set_configure(stInfo);
    static CEventManage *detector = CEventManage::instance();

    if(!stInfo.stLinkageList.tradition.empty() || !stInfo.stLinkageList.alarmOutput.empty()) 
    {
        stInfo.bEnable = true;
    }
    else
    {
        stInfo.bEnable = false;
    }
    
    if(0 == nRet) 
    {
        if(stInfo.bEnable)
        {
            dlog_debug("start Detction ");
            //启动或更新异常检测线程 
            detector->startDetection(stInfo.enAbnormalType); 
        }else{
            //停止异常类型的检测线程
            detector->stopDetection(stInfo.enAbnormalType); 
        }
    }
    result(nRet);
}

/* 获取声音报警参数 */
void Task::Event::GetAudioAlarmInfo::handle()
{
    Alarm::SoundOutputAlarm_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));

}

/* 设置声音报警参数 */
void Task::Event::SetAudioAlarmInfo::handle()
{
    Alarm::SoundOutputAlarm_S stInfo;
    Convert::to_struct(m_taskData, stInfo);

    /* 参数有效性判断 */
    if (stInfo.nTimes < 1 || stInfo.nTimes > 50)
    {
        dlog_error("设置声音报警信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::AUDIO_ALARM;
    stEventSchedule.bStatus = true;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

/* 编辑声音报警自定义音频信息 */
void Task::Event::EditAudioAlarmCustomInfo::handle()
{
    Alarm::CustomOperation_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    int nRet = CEventAlarm::instance()->edit_audioAlarmCustom_info(stInfo);
    result(nRet);
}

/* 获取声音报警自定义音频信息 */
void Task::Event::GetAudioAlarmCustomInfo::handle()
{
    std::vector<Alarm::CustomAudio_S> customAudioList;
    int nRet = CEventAlarm::instance()->get_audioAlarmCustom_info(customAudioList);
    if (nRet == 0)
    {
        result(Convert::to_string(customAudioList));
    }
    else
    {
        result(nRet);
    }
}

/* 设置声音报警自定义音频信息 */
void Task::Event::SetAudioAlarmCustomInfo::handle()
{
    Alarm::CustomOperation_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    int nRet = CEventAlarm::instance()->set_audioAlarmCustom_info(stInfo);
    result(nRet);
}

/* 获取报警输入参数 */
void Task::Event::GetIoInputInfo::handle()
{
    std::set<::Alarm::IoInputInfo_S> stInfo;
    int nRet = CEventAlarm::instance()->get_alarm(stInfo);
    if (nRet < 0)
    {
        result(nRet);
        return;
    }
    result(Convert::to_string(stInfo));
}

/* 设置报警输入参数 */
void Task::Event::SetIoInputInfo::handle()
{
    ::Alarm::IoInputInfo_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    int nRet = CEventAlarm::instance()->set_alarm(stInfo);
    if (nRet != OK)
    {
        result(nRet);
        return;
    }

    /* 判断是否启用：如果状态为ON或有布防时间设置，则认为启用 */
    bool bEnabled = (stInfo.nDealType == 1) || 
    std::any_of(stInfo.aAlarmTime.begin(), stInfo.aAlarmTime.end(),
                [](const std::vector<Common::SchedTime_S>& daySchedule) {
                    return !daySchedule.empty();
                });
    
    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::ALARM_INPUT;
    stEventSchedule.bStatus = bEnabled;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

/* 获取报警输出参数 */
void Task::Event::GetIoOutputInfo::handle()
{
    std::set<::Alarm::IoOutputInfo_S> stInfo;
    int nRet = CEventAlarm::instance()->get_alarm(stInfo);
    if (nRet < 0)
    {
        result(nRet);
        return;
    }
    result(Convert::to_string(stInfo));
}

/* 设置报警输出参数 */
void Task::Event::SetIoOutputInfo::handle()
{
    ::Alarm::IoOutputInfo_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    int nRet = CEventAlarm::instance()->set_alarm(stInfo);
    if (nRet < 0)
    {
        result(nRet);
        return;
    }

    /* 判断是否启用：如果状态为ON或有布防时间设置，则认为启用 */
    bool bEnabled = (stInfo.enState == Alarm::IoOutputState_E::ON) ||
                    std::any_of(stInfo.aAlarmTime.begin(),
                                stInfo.aAlarmTime.end(),
                                [](const std::vector<Common::SchedTime_S>& daySchedule)
                                {
                                    return !daySchedule.empty();
                                });

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::ALARM_OUTPUT;
    stEventSchedule.bStatus = bEnabled;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

/* 获取闪光报警参数 */
void Task::Event::GetFlashAlarmInfo::handle()
{
    Alarm::FlashInfo_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
 
}

/* 设置闪光报警参数 */
void Task::Event::SetFlashAlarmInfo::handle()
{
    Alarm::FlashInfo_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 参数有效性判断 */
    if (stInfo.nFlashTime < 1 || stInfo.nFlashTime > 300 ||
        stInfo.enFalshFrequency < Alarm::FlashFrequency_E::FLASH_STEADY_ON ||
        stInfo.enFalshFrequency > Alarm::FlashFrequency_E::FLASH_HIGH_FREQ)
    {
        dlog_error("设置闪光报警参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::FLASH_ALARM;
    stEventSchedule.bStatus = true;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
  
}

/* 获取PIR参数 */
void Task::Event::GetPirAlarmInfo::handle()
{
    Alarm::PirAlarmInfo_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
 
}

/* 设置PIR参数 */
void Task::Event::SetPirAlarmInfo::handle()
{
    Alarm::PirAlarmInfo_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::PIR_ALARM;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

 /**
 * @brief   : 周界事件
 */

 /* 获取越界侦测参数 */
void Task::Event::GetBoundaryDetectionInfo::handle()
{
    Alarm::BoundaryDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));

}

/* 设置越界侦测参数 */
void Task::Event::SetBoundaryDetectionInfo::handle()
{
    Alarm::BoundaryDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 检查智能事件资源冲突 */
    int ret = check_analytics_resource(::Event::Type::LINE_CROSSING, stInfo.bEnable);
    if (ret != 0) 
    {
        result(ret);
        return;
    }
    /* 参数有效性判断 */
    for (auto &rule : stInfo.aRule)
    {
        /* 坐标有效性判断 */
        if (!rule.stStartPos.IsValid())
        {
            dlog_error("设置越界侦测参数的区域绘制异常");
            result(ERR_WEB_REGION);
            return;
        }
        /* 判断每个区域的起始点和终点是否反了，我们规定，左边的点都是起始点,如果横坐标相同，上方的点为起始点 */
        bool bSwap = false;
        if (rule.stStartPos.fX == rule.stEndPos.fX)
        {
            if (rule.stStartPos.fY > rule.stEndPos.fY)
            {
                bSwap = true;
            }
        }
        else if (rule.stStartPos.fX > rule.stEndPos.fX)
        {
            bSwap = true;
        }
        if (bSwap)
        {
            /* 对调点坐标 */
            Common::PosF_S stTmpPos = rule.stStartPos;
            rule.stStartPos = rule.stEndPos;
            rule.stEndPos = stTmpPos;
        }
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::LINE_CROSSING;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

 /* 获取区域入侵参数 */
void Task::Event::GetFieldDetectionInfo::handle()
{
    Alarm::FieldDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
 
}

/* 设置区域入侵参数 */
void Task::Event::SetFieldDetectionInfo::handle()
{
    Alarm::FieldDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 检查智能事件资源冲突 */
    int ret = check_analytics_resource(::Event::Type::INTRUSION, stInfo.bEnable);
    if (ret != 0) 
    {
        result(ret);
        return;
    }
    /* 参数有效性判断 */
    for (auto &rule : stInfo.aRule)
    {
        if (rule.nTimeThreshold < 0 || rule.nTimeThreshold > 100 || rule.nSensitivity < 1 || rule.nSensitivity > 100)
        {
            dlog_error("设置区域入侵参数错误");
            result(ERR_WEB_PARAM);
            return;
        }
        /* 坐标有效性判断 */
        if (!rule.stRegion.IsValid())
        {
            dlog_error("设置区域入侵参数的区域绘制异常");
            result(ERR_WEB_REGION);
            return;
        }
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::INTRUSION;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

/* 获取进入区域参数 */
void Task::Event::GetEnterRegionDetectInfo::handle()
{
    Alarm::EntranceDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置进入区域参数 */
void Task::Event::SetEnterRegionDetectInfo::handle()
{
    Alarm::EntranceDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 检查智能事件资源冲突 */
    int ret = check_analytics_resource(::Event::Type::ENTER_REGION, stInfo.bEnable);
    if (ret != 0) 
    {
        result(ret);
        return;
    }
    /* 参数有效性判断 */
    for (auto &rule : stInfo.aRule)
    {
        if (rule.nSensitivity < 1 || rule.nSensitivity > 100)
        {
            dlog_error("设置进入区域参数错误");
            result(ERR_WEB_PARAM);
            return;
        }
        /* 坐标有效性判断 */
        if (!rule.stRegion.IsValid())
        {
            dlog_error("设置进入区域参数的区域绘制异常");
            result(ERR_WEB_REGION);
            return;
        }
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::ENTER_REGION;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

/* 获取离开区域参数 */
void Task::Event::GetLeaveRegionDetectInfo::handle()
{
    Alarm::ExitingDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置离开区域参数 */
void Task::Event::SetLeaveRegionDetectInfo::handle()
{
    Alarm::ExitingDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 检查智能事件资源冲突 */
    int ret = check_analytics_resource(::Event::Type::LEAVE_REGION, stInfo.bEnable);
    if (ret != 0) 
    {
        result(ret);
        return;
    }
    /* 参数有效性判断 */
    for (auto &rule : stInfo.aRule)
    {
        if (rule.nSensitivity < 1 || rule.nSensitivity > 100)
        {
            dlog_error("设置离开区域参数错误");
            result(ERR_WEB_PARAM);
            return;
        }
        /* 坐标有效性判断 */
        if (!rule.stRegion.IsValid())
        {
            dlog_error("设置离开区域参数的区域绘制异常");
            result(ERR_WEB_REGION);
            return;
        }
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::LEAVE_REGION;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

/**
 * @brief   : smart事件
 */

/* 获取音频异常侦测信息 */
void Task::Event::GetAudioAnomalyInfo::handle()
{
    Alarm::AudioAnomaly_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置音频异常侦测信息 */
void Task::Event::SetAudioAnomalyInfo::handle()
{
    Alarm::AudioAnomaly_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 检查智能事件资源冲突 */
    int ret = check_analytics_resource(::Event::Type::AUDIO_ANOMALY, stInfo.bEnable);
    if (ret != 0) {
        result(ret);
        return;
    }
    /* 参数有效性判断 */
    if (stInfo.nUpSensitivity < 1 || stInfo.nUpSensitivity > 100 || stInfo.nUpThreshold < 1 ||
        stInfo.nUpThreshold > 100 || stInfo.nDownSensitivity < 1 || stInfo.nDownSensitivity > 100)
    {
        dlog_error("设置音频异常侦测信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::AUDIO_ANOMALY;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

/* 获取音频异常侦测实时音量 */
void Task::Event::GetAudioAnomalyCurrentDb::handle()
{
    float fCurrentDb = 0.0f;
    CEventManage::instance()->send_algo_controlData(AC_GET_AUDIO_ANOMALY_DETECT_CURRENT_DB,
                                                    m_taskData.c_str(),
                                                    &fCurrentDb);
    Json::Object *pJsonData = Json::init();
    if (pJsonData)
    {
        Json::add(pJsonData, "CurrentDb", fCurrentDb);
    }
    auto data = Json::to_string(pJsonData);
    Json::deinit(pJsonData);
    result(data);
}

/* 获取场景变更侦测信息 */
void Task::Event::GetSceneChangeInfo::handle()
{
    Alarm::SceneChange_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置场景变更侦测信息 */
void Task::Event::SetSceneChangeInfo::handle()
{
    Alarm::SceneChange_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 检查智能事件资源冲突 */
    int ret = check_analytics_resource(::Event::Type::SCENE_CHANGE, stInfo.bEnable);
    if (ret != 0) {
        result(ret);
        return;
    }
    /* 参数有效性判断 */
    if (stInfo.nSensitivity < 1 || stInfo.nSensitivity > 100)
    {
        dlog_error("设置场景变更侦测信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::SCENE_CHANGE;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

/* 获取人脸侦测信息 */
void Task::Event::GetFaceDetectionInfo::handle()
{
    Alarm::FaceDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置人脸侦测信息 */
void Task::Event::SetFaceDetectionInfo::handle()
{
    Alarm::FaceDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 检查智能事件资源冲突 */
    int ret = check_analytics_resource(::Event::Type::FACE_DETECT, stInfo.bEnable);
    if (ret != 0) {
        result(ret);
        return;
    }
    /* 参数有效性判断 */
    if (stInfo.nSensitivity < 1 || stInfo.nSensitivity > 100)
    {
        dlog_error("设置人脸侦测信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
    /* 坐标有效性判断 */
    if (!stInfo.stRegion.IsValid())
    {
        dlog_error("设置人脸侦测信息的区域绘制异常");
        result(ERR_WEB_REGION);
        return;
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::FACE_DETECT;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

/* 获取徘徊侦测信息 */
void Task::Event::GetLoiteringDetectionInfo::handle()
{
    Alarm::LoiteringDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置徘徊侦测信息 */
void Task::Event::SetLoiteringDetectionInfo::handle()
{
    Alarm::LoiteringDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 检查智能事件资源冲突 */
    int ret = check_analytics_resource(::Event::Type::LOITERING_DETECT, stInfo.bEnable);
    if (ret != 0) {
        result(ret);
        return;
    }
    for (auto &rule : stInfo.aRule)
    {
        /* 参数有效性判断 */
        if (rule.nTimeThreshold < 0 || rule.nTimeThreshold > 100 || rule.nSensitivity < 1 || rule.nSensitivity > 100)
        {
            dlog_error("设置徘徊侦测信息参数错误");
            result(ERR_WEB_PARAM);
            return;
        }
        /* 坐标有效性判断 */
        if (!rule.stRegion.IsValid())
        {
            dlog_error("设置徘徊侦测信息的区域绘制异常");
            result(ERR_WEB_REGION);
            return;
        }
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::LOITERING_DETECT;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

/* 获取人员聚集侦测信息 */
void Task::Event::GetCrowdGatheringInfo::handle()
{
    Alarm::CrowdGathering_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置人员聚集侦测信息 */
void Task::Event::SetCrowdGatheringInfo::handle()
{
    Alarm::CrowdGathering_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 检查智能事件资源冲突 */
    int ret = check_analytics_resource(::Event::Type::CROWD_GATHERING, stInfo.bEnable);
    if (ret != 0) {
        result(ret);
        return;
    }
    for (auto &rule : stInfo.aRule)
    {
        /* 参数有效性判断 */
        if (rule.nObjectOccup < 1 || rule.nObjectOccup > 100)
        {
            dlog_error("设置人员聚集侦测信息参数错误");
            result(ERR_WEB_PARAM);
            return;
        }
        /* 坐标有效性判断 */
        if (!rule.stRegion.IsValid())
        {
            dlog_error("设置人员聚集侦测信息的区域绘制异常");
            result(ERR_WEB_REGION);
            return;
        }
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::CROWD_GATHERING;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

/* 获取停车侦测信息 */
void Task::Event::GetParkDetectionInfo::handle()
{
    Alarm::ParkingDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置停车侦测信息 */
void Task::Event::SetParkDetectionInfo::handle()
{
    Alarm::ParkingDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 检查智能事件资源冲突 */
    int ret = check_analytics_resource(::Event::Type::PARKING_DETECT, stInfo.bEnable);
    if (ret != 0) {
        result(ret);
        return;
    }
    for (auto &rule : stInfo.aRule)
    {
        /* 参数有效性判断 */
        if (rule.nTimeThreshold < 0 || rule.nTimeThreshold > 100 || rule.nSensitivity < 1 || rule.nSensitivity > 100)
        {
            dlog_error("设置停车侦测信息参数错误");
            result(ERR_WEB_PARAM);
            return;
        }
        /* 坐标有效性判断 */
        if (!rule.stRegion.IsValid())
        {
            dlog_error("设置停车侦测信息的区域绘制异常");
            result(ERR_WEB_REGION);
            return;
        }
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::PARKING_DETECT;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

/* 获取物品遗留侦测信息 */
void Task::Event::GetUnattendedObjectInfo::handle()
{
    Alarm::UnattendedObject_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置物品遗留侦测信息 */
void Task::Event::SetUnattendedObjectInfo::handle()
{
    Alarm::UnattendedObject_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 检查智能事件资源冲突 */
    int ret = check_analytics_resource(::Event::Type::UNATTENDED_OBJECT, stInfo.bEnable);
    if (ret != 0) {
        result(ret);
        return;
    }
    for (auto &rule : stInfo.aRule)
    {
        /* 参数有效性判断 */
        if (rule.nTimeThreshold < 12 || rule.nTimeThreshold > 100 || rule.nSensitivity < 1 || rule.nSensitivity > 100)
        {
            dlog_error("设置物品遗留侦测信息参数错误");
            result(ERR_WEB_PARAM);
            return;
        }
        /* 坐标有效性判断 */
        if (!rule.stRegion.IsValid())
        {
            dlog_error("设置物品遗留侦测信息的区域绘制异常");
            result(ERR_WEB_REGION);
            return;
        }
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::UNATTENDED_OBJECT;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

/* 获取物品拿取侦测信息 */
void Task::Event::GetObjectRemovalInfo::handle()
{
    Alarm::ObjectRemoval_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置物品拿取侦测信息 */
void Task::Event::SetObjectRemovalInfo::handle()
{
    Alarm::ObjectRemoval_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 检查智能事件资源冲突 */
    int ret = check_analytics_resource(::Event::Type::OBJECT_REMOVAL, stInfo.bEnable);
    if (ret != 0) {
        result(ret);
        return;
    }
    for (auto &rule : stInfo.aRule)
    {
        /* 参数有效性判断 */
        if (rule.nTimeThreshold < 12 || rule.nTimeThreshold > 100 || rule.nSensitivity < 1 || rule.nSensitivity > 100)
        {
            dlog_error("设置物品拿取侦测信息参数错误");
            result(ERR_WEB_PARAM);
            return;
        }
        /* 坐标有效性判断 */
        if (!rule.stRegion.IsValid())
        {
            dlog_error("设置物品拿取侦测信息的区域绘制异常");
            result(ERR_WEB_REGION);
            return;
        }
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::OBJECT_REMOVAL;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

/* 获取宠物识别信息 */
void Task::Event::GetPetRecognitionInfo::handle()
{
    Alarm::PetRecognition_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置宠物识别信息 */
void Task::Event::SetPetRecognitionInfo::handle()
{
    Alarm::PetRecognition_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 检查智能事件资源冲突 */
    int ret = check_analytics_resource(::Event::Type::PET_RECOGNITION, stInfo.bEnable);
    if (ret != 0) {
        result(ret);
        return;
    }
    /* 参数有效性判断 */
    if (stInfo.nSensitivity < 1 || stInfo.nSensitivity > 100)
    {
        dlog_error("设置宠物识别信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
    /* 坐标有效性判断 */
    if (!stInfo.stRegion.IsValid())
    {
        dlog_error("设置宠物识别信息的区域绘制异常");
        result(ERR_WEB_REGION);
        return;
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::PET_RECOGNITION;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}
/*人脸比对设置*/
void Task::Event::SetFaceCompareInfo::handle()
{
    dlog_error("人脸比对设置");
    Alarm::FaceCompare_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    int ret = check_analytics_resource(::Event::Type::FACE_COMPARE, stInfo.bEnable);
    if (ret != 0) {
        result(ret);
        return;
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::FACE_COMPARE;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

/* 获取人脸抓拍信息 */
void Task::Event::GetFaceCaptureInfo::handle()
{
    Alarm::FaceCapture_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置人脸抓拍信息 */
void Task::Event::SetFaceCaptureInfo::handle()
{
    Alarm::FaceCapture_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 检查智能事件资源冲突 */
    int ret = check_analytics_resource(::Event::Type::FACE_CAPTURE, stInfo.bEnable);
    if (ret != 0) {
        result(ret);
        return;
    }
    auto &rule = stInfo.stRule;
    /* 参数有效性判断 */
    if (rule.nSensitivity < 1 || rule.nSensitivity > 100)
    {
        dlog_error("设置人脸抓拍信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
    /* 坐标有效性判断 */
    if (!rule.stRegion.IsValid())
    {
        dlog_error("设置人脸抓拍信息的区域绘制异常");
        result(ERR_WEB_REGION);
        return;
    }
    for (auto &region : rule.vstShieldedRegion)
    {
        /* 坐标有效性判断 */
        if (!region.IsValid())
        {
            dlog_error("设置人脸抓拍信息的区域绘制异常");
            result(ERR_WEB_REGION);
            return;
        }
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::FACE_CAPTURE;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

/* 获取人脸抓拍叠加信息 */
void Task::Event::GetFaceCaptureOverlayInfo::handle()
{
    Alarm::OverlayInfo_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置人脸抓拍叠加信息 */
void Task::Event::SetFaceCaptureOverlayInfo::handle()
{
    Alarm::OverlayInfo_S stInfo;
    System::DeviceInfo_S stDeviceInfo;
    Convert::to_struct(m_taskData, stInfo);
    int nRet = CEventConfigure::instance()->set_configure(stInfo);
    if (nRet != 0)
    {
        goto exit;
    }

    /* 同步修改系统设置的设备基本信息中的设备编号 */
    nRet = Convert::read_file(DEVICE_INFO_CONFIG_FILE, stDeviceInfo);
    if (nRet != 0)
    {
        goto exit;
    }
    stDeviceInfo.deviceID = stInfo.nDeviceID;
    nRet = Convert::write_file(DEVICE_INFO_CONFIG_FILE, stDeviceInfo);
    if (nRet != 0)
    {
        goto exit;
    }

exit:
    result(nRet);
}

/* 添加目标库  */
void Task::Event::AddTargetLib::handle()
{
    // ::Event::FaceLibInfo_S stTargetLibInfo;
    // Convert::to_struct(m_taskData, stTargetLibInfo);
    // std::string data = Convert::to_string(stTargetLibInfo); 
    int nRet = -1;
    CEventManage::instance()->send_algo_controlData(AC_ADD_TARGET_LIB,
                                                    m_taskData.c_str(),&nRet);
    if(OK == nRet)
    {
        /* 开启人脸库 */
        dlog_debug("添加目标库成功，开启人脸算法");
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        Alarm::EventSchedule_S stEventSchedule;
        stEventSchedule.enEventType = ::Event::Type_E::FACE_LIB;
        stEventSchedule.bStatus = true;
        stEventSchedule.defenseTime = aAlarmTime;
        CEventConfigure::instance()->set_configure(stEventSchedule);
        CEventManage::instance()->update_event_schedule();
    }
    result(nRet);
}
/* 删除目标库 */
void Task::Event::DelTargetLib::handle()
{
    // ::Event::FaceLibInfo_S stTargetLibInfo;
    // Convert::to_struct(m_taskData, stTargetLibInfo);
    // std::string data = Convert::to_string(stTargetLibInfo); 
    int nRet = -1;
    CEventManage::instance()->send_algo_controlData(AC_DEL_TARGET_LIB,
                                                    m_taskData.c_str(),&nRet);
    
    if(OK == nRet )
    {
        /* 关闭人脸库 */
        dlog_debug("删除目标库成功，关闭人脸算法");
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        Alarm::EventSchedule_S stEventSchedule;
        stEventSchedule.enEventType = ::Event::Type_E::FACE_LIB;
        stEventSchedule.bStatus = false;
        stEventSchedule.defenseTime = aAlarmTime;
        CEventConfigure::instance()->set_configure(stEventSchedule);
        CEventManage::instance()->update_event_schedule();
    }
    result(nRet);
}
/* 设置目标库 */
void Task::Event::SetTargetLib::handle()
{
    int nRet = 0;
    CEventManage::instance()->send_algo_controlData(AC_SET_TARGET_LIB,
                                                    m_taskData.c_str(),&nRet);
    
    result(nRet);
}
/* 获取目标库 */
void Task::Event::GetTargetLib::handle()
{
    
    auto pData = std::make_shared<std::string>();
    CEventManage::instance()->send_algo_controlData(AC_GET_TARGET_LIB,
                                                    m_taskData.c_str(),pData.get());
    
    result(*pData);
}


/* 添加人脸信息 */
void Task::Event::AddFaceInfo::handle()
{
    int nRet = 0;
    CEventManage::instance()->send_algo_controlData(AC_ADD_FACE_INFO,
                                                    m_taskData.c_str(),&nRet);
    
    result(nRet);
}
/* 删除人脸信息 */
void Task::Event::DelFaceInfo::handle()
{
    int nRet = 0;
    CEventManage::instance()->send_algo_controlData(AC_DEL_FACE_INFO,
            m_taskData.c_str());

    result(nRet);
}
/* 设置人脸信息 */
void Task::Event::SetFaceInfo::handle()
{
    int nRet = 0;

    CEventManage::instance()->send_algo_controlData(AC_SET_FACE_INFO,
        m_taskData.c_str());
    result(nRet);
}
/* 获取人脸信息 */
void Task::Event::GetFaceInfo::handle()
{
    auto pData = std::make_shared<std::string>();
    CEventManage::instance()->send_algo_controlData(AC_GET_FACE_INFO,
        m_taskData.c_str(),
        pData.get());
    result(*pData);
}

#ifdef SCENE_INTELLIGENT_ANALYSIS
/**
 * @brief   : 场景智能分析
*/

/* 获取画面分析 */
void Task::Event::GetImageAnalysisInfo::handle()
{
    Alarm::LLMImageAnalysis_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);

    /*当输入为预设提问标记时，清空输入文本以便用户重新输入*/
    if (stInfo.strAnalysisInputText == "[*]预设提问") {
        stInfo.strAnalysisInputText.clear();  
    }
    result(Convert::to_string(stInfo));
}


/* 设置画面分析 */
void Task::Event::SetImageAnalysisInfo::handle()
{
    Alarm::LLMImageAnalysis_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    stInfo.stRepeatedConfig.NormalizeWeekdays(); 
    CEventConfigure::instance()->set_configure(stInfo);
    /* 这里只需要设置事件的启用状态，时间控制由定时分析配置处理 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::IMAGE_ANALYSIS;
    stEventSchedule.bStatus = stInfo.bEnable;
    /* 画面分析不使用传统布防时间，留空即可 */
    stEventSchedule.defenseTime.clear();
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);

    /*加载场景智能分析总控制参数*/
    Alarm::LLMAISceneAnalysis_S stAISceneAnalysisCfg;
    CEventConfigure::instance()->get_configure(stAISceneAnalysisCfg);
     
    ::Event::AlgorithmConfig_S AlgoInfo;
    AlgoInfo.nEnAISceneAnalysis = (int)stAISceneAnalysisCfg.bEnable;
    /*用于画面分析-定时分析实时分析*/
    if(stInfo.bEnable && !stInfo.bScheduleEnable)
    {
        AlgoInfo.nEnLLmInference = 1;
        CEventManage::instance()->update_ai_analysis_switch_status(AlgoInfo);
    }
    else
    {
        CEventManage::instance()->update_ai_analysis_schedule();
    }
    result(nRet);
}

/* 画面分析结果返回 ，实际业务层分析后主动返回*/
void Task::Event::RtImageAnalysisInfoResult::handle()
{
    result(OK);
}

/*  推理分析中断停止 */
void Task::Event::CtrlImageAnalysisStop::handle()
{
    Alarm::LLMAISceneAnalysis_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    stInfo.bEnable=true;
    int nRet = CEventConfigure::instance()->set_configure(stInfo);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}


/* 操作画面分析记录 */
void Task::Event::OperateImageAnalysisRecord::handle()
{
    /* 删除推理记录图片视频文件辅助Lambda：从记录中提取所有路径并提交给后台*/
    auto postDeleteTask = [](const std::vector<Alarm::AnalysisRecords_S>& records) {
        std::vector<std::string> paths;
        /*预分配内存优化性能*/
        paths.reserve(records.size() * 2);
        for (const auto& rec : records) {
            if (!rec.strInputImagePath.empty()) paths.push_back(rec.strInputImagePath);
            if (!rec.strVideoPath.empty()) paths.push_back(rec.strVideoPath);
        }
        /*提交给单例清理器*/
        AnalysisFileCleaner::instance().pushTasks(paths);
    };

    Alarm::AnalysisAllRecordIndexItem_S stSrcStruct;
    Alarm::AnalysisAllRecordIndexItem_S stInfo;
    Convert::to_struct(m_taskData, stInfo);

    /* 获取全部记录*/ 
    /* 功能：返回所有会话的列表。为了减少数据传输，每个会话只包含第一条记录作为"标题"*/
    if(stInfo.enAnalysisRecordOperate == Alarm::AnalysisRecordOperate_E::GET_ALL_RECORDS)
    {
        CEventConfigure::instance()->get_configure(stInfo);
        
        if (stInfo.Allrecords.empty()) {
            /*记录为空*/
            result("[]",OK_NOT_EXIST);
           return;
        } 

        Alarm::AnalysisAllRecordIndexItem_S stPreview;
        // 手动复制必要的元数据 (Metadata)
        stPreview.total_sessions = stInfo.total_sessions;
        stPreview.current_session_index = stInfo.current_session_index;
        stPreview.enAnalysisRecordOperate = Alarm::AnalysisRecordOperate_E::GET_ALL_RECORDS;

        // 遍历源数据，只提取摘要信息
        for (const auto& session : stInfo.Allrecords) {
            Alarm::AnalysisRecordIndexItem_S simpleSession;
            simpleSession.indexKey = session.indexKey; // 复制会话ID
            
            // 提取该会话的第一条记录作为标题/摘要
            // (根据之前的设定，records[0] 是会话的开场白/第一句)
            if (!session.records.empty()) {
                simpleSession.records.push_back(session.records.at(0));
            }
            
            // 将轻量级的会话对象加入预览列表
            stPreview.Allrecords.push_back(simpleSession);
        }
        
        result(Convert::to_string(stPreview));
    }
    /*获取单个会话的所有记录*/
    else if(stInfo.enAnalysisRecordOperate == Alarm::AnalysisRecordOperate_E::GET_SESSION_ALL_RECORDS)
    {
        int index = stInfo.Operateindex;

        CEventConfigure::instance()->get_configure(stInfo);
        stInfo.enAnalysisRecordOperate = Alarm::AnalysisRecordOperate_E::GET_SESSION_ALL_RECORDS;
         /*记录为空*/
        if (stInfo.Allrecords.empty()) {
            /*记录为空*/
            result("[]",OK_NOT_EXIST);
           return;
        } 

         // 检查索引有效性
        if(index < 0 || index >= (int)stInfo.Allrecords.size()) 
        {
            /*记录为空*/
            result("[]",OK_NOT_EXIST);
            return;
        }

        result(Convert::to_string(stInfo.Allrecords.at(index)));
    }
    /* 获取单个会话的单条指定记录*/
    else if(stInfo.enAnalysisRecordOperate == Alarm::AnalysisRecordOperate_E::GET_SESSION_SINGLE_RECORD)
    {

        int index = stInfo.Operateindex;
        int subindex = stInfo.Operatesubindex;

        CEventConfigure::instance()->get_configure(stInfo);
        stInfo.enAnalysisRecordOperate = Alarm::AnalysisRecordOperate_E::GET_SESSION_SINGLE_RECORD;
        /*记录为空*/
        if (stInfo.Allrecords.empty()) {
            /*记录为空*/
            result("[]",OK_NOT_EXIST);
           return;
        } 

        // 检查索引有效性
        if(index < 0 || index >= (int)stInfo.Allrecords.size()) 
        {
            /*记录为空*/
            result("[]",OK_NOT_EXIST);
            return;
        }
        if(subindex < 0 || subindex >= (int)stInfo.Allrecords[index].records.size()) 
        {
            /*记录为空*/
            result("[]",OK_NOT_EXIST);
            return;
        }

        result(Convert::to_string(stInfo.Allrecords.at(index).records.at(subindex)));
    }
    /*  搜索记录*/
    else if(stInfo.enAnalysisRecordOperate == Alarm::AnalysisRecordOperate_E::SEARCH_RECORDS)
    {
        std::string keyword = stInfo.SearchKeyword;
        CEventConfigure::instance()->get_configure(stInfo); 

        if (keyword.empty()) 
        {
            if (stInfo.Allrecords.empty()) {
                /*记录为空*/
                result("[]",OK_NOT_EXIST);
                return;
            }

            Alarm::AnalysisAllRecordIndexItem_S stPreview;
            stPreview.enAnalysisRecordOperate = Alarm::AnalysisRecordOperate_E::GET_ALL_RECORDS;
            stPreview.total_sessions = stInfo.total_sessions;

            // 遍历所有源会话
            for (const auto& session : stInfo.Allrecords) {
                Alarm::AnalysisRecordIndexItem_S simpleSession;
                simpleSession.indexKey = session.indexKey; // 保留会话ID
                
                // 提取该会话的第一条记录作为标题/摘要
                if (!session.records.empty()) {
                    simpleSession.records.push_back(session.records.at(0));
                }
                
                stPreview.Allrecords.push_back(simpleSession);
            }
            
            // 循环结束后统一返回
            result(Convert::to_string(stPreview));
            return;
        }

        // 构造搜索结果容器 (使用 AnalysisAllRecordIndexItem_S 类型)
        Alarm::AnalysisAllRecordIndexItem_S searchResults;
        searchResults.enAnalysisRecordOperate = Alarm::AnalysisRecordOperate_E::SEARCH_RECORDS;
        searchResults.total_sessions = 0; 

        // 遍历所有源会话
        for (const auto& srcSession : stInfo.Allrecords) 
        {
            // 创建一个临时会话容器，用于存放该会话中匹配到的记录
            Alarm::AnalysisRecordIndexItem_S matchedSession;
            matchedSession.indexKey = srcSession.indexKey; 

            // 遍历该会话内的所有记录
            for (const auto& record : srcSession.records) 
            {
                // 简单的子串查找 (同时搜提问和回答)
                if (record.strInputText.find(keyword) != std::string::npos || 
                    record.strOutputText.find(keyword) != std::string::npos) 
                {
                    // 找到了，加入临时会话
                    matchedSession.records.push_back(record);
                }
            }

            // 如果这个会话里有匹配的记录，才把它加入到最终结果列表里
            if (!matchedSession.records.empty()) {
                searchResults.Allrecords.push_back(matchedSession);
            }
        }
        
        // 更新一下匹配到的会话总数
        searchResults.total_sessions = (int)searchResults.Allrecords.size();

        // 返回结果
        if (searchResults.Allrecords.empty()) {
            /*记录为空*/
            result("[]",OK_NOT_EXIST);
        } else {
            result(Convert::to_string(searchResults));
        }
    }
    /* 删除全部记录*/
    else if(stInfo.enAnalysisRecordOperate == Alarm::AnalysisRecordOperate_E::DELETE_ALL_RECORDS)
    {
        CEventConfigure::instance()->get_configure(stSrcStruct);

        /*收集所有待删除的记录 */
        std::vector<Alarm::AnalysisRecords_S> allRecordsBackup;
        for (const auto& session : stSrcStruct.Allrecords) {
            allRecordsBackup.insert(allRecordsBackup.end(), session.records.begin(), session.records.end());
        }

        /* 清空记录*/
        stSrcStruct.Allrecords.clear();
        stSrcStruct.total_sessions = 0;

        /*更新配置*/
        int nRet = CEventConfigure::instance()->set_configure(stSrcStruct);
        result(nRet);

        /*物理文件删除任务给后台*/
        postDeleteTask(allRecordsBackup);

    }
    /*删除单个会话的所有记录*/
    else if(stInfo.enAnalysisRecordOperate == Alarm::AnalysisRecordOperate_E::DELETE_SESSION_ALL_RECORDS)
    {
        CEventConfigure::instance()->get_configure(stSrcStruct);

        if (stInfo.Operateindex >= 0 && stInfo.Operateindex < (int)stSrcStruct.Allrecords.size()) {
            
            /*备份该会话下的记录*/
            std::vector<Alarm::AnalysisRecords_S> sessionRecordsBackup = stSrcStruct.Allrecords[stInfo.Operateindex].records;

            /*从内存中移除会话*/
            stSrcStruct.Allrecords.erase(stSrcStruct.Allrecords.begin() + stInfo.Operateindex);
            stSrcStruct.total_sessions = (int)stSrcStruct.Allrecords.size();

            int nRet = CEventConfigure::instance()->set_configure(stSrcStruct);
            result(nRet);

             /*物理文件删除任务给后台*/
            postDeleteTask(sessionRecordsBackup);
        } 
        else 
        {
            result(ERR);
            return;
        }    
    }
    /* 删除单个会话的单条指定记录，增加DelKeyID关键ID删除*/
    else if(stInfo.enAnalysisRecordOperate == Alarm::AnalysisRecordOperate_E::DELETE_SESSION_SINGLE_RECORD)
    {
        CEventConfigure::instance()->get_configure(stSrcStruct);

        /* 优先级最高：判断 DelKeyID 是否有值 */
        if (!stInfo.DelKeyID.empty())
        {
            bool bFound = false;
            
            /* 遍历所有会话 */
            for (int i = 0; i < (int)stSrcStruct.Allrecords.size(); ++i)
            {
                auto& targetSessionRecords = stSrcStruct.Allrecords[i].records;
                
                /* 遍历当前会话下的所有记录 */
                for (int j = 0; j < (int)targetSessionRecords.size(); ++j)
                {
                    /* 找到匹配的 strId */
                    if (targetSessionRecords[j].strId == stInfo.DelKeyID)
                    {
                        /*备份这一条记录*/
                        std::vector<Alarm::AnalysisRecords_S> singleRecordBackup;
                        singleRecordBackup.push_back(targetSessionRecords[j]);
                        
                        /* 删除记录 */
                        targetSessionRecords.erase(targetSessionRecords.begin() + j);

                        /*如果该会话没有记录了，删除整个会话*/
                        if (targetSessionRecords.empty()) {
                            stSrcStruct.Allrecords.erase(stSrcStruct.Allrecords.begin() + i);
                            stSrcStruct.total_sessions = (int)stSrcStruct.Allrecords.size();
                        }

                        int nRet = CEventConfigure::instance()->set_configure(stSrcStruct);
                        result(nRet);

                        /*物理文件删除任务给后台*/
                        postDeleteTask(singleRecordBackup);

                        bFound = true;
                        break; // 找到并删除后，跳出内层循环
                    }
                }
                
                if (bFound)
                {
                    break; 
                }
            }

            /*  没有找到任何记录，返回错误 */
            if (!bFound)
            {
                result(ERR);
                return;
            }
        }
        else
        {
            /* DelKeyID 无值时 */
            if (stInfo.Operateindex >= 0 && stInfo.Operateindex < (int)stSrcStruct.Allrecords.size() &&
                stInfo.Operatesubindex >= 0 && stInfo.Operatesubindex < (int)stSrcStruct.Allrecords[stInfo.Operateindex].records.size()) 
            {
                auto& targetSessionRecords = stSrcStruct.Allrecords[stInfo.Operateindex].records;
                
                /*备份这一条记录*/
                std::vector<Alarm::AnalysisRecords_S> singleRecordBackup;
                singleRecordBackup.push_back(targetSessionRecords[stInfo.Operatesubindex]);
                
                targetSessionRecords.erase(targetSessionRecords.begin() + stInfo.Operatesubindex);

                /*如果该会话没有记录了，删除整个会话*/
                if (targetSessionRecords.empty()) {
                    stSrcStruct.Allrecords.erase(stSrcStruct.Allrecords.begin() + stInfo.Operateindex);
                    stSrcStruct.total_sessions = (int)stSrcStruct.Allrecords.size();
                }

                int nRet = CEventConfigure::instance()->set_configure(stSrcStruct);
                result(nRet);

                /*物理文件删除任务给后台*/
                postDeleteTask(singleRecordBackup);
            } 
            else 
            {
                result(ERR);
                return;
            }
        }
    }
    //设置会话的索引值，如当前是第二个会话,current_session_index=1
    else if(stInfo.enAnalysisRecordOperate == Alarm::AnalysisRecordOperate_E::SET_CURRENT_SESSION_INDEX)
    {

        int current_index = stInfo.current_session_index;

        CEventConfigure::instance()->get_configure(stSrcStruct);

        // 检查索引有效性
        if(current_index < 0 || current_index > (int)stSrcStruct.Allrecords.size()) 
        {
            result(ERR);
            return;
        }

        /*当前会话的索引*/
        stSrcStruct.current_session_index = current_index;
        /*会话总数*/
        stSrcStruct.total_sessions = (int)stSrcStruct.Allrecords.size();

        int nRet = CEventConfigure::instance()->set_configure(stSrcStruct);
        result(nRet);
    }
}

/* 获取文字预设任务 */
void Task::Event::GetTextPresetTaskInfo::handle()
{
    /* 解析查询过滤条件 */
    Alarm::TextPresetQueryFilter_S stFilter;
    Convert::to_struct(m_taskData, stFilter);
    
    /* 调用管理器查询接口 */
    Alarm::TextPresetTaskManager_S stResult;
    int nRet = CEventVlmManager::instance()->queryTextPresetTasks(stFilter, stResult);
    
    if (nRet == OK)
    {
        result(Convert::to_string(stResult));
    }
    else
    {
        result(nRet);
    }
}

/* 设置文字预设任务 */
void Task::Event::SetTextPresetTaskInfo::handle()
{
    Alarm::TextPreset_S stTaskRequest;
    Convert::to_struct(m_taskData, stTaskRequest);
    
    /* 调用管理器设置接口 */
    int nRet = CEventVlmManager::instance()->setTextPresetTask(stTaskRequest);
    
    result(nRet);
}

/* 获取实时预警推送 */
void Task::Event::GetRealAlarmPushInfo::handle()
{
    /* 解析查询过滤条件 */
    Alarm::RealAlarmPushQueryFilter_S stFilter;
    Convert::to_struct(m_taskData, stFilter);

    dlog_info("接收到实时预警推送查询请求 - 任务名: [%s], 目标: [%s], 条件: [%s], 处理状态: %d", 
    stFilter.strTaskNameFilter.c_str(),
    stFilter.strObjectNameFilter.c_str(), 
    stFilter.strConditionNameFilter.c_str(),
    stFilter.enDealStatusFilter);

    
    /* 调用管理器查询接口 */
    Alarm::RealAlarmPushManager_S stResult;
    int nRet = CEventVlmManager::instance()->queryRealAlarmPushRecords(stFilter, stResult);
    
    if (nRet == OK)
    {
        result(Convert::to_string(stResult));
    }
    else
    {
        result(nRet);
    }
}

/* 获取实时预警推送处理记录 */
void Task::Event::GetRealAlarmProcessInfo::handle()
{
    /* 解析为批量请求格式 */
    Alarm::RealAlarmPushRecord_S stInfo;
    Convert::to_struct(m_taskData, stInfo);

    /* 获取所有推送记录 */
    Alarm::RealAlarmPushManager_S stManager;
    int nRet = CEventConfigure::instance()->get_configure(stManager);
    if (nRet != 0)
    {
        dlog_warn("获取实时预警推送配置失败，错误码: %d", nRet);
        result(nRet);
    }

    for (const auto& record : stManager.aPushRecords) 
    {
        if (record.strTaskId == stInfo.strTaskId) {
            auto FoundRecord = record;                       
            result(Convert::to_string(FoundRecord));       
            return;
        }
    }
    result(OK_NOT_EXIST);
}

/* 设置实时预警推送 */
void Task::Event::SetRealAlarmPushInfo::handle()
{
    /* 解析为批量请求格式 */
    Alarm::RealAlarmPushBatchRequest_S stBatchRequest;
    Convert::to_struct(m_taskData, stBatchRequest);

    const char* operationNames[] = {"处理", "删除", "忽视", "设置"};
    const char* opName = (stBatchRequest.enOperationType >= 0 && stBatchRequest.enOperationType <= 3) 
                        ? operationNames[stBatchRequest.enOperationType] : "未知";
    
    bool bIsBatch = stBatchRequest.aTaskIds.size() > 1;
    
    dlog_info("接收到%s实时预警推送%s请求 - 操作类型: %d, 任务数量: %zu", 
             bIsBatch ? "批量" : "单个",
             opName, 
             stBatchRequest.enOperationType, 
             stBatchRequest.aTaskIds.size());
 
    /* 处理自动配置设置操作 */
    if (stBatchRequest.enOperationType == Alarm::PUSH_OP_AUTO)
    {
        result(CEventVlmManager::instance()->setRealAlarmPushAutoConfig(stBatchRequest));
    }
    
    /* 验证请求数据 */
    if (stBatchRequest.aTaskIds.empty())
    {
        dlog_error("任务ID数组为空");
        result(ERR);
        return;
    }
    
    
    /* 调用统一处理接口 */
    Alarm::RealAlarmPushBatchResult_S stBatchResult;
    int nRet = CEventVlmManager::instance()->processRealAlarmPushRecords(stBatchRequest, stBatchResult);

    dlog_error("%s%s失败，错误码: %d", bIsBatch ? "批量" : "单个", opName, nRet);
    result(nRet);

}
#endif

#ifdef SCENE_INTELLIGENCE
void Task::Event::PushFaceCaptureInfo::handle()
{

}

void Task::Event::SetPushFaceCaptureInfo::handle()
{

}

void Task::Event::PushPersonCaptureInfo::handle()
{

}

void Task::Event::PushMotorVehicleCaptureInfo::handle()
{

}

void Task::Event::PushNonMotorVehicleCaptureInfo::handle()
{

}

void Task::Event::SetAttributeInfo::handle()
{
    Alarm::AttributeDetectSwitch_S stInfo;
    Convert::to_struct(m_taskData, stInfo);

    int nRet = CEventConfigure::instance()->set_configure(stInfo);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetAttributeInfo::handle()
{
    Alarm::AttributeDetectSwitch_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::GetFenceClimbingInfo::handle()
{
    Alarm::FenceClimbingDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetFenceClimbingInfo::handle()
{
    Alarm::FenceClimbingDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    for (auto &rule : stInfo.aRule)
    {
        /* 参数有效性判断 */
        if (rule.nSensitivity < 1 || rule.nSensitivity > 100)
        {
            dlog_error("设置翻越围栏信息参数错误");
            result(ERR_WEB_PARAM);
            return;
        }
        /* 坐标有效性判断 */
        if (!rule.stRegion.IsValid())
        {
            dlog_error("设置翻越围栏信息的区域绘制异常");
            result(ERR_WEB_REGION);
            return;
        }
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::FENCE_CLIMBING;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetLeavePostInfo::handle()
{
    Alarm::LeavePostDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetLeavePostInfo::handle()
{
    Alarm::LeavePostDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    for (auto &rule : stInfo.aRule)
    {
        /* 参数有效性判断 */
        if (rule.nTimeThreshold < 0 || rule.nTimeThreshold > 100 || rule.nSensitivity < 1 || rule.nSensitivity > 100)
        {
            dlog_error("设置离岗识别信息参数错误");
            result(ERR_WEB_PARAM);
            return;
        }
        /* 坐标有效性判断 */
        if (!rule.stRegion.IsValid())
        {
            dlog_error("设置离岗识别的区域绘制异常");
            result(ERR_WEB_REGION);
            return;
        }
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::LEAVE_POST;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetIllegalLaneChangeInfo::handle()
{
    Alarm::IllegalLaneChangeDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetIllegalLaneChangeInfo::handle()
{
    Alarm::IllegalLaneChangeDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 参数有效性判断 */
    for (auto &rule : stInfo.aRule)
    {
        /* 坐标有效性判断 */
        if (!rule.stStartPos.IsValid())
        {
            dlog_error("设置违规变道侦测参数的区域绘制异常");
            result(ERR_WEB_REGION);
            return;
        }

    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::ILLEGAL_LANE_CHANGE;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);

}

void Task::Event::GetReverseDirectionInfo::handle()
{
    Alarm::DrivingAgainstTrafficDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetReverseDirectionInfo::handle()
{
    Alarm::DrivingAgainstTrafficDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 参数有效性判断 */
    for (auto &rule : stInfo.aRule)
    {
        /* 坐标有效性判断 */
        if (!rule.stStartPos.IsValid())
        {
            dlog_error("设置逆行侦测参数的区域绘制异常");
            result(ERR_WEB_REGION);
            return;
        }
        /* 判断每个区域的起始点和终点是否反了，我们规定，左边的点都是起始点,如果横坐标相同，上方的点为起始点 */
        bool bSwap = false;
        if (rule.stStartPos.fX == rule.stEndPos.fX)
        {
            if (rule.stStartPos.fY > rule.stEndPos.fY)
            {
                bSwap = true;
            }
        }
        else if (rule.stStartPos.fX > rule.stEndPos.fX)
        {
            bSwap = true;
        }
        if (bSwap)
        {
            /* 对调点坐标 */
            Common::PosF_S stTmpPos = rule.stStartPos;
            rule.stStartPos = rule.stEndPos;
            rule.stEndPos = stTmpPos;
        }
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::REVERSE_DIRECTION;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetNonMotorVehicleIntrusionInfo::handle()
{
    Alarm::NonMotorVehicleIntrusionDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetNonMotorVehicleIntrusionInfo::handle()
{
    Alarm::NonMotorVehicleIntrusionDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    for (auto &rule : stInfo.aRule)
    {
        /* 参数有效性判断 */
        if (rule.nSensitivity < 1 || rule.nSensitivity > 100)
        {
            dlog_error("设置非机动车闯入信息参数错误");
            result(ERR_WEB_PARAM);
            return;
        }
        /* 坐标有效性判断 */
        if (!rule.stRegion.IsValid())
        {
            dlog_error("设置非机动车闯入的区域绘制异常");
            result(ERR_WEB_REGION);
            return;
        }
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::NON_MOTOR_VEHICLE_INTRUSION;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetEmergencyLaneOccupancyInfo::handle()
{
    Alarm::EmergencyLaneOccupancyDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetEmergencyLaneOccupancyInfo::handle()
{
    Alarm::EmergencyLaneOccupancyDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    for (auto &rule : stInfo.aRule)
    {
        /* 参数有效性判断 */
        if (rule.nTimeThreshold < 0 || rule.nTimeThreshold > 100 || rule.nSensitivity < 1 || rule.nSensitivity > 100)
        {
            dlog_error("设置应急车道占用信息参数错误");
            result(ERR_WEB_PARAM);
            return;
        }
        /* 坐标有效性判断 */
        if (!rule.stRegion.IsValid())
        {
            dlog_error("设置应急车道占用侦测信息的区域绘制异常");
            result(ERR_WEB_REGION);
            return;
        }
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::EMERGENCY_LANE_OCCUPANCY;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetPedestrianIntrusionInfo::handle()
{
    Alarm::PedestrianIntrusionDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetPedestrianIntrusionInfo::handle()
{
    Alarm::PedestrianIntrusionDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    for (auto &rule : stInfo.aRule)
    {
        /* 参数有效性判断 */
        if (rule.nSensitivity < 1 || rule.nSensitivity > 100)
        {
            dlog_error("设置行人闯入信息参数错误");
            result(ERR_WEB_PARAM);
            return;
        }
        /* 坐标有效性判断 */
        if (!rule.stRegion.IsValid())
        {
            dlog_error("设置行人闯入的区域绘制异常");
            result(ERR_WEB_REGION);
            return;
        }
    }
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::PEDESTRIAN_INTRUSION;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetSmokeFireInfo::handle()
{
    Alarm::SmokeFireDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetSmokeFireInfo::handle()
{
    Alarm::SmokeFireDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
   
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置烟火识别信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }

    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::SMOKE_FIRE;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetRoadPondingInfo::handle()
{
    Alarm::RoadPondingDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetRoadPondingInfo::handle()
{
    Alarm::RoadPondingDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
   
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置道路积水检测信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }

    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::ROAD_PONDING;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetManholeCoverAbnormalInfo::handle()
{
    Alarm::ManholeCoverAbnormalDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetManholeCoverAbnormalInfo::handle()
{
    Alarm::ManholeCoverAbnormalDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
   
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置井盖异常检测信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
        
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::MANHOLE_COVER_ABNORMAL;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetSleepOnDutyInfo::handle()
{
    Alarm::SleepOnDutyDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetSleepOnDutyInfo::handle()
{
     Alarm::SleepOnDutyDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
   
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置睡岗识别信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
        
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::SLEEP_ON_DUTY;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetElectricVehicleInElevatorInfo::handle()
{
    Alarm::ElectricScooterDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetElectricVehicleInElevatorInfo::handle()
{
    Alarm::ElectricScooterDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
   
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置电瓶车识别信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
        
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::ELECTRIC_VEHICLE_IN_ELEVATOR;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetPersonFallDownInfo::handle()
{
    Alarm::PersonFallDownDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetPersonFallDownInfo::handle()
{
    Alarm::PersonFallDownDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置人员倒地识别信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
        
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::PERSON_FALL_DOWN;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetConstructionOccupyRoadInfo::handle()
{
    Alarm::ConstructionEncroachmentRoadDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetConstructionOccupyRoadInfo::handle()
{
    Alarm::ConstructionEncroachmentRoadDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置施工占道识别信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
        
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::CONSTRUCTION_OCCUPY_ROAD;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetCongestionInfo::handle()
{
    Alarm::CongestionDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetCongestionInfo::handle()
{
    Alarm::CongestionDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置拥堵识别信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }

    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::CONGESTION;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetLicensePlateRecognitionInfo::handle()
{
    Alarm::LicensePlateCognitionDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetLicensePlateRecognitionInfo::handle()
{   
    Alarm::LicensePlateCognitionDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置车牌识别信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }

    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::PLATE_NUMBER;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetHighAltitudeSeatbeltInfo::handle()
{
    Alarm::HighAltitudeSeatbeltDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetHighAltitudeSeatbeltInfo::handle()
{
     Alarm::HighAltitudeSeatbeltDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置高空安全带检测信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
        
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::HIGH_ALTITUDE_SEATBELT;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetSafetyHelmetInfo::handle()
{
     Alarm::SafetyHelmetDection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetSafetyHelmetInfo::handle()
{
    Alarm::SafetyHelmetDection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置安全帽检测信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
        
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::SAFETY_HELMET;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetPersonFallInfo::handle()
{
    Alarm::TripDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetPersonFallInfo::handle()
{
    Alarm::TripDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置摔倒识别信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
        
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::PERSON_TRIP;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetPhoneUsageInfo::handle()
{
    Alarm::PhoneUsageDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetPhoneUsageInfo::handle()
{
    Alarm::PhoneUsageDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置玩手机识别信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
        
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::PHONE_USAGE;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetSmokingInfo::handle()
{
    Alarm::SmokingDection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetSmokingInfo::handle()
{
    Alarm::SmokingDection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
   
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置抽烟识别信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
        
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::SMOKING;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetOpenFlameInfo::handle()
{
    Alarm::OpenFlameDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetOpenFlameInfo::handle()
{
    Alarm::OpenFlameDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
   
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置明火识别信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
        
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::OPEN_FLAME;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetBareSoilInfo::handle()
{
     Alarm::BareSoiletDection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetBareSoilInfo::handle()
{
    Alarm::BareSoiletDection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
   
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置黄土裸露检测信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
        
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::BARE_SOIL;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetHoleProtectionBarInfo::handle()
{
    Alarm::HoleProtectionBarDection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetHoleProtectionBarInfo::handle()
{
    Alarm::HoleProtectionBarDection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
   
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置洞口防护栏检测信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
        
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::HOLE_PROTECTION_BAR;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

void Task::Event::GetReflectiveClothingInfo::handle()
{
     Alarm::ReflectiveClothingDection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetReflectiveClothingInfo::handle()
{
    Alarm::ReflectiveClothingDection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
   
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置反光衣检测信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
        
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::REFLECTIVE_CLOTHING;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

#endif

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
void Task::Event::GetGarbageExposureInfo::handle()
{
    Alarm::GarbageExposureDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetGarbageExposureInfo::handle()
{
     Alarm::GarbageExposureDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
   
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置垃圾暴露检测信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
        
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::GARBAGE_EXPOSURE;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}


void Task::Event::GetGarbageOverflowInfo::handle()
{
    Alarm::GarbageOverflowDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

void Task::Event::SetGarbageOverflowInfo::handle()
{
    Alarm::GarbageOverflowDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
   dlog_debug("垃圾满溢灵敏度[%d]",stInfo.stRule.nSensitivity);
    /* 参数有效性判断 */
    if (stInfo.stRule.nSensitivity < 1 || stInfo.stRule.nSensitivity > 100)
    {
        dlog_error("设置垃圾满溢检测信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
        
    CEventConfigure::instance()->set_configure(stInfo);

    /* 更新事件布防时间 */
    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::GARBAGE_OVERFLOW;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}
#endif

#if CAP_AI_PEOPLE_STATISTICS
/* 获取人流统计信息 */
void Task::Event::GetPeopleFlowStatisticsInfo::handle()
{
    Alarm::PeopleFlowStatistics_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置人流统计信息 */
void Task::Event::SetPeopleFlowStatisticsInfo::handle()
{
    Alarm::PeopleFlowStatistics_S stInfo;
    Convert::to_struct(m_taskData, stInfo);

    if (stInfo.nSensitivity < 1 || stInfo.nSensitivity > 100 || stInfo.nReportInterval == 0)
    {
        dlog_error("设置人流统计信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }

    if (!stInfo.stRuleLine.stStartPos.IsValid() || !stInfo.stRuleLine.stEndPos.IsValid())
    {
        dlog_error("设置人流统计信息的规则线绘制异常");
        result(ERR_WEB_REGION);
        return;
    }

    if (!stInfo.stDetectRegion.IsValid())
    {
        dlog_error("设置人流统计信息的区域绘制异常");
        result(ERR_WEB_REGION);
        return;
    }

    if (!check_population_alarm_thresholds(stInfo.stStayAlarm))
    {
        dlog_error("设置人流统计信息告警阈值错误");
        result(ERR_WEB_PARAM);
        return;
    }

    int ret = check_analytics_resource(::Event::Type::PEOPLE_FLOW_STATISTICS, stInfo.bEnable);
    if (ret != 0)
    {
        result(ret);
        return;
    }

    CEventConfigure::instance()->set_configure(stInfo);

    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::PEOPLE_FLOW_STATISTICS;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}

/* 立即清零人流统计结果 */
void Task::Event::ClearPeopleFlowStatisticsResult::handle()
{
    int nRet = CEventManage::instance()->send_algo_controlData(AC_CLEAR_PEOPLE_FLOW_STATISTICS_RESULT);
    result(nRet);
}

/* 获取人员密度检测信息 */
void Task::Event::GetPeopleDensityDetectionInfo::handle()
{
    Alarm::PeopleDensityDetection_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置人员密度检测信息 */
void Task::Event::SetPeopleDensityDetectionInfo::handle()
{
    Alarm::PeopleDensityDetection_S stInfo;
    Convert::to_struct(m_taskData, stInfo);

    if (stInfo.nSensitivity < 1 || stInfo.nSensitivity > 100 || stInfo.nReportInterval == 0)
    {
        dlog_error("设置人员密度检测信息参数错误");
        result(ERR_WEB_PARAM);
        return;
    }

    if (!stInfo.stDetectRegion.IsValid())
    {
        dlog_error("设置人员密度检测信息的区域绘制异常");
        result(ERR_WEB_REGION);
        return;
    }

    if (!check_population_alarm_thresholds(stInfo.stDensityAlarm))
    {
        dlog_error("设置人员密度检测信息告警阈值错误");
        result(ERR_WEB_PARAM);
        return;
    }

    int ret = check_analytics_resource(::Event::Type::PEOPLE_DENSITY_DETECTION, stInfo.bEnable);
    if (ret != 0)
    {
        result(ret);
        return;
    }

    CEventConfigure::instance()->set_configure(stInfo);

    Alarm::EventSchedule_S stEventSchedule;
    stEventSchedule.enEventType = ::Event::Type_E::PEOPLE_DENSITY_DETECTION;
    stEventSchedule.bStatus = stInfo.bEnable;
    stEventSchedule.defenseTime = stInfo.aAlarmTime;
    int nRet = CEventConfigure::instance()->set_configure(stEventSchedule);
    CEventManage::instance()->update_event_schedule();
    result(nRet);
}
#endif
