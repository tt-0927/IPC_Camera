/**
 * @FilePath     : event_linkage_resolver.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-15 16:29:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-20 14:42:14
 * @Description  : 事件联动规则解析器实现
 */

#include "event_linkage_resolver.h"

#include <algorithm>

#include "convert_interface.h"
#include "event_configure.h"
#include "path_define.h"
#include "dlog.h"
#include "IpcRet.h"

int EventLinkageResolver::resolve(const EventTriggerContext_S &stContext,
                                  const Event::Info_S &stEventInfo,
                                  ResolvedLinkagePlan_S &stPlan)
{
    /* 先写入输入快照，保证后续无论命中哪条规则都能拿到完整上下文 */
    stPlan = ResolvedLinkagePlan_S();
    stPlan.stContext = stContext;
    stPlan.stEventInfo = stEventInfo;

    /**
     * 默认联动是事件框架的主路径：
     * 1. 先按事件类型装载代码内建的默认联动配置；
     * 2. 再尝试读取扩展策略文件做高级覆盖；
     * 3. 若扩展策略未命中，则继续沿用默认联动。
     */
    if (load_default_linkage_list(stContext.enEventType, stPlan.stLinkageList) != OK)
    {
        /* 默认联动缺失时，说明当前事件还没有接入主链路联动能力 */
        dlog_warn("未处理事件联动，事件类型: %d", static_cast<int>(stContext.enEventType));
        return ERR;
    }

    /* 扩展联动策略属于高级可选能力，仅在命中规则时覆盖默认联动 */
    // note 暂无任一事件需要扩展联动
    LinkagePolicySet_S stPolicySet;
    load_policy_set(stPolicySet);
    const LinkageRule_S *pstBestRule = find_best_rule(stPolicySet, stContext);
    if (pstBestRule)
    {
        apply_extended_rule(*pstBestRule, stPlan);
    }

    /* 补齐算法总开关，再把联动列表整理为便于调度的布尔标志 */
    CEventConfigure::instance()->get_configure(stPlan.stAlgorithmConfig);
    fill_plan_flags(stPlan);
    return OK;
}

const LinkageRule_S *EventLinkageResolver::find_best_rule(const LinkagePolicySet_S &stPolicySet,
                                                          const EventTriggerContext_S &stContext) const
{
    const LinkageRule_S *pstBestRule = nullptr;
    for (size_t i = 0; i < stPolicySet.vecRules.size(); ++i)
    {
        const auto &stRule = stPolicySet.vecRules[i];
        if (!match_rule(stRule, stContext))
        {
            continue;
        }

        if (!pstBestRule)
        {
            pstBestRule = &stRule;
            continue;
        }

        if (stRule.nPriority < pstBestRule->nPriority)
        {
            pstBestRule = &stRule;
            continue;
        }

        if (stRule.nPriority == pstBestRule->nPriority)
        {
            /* 同优先级下，优先选择属性条件更具体的规则 */
            if (stRule.vecAttrMatches.size() > pstBestRule->vecAttrMatches.size())
            {
                pstBestRule = &stRule;
                continue;
            }

            if (stRule.vecAttrMatches.size() == pstBestRule->vecAttrMatches.size())
            {
                /* 属性条件同样具体时，明确区分开始/结束阶段的规则优先 */
                const bool bCurrentExactPhase = stRule.nPhase != -1;
                const bool bBestExactPhase = pstBestRule->nPhase != -1;
                if (bCurrentExactPhase && !bBestExactPhase)
                {
                    pstBestRule = &stRule;
                    continue;
                }
            }
        }
    }

    return pstBestRule;
}

void EventLinkageResolver::apply_extended_rule(const LinkageRule_S &stRule, ResolvedLinkagePlan_S &stPlan) const
{
    /* 命中扩展规则时，以规则结果覆盖默认联动，用于项目级特殊定制 */
    stPlan.bUseExtendedRule = true;
    stPlan.nMatchedRuleId = stRule.nRuleId;
    stPlan.stLinkageList = stRule.stLinkageList;
}

int EventLinkageResolver::load_default_linkage_list(Event::Type_E enEventType, Alarm::LinkageList_S &stLinkageList)
{
    /* 按事件类型回退到现有告警配置，保持与旧链路行为一致 */
    switch (enEventType)
    {
    case Event::Type_E::MOTION_DETECT:
    {
        Alarm::MotionDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::OCCLUSION_DETECT:
    {
        Alarm::HideAlarm_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::ANOMALY_ALARM:
    {
        Alarm::AbnormalDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::DISK_FULL:
    case Event::Type_E::DISK_ERROR:
    case Event::Type_E::NET_BROKEN:
    case Event::Type_E::IP_CONFLICT:
    case Event::Type_E::ILLEGAL_ACCESS:
    {
        std::set<Alarm::AbnormalDetection_S> alarm;
        CEventConfigure::instance()->get_configure(alarm);
        /* 多种异常共用一份配置集合，先换算成异常子类型再查找 */
        Alarm::AbnormalType_E enAbnormalType = Alarm::AbnormalType_E::DISK_FULL;
        switch (enEventType)
        {
        case Event::Type_E::DISK_FULL:
            enAbnormalType = Alarm::AbnormalType_E::DISK_FULL;
            break;
        case Event::Type_E::DISK_ERROR:
            enAbnormalType = Alarm::AbnormalType_E::DISK_ERROR;
            break;
        case Event::Type_E::NET_BROKEN:
            enAbnormalType = Alarm::AbnormalType_E::NET_BROKEN;
            break;
        case Event::Type_E::IP_CONFLICT:
            enAbnormalType = Alarm::AbnormalType_E::IP_CONFLICT;
            break;
        case Event::Type_E::ILLEGAL_ACCESS:
            enAbnormalType = Alarm::AbnormalType_E::ILLEGAL_ACCESS;
            break;
        default:
            break;
        }

        for (const auto &item : alarm)
        {
            if (item.enAbnormalType == enAbnormalType)
            {
                stLinkageList = item.stLinkageList;
                break;
            }
        }
        break;
    }
    case Event::Type_E::ALARM_INPUT:
    {
        Alarm::IoInputInfo_S alarm;
        alarm.nIoNumer = 0;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::PIR_ALARM:
    {
        Alarm::PirAlarmInfo_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::MANUAL_SOUND_LIGHT_ALARM:
    {
        if (CEventConfigure::instance()->get_configure(stLinkageList) != OK)
        {
            return ERR;
        }
        break;
    }
    case Event::Type_E::LINE_CROSSING:
    {
        Alarm::BoundaryDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::INTRUSION:
    {
        Alarm::FieldDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::ENTER_REGION:
    {
        Alarm::EntranceDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::LEAVE_REGION:
    {
        Alarm::ExitingDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::AUDIO_ANOMALY:
    case Event::Type_E::AUDIO_SUDDEN_RISE:
    case Event::Type_E::AUDIO_SUDDEN_DROP:
    {
        Alarm::AudioAnomaly_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::SCENE_CHANGE:
    {
        Alarm::SceneChange_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::FACE_DETECT:
    {
        Alarm::FaceDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::LOITERING_DETECT:
    {
        Alarm::LoiteringDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::CROWD_GATHERING:
    {
        Alarm::CrowdGathering_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::PARKING_DETECT:
    {
        Alarm::ParkingDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::UNATTENDED_OBJECT:
    {
        Alarm::UnattendedObject_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::OBJECT_REMOVAL:
    {
        Alarm::ObjectRemoval_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::PET_RECOGNITION:
    {
        Alarm::PetRecognition_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::FACE_CAPTURE:
    {
        Alarm::FaceCapture_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::FACE_COMPARE_SUCCESS:
    {
        Alarm::FaceCompare_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageListSuccess;
        break;
    }
    case Event::Type_E::FACE_COMPARE_FAIL:
    {
        Alarm::FaceCompare_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageListFail;
        break;
    }
#ifdef SCENE_INTELLIGENT_ANALYSIS
    case Event::Type_E::TEXT_PRESET:
    {
        Alarm::TextPresetTaskManager_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        for (const auto &task : alarm.aTaskConfig)
        {
            /* 文本预设任务只取当前启用任务对应的联动配置 */
            if (task.strTaskId == alarm.strCurrentActiveTaskId)
            {
                stLinkageList = task.stLinkageList;
                break;
            }
        }
        break;
    }
#endif
#ifdef SCENE_INTELLIGENCE
    case Event::Type_E::SLEEP_ON_DUTY:
    {
        Alarm::SleepOnDutyDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::LEAVE_POST:
    {
        Alarm::LeavePostDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::ELECTRIC_VEHICLE_IN_ELEVATOR:
    {
        Alarm::ElectricScooterDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::PERSON_FALL_DOWN:
    {
        Alarm::PersonFallDownDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::FENCE_CLIMBING:
    {
        Alarm::FenceClimbingDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::SMOKING:
    {
        Alarm::SmokingDection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::PHONE_USAGE:
    {
        Alarm::PhoneUsageDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::SMOKE_FIRE:
    {
        Alarm::SmokeFireDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::OPEN_FLAME:
    {
        Alarm::OpenFlameDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::MANHOLE_COVER_ABNORMAL:
    {
        Alarm::ManholeCoverAbnormalDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::BARE_SOIL:
    {
        Alarm::BareSoiletDection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::HOLE_PROTECTION_BAR:
    {
        Alarm::HoleProtectionBarDection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::PEDESTRIAN_INTRUSION:
    {
        Alarm::PedestrianIntrusionDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::PERSON_TRIP:
    {
        Alarm::TripDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::SAFETY_HELMET:
    {
        Alarm::SafetyHelmetDection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::REFLECTIVE_CLOTHING:
    {
        Alarm::ReflectiveClothingDection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::HIGH_ALTITUDE_SEATBELT:
    {
        Alarm::HighAltitudeSeatbeltDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::CONSTRUCTION_OCCUPY_ROAD:
    {
        Alarm::ConstructionEncroachmentRoadDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::EMERGENCY_LANE_OCCUPANCY:
    {
        Alarm::EmergencyLaneOccupancyDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::REVERSE_DIRECTION:
    {
        Alarm::DrivingAgainstTrafficDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::NON_MOTOR_VEHICLE_INTRUSION:
    {
        Alarm::NonMotorVehicleIntrusionDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::ROAD_PONDING:
    {
        Alarm::RoadPondingDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::CONGESTION:
    {
        Alarm::CongestionDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::ILLEGAL_PARKING:
    {
        Alarm::ParkingDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::ILLEGAL_LANE_CHANGE:
    {
        Alarm::IllegalLaneChangeDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::PLATE_NUMBER:
    {
        Alarm::LicensePlateCognitionDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
#endif
#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
    case Event::Type_E::GARBAGE_EXPOSURE:
    {
        Alarm::GarbageExposureDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
    case Event::Type_E::GARBAGE_OVERFLOW:
    {
        Alarm::GarbageOverflowDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stLinkageList;
        break;
    }
#endif
#if CAP_AI_PEOPLE_STATISTICS
    case Event::Type_E::PEOPLE_FLOW_STATISTICS:
    case Event::Type_E::PEOPLE_DENSITY_DETECTION:
    {
        /* 人数统计主事件不直接配置总联动，保留空联动并视为已接入 */
        return OK;
    }
    case Event::Type_E::PEOPLE_FLOW_STAY_NORMAL:
    {
        Alarm::PeopleFlowStatistics_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stStayAlarm.stNormal.stLinkageList;
        break;
    }
    case Event::Type_E::PEOPLE_FLOW_STAY_MEDIUM:
    {
        Alarm::PeopleFlowStatistics_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stStayAlarm.stMedium.stLinkageList;
        break;
    }
    case Event::Type_E::PEOPLE_FLOW_STAY_SEVERE:
    {
        Alarm::PeopleFlowStatistics_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stStayAlarm.stSevere.stLinkageList;
        break;
    }
    case Event::Type_E::PEOPLE_DENSITY_NORMAL:
    {
        Alarm::PeopleDensityDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stDensityAlarm.stNormal.stLinkageList;
        break;
    }
    case Event::Type_E::PEOPLE_DENSITY_MEDIUM:
    {
        Alarm::PeopleDensityDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stDensityAlarm.stMedium.stLinkageList;
        break;
    }
    case Event::Type_E::PEOPLE_DENSITY_SEVERE:
    {
        Alarm::PeopleDensityDetection_S alarm;
        CEventConfigure::instance()->get_configure(alarm);
        stLinkageList = alarm.stDensityAlarm.stSevere.stLinkageList;
        break;
    }
#endif
    default:
        return ERR;
    }

    return OK;
}

int EventLinkageResolver::load_policy_set(LinkagePolicySet_S &stPolicySet) const
{
    std::lock_guard<std::mutex> lock(m_policyMutex);
    if (!m_bPolicyLoaded)
    {
        /* 首次访问时读取配置文件，后续直接复用缓存避免重复解析 */
        m_cachedPolicySet = LinkagePolicySet_S();
        if (Convert::read_file(EVENT_LINKAGE_POLICY_CONFIG_FILE, m_cachedPolicySet) != OK)
        {
            dlog_info("事件联动扩展策略文件不存在或读取失败，回退默认联动");
        }
        m_bPolicyLoaded = true;
    }

    stPolicySet = m_cachedPolicySet;
    return OK;
}

bool EventLinkageResolver::match_rule(const LinkageRule_S &stRule, const EventTriggerContext_S &stContext) const
{
    /* 规则先按事件类型过滤，这是最基本的匹配条件 */
    if (stRule.enEventType != stContext.enEventType)
    {
        return false;
    }

    /* 通道号为-1表示匹配任意通道，否则必须与当前事件一致 */
    if (stRule.nChnId != -1 && stRule.nChnId != stContext.nChnId)
    {
        return false;
    }

    /* 事件阶段使用0/1表示，便于与配置文件中的整数字段对齐 */
    const int nPhase = stContext.bEventEnded ? 0 : 1;
    if (stRule.nPhase != -1 && stRule.nPhase != nPhase)
    {
        return false;
    }

    /* 只要有一个属性条件不满足，整条规则就视为不命中 */
    for (const auto &stAttrMatch : stRule.vecAttrMatches)
    {
        if (!match_attr(stAttrMatch, stContext))
        {
            return false;
        }
    }

    return true;
}

bool EventLinkageResolver::match_attr(const LinkageAttrMatch_S &stAttrMatch, const EventTriggerContext_S &stContext) const
{
    if (stAttrMatch.enOp == LinkageMatchOp_E::ANY)
    {
        /* ANY表示仅要求键存在与否无关，直接视为匹配成功 */
        return true;
    }

    /* 需要比较值的规则，先确认事件上下文里已经携带该属性 */
    auto it = stContext.mapAttrs.find(stAttrMatch.strKey);
    if (it == stContext.mapAttrs.end())
    {
        return false;
    }

    if (stAttrMatch.enOp == LinkageMatchOp_E::EQ)
    {
        /* EQ只取首个候选值，与当前属性做精确比较 */
        return !stAttrMatch.vecValues.empty() && it->second == stAttrMatch.vecValues.front();
    }

    if (stAttrMatch.enOp == LinkageMatchOp_E::IN)
    {
        /* IN用于判断当前属性值是否位于候选集合中 */
        return std::find(stAttrMatch.vecValues.begin(), stAttrMatch.vecValues.end(), it->second) != stAttrMatch.vecValues.end();
    }

    return false;
}

void EventLinkageResolver::fill_plan_flags(ResolvedLinkagePlan_S &stPlan) const
{
    /* 把联动列表转换成显式布尔标志，便于调度层直接判断是否派发 */
    stPlan.bUploadSdCard = linkage_list_contains(stPlan.stLinkageList, Alarm::LinkageType_E::UPLOAD_SD_CARD);
    stPlan.bSendEmail = linkage_list_contains(stPlan.stLinkageList, Alarm::LinkageType_E::SEND_EMAIL) &&
                        /* 人脸抓拍邮件走独立链路，这里不重复派发 */
                        stPlan.stContext.enEventType != Event::Type_E::FACE_CAPTURE;
    stPlan.bSound = linkage_list_contains(stPlan.stLinkageList, Alarm::LinkageType_E::SOUND);
    stPlan.bFlashingLightAlarm = linkage_list_contains(stPlan.stLinkageList, Alarm::LinkageType_E::FLASHING_LIGHT_ALARM);
    stPlan.bUploadToCenter = linkage_list_contains(stPlan.stLinkageList, Alarm::LinkageType_E::UPLOAD_TOCENTER);
}
