/**
 * @FilePath     : isp_dayNight.cpp
 * @Author       : cyc
 * @Date         : 2025-08-27 19:02:28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-08 10:06:04
 * @Description  : 日夜切换控制器实现
 */

#include "isp_dayNight.h"
#include "dlog.h"
#include "IpcRet.h"
#include "ss_mpi_isp.h"
#include <ctime>
#include "os.h"
#include "ot_common_isp.h"
#include "ss_mpi_ae.h"
#include "isp_configure.h"
#include "light_manager.h"

CDayNightController::CDayNightController()
{
    /* 初始化IR自动切换属性 */
    m_irAttr.enable = TD_TRUE;
    m_irAttr.normal_to_ir_iso_threshold = NORMAL_TO_IR_THRESHOLD;
    m_irAttr.ir_to_normal_iso_threshold = IR_TO_NORMAL_THRESHOLD;
    // m_irAttr.rg_max = 300;
    // m_irAttr.rg_min = 130;
    // m_irAttr.bg_max = 300;
    // m_irAttr.bg_min = 100;
    /* 红外切白天的条件 */
    m_irAttr.rg_max = 200;
    m_irAttr.rg_min = 150;
    m_irAttr.bg_max = 130;
    m_irAttr.bg_min = 100;
    m_irAttr.ir_status = OT_ISP_IR_STATUS_NORMAL;

    auto now = std::chrono::steady_clock::now();
    /* 初始化条件开始时间 */
    m_lastSwitchTime = now;
    m_conditionStartTime = now;
    /* 获取补光模式 */
    ISP::DayNightAttr_S stDayNightAttr;
    CIspConfigure::instance()->get_configure(stDayNightAttr);
    m_enLightType = stDayNightAttr.stFillLight.enLightType;
}

CDayNightController::~CDayNightController()
{
    stop();
}

bool CDayNightController::start()
{
    if (m_running.load())
    {
        dlog_warn("DayNight controller already running");
        return true;
    }

    m_running.store(true);

    m_workerThread = std::make_unique<std::thread>(&CDayNightController::workerThread, this);
    if (m_workerThread)
    {
        dlog_info("DayNight controller started successfully");
        return true;
    }
    else
    {
        dlog_error("Failed to start day night controller");
        m_running.store(false);
        return false;
    }
}

void CDayNightController::stop()
{
    if (!m_running.load())
    {
        return;
    }

    m_running.store(false);

    if (m_workerThread && m_workerThread->joinable())
    {
        m_workerThread->join();
    }

    m_workerThread.reset();
    dlog_info("DayNight controller stopped");
}

void CDayNightController::setMode(DayNightMode_E enDayNightMode)
{
    // if (m_currentMode == enDayNightMode)
    // {
    //     dlog_info("日夜模式未改变，不进行设置");
    //     return;
    // }

    m_currentMode.store(enDayNightMode);

    switch (enDayNightMode)
    {
    case DAY_MODE:
    {
        dlog_info("强制切换到白天模式");
        /* 停止自动检测，强制切换到白天 */
        forceSwitch(false);
        m_irAttr.ir_status = OT_ISP_IR_STATUS_NORMAL;
        break;
    }
    case NIGHT_MODE:
    {
        dlog_info("强制切换到夜晚模式");
        /* 停止自动检测，强制切换到夜晚 */
        forceSwitch(true);
        m_irAttr.ir_status = OT_ISP_IR_STATUS_IR;
        break;
    }
    case AUTO_MODE:
    {
        dlog_info("启动自动模式");
        /* 重置自动检测状态 */
        m_needSwitch.store(false);
        /* 重置ISP红外状态为正常，避免沿用强制夜晚模式的残留状态导致自动检测被跳过 */
        m_irAttr.ir_status = OT_ISP_IR_STATUS_NORMAL;
        m_thresholdNeedsUpdate.store(true);
        break;
    }
    case TIME_MODE:
    {
        dlog_info("启动定时模式");
        /* 立即检查当前时间应该处于什么状态 */
        bool shouldBeNight = shouldBeNightByTime();
        bool currentIsNight = m_isNight.load();
        dlog_info("定时模式：当前应该处于%s模式", shouldBeNight ? "夜晚" : "白天");
        if (shouldBeNight != currentIsNight)
        {
            performStateChange(shouldBeNight);
        }
        break;
    }
    default:
    {
        dlog_error("Unknown day night mode: %d", enDayNightMode);
        break;
    }
    }
}

void CDayNightController::setTimeRange(const TimeRange_S &timeRange)
{
    m_timeRange = timeRange;
    dlog_info("Time range updated: %02d:%02d:%02d - %02d:%02d:%02d",
              timeRange.stStartTime.nHour,
              timeRange.stStartTime.nMinute,
              timeRange.stStartTime.nSecond,
              timeRange.stEndTime.nHour,
              timeRange.stEndTime.nMinute,
              timeRange.stEndTime.nSecond);

    /* 如果当前是定时模式，立即重新评估状态 */
    if (m_currentMode.load() == TIME_MODE)
    {
        bool shouldBeNight = shouldBeNightByTime();
        bool currentIsNight = m_isNight.load();
        if (shouldBeNight != currentIsNight)
        {
            dlog_info("时间范围更新后立即调整状态到：%s", shouldBeNight ? "夜晚" : "白天");
            performStateChange(shouldBeNight);
        }
    }
}

void CDayNightController::setSensitivity(unsigned int sensitivity)
{
    m_sensitivity.store(sensitivity);
    m_thresholdNeedsUpdate.store(true);
    /* 获取补光模式 */
    ISP::DayNightAttr_S stDayNightAttr;
    CIspConfigure::instance()->get_configure(stDayNightAttr);
    m_enLightType = stDayNightAttr.stFillLight.enLightType;
}

void CDayNightController::setFillLight()
{
    ISP::DayNightAttr_S stDayNightAttr;
    CIspConfigure::instance()->get_configure(stDayNightAttr);
    if (stDayNightAttr.enLightMode == LightBrightMode_E::MANUAL_LIGHT_BRIGHT && m_isNight.load() == true)
    {
        /* 灯光管理器控制补光灯 */
        CLightManager::instance()->on_dayNight_changed(m_isNight.load(), stDayNightAttr.stFillLight);
    }
}

void CDayNightController::setFilterTime(unsigned int filterTimeSeconds)
{
    if (filterTimeSeconds < FILTER_TIME_MIN)
    {
        filterTimeSeconds = FILTER_TIME_MIN;
    }
    else if (filterTimeSeconds > FILTER_TIME_MAX)
    {
        filterTimeSeconds = FILTER_TIME_MAX;
    }

    m_filterTime.store(filterTimeSeconds);
    dlog_info("Filter time updated to: %u seconds", filterTimeSeconds);
}

void CDayNightController::setStateChangeCallback(StateChangeCallback callback)
{
    m_stateChangeCallback = std::move(callback);
}

void CDayNightController::forceSwitch(bool toNight)
{
    dlog_info("强制切换到%s模式", toNight ? "夜晚" : "白天");
    /* 首次强制切换或状态变化时，都触发状态变更 */
    if (m_firstForcedSwitch.load() || m_isNight.load() != toNight)
    {
        performStateChange(toNight);
        m_firstForcedSwitch.store(false);
    }
    /* 更新最后切换时间，防止立即被其他模式覆盖 */
    m_lastSwitchTime = std::chrono::steady_clock::now();
}

void CDayNightController::workerThread()
{
    while (m_running.load())
    {
        if (!access("testPrint_DayNight", F_OK))
        {
            ot_isp_wb_stats stat;
            /* 获取白平衡统计信息 */
            ss_mpi_isp_get_wb_stats(m_viPipe, &stat);
            td_u32 rg, bg;
            rg = ((td_u32) stat.global_r << SHIFT_8BIT) / div_0_to_1(stat.global_g);
            bg = ((td_u32) stat.global_b << SHIFT_8BIT) / div_0_to_1(stat.global_g);
            // dlog_debug("rg_max:%u, rg_min:%u, bg_max:%u, bg_min:%u", (rg * GAIN_MAX_COEF) >> SHIFT_8BIT, (rg *
            // GAIN_MIN_COEF) >> SHIFT_8BIT, (bg * GAIN_MAX_COEF) >> SHIFT_8BIT, (bg * GAIN_MIN_COEF) >> SHIFT_8BIT); //
            // 获取实时的r/g、b/g
            dlog_debug("[日夜转换] rg:[%u],bg:[%u]", rg, bg);

            // 获取自动曝光内部状态信息，包括全局1024段直方图和平均亮度等统计信息。
            // 同时还可获取AE运行状态中的曝光时间、增益、曝光量和实际生效的AE route等信息
            int ret = ss_mpi_isp_query_exposure_info(m_viPipe, &isp_exp_info);
            if (OK != ret)
            {
                dlog_error("ss_mpi_isp_query_exposure_info failed:%d", ret);
                return;
            }

            dlog_debug("[日夜转换] ir_status:[%d],bright:[%u],exposure:[%u],ave_lum:[%u],iso:[%u]",
                       m_irAttr.ir_status,
                       (isp_exp_info.exposure * isp_exp_info.ave_lum),
                       isp_exp_info.exposure,
                       isp_exp_info.ave_lum,
                       isp_exp_info.iso);
        }

        auto currentMode = m_currentMode.load();

        /* 只有在相应模式激活时才执行检测 */
        switch (currentMode)
        {
        case AUTO_MODE:
        {
            handleAutoMode();
            break;
        }
        case TIME_MODE:
        {
            handleTimeMode();
            break;
        }
        case DAY_MODE:
        case NIGHT_MODE:
            /* 强制模式不需要线程处理，只需要保持状态 */
            break;

        default:
            dlog_warn("Unknown mode in worker thread: %d", currentMode);
            break;
        }

        std::this_thread::sleep_for(THREAD_SLEEP_INTERVAL);
    }

    dlog_info("DayNight worker thread exited");
}

bool CDayNightController::isMinIntervalSatisfied() const
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastSwitchTime);

    /* 至少间隔5秒，避免频繁切换（可配置为常量） */
    const unsigned int MIN_INTERVAL_SEC = 5;
    return elapsed.count() >= MIN_INTERVAL_SEC;
}

void CDayNightController::handleAutoMode()
{
    /* 初始化为当前状态 */
    bool toNight = m_isNight.load();
    /* 当前帧是否满足条件 */
    bool bConditionNowMet = false;

    auto now = std::chrono::steady_clock::now();

    if (m_thresholdNeedsUpdate.exchange(false))
    {
        bool bIspThinksIsNight = (m_irAttr.ir_status == OT_ISP_IR_STATUS_IR);

        dlog_info("阈值参数已更新，ISP状态=%s, 本地状态=%s", bIspThinksIsNight ? "夜间" : "白天", toNight ? "夜间" : "白天");

        /* 仅在 ISP 状态与本地状态不一致时，才执行状态切换 */
        // if (m_isNight.load() != bIspThinksIsNight)
        // {
        performStateChange(bIspThinksIsNight);
        // }

        /* 重置计时器，准备下一轮过滤 */
        m_switchState.store(SwitchState_E::IDLE);
        dlog_debug("计时器已重置");
        return;
    }

    /* 调用ISP自动检测 */
    int nRet = ss_mpi_isp_ir_auto(m_viPipe, &m_irAttr);
    if (nRet != OK)
    {
        dlog_error("ss_mpi_isp_ir_auto failed: %d", nRet);
        return;
    }

    nRet = ss_mpi_isp_query_exposure_info(m_viPipe, &isp_exp_info);
    if (OK != nRet)
    {
        dlog_error("ss_mpi_isp_query_exposure_info failed:%d", nRet);
        return;
    }

    ot_isp_wb_stats stat;
    ss_mpi_isp_get_wb_stats(m_viPipe, &stat);
    td_u32 rg, bg;
    rg = ((td_u32) stat.global_r << SHIFT_8BIT) / div_0_to_1(stat.global_g);
    bg = ((td_u32) stat.global_b << SHIFT_8BIT) / div_0_to_1(stat.global_g);

    // dlog_debug("(%d, %d) (%d, %d) (%d, %d) %d %d %f", m_irAttr.normal_to_ir_iso_threshold,
    // m_irAttr.ir_to_normal_iso_threshold, m_irAttr.bg_min, m_irAttr.bg_max, m_irAttr.rg_min, m_irAttr.rg_max,
    // m_irAttr.ir_status, m_irAttr.ir_switch, fSensitivity);

    /* 根据灵敏度获取白天切夜间的亮度阈值 */
    uint32_t nDayToNightThresh = BRIGHT_DAY_TO_NIGHT_VALUE;
    int currentSens = m_sensitivity.load();
    auto it = m_mapBrightDayToNight.find(currentSens);
    if (it != m_mapBrightDayToNight.end())
    {
        nDayToNightThresh = it->second;
    }
    else
    {
        dlog_warn("未知灵敏度：%d，使用默认阈值", currentSens);
    }

    /* 当前亮度指标 (使用64位防止溢出) */
    uint64_t u64CurrentBright = (uint64_t) isp_exp_info.exposure * isp_exp_info.ave_lum;

    /* 当前是白天 -> 判断是否切夜间 */
    if (m_irAttr.ir_status == OT_ISP_IR_STATUS_NORMAL)
    {
        // 比值判断 (适用于黄昏/低光)
        bool bMetStandard = (u64CurrentBright > nDayToNightThresh);
        // 极低照度补丁 (适用于纯黑环境 ave_lum=0~1 的情况)
        // 当画面几乎全黑(ave_lum很低)且曝光时间/增益很高时，强制切夜间
        // 阈值说明: ave_lum < 5 表示画面极黑, exposure > 200000 表示AE已经拉高了曝光
        bool bMetPitchBlack = (isp_exp_info.ave_lum < 5 && isp_exp_info.exposure > 200000);

        if (bMetStandard || bMetPitchBlack)
        {
            bConditionNowMet = true;
            toNight = true;
            dlog_debug("条件满足: 白天→夜间, 当前值=%llu, 阈值=%u (MetStd:%d, MetBlack:%d, Exp:%u, Lum:%u)",
                       u64CurrentBright,
                       nDayToNightThresh,
                       bMetStandard,
                       bMetPitchBlack,
                       isp_exp_info.exposure,
                       isp_exp_info.ave_lum);
        }
    }
    /* 当前是夜间 -> 判断是否切白天 */
    else if (m_irAttr.ir_status == OT_ISP_IR_STATUS_IR)
    {
        bool bSwitchToDay = false;
        const char *pReason = "None";

        /* 分类处理：根据补光灯类型选择不同的判定逻辑 */
        if (m_enLightType == LightType_E::LIGHT_TYPE_WHITE)
        {
            /* 白光模式切白天 */
            bool bBrightMet = (u64CurrentBright < BRIGHT_WHITE_LIGHT_TO_DAY_VALUE);
            bool bColorMet = (rg < WHITE_RG_VALUE && bg < WHITE_BG_VALUE);

            if (bBrightMet && bColorMet)
            {
                bSwitchToDay = true;
                pReason = "白光模式";
            }
        }
        else if (m_enLightType == LightType_E::LIGHT_TYPE_RED || m_enLightType == LightType_E::LIGHT_TYPE_SMART)
        {
            /* 红外/智能模式切白天 */
            bool bBrightMet = (u64CurrentBright < BRIGHT_RED_LIGHT_TO_DAY_VALUE);
            bool bColorMet = (rg < RED_RG_VALUE && bg < RED_BG_VALUE);

            if (bBrightMet && bColorMet)
            {
                bSwitchToDay = true;
                pReason = "红外/智能模式";
            }
        }

        /* 统一执行切白天逻辑 */
        if (bSwitchToDay)
        {
            bConditionNowMet = true;
            toNight = false; // 目标变为白天
            dlog_debug("条件满足: 夜间->白天满足 (%s): Bright=%llu, RG=%u, BG=%u", pReason, u64CurrentBright, rg, bg);
        }
    }

    auto currentState = m_switchState.load();

    if (!bConditionNowMet)
    {
        /* 条件不满足 → 彻底重置状态机 */
        if (currentState != SwitchState_E::IDLE)
        {
            dlog_debug("条件不满足，计时器已重置（原状态=%d）", static_cast<int>(currentState));
            m_switchState.store(SwitchState_E::IDLE);
        }
        return;
    }

    /* 条件满足，进入状态机 */
    switch (currentState)
    {
    case SwitchState_E::IDLE:
    {
        /* 首次满足条件 → 启动计时器（此时刻之前不计时） */
        m_conditionStartTime = now;
        m_switchState.store(SwitchState_E::FIRST_MET);
        dlog_info("计时器已启动: 目标=%s, 过滤时间=%u秒", toNight ? "夜间" : "白天", m_filterTime.load());
        break;
    }

    case SwitchState_E::FIRST_MET:
    case SwitchState_E::WAITING:
    {
        /* 持续满足条件 → 计算已持续时间 */
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_conditionStartTime).count();

        dlog_debug("计时中: 已持续 %lld/%u 秒", elapsed, m_filterTime.load());

        if (elapsed >= static_cast<long long>(m_filterTime.load()))
        {
            m_switchState.store(SwitchState_E::TRIGGERED);
            dlog_info("计时完成，触发切换: %s", toNight ? "夜间" : "白天");
        }
        else
        {
            m_switchState.store(SwitchState_E::WAITING);
        }
        break;
    }

    case SwitchState_E::TRIGGERED:
    {
        /* 已触发，等待执行 */
        break;
    }
    }

    /* 执行切换（仅TRIGGERED状态） */

    if (m_switchState.load() == SwitchState_E::TRIGGERED && isMinIntervalSatisfied())
    {
        dlog_info("[AUTO] 执行切换: %s 模式", toNight ? "夜间" : "白天");

        performStateChange(toNight);
        m_irAttr.ir_status = toNight ? OT_ISP_IR_STATUS_IR : OT_ISP_IR_STATUS_NORMAL;
        m_lastSwitchTime = now;

        /* 重置状态机 */
        m_switchState.store(SwitchState_E::IDLE);
    }
    // dlog_debug("(%d, %d) (%d, %d) (%d, %d) %d %d %f", m_irAttr.normal_to_ir_iso_threshold,
    // m_irAttr.ir_to_normal_iso_threshold, m_irAttr.bg_min, m_irAttr.bg_max, m_irAttr.rg_min, m_irAttr.rg_max,
    // m_irAttr.ir_status, m_irAttr.ir_switch, fSensitivity);
}

void CDayNightController::handleTimeMode()
{

    bool shouldBeNight = shouldBeNightByTime();
    bool currentIsNight = m_isNight.load();

    if (shouldBeNight != currentIsNight)
    {
        dlog_info("[TIME] 执行切换: %s 模式", shouldBeNight ? "夜间" : "白天");
        performStateChange(shouldBeNight);
        m_lastSwitchTime = std::chrono::steady_clock::now();
    }
}

bool CDayNightController::shouldBeNightByTime() const
{
    auto now = std::time(nullptr);
    auto *localTime = std::localtime(&now);

    int currentSeconds = localTime->tm_hour * 3600 + localTime->tm_min * 60 + localTime->tm_sec;

    int startSeconds = m_timeRange.stStartTime.nHour * 3600 + m_timeRange.stStartTime.nMinute * 60 +
                       m_timeRange.stStartTime.nSecond;

    int endSeconds = m_timeRange.stEndTime.nHour * 3600 + m_timeRange.stEndTime.nMinute * 60 + m_timeRange.stEndTime.nSecond;

    /* 判断是否在白天时间范围内 */
    bool isDayTime;
    if (startSeconds < endSeconds)
    {
        /* 正常情况：如 08:00 - 18:00 */
        isDayTime = (currentSeconds >= startSeconds) && (currentSeconds < endSeconds);
    }
    else
    {
        /* 跨天情况：如 18:00 - 08:00 (次日) */
        isDayTime = (currentSeconds >= startSeconds) || (currentSeconds < endSeconds);
    }

    return !isDayTime;
}

bool CDayNightController::isFilterTimeSatisfied() const
{
    /* 必须条件已满足，并且持续了足够时间 */
    if (!m_conditionMet.load())
    {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_conditionStartTime);

    return elapsed.count() >= m_filterTime.load();
}

void CDayNightController::performStateChange(bool bToNight)
{
    /* 已完成首次同步且状态未变，不触发冗余回调 */
    if (m_firstStateSynced.load() && m_isNight.load() == bToNight)
    {
        return;
    }
    m_firstStateSynced.store(true);
    m_isNight.store(bToNight);

    /* 调用回调通知状态变化 */
    if (m_stateChangeCallback)
    {
        m_stateChangeCallback(bToNight, m_currentMode.load());
    }
}
