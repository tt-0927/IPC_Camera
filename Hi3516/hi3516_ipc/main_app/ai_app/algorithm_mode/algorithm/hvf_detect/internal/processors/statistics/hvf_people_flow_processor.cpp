/**
 * @FilePath     : hvf_people_flow_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-22 18:44:48
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-29 15:16:07
 * @Description  : HVF 人流统计处理器实现
 */

#include "hvf_people_flow_processor.hpp"

#if CAP_AI_PEOPLE_STATISTICS
#include <algorithm>
#include <string>

#include "common_process.h"
#include "dlog.h"

namespace
{
/**
 * @brief   : 判断人流统计规则线是否有效
 * @param    {PeopleFlowRuleLine_S} &stRuleLine：规则线
 * @return   {bool} true：有效 false：无效
 */
bool is_valid_rule_line(const Alarm::PeopleFlowRuleLine_S &stRuleLine)
{
    return (stRuleLine.stStartPos.fX != stRuleLine.stEndPos.fX) ||
           (stRuleLine.stStartPos.fY != stRuleLine.stEndPos.fY);
}

/**
 * @brief   : 获取穿越方向的反方向
 * @param    {CrossDirection_E} enDirection：穿越方向
 * @return   {CrossDirection_E} 反方向
 */
Alarm::CrossDirection_E get_reverse_direction(Alarm::CrossDirection_E enDirection)
{
    if (enDirection == Alarm::CrossDirection_E::A_TO_B)
    {
        return Alarm::CrossDirection_E::B_TO_A;
    }

    if (enDirection == Alarm::CrossDirection_E::B_TO_A)
    {
        return Alarm::CrossDirection_E::A_TO_B;
    }

    return Alarm::CrossDirection_E::CROSS_DIRECTION_INVALID;
}

/**
 * @brief   : 构造人体目标矩形
 * @param    {ot_aidetect_object} &stObject：HVF 目标
 * @return   {Common::RectInfo_S} 目标矩形
 */
Common::RectInfo_S build_rect(const ot_aidetect_object &stObject)
{
    /* 当前人体目标框，转换为通用矩形坐标结构 */
    Common::RectInfo_S stRect;
    stRect.nX1 = stObject.detect_rect.x;
    stRect.nY1 = stObject.detect_rect.y;
    stRect.nX2 = stObject.detect_rect.x + stObject.detect_rect.width;
    stRect.nY2 = stObject.detect_rect.y + stObject.detect_rect.height;
    return stRect;
}

/**
 * @brief   : 构造人流统计目标快照
 * @param    {ot_aidetect_object} &stObject：HVF 目标
 * @param    {SnapshotType_E} enSnapshotType：快照类型
 * @param    {CrossDirection_E} enDirection：穿越方向
 * @param    {long long} llNowMs：当前时间戳
 * @return   {TargetSnapshot_S} 目标快照
 */
EventStatistics_NS::TargetSnapshot_S build_snapshot(const ot_aidetect_object &stObject,
                                                    EventStatistics_NS::SnapshotType_E enSnapshotType,
                                                    Alarm::CrossDirection_E enDirection,
                                                    long long llNowMs)
{
    /* 当前人流统计目标快照，记录穿线目标的框、方向和时间 */
    EventStatistics_NS::TargetSnapshot_S stSnapshot;
    stSnapshot.nTrackId = static_cast<int>(stObject.track_id);
    stSnapshot.nRuleId = 0;
    stSnapshot.enSnapshotType = enSnapshotType;
    stSnapshot.stRect = build_rect(stObject);
    stSnapshot.llTimestampMs = llNowMs;
    stSnapshot.nDirection = static_cast<int>(enDirection);
    return stSnapshot;
}

/**
 * @brief   : 获取人流统计滞留报警等级文本
 * @param    {Event::Type_E} enEventType：滞留报警事件类型
 * @return   {std::string} 报警等级文本
 */
std::string get_people_flow_alarm_level_text(Event::Type_E enEventType)
{
    switch (enEventType)
    {
    case Event::Type_E::PEOPLE_FLOW_STAY_NORMAL:
        return "normal";
    case Event::Type_E::PEOPLE_FLOW_STAY_MEDIUM:
        return "medium";
    case Event::Type_E::PEOPLE_FLOW_STAY_SEVERE:
        return "severe";
    default:
        return "none";
    }
}

/**
 * @brief   : 构造人流统计联动上下文
 * @param    {int} nChnId：通道号
 * @param    {long long} llNowMs：当前毫秒时间戳
 * @param    {Event::Type_E} enEventType：联动事件类型
 * @param    {CHVFPeopleFlowStateStore} &stStateStore：人流统计状态仓库
 * @return   {EventTriggerContext_S} 联动上下文
 */
EventTriggerContext_S build_people_flow_context(int nChnId,
                                                 long long llNowMs,
                                                 Event::Type_E enEventType,
                                                 const HVFDetectInternal::CHVFPeopleFlowStateStore &stStateStore)
{
    /* 人流统计联动上下文，承载规则匹配所需的摘要属性 */
    EventTriggerContext_S stContext;
    stContext.enEventType = enEventType;
    stContext.nChnId = nChnId;
    stContext.llTimestamp = llNowMs;
    stContext.mapAttrs["rule_id"] = "0";
    stContext.mapAttrs["enter_count"] = std::to_string(stStateStore.getEnterCount());
    stContext.mapAttrs["leave_count"] = std::to_string(stStateStore.getLeaveCount());
    stContext.mapAttrs["total_count"] = std::to_string(stStateStore.getTotalCount());
    stContext.mapAttrs["current_stay_count"] = std::to_string(stStateStore.getCurrentStayCount());
    stContext.mapAttrs["alarm_level"] = get_people_flow_alarm_level_text(enEventType);
    return stContext;
}

} // namespace

namespace HVFDetectInternal
{
CHVFPeopleFlowProcessor::CHVFPeopleFlowProcessor()
{
    clearStatisticsResult();
}

void CHVFPeopleFlowProcessor::setEnabled(bool bEnable)
{
    m_stAlgoCfg.bEnable = bEnable;
    if (!bEnable)
    {
        clearStatisticsResult();
    }
}

void CHVFPeopleFlowProcessor::setAlgoParamCfg(const Alarm::PeopleFlowStatistics_S &stAlgoCfg,
                                              int nWidth,
                                              int nHeight)
{
    dlog_debug("ai_app: 设置人流统计参数");
    m_stAlgoCfg = stAlgoCfg;
    m_stAlgoCfg.stRuleLine.stStartPos.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, nWidth, nHeight);
    m_stAlgoCfg.stRuleLine.stEndPos.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, nWidth, nHeight);
    m_stAlgoCfg.stDetectRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, nWidth, nHeight);

    if (!is_valid_rule_line(m_stAlgoCfg.stRuleLine) || !m_stAlgoCfg.stDetectRegion.IsValid())
    {
        m_stAlgoCfg.bEnable = false;
        dlog_warn("人流统计配置无效，关闭人流统计处理器");
    }

    m_stateStore.setConfig(m_stAlgoCfg);
}

void CHVFPeopleFlowProcessor::setReporter(const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter)
{
    m_pReporter = pReporter;
}

void CHVFPeopleFlowProcessor::process(SHVFProcessContext &stContext)
{
    if (!m_stAlgoCfg.bEnable)
    {
        return;
    }

    /* 当前帧处理时间戳，后续统计快照和联动上下文共用该时间 */
    const long long llNowMs = static_cast<long long>(get_time_ms());
    /* 当前帧是否发生进入或离开，仅用于统计变化标记，不触发主事件报警 */
    bool bStatisticsChanged = false;
    m_stateStore.maybeTimedReset(llNowMs);

    /* 当前帧人体类别检测结果，人流统计只处理人体目标 */
    const ot_aidetect_object_of_one_class *pstHumanClass = find_object_class(stContext.stResult, OT_AIDETECT_CLASS_HUMAN);
    m_indexManager.cleanupLostTargets(collect_track_ids(pstHumanClass));

    if (pstHumanClass != nullptr)
    {
        for (size_t i = 0; i < pstHumanClass->object_num; ++i)
        {
            /* 当前遍历到的人体目标 */
            const ot_aidetect_object &stObject = pstHumanClass->objects[i];
            if ((stObject.track_status == OT_AIDETECT_TRACK_STATUS_NEW ||
                 stObject.track_status == OT_AIDETECT_TRACK_STATUS_UPDATE) &&
                stObject.detect_confidence < (1.0f - m_stAlgoCfg.nSensitivity / 100.0f))
            {
                continue;
            }

            if (!is_in_region(m_stAlgoCfg.stDetectRegion, stObject))
            {
                continue;
            }

            /* 当前 track_id 对应的内部状态索引 */
            const int nInternalIndex = m_indexManager.getOrAllocateIndex(stObject.track_id);
            if (nInternalIndex < 0 || nInternalIndex >= m_indexManager.getMaxTargets())
            {
                dlog_warn("无法为track_id %u 分配索引，跳过人流统计处理", stObject.track_id);
                continue;
            }

            add_result_to_vector(stObject, stContext.vstRectInfo);

            /* 当前人体目标中心点，用于和规则线进行轨迹相交判断 */
            Common::PosF_S stCurrentPos;
            stCurrentPos.fX = stObject.detect_rect.x + stObject.detect_rect.width / 2.0f;
            stCurrentPos.fY = stObject.detect_rect.y + stObject.detect_rect.height / 2.0f;

            /* 当前目标跨线跟踪状态，按内部状态索引复用固定数组 */
            BoundaryTrackStatus_S &stTrackStatus = m_stTrackStatus[nInternalIndex];
            if (!stTrackStatus.bIsTracking)
            {
                stTrackStatus.bIsTracking = true;
                stTrackStatus.stCurrentPosition = stCurrentPos;
                stTrackStatus.stLastPosition = stCurrentPos;
                stTrackStatus.dLastUpdateTime = static_cast<double>(llNowMs);
                continue;
            }

            stTrackStatus.stLastPosition = stTrackStatus.stCurrentPosition;
            stTrackStatus.stCurrentPosition = stCurrentPos;
            stTrackStatus.dLastUpdateTime = static_cast<double>(llNowMs);

            /* 当前运动轨迹与人流统计规则线的穿越方向 */
            const Alarm::CrossDirection_E enCrossResult = tripLineDetection(stTrackStatus.stLastPosition,
                                                                            stTrackStatus.stCurrentPosition,
                                                                            m_stAlgoCfg.stRuleLine.stStartPos,
                                                                            m_stAlgoCfg.stRuleLine.stEndPos);
            if (enCrossResult == Alarm::CrossDirection_E::CROSS_DIRECTION_INVALID ||
                enCrossResult == Alarm::CrossDirection_E::BOTH_WAYS)
            {
                continue;
            }

            /* 配置方向作为进入方向，反方向自动作为离开方向 */
            const Alarm::CrossDirection_E enEnterDirection = m_stAlgoCfg.stRuleLine.enDirection;
            /* 配置进入方向的反方向，用于判定离开人数 */
            const Alarm::CrossDirection_E enLeaveDirection = get_reverse_direction(enEnterDirection);
            if (enCrossResult == enEnterDirection)
            {
                m_stateStore.onEnter(build_snapshot(stObject,
                                                    EventStatistics_NS::SnapshotType_E::ENTER,
                                                    enCrossResult,
                                                    llNowMs));
                bStatisticsChanged = true;
                dlog_info("人流统计进入: track_id[%u] direction[%d]", stObject.track_id, static_cast<int>(enCrossResult));
            }
            else if (enCrossResult == enLeaveDirection)
            {
                m_stateStore.onLeave(build_snapshot(stObject,
                                                    EventStatistics_NS::SnapshotType_E::LEAVE,
                                                    enCrossResult,
                                                    llNowMs));
                bStatisticsChanged = true;
                dlog_info("人流统计离开: track_id[%u] direction[%d]", stObject.track_id, static_cast<int>(enCrossResult));
            }
        }
    }

    for (int i = 0; i < m_indexManager.getMaxTargets(); ++i)
    {
        if (m_stTrackStatus[i].bIsTracking && (llNowMs - m_stTrackStatus[i].dLastUpdateTime) > 5000)
        {
            m_stTrackStatus[i].reset();
        }
    }

    /* 当前滞留人数对应的报警等级事件，未达阈值时返回主事件用于标识无等级报警 */
    const Event::Type_E enStayEventType = m_stateStore.getStayAlarmEventType();
    /* 普通滞留报警上下文，包含当前滞留人数与普通等级标识 */
    EventTriggerContext_S stNormalContext = build_people_flow_context(stContext.nChnId,
                                                                      llNowMs,
                                                                      Event::Type_E::PEOPLE_FLOW_STAY_NORMAL,
                                                                      m_stateStore);
    /* 中度滞留报警上下文，包含当前滞留人数与中度等级标识 */
    EventTriggerContext_S stMediumContext = build_people_flow_context(stContext.nChnId,
                                                                      llNowMs,
                                                                      Event::Type_E::PEOPLE_FLOW_STAY_MEDIUM,
                                                                      m_stateStore);
    /* 严重滞留报警上下文，包含当前滞留人数与严重等级标识 */
    EventTriggerContext_S stSevereContext = build_people_flow_context(stContext.nChnId,
                                                                      llNowMs,
                                                                      Event::Type_E::PEOPLE_FLOW_STAY_SEVERE,
                                                                      m_stateStore);
    m_normalAlarmStateMachine.handleAlarmState(enStayEventType == Event::Type_E::PEOPLE_FLOW_STAY_NORMAL,
                                               stNormalContext);
    m_mediumAlarmStateMachine.handleAlarmState(enStayEventType == Event::Type_E::PEOPLE_FLOW_STAY_MEDIUM,
                                               stMediumContext);
    m_severeAlarmStateMachine.handleAlarmState(enStayEventType == Event::Type_E::PEOPLE_FLOW_STAY_SEVERE,
                                               stSevereContext);

    if (m_pReporter && shouldEmitStatisticsReport(llNowMs))
    {
        /* 统计业务周期上报结构，和事件开始/结束联动分离，供 TVSDK 客户端持续获取统计数据 */
        EventStatistics_NS::Report_S stReport = m_stateStore.buildAndConsumeReport(stContext.nChnId, llNowMs, bStatisticsChanged);
        /* 周期上报前输出统计摘要，便于排查定时上报时的计数状态和目标缓存规模 */
        dlog_info("人流统计周期上报: 通道[%d] 序号[%u] 进入人数[%u] 离开人数[%u] 总人数[%u] "
                  "滞留人数[%u] 目标数[%zu] 是否强制[%d] 上报间隔毫秒[%lld]",
                  stReport.nChnId,
                  stReport.nReportSeq,
                  stReport.nEnterCount,
                  stReport.nLeaveCount,
                  stReport.nTotalCount,
                  m_stateStore.getCurrentStayCount(),
                  stReport.vecTargets.size(),
                  bStatisticsChanged ? 1 : 0,
                  m_llStatisticsReportIntervalMs);
        m_pReporter->report(stReport);
    }
}

bool CHVFPeopleFlowProcessor::shouldEmitStatisticsReport(long long llNowMs)
{
    if (m_llLastStatisticsReportTs <= 0)
    {
        m_llLastStatisticsReportTs = llNowMs;
        return false;
    }

    if ((llNowMs - m_llLastStatisticsReportTs) < m_llStatisticsReportIntervalMs)
    {
        return false;
    }

    m_llLastStatisticsReportTs = llNowMs;
    return true;
}

void CHVFPeopleFlowProcessor::clearStatisticsResult()
{
    m_stateStore.clear();
    m_indexManager.reset();
    for (auto &stTrackStatus : m_stTrackStatus)
    {
        stTrackStatus.reset();
    }
    m_normalAlarmStateMachine.reset();
    m_mediumAlarmStateMachine.reset();
    m_severeAlarmStateMachine.reset();
    m_llLastStatisticsReportTs = 0;
}

bool CHVFPeopleFlowProcessor::isEnabled() const
{
    return m_stAlgoCfg.bEnable;
}
} // namespace HVFDetectInternal
#endif
