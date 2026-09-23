/**
 * @FilePath     : hvf_enter_exit_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-26 16:20:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-26 16:20:00
 * @Description  : HVF 进入/离开区域处理器实现
 */

#include "hvf_enter_exit_processor.hpp"

#include "internal/base/hvf_detect_common.hpp"

namespace HVFDetectInternal
{
namespace
{
template <size_t MaxRegions>
void reset_enter_exit_status_array(
    EnterExitTrackStatus_S (&stStatusArray)[MaxRegions][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM])
{
    for (size_t i = 0; i < MaxRegions; ++i)
    {
        for (size_t j = 0; j < SVP_AIDETECT_MAX_OUTPUT_RECT_NUM; ++j)
        {
            stStatusArray[i][j].reset();
        }
    }
}

template <size_t MaxRegions>
void cleanup_lost_track_states(
    const std::set<int> &stCurrentTrackIds,
    EnterExitTrackStatus_S (&stStatusArray)[MaxRegions][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM],
    CTargetIndexManager20 &indexManager)
{
    const std::set<int> stPreviousTrackIds = indexManager.getCurrentTrackIds();

    for (int nTrackId : stPreviousTrackIds)
    {
        if (stCurrentTrackIds.find(nTrackId) != stCurrentTrackIds.end())
        {
            continue;
        }

        const int nInternalIndex = indexManager.getIndexByTrackId(nTrackId);
        if (nInternalIndex >= 0 && nInternalIndex < indexManager.getMaxTargets())
        {
            for (size_t i = 0; i < MaxRegions; ++i)
            {
                stStatusArray[i][nInternalIndex].reset();
            }
        }

        indexManager.releaseIndex(nTrackId);
    }
}


} // namespace

CHVFEnterExitProcessor::CHVFEnterExitProcessor()
{
    reset_enter_exit_status_array(m_stEntranceStatus);
    reset_enter_exit_status_array(m_stExitStatus);
}

void CHVFEnterExitProcessor::resetEntranceRuntimeState()
{
    reset_enter_exit_status_array(m_stEntranceStatus);
    m_enterIndexManager.reset();
}

void CHVFEnterExitProcessor::resetExitRuntimeState()
{
    reset_enter_exit_status_array(m_stExitStatus);
    m_exitIndexManager.reset();
}

void CHVFEnterExitProcessor::setEntranceEnabled(bool bEnable)
{
    if (m_stEntranceCfg.bEnable != bEnable)
    {
        resetEntranceRuntimeState();
    }

    m_stEntranceCfg.bEnable = bEnable;
}

void CHVFEnterExitProcessor::setExitEnabled(bool bEnable)
{
    if (m_stExitCfg.bEnable != bEnable)
    {
        resetExitRuntimeState();
    }

    m_stExitCfg.bEnable = bEnable;
}

void CHVFEnterExitProcessor::setEntranceAlgoParamCfg(const Alarm::EntranceDetection_S &stAlgoCfg, int nWidth, int nHeight)
{
    dlog_debug("ai_app: 设置进入区域侦测参数");
    m_stEntranceCfg = stAlgoCfg;
    convert_resolution_and_enable(m_stEntranceCfg, nWidth, nHeight);
    resetEntranceRuntimeState();
}

void CHVFEnterExitProcessor::setExitAlgoParamCfg(const Alarm::ExitingDetection_S &stAlgoCfg, int nWidth, int nHeight)
{
    dlog_debug("ai_app: 设置离开区域侦测参数");
    m_stExitCfg = stAlgoCfg;
    convert_resolution_and_enable(m_stExitCfg, nWidth, nHeight);
    resetExitRuntimeState();
}

void CHVFEnterExitProcessor::processEntrance(SHVFProcessContext &stContext)
{
    if (stContext.stResult.class_num == 0)
    {
        return;
    }

    /* 当前帧人体类别结果 */
    const ot_aidetect_object_of_one_class *pstObjectClass = find_object_class(stContext.stResult, OT_AIDETECT_CLASS_HUMAN);
    cleanup_lost_track_states(collect_track_ids(pstObjectClass), m_stEntranceStatus, m_enterIndexManager);
    process_region_enter_exit_detection(pstObjectClass,
                                        m_stEntranceCfg.aRule,
                                        m_stEntranceStatus,
                                        m_enterIndexManager,
                                        m_enterAlarmStateMachine,
                                        Event::Type_E::ENTER_REGION,
                                        "进入区域",
                                        stContext);
}

void CHVFEnterExitProcessor::processExit(SHVFProcessContext &stContext)
{
    if (stContext.stResult.class_num == 0)
    {
        return;
    }

    /* 当前帧人体类别结果 */
    const ot_aidetect_object_of_one_class *pstObjectClass = find_object_class(stContext.stResult, OT_AIDETECT_CLASS_HUMAN);
    cleanup_lost_track_states(collect_track_ids(pstObjectClass), m_stExitStatus, m_exitIndexManager);
    process_region_enter_exit_detection(pstObjectClass,
                                        m_stExitCfg.aRule,
                                        m_stExitStatus,
                                        m_exitIndexManager,
                                        m_exitAlarmStateMachine,
                                        Event::Type_E::LEAVE_REGION,
                                        "离开区域",
                                        stContext);
}

bool CHVFEnterExitProcessor::isEntranceEnabled() const
{
    return m_stEntranceCfg.bEnable;
}

bool CHVFEnterExitProcessor::isExitEnabled() const
{
    return m_stExitCfg.bEnable;
}
} // namespace HVFDetectInternal
