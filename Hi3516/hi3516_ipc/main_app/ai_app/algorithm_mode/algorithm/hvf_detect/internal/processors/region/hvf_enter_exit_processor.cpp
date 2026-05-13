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
CHVFEnterExitProcessor::CHVFEnterExitProcessor()
{
    reset_area_status_array(m_stEntranceStatus);
    reset_area_status_array(m_stExitStatus);
}

void CHVFEnterExitProcessor::setEntranceEnabled(bool bEnable)
{
    m_stEntranceCfg.bEnable = bEnable;
}

void CHVFEnterExitProcessor::setExitEnabled(bool bEnable)
{
    m_stExitCfg.bEnable = bEnable;
}

void CHVFEnterExitProcessor::setEntranceAlgoParamCfg(const Alarm::EntranceDetection_S &stAlgoCfg, int nWidth, int nHeight)
{
    dlog_debug("ai_app: 设置进入区域侦测参数");
    m_stEntranceCfg = stAlgoCfg;
    convert_resolution_and_enable(m_stEntranceCfg, nWidth, nHeight);
}

void CHVFEnterExitProcessor::setExitAlgoParamCfg(const Alarm::ExitingDetection_S &stAlgoCfg, int nWidth, int nHeight)
{
    dlog_debug("ai_app: 设置离开区域侦测参数");
    m_stExitCfg = stAlgoCfg;
    convert_resolution_and_enable(m_stExitCfg, nWidth, nHeight);
}

void CHVFEnterExitProcessor::processEntrance(SHVFProcessContext &stContext)
{
    if (stContext.stResult.class_num == 0)
    {
        return;
    }

    /* 当前帧人体类别结果 */
    const ot_aidetect_object_of_one_class *pstObjectClass = find_object_class(stContext.stResult, OT_AIDETECT_CLASS_HUMAN);
    m_enterIndexManager.cleanupLostTargets(collect_track_ids(pstObjectClass));
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
    m_exitIndexManager.cleanupLostTargets(collect_track_ids(pstObjectClass));
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
