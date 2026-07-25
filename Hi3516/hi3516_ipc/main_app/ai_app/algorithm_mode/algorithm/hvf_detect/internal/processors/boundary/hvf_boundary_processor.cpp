/**
 * @FilePath     : hvf_boundary_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-26 16:20:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-28 10:39:18
 * @Description  : HVF 越界侦测处理器实现
 */

#include "hvf_boundary_processor.hpp"

#include "internal/base/hvf_detect_common.hpp"

namespace
{
/**
 * @brief   : 处理单个类别的越界侦测结果
 * @param    {const ot_aidetect_object_of_one_class *} pstObjectClass：当前类别算法结果
 * @param    {const std::vector<Alarm::BoundaryPlane_S> &} aRules：越界规则数组
 * @param    {BoundaryTrackStatus_S [BOUND_DETECT_REGION_DEFAULT][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM]}* stStatusArray：越界状态数组
 * @param    {CTargetIndexManager20 &} indexManager：目标索引管理器
 * @param    {CAlarmStateMachine &} alarmStateMachine：报警状态机
 * @param    {const HVFDetectInternal::SHVFProcessContext &} stCtx：HVF处理上下文
 * @return   {bool} true：当前类别触发报警 false：当前类别未触发报警
 */
bool process_boundary_detection(
    const ot_aidetect_object_of_one_class *pstObjectClass,
    const std::vector<Alarm::BoundaryPlane_S> &aRules,
    BoundaryTrackStatus_S stStatusArray[BOUND_DETECT_REGION_DEFAULT][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM],
    CTargetIndexManager20 &indexManager,
    CAlarmStateMachine &alarmStateMachine,
    const HVFDetectInternal::SHVFProcessContext &stCtx)
{
    /* 当前类别是否存在报警 */
    bool bIsAlarm = false;
    const ot_aidetect_object *pAlarmObject = nullptr;
    int nAlarmRuleId = -1;
    if (pstObjectClass && !aRules.empty())
    {
        /* 当前毫秒级时间戳 */
        const double dCurrentTime = get_time_ms();
        for (size_t i = 0; i < pstObjectClass->object_num; ++i)
        {
            /* 当前遍历到的算法目标 */
            const ot_aidetect_object &stObject = pstObjectClass->objects[i];
            /* 当前目标对应的内部状态索引 */
            const int nInternalIndex = indexManager.getOrAllocateIndex(stObject.track_id);
            if (nInternalIndex < 0 || nInternalIndex >= indexManager.getMaxTargets())
            {
                dlog_warn("无法为track_id %u 分配索引，跳过越界侦测处理", stObject.track_id);
                continue;
            }

            /* 当前目标中心点坐标 */
            Common::PosF_S stCurrentPos;
            stCurrentPos.fX = stObject.detect_rect.x + stObject.detect_rect.width / 2.0f;
            stCurrentPos.fY = stObject.detect_rect.y + stObject.detect_rect.height / 2.0f;

            for (size_t j = 0; j < aRules.size() && j < BOUND_DETECT_REGION_DEFAULT; ++j)
            {
                /* 当前遍历到的警戒线规则 */
                const Alarm::BoundaryPlane_S &stRule = aRules[j];
                if (!HVFDetectInternal::is_target_match(stRule.aDetectionTarget, pstObjectClass->class_type))
                {
                    continue;
                }

                if ((stObject.track_status == OT_AIDETECT_TRACK_STATUS_NEW ||
                     stObject.track_status == OT_AIDETECT_TRACK_STATUS_UPDATE) &&
                    stObject.detect_confidence < (1.0f - stRule.nSensitivity / 100.0f))
                {
                    continue;
                }

                add_result_to_vector(stObject, stCtx.vstRectInfo);

                BoundaryTrackStatus_S &stTrackStatus = stStatusArray[j][nInternalIndex];
                if (!stTrackStatus.bIsTracking)
                {
                    stTrackStatus.bIsTracking = true;
                    stTrackStatus.stCurrentPosition = stCurrentPos;
                    stTrackStatus.stLastPosition = stCurrentPos;
                    stTrackStatus.dLastUpdateTime = dCurrentTime;
                    stTrackStatus.bAlarmed = false;
                    if (!access("testPrint", F_OK))
                    {
                        dlog_debug("开始跟踪目标 ID: %u (内部索引: %d) 在警戒线 %zu", stObject.track_id, nInternalIndex, j + 1);
                    }
                    continue;
                }

                stTrackStatus.stLastPosition = stTrackStatus.stCurrentPosition;
                stTrackStatus.stCurrentPosition = stCurrentPos;
                stTrackStatus.dLastUpdateTime = dCurrentTime;

                /* 当前运动轨迹与警戒线的穿越方向 */
                const Alarm::CrossDirection_E enCrossResult = tripLineDetection(stTrackStatus.stLastPosition,
                                                                                stTrackStatus.stCurrentPosition,
                                                                                stRule.stStartPos,
                                                                                stRule.stEndPos);
                if (enCrossResult == Alarm::CrossDirection_E::CROSS_DIRECTION_INVALID)
                {
                    continue;
                }

                /* 当前规则方向下是否应该触发报警 */
                bool bShouldAlarm = false;
                switch (stRule.enCrossDirection)
                {
                case Alarm::CrossDirection_E::BOTH_WAYS:
                    bShouldAlarm = true;
                    break;
                case Alarm::CrossDirection_E::A_TO_B:
                    bShouldAlarm = (enCrossResult == Alarm::CrossDirection_E::A_TO_B);
                    break;
                case Alarm::CrossDirection_E::B_TO_A:
                    bShouldAlarm = (enCrossResult == Alarm::CrossDirection_E::B_TO_A);
                    break;
                case Alarm::CrossDirection_E::CROSS_DIRECTION_INVALID:
                default:
                    break;
                }

                if (!bShouldAlarm)
                {
                    continue;
                }

                stTrackStatus.bAlarmed = true;
                bIsAlarm = true;
                if (pAlarmObject == nullptr)
                {
                    pAlarmObject = &stObject;
                    nAlarmRuleId = static_cast<int>(j);
                }
                dlog_info("目标 ID: %u (内部索引: %d) 越过警戒线 %zu，方向: %d，触发报警",
                          stObject.track_id,
                          nInternalIndex,
                          j + 1,
                          static_cast<int>(enCrossResult));
            }
        }

        for (size_t j = 0; j < aRules.size() && j < BOUND_DETECT_REGION_DEFAULT; ++j)
        {
            for (int k = 0; k < indexManager.getMaxTargets(); ++k)
            {
                BoundaryTrackStatus_S &stTrackStatus = stStatusArray[j][k];
                if (!stTrackStatus.bIsTracking || (dCurrentTime - stTrackStatus.dLastUpdateTime) <= 5000)
                {
                    continue;
                }

                if (!access("testPrint", F_OK))
                {
                    dlog_debug("目标(内部索引: %d)在警戒线 %zu 超时，重置跟踪状态", k, j + 1);
                }
                stTrackStatus.reset();
            }
        }
    }

    EventTriggerContext_S stEventContext;
    stEventContext.enEventType = Event::Type_E::LINE_CROSSING;
    stEventContext.nChnId = stCtx.nChnId;
    stEventContext.llTimestamp = stCtx.llTimestamp;
#ifdef ENABLE_TVSDK_SRC
    if (bIsAlarm && pAlarmObject != nullptr)
    {
        fill_hvf_tvsdk_event_context(stEventContext, stCtx, pstObjectClass, *pAlarmObject, nAlarmRuleId);
    }
#endif
    alarmStateMachine.handleAlarmState(bIsAlarm, stEventContext);
    return bIsAlarm;
}
} // namespace

namespace HVFDetectInternal
{
CHVFBoundaryProcessor::CHVFBoundaryProcessor()
{
    reset_boundary_status_array(m_stBoundaryStatus);
}

void CHVFBoundaryProcessor::setEnabled(bool bEnable)
{
    m_stAlgoCfg.bEnable = bEnable;
}

void CHVFBoundaryProcessor::setAlgoParamCfg(const Alarm::BoundaryDetection_S &stAlgoCfg, int nWidth, int nHeight)
{
    dlog_debug("ai_app: 设置越界侦测参数");
    m_stAlgoCfg = stAlgoCfg;
    convert_boundary_and_enable(m_stAlgoCfg, nWidth, nHeight);
}

void CHVFBoundaryProcessor::process(SHVFProcessContext &stContext)
{
    if (stContext.stResult.class_num == 0)
    {
        return;
    }

    m_indexManager.cleanupLostTargets(collect_all_track_ids(stContext.stResult));
    for (size_t i = 0; i < stContext.stResult.class_num; ++i)
    {
        process_boundary_detection(&stContext.stResult.object_class[i],
                                   m_stAlgoCfg.aRule,
                                   m_stBoundaryStatus,
                                   m_indexManager,
                                   m_alarmStateMachine,
                                   stContext);
    }
}

bool CHVFBoundaryProcessor::isEnabled() const
{
    return m_stAlgoCfg.bEnable;
}
} // namespace HVFDetectInternal
