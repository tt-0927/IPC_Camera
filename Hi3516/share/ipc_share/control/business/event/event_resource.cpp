/**
 * @FilePath     : event_resource.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-04 19:47:13
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-25 15:27:04
 * @Description  : 事件资源管理
 */

#include "event_resource.h"
#include "event_manage.h"
#include "IpcRet.h"

#if CAP_SYSTEM_REBOOT_MUTE // 系统重启静音处理
#include "stream_ao.h"
#endif

/* 将SmartEventEnableStatus_S的bool成员映射到对应的Event::Type_E */
const std::map<Event::Type_E, CEventResource::BoolMemberPtr> CEventResource::m_event_to_status_map = {
    /* 周界事件 */
    {Event::Type_E::LINE_CROSSING, &Event::SmartEventEnableStatus_S::bLineCrossing},
    {Event::Type_E::INTRUSION, &Event::SmartEventEnableStatus_S::bIntrusion},
    {Event::Type_E::ENTER_REGION, &Event::SmartEventEnableStatus_S::bEnterRegion},
    {Event::Type_E::LEAVE_REGION, &Event::SmartEventEnableStatus_S::bLeaveRegion},
    /* 行为分析 */
    {Event::Type_E::LOITERING_DETECT, &Event::SmartEventEnableStatus_S::bLoiteringDetect},
    {Event::Type_E::CROWD_GATHERING, &Event::SmartEventEnableStatus_S::bCrowdGathering},
    {Event::Type_E::PARKING_DETECT, &Event::SmartEventEnableStatus_S::bParkingDetect},
    /* 场景检测 */
    {Event::Type_E::AUDIO_ANOMALY, &Event::SmartEventEnableStatus_S::bAudioAnomaly},
    {Event::Type_E::SCENE_CHANGE, &Event::SmartEventEnableStatus_S::bSceneChange},
    {Event::Type_E::UNATTENDED_OBJECT, &Event::SmartEventEnableStatus_S::bUnattendedObject},
    {Event::Type_E::OBJECT_REMOVAL, &Event::SmartEventEnableStatus_S::bObjectRemoval},
    /* 目标检测 */
    {Event::Type_E::FACE_DETECT, &Event::SmartEventEnableStatus_S::bFaceDetect},
    {Event::Type_E::PET_RECOGNITION, &Event::SmartEventEnableStatus_S::bPetRecognition},
    /* 人脸抓拍 */
    {Event::Type_E::FACE_CAPTURE, &Event::SmartEventEnableStatus_S::bFaceCapture},
    {Event::Type_E::FACE_COMPARE, &Event::SmartEventEnableStatus_S::bFaceCompare},
#ifdef SCENE_INTELLIGENCE
    /* 行为监管 */
    {Event::Type_E::SLEEP_ON_DUTY, &Event::SmartEventEnableStatus_S::bSleepOnDuty},
    {Event::Type_E::LEAVE_POST, &Event::SmartEventEnableStatus_S::bLeavePost},
    {Event::Type_E::ELECTRIC_VEHICLE_IN_ELEVATOR, &Event::SmartEventEnableStatus_S::bElectricVehicleInElevator},
    {Event::Type_E::PERSON_FALL_DOWN, &Event::SmartEventEnableStatus_S::bPersonFallDown},
    {Event::Type_E::FENCE_CLIMBING, &Event::SmartEventEnableStatus_S::bFenceClimbing},
    {Event::Type_E::SMOKING, &Event::SmartEventEnableStatus_S::bSmoking},
    {Event::Type_E::PHONE_USAGE, &Event::SmartEventEnableStatus_S::bPhoneUsage},
    {Event::Type_E::SMOKE_FIRE, &Event::SmartEventEnableStatus_S::bSmokeFire},
    {Event::Type_E::OPEN_FLAME, &Event::SmartEventEnableStatus_S::bOpenFlame},
    {Event::Type_E::MANHOLE_COVER_ABNORMAL, &Event::SmartEventEnableStatus_S::bManholeCoverAbnormal},
    {Event::Type_E::BARE_SOIL, &Event::SmartEventEnableStatus_S::bBareSoil},
    {Event::Type_E::HOLE_PROTECTION_BAR, &Event::SmartEventEnableStatus_S::bHoleProtectionBar},
    {Event::Type_E::PEDESTRIAN_INTRUSION, &Event::SmartEventEnableStatus_S::bPedestrianIntrusion},
    {Event::Type_E::PERSON_TRIP, &Event::SmartEventEnableStatus_S::bTrip},
    /* 穿戴规范 */
    {Event::Type_E::SAFETY_HELMET, &Event::SmartEventEnableStatus_S::bSafetyHelmet},
    {Event::Type_E::REFLECTIVE_CLOTHING, &Event::SmartEventEnableStatus_S::bReflectiveClothing},
    {Event::Type_E::HIGH_ALTITUDE_SEATBELT, &Event::SmartEventEnableStatus_S::bHighAltitudeSeatbelt},
    /* 交通行为监管 */
    {Event::Type_E::CONSTRUCTION_OCCUPY_ROAD, &Event::SmartEventEnableStatus_S::bConstructionOccupyRoad},
    {Event::Type_E::EMERGENCY_LANE_OCCUPANCY, &Event::SmartEventEnableStatus_S::bEmergencyLaneOccupancy},
    {Event::Type_E::REVERSE_DIRECTION, &Event::SmartEventEnableStatus_S::bReverseDirection},
    {Event::Type_E::NON_MOTOR_VEHICLE_INTRUSION, &Event::SmartEventEnableStatus_S::bNonMotorVehicleIntrusion},
    {Event::Type_E::ROAD_PONDING, &Event::SmartEventEnableStatus_S::bRoadPonding},
    {Event::Type_E::CONGESTION, &Event::SmartEventEnableStatus_S::bCongestion},
    {Event::Type_E::ILLEGAL_PARKING, &Event::SmartEventEnableStatus_S::bIllegalParking},
    {Event::Type_E::ILLEGAL_LANE_CHANGE, &Event::SmartEventEnableStatus_S::bIllegalLaneChange},
    /* 属性识别 */
    {Event::Type_E::PLATE_NUMBER, &Event::SmartEventEnableStatus_S::bPlateNumber},
#endif
#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
    /* 垃圾暴露检测 */
    {Event::Type_E::GARBAGE_EXPOSURE, &Event::SmartEventEnableStatus_S::bGarbageExposure},
    /* 垃圾满溢识别 */
    {Event::Type_E::GARBAGE_OVERFLOW, &Event::SmartEventEnableStatus_S::bGarbageOverflow},
#endif
#if CAP_AI_PEOPLE_STATISTICS
    /* 人数统计 */
    {Event::Type_E::PEOPLE_FLOW_STATISTICS, &Event::SmartEventEnableStatus_S::bPeopleFlowStatistics},
    {Event::Type_E::PEOPLE_DENSITY_DETECTION, &Event::SmartEventEnableStatus_S::bPeopleDensityDetection},
#endif
/*场景智能分析*/
#ifdef SCENE_INTELLIGENT_ANALYSIS
    /* AI场景智能分析开关 */
    {Event::Type_E::AI_SCENE_ANALYSIS, &Event::SmartEventEnableStatus_S::bSceneAnalysis},
#endif
};

/* 将事件类型映射到其所属的互斥组 */
const std::map<Event::Type_E, Event::SmartCategory_E> CEventResource::m_event_to_group_map = {
    /* 周界事件 */
    {Event::Type_E::LINE_CROSSING, Event::SmartCategory_E::PERIMETER},
    {Event::Type_E::INTRUSION, Event::SmartCategory_E::PERIMETER},
    {Event::Type_E::ENTER_REGION, Event::SmartCategory_E::PERIMETER},
    {Event::Type_E::LEAVE_REGION, Event::SmartCategory_E::PERIMETER},
    /* 行为分析 */
    {Event::Type_E::LOITERING_DETECT, Event::SmartCategory_E::BEHAVIOURAL_ANALYSIS},
    {Event::Type_E::CROWD_GATHERING, Event::SmartCategory_E::BEHAVIOURAL_ANALYSIS},
    {Event::Type_E::PARKING_DETECT, Event::SmartCategory_E::BEHAVIOURAL_ANALYSIS},
    /* 场景检测 */
    {Event::Type_E::AUDIO_ANOMALY, Event::SmartCategory_E::SCENE_DETECTION},
    {Event::Type_E::SCENE_CHANGE, Event::SmartCategory_E::SCENE_DETECTION},
    {Event::Type_E::UNATTENDED_OBJECT, Event::SmartCategory_E::SCENE_DETECTION},
    {Event::Type_E::OBJECT_REMOVAL, Event::SmartCategory_E::SCENE_DETECTION},
    /* 目标检测 */
    {Event::Type_E::FACE_DETECT, Event::SmartCategory_E::TARGET_DETECTION},
    {Event::Type_E::PET_RECOGNITION, Event::SmartCategory_E::TARGET_DETECTION},
    /* 人脸抓拍 */
    {Event::Type_E::FACE_CAPTURE, Event::SmartCategory_E::FACE_CAPTURE},

    /* 人脸比对 */
    {Event::Type_E::FACE_COMPARE, Event::SmartCategory_E::FACE_CAPTURE},
#ifdef SCENE_INTELLIGENCE
    /* 行为监管 */
    {Event::Type_E::SLEEP_ON_DUTY, Event::SmartCategory_E::BEHAVIOR_MONITORING},
    {Event::Type_E::LEAVE_POST, Event::SmartCategory_E::BEHAVIOR_MONITORING},
    {Event::Type_E::ELECTRIC_VEHICLE_IN_ELEVATOR, Event::SmartCategory_E::BEHAVIOR_MONITORING},
    {Event::Type_E::PERSON_FALL_DOWN, Event::SmartCategory_E::BEHAVIOR_MONITORING},
    {Event::Type_E::FENCE_CLIMBING, Event::SmartCategory_E::BEHAVIOR_MONITORING},
    {Event::Type_E::SMOKING, Event::SmartCategory_E::BEHAVIOR_MONITORING},
    {Event::Type_E::PHONE_USAGE, Event::SmartCategory_E::BEHAVIOR_MONITORING},
    {Event::Type_E::SMOKE_FIRE, Event::SmartCategory_E::BEHAVIOR_MONITORING},
    {Event::Type_E::OPEN_FLAME, Event::SmartCategory_E::BEHAVIOR_MONITORING},
    {Event::Type_E::MANHOLE_COVER_ABNORMAL, Event::SmartCategory_E::BEHAVIOR_MONITORING},
    {Event::Type_E::BARE_SOIL, Event::SmartCategory_E::BEHAVIOR_MONITORING},
    {Event::Type_E::HOLE_PROTECTION_BAR, Event::SmartCategory_E::BEHAVIOR_MONITORING},
    {Event::Type_E::PEDESTRIAN_INTRUSION, Event::SmartCategory_E::BEHAVIOR_MONITORING},
    {Event::Type_E::PERSON_TRIP, Event::SmartCategory_E::BEHAVIOR_MONITORING},
    /* 穿戴规范 */
    {Event::Type_E::SAFETY_HELMET, Event::SmartCategory_E::CLOTHING_COMPLIANCE},
    {Event::Type_E::REFLECTIVE_CLOTHING, Event::SmartCategory_E::CLOTHING_COMPLIANCE},
    {Event::Type_E::HIGH_ALTITUDE_SEATBELT, Event::SmartCategory_E::CLOTHING_COMPLIANCE},
    /* 交通行为监管 */
    {Event::Type_E::CONSTRUCTION_OCCUPY_ROAD, Event::SmartCategory_E::TRAFFIC_BEHAVIOR_MONITORING},
    {Event::Type_E::EMERGENCY_LANE_OCCUPANCY, Event::SmartCategory_E::TRAFFIC_BEHAVIOR_MONITORING},
    {Event::Type_E::REVERSE_DIRECTION, Event::SmartCategory_E::TRAFFIC_BEHAVIOR_MONITORING},
    {Event::Type_E::NON_MOTOR_VEHICLE_INTRUSION, Event::SmartCategory_E::TRAFFIC_BEHAVIOR_MONITORING},
    {Event::Type_E::CONGESTION, Event::SmartCategory_E::TRAFFIC_BEHAVIOR_MONITORING},
    {Event::Type_E::ILLEGAL_PARKING, Event::SmartCategory_E::TRAFFIC_BEHAVIOR_MONITORING},
    {Event::Type_E::ILLEGAL_LANE_CHANGE, Event::SmartCategory_E::TRAFFIC_BEHAVIOR_MONITORING},
    /* 属性识别 */
    {Event::Type_E::PLATE_NUMBER, Event::SmartCategory_E::ATTRIBUTE_RECOGNITION},
#endif
#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
    /* 垃圾暴露检测 */    
    {Event::Type_E::GARBAGE_EXPOSURE, Event::SmartCategory_E::BEHAVIOR_MONITORING},
    /* 垃圾满溢识别 */
    {Event::Type_E::GARBAGE_OVERFLOW, Event::SmartCategory_E::BEHAVIOR_MONITORING},
#endif
#if CAP_AI_PEOPLE_STATISTICS
    /* 人数统计 */
    {Event::Type_E::PEOPLE_FLOW_STATISTICS, Event::SmartCategory_E::PEOPLE_STATISTICS},
    {Event::Type_E::PEOPLE_DENSITY_DETECTION, Event::SmartCategory_E::PEOPLE_STATISTICS},
#endif
/*场景智能分析*/
#ifdef SCENE_INTELLIGENT_ANALYSIS
    {Event::Type_E::AI_SCENE_ANALYSIS, Event::SmartCategory_E::SCENE_ANALYSIS},
#endif
};

/* 定义不受互斥规则影响的独立事件 */
const std::set<Event::Type_E> CEventResource::m_independent_events = {
    // Event::Type_E::AUDIO_ANOMALY
};

/* 定义兼容组：key 组可以与 value 中的所有组同时激活 */
const std::map<Event::SmartCategory_E, std::set<Event::SmartCategory_E>> CEventResource::m_compatible_groups = {
#if CAP_AI_PEOPLE_STATISTICS
    {Event::SmartCategory_E::FACE_CAPTURE, {Event::SmartCategory_E::PEOPLE_STATISTICS}},
    {Event::SmartCategory_E::PEOPLE_STATISTICS, {Event::SmartCategory_E::FACE_CAPTURE}},
#endif
};

/* 定义每个组包含的所有事件 */
const std::map<Event::SmartCategory_E, std::set<Event::Type_E>> CEventResource::m_group_events = {
    {
        Event::SmartCategory_E::PERIMETER,
        {
            Event::Type_E::LINE_CROSSING,
            Event::Type_E::INTRUSION,
            Event::Type_E::ENTER_REGION,
            Event::Type_E::LEAVE_REGION,
        }
    },
    {
        Event::SmartCategory_E::BEHAVIOURAL_ANALYSIS, 
        {
            Event::Type_E::LOITERING_DETECT,
            Event::Type_E::CROWD_GATHERING,
            Event::Type_E::PARKING_DETECT,

        }
    },
    {
        Event::SmartCategory_E::SCENE_DETECTION,
        {
            Event::Type_E::AUDIO_ANOMALY,
            Event::Type_E::UNATTENDED_OBJECT,
            Event::Type_E::OBJECT_REMOVAL,
            Event::Type_E::SCENE_CHANGE,
        }
    },
    {
        Event::SmartCategory_E::TARGET_DETECTION,
        {
            Event::Type_E::FACE_DETECT,
            Event::Type_E::PET_RECOGNITION,
        }
    },
    {
        Event::SmartCategory_E::FACE_CAPTURE,
        {
            Event::Type_E::FACE_CAPTURE,
            Event::Type_E::FACE_COMPARE,
        }
    },
    {
        Event::SmartCategory_E::BEHAVIOR_MONITORING,
        {
#ifdef SCENE_INTELLIGENCE
            Event::Type_E::SLEEP_ON_DUTY,
            Event::Type_E::LEAVE_POST,
            Event::Type_E::ELECTRIC_VEHICLE_IN_ELEVATOR,
            Event::Type_E::PERSON_FALL_DOWN,
            Event::Type_E::FENCE_CLIMBING,
            Event::Type_E::SMOKING,
            Event::Type_E::PHONE_USAGE,
            Event::Type_E::SMOKE_FIRE,
            Event::Type_E::OPEN_FLAME,
            Event::Type_E::MANHOLE_COVER_ABNORMAL,
            Event::Type_E::BARE_SOIL,
            Event::Type_E::HOLE_PROTECTION_BAR,
            Event::Type_E::PEDESTRIAN_INTRUSION,
            Event::Type_E::PERSON_TRIP,
#endif
#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
            Event::Type_E::GARBAGE_EXPOSURE,
            Event::Type_E::GARBAGE_OVERFLOW,
#endif
        }
    },
#ifdef SCENE_INTELLIGENCE
    {
        Event::SmartCategory_E::CLOTHING_COMPLIANCE,
        {
            Event::Type_E::SAFETY_HELMET,
            Event::Type_E::REFLECTIVE_CLOTHING,
            Event::Type_E::HIGH_ALTITUDE_SEATBELT,
        }
    },
    {
        Event::SmartCategory_E::TRAFFIC_BEHAVIOR_MONITORING,
        {
            Event::Type_E::CONSTRUCTION_OCCUPY_ROAD,
            Event::Type_E::EMERGENCY_LANE_OCCUPANCY,
            Event::Type_E::REVERSE_DIRECTION,
            Event::Type_E::NON_MOTOR_VEHICLE_INTRUSION,
            Event::Type_E::ROAD_PONDING,
            Event::Type_E::CONGESTION,
            Event::Type_E::ILLEGAL_PARKING,
            Event::Type_E::ILLEGAL_LANE_CHANGE,
        }
    },
    {
        Event::SmartCategory_E::ATTRIBUTE_RECOGNITION,
        {
            Event::Type_E::PLATE_NUMBER,
        }
    },
#endif
#if CAP_AI_PEOPLE_STATISTICS
    {
        Event::SmartCategory_E::PEOPLE_STATISTICS,
        {
            Event::Type_E::PEOPLE_FLOW_STATISTICS,
            Event::Type_E::PEOPLE_DENSITY_DETECTION,
        }
    },
#endif
#ifdef SCENE_INTELLIGENT_ANALYSIS
    {
        Event::SmartCategory_E::SCENE_ANALYSIS,
        {
            Event::Type_E::AI_SCENE_ANALYSIS,
        }
    },
#endif
};

CEventResource::CEventResource()
{
}

CEventResource::~CEventResource()
{
}

int CEventResource::get_canEventResource_rules(const Event::SmartEventEnableStatus_S &stStatus, std::vector<Event::Type_E> &aCanEnableEvent)
{
    aCanEnableEvent.clear();
#if   0
    for (const auto& pair : m_event_to_status_map) 
    {
        aCanEnableEvent.push_back(pair.first);
    }

    return 0;

#endif
    std::set<Event::Type_E> enabled_events;
    std::set<Event::SmartCategory_E> active_groups;

    /* 遍历当前状态，找出所有已启用的事件，并确定当前激活的所有互斥组 */
    for (const auto &pair : m_event_to_status_map)
    {
        Event::Type_E event_type = pair.first;
        BoolMemberPtr member_ptr = pair.second;
        if (stStatus.*member_ptr)
        {
            enabled_events.insert(event_type);
            /* 如果这个事件属于一个互斥组，记录下激活的组 */
            auto it = m_event_to_group_map.find(event_type);
            if (it != m_event_to_group_map.end())
            {
                active_groups.insert(it->second);
            }
        }
    }

    std::set<Event::Type_E> can_enable_set;

    /* 根据激活的互斥组，确定哪些事件可以被启用 */
    if (active_groups.empty())
    {
        /* 如果没有任何互斥组被激活，说明所有事件组都可以被选择 */
        /* 因此，所有互斥组内的事件都可以启用 */
        for (const auto &group_pair : m_group_events)
        {
            can_enable_set.insert(group_pair.second.begin(), group_pair.second.end());
        }
    }
    else
    {
        /* 如果某个互斥组已被激活，则只有该组及其兼容组内的事件可以继续启用 */
        for (const auto &active_group : active_groups)
        {
            /* 添加当前激活组的所有事件 */
            auto group_it = m_group_events.find(active_group);
            if (group_it != m_group_events.end())
            {
                can_enable_set.insert(group_it->second.begin(), group_it->second.end());
            }

            /* 添加兼容组的所有事件 */
            auto compat_it = m_compatible_groups.find(active_group);
            if (compat_it != m_compatible_groups.end())
            {
                for (const auto &compat_group : compat_it->second)
                {
                    auto compat_group_it = m_group_events.find(compat_group);
                    if (compat_group_it != m_group_events.end())
                    {
                        can_enable_set.insert(compat_group_it->second.begin(), compat_group_it->second.end());
                    }
                }
            }
        }

        /* 特殊规则：示例：假设目标检测组内，人脸侦测和宠物识别互斥 */
        if (active_groups.count(Event::SmartCategory_E::TARGET_DETECTION))
        {
            /* 如果已启用一个，另一个就不能再启用了(它们都在enabled_events里，之后会被过滤掉) */
            /* 如果想更明确，可以这样： */
            // if (enabled_events.count(Event::Type_E::FACE_DETECT))
            // {
            //     can_enable_set.erase(Event::Type_E::PET_RECOGNITION);
            // }
            // if (enabled_events.count(Event::Type_E::PET_RECOGNITION))
            // {
            //     can_enable_set.erase(Event::Type_E::FACE_DETECT);
            // }
        }
    }

    /* 总是可以启用独立事件 */
    can_enable_set.insert(m_independent_events.begin(), m_independent_events.end());

    /* 从“可以启用”的集合中，移除“已经启用”的事件 */
    for (Event::Type_E enabled_event : enabled_events)
    {
        can_enable_set.erase(enabled_event);
    }

    /* 将最终结果（set）转换为输出的vector<int> */
    for (Event::Type_E event_type : can_enable_set)
    {
        aCanEnableEvent.push_back(event_type);
    }

    return OK;
}

int CEventResource::enableStatus_convertArray(const Event::SmartEventEnableStatus_S &stStatus, std::vector<Event::Type_E> &aEventType)
{
    aEventType.clear();

    std::set<Event::Type_E> aEnabledEvents;

    /* 遍历当前状态，找出所有已启用的事件，并确定当前激活的互斥组 */
    for (const auto &pair : m_event_to_status_map)
    {
        Event::Type_E event_type = pair.first;
        BoolMemberPtr member_ptr = pair.second;
        if (stStatus.*member_ptr)
        {
            aEnabledEvents.insert(event_type);
        }
    }

    /* 将最终结果（set）转换为输出的vector<int> */
    for (Event::Type_E event_type : aEnabledEvents)
    {
        aEventType.push_back(event_type);
    }

    return OK;
}

/**
 * @brief   : 根据智能事件启用状态的变化，自动禁用相关事件的配置
 * @note    : 当一个智能事件从启用变为禁用时，此函数会查找该事件的具体配置，并将其中的 bEnable 标志设为
 * false，同时更新总的算法配置并通知AI模块
 * @param    {Event::SmartEventEnableStatus_S} &oldStatus：变更前的启用状态
 * @param    {Event::SmartEventEnableStatus_S} &newStatus：变更后的启用状态
 * @return   {void}
 */
void CEventResource::update_event_configurations_on_disable(const Event::SmartEventEnableStatus_S &oldStatus,
                                                            const Event::SmartEventEnableStatus_S &newStatus)
{
    std::vector<Event::Type_E> disabledEvents;

    /* 遍历所有智能事件，找出从“启用”变为“禁用”的事件 */
    for (const auto &pair : m_event_to_status_map)
    {
        Event::Type_E event_type = pair.first;
        BoolMemberPtr member_ptr = pair.second;
        if ((oldStatus.*member_ptr) && !(newStatus.*member_ptr))
        {
            disabledEvents.push_back(event_type);
        }
    }

    if (disabledEvents.empty())
    {
        return; /* 没有状态变更 */
    }

    /* 遍历所有被禁用的事件，逐一更新它们的具体配置和布防计划配置 */
    for (const auto event_type : disabledEvents)
    {
        dlog_info("智能事件 %d 已通过资源分配被禁用，正在更新其相关配置...", static_cast<int>(event_type));

        /* 使用模板函数禁用事件自身的配置 (bEnable = false) */
        switch (event_type)
        {
        /* 周界事件 */
        case Event::Type_E::LINE_CROSSING:      disable_specific_config<Alarm::BoundaryDetection_S>(); break;
        case Event::Type_E::INTRUSION:          disable_specific_config<Alarm::FieldDetection_S>();    break;
        case Event::Type_E::ENTER_REGION:       disable_specific_config<Alarm::EntranceDetection_S>(); break;
        case Event::Type_E::LEAVE_REGION:       disable_specific_config<Alarm::ExitingDetection_S>();  break;
        /* 行为分析 */
        case Event::Type_E::LOITERING_DETECT:   disable_specific_config<Alarm::LoiteringDetection_S>(); break;
        case Event::Type_E::CROWD_GATHERING:    disable_specific_config<Alarm::CrowdGathering_S>();     break;
        case Event::Type_E::PARKING_DETECT:     disable_specific_config<Alarm::ParkingDetection_S>();   break;
        /* 场景检测 */
        case Event::Type_E::AUDIO_ANOMALY:      disable_specific_config<Alarm::AudioAnomaly_S>();      break;
        case Event::Type_E::SCENE_CHANGE:       disable_specific_config<Alarm::SceneChange_S>();       break;
        case Event::Type_E::UNATTENDED_OBJECT:  disable_specific_config<Alarm::UnattendedObject_S>();  break;
        case Event::Type_E::OBJECT_REMOVAL:     disable_specific_config<Alarm::ObjectRemoval_S>();     break;
        /* 目标检测 */
        case Event::Type_E::FACE_DETECT:        disable_specific_config<Alarm::FaceDetection_S>();     break;
        case Event::Type_E::PET_RECOGNITION:    disable_specific_config<Alarm::PetRecognition_S>();    break;
        /* 人脸抓拍 */
        case Event::Type_E::FACE_CAPTURE:       disable_specific_config<Alarm::FaceCapture_S>();       break;
        /*人脸比对*/
        case Event::Type_E::FACE_COMPARE:       disable_specific_config<Alarm::FaceCompare_S>();       break;
#ifdef SCENE_INTELLIGENCE
        /* 行为监管 */
        case Event::Type_E::SLEEP_ON_DUTY:                  disable_specific_config<Alarm::SleepOnDutyDetection_S>();      break;
        case Event::Type_E::LEAVE_POST:                     disable_specific_config<Alarm::LeavePostDetection_S>();      break;
        case Event::Type_E::ELECTRIC_VEHICLE_IN_ELEVATOR:   disable_specific_config<Alarm::AudioAnomaly_S>();      break;
        case Event::Type_E::PERSON_FALL_DOWN:               disable_specific_config<Alarm::PersonFallDownDetection_S>();      break;
        case Event::Type_E::FENCE_CLIMBING:                 disable_specific_config<Alarm::FenceClimbingDetection_S>();      break;
        case Event::Type_E::SMOKING:                        disable_specific_config<Alarm::SmokingDection_S>();      break;
        case Event::Type_E::PHONE_USAGE:                    disable_specific_config<Alarm::PhoneUsageDetection_S>();      break;
        case Event::Type_E::SMOKE_FIRE:                     disable_specific_config<Alarm::SmokeFireDetection_S>();      break;
        case Event::Type_E::OPEN_FLAME:                     disable_specific_config<Alarm::OpenFlameDetection_S>();      break;
        case Event::Type_E::MANHOLE_COVER_ABNORMAL:         disable_specific_config<Alarm::ManholeCoverAbnormalDetection_S>();      break;
        case Event::Type_E::BARE_SOIL:                      disable_specific_config<Alarm::BareSoiletDection_S>();      break;
        case Event::Type_E::HOLE_PROTECTION_BAR:            disable_specific_config<Alarm::HoleProtectionBarDection_S>();      break;
        case Event::Type_E::PEDESTRIAN_INTRUSION:           disable_specific_config<Alarm::PedestrianIntrusionDetection_S>();      break;
        case Event::Type_E::PERSON_TRIP:                    disable_specific_config<Alarm::TripDetection_S>();      break;
        /* 穿戴规范 */
        case Event::Type_E::SAFETY_HELMET:                  disable_specific_config<Alarm::SafetyHelmetDection_S>();      break;
        case Event::Type_E::REFLECTIVE_CLOTHING:            disable_specific_config<Alarm::ReflectiveClothingDection_S>();      break;
        case Event::Type_E::HIGH_ALTITUDE_SEATBELT:         disable_specific_config<Alarm::HighAltitudeSeatbeltDetection_S>();      break;
        /* 交通行为监管 */
        case Event::Type_E::CONSTRUCTION_OCCUPY_ROAD:       disable_specific_config<Alarm::ConstructionEncroachmentRoadDetection_S>();      break;
        case Event::Type_E::EMERGENCY_LANE_OCCUPANCY:       disable_specific_config<Alarm::EmergencyLaneOccupancyDetection_S>();      break;
        case Event::Type_E::REVERSE_DIRECTION:              disable_specific_config<Alarm::DrivingAgainstTrafficDetection_S>();      break;
        case Event::Type_E::NON_MOTOR_VEHICLE_INTRUSION:    disable_specific_config<Alarm::NonMotorVehicleIntrusionDetection_S>();      break;
        case Event::Type_E::ROAD_PONDING:                   disable_specific_config<Alarm::RoadPondingDetection_S>();      break;
        case Event::Type_E::CONGESTION:                     disable_specific_config<Alarm::CongestionDetection_S>();      break;
        // case Event::Type_E::ILLEGAL_PARKING:                disable_specific_config<Alarm::AudioAnomaly_S>();      break;
        case Event::Type_E::ILLEGAL_LANE_CHANGE:            disable_specific_config<Alarm::IllegalLaneChangeDetection_S>();      break;
        /* 属性识别 */
        case Event::Type_E::PLATE_NUMBER:                   disable_specific_config<Alarm::LicensePlateCognitionDetection_S>();      break;
#endif
#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
        case Event::Type_E::GARBAGE_EXPOSURE:               disable_specific_config<Alarm::GarbageExposureDetection_S>();      break;
        case Event::Type_E::GARBAGE_OVERFLOW:               disable_specific_config<Alarm::GarbageOverflowDetection_S>();      break;
#endif
#if CAP_AI_PEOPLE_STATISTICS
        /* 人数统计 */
        case Event::Type_E::PEOPLE_FLOW_STATISTICS:         disable_specific_config<Alarm::PeopleFlowStatistics_S>();    break;
        case Event::Type_E::PEOPLE_DENSITY_DETECTION:       disable_specific_config<Alarm::PeopleDensityDetection_S>();  break;
#endif
/*场景智能分析*/
#ifdef SCENE_INTELLIGENT_ANALYSIS
        case Event::Type_E::AI_SCENE_ANALYSIS:                
                                                            disable_specific_config<Alarm::LLMAISceneAnalysis_S>();                break;
#endif
        default:
            dlog_warn("在配置更新中遇到未处理的智能事件类型: %d", static_cast<int>(event_type));
            continue; /* 跳过当前事件，继续处理下一个 */
        }

        /* 同步更新该事件的布防计划 (bStatus = false) */
        Alarm::EventSchedule_S stEventSchedule;
        stEventSchedule.enEventType = event_type;

        /* 更新场景智能分析的定时分析任务计划*/
#ifdef SCENE_INTELLIGENT_ANALYSIS   
        std::vector<Event::Type_E> types;
        if (event_type == Event::Type_E::AI_SCENE_ANALYSIS)
        {
            types = { Event::Type_E::AI_SCENE_ANALYSIS, Event::Type_E::IMAGE_ANALYSIS };
        }
        else
        {
            types = { event_type };
        }

        for (auto type : types)
        {
            stEventSchedule.enEventType = type;
            if (CEventConfigure::instance()->get_configure(stEventSchedule) == 0 && stEventSchedule.bStatus)
            {
                stEventSchedule.bStatus = false;
                CEventConfigure::instance()->set_configure(stEventSchedule);
            }
        }   
#else
        if (CEventConfigure::instance()->get_configure(stEventSchedule) == 0 && stEventSchedule.bStatus)
        {
            stEventSchedule.bStatus = false;
            CEventConfigure::instance()->set_configure(stEventSchedule);
        }
#endif
    }
    /* 在所有配置更新完毕后，统一通知 EventManage 重新计算和应用算法状态 */
    dlog_info("智能事件配置已更新，正在触发事件管理器重新评估算法状态...");
    CEventManage::instance()->update_event_schedule();
}

/**
 * @brief   : [模板辅助函数] 禁用指定类型的事件配置
 * @tparam   {ConfigType} 事件具体的配置结构体类型 (e.g., Alarm::BoundaryDetection_S)
 */
template <typename ConfigType>
void CEventResource::disable_specific_config()
{
    ConfigType stConfig;
    if (CEventConfigure::instance()->get_configure(stConfig) == 0 && stConfig.bEnable)
    {
        stConfig.bEnable = false;
        CEventConfigure::instance()->set_configure(stConfig);
    }
}


#ifdef SCENE_INTELLIGENT_ANALYSIS

/**
 * @brief   : 根据智能事件启用状态的变化，自动开启相关事件的配置(现只用于大模型场景智能分析)
 * @note    : 当一个智能事件从禁用变为启用时，此函数会查找该事件的具体配置，并将其中的 bEnable 标志设为
 * true，同时更新总的算法配置并通知AI模块
 * @param    {Event::SmartEventEnableStatus_S} &oldStatus：变更前的启用状态
 * @param    {Event::SmartEventEnableStatus_S} &newStatus：变更后的启用状态
 * @return   {void}
 */
void CEventResource::update_event_configurations_on_enable(const Event::SmartEventEnableStatus_S &oldStatus,
                                                            const Event::SmartEventEnableStatus_S &newStatus)
{
    auto it = m_event_to_status_map.find(Event::Type_E::AI_SCENE_ANALYSIS);
    if (it != m_event_to_status_map.end())
    {
        const auto &member_ptr = it->second;
        Event::Type_E event_type = it->first;

        // 检查事件状态从禁用变为启用
        if (!(oldStatus.*member_ptr) && (newStatus.*member_ptr))
        {
            Alarm::LLMAISceneAnalysis_S stConfig;
            if (CEventConfigure::instance()->get_configure(stConfig) == 0 && !stConfig.bEnable)
            {
                stConfig.bEnable = true;
                CEventConfigure::instance()->set_configure(stConfig);
            }

             /* 同步更新该事件的布防计划 (bStatus = true) */
            Alarm::EventSchedule_S stEventSchedule;
            /* 更新场景智能分析的定时分析任务计划*/
            std::vector<Event::Type_E> types;
            if (event_type == Event::Type_E::AI_SCENE_ANALYSIS)
            {
                types = { Event::Type_E::AI_SCENE_ANALYSIS, Event::Type_E::IMAGE_ANALYSIS };
            }
            else
            {
                types = { event_type };
            }

            for (auto type : types)
            {
                stEventSchedule.enEventType = type;
                int ret = CEventConfigure::instance()->get_configure(stEventSchedule);
                if (ret == 0 && !stEventSchedule.bStatus)
                {
                    stEventSchedule.bStatus = true;
                    CEventConfigure::instance()->set_configure(stEventSchedule);
                }
                else if(ret != 0)
                {
                    stEventSchedule.enEventType = event_type;
                    stEventSchedule.bStatus = true;
                    CEventConfigure::instance()->set_configure(stEventSchedule);
                }
            }   

            dlog_info("场景智能分析配置已更新，正在执行重启...");
            sleep(1);
            /* 静音，防止重启喇叭异响 */
#if CAP_SYSTEM_REBOOT_MUTE // 系统重启静音处理
            CStreamAo::instance()->update_audioOutputType(Audio_NS::AudioOutputType_E::MUTE);
#endif
            // 执行重启
            system("sync");
            system("reboot");
            
            // dlog_info("场景智能分析配置已更新，正在触发事件管理器重新评估算法状态...");
            // CEventManage::instance()->update_event_schedule();
        }
    }
}

#endif
