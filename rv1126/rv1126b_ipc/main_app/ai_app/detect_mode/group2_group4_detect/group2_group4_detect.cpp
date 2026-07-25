/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-09 10:47:39
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-06-03 09:37:42
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/detect_mode/group2_group4_detect/group2_group4_detect.hpp
 * @Description: 人、车、非/模型组合4事件相关
 */
#include "group2_group4_detect.hpp"
#include "common_process.h"
#include "StatisticsTimer.hpp"
#include "alarm_define.h"
#include "storage_manage.h"
#include "task_publish.h"
#include "capture_database.h"

#include <algorithm>

#ifdef ENABLE_GAT1400_SRC
#include "gat1400.h"
#endif

/* 数据队列 */
#define QUEUE_MAX              (2)
// 检测帧数阈值
#define DETECT_FRAME_THRESHOLD (6)

static const char *VehicleBrandToString(VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleBrand brand);

namespace {
#if CAP_EXHIBITION_OSD_PANEL
/**
 * @brief   : 展会面板单个人员事件统计结果
 * @return   {struct} 单个人员的玩手机/抽烟命中结果
 */
struct Group4PersonPanel_S
{
    /* 当前命中事件的父级人员框。 */
    Common::RectInfo_S stPersonRect;
    /* 当前人员命中的最高事件置信度。 */
    float fConfidence = 0.0f;
};

/**
 * @brief   : 将 Group2 的人员检测结果转换为普通矩形框
 * @param    {const Group2Detect_NS::Result_S &} stResult：Group2 人员结果
 * @return   {Common::RectInfo_S} 转换后的矩形框
 */
Common::RectInfo_S to_person_rect(const Group2Detect_NS::Result_S &stResult)
{
    /* 转换后的矩形框结果。 */
    Common::RectInfo_S stRectInfo;
    stRectInfo.nX1 = static_cast<int>(stResult.fX1);
    stRectInfo.nY1 = static_cast<int>(stResult.fY1);
    stRectInfo.nX2 = static_cast<int>(stResult.fX2);
    stRectInfo.nY2 = static_cast<int>(stResult.fY2);
    return stRectInfo;
}

/**
 * @brief   : 判断矩形框是否有效
 * @param    {const Common::RectInfo_S &} stRectInfo：待判断矩形框
 * @return   {bool} true：有效 false：无效
 */
bool is_valid_rect(const Common::RectInfo_S &stRectInfo)
{
    return stRectInfo.nX1 < stRectInfo.nX2 && stRectInfo.nY1 < stRectInfo.nY2;
}

/**
 * @brief   : 合并两个矩形框，生成能覆盖二者的外接矩形
 * @param    {const Common::RectInfo_S &} stLeft：左侧矩形
 * @param    {const Common::RectInfo_S &} stRight：右侧矩形
 * @return   {Common::RectInfo_S} 合并后的外接矩形
 */
Common::RectInfo_S merge_rect(const Common::RectInfo_S &stLeft, const Common::RectInfo_S &stRight)
{
    if (!is_valid_rect(stLeft))
    {
        return stRight;
    }
    if (!is_valid_rect(stRight))
    {
        return stLeft;
    }

    Common::RectInfo_S stMergedRect;
    stMergedRect.nX1 = std::min(stLeft.nX1, stRight.nX1);
    stMergedRect.nY1 = std::min(stLeft.nY1, stRight.nY1);
    stMergedRect.nX2 = std::max(stLeft.nX2, stRight.nX2);
    stMergedRect.nY2 = std::max(stLeft.nY2, stRight.nY2);
    return stMergedRect;
}

/**
 * @brief   : 构造玩手机/抽烟事件的展会面板条目
 * @param    {Event::Type_E} enEventType：当前事件类型
 * @param    {const std::vector<Group4PersonPanel_S> &} vecMatches：当前事件命中的人员列表
 * @param    {bool} bAlarm：当前事件是否处于报警态
 * @return   {OsdPanel::PanelItem_S} 构造后的面板条目
 */
OsdPanel::PanelItem_S build_group4_count_panel_item(Event::Type_E                           enEventType,
                                                    const std::vector<Group4PersonPanel_S> &vecMatches,
                                                    bool                                    bAlarm)
{
    (void)enEventType;
    OsdPanel::PanelItem_S stItem;
    if (vecMatches.empty())
    {
        return stItem;
    }

    /* 当前条目需要展示的聚合矩形框。 */
    Common::RectInfo_S stMergedRect;
    /* 当前命中人员中的最高置信度。 */
    float fMaxConfidence = 0.0f;
    for (const auto &stMatch : vecMatches)
    {
        stMergedRect   = merge_rect(stMergedRect, stMatch.stPersonRect);
        fMaxConfidence = std::max(fMaxConfidence, stMatch.fConfidence);
    }

    stItem.clear();
    /* 抽烟/玩手机配置本身没有独立区域规则，因此不显示区域标题。 */
    stItem.strTitle.clear();
    stItem.bAlarm    = bAlarm;
    stItem.bHasRect  = is_valid_rect(stMergedRect);
    stItem.stRect    = stMergedRect;
    stItem.nSortKey  = 1;
    stItem.nPriority = build_exhibition_panel_priority(bAlarm, fMaxConfidence);
    stItem.vecFields = {{"人数", std::to_string(static_cast<int>(vecMatches.size()))}};
    return stItem;
}

/**
 * @brief   : 在玩手机与抽烟两个候选面板中选出当前需要展示的一帧
 * @param    {const OsdPanel::PanelFrame_S &} stPhonePanelFrame：玩手机面板
 * @param    {bool} bPhoneAlarm：玩手机是否报警
 * @param    {int} nPhoneCount：玩手机人数
 * @param    {const OsdPanel::PanelFrame_S &} stSmokingPanelFrame：抽烟面板
 * @param    {bool} bSmokingAlarm：抽烟是否报警
 * @param    {int} nSmokingCount：抽烟人数
 * @return   {const OsdPanel::PanelFrame_S *} 需要发送的面板帧指针
 * @note    : 报警态优先，其次人数更多，再次默认玩手机优先
 */
const OsdPanel::PanelFrame_S *select_group4_panel_frame(const OsdPanel::PanelFrame_S &stPhonePanelFrame,
                                                        bool                          bPhoneAlarm,
                                                        int                           nPhoneCount,
                                                        const OsdPanel::PanelFrame_S &stSmokingPanelFrame,
                                                        bool                          bSmokingAlarm,
                                                        int                           nSmokingCount)
{
    /* 玩手机候选面板是否有可展示内容。 */
    const bool bHasPhonePanel = !stPhonePanelFrame.empty();
    /* 抽烟候选面板是否有可展示内容。 */
    const bool bHasSmokingPanel = !stSmokingPanelFrame.empty();
    if (!bHasPhonePanel && !bHasSmokingPanel)
    {
        return nullptr;
    }
    if (!bHasPhonePanel)
    {
        return &stSmokingPanelFrame;
    }
    if (!bHasSmokingPanel)
    {
        return &stPhonePanelFrame;
    }
    if (bPhoneAlarm != bSmokingAlarm)
    {
        return bPhoneAlarm ? &stPhonePanelFrame : &stSmokingPanelFrame;
    }
    if (nPhoneCount != nSmokingCount)
    {
        return nPhoneCount > nSmokingCount ? &stPhonePanelFrame : &stSmokingPanelFrame;
    }
    return &stPhonePanelFrame;
}
#endif
}  // namespace

CGroup2_Group4Detect::CGroup2_Group4Detect()
    : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CGroup2_Group4Detect::run, this);
}

CGroup2_Group4Detect::~CGroup2_Group4Detect()
{
    /* 通知线程停止 */
    m_bRunning.store(false);
    m_condition.notify_all();
    MediaData_S stMediaData;
    m_dateQueue.pushOrReplace(stMediaData);
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    unInitGroup2();
    unInitGroup4();
    unInitLicensePlateCognitionDetect();
    unInitPersonAttribute();
    unInitMotorVehicleAttribute();
    unInitNonMotorizedAttribute();
}

static void printArea(const std::vector<::Event::Point_S> &area)
{
    // 打印区域包含的点数量
    std::cout << "当前区域包含 " << area.size() << " 个点：" << std::endl;

    // 遍历区域内的每个点
    for (size_t pointIdx = 0; pointIdx < area.size(); ++pointIdx)
    {
        const ::Event::Point_S &point = area[pointIdx];  // 获取当前点（注意命名空间::Event::）
        // 打印点的索引和坐标（nX 为x坐标，nY 为y坐标）
        std::cout << "  点 " << pointIdx << "：(nX=" << point.nX << ", nY=" << point.nY << ")" << std::endl;
    }
    std::cout << "-------------------------" << std::endl;
}

void CGroup2_Group4Detect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stAlgoCrossCfg.bEnable && !m_stAlgoIntruCfg.bEnable && !m_stAlgoEntryCfg.bEnable && !m_stAlgoExitCfg.bEnable && !m_stAlgoEmergencyLaneOccupancyCfg.bEnable && !m_stPersonFallDownCfg.bEnable &&
        !m_stAlgoNonMotorVehicleIntrusionCfg.bEnable && !m_stAlgoElectricScooterCfg.bEnable && !m_stDrivingAgainstTrafficDetectionCfg.bEnable && !m_stAlgoIllegalLaneChangeDetectionCfg.bEnable &&
        !m_stAlgoCongestionDetectionCfg.bEnable && !m_stAlgoParkingDetectionCfg.bEnable && !m_stLoiteringCfg.bEnable && !m_stFenceClimbingCfg.bEnable && !m_stLeavePostCfg.bEnable && !m_stAlgoLicensePlateCognitionCfg.bEnable &&
        !m_stPedestrianIntrusionCfg.bEnable && !m_stCrowdGatheringDetCfg.bEnable && !m_stAlgoSmokingCfg.bEnable && !m_stAlgoSleepOnDutyCfg.bEnable && !m_stPhoneUsageCfg.bEnable && !m_stAlgoTripCfg.bEnable &&
        !m_bPedestrianAttribute.load() && !m_bMotorVehicleAttribute.load() && !m_bNonMotorVehicleAttribute.load())
    {
        dlog_debug("ai_app: 机动车、行人、非机动车/模型组合4/车牌检测模型-开关未启用");
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("ai_app: 机动车、行人、非机动车/模型组合4/车牌检测模型-数据队列满了");
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CGroup2_Group4Detect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stAlgoCrossCfg.bEnable = stAlgoConfig.nEnLineCrossing;
    m_stAlgoIntruCfg.bEnable = stAlgoConfig.nEnIntrusion;
    m_stAlgoEntryCfg.bEnable = stAlgoConfig.nEnEnterRegion;
    m_stAlgoExitCfg.bEnable  = stAlgoConfig.nEnLeaveRegion;

    m_stLoiteringCfg.bEnable           = stAlgoConfig.nEnLoiteringDetect;
    m_stFenceClimbingCfg.bEnable       = stAlgoConfig.nEnFenceClimbing;
    m_stLeavePostCfg.bEnable           = stAlgoConfig.nEnLeavePost;
    m_stPedestrianIntrusionCfg.bEnable = stAlgoConfig.nEnPedestrianIntrusion;
    m_stCrowdGatheringDetCfg.bEnable   = stAlgoConfig.nEnCrowdGathering;
    m_stPersonFallDownCfg.bEnable      = stAlgoConfig.nEnPersonFallDown;

    m_stAlgoEmergencyLaneOccupancyCfg.bEnable     = stAlgoConfig.nEnEmergencyLaneOccupancy;
    m_stAlgoNonMotorVehicleIntrusionCfg.bEnable   = stAlgoConfig.nEnNonMotorVehicleIntrusion;
    m_stAlgoElectricScooterCfg.bEnable            = stAlgoConfig.nEnElectricVehicleInElevator;
    m_stDrivingAgainstTrafficDetectionCfg.bEnable = stAlgoConfig.nEnReverseDirection;
    m_stAlgoCongestionDetectionCfg.bEnable        = stAlgoConfig.nEnCongestion;
    m_stAlgoParkingDetectionCfg.bEnable           = stAlgoConfig.nEnParkingDetect;
    m_stAlgoIllegalLaneChangeDetectionCfg.bEnable = stAlgoConfig.nEnIllegalLaneChange;

    m_stAlgoSmokingCfg.bEnable     = stAlgoConfig.nEnSmoking;
    m_stAlgoSleepOnDutyCfg.bEnable = stAlgoConfig.nEnSleepOnDuty;
    m_stPhoneUsageCfg.bEnable      = stAlgoConfig.nEnPhoneUsage;
    m_stAlgoTripCfg.bEnable        = stAlgoConfig.nEnTrip;

    m_stAlgoLicensePlateCognitionCfg.bEnable = stAlgoConfig.nPlateNumber;

    if (m_stAlgoCrossCfg.bEnable)
    {
        Alarm::BoundaryDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
    if (m_stAlgoIntruCfg.bEnable)
    {
        Alarm::FieldDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
    if (m_stAlgoEntryCfg.bEnable)
    {
        Alarm::EntranceDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
    if (m_stAlgoExitCfg.bEnable)
    {
        Alarm::ExitingDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_stLoiteringCfg.bEnable)
    {
        Alarm::LoiteringDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
    if (m_stFenceClimbingCfg.bEnable)
    {
        Alarm::FenceClimbingDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
    if (m_stLeavePostCfg.bEnable)
    {
        Alarm::LeavePostDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
    if (m_stPedestrianIntrusionCfg.bEnable)
    {
        Alarm::PedestrianIntrusionDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
        if (m_pGroup2Handle)
        {
            m_pGroup2Handle->resetPedestrianIntrusionStatus();
        }
    }
    if (m_stCrowdGatheringDetCfg.bEnable)
    {
        Alarm::CrowdGathering_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
    if (m_stPersonFallDownCfg.bEnable)
    {
        Alarm::PersonFallDownDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_stAlgoEmergencyLaneOccupancyCfg.bEnable)
    {
        Alarm::EmergencyLaneOccupancyDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_stAlgoNonMotorVehicleIntrusionCfg.bEnable)
    {
        Alarm::NonMotorVehicleIntrusionDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
        if (m_pGroup2Handle)
        {
            m_pGroup2Handle->resetNonMotorVehicleIntrusionStatus();
        }
    }

    if (m_stAlgoElectricScooterCfg.bEnable)
    {
        Alarm::ElectricScooterDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_stDrivingAgainstTrafficDetectionCfg.bEnable)
    {
        Alarm::DrivingAgainstTrafficDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo, Event::Type::REVERSE_DIRECTION);
    }

    if (m_stAlgoCongestionDetectionCfg.bEnable)
    {
        Alarm::CongestionDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo, Event::Type::CONGESTION);
    }

    if (m_stAlgoParkingDetectionCfg.bEnable)
    {
        Alarm::ParkingDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo, Event::Type::PARKING_DETECT);
    }

    if (m_stAlgoIllegalLaneChangeDetectionCfg.bEnable)
    {
        Alarm::IllegalLaneChangeDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo, Event::Type::ILLEGAL_LANE_CHANGE);
    }

    if (m_stAlgoSmokingCfg.bEnable)
    {
        Alarm::SmokingDection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_stAlgoSleepOnDutyCfg.bEnable)
    {
        Alarm::SleepOnDutyDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_stPhoneUsageCfg.bEnable)
    {
        Alarm::PhoneUsageDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_stAlgoTripCfg.bEnable)
    {
        Alarm::TripDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_stAlgoLicensePlateCognitionCfg.bEnable)
    {
        Alarm::LicensePlateCognitionDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    Alarm::AttributeDetectSwitch_S stAttributeDetectSwitch;
    CEventConfigure::instance()->get_configure(stAttributeDetectSwitch);
    m_bPedestrianAttribute.store(stAttributeDetectSwitch.bPedestrianAttribute);
    m_bMotorVehicleAttribute.store(stAttributeDetectSwitch.bMotorVehicleAttribute);
    m_bNonMotorVehicleAttribute.store(stAttributeDetectSwitch.bNonMotorVehicleAttribute);

    return;
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::BoundaryDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app:  设置越界检测参数");
    m_stAlgoCrossCfg = stAlgoCfg;
    convertBoundaryAndEnable(m_stAlgoCrossCfg);
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::FieldDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app:  设置区域入侵参数");
    m_stAlgoIntruCfg = stAlgoCfg;
    m_vstIntruRule.clear();
    convertResolutionAndEnable(m_stAlgoIntruCfg, enType);
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::EntranceDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app:  设置进入区域参数");
    m_stAlgoEntryCfg = stAlgoCfg;
    m_vstEntryRule.clear();
    convertResolutionAndEnable(m_stAlgoEntryCfg, enType);
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::ExitingDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app:  设置离开区域参数");
    m_stAlgoExitCfg = stAlgoCfg;
    m_vstExitRule.clear();
    convertResolutionAndEnable(m_stAlgoExitCfg, enType);
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::LoiteringDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app:  设置徘徊侦测区域参数");
    m_stLoiteringCfg = stAlgoCfg;
    m_vstLoiteringRule.clear();
    convertResolutionAndEnable(m_stLoiteringCfg, enType);
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::FenceClimbingDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app:  设置翻越围栏参数");
    m_stFenceClimbingCfg = stAlgoCfg;
    m_vstFenceClimbingRule.clear();
    convertResolutionAndEnable(m_stFenceClimbingCfg, enType);
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::LeavePostDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app:  设置离岗识别参数");
    m_stLeavePostCfg = stAlgoCfg;
    m_vstLeavePostRule.clear();
    convertResolutionAndEnable(m_stLeavePostCfg, enType);
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::PedestrianIntrusionDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app:  设置行人闯入参数");
    m_stPedestrianIntrusionCfg = stAlgoCfg;
    m_vstPedestrianIntrusionRule.clear();
    convertResolutionAndEnable(m_stPedestrianIntrusionCfg, enType);
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::CrowdGathering_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app:  设置人员聚集参数");
    m_stCrowdGatheringDetCfg = stAlgoCfg;
    m_vstCrowdGatheringDetRule.clear();
    convertCrowdGatherAndEnable(m_stCrowdGatheringDetCfg, Event::Type_E::CROWD_GATHERING);
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::PersonFallDownDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app:  设置人员倒地参数");
    m_stPersonFallDownCfg = stAlgoCfg;
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::EmergencyLaneOccupancyDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app:  设置应急车道占用参数");
    m_stAlgoEmergencyLaneOccupancyCfg = stAlgoCfg;
    m_vstEmergencyLaneOccupancyRule.clear();
    convertResolutionAndEnable(m_stAlgoEmergencyLaneOccupancyCfg, enType);
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::NonMotorVehicleIntrusionDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app:  设置非机动车闯入参数");
    m_stAlgoNonMotorVehicleIntrusionCfg = stAlgoCfg;
    m_vstNonMotorVehicleIntrusionRule.clear();
    convertResolutionAndEnable(m_stAlgoNonMotorVehicleIntrusionCfg, enType);
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::DrivingAgainstTrafficDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app:  设置逆行检测参数");
    m_stDrivingAgainstTrafficDetectionCfg = stAlgoCfg;
    convertAlertLineToZoneAndIsEnable(m_stDrivingAgainstTrafficDetectionCfg, enType);
    return;
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::CongestionDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置拥堵识别检测参数");
    m_stAlgoCongestionDetectionCfg = stAlgoCfg;
    return;
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::ParkingDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app:  设置违停参数");
    m_stAlgoParkingDetectionCfg = stAlgoCfg;
    m_vstIllegalParkingRule.clear();
    convertGuardAreaAndCheckAlgoEnable(m_stAlgoParkingDetectionCfg, enType);
    return;
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::IllegalLaneChangeDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app:  设置违规变道参数");
    m_stAlgoIllegalLaneChangeDetectionCfg = stAlgoCfg;
    m_vstIllegalLaneChangeRule.clear();
    convertAlertLineToZoneAndIsEnable(m_stAlgoIllegalLaneChangeDetectionCfg, enType);
    return;
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::ElectricScooterDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置电瓶车识别参数");
    m_stAlgoElectricScooterCfg = stAlgoCfg;
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::SmokingDection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置抽烟识别参数");
    m_stAlgoSmokingCfg = stAlgoCfg;
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::SleepOnDutyDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置睡岗识别参数");
    m_stAlgoSleepOnDutyCfg = stAlgoCfg;
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::PhoneUsageDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置玩手机识别参数");
    m_stPhoneUsageCfg = stAlgoCfg;
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::TripDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置摔倒识别参数");
    m_stAlgoTripCfg = stAlgoCfg;
}

void CGroup2_Group4Detect::setAlgoParamCfg(const Alarm::LicensePlateCognitionDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置车牌识别检测参数");
    m_stAlgoLicensePlateCognitionCfg = stAlgoCfg;
    return;
}

float CGroup2_Group4Detect::sensitivityToConfidence(int sensitivity, float minConfidence, float maxConfidence)
{
    int clampedSens = std::clamp(sensitivity, 1, 100);

    float confidence = maxConfidence - (clampedSens - 1) * (maxConfidence - minConfidence) / (100 - 1);

    confidence = std::clamp(confidence, minConfidence, maxConfidence);

    return confidence;
}

int CGroup2_Group4Detect::sensitivityToFrames(int sensitivity, int minFrames, int maxFrames)
{
    if (sensitivity <= 0)
        return maxFrames;
    if (sensitivity >= 100)
        return minFrames;

    // 线性映射
    double ratio  = (100.0 - sensitivity) / 100.0;
    int    frames = minFrames + static_cast<int>(ratio * (maxFrames - minFrames));

    return frames;
}

int CGroup2_Group4Detect::sensitivityToDuration(int sensitivity, int minDuration, int maxDuration)
{
    if (sensitivity <= 0)
    {
        return maxDuration;
    }

    if (sensitivity >= 100)
    {
        return minDuration;
    }

    // 线性映射
    double ratio  = (100.0 - sensitivity) / 100.0;
    int    frames = minDuration + static_cast<int>(ratio * (maxDuration - minDuration));

    return frames;
}

bool CGroup2_Group4Detect::initPersonAttribute()
{
    if (!m_pPersonAttributeHandle)
    {
        PresonAttribute_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/PersonAttribute.json";
        stInParam.bDebug       = false;

        m_pPersonAttributeHandle = new PresonAttribute_NS::CPresonAttributeV2_0(stInParam);
        if (m_pPersonAttributeHandle)
        {
            if (m_pPersonAttributeHandle->init())
            {
                dlog_debug("ai_app:行人属性检测算法初始化成功, %s", stInParam.strModelPath.c_str());

                return true;
            }
            else
            {
                delete m_pPersonAttributeHandle;
                m_pPersonAttributeHandle = nullptr;
                dlog_debug("行人属性检测算法初始化失败");
            }
        }
    }
    return false;
}

bool CGroup2_Group4Detect::unInitPersonAttribute()
{
    if (m_pPersonAttributeHandle)
    {
        delete m_pPersonAttributeHandle;
        m_pPersonAttributeHandle = nullptr;
    }

    return true;
}

bool CGroup2_Group4Detect::initMotorVehicleAttribute()
{
    if (!m_pMotorVehicleAttributeHandle)
    {
        VehicleAttribute_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/VehicleAttribute.json";
        stInParam.bDebug       = false;

        m_pMotorVehicleAttributeHandle = new VehicleAttribute_NS::CVehicleAttributeV2_0(stInParam);
        if (m_pMotorVehicleAttributeHandle)
        {
            if (m_pMotorVehicleAttributeHandle->init())
            {
                dlog_debug("ai_app:机动车属性检测算法初始化成功, %s", stInParam.strModelPath.c_str());

                return true;
            }
            else
            {
                delete m_pMotorVehicleAttributeHandle;
                m_pMotorVehicleAttributeHandle = nullptr;
                dlog_debug("机动车属性检测算法初始化失败");
            }
        }
    }
    return false;
}

bool CGroup2_Group4Detect::unInitMotorVehicleAttribute()
{
    if (m_pMotorVehicleAttributeHandle)
    {
        delete m_pMotorVehicleAttributeHandle;
        m_pMotorVehicleAttributeHandle = nullptr;
    }

    return true;
}

bool CGroup2_Group4Detect::initNonMotorizedAttribute()
{
    if (!m_pNonMotorizedAttributeHandle)
    {
        NonMotorizedAttribute_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/NonMotorizedAttribute.json";
        stInParam.bDebug       = false;

        m_pNonMotorizedAttributeHandle = new NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0(stInParam);
        if (m_pNonMotorizedAttributeHandle)
        {
            if (m_pNonMotorizedAttributeHandle->init())
            {
                dlog_debug("ai_app:非机动车属性检测算法初始化成功, %s", stInParam.strModelPath.c_str());

                return true;
            }
            else
            {
                delete m_pNonMotorizedAttributeHandle;
                m_pNonMotorizedAttributeHandle = nullptr;
                dlog_debug("非机动车属性检测算法初始化失败");
            }
        }
    }
    return false;
}

bool CGroup2_Group4Detect::unInitNonMotorizedAttribute()
{
    if (m_pNonMotorizedAttributeHandle)
    {
        delete m_pNonMotorizedAttributeHandle;
        m_pNonMotorizedAttributeHandle = nullptr;
    }

    return true;
}

bool CGroup2_Group4Detect::initGroup2()
{
    if (!m_pGroup2Handle)
    {
        Group2Detect_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/group2.json";
        stInParam.bDebug       = false;

        m_pGroup2Handle = new Group2Detect_NS::CGroup2DetectV1_0(stInParam);
        if (m_pGroup2Handle)
        {
            if (m_pGroup2Handle->init())
            {
                dlog_debug("ai_app: 机动车、行人、非机动车检测算法初始化成功, %s", stInParam.strModelPath.c_str());

                return true;
            }
            else
            {
                delete m_pGroup2Handle;
                m_pGroup2Handle = nullptr;
                dlog_debug("机动车、行人、非机动车检测算法初始化失败");
            }
        }
    }
    return false;
}

bool CGroup2_Group4Detect::unInitGroup2()
{
    if (m_pGroup2Handle)
    {
        delete m_pGroup2Handle;
        m_pGroup2Handle = nullptr;
        dlog_debug("去初始化模型组合2");
    }

    return true;
}

bool CGroup2_Group4Detect::initGroup4()
{
    if (!m_pGroup4Handle)
    {
        Group4Detect_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/group4.json";
        stInParam.bDebug       = false;

        m_pGroup4Handle = new Group4Detect_NS::CGroup4DetectV1_0(stInParam);
        if (m_pGroup4Handle)
        {
            if (m_pGroup4Handle->init())
            {
                dlog_debug("ai_app: 模型组合4 检测算法初始化成功, %s", stInParam.strModelPath.c_str());

                return true;
            }
            else
            {
                delete m_pGroup4Handle;
                m_pGroup4Handle = nullptr;
                dlog_debug("模型组合4 检测算法初始化失败");
            }
        }
    }
    return false;
}

bool CGroup2_Group4Detect::unInitGroup4()
{
    if (m_pGroup4Handle)
    {
        delete m_pGroup4Handle;
        m_pGroup4Handle = nullptr;
        dlog_debug("去初始化模型组合4");
    }

    return true;
}

bool CGroup2_Group4Detect::initLicensePlateCognitionDetect()
{
    if (!m_pLicensePlateHandle)
    {
        LicensePlateCognition_NS::InParam_S stInParam;
        stInParam.strModelPath1 = "/opt/cam/model/LicensePlateDetect.json";
        stInParam.strModelPath2 = "/opt/cam/model/LicensePlateRec.json";
        // stInParam.strOriginalDataPath = "/mnt/licensePlateCognition_detect/";
        stInParam.bDebug      = false;
        m_pLicensePlateHandle = new LicensePlateCognition_NS::CLicensePlateCognitionV1_0(stInParam);

        if (m_pLicensePlateHandle)
        {
            if (m_pLicensePlateHandle->init())
            {
                dlog_debug("ai_app: 车牌检测算法初始化成功, %s", stInParam.strModelPath1.c_str());
                dlog_debug("ai_app: 车牌号识别检测算法初始化成功, %s", stInParam.strModelPath2.c_str());

                return true;
            }
            else
            {
                delete m_pLicensePlateHandle;
                m_pLicensePlateHandle = nullptr;
                dlog_debug("车牌识别检测算法初始化失败");
            }
        }
    }
    return false;
}

bool CGroup2_Group4Detect::unInitLicensePlateCognitionDetect()
{
    if (m_pLicensePlateHandle)
    {
        delete m_pLicensePlateHandle;
        m_pLicensePlateHandle = nullptr;
        dlog_debug("去初始化车牌识别模型");
    }

    return true;
}

void CGroup2_Group4Detect::changeRuleInfos(const Event::RuleInfo &stRuleInfo, std::vector<int> veDetectionTargets, bool bNeedClear)
{
    if (bNeedClear)
    {
        m_vstRuleInfo.clear();
    }

    if (stRuleInfo.enType == Event::Type::LINE_CROSSING || stRuleInfo.enType == Event::Type::INTRUSION ||
        stRuleInfo.enType == Event::Type::ENTER_REGION || stRuleInfo.enType == Event::Type::LEAVE_REGION ||
        stRuleInfo.enType == Event::Type_E::EMERGENCY_LANE_OCCUPANCY)
    {
        dlog_debug("ai_app:  更新边界检测划线数据 type[%d]", (int)stRuleInfo.enType);
        PMNMDetectRuleInfo_S stPMNMDetectRuleInfo;
        stPMNMDetectRuleInfo.stRuleInfo         = stRuleInfo;
        stPMNMDetectRuleInfo.veDetectionTargets = veDetectionTargets;

        m_vstRuleInfo.push_back(stPMNMDetectRuleInfo);
    }
}

void CGroup2_Group4Detect::drawRulesToImage(cv::Mat &inMat)
{
    for (const auto &rule : m_vstRuleInfo)
    {
        /* 绘制所有的线条 */
        for (const auto &line : rule.stRuleInfo.lines)
        {
            if (line.size() < 2)
            {
                continue;
            }

            for (size_t i = 0; i < line.size() - 1; ++i)
            {
                cv::line(inMat,
                         cv::Point(line[i].nX, line[i].nY),
                         cv::Point(line[i + 1].nX, line[i + 1].nY),
                         cv::Scalar(0, 0, 255), /* 线条颜色（蓝色） */
                         2);
            }
        }

        /* 绘制所有的区域（四边形边框） */
        for (const auto &area : rule.stRuleInfo.areas)
        {
            std::vector<cv::Point> polygon;
            for (const auto &point : area)
            {
                polygon.emplace_back(point.nX, point.nY);
            }
            cv::polylines(inMat,
                          polygon,
                          true,
                          cv::Scalar(0, 255, 0), /* 边框颜色（绿色） */
                          2,
                          cv::LINE_AA);
        }
    }
}

bool CGroup2_Group4Detect::saveImage(const cv::Mat &image, const std::string &strOutputPath)
{
    struct stat info;
    /* 目录不存在 */
    if (stat(strOutputPath.c_str(), &info) != 0)
    {
        /* 使用命令 mkdir -p 来递归创建目录 */
        std::string strCmd = "mkdir -p \"" + strOutputPath + "\"";

        int nRet = system(strCmd.c_str());
        if (nRet != 0)
        {
            return false;
        }
    }

    /* 获取当前时间（精确到微秒） */
    auto now          = std::chrono::system_clock::now();
    auto time_t_now   = std::chrono::system_clock::to_time_t(now);
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;

    /* 将 time_t 转换为本地时间 */
    struct tm timeinfo;
#ifdef _WIN32
    localtime_s(&timeinfo, &time_t_now);
#else
    localtime_r(&time_t_now, &timeinfo);
#endif

    /* 格式化时间戳，精确到微秒 */
    std::ostringstream timestamp;
    timestamp << std::put_time(&timeinfo, "%Y%m%d_%H%M%S")
              << "_" << std::setw(6) << std::setfill('0') << microseconds.count();

    /* 构造完整的文件名（包含路径） */
    std::ostringstream filename;
    filename << strOutputPath << "/image_" << timestamp.str() << ".jpg";

    /* 使用 OpenCV 的 imwrite 函数保存图像 */
    bool bSaved = cv::imwrite(filename.str(), image);

    return bSaved; /* 返回保存结果 */
}

void CGroup2_Group4Detect::run()
{
    MediaData_S      stMediaData;
    std::vector<int> vResultData;
    vResultData.clear();
    // VIDEO_FRAME_INFO_S* pSrcFrame = nullptr;

    while (m_bRunning.load())
    {
        if (m_stAlgoCrossCfg.bEnable || m_stAlgoIntruCfg.bEnable || m_stAlgoEntryCfg.bEnable || m_stAlgoExitCfg.bEnable || m_stAlgoEmergencyLaneOccupancyCfg.bEnable || m_stPersonFallDownCfg.bEnable ||
            m_stAlgoNonMotorVehicleIntrusionCfg.bEnable || m_stAlgoElectricScooterCfg.bEnable || m_stDrivingAgainstTrafficDetectionCfg.bEnable || m_stAlgoIllegalLaneChangeDetectionCfg.bEnable ||
            m_stAlgoCongestionDetectionCfg.bEnable || m_stAlgoParkingDetectionCfg.bEnable || m_stLoiteringCfg.bEnable || m_stFenceClimbingCfg.bEnable || m_stLeavePostCfg.bEnable ||
            m_stPedestrianIntrusionCfg.bEnable || m_stCrowdGatheringDetCfg.bEnable || m_stAlgoSmokingCfg.bEnable || m_stPhoneUsageCfg.bEnable || m_stAlgoLicensePlateCognitionCfg.bEnable ||
            m_bPedestrianAttribute.load() || m_bMotorVehicleAttribute.load() || m_bNonMotorVehicleAttribute.load())
        {
            if (!m_pGroup2Handle)
            {
                if (!initGroup2())
                {
                    dlog_error("等待模型组合2检测初始化");
                    /* 延迟等待 1s */
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_condition.wait_for(lock, std::chrono::seconds(1), [this] {
                        return !m_bRunning.load();
                    });
                }
                continue;
            }
        }
        else
        {
            unInitGroup2();
        }

        if (m_stPhoneUsageCfg.bEnable || m_stAlgoSmokingCfg.bEnable || m_stAlgoSleepOnDutyCfg.bEnable || m_stAlgoTripCfg.bEnable)
        {
            if (!m_pGroup4Handle)
            {
                if (!initGroup4())
                {
                    dlog_error("等待模型组合4检测初始化");
                    /* 延迟等待 1s */
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_condition.wait_for(lock, std::chrono::seconds(1), [this] {
                        return !m_bRunning.load();
                    });
                }
                continue;
            }
        }
        else
        {
            unInitGroup4();
        }

        if (m_stAlgoLicensePlateCognitionCfg.bEnable || m_bMotorVehicleAttribute.load())
        {
            if (!m_pLicensePlateHandle)
            {
                if (!initLicensePlateCognitionDetect())
                {
                    dlog_error("等待车牌检测模型初始化");
                    /* 延迟等待 1s */
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_condition.wait_for(lock, std::chrono::seconds(1), [this] {
                        return !m_bRunning.load();
                    });
                }
                continue;
            }
        }
        else
        {
            unInitLicensePlateCognitionDetect();
        }

        /* 阻塞获取 */
        m_dateQueue.pop(stMediaData, -1);
        if (stMediaData.nSize == 0)
        {
            /* 数据为空 */
            continue;
        }

        // dlog_debug("ai_app [机动车、行人、非机动车检测]: 处理前Image分辨率[ %d × %d ] 格式[%d] 大小[%d]",
        //  stMediaData.stMediaParam.nVideoWidth,stMediaData.stMediaParam.nVideoHeight
        //  ,stMediaData.stMediaParam.enPixelFormat,stMediaData.nSize);

        CStatisticsTimer runTime("机动车、行人、非机动车检测完整耗时");

        /* 送分析 */
        if (1)
        {
            frameRate("机动车、行人、非机动车检测-分析数据", 5);

            Group2Detect_NS::InData_S              stInData{};
            Group2Detect_NS::OutData_S             stOutData;
            std::vector<Group2Detect_NS::Result_S> vecResult;
            std::vector<Group2Detect_NS::Result_S> vecAllResult;

            /* 是否开启人体检测 */
            bool bPersonDetect = false;
            /* 是否开启机动车检测 */
            bool bMotorVehicle = false;
            /* 是否开启非机动车检测 */
            bool bNonMotorVehicle = false;

            cv::Mat i420Mat(
                stMediaData.stMediaParam.nVideoHeight * 3 / 2,
                stMediaData.stMediaParam.nVideoWidth,
                CV_8UC1,
                stMediaData.pData.get());

            // stOutData.nType = 0;
            // stOutData.validResult = false;

            /* rgb格式转换 */
            cv::Mat rgbMat;
            cv::cvtColor(i420Mat, rgbMat, cv::COLOR_YUV2RGB_NV12);
            // cv::rotate(rgbMat, rgbMat, cv::ROTATE_180);

            /* 分辨率大小转换 */
            cv::resize(
                rgbMat,
                stInData.inMat,
                cv::Size(m_nWidth, m_nHeight),
                0,
                0,
                cv::INTER_LINEAR);

            if (!stInData.inMat.empty())
            {
                // if (access("/debugImage", F_OK) == 0)
                // {
                //     dlog_debug("============>debugImage");
                //     cv::imwrite("/opt/algo/Image/group2Detect_debugImage.jpg", stInData.inMat);
                // }
                stInData.stParam.bVecEnable = true;
                /* 越界规则 */
                if (m_stAlgoCrossCfg.bEnable)
                {
                    for (auto &CrossRule : m_vstCrossRule)
                    {
                        Group2Detect_NS::TripLineParam_S stTripLineParam;
                        if (!CrossRule.stRuleInfo.lines.empty() && CrossRule.stRuleInfo.bEnable && CrossRule.veDetectionTargets.size() > 0)
                        {
                            stTripLineParam.bEnable = true;
                            for (const auto &line : CrossRule.stRuleInfo.lines)
                            {
                                if (line.size() >= 2)
                                {
                                    stTripLineParam.alertLine1 = cv::Point(line[0].nX, line[0].nY);
                                    stTripLineParam.alertLine2 = cv::Point(line[1].nX, line[1].nY);
                                }
                            }
                        }
                        else
                        {
                            stTripLineParam.bEnable = false;
                        }
                        stTripLineParam.eTripLineType          = (Group2Detect_NS::TripLineType_E)CrossRule.enCrossDirection;
                        stTripLineParam.veDetectionTargetTypes = CrossRule.veDetectionTargets;
                        stTripLineParam.fTripLineThreshold     = sensitivityToConfidence(CrossRule.stRuleInfo.nSensitivity);
                        stInData.stParam.vstTripLineParam.push_back(stTripLineParam);
                        if (access("/PrintstAlgoCfg", F_OK) == 0)
                        {
                            dlog_debug("越界灵敏度 fBoxThreshold = %f nSensitivity = %d", stTripLineParam.fTripLineThreshold, CrossRule.stRuleInfo.nSensitivity);
                        }

                        if (stTripLineParam.bEnable)
                        {
                            /* 0-人 1-机动车 2-非机动车 */
                            for (auto &DetectionTarget : CrossRule.veDetectionTargets)
                            {
                                if (DetectionTarget == 0)
                                {
                                    bPersonDetect = true;
                                }
                                else if (DetectionTarget == 1)
                                {
                                    bMotorVehicle = true;
                                }
                                else if (DetectionTarget == 2)
                                {
                                    bNonMotorVehicle = true;
                                }
                            }
                        }
                    }
                }

                /* 区域入侵规则 */
                if (m_stAlgoIntruCfg.bEnable)
                {
                    for (auto &IntruRule : m_vstIntruRule)
                    {
                        Group2Detect_NS::IntrusionParam_S stIntrusionParam;
                        if (!IntruRule.stRuleInfo.areas.empty() && IntruRule.stRuleInfo.bEnable && IntruRule.veDetectionTargets.size() > 0)
                        {
                            stIntrusionParam.bEnable = true;
                            for (const auto &point : IntruRule.stRuleInfo.areas[0])
                            {
                                stIntrusionParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stIntrusionParam.bEnable = false;
                        }
                        stIntrusionParam.veDetectionTargetTypes  = IntruRule.veDetectionTargets;
                        stIntrusionParam.fIntrusionThreshold     = sensitivityToConfidence(IntruRule.stRuleInfo.nSensitivity);
                        stIntrusionParam.nIntrusionTimeThreshold = IntruRule.stRuleInfo.nTimeThreshold;
                        stInData.stParam.vstIntrusionParam.push_back(stIntrusionParam);
                        if (access("/PrintstAlgoCfg", F_OK) == 0)
                        {
                            dlog_debug("区域入侵灵敏度 fBoxThreshold = %f nSensitivity = %d 时间阈值：%d", stIntrusionParam.fIntrusionThreshold, IntruRule.stRuleInfo.nSensitivity, stIntrusionParam.nIntrusionTimeThreshold);
                        }

                        if (stIntrusionParam.bEnable)
                        {
                            /* 0-人 1-机动车 2-非机动车 */
                            for (auto &DetectionTarget : IntruRule.veDetectionTargets)
                            {
                                if (DetectionTarget == 0)
                                {
                                    bPersonDetect = true;
                                }
                                else if (DetectionTarget == 1)
                                {
                                    bMotorVehicle = true;
                                }
                                else if (DetectionTarget == 2)
                                {
                                    bNonMotorVehicle = true;
                                }
                            }
                        }
                    }
                }

                /* 进入区域规则 */
                if (m_stAlgoEntryCfg.bEnable)
                {
                    for (auto &EntryRule : m_vstEntryRule)
                    {
                        Group2Detect_NS::EntryParam_S stEntryParam;
                        if (!EntryRule.stRuleInfo.areas.empty() && EntryRule.stRuleInfo.bEnable && EntryRule.veDetectionTargets.size() > 0)
                        {
                            stEntryParam.bEnable = true;
                            for (const auto &point : EntryRule.stRuleInfo.areas[0])
                            {
                                stEntryParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stEntryParam.bEnable = false;
                        }
                        stEntryParam.veDetectionTargetTypes = EntryRule.veDetectionTargets;
                        stEntryParam.fEntryThreshold        = sensitivityToConfidence(EntryRule.stRuleInfo.nSensitivity);
                        stInData.stParam.vstEntryParam.push_back(stEntryParam);
                        if (access("/PrintstAlgoCfg", F_OK) == 0)
                        {
                            dlog_debug("进入区域灵敏度 fBoxThreshold = %f nSensitivity = %d", stEntryParam.fEntryThreshold, EntryRule.stRuleInfo.nSensitivity);
                        }

                        if (stEntryParam.bEnable)
                        {
                            /* 0-人 1-机动车 2-非机动车 */
                            for (auto &DetectionTarget : EntryRule.veDetectionTargets)
                            {
                                if (DetectionTarget == 0)
                                {
                                    bPersonDetect = true;
                                }
                                else if (DetectionTarget == 1)
                                {
                                    bMotorVehicle = true;
                                }
                                else if (DetectionTarget == 2)
                                {
                                    bNonMotorVehicle = true;
                                }
                            }
                        }
                    }
                }

                /* 离开区域规则 */
                if (m_stAlgoExitCfg.bEnable)
                {
                    for (auto &ExitRule : m_vstExitRule)
                    {
                        Group2Detect_NS::LeaveParam_S stLeaveParam;
                        if (!ExitRule.stRuleInfo.areas.empty() && ExitRule.stRuleInfo.bEnable && ExitRule.veDetectionTargets.size() > 0)
                        {
                            stLeaveParam.bEnable = true;
                            for (const auto &point : ExitRule.stRuleInfo.areas[0])
                            {
                                stLeaveParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stLeaveParam.bEnable = false;
                        }

                        stLeaveParam.veDetectionTargetTypes = ExitRule.veDetectionTargets;
                        stLeaveParam.fLeaveThreshold        = sensitivityToConfidence(ExitRule.stRuleInfo.nSensitivity);
                        stInData.stParam.vstLeaveParam.push_back(stLeaveParam);
                        if (access("/PrintstAlgoCfg", F_OK) == 0)
                        {
                            dlog_debug("离开区域灵敏度 fBoxThreshold = %f nSensitivity = %d", stLeaveParam.fLeaveThreshold, ExitRule.stRuleInfo.nSensitivity);
                        }

                        if (stLeaveParam.bEnable)
                        {
                            /* 0-人 1-机动车 2-非机动车 */
                            for (auto &DetectionTarget : ExitRule.veDetectionTargets)
                            {
                                if (DetectionTarget == 0)
                                {
                                    bPersonDetect = true;
                                }
                                else if (DetectionTarget == 1)
                                {
                                    bMotorVehicle = true;
                                }
                                else if (DetectionTarget == 2)
                                {
                                    bNonMotorVehicle = true;
                                }
                            }
                        }
                    }
                }

                /* 应急车道规则 */
                if (m_stAlgoEmergencyLaneOccupancyCfg.bEnable)
                {
                    for (auto &EmergencyLaneOccupancy : m_vstEmergencyLaneOccupancyRule)
                    {
                        Group2Detect_NS::EmergencyLaneOccupancyParam_S stEmergencyLaneOccupancyParam;
                        if (!EmergencyLaneOccupancy.stRuleInfo.areas.empty() && EmergencyLaneOccupancy.stRuleInfo.bEnable && EmergencyLaneOccupancy.veDetectionTargets.size() > 0)
                        {
                            stEmergencyLaneOccupancyParam.bEnable = true;
                            for (const auto &point : EmergencyLaneOccupancy.stRuleInfo.areas[0])
                            {
                                stEmergencyLaneOccupancyParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stEmergencyLaneOccupancyParam.bEnable = false;
                        }

                        stEmergencyLaneOccupancyParam.fEmergencyLaneOccupancyThreshold     = sensitivityToConfidence(EmergencyLaneOccupancy.stRuleInfo.nSensitivity);
                        stEmergencyLaneOccupancyParam.nEmergencyLaneOccupancyTimeThreshold = EmergencyLaneOccupancy.stRuleInfo.nTimeThreshold * 1000;  // s转换为ms
                        stEmergencyLaneOccupancyParam.veDetectionTargetTypes               = EmergencyLaneOccupancy.veDetectionTargets;
                        stInData.stParam.vstEmergencyLaneOccupancyParam.push_back(stEmergencyLaneOccupancyParam);
                        if (access("/PrintstAlgoCfg", F_OK) == 0)
                        {
                            dlog_debug("区域入侵灵敏度 fBoxThreshold = %f nSensitivity = %d 时间阈值：%d", stEmergencyLaneOccupancyParam.fEmergencyLaneOccupancyThreshold, EmergencyLaneOccupancy.stRuleInfo.nSensitivity, stEmergencyLaneOccupancyParam.nEmergencyLaneOccupancyTimeThreshold);
                        }

                        if (stEmergencyLaneOccupancyParam.bEnable)
                        {
                            /* 0-人 1-机动车 2-非机动车 */
                            for (auto &DetectionTarget : EmergencyLaneOccupancy.veDetectionTargets)
                            {
                                if (DetectionTarget == 0)
                                {
                                    bPersonDetect = true;
                                }
                                else if (DetectionTarget == 1)
                                {
                                    bMotorVehicle = true;
                                }
                                else if (DetectionTarget == 2)
                                {
                                    bNonMotorVehicle = true;
                                }
                            }
                        }
                    }
                }

                /* 非机动车闯入规则 */
                if (m_stAlgoNonMotorVehicleIntrusionCfg.bEnable)
                {
                    for (auto &NonMotorVehicleIntrusionRule : m_vstNonMotorVehicleIntrusionRule)
                    {
                        Group2Detect_NS::NonMotorVehicleIntrusionParam_S stNonMotorVehicleIntrusionParam;
                        if (!NonMotorVehicleIntrusionRule.stRuleInfo.areas.empty() && NonMotorVehicleIntrusionRule.stRuleInfo.bEnable /*&& NonMotorVehicleIntrusionRule.veDetectionTargets.size() > 0*/)
                        {
                            bNonMotorVehicle                        = true;
                            stNonMotorVehicleIntrusionParam.bEnable = true;
                            for (const auto &point : NonMotorVehicleIntrusionRule.stRuleInfo.areas[0])
                            {
                                stNonMotorVehicleIntrusionParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stNonMotorVehicleIntrusionParam.bEnable = false;
                        }
                        stNonMotorVehicleIntrusionParam.fNonMotorVehicleIntrusionThreshold     = sensitivityToConfidence(NonMotorVehicleIntrusionRule.stRuleInfo.nSensitivity);
                        stNonMotorVehicleIntrusionParam.nNonMotorVehicleIntrusionTimeThreshold = NonMotorVehicleIntrusionRule.stRuleInfo.nTimeThreshold * 1000;  // s转换为ms
                        // stNonMotorVehicleIntrusionParam.veDetectionTargetTypes = NonMotorVehicleIntrusionRule.veDetectionTargets;
                        stInData.stParam.vstNonMotorVehicleIntrusionParam.push_back(stNonMotorVehicleIntrusionParam);
                    }
                }

                /* 徘徊侦测规则 */
                if (m_stLoiteringCfg.bEnable)
                {
                    for (auto &LoiteringRule : m_vstLoiteringRule)
                    {
                        Group2Detect_NS::LoiteringParam_S stLoiteringParam;
                        if (!LoiteringRule.areas.empty() && LoiteringRule.bEnable)
                        {
                            bPersonDetect                        = true;
                            stLoiteringParam.bEnable             = true;
                            stLoiteringParam.nTimeThreshold      = LoiteringRule.nTimeThreshold;
                            stLoiteringParam.fLoiteringThreshold = sensitivityToConfidence(LoiteringRule.nSensitivity);
                            for (const auto &point : LoiteringRule.areas[0])
                            {
                                stLoiteringParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stLoiteringParam.bEnable = false;
                        }
                        if (access("/PrintstAlgoCfg", F_OK) == 0)
                        {
                            dlog_debug("徘徊侦测 灵敏度 fBoxThreshold = %f nSensitivity = %d", stLoiteringParam.fLoiteringThreshold, LoiteringRule.nSensitivity);
                        }
                        stInData.stParam.vsLoiteringParam.push_back(stLoiteringParam);
                    }
                }
                /* 翻越围栏规则 */
                if (m_stFenceClimbingCfg.bEnable)
                {
                    for (auto &FenceClimbingRule : m_vstFenceClimbingRule)
                    {
                        Group2Detect_NS::FenceClimbingParam_S stFenceClimbingParam;
                        if (!FenceClimbingRule.areas.empty() && FenceClimbingRule.bEnable)
                        {
                            bPersonDetect                = true;
                            stFenceClimbingParam.bEnable = true;
                            for (const auto &point : FenceClimbingRule.areas[0])
                            {
                                stFenceClimbingParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                            /* 灵敏度转换检测帧数 */
                            stFenceClimbingParam.nDetectFrame = (100 - FenceClimbingRule.nSensitivity);
                            if (stFenceClimbingParam.nDetectFrame <= 0 || stFenceClimbingParam.nDetectFrame > 100)
                            {
                                stFenceClimbingParam.nDetectFrame = 1;
                            }
                            stFenceClimbingParam.fFenceClimbingThreshold = sensitivityToConfidence(FenceClimbingRule.nSensitivity);
                        }
                        else
                        {
                            stFenceClimbingParam.bEnable = false;
                        }
                        stInData.stParam.vstFenceClimbingParam.push_back(stFenceClimbingParam);
                    }
                }
                /* 离岗规则 */
                if (m_stLeavePostCfg.bEnable)
                {
                    for (auto &LeavePostRule : m_vstLeavePostRule)
                    {
                        Group2Detect_NS::LeavePostParam_S stLeavePostParam;
                        if (!LeavePostRule.areas.empty() && LeavePostRule.bEnable)
                        {
                            bPersonDetect                        = true;
                            stLeavePostParam.bEnable             = true;
                            stLeavePostParam.nTimeThreshold      = LeavePostRule.nTimeThreshold;
                            stLeavePostParam.fLeavePostThreshold = sensitivityToConfidence(LeavePostRule.nSensitivity);
                            for (const auto &point : LeavePostRule.areas[0])
                            {
                                stLeavePostParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stLeavePostParam.bEnable = false;
                        }
                        if (access("/PrintstAlgoCfg", F_OK) == 0)
                        {
                            dlog_debug("进入区域灵敏度 fBoxThreshold = %f nSensitivity = %d", stLeavePostParam.fLeavePostThreshold, LeavePostRule.nSensitivity);
                        }
                        stInData.stParam.vstLeavePostParam.push_back(stLeavePostParam);
                    }
                }
                /* 行人闯入识别规则 */
                if (m_stPedestrianIntrusionCfg.bEnable)
                {
                    for (auto &PedestrianIntrusionRule : m_vstPedestrianIntrusionRule)
                    {
                        Group2Detect_NS::PedestrianIntrusionParam_S stPedestrianIntrusionParam;
                        if (!PedestrianIntrusionRule.areas.empty() && PedestrianIntrusionRule.bEnable)
                        {
                            bPersonDetect                                            = true;
                            stPedestrianIntrusionParam.bEnable                       = true;
                            stPedestrianIntrusionParam.nTimeThreshold                = PedestrianIntrusionRule.nTimeThreshold * 1000;  // 转化为ms
                            stPedestrianIntrusionParam.fPedestrianIntrusionThreshold = sensitivityToConfidence(PedestrianIntrusionRule.nSensitivity);
                            for (const auto &point : PedestrianIntrusionRule.areas[0])
                            {
                                stPedestrianIntrusionParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stPedestrianIntrusionParam.bEnable = false;
                        }
                        stInData.stParam.vstPedestrianIntrusionParam.push_back(stPedestrianIntrusionParam);
                    }
                }

                /* 人员聚集识别规则 */
                if (m_stCrowdGatheringDetCfg.bEnable)
                {
                    for (auto &CrowdGatheringDetRule : m_vstCrowdGatheringDetRule)
                    {
                        Group2Detect_NS::CrowdGatheringDetParam_S stCrowdGatheringDetParam;
                        if (!CrowdGatheringDetRule.areas.empty() && CrowdGatheringDetRule.bEnable)
                        {
                            bPersonDetect                                 = true;
                            stCrowdGatheringDetParam.bEnable              = true;
                            stCrowdGatheringDetParam.nProportionThreshold = CrowdGatheringDetRule.nSensitivity;
                            for (const auto &point : CrowdGatheringDetRule.areas[0])
                            {
                                stCrowdGatheringDetParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stCrowdGatheringDetParam.bEnable = false;
                        }
                        stInData.stParam.vstCrowdGatheringDetParam.push_back(stCrowdGatheringDetParam);
                    }
                }

                /* 人员倒地规则 */
                if (m_stPersonFallDownCfg.bEnable)
                {
                    bPersonDetect                                                   = true;
                    stInData.stParam.stPersonFallDownParam.bEnable                  = true;
                    stInData.stParam.stPersonFallDownParam.fPersonFallDownThreshold = sensitivityToConfidence(m_stAlgoElectricScooterCfg.stRule.nSensitivity);
                    stInData.stParam.stPersonFallDownParam.nTimeThreshold           = sensitivityToDuration(m_stAlgoElectricScooterCfg.stRule.nSensitivity);
                    if (access("/debugPersonFallDown", F_OK) == 0)
                    {
                        dlog_debug("============> fPersonFallDownThreshold = %f, nTimeThreshold = %d", stInData.stParam.stPersonFallDownParam.fPersonFallDownThreshold, stInData.stParam.stPersonFallDownParam.nTimeThreshold);
                    }
                }

                /* 电瓶车进电梯规则 */
                if (m_stAlgoElectricScooterCfg.bEnable)
                {
                    bNonMotorVehicle                                     = true;
                    stInData.stParam.stElectricScooterParam.bEnable      = true;
                    stInData.stParam.stElectricScooterParam.fConfidence  = sensitivityToConfidence(m_stAlgoElectricScooterCfg.stRule.nSensitivity);
                    stInData.stParam.stElectricScooterParam.nDetectFrame = sensitivityToFrames(m_stAlgoElectricScooterCfg.stRule.nSensitivity);
                }

                /* 逆行规则 */
                if (m_stDrivingAgainstTrafficDetectionCfg.bEnable)
                {
                    for (auto &DrivingAgainstTrafficRule : m_vstDrivingAgainstTrafficRule)
                    {
                        Group2Detect_NS::DrivingAgainstTrafficParam_S stDrivingAgainstTrafficParam;
                        if (!DrivingAgainstTrafficRule.stRuleInfo.lines.empty() && DrivingAgainstTrafficRule.stRuleInfo.bEnable)
                        {
                            bMotorVehicle                        = true;
                            stDrivingAgainstTrafficParam.bEnable = true;
                            for (const auto &line : DrivingAgainstTrafficRule.stRuleInfo.lines)
                            {
                                if (line.size() >= 2)
                                {
                                    stDrivingAgainstTrafficParam.alertLine1                         = cv::Point(line[0].nX, line[0].nY);
                                    stDrivingAgainstTrafficParam.alertLine2                         = cv::Point(line[1].nX, line[1].nY);
                                    stDrivingAgainstTrafficParam.eTripLineType                      = (Group2Detect_NS::TripLineType_E)DrivingAgainstTrafficRule.enCrossDirection;
                                    stDrivingAgainstTrafficParam.fDrivingAgainstTrafficBoxThreshold = sensitivityToConfidence(DrivingAgainstTrafficRule.stRuleInfo.nSensitivity);
                                    if (access("/PrintDrivingAgainstTrafficDetectionCfg", F_OK) == 0)
                                    {
                                        dlog_debug("fCongestionBoxThreshold = %f", stDrivingAgainstTrafficParam.fDrivingAgainstTrafficBoxThreshold);
                                    }
                                }
                            }
                        }
                        else
                        {
                            stDrivingAgainstTrafficParam.bEnable = false;
                        }
                        stInData.stParam.vstDrivingAgainstTrafficParam.push_back(stDrivingAgainstTrafficParam);
                    }
                }

                /* 拥堵识别 */
                if (m_stAlgoCongestionDetectionCfg.bEnable)
                {
                    bMotorVehicle                                              = true;
                    stInData.stParam.stCongestionParam.fCongestionBoxThreshold = sensitivityToConfidence(m_stAlgoCongestionDetectionCfg.stRule.nSensitivity);
                    stInData.stParam.stCongestionParam.nCongestionThreshold    = 100.0f * stInData.stParam.stCongestionParam.fCongestionBoxThreshold;
                    if (access("/PrintCongestionDetectionCfg", F_OK) == 0)
                    {
                        dlog_debug("fCongestionBoxThreshold = %f  nCongestionThreshold = %d ", stInData.stParam.stCongestionParam.fCongestionBoxThreshold, stInData.stParam.stCongestionParam.nCongestionThreshold);
                    }
                }

                /* 违规停车规则 */
                if (m_stAlgoParkingDetectionCfg.bEnable)
                {
                    for (auto &IllegalParkingRule : m_vstIllegalParkingRule)
                    {
                        Group2Detect_NS::ParkingParam_S stParkingParam;
                        if (!IllegalParkingRule.areas.empty() && IllegalParkingRule.bEnable)
                        {
                            bMotorVehicle                        = true;
                            stParkingParam.bEnable               = true;
                            stParkingParam.nParkingTimeThreshold = IllegalParkingRule.nTimeThreshold * 1000; /* s转换为ms */
                            stParkingParam.fParkingBoxThreshold  = sensitivityToConfidence(IllegalParkingRule.nSensitivity);
                            for (const auto &point : IllegalParkingRule.areas[0])
                            {
                                stParkingParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }

                            if (access("/PrintParkingDetectionCfg", F_OK) == 0)
                            {
                                dlog_debug(" nParkingTimeThreshold = %d,  fParkingBoxThreshold = %f", stParkingParam.nParkingTimeThreshold, stParkingParam.fParkingBoxThreshold);
                            }
                        }
                        else
                        {
                            stParkingParam.bEnable = false;
                        }
                        stInData.stParam.vstParkingParam.push_back(stParkingParam);
                    }
                }

                /* 违规变道规则 */
                if (m_stAlgoIllegalLaneChangeDetectionCfg.bEnable)
                {
                    for (auto &IllegalLaneChangeRule : m_vstIllegalLaneChangeRule)
                    {
                        Group2Detect_NS::IllegalLaneChangeParam_S stIllegalLaneChangeParam;
                        if (!IllegalLaneChangeRule.stRuleInfo.lines.empty() && IllegalLaneChangeRule.stRuleInfo.bEnable)
                        {
                            bMotorVehicle                    = true;
                            stIllegalLaneChangeParam.bEnable = true;
                            for (const auto &line : IllegalLaneChangeRule.stRuleInfo.lines)
                            {
                                if (line.size() >= 2)
                                {
                                    stIllegalLaneChangeParam.alertLine1                     = cv::Point(line[0].nX, line[0].nY);
                                    stIllegalLaneChangeParam.alertLine2                     = cv::Point(line[1].nX, line[1].nY);
                                    stIllegalLaneChangeParam.fIllegalLaneChangeBoxThreshold = sensitivityToConfidence(IllegalLaneChangeRule.stRuleInfo.nSensitivity);
                                    if (access("/PrintIllegalLaneChangeDetectionCfg", F_OK) == 0)
                                    {
                                        dlog_debug("fIllegalLaneChangeBoxThreshold = %f", stIllegalLaneChangeParam.fIllegalLaneChangeBoxThreshold);
                                    }
                                }
                            }
                        }
                        else
                        {
                            stIllegalLaneChangeParam.bEnable = false;
                        }
                        stInData.stParam.vstIllegalLaneChangeParam.push_back(stIllegalLaneChangeParam);
                    }
                }

                /* 分析数据 */
                {
                    CStatisticsTimer runTime("边界检测算法耗时");

                    if (m_pGroup2Handle)
                    {
                        m_pGroup2Handle->process(stInData, vecResult, vecAllResult, &stOutData);
                    }

                    /* 人非车事件后处理 */
                    processGroup2Detect(stOutData);

                    float                                  fRoiW = (float)rgbMat.cols / m_nWidth;
                    float                                  fRoiH = (float)rgbMat.rows / m_nHeight;
                    std::vector<Group2Detect_NS::Result_S> vecRGBSizeResult;

                    for (auto &stResult : vecAllResult)
                    {
                        Group2Detect_NS::Result_S stRGBSizeResult;
                        stRGBSizeResult     = stResult;
                        stRGBSizeResult.fX1 = stResult.fX1 * fRoiW;
                        stRGBSizeResult.fY1 = stResult.fY1 * fRoiH;
                        stRGBSizeResult.fX2 = stResult.fX2 * fRoiW;
                        stRGBSizeResult.fY2 = stResult.fY2 * fRoiH;

                        vecRGBSizeResult.push_back(stRGBSizeResult);
                    }

                    /* 模型组合4事件检测 */
                    // group4DetectProcess(stInData.inMat, vecAllResult);
                    group4DetectProcess(rgbMat, vecRGBSizeResult);

                    /* 人、机动车车、非机动车属性分析 */
                    // pnmAttributeAnalysis(stInData.inMat, vecAllResult);
                    pnmAttributeAnalysis(rgbMat, vecRGBSizeResult);

                    if (m_bPedestrianAttribute.load())
                    {
                        bPersonDetect = true;
                    }
                    if (m_bMotorVehicleAttribute.load())
                    {
                        bMotorVehicle = true;
                    }
                    if (m_bNonMotorVehicleAttribute.load())
                    {
                        bNonMotorVehicle = true;
                    }

                    /* 判断人非车动态分析 */
                    dynamicAnalysis(vecAllResult, bPersonDetect, bMotorVehicle, bNonMotorVehicle);

#ifdef ENABLE_GAT1400_SRC
                    Network::Gat1400Client_S config;
                    GAT1400::CGAT1400::instance()->getGat1400Config(config);
                    if (config.enableGat1400)
                    {
                        /* 检查上传状态及类型 */
                        int flag = checkPushState(stOutData);
                        /* flag: 0x001 人 0x010 车 0x100 非 */
                        if (flag)
                        {
                            std::vector<Group2Detect_NS::Result_S> res;
                            for (auto &it : vecResult)
                            {
                                if (flag & (1 << it.nID))
                                {
                                    res.push_back(it);
                                }
                            }
                            cv::Mat imageMat;
                            cv::cvtColor(i420Mat, imageMat, cv::COLOR_YUV2BGR_NV12);
                            pushImageToGat1400(imageMat, res);
                        }
                    }
#endif
                }
            }
            else
            {
                dlog_error("ai_app: 图片数据为空");
            }
        }
        else
        {
            dlog_error("ai_app: 边界检测-获取虚拟地址失败");
        }
    }
}

int CGroup2_Group4Detect::pnmAttributeAnalysis(cv::Mat &srcData, const std::vector<Group2Detect_NS::Result_S> &vecAllResult)
{
    std::vector<Group2Detect_NS::Result_S> vstPersonResult;
    std::vector<Group2Detect_NS::Result_S> vstMotorVehicleResult;
    std::vector<Group2Detect_NS::Result_S> vstNonMotorVehicleResult;

    for (auto &stResult : vecAllResult)
    {
        // dlog_debug("置信度：[%f]  ID：[%d]", stResult.fBoxConfidence, stResult.nID);

        /* 种类ID: 0-人 1-机动车 2-非机动车 */
        if (stResult.nID == 0 && stResult.fBoxConfidence >= 0.6)
        {
            vstPersonResult.push_back(stResult);
        }
        else if (stResult.nID == 1 && stResult.fBoxConfidence >= 0.6)
        {
            vstMotorVehicleResult.push_back(stResult);
        }
        else if (stResult.nID == 2 && stResult.fBoxConfidence >= 0.6)
        {
            vstNonMotorVehicleResult.push_back(stResult);
        }
    }

    m_nFrameCount++;

    /* 降低属性分析频率 */
    if (m_nFrameCount > DETECT_FRAME_THRESHOLD)
    {
        if (m_bPedestrianAttribute.load())
        {
            personAttributeAnalysis(srcData, vstPersonResult);
            m_vecLastFramePersonResult = vstPersonResult;
        }
        else
        {
            unInitPersonAttribute();
        }

        if (m_bNonMotorVehicleAttribute.load())
        {
            nonMotorvehicleAttributeAnalysis(srcData, vstNonMotorVehicleResult);
            m_vecLastFrameNonMotorvehicleResult = vstNonMotorVehicleResult;
        }
        else
        {
            unInitNonMotorizedAttribute();
        }
    }

    if (m_stAlgoLicensePlateCognitionCfg.bEnable || m_bMotorVehicleAttribute.load())
    {
        /* 车牌检测 + 车辆属性识别 */
        licensePlateDetectProcess(srcData, vstMotorVehicleResult, m_bMotorVehicleAttribute.load());
    }
    else
    {
        unInitMotorVehicleAttribute();
    }

    if (m_nFrameCount > DETECT_FRAME_THRESHOLD)
    {
        m_nFrameCount = 0;
    }

    return 0;
}

static bool isSameTarget(Group2Detect_NS::Result_S stLastFrameResult, Group2Detect_NS::Result_S stCurFrameResult)
{
    float fDx                  = 0;
    float fDy                  = 0;
    float fCenterPointDistance = 0;
    float fDistance1           = 0;
    float fDistance2           = 0;

    float fCurCenterPointX  = 0;
    float fCurCenterPointY  = 0;
    float fLastCenterPointX = 0;
    float fLastCenterPointY = 0;
    float fDiagonalLen      = 0;

    fDx          = stLastFrameResult.fX2 - stLastFrameResult.fX1;
    fDy          = stLastFrameResult.fY2 - stLastFrameResult.fY1;
    fDiagonalLen = sqrt(fDx * fDx + fDy * fDy);

    fDx        = stCurFrameResult.fX1 - stLastFrameResult.fX1;
    fDy        = stCurFrameResult.fY1 - stLastFrameResult.fY1;
    fDistance1 = sqrt(fDx * fDx + fDy * fDy);

    fDx        = stCurFrameResult.fX2 - stLastFrameResult.fX2;
    fDy        = stCurFrameResult.fY2 - stLastFrameResult.fY2;
    fDistance2 = sqrt(fDx * fDx + fDy * fDy);

    fCurCenterPointX     = (stCurFrameResult.fX1 + stCurFrameResult.fX2) * 0.5f;
    fCurCenterPointY     = (stCurFrameResult.fY1 + stCurFrameResult.fY2) * 0.5f;
    fLastCenterPointX    = (stLastFrameResult.fX1 + stLastFrameResult.fX2) * 0.5f;
    fLastCenterPointY    = (stLastFrameResult.fY1 + stLastFrameResult.fY2) * 0.5f;
    fDx                  = fCurCenterPointX - fLastCenterPointX;
    fDy                  = fCurCenterPointY - fLastCenterPointY;
    fCenterPointDistance = sqrt(fDx * fDx + fDy * fDy);

    /* 两个中心点的距离大于上一帧目标框对角线的0.3倍，判定为不是同一个目标 */
    if (fCenterPointDistance > fDiagonalLen * 0.3f)
    {
        return false;
    }

    /* 两个中心点的距离小于等于上一帧目标框对角线的0.3倍，判定为是同一个目标 */
    if (fCenterPointDistance <= fDiagonalLen * 0.25f)
    {
        return true;
    }

    /* 中心点很近，且至少一个角点接近 */
    if (fCenterPointDistance <= fDiagonalLen * 0.30f && (fDistance1 < fDiagonalLen * 0.3f || fDistance2 < fDiagonalLen * 0.3f))
    {
        return true;
    }

    return false;
}

int CGroup2_Group4Detect::personAttributeAnalysis(cv::Mat &srcData, std::vector<Group2Detect_NS::Result_S> vecPersonResult)
{
    if (!m_pPersonAttributeHandle)
    {
        if (!initPersonAttribute())
        {
            dlog_error("行人属性检测初始化失败");
            return -1;
        }
    }

    for (unsigned int i = 0; i < vecPersonResult.size(); i++)
    {
        bool bFlag = false;
        for (auto &stResult : m_vecLastFramePersonResult)
        {
            if (isSameTarget(stResult, vecPersonResult[i]))
            {
                bFlag = true;
                break;
            }
        }
        if (bFlag)
        {
            continue;
        }
        PresonAttribute_NS::InData_S              stInData{};
        std::vector<PresonAttribute_NS::Result_S> stOutData;

        // vecPersonResult[i].fX1 /= 1.25;
        // vecPersonResult[i].fY1 /= 1.25;

        // vecPersonResult[i].fX2 *= 1.25;
        // vecPersonResult[i].fY2 *= 1.25;

        // if (vecPersonResult[i].fX2 > m_nAiChnWith)
        // {
        //     vecPersonResult[i].fX2 = m_nAiChnWith;
        // }

        // if (vecPersonResult[i].fY2 > m_nAiChnHeigh)
        // {
        //     vecPersonResult[i].fY2 = m_nAiChnHeigh;
        // }

        cv::Rect roi(static_cast<int>(vecPersonResult[i].fX1), static_cast<int>(vecPersonResult[i].fY1), static_cast<int>(vecPersonResult[i].fX2 - vecPersonResult[i].fX1), static_cast<int>(vecPersonResult[i].fY2 - vecPersonResult[i].fY1));
        cv::Mat  cropped = srcData(roi).clone();
        // if (access("/test_personAttribute", F_OK) == 0)
        // {
        //     saveImage(cropped, "/mnt/event_image");
        // }

        cv::resize(
            cropped,
            stInData.inMat,
            cv::Size(m_nPersonAndNonMotorAttributeWidth, m_nPersonAndNonMotorAttributeHeight),
            0,
            0,
            cv::INTER_LINEAR);

        m_pPersonAttributeHandle->process(stInData, stOutData);

        if (stOutData.size())
        {
            /* 保存目标小图 */
            Common::Rect_S stRect;
            stRect.nX                    = (int)vecPersonResult[i].fX1;
            stRect.nY                    = (int)vecPersonResult[i].fY1;
            stRect.nWidth                = (int)(vecPersonResult[i].fX2 - vecPersonResult[i].fX1);
            stRect.nHeight               = (int)(vecPersonResult[i].fY2 - vecPersonResult[i].fY1);
            std::string strPersonPicture = saveCropImage(srcData, stRect, "Person");
            /* 保存全景大图 */
            std::string strCurrentPicture = saveFullImage(srcData, "Person");
            if (!strPersonPicture.empty() && !strCurrentPicture.empty())
            {
                /* 截取的是单个行人进行的行人属性分析，分析结果只有一个 */
                PresonAttribute_NS::Result_S stResult = stOutData.at(0);
                /* 行人抓拍信息推送 */
                pushPersonCaptureInfo(strCurrentPicture, strPersonPicture, stResult);
            }
        }
    }

    return 0;
}

int CGroup2_Group4Detect::motorvehicleAttributeAnalysis(cv::Mat &srcData, cv::Mat &cropped, LicensePlateCognition_NS::Result_S stActualResult)
{
    if (!m_pMotorVehicleAttributeHandle)
    {
        if (!initMotorVehicleAttribute())
        {
            dlog_error("机动车属性检测初始化失败");
            return -1;
        }
    }

    std::vector<VehicleAttribute_NS::Result_S> stOutData;
    VehicleAttribute_NS::InData_S              stInData{};

    cv::resize(
        cropped,
        stInData.inMat,
        cv::Size(m_nMotorVehicleAttributeWidth, m_nMotorVehicleAttributeHeight),
        0,
        0,
        cv::INTER_LINEAR);

    // if (access("/test_vehicleAttribute", F_OK) == 0)
    // {
    //     saveImage(cropped, "/mnt/event_image");
    // }

    if (m_pMotorVehicleAttributeHandle)
    {
        m_pMotorVehicleAttributeHandle->process(stInData, stOutData);
    }
    else
    {
        dlog_error("机动车属性识别句柄为空");
        return -1;
    }

    /* 仅保存车辆图片？/直接保存整张大图 */
    std::string strCurrentPicture = saveFullImage(srcData, "MotorVehicle");

    /* 保存车牌小图 */
    Common::Rect_S stRect;
    stRect.nX      = (int)stActualResult.fX;
    stRect.nY      = (int)stActualResult.fY;
    stRect.nWidth  = (int)stActualResult.fWidth;
    stRect.nHeight = (int)stActualResult.fHeight;

    std::string strTargetPicture = saveCropImage(srcData, stRect, "MotorVehicle");

    if (stOutData.size() && !strCurrentPicture.empty() && !strTargetPicture.empty())
    {
        /* 截取的是单个机动车进行的属性分析，分析结果正常只有一个 */
        VehicleAttribute_NS::Result_S stVehicleAttributeResult = stOutData.at(0);

        pushMotorvehicleCaptureInfo(strCurrentPicture, strTargetPicture, stActualResult.licensePlateNumber, stVehicleAttributeResult);
    }

    return 0;
}

int CGroup2_Group4Detect::nonMotorvehicleAttributeAnalysis(cv::Mat &srcData, std::vector<Group2Detect_NS::Result_S> vecNonMotorvehicleResult)
{
    if (!m_pNonMotorizedAttributeHandle)
    {
        if (!initNonMotorizedAttribute())
        {
            dlog_error("非机动车属性检测初始化失败");
            return -1;
        }
    }
    for (unsigned int i = 0; i < vecNonMotorvehicleResult.size(); i++)
    {
        bool bFlag = false;
        for (auto &stResult : m_vecLastFrameNonMotorvehicleResult)
        {
            if (isSameTarget(stResult, vecNonMotorvehicleResult[i]))
            {
                bFlag = true;
                break;
            }
        }
        if (bFlag)
        {
            continue;
        }

        NonMotorizedAttribute_NS::InData_S              stInData{};
        std::vector<NonMotorizedAttribute_NS::Result_S> stOutData;

        // vecNonMotorvehicleResult[i].fX1 /= 1.25;
        // vecNonMotorvehicleResult[i].fY1 /= 1.25;

        // vecNonMotorvehicleResult[i].fX2 *= 1.25;
        // vecNonMotorvehicleResult[i].fY2 *= 1.25;

        // if (vecNonMotorvehicleResult[i].fX2 > m_nAiChnWith)
        // {
        //     vecNonMotorvehicleResult[i].fX2 = m_nAiChnWith;
        // }

        // if (vecNonMotorvehicleResult[i].fY2 > m_nAiChnHeigh)
        // {
        //     vecNonMotorvehicleResult[i].fY2 = m_nAiChnHeigh;
        // }

        cv::Rect roi(static_cast<int>(vecNonMotorvehicleResult[i].fX1), static_cast<int>(vecNonMotorvehicleResult[i].fY1), static_cast<int>(vecNonMotorvehicleResult[i].fX2 - vecNonMotorvehicleResult[i].fX1), static_cast<int>(vecNonMotorvehicleResult[i].fY2 - vecNonMotorvehicleResult[i].fY1));
        cv::Mat  cropped = srcData(roi).clone();
        // if (access("/test_NonMotorAttribute", F_OK) == 0)
        // {
        //     saveImage(cropped, "/mnt/event_image");
        // }

        cv::resize(
            cropped,
            stInData.inMat,
            cv::Size(m_nPersonAndNonMotorAttributeWidth, m_nPersonAndNonMotorAttributeHeight),
            0,
            0,
            cv::INTER_LINEAR);

        m_pNonMotorizedAttributeHandle->process(stInData, stOutData);

        if (stOutData.size())
        {
            /* 保存目标小图 */
            Common::Rect_S stRect;
            stRect.nX                    = (int)vecNonMotorvehicleResult[i].fX1;
            stRect.nY                    = (int)vecNonMotorvehicleResult[i].fY1;
            stRect.nWidth                = (int)(vecNonMotorvehicleResult[i].fX2 - vecNonMotorvehicleResult[i].fX1);
            stRect.nHeight               = (int)(vecNonMotorvehicleResult[i].fY2 - vecNonMotorvehicleResult[i].fY1);
            std::string strTargetPicture = saveCropImage(srcData, stRect, "NonMotorvehicle");
            /* 保存全景大图 */
            std::string strCurrentPicture = saveFullImage(srcData, "NonMotorvehicle");

            if (!strCurrentPicture.empty() && !strTargetPicture.empty())
            {
                /* 截取的是单个非机动车进行的属性分析，分析结果只有一个 */
                NonMotorizedAttribute_NS::Result_S stResult = stOutData.at(0);
                /* 非机动车抓拍信息推送 */
                pushNonMotorvehicleCaptureInfo(strCurrentPicture, strTargetPicture, stResult);
            }
        }
    }

    return 0;
}

int CGroup2_Group4Detect::licensePlateDetectProcess(cv::Mat &srcData, const std::vector<Group2Detect_NS::Result_S> &vstResult, bool bMotorVehicleAttribute)
{
    if (!m_stAlgoLicensePlateCognitionCfg.bEnable && !bMotorVehicleAttribute)
    {
        return 0;
    }

    bool bIsAlarm = false;

    LicensePlateCognition_NS::InData_S              stInData{};
    std::vector<LicensePlateCognition_NS::Result_S> vOutData;

    /* 所有的车牌检测结果 */
    std::vector<LicensePlateCognition_NS::Result_S> vecLicensePlateCognitionResult;

    stInData.stParam.fBoxThreshold = sensitivityToConfidence(m_stAlgoLicensePlateCognitionCfg.stRule.nSensitivity);

    std::vector<Group2Detect_NS::Result_S> vecVehicleResult = vstResult;
    // std::vector<Group2Detect_NS::Result_S> vecVehicleResult;
    // for (auto &stResult : vstResult)
    // {
    //     // 0-人 1-机动车 2-非机动车
    //     if (stResult.nID == 1)
    //     {
    //         vecVehicleResult.push_back(stResult);
    //     }
    // }

    for (unsigned int i = 0; i < vecVehicleResult.size(); i++)
    {
        vecVehicleResult[i].fX1 /= 1.25;
        vecVehicleResult[i].fY1 /= 1.25;

        vecVehicleResult[i].fX2 *= 1.25;
        vecVehicleResult[i].fY2 *= 1.25;

        if (vecVehicleResult[i].fX2 > m_nAiChnWith)
        {
            vecVehicleResult[i].fX2 = m_nAiChnWith;
        }

        if (vecVehicleResult[i].fY2 > m_nAiChnHeigh)
        {
            vecVehicleResult[i].fY2 = m_nAiChnHeigh;
        }

        float fRoiW = vecVehicleResult[i].fX2 - vecVehicleResult[i].fX1;
        float fRoiH = vecVehicleResult[i].fY2 - vecVehicleResult[i].fY1;

        if (fRoiW <= 1.0f || fRoiH <= 1.0f)
        {
            dlog_warn("Invalid person roi: w=%f h=%f", fRoiW, fRoiH);
            continue;
        }

        cv::Rect roi(static_cast<int>(vecVehicleResult[i].fX1), static_cast<int>(vecVehicleResult[i].fY1), static_cast<int>(vecVehicleResult[i].fX2 - vecVehicleResult[i].fX1), static_cast<int>(vecVehicleResult[i].fY2 - vecVehicleResult[i].fY1));
        cv::Mat  cropped = srcData(roi).clone();
        // if (access("/group4SaveImageDebug", F_OK) == 0)
        // {
        //     saveImage(cropped, "/mnt/event_image");
        // }

        /* 分辨率大小转换 */
        cv::resize(
            cropped,
            stInData.inMat,
            cv::Size(m_nLicensePlateWidth, m_nLicensePlateHeight),
            0,
            0,
            cv::INTER_LINEAR);

        if (!m_pLicensePlateHandle)
        {
            dlog_error("车牌识别句柄为空");
            return -1;
        }

        m_pLicensePlateHandle->process(stInData, vOutData);

        if (vOutData.size())
        {
            bIsAlarm = true;
        }

        float fWRatio = static_cast<float>(m_nLicensePlateWidth) / fRoiW;
        float fHRatio = static_cast<float>(m_nLicensePlateHeight) / fRoiH;

        for (auto &stResult : vOutData)
        {
            LicensePlateCognition_NS::Result_S stActualResult;
            stActualResult         = stResult;
            stActualResult.fX      = vecVehicleResult[i].fX1 + stResult.fX / fWRatio;
            stActualResult.fWidth  = stResult.fWidth / fWRatio;
            stActualResult.fY      = vecVehicleResult[i].fY1 + stResult.fY / fHRatio;
            stActualResult.fHeight = stResult.fHeight / fHRatio;

            vecLicensePlateCognitionResult.push_back(stActualResult);

            /* 车辆属性检测 + 降低属性分析频率 */
            if (bMotorVehicleAttribute && m_nFrameCount > DETECT_FRAME_THRESHOLD)
            {
                bool bFlag = false;
                for (auto &stResult : m_vecLastFrameMotorvehicleResult)
                {
                    if (stResult == stActualResult.licensePlateNumber)
                    {
                        bFlag = true;
                        break;
                    }
                }
                if (bFlag)
                {
                    continue;
                }

                /* 把车辆图片进行机动车属性分析 */
                motorvehicleAttributeAnalysis(srcData, cropped, stActualResult);
            }
        }
    }

    if (bMotorVehicleAttribute && m_nFrameCount > DETECT_FRAME_THRESHOLD)
    {
        m_vecLastFrameMotorvehicleResult.clear();
        for (auto &stResult : vOutData)
        {
            m_vecLastFrameMotorvehicleResult.push_back(stResult.licensePlateNumber);
        }
    }

    /* 车牌识别动态分析 */
    dynamicAnalysis(vecLicensePlateCognitionResult);

    if (m_stAlgoLicensePlateCognitionCfg.bEnable)
    {
        m_LicensePlateStateMachine.handleAlarmState(bIsAlarm, Event::Type_E::PLATE_NUMBER);
    }

    return 0;
}

int CGroup2_Group4Detect::group4DetectProcess(cv::Mat &srcData, const std::vector<Group2Detect_NS::Result_S> &vstResult)
{
    if (!m_stAlgoSleepOnDutyCfg.bEnable && !m_stAlgoTripCfg.bEnable && !m_stAlgoSmokingCfg.bEnable && !m_stPhoneUsageCfg.bEnable)
    {
        return 0;
    }

    if (srcData.empty())
    {
        dlog_error("图片数据为空");
        return -1;
    }

    Group4Detect_NS::InData_S  stInData{};
    Group4Detect_NS::OutData_S stOutData;

    std::vector<Group4Detect_NS::Result_S> vecSleepAndTripResult;
    std::vector<Group4Detect_NS::Result_S> vecSmokingAndPhoneResult;

    // std::vector<std::vector<Group4Detect_NS::Result_S>> vecSmokingAndPhoneAllResult;
    std::vector<Group4Detect_NS::Result_S> vecSmokingAndPhoneActualResult;
#if CAP_EXHIBITION_OSD_PANEL
    /* 当前帧命中玩手机事件的人员列表。 */
    std::vector<Group4PersonPanel_S> vecPhonePanelMatches;
    /* 当前帧命中抽烟事件的人员列表。 */
    std::vector<Group4PersonPanel_S> vecSmokingPanelMatches;
#endif

    if (m_stAlgoSleepOnDutyCfg.bEnable || m_stAlgoTripCfg.bEnable)
    {
        stInData.inMat = srcData;
        /* 睡觉识别 */
        if (m_stAlgoSleepOnDutyCfg.bEnable)
        {
            stInData.stParam.stSleepParam.bEnable      = true;
            stInData.stParam.stSleepParam.fConfidence  = sensitivityToConfidence(m_stAlgoSleepOnDutyCfg.stRule.nSensitivity);
            stInData.stParam.stSleepParam.nDetectFrame = sensitivityToFrames(m_stAlgoSleepOnDutyCfg.stRule.nSensitivity);
            if (access("/group4Debug", F_OK) == 0)
            {
                printf(" [%s][%d]=== 睡觉识别 %d -> %f %d\n", __FILE__, __LINE__, m_stAlgoSleepOnDutyCfg.stRule.nSensitivity, stInData.stParam.stSleepParam.fConfidence, stInData.stParam.stSleepParam.nDetectFrame);
            }
        }

        /* 摔倒识别 */
        if (m_stAlgoTripCfg.bEnable)
        {
            stInData.stParam.stFallParam.bEnable      = true;
            stInData.stParam.stFallParam.fConfidence  = sensitivityToConfidence(m_stAlgoTripCfg.stRule.nSensitivity);
            stInData.stParam.stFallParam.nDetectFrame = sensitivityToFrames(m_stAlgoTripCfg.stRule.nSensitivity);
            if (access("/group4Debug", F_OK) == 0)
            {
                printf(" [%s][%d]=== 摔倒识别 %d -> %f %d\n", __FILE__, __LINE__, m_stAlgoTripCfg.stRule.nSensitivity, stInData.stParam.stFallParam.fConfidence, stInData.stParam.stFallParam.nDetectFrame);
            }
        }
        if (m_pGroup4Handle)
        {
            m_pGroup4Handle->process(false, stInData, vecSleepAndTripResult, &stOutData);
        }
    }

    std::vector<Group2Detect_NS::Result_S> vecPersonResult;

    if (m_stAlgoSmokingCfg.bEnable || m_stPhoneUsageCfg.bEnable)
    {
        for (auto &stResult : vstResult)
        {
            if (stResult.nID == 0)
            {
                vecPersonResult.push_back(stResult);
            }
        }

        for (unsigned int i = 0; i < vecPersonResult.size(); i++)
        {
#if CAP_EXHIBITION_OSD_PANEL
            /* 当前 ROI 对应的父级人员框。 */
            const Common::RectInfo_S stPersonRect = to_person_rect(vecPersonResult[i]);
#endif
            vecPersonResult[i].fX1 /= 1.25;
            vecPersonResult[i].fY1 /= 1.25;

            vecPersonResult[i].fX2 *= 1.25;
            vecPersonResult[i].fY2 *= 1.25;

            if (vecPersonResult[i].fX2 > m_nAiChnWith)
            {
                vecPersonResult[i].fX2 = m_nAiChnWith;
            }

            if (vecPersonResult[i].fY2 > m_nAiChnHeigh)
            {
                vecPersonResult[i].fY2 = m_nAiChnHeigh;
            }

            cv::Rect roi(static_cast<int>(vecPersonResult[i].fX1), static_cast<int>(vecPersonResult[i].fY1), static_cast<int>(vecPersonResult[i].fX2 - vecPersonResult[i].fX1), static_cast<int>(vecPersonResult[i].fY2 - vecPersonResult[i].fY1));
            cv::Mat  cropped = srcData(roi).clone();
            // if (access("/group4SaveImageDebug", F_OK) == 0)
            // {
            //     saveImage(cropped, "/mnt/event_image");
            // }

            cv::resize(
                cropped,
                stInData.inMat,
                cv::Size(m_nWidth, m_nHeight),
                0,
                0,
                cv::INTER_LINEAR);

            if (m_stAlgoSmokingCfg.bEnable)
            {
                stInData.stParam.stCigaretteDetectParam.bEnable         = true;
                stInData.stParam.stCigaretteDetectParam.fConfidence     = sensitivityToConfidence(m_stAlgoSmokingCfg.stRule.nSensitivity);
                stInData.stParam.stCigaretteDetectParam.nDetectDuration = sensitivityToDuration(m_stAlgoSmokingCfg.stRule.nSensitivity);
                if (access("/group4Debug", F_OK) == 0)
                {
                    printf(" [%s][%d]=== 吸烟识别 %d -> %f %d\n", __FILE__, __LINE__, m_stAlgoSmokingCfg.stRule.nSensitivity, stInData.stParam.stCigaretteDetectParam.fConfidence, stInData.stParam.stCigaretteDetectParam.nDetectDuration);
                }
            }

            if (m_stPhoneUsageCfg.bEnable)
            {
                stInData.stParam.stPhoneParam.bEnable         = true;
                stInData.stParam.stPhoneParam.fConfidence     = sensitivityToConfidence(m_stPhoneUsageCfg.stRule.nSensitivity);
                stInData.stParam.stPhoneParam.nDetectDuration = sensitivityToDuration(m_stPhoneUsageCfg.stRule.nSensitivity);
                if (access("/group4Debug", F_OK) == 0)
                {
                    printf(" [%s][%d]=== 玩手机识别 %d -> %f %d\n", __FILE__, __LINE__, m_stPhoneUsageCfg.stRule.nSensitivity, stInData.stParam.stPhoneParam.fConfidence, stInData.stParam.stPhoneParam.nDetectDuration);
                }
            }

            if (m_pGroup4Handle)
            {
                m_pGroup4Handle->process(true, stInData, vecSmokingAndPhoneResult, &stOutData);
            }

#if CAP_EXHIBITION_OSD_PANEL
            /* 当前人员是否命中过玩手机事件。 */
            bool bPhoneDetected = false;
            /* 当前人员是否命中过抽烟事件。 */
            bool bSmokingDetected = false;
            /* 当前人员玩手机事件的最高置信度。 */
            float fPhoneConfidence = 0.0f;
            /* 当前人员抽烟事件的最高置信度。 */
            float fSmokingConfidence = 0.0f;
#endif
            float fRoiW = vecPersonResult[i].fX2 - vecPersonResult[i].fX1;
            float fRoiH = vecPersonResult[i].fY2 - vecPersonResult[i].fY1;

            if (fRoiW <= 1.0f || fRoiH <= 1.0f)
            {
                dlog_warn("Invalid person roi: w=%f h=%f", fRoiW, fRoiH);
                continue;
            }

            float fWRatio = static_cast<float>(m_nWidth) / fRoiW;
            float fHRatio = static_cast<float>(m_nHeight) / fRoiH;

            for (auto &stResult : vecSmokingAndPhoneResult)
            {
                Group4Detect_NS::Result_S stActualResult;
                stActualResult.fX1 = vecPersonResult[i].fX1 + stResult.fX1 / fWRatio;
                stActualResult.fX2 = vecPersonResult[i].fX1 + stResult.fX2 / fWRatio;
                stActualResult.fY1 = vecPersonResult[i].fY1 + stResult.fY1 / fHRatio;
                stActualResult.fY2 = vecPersonResult[i].fY1 + stResult.fY2 / fHRatio;
#if CAP_EXHIBITION_OSD_PANEL
                stActualResult.fBoxConfidence = stResult.fBoxConfidence;
                stActualResult.nClassId       = stResult.nClassId;
#endif

                vecSmokingAndPhoneActualResult.push_back(stActualResult);
#if CAP_EXHIBITION_OSD_PANEL
                if (stResult.nClassId == Group4Detect_NS::CGroup4DetectV1_0::PHONE)
                {
                    bPhoneDetected   = true;
                    fPhoneConfidence = std::max(fPhoneConfidence, stResult.fBoxConfidence);
                }
                else if (stResult.nClassId == Group4Detect_NS::CGroup4DetectV1_0::CIGARETTE)
                {
                    bSmokingDetected   = true;
                    fSmokingConfidence = std::max(fSmokingConfidence, stResult.fBoxConfidence);
                }
#endif
                // dlog_debug(" (%f, %f) === {(%f, %f)(%f, %f)}   {(%f, %f)(%f, %f)}", vecPersonResult[i].fX1, vecPersonResult[i].fY1, stResult.fX1, stResult.fY1, stResult.fX2, stResult.fY2, stActualResult.fX1, stActualResult.fY1, stActualResult.fX2, stActualResult.fY2);
            }
#if CAP_EXHIBITION_OSD_PANEL
            if (bPhoneDetected)
            {
                vecPhonePanelMatches.push_back({stPersonRect, fPhoneConfidence});
            }
            if (bSmokingDetected)
            {
                vecSmokingPanelMatches.push_back({stPersonRect, fSmokingConfidence});
            }
#endif
        }
    }

    std::vector<Group4Detect_NS::Result_S> vecAllResult;

    /* 预留空间 */
    vecAllResult.reserve(vecSleepAndTripResult.size() + vecSmokingAndPhoneActualResult.size());

    vecAllResult.insert(vecAllResult.end(),
                        std::make_move_iterator(vecSleepAndTripResult.begin()),
                        std::make_move_iterator(vecSleepAndTripResult.end()));

    vecAllResult.insert(vecAllResult.end(),
                        std::make_move_iterator(vecSmokingAndPhoneActualResult.begin()),
                        std::make_move_iterator(vecSmokingAndPhoneActualResult.end()));

    /* 相关事件动态分析 */
    dynamicAnalysis(vecAllResult);

#if CAP_EXHIBITION_OSD_PANEL
    /* 玩手机事件的展会面板帧。 */
    OsdPanel::PanelFrame_S stPhonePanelFrame;
    if (prepare_exhibition_panel_frame(&stPhonePanelFrame, Event::Type_E::PHONE_USAGE, m_nWidth, m_nHeight))
    {
        upsert_exhibition_panel_item(&stPhonePanelFrame,
                                     build_group4_count_panel_item(Event::Type_E::PHONE_USAGE,
                                                                   vecPhonePanelMatches,
                                                                   stOutData.bPhone));
    }

    /* 抽烟事件的展会面板帧。 */
    OsdPanel::PanelFrame_S stSmokingPanelFrame;
    if (prepare_exhibition_panel_frame(&stSmokingPanelFrame, Event::Type_E::SMOKING, m_nWidth, m_nHeight))
    {
        upsert_exhibition_panel_item(&stSmokingPanelFrame,
                                     build_group4_count_panel_item(Event::Type_E::SMOKING,
                                                                   vecSmokingPanelMatches,
                                                                   stOutData.bCigarette));
    }

    /* 当前帧最终需要发送到 OSD 的面板结果。 */
    const OsdPanel::PanelFrame_S *pstSelectedPanelFrame = select_group4_panel_frame(
        stPhonePanelFrame,
        stOutData.bPhone,
        static_cast<int>(vecPhonePanelMatches.size()),
        stSmokingPanelFrame,
        stOutData.bCigarette,
        static_cast<int>(vecSmokingPanelMatches.size()));
    if (pstSelectedPanelFrame)
    {
        send_panelResult_to_osd(*pstSelectedPanelFrame);
    }
#endif

    processGroup4Detect(stOutData);

    return 0;
}

void CGroup2_Group4Detect::processGroup4Detect(const Group4Detect_NS::OutData_S &stGroup4OutData)
{
    if (m_stAlgoSmokingCfg.bEnable)
    {
        m_SmokingStateMachine.handleAlarmState(stGroup4OutData.bCigarette, Event::Type_E::SMOKING);
    }

    if (m_stAlgoSleepOnDutyCfg.bEnable)
    {
        m_SleepOnDutyStateMachine.handleAlarmState(stGroup4OutData.bSleep, Event::Type_E::SLEEP_ON_DUTY);
    }

    if (m_stPhoneUsageCfg.bEnable)
    {
        m_PhoneUsageStateMachine.handleAlarmState(stGroup4OutData.bPhone, Event::Type_E::PHONE_USAGE);
    }

    if (m_stAlgoTripCfg.bEnable)
    {
        m_TripStateMachine.handleAlarmState(stGroup4OutData.bFall, Event::Type_E::PERSON_TRIP);
    }

    return;
}

void CGroup2_Group4Detect::processGroup2Detect(const Group2Detect_NS::OutData_S &stGroup2OutData)
{

    if (m_stAlgoCrossCfg.bEnable)
    {
        m_CrossAlarmStateMachine.handleAlarmState(stGroup2OutData.bTripLineType, Event::Type_E::LINE_CROSSING);
    }
    if (m_stAlgoIntruCfg.bEnable)
    {
        m_IntruAlarmStateMachine.handleAlarmState(stGroup2OutData.bIntrusionFlag, Event::Type_E::INTRUSION);
    }
    if (m_stAlgoEntryCfg.bEnable)
    {
        m_EntryAlarmStateMachine.handleAlarmState(stGroup2OutData.bEntryFlag, Event::Type_E::ENTER_REGION);
    }
    if (m_stAlgoExitCfg.bEnable)
    {
        m_ExitAlarmStateMachine.handleAlarmState(stGroup2OutData.bLeaveFlag, Event::Type_E::LEAVE_REGION);
    }
    if (m_stLoiteringCfg.bEnable)
    {
        m_LoiteringAlarmStateMachine.handleAlarmState(stGroup2OutData.bLoiteringFlag, Event::Type_E::LOITERING_DETECT);
    }
    if (m_stFenceClimbingCfg.bEnable)
    {
        m_FenceClimbingAlarmStateMachine.handleAlarmState(stGroup2OutData.bFenceClimbFlag, Event::Type_E::FENCE_CLIMBING);
    }
    if (m_stPersonFallDownCfg.bEnable)
    {
        m_PersonFallDownStateMachine.handleAlarmState(stGroup2OutData.bPersonFalldownFlag, Event::Type_E::PERSON_FALL_DOWN);
    }

    if (m_stLeavePostCfg.bEnable)
    {
        m_LeavePostAlarmStateMachine.handleAlarmState(stGroup2OutData.bLeavePostFlag, Event::Type_E::LEAVE_POST);
    }
    if (m_stPedestrianIntrusionCfg.bEnable)
    {
        m_PedestrianIntrusionAlarmStateMachine.handleAlarmState(stGroup2OutData.bPedestrianIntrusionFlag, Event::Type_E::PEDESTRIAN_INTRUSION);
    }
    if (m_stCrowdGatheringDetCfg.bEnable)
    {
        m_CrowdGatheringAlarmStateMachine.handleAlarmState(stGroup2OutData.bCrowdGatheringDetParamFlag, Event::Type_E::CROWD_GATHERING);
    }

    if (m_stAlgoElectricScooterCfg.bEnable)
    {
        m_ElectricScooterStateMachine.handleAlarmState(stGroup2OutData.bElectricScooter, Event::Type_E::ELECTRIC_VEHICLE_IN_ELEVATOR);
    }

    if (m_stAlgoEmergencyLaneOccupancyCfg.bEnable)
    {
        m_EmergencyLaneOccupancyAlarmStateMachine.handleAlarmState(stGroup2OutData.bEmergencyLaneOccupancyFlag, Event::Type_E::EMERGENCY_LANE_OCCUPANCY);
    }
    if (m_stAlgoNonMotorVehicleIntrusionCfg.bEnable)
    {
        m_NonMotorVehicleIntrusionAlarmStateMachine.handleAlarmState(stGroup2OutData.bNonMotorVehicleIntrusionFlag, Event::Type_E::NON_MOTOR_VEHICLE_INTRUSION);
    }

    if (m_stDrivingAgainstTrafficDetectionCfg.bEnable)
    {
        m_DrivingAgainstTrafficStateMachine.handleAlarmState(stGroup2OutData.bDrivingAgainstTrafficFlag, Event::Type_E::REVERSE_DIRECTION);
    }
    if (m_stAlgoParkingDetectionCfg.bEnable)
    {
        m_IllegalParkingStateMachine.handleAlarmState(stGroup2OutData.bParkingFlag, Event::Type_E::PARKING_DETECT);
    }
    if (m_stAlgoIllegalLaneChangeDetectionCfg.bEnable)
    {
        m_IllegalLaneChangeStateMachine.handleAlarmState(stGroup2OutData.bIllegalLaneChangeFlag, Event::Type_E::ILLEGAL_LANE_CHANGE);
    }
    if (m_stAlgoCongestionDetectionCfg.bEnable)
    {
        m_CongestionStateMachine.handleAlarmState(stGroup2OutData.bCongestionFlag, Event::Type_E::CONGESTION);
    }

    return;
}

int CGroup2_Group4Detect::dynamicAnalysis(const std::vector<Group2Detect_NS::Result_S> &vecAllResult, bool bPersonDetect, bool bMotorVehicle, bool bNonMotorVehicle)
{
    std::vector<Common::RectInfo_S> vstRectInfo;

    for (auto &stResult : vecAllResult)
    {
        Common::RectInfo_S stRectInfo;
        stRectInfo.nX1 = (int)stResult.fX1;
        stRectInfo.nY1 = (int)stResult.fY1;
        stRectInfo.nX2 = (int)(stResult.fX2);
        stRectInfo.nY2 = (int)(stResult.fY2);

        if(stResult.fBoxConfidence < 0.5)
        {
            continue;
        }

        if (bPersonDetect)
        {
            /* 0-人 1-机动车 2-非机动车 */
            if (stResult.nID == 0)
            {
                vstRectInfo.push_back(stRectInfo);
                continue;
            }
        }

        if (bMotorVehicle)
        {
            if (stResult.nID == 1)
            {
                vstRectInfo.push_back(stRectInfo);
                continue;
            }
        }

        if (bNonMotorVehicle)
        {
            if (stResult.nID == 2)
            {
                vstRectInfo.push_back(stRectInfo);
            }
        }
    }

    if (vstRectInfo.size())
    {
        send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectInfo);
    }
    return 0;
}

int CGroup2_Group4Detect::dynamicAnalysis(const std::vector<Group4Detect_NS::Result_S> &vecResult)
{
    std::vector<Common::RectInfo_S> vstRectInfo;

    float fWRatio = static_cast<float>(m_nWidth) / PIXEL_WIDTH_1280;
    float fHRatio = static_cast<float>(m_nHeight) / PIXEL_HEIGHT_720;

    for (auto &stResult : vecResult)
    {
        Common::RectInfo_S stRectInfo;
        stRectInfo.nX1 = (int)stResult.fX1 * fWRatio;
        stRectInfo.nY1 = (int)stResult.fY1 * fHRatio;
        stRectInfo.nX2 = (int)(stResult.fX2 * fWRatio);
        stRectInfo.nY2 = (int)(stResult.fY2 * fHRatio);
        vstRectInfo.push_back(stRectInfo);
    }
    if (vstRectInfo.size())
    {
        send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectInfo);
    }
    return 0;
}

int CGroup2_Group4Detect::dynamicAnalysis(const std::vector<LicensePlateCognition_NS::Result_S> &vecResult)
{
    std::vector<Common::RectInfo_S> vstRectInfo;

    float fWRatio = static_cast<float>(m_nWidth) / PIXEL_WIDTH_1280;
    float fHRatio = static_cast<float>(m_nHeight) / PIXEL_HEIGHT_720;

    for (auto &stResult : vecResult)
    {
        Common::RectInfo_S stRectInfo;
        stRectInfo.nX1 = (int)stResult.fX * fWRatio;
        stRectInfo.nY1 = (int)stResult.fY * fHRatio;
        stRectInfo.nX2 = (int)(stResult.fX + stResult.fWidth) * fWRatio;
        stRectInfo.nY2 = (int)(stResult.fY + stResult.fHeight) * fHRatio;
        vstRectInfo.push_back(stRectInfo);
    }
    if (vstRectInfo.size())
    {
        send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectInfo);
    }
    return 0;
}

int CGroup2_Group4Detect::saveToDatebase(const std::string &strFilename, Event::Type_E eEventType)
{
    using namespace Db;
    Capture_NS::CaptureInfo_S stInfo;

    std::string strCurrentDate = TimeUtils_NS::get_currentDateWithDash();
    std::string strCurrentTime = TimeUtils_NS::get_currentTimeWithColon();

    stInfo.nChnId       = 0;
    stInfo.strImagePath = strFilename;
    stInfo.strStartTime = strCurrentDate + " " + strCurrentTime;
    stInfo.strEndTime   = stInfo.strStartTime;
    stInfo.enType       = eEventType;
    try
    {
        stInfo.nImageSize = std::filesystem::file_size(strFilename);
    } catch (...)  // 文件不存在或错误
    {
        dlog_warn("文件不存在");
        stInfo.nImageSize = 0;
    }

    /* 添加图片信息至数据库表 */
    CCaptureDatabase::instance()->add(stInfo);

    /* 更新图片数量、总大小至数据库表 */
    Capture_NS::CaptureDirInfo_S stDirInfo;
    stDirInfo.nChnId = 0;
    /* 获取当前信息 */
    int nRet = CCaptureDatabase::instance()->get_itemInfo(stDirInfo);
    stDirInfo.nTotalSize += (long long)stInfo.nImageSize;
    stDirInfo.nCount++;

    if (nRet < 0)
    {
        /* 未创建表数据，进行添加 */
        CCaptureDatabase::instance()->add(stDirInfo);
    }
    else
    {
        /* 已经存在表数据，进行更新 */
        CCaptureDatabase::instance()->update(stDirInfo);
    }

    return 0;
}

std::string CGroup2_Group4Detect::saveFullImage(cv::Mat image, std::string strPicType)
{
    if (image.empty() || SD_CARD_STATUS_E::NORMAL != CStorageManage::instance()->get_SdCardStatus())
    {
        return "";
    }

    std::string strCurrentDate = TimeUtils_NS::get_currentDate();

    std::string fullPath = std::string(CAPTURE_PATH) + "/" + strCurrentDate;

    struct stat info;
    /* 目录不存在 */
    if (stat(fullPath.c_str(), &info) != 0)
    {
        std::string strCmd = "mkdir -p \"" + fullPath + "\"";
        int         nRet   = system(strCmd.c_str());

        if (nRet != 0)
        {
            dlog_error("[%s]命令执行失败", strCmd.c_str());
            return "";
        }
    }
    cv::Mat outImage;
    cv::cvtColor(image, outImage, cv::COLOR_RGB2BGR);
    // /* 添加叠加数据 */
    // addFaceOverlayInfo(image);
    /* 抓图路径 + 日期（YYYYMMDD）+ 时间（HHMMSS）+ 事件类型 + 计数值.jpg */
    std::string strFilename = std::string(CAPTURE_PATH) + "/" + strCurrentDate + "/" + strCurrentDate + "_" +
                              TimeUtils_NS::get_currentTimeMs() + "_" +
                              //   std::to_string(static_cast<int>(Event::Type_E::PLATE_NUMBER)) + "_" +
                              strPicType + "_" +
                              std::to_string(int(Alarm::LinkageType::UPLOAD_PANORAMIC_IMAGE)) + ".jpg";
    dlog_debug("全景大图] 保存全景大图[%s]", strFilename.c_str());
    if (cv::imwrite(strFilename, outImage))
    {
        return strFilename;
    }
    else
    {
        return "";
    }
}

std::string CGroup2_Group4Detect::saveCropImage(cv::Mat image, Common::Rect_S stRect, std::string strPicType)
{
    if (image.empty() || SD_CARD_STATUS_E::NORMAL != CStorageManage::instance()->get_SdCardStatus())
    {
        return "";
    }

    std::string strCurrentDate = TimeUtils_NS::get_currentDate();

    std::string fullPath = std::string(CAPTURE_PATH) + "/" + strCurrentDate;

    struct stat info;
    /* 目录不存在 */
    if (stat(fullPath.c_str(), &info) != 0)
    {
        std::string strCmd = "mkdir -p \"" + fullPath + "\"";
        int         nRet   = system(strCmd.c_str());

        if (nRet != 0)
        {
            dlog_error("[%s]命令执行失败", strCmd.c_str());
            return "";
        }
    }
    /* 抓图路径 + 日期（YYYYMMDD）+ 时间（HHMMSS）+ 事件类型 + 计数值.jpg */
    std::string strFilename = std::string(CAPTURE_PATH) + "/" + strCurrentDate + "/" + strCurrentDate + "_" +
                              TimeUtils_NS::get_currentTimeMs() + "_" +
                              //   std::to_string(static_cast<int>(Event::Type_E::FACE_CAPTURE)) + "_" +
                              strPicType + "_" +
                              std::to_string(int(Alarm::LinkageType::UPLOAD_TARGET_IMAGE)) + ".jpg";
    dlog_debug("[目标小图] 保存目标小图[%s]", strFilename.c_str());
    cv::Mat  outImage;
    cv::Rect roi(stRect.nX, stRect.nY, stRect.nWidth, stRect.nHeight);
    cv::cvtColor(image, outImage, cv::COLOR_RGB2BGR);
    cv::Mat subImage = outImage(roi).clone();

    if (cv::imwrite(strFilename, subImage))
    {
        return strFilename;
    }
    else
    {
        return "";
    }
}

void CGroup2_Group4Detect::pushPersonCaptureInfo(const std::string &strCurrentPicture, const std::string &strPersonPicture, const PresonAttribute_NS::Result_S &stResult)
{
    Alarm::PersonAlarmInfo_S stPersonAlarmInfo;

    stPersonAlarmInfo.stPersonAlarmAttribute.bIsMale = stResult.bIsMale;

    if (stResult.bBackPack || stResult.bHandBag || stResult.bPostManBag)
    {
        stPersonAlarmInfo.stPersonAlarmAttribute.bBag = true;
    }

    if (stResult.nAgeLabel == 0)
    {
        stPersonAlarmInfo.stPersonAlarmAttribute.nAgeLabel = 1;
    }
    else if (stResult.nAgeLabel == 1)
    {
        stPersonAlarmInfo.stPersonAlarmAttribute.nAgeLabel = 2;
    }
    else if (stResult.nAgeLabel == 2 || stResult.nAgeLabel == 3)
    {
        stPersonAlarmInfo.stPersonAlarmAttribute.nAgeLabel = 3;
    }
    else
    {
        stPersonAlarmInfo.stPersonAlarmAttribute.nAgeLabel = 4;
    }

    switch (stResult.nBottomColorLabel)
    {
    case 0:
        stPersonAlarmInfo.stPersonAlarmAttribute.eBottomColorLabel = Alarm::PMNAttributeColor_E::BLACK;
        break;
    case 1:
        stPersonAlarmInfo.stPersonAlarmAttribute.eBottomColorLabel = Alarm::PMNAttributeColor_E::GRAY;
        break;
    case 2:
        stPersonAlarmInfo.stPersonAlarmAttribute.eBottomColorLabel = Alarm::PMNAttributeColor_E::ORANGE;
        break;
    case 3:
        stPersonAlarmInfo.stPersonAlarmAttribute.eBottomColorLabel = Alarm::PMNAttributeColor_E::PINK;
        break;
    case 4:
        stPersonAlarmInfo.stPersonAlarmAttribute.eBottomColorLabel = Alarm::PMNAttributeColor_E::RED;
        break;
    case 5:
        stPersonAlarmInfo.stPersonAlarmAttribute.eBottomColorLabel = Alarm::PMNAttributeColor_E::WHITE;
        break;
    case 6:
        stPersonAlarmInfo.stPersonAlarmAttribute.eBottomColorLabel = Alarm::PMNAttributeColor_E::YELLOW;
        break;
    default:
        stPersonAlarmInfo.stPersonAlarmAttribute.eBottomColorLabel = Alarm::PMNAttributeColor_E::UNKNOW;
        break;
    }

    switch (stResult.nTopColorLabel)
    {
    case 0:
        stPersonAlarmInfo.stPersonAlarmAttribute.eTopColorLabel = Alarm::PMNAttributeColor_E::BLACK;
        break;
    case 1:
        stPersonAlarmInfo.stPersonAlarmAttribute.eTopColorLabel = Alarm::PMNAttributeColor_E::GRAY;
        break;
    case 2:
        stPersonAlarmInfo.stPersonAlarmAttribute.eTopColorLabel = Alarm::PMNAttributeColor_E::ORANGE;
        break;
    case 3:
        stPersonAlarmInfo.stPersonAlarmAttribute.eTopColorLabel = Alarm::PMNAttributeColor_E::PINK;
        break;
    case 4:
        stPersonAlarmInfo.stPersonAlarmAttribute.eTopColorLabel = Alarm::PMNAttributeColor_E::RED;
        break;
    case 5:
        stPersonAlarmInfo.stPersonAlarmAttribute.eTopColorLabel = Alarm::PMNAttributeColor_E::WHITE;
        break;
    case 6:
        stPersonAlarmInfo.stPersonAlarmAttribute.eTopColorLabel = Alarm::PMNAttributeColor_E::YELLOW;
        break;
    default:
        stPersonAlarmInfo.stPersonAlarmAttribute.eTopColorLabel = Alarm::PMNAttributeColor_E::UNKNOW;
        break;
    }

    stPersonAlarmInfo.strPersonPicture  = strPersonPicture;
    stPersonAlarmInfo.strCurrentPicture = strCurrentPicture;
    if (SD_CARD_STATUS_E::NORMAL == CStorageManage::instance()->get_SdCardStatus())
    {
        stPersonAlarmInfo.bIsDownLoad = true;
        saveToDatebase(strPersonPicture, Event::Type::PEDESTRIAN_ATTRIBUTE);
        saveToDatebase(strCurrentPicture, Event::Type::PEDESTRIAN_ATTRIBUTE);
    }
    else
    {
        stPersonAlarmInfo.bIsDownLoad = false;
    }

    stPersonAlarmInfo.strTimeStamp = TimeUtils_NS::get_currentDateAndTimeNoT();

    dlog_debug("推送行人抓拍信息:%s", Convert::to_string(stPersonAlarmInfo).c_str());
    TaskPublish::instance()->message(AC_PUSH_PERSON_CAPTURE_INFO, Convert::to_string(stPersonAlarmInfo));

    return;
}

void CGroup2_Group4Detect::pushMotorvehicleCaptureInfo(const std::string &strCurrentPicture, const std::string &strTargetPicture, const std::string &strLicensePlateNumber, const VehicleAttribute_NS::Result_S &stResult)
{
    Alarm::MotorvehicleAlarmInfo_S stMotorVehicleAlarmInfo;

    switch ((VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleType)stResult.nVehicleType)
    {
    case VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleType::HEAVY_TRUCK:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleType = Alarm::VehicleType_E::HEAVY_TRUCK;
        break;
    case VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleType::SUV:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleType = Alarm::VehicleType_E::SUV;
        break;
    case VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleType::MPV_BUSINESS:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleType = Alarm::VehicleType_E::MPV_BUSINESS;
        break;
    case VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleType::LARGE_BUS:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleType = Alarm::VehicleType_E::LARGE_BUS;
        break;
    case VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleType::LIGHT_PASSENGER:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleType = Alarm::VehicleType_E::LIGHT_PASSENGER;
        break;
    case VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleType::SMALL_MPV:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleType = Alarm::VehicleType_E::SMALL_MPV;
        break;
    case VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleType::PICKUP:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleType = Alarm::VehicleType_E::PICKUP;
        break;
    case VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleType::SEDAN:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleType = Alarm::VehicleType_E::SEDAN;
        break;
    case VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleType::SMALL_TRUCK:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleType = Alarm::VehicleType_E::SMALL_TRUCK;
        break;
    default:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleType = Alarm::VehicleType_E::UNKNOW;
        break;
    }

    switch ((VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleColor)stResult.nVehicleColor)
    {
    case VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleColor::BLACK:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleColor = Alarm::PMNAttributeColor_E::BLACK;
        break;
    case VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleColor::BLUE:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleColor = Alarm::PMNAttributeColor_E::BLUE;
        break;
    case VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleColor::BROWN:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleColor = Alarm::PMNAttributeColor_E::BROWN;
        break;
    case VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleColor::CYAN:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleColor = Alarm::PMNAttributeColor_E::CYAN;
        break;
    case VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleColor::DARK_GRAY:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleColor = Alarm::PMNAttributeColor_E::DARK_GRAY;
        break;
    case VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleColor::GRAY:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleColor = Alarm::PMNAttributeColor_E::GRAY;
        break;
    case VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleColor::GREEN:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleColor = Alarm::PMNAttributeColor_E::GREEN;
        break;
    case VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleColor::RED:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleColor = Alarm::PMNAttributeColor_E::RED;
        break;
    case VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleColor::WHITE:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleColor = Alarm::PMNAttributeColor_E::WHITE;
        break;
    case VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleColor::YELLOW:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleColor = Alarm::PMNAttributeColor_E::YELLOW;
        break;
    default:
        stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.eVehicleColor = Alarm::PMNAttributeColor_E::UNKNOW;
        break;
    }

    auto brand = static_cast<VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleBrand>(stResult.nVehicleBrand);

    stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.strVehicleBrand = VehicleBrandToString(brand);
    // printf("车辆品牌: %d - %s\n", stResult.nVehicleBrand, stMotorVehicleAlarmInfo.stMotorvehicleAlarmAttribute.strVehicleBrand.c_str());

    stMotorVehicleAlarmInfo.strLicensePlateNumber = strLicensePlateNumber;
    stMotorVehicleAlarmInfo.strTargetPicture      = strTargetPicture;
    stMotorVehicleAlarmInfo.strCurrentPicture     = strCurrentPicture;
    if (SD_CARD_STATUS_E::NORMAL == CStorageManage::instance()->get_SdCardStatus())
    {
        stMotorVehicleAlarmInfo.bIsDownLoad = true;
        saveToDatebase(strTargetPicture, Event::Type::MOTORVEHICLE_ATTRIBUTE);
        saveToDatebase(strCurrentPicture, Event::Type::MOTORVEHICLE_ATTRIBUTE);
    }
    else
    {
        stMotorVehicleAlarmInfo.bIsDownLoad = false;
    }

    stMotorVehicleAlarmInfo.strTimeStamp = TimeUtils_NS::get_currentDateAndTimeNoT();

    dlog_debug("推送机动车抓拍信息:%s", Convert::to_string(stMotorVehicleAlarmInfo).c_str());

    TaskPublish::instance()->message(AC_PUSH_MOTORVEHICLE_CAPTURE_INFO, Convert::to_string(stMotorVehicleAlarmInfo));

    return;
}

void CGroup2_Group4Detect::pushNonMotorvehicleCaptureInfo(const std::string &strCurrentPicture, const std::string &strTargetPicture, const NonMotorizedAttribute_NS::Result_S &stResult)
{
    Alarm::NonMotorvehicleAlarmInfo_S stNonMotorvehicleAlarmInfo;

    switch ((NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorColor)stResult.nNonMotorizedVehicleColor)
    {
    case NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorColor::WHITE:
        stNonMotorvehicleAlarmInfo.stNonMotorvehicleAlarmAttribute.eNonMotorizedVehicleColor = Alarm::PMNAttributeColor_E::WHITE;
        break;
    case NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorColor::ORANGE:
        stNonMotorvehicleAlarmInfo.stNonMotorvehicleAlarmAttribute.eNonMotorizedVehicleColor = Alarm::PMNAttributeColor_E::ORANGE;
        break;
    case NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorColor::PINK:
        stNonMotorvehicleAlarmInfo.stNonMotorvehicleAlarmAttribute.eNonMotorizedVehicleColor = Alarm::PMNAttributeColor_E::PINK;
        break;
    case NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorColor::BLACK:
        stNonMotorvehicleAlarmInfo.stNonMotorvehicleAlarmAttribute.eNonMotorizedVehicleColor = Alarm::PMNAttributeColor_E::BLACK;
        break;
    case NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorColor::RED:
        stNonMotorvehicleAlarmInfo.stNonMotorvehicleAlarmAttribute.eNonMotorizedVehicleColor = Alarm::PMNAttributeColor_E::RED;
        break;
    case NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorColor::YELLOW:
        stNonMotorvehicleAlarmInfo.stNonMotorvehicleAlarmAttribute.eNonMotorizedVehicleColor = Alarm::PMNAttributeColor_E::YELLOW;
        break;
    case NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorColor::GRAY:
        stNonMotorvehicleAlarmInfo.stNonMotorvehicleAlarmAttribute.eNonMotorizedVehicleColor = Alarm::PMNAttributeColor_E::GRAY;
        break;
    case NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorColor::BLUE:
        stNonMotorvehicleAlarmInfo.stNonMotorvehicleAlarmAttribute.eNonMotorizedVehicleColor = Alarm::PMNAttributeColor_E::BLUE;
        break;
    case NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorColor::GREEN:
        stNonMotorvehicleAlarmInfo.stNonMotorvehicleAlarmAttribute.eNonMotorizedVehicleColor = Alarm::PMNAttributeColor_E::GREEN;
        break;
    case NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorColor::BROWN:
        stNonMotorvehicleAlarmInfo.stNonMotorvehicleAlarmAttribute.eNonMotorizedVehicleColor = Alarm::PMNAttributeColor_E::BROWN;
        break;
    case NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorColor::PURPLE:
        stNonMotorvehicleAlarmInfo.stNonMotorvehicleAlarmAttribute.eNonMotorizedVehicleColor = Alarm::PMNAttributeColor_E::PURPLE;
        break;
    default:
        stNonMotorvehicleAlarmInfo.stNonMotorvehicleAlarmAttribute.eNonMotorizedVehicleColor = Alarm::PMNAttributeColor_E::UNKNOW;
        break;
    }

    switch ((NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorType)stResult.nNonMotorizedVehicleType)
    {
    case NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorType::BICYCLE:
        stNonMotorvehicleAlarmInfo.stNonMotorvehicleAlarmAttribute.eNonMotorizedVehicleType = Alarm::NonMotorType_E::BICYCLE;
        break;
    case NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorType::TWO_WHEELER:
        stNonMotorvehicleAlarmInfo.stNonMotorvehicleAlarmAttribute.eNonMotorizedVehicleType = Alarm::NonMotorType_E::TWO_WHEELER;
        break;
    case NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0::NonMotorType::THREE_WHEELER:
        stNonMotorvehicleAlarmInfo.stNonMotorvehicleAlarmAttribute.eNonMotorizedVehicleType = Alarm::NonMotorType_E::THREE_WHEELER;
        break;
    default:
        stNonMotorvehicleAlarmInfo.stNonMotorvehicleAlarmAttribute.eNonMotorizedVehicleType = Alarm::NonMotorType_E::UNKNOW;
        break;
    }

    stNonMotorvehicleAlarmInfo.strTargetPicture  = strTargetPicture;
    stNonMotorvehicleAlarmInfo.strCurrentPicture = strCurrentPicture;
    if (SD_CARD_STATUS_E::NORMAL == CStorageManage::instance()->get_SdCardStatus())
    {
        stNonMotorvehicleAlarmInfo.bIsDownLoad = true;
        saveToDatebase(strTargetPicture, Event::Type::NONMOTORVEHICLE_ATTRIBUTE);
        saveToDatebase(strCurrentPicture, Event::Type::NONMOTORVEHICLE_ATTRIBUTE);
    }
    else
    {
        stNonMotorvehicleAlarmInfo.bIsDownLoad = false;
    }

    stNonMotorvehicleAlarmInfo.strTimeStamp = TimeUtils_NS::get_currentDateAndTimeNoT();

    dlog_debug("推送非机动车抓拍信息:%s", Convert::to_string(stNonMotorvehicleAlarmInfo).c_str());

    TaskPublish::instance()->message(AC_PUSH_NONMOTORVEHICLE_CAPTURE_INFO, Convert::to_string(stNonMotorvehicleAlarmInfo));

    return;
}

void CGroup2_Group4Detect::convertBoundaryAndEnable(Alarm::BoundaryDetection_S &stConfig)
{
    if (!stConfig.aRule.empty())
    {
        m_vstCrossRule.clear();
        /* 转换警戒线坐标分辨率至算法分辨率 */
        for (auto &rule : stConfig.aRule)
        {
            bool            bIsInit = false;
            Event::RuleInfo stRule;
            /* 转换起始点坐标 */
            float scaleX = static_cast<float>(m_nWidth) / PIXEL_WIDTH_1920;
            float scaleY = static_cast<float>(m_nHeight) / PIXEL_HEIGHT_1080;

            rule.stStartPos.fX *= scaleX;
            rule.stStartPos.fY *= scaleY;
            rule.stEndPos.fX *= scaleX;
            rule.stEndPos.fY *= scaleY;

            /* 判断是否设置了有效的警戒线 */
            if (((rule.stStartPos.fX != rule.stEndPos.fX) ||
                 (rule.stStartPos.fY != rule.stEndPos.fY)) &&
                (rule.stStartPos.fX != 0 && rule.stStartPos.fY != 0 && rule.stEndPos.fX != 0 && rule.stEndPos.fY != 0))
            {
                bIsInit = true;
            }

            /* 没有一条有效的警戒线，不使能 */
            if (!bIsInit)
            {
                dlog_debug("ai_app: 边界检测-越界检测警戒线无效");
                stRule.bEnable = false;
            }
            else
            {
                dlog_debug("ai_app: 边界检测-越界检测警戒线有效");
                std::vector<::Event::Point_S> line;
                ::Event::Point_S              stPoint;
                stPoint.nX = rule.stStartPos.fX;
                stPoint.nY = rule.stStartPos.fY;
                line.push_back(stPoint);
                stPoint.nX = rule.stEndPos.fX;
                stPoint.nY = rule.stEndPos.fY;
                line.push_back(stPoint);
                stRule.nSensitivity = rule.nSensitivity;
                // /* 判断是否开启了人体检测 */
                // bool bHasPerson = std::find(rule.aDetectionTarget.begin(), rule.aDetectionTarget.end(), PERSON_DETECT_TARGET) != rule.aDetectionTarget.end();
                // if(!bHasPerson)
                // {
                //     stRule.bEnable = false;
                //     dlog_debug("ai_app:  当前越界检测规则未开启人体检测");
                //     continue;
                // }
                // else
                {
                    stRule.bEnable = true;
                    stRule.lines.push_back(line);
                }
            }
            if (stRule.bEnable)
            {
                PMNMDetectRuleInfo_S stPMNMDetectRuleInfo;

                stPMNMDetectRuleInfo.stRuleInfo         = stRule;
                stPMNMDetectRuleInfo.enCrossDirection   = rule.enCrossDirection;
                stPMNMDetectRuleInfo.veDetectionTargets = rule.aDetectionTarget;
                m_vstCrossRule.push_back(stPMNMDetectRuleInfo);
            }
        }
    }
}

template <typename T>
void CGroup2_Group4Detect::convertAlertLineToZoneAndIsEnable(T &stConfig, Event::Type_E enType)
{
    if (enType == Event::Type_E::REVERSE_DIRECTION)
    {
        m_vstDrivingAgainstTrafficRule.clear();
    }

    if (!stConfig.aRule.empty())
    {
        /* 转换警戒线坐标分辨率至算法分辨率坐标 */
        for (auto &rule : stConfig.aRule)
        {
            bool              bIsInit = false;
            TrafficRuleInfo_S stRule;
            /* 转换起始点坐标 */
            float scaleX = static_cast<float>(m_nWidth) / PIXEL_WIDTH_1920;
            float scaleY = static_cast<float>(m_nHeight) / PIXEL_HEIGHT_1080;

            rule.stStartPos.fX *= scaleX;
            rule.stStartPos.fY *= scaleY;
            rule.stEndPos.fX *= scaleX;
            rule.stEndPos.fY *= scaleY;

            /* 判断是否设置了有效的警戒线 */
            if (((rule.stStartPos.fX != rule.stEndPos.fX) ||
                 (rule.stStartPos.fY != rule.stEndPos.fY)) &&
                (rule.stStartPos.fX != 0 && rule.stStartPos.fY != 0 && rule.stEndPos.fX != 0 && rule.stEndPos.fY != 0))
            {
                bIsInit = true;
            }

            /* 没有一条有效的警戒线，不使能 */
            if (!bIsInit)
            {
                dlog_debug("ai_app: 车辆检测-逆行检测/违规变道 警戒线无效");
                stRule.stRuleInfo.bEnable = false;
            }
            else
            {
                dlog_debug("ai_app: 车辆检测-逆行检测/违规变道 警戒线有效");
                std::vector<::Event::Point_S> line;
                ::Event::Point_S              stPoint;
                stPoint.nX = rule.stStartPos.fX;
                stPoint.nY = rule.stStartPos.fY;
                line.push_back(stPoint);
                stPoint.nX = rule.stEndPos.fX;
                stPoint.nY = rule.stEndPos.fY;
                line.push_back(stPoint);
                stRule.stRuleInfo.bEnable      = true;
                stRule.stRuleInfo.nSensitivity = rule.nSensitivity;
                stRule.stRuleInfo.lines.push_back(line);
            }
            if (stRule.stRuleInfo.bEnable)
            {
                if (enType == Event::Type_E::REVERSE_DIRECTION)
                {
                    stRule.enCrossDirection = rule.enCrossDirection;
                    m_vstDrivingAgainstTrafficRule.push_back(stRule);
                }
                else
                {
                    stRule.enCrossDirection = Alarm::CrossDirection_E::CROSS_DIRECTION_INVALID;
                    m_vstIllegalLaneChangeRule.push_back(stRule);
                }
            }
        }
    }
}

template <typename T>
void CGroup2_Group4Detect::convertResolutionAndEnable(T &stConfig, Event::Type_E enType)
{
    if (!stConfig.aRule.empty())
    {
        /* 是否有任何一个区域初始化成功 */
        // bool bIsInit = false;

        /* 转换区域坐标分辨率至算法分辨率 */
        for (auto &rule : stConfig.aRule) /* 使用引用而不是值拷贝 */
        {
            Event::RuleInfo               stRule;
            std::vector<::Event::Point_S> area;
            /* 判断是否设置了正确的多边形 */
            if (!rule.stRegion.IsValid())
            {
                stRule.bEnable = false;
                dlog_debug("当前多边形区域无效");
                continue;
            }

            bool bVaildPoint = false;
            for (int i = 0; i < 4; ++i)
            {
                const auto &point = rule.stRegion.aPoint[i];

                if (point.fX != 0 || point.fY != 0)
                {
                    bVaildPoint = true;
                    dlog_debug("当前多边形区域有效");
                    break;
                }
            }
            if (!bVaildPoint)
            {
                stRule.bEnable = false;
                dlog_debug("当前多边形区域无效");
                continue;
            }
            else
            {
                stRule.bEnable = true;
            }

            rule.stRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);

            stRule.nSensitivity   = rule.nSensitivity;
            stRule.nTimeThreshold = rule.nTimeThreshold;
            dlog_debug("ai_app:  当前规则获取到 灵敏度[%d] 时间阈值[%d] ", rule.nSensitivity, rule.nTimeThreshold);
            for (auto &pos : rule.stRegion.aPoint)
            {
                ::Event::Point_S stPoint;
                stPoint.nX = pos.fX;
                stPoint.nY = pos.fY;
                area.push_back(stPoint);
            }
            stRule.areas.push_back(area);
            stRule.enType = enType;

            if (!stRule.bEnable)
            {
                dlog_debug("当前规则无效");
                continue;
            }

            switch (enType)
            {
            case Event::Type_E::INTRUSION: {
                PMNMDetectRuleInfo_S stPMNMDetectRuleInfo;
                stPMNMDetectRuleInfo.stRuleInfo         = stRule;
                stPMNMDetectRuleInfo.veDetectionTargets = rule.aDetectionTarget;
                m_vstIntruRule.push_back(stPMNMDetectRuleInfo);
#if Group2_Group4_Detect_DEBUG
                dlog_debug("=====================打印区域入侵area=================");
                printArea(area);
#endif
                break;
            }
            case Event::Type_E::ENTER_REGION: {
                PMNMDetectRuleInfo_S stPMNMDetectRuleInfo;
                stPMNMDetectRuleInfo.stRuleInfo         = stRule;
                stPMNMDetectRuleInfo.veDetectionTargets = rule.aDetectionTarget;
                m_vstEntryRule.push_back(stPMNMDetectRuleInfo);
#if Group2_Group4_Detect_DEBUG
                dlog_debug("=====================打印进入区域area=================");
                printArea(area);
#endif
                break;
            }
            case Event::Type_E::LEAVE_REGION: {
                PMNMDetectRuleInfo_S stPMNMDetectRuleInfo;
                stPMNMDetectRuleInfo.stRuleInfo         = stRule;
                stPMNMDetectRuleInfo.veDetectionTargets = rule.aDetectionTarget;
                m_vstExitRule.push_back(stPMNMDetectRuleInfo);
#if Group2_Group4_Detect_DEBUG
                dlog_debug("=====================打印离开区域area=================");
                printArea(area);
#endif
                break;
            }

            case Event::Type_E::LOITERING_DETECT: {
                m_vstLoiteringRule.push_back(stRule);
#if Group2_Group4_Detect_DEBUG
                dlog_debug("=====================打印徘徊侦测区域area=================");
                printArea(area);
#endif
                break;
            }
            case Event::Type_E::FENCE_CLIMBING: {
                m_vstFenceClimbingRule.push_back(stRule);
#if Group2_Group4_Detect_DEBUG
                dlog_debug("=====================打印翻越围栏区域area=================");
                printArea(area);
#endif
                break;
            }
            case Event::Type_E::LEAVE_POST: {
                m_vstLeavePostRule.push_back(stRule);
#if Group2_Group4_Detect_DEBUG
                dlog_debug("=====================打印离岗识别区域area=================");
                printArea(area);
#endif
                break;
            }
            case Event::Type_E::PEDESTRIAN_INTRUSION: {
                m_vstPedestrianIntrusionRule.push_back(stRule);
#if Group2_Group4_Detect_DEBUG
                dlog_debug("=====================打印行人闯入区域area=================");
                printArea(area);
#endif
                break;
            }

            case Event::Type_E::EMERGENCY_LANE_OCCUPANCY: {
                PMNMDetectRuleInfo_S stPMNMDetectRuleInfo;
                stPMNMDetectRuleInfo.stRuleInfo         = stRule;
                stPMNMDetectRuleInfo.veDetectionTargets = rule.aDetectionTarget;
                m_vstEmergencyLaneOccupancyRule.push_back(stPMNMDetectRuleInfo);
#if Group2_Group4_Detect_DEBUG
                dlog_debug("=====================打印应急车道区域area=================");
                printArea(area);
#endif
                break;
            }
            case Event::Type_E::NON_MOTOR_VEHICLE_INTRUSION: {
                PMNMDetectRuleInfo_S stPMNMDetectRuleInfo;
                stPMNMDetectRuleInfo.stRuleInfo         = stRule;
                stPMNMDetectRuleInfo.veDetectionTargets = rule.aDetectionTarget;
                m_vstNonMotorVehicleIntrusionRule.push_back(stPMNMDetectRuleInfo);
#if Group2_Group4_Detect_DEBUG
                dlog_debug("=====================打印非机动车闯入区域area=================");
                printArea(area);
#endif
                break;
            }

            default: {
                dlog_info("Unkonw event type %d", enType);
                break;
            }
            }
        }
    }
}

template <typename T>
void CGroup2_Group4Detect::convertGuardAreaAndCheckAlgoEnable(T &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置停车侦测参数");

    // /* 是否初始化 */
    // bool bIsInit = false;

    if (!stAlgoCfg.aRule.empty())
    {
        /* 转换区域坐标分辨率至算法分辨率 */
        for (auto &rule : stAlgoCfg.aRule)
        {
            Event::RuleInfo               stRule;
            std::vector<::Event::Point_S> area;
            /* 判断是否设置了正确的多边形 */
            if (!rule.stRegion.IsValid())
            {
                stRule.bEnable = false;
                dlog_debug("当前多边形区域无效");
                continue;
            }

            bool bVaildPoint = false;
            for (int i = 0; i < 4; ++i)
            {
                const auto &point = rule.stRegion.aPoint[i];
                if (point.fX != 0 || point.fY != 0)
                {
                    bVaildPoint = true;
                    dlog_debug("当前多边形区域有效");
                    break;
                }
            }

            if (!bVaildPoint)
            {
                stRule.bEnable = false;
                dlog_debug("当前多边形区域无效");
                continue;
            }
            else
            {
                stRule.bEnable = true;
            }

            rule.stRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);

            stRule.nSensitivity   = rule.nSensitivity;
            stRule.nTimeThreshold = rule.nTimeThreshold;

            dlog_debug("ai_app:  当前规则获取到 灵敏度[%d]", stRule.nSensitivity);
            for (auto &pos : rule.stRegion.aPoint)
            {
                ::Event::Point_S stPoint;
                stPoint.nX = pos.fX;
                stPoint.nY = pos.fY;
                area.push_back(stPoint);
            }
            stRule.areas.push_back(area);
            stRule.enType = enType;

            switch (enType)
            {
            case Event::Type_E::PARKING_DETECT: {
                m_vstIllegalParkingRule.push_back(stRule);
#if Group2_Group4_Detect_DEBUG
                dlog_debug("=====================打印违停区域area=================");
                printArea(area);
#endif
                break;
            }
            default: {
                dlog_info("Unkonw event type %d", enType);
                break;
            }
            }
        }
    }
}

void CGroup2_Group4Detect::convertCrowdGatherAndEnable(Alarm::CrowdGathering_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置人员聚集侦测参数");

    if (!stAlgoCfg.aRule.empty())
    {
        /* 转换区域坐标分辨率至算法分辨率 */
        for (auto &rule : stAlgoCfg.aRule)
        {
            Event::RuleInfo               stRule;
            std::vector<::Event::Point_S> area;
            /* 判断是否设置了正确的多边形 */
            if (!rule.stRegion.IsValid())
            {
                stRule.bEnable = false;
                dlog_debug("当前多边形区域无效");
                continue;
            }

            bool bVaildPoint = false;
            for (int i = 0; i < 4; ++i)
            {
                const auto &point = rule.stRegion.aPoint[i];
                if (point.fX != 0 || point.fY != 0)
                {
                    bVaildPoint = true;
                    dlog_debug("当前多边形区域有效");
                    break;
                }
            }

            if (!bVaildPoint)
            {
                stRule.bEnable = false;
                dlog_debug("当前多边形区域无效");
                continue;
            }
            else
            {
                stRule.bEnable = true;
            }

            rule.stRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);

            stRule.nSensitivity   = rule.nObjectOccup;
            stRule.nTimeThreshold = 0;
            stRule.enType         = enType;
            dlog_debug("ai_app:  当前规则获取到 灵敏度[%d]", stRule.nSensitivity);
            for (auto &pos : rule.stRegion.aPoint)
            {
                ::Event::Point_S stPoint;
                stPoint.nX = pos.fX;
                stPoint.nY = pos.fY;
                area.push_back(stPoint);
            }
            stRule.areas.push_back(area);

            m_vstCrowdGatheringDetRule.push_back(stRule);
#if Group2_Group4_Detect_DEBUG
            dlog_debug("=====================打印人员聚集区域area=================");
            printArea(area);
#endif
        }
    }
}

#ifdef ENABLE_GAT1400_SRC
void CGroup2_Group4Detect::pushImageToGat1400(const cv::Mat &image, const std::vector<Group2Detect_NS::Result_S> &vecResult)
{
    std::vector<uchar>       buffer;
    security_subimage_info_t stImageInfo;
    int                      status = 0;
    if (image.empty())
    {
        dlog_debug("image empty");
        return;
    }
    if (!cv::imencode(".jpg", image, buffer))
    {
        dlog_debug("jpeg 编码失败");
        return;
    }
    if (!buffer.empty())
    {
        stImageInfo.Data.resize(buffer.size());
        memcpy(&(stImageInfo.Data)[0], buffer.data(), buffer.size());
        stImageInfo.FileFormat = "Jpeg";
        stImageInfo.Width      = image.cols;
        stImageInfo.Height     = image.rows;
    }

    for (auto &result : vecResult)
    {
        // 坐标点转换
        Common::PosF_S stPosition1{result.fX1, result.fY1};
        Common::PosF_S stPosition2{result.fX2, result.fY2};
        bool           bConvert1 = stPosition1.ConvertResolution(m_nWidth, m_nHeight, image.cols, image.rows);
        bool           bConvert2 = stPosition2.ConvertResolution(m_nWidth, m_nHeight, image.cols, image.rows);

        /* 判断上传类型 */
        /* 0-人 1-机动车 2-非机动车 */
        if (result.nID == 0)
        {
            security_persons_t stPersons;
            security_person_t  stPerson;

            stPerson.InfoKind = SecurityInfoType::Auto;
            if (bConvert1 && bConvert2)
            {
                stPerson.LeftTopX  = stPosition1.fX;
                stPerson.LeftTopY  = stPosition1.fY;
                stPerson.RightBtmX = stPosition2.fX;
                stPerson.RightBtmY = stPosition2.fY;
            }
            stPerson.Behavior = BEHAVIOR_TYPE_OTHER;
            stImageInfo.Type  = IMAGE_TYPE_SCENE;

            stPerson.SubImageList.push_back(stImageInfo);
            stPersons.push_back(stPerson);
            status = GAT1400::CGAT1400::instance()->uploadPersons(stPersons);
            if (status)
            {
                dlog_debug("uploadPersons faild %d", status);
            }
        }
        else if (result.nID == 1)
        {
            security_motorvehicles_t stVehicles;
            security_motorvehicle_t  stVehicle;

            stVehicle.InfoKind = SecurityInfoType::Auto;
            if (bConvert1 && bConvert2)
            {
                stVehicle.LeftTopX  = stPosition1.fX;
                stVehicle.LeftTopY  = stPosition1.fY;
                stVehicle.RightBtmX = stPosition2.fX;
                stVehicle.RightBtmY = stPosition2.fY;
            }
            stImageInfo.Type = IMAGE_TYPE_MOTOR_VEHICLE;

            stVehicle.SubImageList.push_back(stImageInfo);
            stVehicles.push_back(stVehicle);
            status = GAT1400::CGAT1400::instance()->uploadMotorvehicles(stVehicles);
            if (status)
            {
                dlog_debug("uploadMotorvehicles faild %d", status);
            }
        }
        else if (result.nID == 2)
        {
            security_nonmotorvehicles_t stNonmotors;
            security_nonmotorvehicle_t  stNonmotor;

            stNonmotor.InfoKind = SecurityInfoType::Auto;
            if (bConvert1 && bConvert2)
            {
                stNonmotor.LeftTopX  = stPosition1.fX;
                stNonmotor.LeftTopY  = stPosition1.fY;
                stNonmotor.RightBtmX = stPosition2.fX;
                stNonmotor.RightBtmY = stPosition2.fY;
            }
            stImageInfo.Type = IMAGE_TYPE_SCENE;

            stNonmotor.SubImageList.push_back(stImageInfo);
            stNonmotors.push_back(stNonmotor);
            status = GAT1400::CGAT1400::instance()->uploadNonmotorvehicles(stNonmotors);
            if (status)
            {
                dlog_debug("uploadNonmotorvehicles faild %d", status);
            }
        }
    }
}

int CGroup2_Group4Detect::checkPushState(const Group2Detect_NS::OutData_S &stGroup2OutData)
{
    int flag    = 0;
    int nPerson = 1;
    int nMotor  = 2;
    int nNoMotr = 4;
    if (m_stAlgoCrossCfg.bEnable && m_CrossUploadStateMachine.handleAlarmState(stGroup2OutData.bTripLineType))
    {
        auto target = m_stAlgoCrossCfg.aRule[stGroup2OutData.nChnId].aDetectionTarget;
        for (auto &it : target)
        {
            flag |= 1 << it;
        }
    }
    if (m_stAlgoIntruCfg.bEnable && m_IntruUploadStateMachine.handleAlarmState(stGroup2OutData.bIntrusionFlag))
    {
        auto target = m_stAlgoIntruCfg.aRule[stGroup2OutData.nChnId].aDetectionTarget;
        for (auto &it : target)
        {
            flag |= 1 << it;
        }
    }
    if (m_stAlgoEntryCfg.bEnable && m_EntryUploadStateMachine.handleAlarmState(stGroup2OutData.bEntryFlag))
    {
        auto target = m_stAlgoEntryCfg.aRule[stGroup2OutData.nChnId].aDetectionTarget;
        for (auto &it : target)
        {
            flag |= 1 << it;
        }
    }
    if (m_stAlgoExitCfg.bEnable && m_ExitUploadStateMachine.handleAlarmState(stGroup2OutData.bLeaveFlag))
    {
        auto target = m_stAlgoEntryCfg.aRule[stGroup2OutData.nChnId].aDetectionTarget;
        for (auto &it : target)
        {
            flag |= 1 << it;
        }
    }
    if (m_stLoiteringCfg.bEnable && m_LoiteringUploadStateMachine.handleAlarmState(stGroup2OutData.bLoiteringFlag))
    {
        flag |= nPerson;
    }
    if (m_stFenceClimbingCfg.bEnable && m_FenceClimbingUploadStateMachine.handleAlarmState(stGroup2OutData.bFenceClimbFlag))
    {
        flag |= nPerson;
    }

    if (m_stLeavePostCfg.bEnable && m_LeavePostUploadStateMachine.handleAlarmState(stGroup2OutData.bLeavePostFlag))
    {
        flag |= nPerson;
    }
    if (m_stPedestrianIntrusionCfg.bEnable && m_PedestrianIntrusionUploadStateMachine.handleAlarmState(stGroup2OutData.bPedestrianIntrusionFlag))
    {
        flag |= nPerson;
    }
    if (m_stCrowdGatheringDetCfg.bEnable && m_CrowdGatheringUploadStateMachine.handleAlarmState(stGroup2OutData.bCrowdGatheringDetParamFlag))
    {
        flag |= nPerson;
    }

    if (m_stAlgoElectricScooterCfg.bEnable && m_ElectricScooterUploadStateMachine.handleAlarmState(stGroup2OutData.bElectricScooter))
    {
        flag |= nNoMotr;
    }

    if (m_stAlgoEmergencyLaneOccupancyCfg.bEnable && m_EmergencyLaneOccupancyUploadStateMachine.handleAlarmState(stGroup2OutData.bEmergencyLaneOccupancyFlag))
    {
        auto target = m_stAlgoEmergencyLaneOccupancyCfg.aRule[stGroup2OutData.nChnId].aDetectionTarget;
        for (auto &it : target)
        {
            flag |= 1 << it;
        }
    }
    if (m_stAlgoNonMotorVehicleIntrusionCfg.bEnable && m_NonMotorVehicleIntrusionUploadStateMachine.handleAlarmState(stGroup2OutData.bNonMotorVehicleIntrusionFlag))
    {
        flag |= nNoMotr;
    }

    if (m_stDrivingAgainstTrafficDetectionCfg.bEnable && m_DrivingAgainstTrafficUploadStateMachine.handleAlarmState(stGroup2OutData.bDrivingAgainstTrafficFlag))
    {
        flag |= nMotor;
    }
    if (m_stAlgoIllegalLaneChangeDetectionCfg.bEnable && m_IllegalLaneChangeUploadStateMachine.handleAlarmState(stGroup2OutData.bIllegalLaneChangeFlag))
    {
        flag |= nMotor;
    }
    if (m_stAlgoParkingDetectionCfg.bEnable && m_IllegalParkingUploadStateMachine.handleAlarmState(stGroup2OutData.bParkingFlag))
    {
        flag |= nMotor;
    }
    if (m_stAlgoCongestionDetectionCfg.bEnable && m_CongestionUploadStateMachine.handleAlarmState(stGroup2OutData.bCongestionFlag))
    {
        flag |= nMotor;
    }
    return flag;
}
#endif

static const char *VehicleBrandToString(VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleBrand brand)
{
    using VB = VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleBrand;

    switch (brand)
    {
    case VB::ACURA:
        return "阿库拉";
    case VB::ANKAI:
        return "安凯";
    case VB::AUDI:
        return "奥迪";
    case VB::BAIC:
        return "北汽";
    case VB::BMW:
        return "宝马";
    case VB::BYD:
        return "比亚迪";
    case VB::BAOJUN:
        return "宝骏";
    case VB::BESTUNE:
        return "奔腾";
    case VB::BUICK:
        return "别克";
    case VB::CADILLAC:
        return "凯迪拉克";
    case VB::CHANGAN:
        return "长安";
    case VB::CHANGHE:
        return "长河";
    case VB::CHERY:
        return "奇瑞";
    case VB::CHEVROLET:
        return "雪佛兰";
    case VB::CHRYSLER:
        return "克莱斯勒";
    case VB::CITROEN:
        return "雪铁龙";
    case VB::COWIN:
        return "凯翼";
    case VB::DODGE:
        return "道奇";
    case VB::DS:
        return "DS";
    case VB::DONGFENG:
        return "东风";
    case VB::FAW:
        return "一汽";
    case VB::FAW_JIEFANG:
        return "一汽解放";
    case VB::FIAT:
        return "菲亚特";
    case VB::FORD:
        return "福特";
    case VB::FORLAND_TIMES:
        return "时代汽车";
    case VB::FOTON:
        return "福田";
    case VB::GAC_TRUMPCHI:
        return "广汽传祺";
    case VB::GEELY:
        return "吉利";
    case VB::GREAT_WALL:
        return "长城";
    case VB::HIGER:
        return "海格";
    case VB::HAFEI:
        return "哈飞";
    case VB::HAIMA:
        return "海马";
    case VB::HAVAL:
        return "哈弗";
    case VB::HUATAI:
        return "华泰";
    case VB::HEIBAO:
        return "黑豹汽车";
    case VB::HENGTONG:
        return "恒通";
    case VB::HONDA:
        return "本田";
    case VB::HONGQI:
        return "红旗";
    case VB::HUANGHAI:
        return "黄海";
    case VB::INFINITI:
        return "英菲尼迪";
    case VB::ISUZU:
        return "五十铃";
    case VB::IVECO:
        return "依维柯";
    case VB::JAC:
        return "江淮";
    case VB::JAGUAR:
        return "捷豹";
    case VB::JEEP:
        return "Jeep";
    case VB::JMC:
        return "江铃";
    case VB::JINBEI:
        return "金杯";
    case VB::KINGLONG:
        return "金龙";
    case VB::KIA:
        return "起亚";
    case VB::KAMA:
        return "凯马";
    case VB::KARRY:
        return "开瑞";
    case VB::LUXGEN:
        return "纳智捷";
    case VB::LAND_ROVER:
        return "路虎";
    case VB::LUFENG:
        return "陆风";
    case VB::LIEBAO:
        return "猎豹";
    case VB::LEXUS:
        return "雷克萨斯";
    case VB::LIFAN:
        return "力帆";
    case VB::LINCOLN:
        return "林肯";
    case VB::MG:
        return "名爵";
    case VB::MINI:
        return "MINI";
    case VB::MASERATI:
        return "玛莎拉蒂";
    case VB::MAZDA:
        return "马自达";
    case VB::BENZ:
        return "梅赛德斯-奔驰";
    case VB::SMALL_TRUCK:
        return "小型卡车";
    case VB::MITSUBISHI:
        return "三菱";
    case VB::HYUNDAI:
        return "现代";
    case VB::MUSTANG:
        return "野马";
    case VB::NISSAN:
        return "日产";
    case VB::OLD_SCOOTER:
        return "老年代步车";
    case VB::PEUGEOT:
        return "标致";
    case VB::PORSCHE:
        return "保时捷";
    case VB::RENAULT:
        return "雷诺";
    case VB::ROEWE:
        return "荣威";
    case VB::MAXUS:
        return "上汽大通";
    case VB::SINOTRUK:
        return "中国重汽";
    case VB::SUNWIN:
        return "申沃";
    case VB::SHACMAN_TONGJIA:
        return "陕汽通佳";
    case VB::SKODA:
        return "斯柯达";
    case VB::SMART:
        return "Smart";
    case VB::SOUTHEAST:
        return "东南";
    case VB::SSANGYONG:
        return "双龙";
    case VB::SUBARU:
        return "斯巴鲁";
    case VB::SUZUKI:
        return "铃木";
    case VB::TKNG:
        return "唐骏";
    case VB::TESLA:
        return "特斯拉";
    case VB::TOYOTA:
        return "丰田";
    case VB::TRUCK:
        return "卡车";
    case VB::VENUCIA:
        return "启辰";
    case VB::VOLKSWAGEN:
        return "大众";
    case VB::VOLVO:
        return "沃尔沃";
    case VB::WULING:
        return "五菱";
    case VB::WUZHENG:
        return "五征";
    case VB::YUEJIN:
        return "跃进";
    case VB::YUTONG:
        return "宇通";
    case VB::ZHONGHUA:
        return "中华";
    case VB::ZHONGTONG:
        return "中通";
    case VB::ZXAUTO:
        return "中兴";
    case VB::ZOTYE:
        return "众泰";
    case VB::BULK_TRUCK:
        return "散货车";
    case VB::PICKUP_TRUCK:
        return "皮卡";

    default:
        return "未知品牌";
    }
}
