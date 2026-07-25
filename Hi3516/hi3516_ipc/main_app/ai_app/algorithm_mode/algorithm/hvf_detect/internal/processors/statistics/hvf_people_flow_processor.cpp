/**
 * @FilePath     : hvf_people_flow_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-22 18:44:48
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-01 15:08:32
 * @Description  : HVF 人流统计处理器实现
 */

#include "hvf_people_flow_processor.hpp"

#if CAP_AI_PEOPLE_STATISTICS
#include <algorithm>
#include <string>

#include "common_process.h"
#include "dlog.h"
#include "video_frame_jpeg_encoder.hpp"

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
    if (m_stAlgoCfg.nReportInterval > 0)
    {
        m_llStatisticsReportIntervalMs = static_cast<long long>(m_stAlgoCfg.nReportInterval) * 1000;
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
    /* 当前帧是否发生跨线，用于触发强制上报并携带图片 */
    bool bForceReport = false;
    /* 强制上报时缓存的帧指针，跨线时记录用于图片编码 */
    ot_video_frame_info *pReportFrameInfo = nullptr;
    /* 强制上报时涉及的目标框列表，仅填充发生跨线的目标 */
    std::vector<Common::RectInfo_S> vecReportTargets;
    m_stateStore.maybeTimedReset(llNowMs);

    /* 当前帧人体类别检测结果，人流统计只处理人体目标 */
    const ot_aidetect_object_of_one_class *pstHumanClass = find_object_class(stContext.stResult, OT_AIDETECT_CLASS_HUMAN);
    // dlog_debug("[people_flow] 帧处理开始 通道[%d] 人体目标数[%zu] 时间[%lld]",
    //            stContext.nChnId,
    //            (pstHumanClass != nullptr) ? pstHumanClass->object_num : 0,
    //            llNowMs);
    m_indexManager.cleanupLostTargets(collect_track_ids(pstHumanClass));

    if (pstHumanClass != nullptr)
    {
        for (size_t i = 0; i < pstHumanClass->object_num; ++i)
        {
            /* 当前遍历到的人体目标 */
            const ot_aidetect_object &stObject = pstHumanClass->objects[i];
            // dlog_debug("[people_flow] 处理目标[%zu] track_id[%u] status[%d] conf[%.3f] rect[%.1f,%.1f,%.1f,%.1f]",
            //            i,
            //            stObject.track_id,
            //            stObject.track_status,
            //            stObject.detect_confidence,
            //            stObject.detect_rect.x,
            //            stObject.detect_rect.y,
            //            stObject.detect_rect.width,
            //            stObject.detect_rect.height);

            /* 断开跟踪（DIE）状态的目标坐标退化为 [0,0,0,0]，不代表真实位置，
             * 不参与跨线检测，避免误判为穿越事件 */
            if (stObject.track_status == OT_AIDETECT_TRACK_STATUS_DIE)
            {
                // dlog_info("人流统计跳过 DIE 状态目标: track_id[%u]", stObject.track_id);
                continue;
            }

            if ((stObject.track_status == OT_AIDETECT_TRACK_STATUS_NEW ||
                 stObject.track_status == OT_AIDETECT_TRACK_STATUS_UPDATE) &&
                stObject.detect_confidence < (1.0f - m_stAlgoCfg.nSensitivity / 100.0f))
            {
                // dlog_debug("[people_flow] 目标 track_id[%u] 置信度[%.3f]低于阈值[%.3f]，跳过",
                //            stObject.track_id,
                //            stObject.detect_confidence,
                //            (1.0f - m_stAlgoCfg.nSensitivity / 100.0f));
                continue;
            }

            if (!is_in_region(m_stAlgoCfg.stDetectRegion, stObject))
            {
                // dlog_debug("[people_flow] 目标 track_id[%u] 不在检测区域内，跳过", stObject.track_id);
                continue;
            }

            /* 当前 track_id 对应的内部状态索引 */
            const int nInternalIndex = m_indexManager.getOrAllocateIndex(stObject.track_id);
            // dlog_debug("[people_flow] 目标 track_id[%u] 分配索引[%d]", stObject.track_id, nInternalIndex);
            if (nInternalIndex < 0 || nInternalIndex >= m_indexManager.getMaxTargets())
            {
                dlog_warn("无法为track_id %u 分配索引，跳过人流统计处理", stObject.track_id);
                continue;
            }

            add_result_to_vector(stObject, stContext.vstRectInfo);

            /* 当前人体目标底边中点，用于和规则线进行轨迹相交判断 */
            Common::PosF_S stCurrentPos;
            stCurrentPos.fX = stObject.detect_rect.x + stObject.detect_rect.width / 2.0f;
            stCurrentPos.fY = stObject.detect_rect.y + stObject.detect_rect.height;

            /* 当前目标跨线跟踪状态，按内部状态索引复用固定数组 */
            BoundaryTrackStatus_S &stTrackStatus = m_stTrackStatus[nInternalIndex];
            if (!stTrackStatus.bIsTracking)
            {
                stTrackStatus.bIsTracking = true;
                stTrackStatus.stCurrentPosition = stCurrentPos;
                stTrackStatus.stLastPosition = stCurrentPos;
                stTrackStatus.dLastUpdateTime = static_cast<double>(llNowMs);
                // dlog_debug("[people_flow] 目标 track_id[%u] 初始化跟踪状态 位置[%.1f,%.1f]",
                //            stObject.track_id,
                //            stCurrentPos.fX,
                //            stCurrentPos.fY);
                continue;
            }

            stTrackStatus.stLastPosition = stTrackStatus.stCurrentPosition;
            stTrackStatus.stCurrentPosition = stCurrentPos;
            stTrackStatus.dLastUpdateTime = static_cast<double>(llNowMs);

            // dlog_debug("[people_flow] 目标 track_id[%u] 轨迹更新 last[%.1f,%.1f] -> curr[%.1f,%.1f]",
            //            stObject.track_id,
            //            stTrackStatus.stLastPosition.fX, stTrackStatus.stLastPosition.fY,
            //            stCurrentPos.fX, stCurrentPos.fY);

            /* 当前运动轨迹与人流统计规则线的穿越方向 */
            const Alarm::CrossDirection_E enCrossResult = tripLineDetection(stTrackStatus.stLastPosition,
                                                                            stTrackStatus.stCurrentPosition,
                                                                            m_stAlgoCfg.stRuleLine.stStartPos,
                                                                            m_stAlgoCfg.stRuleLine.stEndPos);
            // dlog_debug("[people_flow] 目标 track_id[%u] 跨线检测结果[%d] 规则线[%.1f,%.1f]-[%.1f,%.1f] 配置进入方向[%d]",
            //            stObject.track_id,
            //            static_cast<int>(enCrossResult),
            //            m_stAlgoCfg.stRuleLine.stStartPos.fX,
            //            m_stAlgoCfg.stRuleLine.stStartPos.fY,
            //            m_stAlgoCfg.stRuleLine.stEndPos.fX,
            //            m_stAlgoCfg.stRuleLine.stEndPos.fY,
            //            static_cast<int>(m_stAlgoCfg.stRuleLine.enDirection));
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
                bForceReport = true;
                if (pReportFrameInfo == nullptr)
                {
                    pReportFrameInfo = stContext.pFrameInfo;
                }
                vecReportTargets.push_back(build_rect(stObject));
                dlog_info("人流统计进入: track_id[%u] direction[%d] detect_rect[%d,%d,%dx%d]",
                          stObject.track_id,
                          static_cast<int>(enCrossResult),
                          stObject.detect_rect.x,
                          stObject.detect_rect.y,
                          stObject.detect_rect.width,
                          stObject.detect_rect.height);
            }
            else if (enCrossResult == enLeaveDirection)
            {
                m_stateStore.onLeave(build_snapshot(stObject,
                                                    EventStatistics_NS::SnapshotType_E::LEAVE,
                                                    enCrossResult,
                                                    llNowMs));
                bStatisticsChanged = true;
                bForceReport = true;
                if (pReportFrameInfo == nullptr)
                {
                    pReportFrameInfo = stContext.pFrameInfo;
                }
                vecReportTargets.push_back(build_rect(stObject));
                dlog_info("人流统计离开: track_id[%u] direction[%d] detect_rect[%d,%d,%dx%d]",
                          stObject.track_id,
                          static_cast<int>(enCrossResult),
                          stObject.detect_rect.x,
                          stObject.detect_rect.y,
                          stObject.detect_rect.width,
                          stObject.detect_rect.height);
            }
        }
    }
    // else
    // {
    //     dlog_debug("[people_flow] 帧处理 无人体目标");
    // }

    // int nTimeoutCount = 0;
    for (int i = 0; i < m_indexManager.getMaxTargets(); ++i)
    {
        if (m_stTrackStatus[i].bIsTracking && (llNowMs - m_stTrackStatus[i].dLastUpdateTime) > 5000)
        {
            m_stTrackStatus[i].reset();
            // dlog_debug("[people_flow] 索引[%d] 跟踪状态超时清理 最后更新[%lld] 当前[%lld]",
            //            i,
            //            static_cast<long long>(m_stTrackStatus[i].dLastUpdateTime),
            //            llNowMs);
            // ++nTimeoutCount;
        }
    }
    // if (nTimeoutCount > 0)
    // {
    //     dlog_debug("[people_flow] 本次共清理[%d]个超时跟踪状态", nTimeoutCount);
    // }

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

    // dlog_debug("[people_flow] 帧处理结束 通道[%d] 统计变化[%d] 进入[%u] 离开[%u] 滞留[%u]",
    //            stContext.nChnId,
    //            bStatisticsChanged ? 1 : 0,
    //            m_stateStore.getEnterCount(),
    //            m_stateStore.getLeaveCount(),
    //            m_stateStore.getCurrentStayCount());

    if (m_pReporter && (bForceReport || shouldEmitStatisticsReport(llNowMs)))
    {
        /* 统计业务周期上报结构，和事件开始/结束联动分离，供 TVSDK 客户端持续获取统计数据 */
        EventStatistics_NS::Report_S stReport = m_stateStore.buildAndConsumeReport(stContext.nChnId, llNowMs, bStatisticsChanged);

        /* 填充全景图 */
        if (pReportFrameInfo != nullptr && buildPanoramaImage(pReportFrameInfo, stReport.stPanoramaImage.vecJpeg))
        {
            stReport.stPanoramaImage.nWidth = pReportFrameInfo->video_frame.width;
            stReport.stPanoramaImage.nHeight = pReportFrameInfo->video_frame.height;
            stReport.stPanoramaImage.strTag = "panorama";
        }

        /* 填充目标图（仅跨线强制上报时） */
        if (bForceReport && pReportFrameInfo != nullptr)
        {
            /* 单帧目标图上限，防止过多目标导致性能和内存问题 */
            constexpr size_t MAX_TARGET_IMAGES_PER_REPORT = 4;
            for (size_t i = 0; i < vecReportTargets.size() && i < MAX_TARGET_IMAGES_PER_REPORT; ++i)
            {
                std::vector<unsigned char> vecJpeg;
                if (buildTargetImage(vecReportTargets[i], pReportFrameInfo, vecJpeg))
                {
                    EventStatistics_NS::ImagePayload_S stImage;
                    stImage.vecJpeg = std::move(vecJpeg);
                    stImage.strTag = "target";
                    stReport.vecTargetImages.push_back(std::move(stImage));
                    // dlog_info("人流统计目标图[%zu] 编码成功 大小[%zu字节]", i, stReport.vecTargetImages.back().vecJpeg.size());
                }       
                else
                {
                    dlog_warn("人流统计目标图[%zu] 编码失败", i);
                }
            }
        }

        /* 周期上报前输出统计摘要，便于排查定时上报时的计数状态和目标缓存规模 */
        dlog_info("人流统计%s上报: 通道[%d] 序号[%u] 进入人数[%u] 离开人数[%u] 总人数[%u] "
                  "滞留人数[%u] 目标数[%zu] 全景图大小[%zu字节] 目标图数量[%zu] 上报间隔毫秒[%lld]",
                  bForceReport ? "强制" : "周期",
                  stReport.nChnId,
                  stReport.nReportSeq,
                  stReport.nEnterCount,
                  stReport.nLeaveCount,
                  stReport.nTotalCount,
                  m_stateStore.getCurrentStayCount(),
                  stReport.vecTargets.size(),
                  stReport.stPanoramaImage.vecJpeg.size(),
                  stReport.vecTargetImages.size(),
                  m_llStatisticsReportIntervalMs);
        m_pReporter->report(stReport);

        /* 强制上报后重置计时器，避免短时间内重复上报 */
        m_llLastStatisticsReportTs = llNowMs;
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

    /* 不在这里更新时间戳，由调用方统一处理，支持强制上报后的计时器重置 */
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

bool CHVFPeopleFlowProcessor::buildPanoramaImage(ot_video_frame_info *pFrameInfo,
                                                  std::vector<unsigned char> &vecJpeg)
{
    vecJpeg.clear();
    if (pFrameInfo == nullptr)
    {
        return false;
    }

    EventTvSdkImage_S stImage;
    if (AiAppCommon::encode_video_frame_to_jpeg_memory(pFrameInfo, stImage) != OK)
    {
        dlog_warn("人流统计全景图编码失败");
        return false;
    }

    vecJpeg = std::move(stImage.vecJpeg);
    return true;
}

bool CHVFPeopleFlowProcessor::buildTargetImage(const Common::RectInfo_S &stRectInfo,
                                                ot_video_frame_info *pFrameInfo,
                                                std::vector<unsigned char> &vecJpeg)
{
    vecJpeg.clear();
    if (pFrameInfo == nullptr)
    {
        return false;
    }

    /* 当前目标框宽高 */
    const int nOrigW = stRectInfo.nX2 - stRectInfo.nX1;
    const int nOrigH = stRectInfo.nY2 - stRectInfo.nY1;
    if (nOrigW <= 0 || nOrigH <= 0)
    {
        dlog_warn("人流统计目标图跳过，原始目标框无效 [%d,%d,%d,%d]",
                  stRectInfo.nX1, stRectInfo.nY1, stRectInfo.nX2, stRectInfo.nY2);
        return false;
    }

    /* 以较长边为基准扩大 1.5 倍，作为正方形边长 */
    constexpr float TARGET_SCALE_RATIO = 1.5f;
    const int nMaxSide = std::max(nOrigW, nOrigH);
    const int nSquareSize = static_cast<int>(nMaxSide * TARGET_SCALE_RATIO);

    /* 以原框中心点为基准计算正方形裁剪区域 */
    const int nCenterX = (stRectInfo.nX1 + stRectInfo.nX2) / 2;
    const int nCenterY = (stRectInfo.nY1 + stRectInfo.nY2) / 2;

    Common::RectInfo_S stCropRect;
    stCropRect.nX1 = nCenterX - nSquareSize / 2;
    stCropRect.nY1 = nCenterY - nSquareSize / 2;
    stCropRect.nX2 = stCropRect.nX1 + nSquareSize;
    stCropRect.nY2 = stCropRect.nY1 + nSquareSize;

    /* 边界检查，限制在图像有效范围内 */
    const int nFrameW = pFrameInfo->video_frame.width;
    const int nFrameH = pFrameInfo->video_frame.height;
    stCropRect.nX1 = std::max(0, stCropRect.nX1);
    stCropRect.nY1 = std::max(0, stCropRect.nY1);
    stCropRect.nX2 = std::min(nFrameW, stCropRect.nX2);
    stCropRect.nY2 = std::min(nFrameH, stCropRect.nY2);

    /* VGS 硬件对齐约束：宽度 16 字节对齐，高度 4 字节对齐 */
    /* 使用 ALIGN_UP 确保对齐后有效区域不缩小，避免小目标框归零 */
    stCropRect.nX1 = ALIGN_BACK(stCropRect.nX1, 16);
    stCropRect.nY1 = ALIGN_BACK(stCropRect.nY1, 4);
    stCropRect.nX2 = ALIGN_UP(stCropRect.nX2, 16);
    stCropRect.nY2 = ALIGN_UP(stCropRect.nY2, 4);

    /* 二次边界检查，防止对齐后超出图像范围 */
    stCropRect.nX2 = std::min(nFrameW, stCropRect.nX2);
    stCropRect.nY2 = std::min(nFrameH, stCropRect.nY2);

    if (stCropRect.nX2 <= stCropRect.nX1 || stCropRect.nY2 <= stCropRect.nY1)
    {
        dlog_warn("人流统计目标图裁剪框对齐后无效 [%d,%d,%d,%d]",
                  stCropRect.nX1, stCropRect.nY1, stCropRect.nX2, stCropRect.nY2);
        return false;
    }

    /* 裁剪后目标图宽高 */
    const unsigned int unDstWidth = stCropRect.nX2 - stCropRect.nX1;
    const unsigned int unDstHeight = stCropRect.nY2 - stCropRect.nY1;

    /* 创建裁剪目标帧 */
    ot_video_frame_info stDstFrameInfo;
    if (TD_SUCCESS != mppVgs_create_video_frame_info(
                          unDstWidth, unDstHeight, OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420, &stDstFrameInfo))
    {
        dlog_warn("人流统计目标图创建 VGS 帧失败 [%u x %u]", unDstWidth, unDstHeight);
        return false;
    }

    /* VGS 裁剪区域 */
    ot_rect stVgsRect;
    stVgsRect.x = stCropRect.nX1;
    stVgsRect.y = stCropRect.nY1;
    stVgsRect.width = unDstWidth;
    stVgsRect.height = unDstHeight;

    if (TD_SUCCESS != mppVgs_crop(pFrameInfo, &stDstFrameInfo, &stVgsRect))
    {
        dlog_warn("人流统计目标图 VGS 裁剪失败");
        mppVgs_destroy_video_frame_info(&stDstFrameInfo);
        return false;
    }

    /* 编码为 JPEG 内存数据 */
    EventTvSdkImage_S stImage;
    const int nEncodeRet = AiAppCommon::encode_video_frame_to_jpeg_memory(
        &stDstFrameInfo, stImage);
    mppVgs_destroy_video_frame_info(&stDstFrameInfo);

    if (nEncodeRet != OK)
    {
        dlog_warn("人流统计目标图 JPEG 编码失败");
        return false;
    }

    vecJpeg = std::move(stImage.vecJpeg);
    return true;
}
} // namespace HVFDetectInternal
#endif
