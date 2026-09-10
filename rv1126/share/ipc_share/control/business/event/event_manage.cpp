/**
 * @FilePath     : event_manage.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2024-12-14
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-04 15:00:25
 * @Description  : 事件布防调度管理，负责事件布防时间计算与算法开关状态通知
 */

#include "event_manage.h"
#include "IpcRet.h"
#include "time_utils.h"

CEventManage::CEventManage()
{
    CEventConfigure::instance()->get_configure(m_stEventSchedule);

    /* 首次运行时，为声音报警、报警输入、报警输出、闪光灯报警创建默认布防计划 */
    if (m_stEventSchedule.empty())
    {
        for (int i = static_cast<int>(Event::Type_E::AUDIO_ALARM); i <= static_cast<int>(Event::Type_E::FLASH_ALARM); ++i)
        {
            Alarm::EventSchedule_S stEventSchedule;
            stEventSchedule.enEventType = static_cast<Event::Type_E>(i);
            stEventSchedule.bStatus = true;
            stEventSchedule.defenseTime.clear();
            stEventSchedule.defenseTime.assign(7, std::vector<Common::SchedTime_S>(1));
            CEventConfigure::instance()->set_configure(stEventSchedule);
        }
        /* 重新加载以获取完整的布防计划 */
        CEventConfigure::instance()->get_configure(m_stEventSchedule);
    }

    m_bRunning.store(true, std::memory_order_release);
    /* 初始化间隔分析时间 */
    m_lastIntervalAnalysisTime = time(nullptr);
    /* 启动布防调度线程 */
    m_thread = std::thread(&CEventManage::run, this);
}

CEventManage::~CEventManage()
{
    m_bRunning.store(false, std::memory_order_release);
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

void CEventManage::set_algoControlDeal_callback(const AlgoControlDealCallback &callback)
{
    if (!callback)
    {
        dlog_error("无效的回调函数 (nullptr或未绑定)");
        return;
    }
    m_algoControlDealCallback = callback;
    dlog_info("事件管理 AI 算法控制处理的回调设置成功");
}

int CEventManage::send_algo_controlData(const int nCode, const char *pJsonData, void *pData)
{
    /* 调用回调通知 AI 算法控制处理 */
    if (m_algoControlDealCallback)
    {
        m_algoControlDealCallback(nCode, pJsonData, pData);
    }
    else
    {
        dlog_warn("事件管理 AI 算法控制处理 未设置回调函数");
        return ERR;
    }
    return OK;
}

void CEventManage::update_event_schedule()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        CEventConfigure::instance()->get_configure(m_stEventSchedule);
    }
    dlog_info("更新事件布防时间");

    /* 计算当前的算法状态 */
    // calculate_current_algorithm_state();

    /* 通知 AI_APP 模块，更新相关算法配置 */
    update_event_switch_status();
}

int CEventManage::update_event_switch_status()
{
    std::string info = Convert::to_string(m_changes);
    dlog_debug("更新事件开关状态");
    CEventConfigure::instance()->set_configure(m_changes);
    /* 通知 AI_APP 模块 */
    return send_algo_controlData(AC_SET_ALGORITHM_CONFIG, info.c_str());
}

#ifdef SCENE_INTELLIGENT_ANALYSIS

void CEventManage::update_ai_analysis_schedule()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    CEventConfigure::instance()->get_configure(m_stEventSchedule);
    dlog_info("更新AI场景智能分析布防时间");
}

/*AI场景智能分析-调度*/
int CEventManage::update_ai_analysis_switch_status(Event::AlgorithmConfig_S AlgoInfo)
{
    std::string info = Convert::to_string(AlgoInfo);
    dlog_debug("更新场景智能分析开关状态");
    /* 通知 AI_APP 模块 */
    return send_algo_controlData(AC_SET_ALGORITHM_CONFIG, info.c_str());
}
#endif

void CEventManage::set_event_type(Event::AlgorithmConfig_S &change, Event::Type_E enEventType, bool bEnable)
{
    // dlog_debug("设置事件类型: %d 启用状态: %d", enEventType, bEnable);

    switch (enEventType)
    {
    /* 普通事件 */
    case Event::Type::MOTION_DETECT: /* 移动侦测 */
        change.nEnMotionDetect = bEnable;
        break;
    case Event::Type::OCCLUSION_DETECT: /* 遮挡侦测 */
        change.nEnOcclusionDetect = bEnable;
        break;
    case Event::Type::ANOMALY_ALARM: /* 异常报警 */
        change.nEnAnomalyAlarm = bEnable;
        break;
    case Event::Type::AUDIO_ALARM: /* 声音报警 */
        change.nEnAudioAlarm = bEnable;
        break;
    case Event::Type::ALARM_INPUT: /* 报警输入 */
        change.nEnAlarmInput = bEnable;
        break;
    case Event::Type::ALARM_OUTPUT: /* 报警输出 */
        change.nEnAlarmOutput = bEnable;
        break;
    case Event::Type::FLASH_ALARM: /* 闪光灯报警 */
        change.nEnFlashAlarm = bEnable;
        break;
    case Event::Type::PIR_ALARM: /* PIR 红外感应报警 */
        change.nEnPIRAlarm = bEnable;
        break;

    /* 周界事件 */
    case Event::Type::LINE_CROSSING: /* 越界侦测 */
        change.nEnLineCrossing = bEnable;
        break;
    case Event::Type::INTRUSION: /* 区域入侵 */
        change.nEnIntrusion = bEnable;
        break;
    case Event::Type::ENTER_REGION: /* 进入区域 */
        change.nEnEnterRegion = bEnable;
        break;
    case Event::Type::LEAVE_REGION: /* 离开区域 */
        change.nEnLeaveRegion = bEnable;
        break;

    /* Smart事件 */
    case Event::Type::AUDIO_ANOMALY: /* 音频异常侦测 */
        change.nEnAudioAnomaly = bEnable;
        break;
    case Event::Type::SCENE_CHANGE: /* 场景变更 */
        change.nEnSceneChange = bEnable;
        break;
    case Event::Type::FACE_DETECT: /* 人脸侦测 */
        change.nEnFaceDetect = bEnable;
        break;
    case Event::Type::LOITERING_DETECT: /* 徘徊侦测 */
        change.nEnLoiteringDetect = bEnable;
        break;
    case Event::Type::CROWD_GATHERING: /* 人员聚集 */
        change.nEnCrowdGathering = bEnable;
        break;
    case Event::Type::PARKING_DETECT: /* 停车侦测 */
        change.nEnParkingDetect = bEnable;
        break;
    case Event::Type::UNATTENDED_OBJECT: /* 物品遗留 */
        change.nEnUnattendedObject = bEnable;
        break;
    case Event::Type::OBJECT_REMOVAL: /* 物品拿取 */
        change.nEnObjectRemoval = bEnable;
        break;
    case Event::Type::PET_RECOGNITION: /* 宠物识别 */
        change.nEnPetRecognition = bEnable;
        break;
    case Event::Type::FACE_CAPTURE: /* 人脸抓拍 */
        change.nEnFaceCapture = bEnable;
        break;
    case Event::Type::FACE_COMPARE: /* 人脸比对 */
        change.nEnFaceCompare = bEnable;
        break;
    case Event::Type::FACE_LIB: /* 人脸名单库 */
        change.nEnFaceLib = bEnable;
        break;
#ifdef SCENE_INTELLIGENT_ANALYSIS
    /* 智能场景分析事件 */
    case Event::Type::AI_SCENE_ANALYSIS: /* 场景智能分析总开关 */
        change.nEnAISceneAnalysis = bEnable;
        break;
    case Event::Type::IMAGE_ANALYSIS: /* 画面分析 */
        change.nEnLLmInference = bEnable;
        break;
    case Event::Type::TEXT_PRESET: /* 文字预设任务 */
        change.nEnTextPreset = bEnable;
        break;
#endif
#ifdef SCENE_INTELLIGENCE
    /* 场景智能 */   
    case Event::Type::SLEEP_ON_DUTY: /* 睡岗识别 */
        change.nEnSleepOnDuty = bEnable;
        break;
    case Event::Type::LEAVE_POST: /* 离岗识别 */
        change.nEnLeavePost = bEnable;
        break;
    case Event::Type::ELECTRIC_VEHICLE_IN_ELEVATOR: /* 电瓶车进电梯识别 */
        change.nEnElectricVehicleInElevator = bEnable;
        break;
    case Event::Type::PERSON_FALL_DOWN: /* 人员倒地识别 */
        change.nEnPersonFallDown = bEnable;
        break;
    case Event::Type::FENCE_CLIMBING: /* 翻越围栏识别 */
        change.nEnFenceClimbing = bEnable;
        break;
    case Event::Type::PERSON_TRIP: /* 摔倒识别 */
        change.nEnTrip = bEnable;
        break;
    case Event::Type::SMOKING: /* 抽烟识别 */
        change.nEnSmoking = bEnable;
        break;
    case Event::Type::PHONE_USAGE: /* 玩手机识别 */
        change.nEnPhoneUsage = bEnable;
        break;
    case Event::Type::SMOKE_FIRE: /* 烟火识别 */
        change.nEnSmokeFire = bEnable;
        break;
    case Event::Type::OPEN_FLAME: /* 明火识别 */
        change.nEnOpenFlame = bEnable;
        break;
    case Event::Type::MANHOLE_COVER_ABNORMAL: /* 井盖异常检测 */
        change.nEnManholeCoverAbnormal = bEnable;
        break;
    case Event::Type::BARE_SOIL: /* 黄土裸露识别 */
        change.nEnBareSoil = bEnable;
        break;
    case Event::Type::HOLE_PROTECTION_BAR: /* 洞口防护栏识别 */
        change.nEnHoleProtectionBar = bEnable;
        break;
    case Event::Type::PEDESTRIAN_INTRUSION: /* 行人闯入识别 */
        change.nEnPedestrianIntrusion = bEnable;
        break;
    case Event::Type::SAFETY_HELMET: /* 安全帽识别 */
        change.nEnSafetyHelmet = bEnable;
        break;
    case Event::Type::REFLECTIVE_CLOTHING: /* 反光衣识别 */
        change.nEnReflectiveClothing = bEnable;
        break;
    case Event::Type::HIGH_ALTITUDE_SEATBELT: /* 高空安全带识别 */
        change.nEnHighAltitudeSeatbelt = bEnable;
        break;
    case Event::Type::CONSTRUCTION_OCCUPY_ROAD: /* 施工占道识别 */
        change.nEnConstructionOccupyRoad = bEnable;
        break;
    case Event::Type::EMERGENCY_LANE_OCCUPANCY: /* 应急车道占用识别 */
        change.nEnEmergencyLaneOccupancy = bEnable;
        break;
    case Event::Type::REVERSE_DIRECTION: /* 逆行识别 */
        change.nEnReverseDirection = bEnable;
        break;
    case Event::Type::NON_MOTOR_VEHICLE_INTRUSION: /* 非机动车闯入识别 */
        change.nEnNonMotorVehicleIntrusion = bEnable;
        break;
    case Event::Type::ROAD_PONDING: /* 道路积水识别 */
        change.nEnRoadPonding = bEnable;
        break;
    case Event::Type::CONGESTION: /* 拥堵识别 */
        change.nEnCongestion = bEnable;
        break;
    case Event::Type::ILLEGAL_PARKING: /* 违规停车识别 */
        change.nEnIllegalParking = bEnable;
        break;
    case Event::Type::ILLEGAL_LANE_CHANGE: /* 违规变道识别 */
        change.nEnIllegalLaneChange = bEnable;
        break;
    case Event::Type::PLATE_NUMBER: /* 车牌识别 */
        change.nPlateNumber = bEnable;
        break;
#endif
#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
    case Event::Type::GARBAGE_EXPOSURE: /* 垃圾暴露识别 */
        change.nEnGarbageExposure = bEnable;
        break;
    case Event::Type::GARBAGE_OVERFLOW: /* 垃圾满溢识别 */
        change.nEnGarbageOverflow = bEnable;
        break;
#endif
#if CAP_AI_PEOPLE_STATISTICS
    case Event::Type::PEOPLE_FLOW_STATISTICS: /* 人流统计 */
        change.nEnPeopleFlowStatistics = bEnable;
        break;
    case Event::Type::PEOPLE_DENSITY_DETECTION: /* 人员密度检测 */
        change.nEnPeopleDensityDetection = bEnable;
        break;
#endif
    /* 未知或无效类型 */
    default:
        dlog_error("无效的事件类型: %d", enEventType);
        break;
    }
}

void CEventManage::calculate_current_algorithm_state()
{
    /* 获取当前星期几和当天已过的秒数 */
    int nDayOfWeek = TimeUtils_NS::getTodayDayOfWeek();
    int nCurrentTime = TimeUtils_NS::getSecondsSinceStartOfDay();
    /* 转换为从星期一开始的索引：星期一为 0，星期天为 6 */
    int nIndex = (nDayOfWeek == 0) ? 6 : nDayOfWeek - 1;

    std::set<Alarm::EventSchedule_S> stEventScheduleSnapshot;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        stEventScheduleSnapshot = m_stEventSchedule;
    }

    Event::AlgorithmConfig_S stNewConfig;
    /* 遍历所有事件的布防计划 */
    for (const auto &schedule : stEventScheduleSnapshot)
    {
        bool bShouldEnable = false;
        /* 只有当事件启用时才需要检查布防时间 */
        if (schedule.bStatus)
        {
#ifdef SCENE_INTELLIGENT_ANALYSIS
            /* 特殊处理画面分析事件的定时分析模式 */
            if (schedule.enEventType == Event::Type_E::IMAGE_ANALYSIS)
            {
                Alarm::LLMImageAnalysis_S imageAnalysisConfig;
                if (CEventConfigure::instance()->get_configure(imageAnalysisConfig) == 0)
                {
                    bShouldEnable = check_image_analysis_schedule(imageAnalysisConfig);
                    if (bShouldEnable && imageAnalysisConfig.strAnalysisInputText != "[*]预设提问")
                    {
                        dlog_info("定时分析触发：更新配置提示词");
                        imageAnalysisConfig.strAnalysisInputText = "[*]预设提问";
                        CEventConfigure::instance()->set_configure(imageAnalysisConfig);
                    }
                }
            }
            /* 特殊处理场景智能分析总控制 */
            else if (schedule.enEventType == Event::Type_E::AI_SCENE_ANALYSIS)
            {
                Alarm::LLMAISceneAnalysis_S stAISceneAnalysisCfg;
                if (CEventConfigure::instance()->get_configure(stAISceneAnalysisCfg) == 0)
                {
                    bShouldEnable = stAISceneAnalysisCfg.bEnable;
                }
            }
            else
#endif
            {
                /* 检查当前时间是否在布防时间段内 */
                if (nIndex < static_cast<int>(schedule.defenseTime.size()))
                {
                    const auto &daySchedules = schedule.defenseTime[nIndex];
                    for (const auto &schedTime : daySchedules)
                    {
                        /* 将开始时间和结束时间转换为秒 */
                        int nStartTime = schedTime.stStart.nHour * 3600 + schedTime.stStart.nMinute * 60 +
                                         schedTime.stStart.nSecond;
                        int nEndTime = schedTime.stStop.nHour * 3600 + schedTime.stStop.nMinute * 60 + schedTime.stStop.nSecond;
                        /* 判断当前时间是否在布防时间段内（包含开始时间，不包含结束时间） */
                        if (nCurrentTime >= nStartTime && nCurrentTime < nEndTime)
                        {
                            bShouldEnable = true;
                            break; // 只要在任一时间段内就是开启状态
                        }
                    }
                }
            }
        }
        /* 设置算法状态：只有当事件启用且在布防时间内时才开启 */
        set_event_type(stNewConfig, schedule.enEventType, bShouldEnable);
        // dlog_debug("事件类型: %d, 使能状态: %d, 应开启算法: %d", schedule.enEventType, schedule.bStatus, bShouldEnable);
    }

    /* 更新算法状态快照 */
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_changes = stNewConfig;
    }
}

#ifdef SCENE_INTELLIGENT_ANALYSIS
bool CEventManage::check_image_analysis_schedule(const Alarm::LLMImageAnalysis_S &imageAnalysisConfig)
{
    /* 画面分析未启用或未启用定时分析时，不触发 */
    if (!imageAnalysisConfig.bEnable || !imageAnalysisConfig.bScheduleEnable)
    {
        return false;
    }

    /* 启用了定时分析，根据定时分析模式进行检查 */
    switch (static_cast<Alarm::AnalysisSchedule_E>(imageAnalysisConfig.enAnalysisScheduleMode))
    {
    case Alarm::AnalysisSchedule_E::REPEATED:
       /* 重复分析模式 */
    //    dlog_debug("画面分析: 检查重复分析模式");
        return check_repeated_analysis_schedule(imageAnalysisConfig.stRepeatedConfig);
    case Alarm::AnalysisSchedule_E::INTERVAL:
       /* 间隔分析模式 */
    //    dlog_debug("画面分析: 检查间隔分析模式");
        return check_interval_analysis_schedule(imageAnalysisConfig.stIntervalConfig);
    default:
        dlog_error("未知的画面分析定时模式: %d", imageAnalysisConfig.enAnalysisScheduleMode);
        return false;
    }
}

bool CEventManage::check_repeated_analysis_schedule(const Alarm::RepeatedAnalysisConfig_S &repeatedConfig)
{
    /*当前星期几*/
    int nDayOfWeek = TimeUtils_NS::getTodayDayOfWeek();
    /*自当天开始的秒数*/
    int nCurrentTime = TimeUtils_NS::getSecondsSinceStartOfDay();

    /* 转换星期几：nDayOfWeek (1=周一, 7=周日) -> repeatedConfig.aWeekdays (1=周一, 7=周日) */
    int nConfigWeekday = (nDayOfWeek == 0) ? 7 : nDayOfWeek;

    /* 检查当前星期几是否在配置的执行日列表中 */
    bool bIsConfiguredWeekday = false;
    for (int weekday : repeatedConfig.aWeekdays)
    {
        if (nConfigWeekday == weekday)
        {
            bIsConfiguredWeekday = true;
            break;
        }
    }

    if (!bIsConfiguredWeekday)
    {
        // dlog_debug("重复分析: 当前星期%d不在配置的执行星期列表中", nConfigWeekday);
        return false;
    }

    /* 检查是否到达配置的执行时间（允许 3 秒容差） */
    int nConfigTime = repeatedConfig.stExecuteTime.nHour * 3600 + repeatedConfig.stExecuteTime.nMinute * 60 +
                      repeatedConfig.stExecuteTime.nSecond;

    /* 允许在执行时间的前后3s内触发，避免错过执行时机 */
    bool bTimeMatched = (nCurrentTime >= nConfigTime && nCurrentTime < nConfigTime + 3);
    
    if (bTimeMatched)
    {
        /* 构建星期几的显示字符串 */
        std::string weekdayStr = "";
        for (size_t i = 0; i < repeatedConfig.aWeekdays.size(); ++i)
        {
            if (i > 0) weekdayStr += ",";
            weekdayStr += std::to_string(repeatedConfig.aWeekdays[i]);
        }
        return true;
    }
    
    return false;
}

bool CEventManage::check_interval_analysis_schedule(const Alarm::IntervalAnalysisConfig_S &intervalConfig)
{
    /* 获取当前日期 */
    Common::Date_S currentDate = Common::Date_S::GetCurrentDate();
        
    /* 检查当前日期是否在执行日期范围内 */
    if (currentDate < intervalConfig.stStartDate || currentDate > intervalConfig.stEndDate)
    {
        return false;
    }

    /* 计算间隔时间（秒） */
    int nIntervalSeconds = intervalConfig.stIntervalTime.nHour * 3600 + intervalConfig.stIntervalTime.nMinute * 60 +
                           intervalConfig.stIntervalTime.nSecond;

    /* 如果间隔时间为0，则不执行 */
    if (nIntervalSeconds <= 0)
    {
        dlog_warn("间隔分析: 间隔时间为0，不执行分析");
        return false;
    }
    /* 获取当前时间 */
    time_t currentTime = time(nullptr);
    /* 3 秒内重复请求直接放行，防止立即关闭时误判 */
    if (currentTime - m_lastIntervalAnalysisTime < 3)
    {
        return true;
    }

    /* 检查是否达到间隔时间 */
    if (currentTime - m_lastIntervalAnalysisTime >= nIntervalSeconds)
    {
        /* 更新上次执行时间 */
        m_lastIntervalAnalysisTime = currentTime;
        return true;
    }
    return false;
}
#endif

void CEventManage::print_algorithm_config_changes(const Event::AlgorithmConfig_S &oldConfig,
                                                  const Event::AlgorithmConfig_S &newConfig)
{
    struct EventInfo
    {
        int Event::AlgorithmConfig_S::*member;
        const char *name;
    };

    static const EventInfo events[] = {
        /* 普通事件 */
        {              &Event::AlgorithmConfig_S::nEnMotionDetect,             "移动侦测" },
        {           &Event::AlgorithmConfig_S::nEnOcclusionDetect,             "遮挡侦测" },
        {              &Event::AlgorithmConfig_S::nEnAnomalyAlarm,             "异常报警" },
        {                &Event::AlgorithmConfig_S::nEnAudioAlarm,             "声音报警" },
        {                &Event::AlgorithmConfig_S::nEnAlarmInput,             "报警输入" },
        {               &Event::AlgorithmConfig_S::nEnAlarmOutput,             "报警输出" },
        {                &Event::AlgorithmConfig_S::nEnFlashAlarm,          "闪光灯报警" },
        {                  &Event::AlgorithmConfig_S::nEnPIRAlarm,    "PIR红外感应报警" },

        /* 周界事件 */
        {              &Event::AlgorithmConfig_S::nEnLineCrossing,             "越界侦测" },
        {                 &Event::AlgorithmConfig_S::nEnIntrusion,             "区域入侵" },
        {               &Event::AlgorithmConfig_S::nEnEnterRegion,             "进入区域" },
        {               &Event::AlgorithmConfig_S::nEnLeaveRegion,             "离开区域" },

        /* Smart事件 */
        {              &Event::AlgorithmConfig_S::nEnAudioAnomaly,       "音频异常侦测" },
        {               &Event::AlgorithmConfig_S::nEnSceneChange,             "场景变更" },
        {                &Event::AlgorithmConfig_S::nEnFaceDetect,             "人脸侦测" },
        {           &Event::AlgorithmConfig_S::nEnLoiteringDetect,             "徘徊侦测" },
        {            &Event::AlgorithmConfig_S::nEnCrowdGathering,             "人员聚集" },
        {             &Event::AlgorithmConfig_S::nEnParkingDetect,             "停车侦测" },
        {          &Event::AlgorithmConfig_S::nEnUnattendedObject,             "物品遗留" },
        {             &Event::AlgorithmConfig_S::nEnObjectRemoval,             "物品拿取" },
        {            &Event::AlgorithmConfig_S::nEnPetRecognition,             "宠物识别" },
        {               &Event::AlgorithmConfig_S::nEnFaceCapture,             "人脸抓拍" },
#ifdef SCENE_INTELLIGENCE
        /* 场景智能 */
        {               &Event::AlgorithmConfig_S::nEnSleepOnDuty,             "睡岗识别" },
        {                 &Event::AlgorithmConfig_S::nEnLeavePost,             "离岗识别" },
        { &Event::AlgorithmConfig_S::nEnElectricVehicleInElevator, "电瓶车进电梯识别" },
        {            &Event::AlgorithmConfig_S::nEnPersonFallDown,       "人员倒地识别" },
        {                      &Event::AlgorithmConfig_S::nEnTrip,             "摔倒识别" },
        {                   &Event::AlgorithmConfig_S::nEnSmoking,             "抽烟识别" },
        {                &Event::AlgorithmConfig_S::nEnPhoneUsage,          "玩手机识别" },
        {                 &Event::AlgorithmConfig_S::nEnSmokeFire,             "烟火识别" },
        {                 &Event::AlgorithmConfig_S::nEnOpenFlame,             "明火识别" },
        {      &Event::AlgorithmConfig_S::nEnManholeCoverAbnormal,       "井盖异常检测" },
        {                  &Event::AlgorithmConfig_S::nEnBareSoil,       "黄土裸露识别" },
        {         &Event::AlgorithmConfig_S::nEnHoleProtectionBar,    "洞口防护栏识别" },
        {       &Event::AlgorithmConfig_S::nEnPedestrianIntrusion,       "行人闯入识别" },
        {              &Event::AlgorithmConfig_S::nEnSafetyHelmet,          "安全帽识别" },
        {        &Event::AlgorithmConfig_S::nEnReflectiveClothing,          "反光衣识别" },
        {      &Event::AlgorithmConfig_S::nEnHighAltitudeSeatbelt,    "高空安全带识别" },
        {    &Event::AlgorithmConfig_S::nEnConstructionOccupyRoad,       "施工占道识别" },
        {    &Event::AlgorithmConfig_S::nEnEmergencyLaneOccupancy, "应急车道占用识别" },
        {          &Event::AlgorithmConfig_S::nEnReverseDirection,             "逆行识别" },
        {  &Event::AlgorithmConfig_S::nEnNonMotorVehicleIntrusion, "非机动车闯入识别" },
        {               &Event::AlgorithmConfig_S::nEnRoadPonding,       "道路积水识别" },
        {                &Event::AlgorithmConfig_S::nEnCongestion,             "拥堵识别" },
        {            &Event::AlgorithmConfig_S::nEnIllegalParking,             "停车识别" },
        {         &Event::AlgorithmConfig_S::nEnIllegalLaneChange,       "违规变道识别" },
        {                 &Event::AlgorithmConfig_S::nPlateNumber,             "车牌识别" },
#endif
#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
        {           &Event::AlgorithmConfig_S::nEnGarbageExposure,       "垃圾暴露识别" },
        {           &Event::AlgorithmConfig_S::nEnGarbageOverflow,       "垃圾满溢识别" },
#endif
#if CAP_AI_PEOPLE_STATISTICS
        {      &Event::AlgorithmConfig_S::nEnPeopleFlowStatistics,             "人流统计" },
        {    &Event::AlgorithmConfig_S::nEnPeopleDensityDetection,       "人员密度检测" },
#endif
    };

    for (const auto &event : events)
    {
        int oldValue = oldConfig.*event.member;
        int newValue = newConfig.*event.member;
        if (oldValue != newValue)
        {
            dlog_info("%s状态变更: %d -> %d", event.name, oldValue, newValue);
        }
    }
}

void CEventManage::run()
{
    pthread_setname_np(pthread_self(), "EventManageRun");

    /* 状态记录，避免重复通知 */
    Event::AlgorithmConfig_S lastConfig;
    bool bFirstRun = true;

    while (m_bRunning.load(std::memory_order_acquire))
    {
        sleep(1);

        /* 计算当前的算法状态 */
        calculate_current_algorithm_state();

        /* 检查是否有状态变化 */
        if (bFirstRun || m_changes != lastConfig)
        {
            if (!bFirstRun)
            {
                dlog_info("定时检查发现算法状态变化，已通知AI_APP模块");
                // 可以在这里添加详细的状态变化日志
                print_algorithm_config_changes(lastConfig, m_changes);
            }

            update_event_switch_status();
            lastConfig = m_changes;
            bFirstRun = false;
        }
    }
}
