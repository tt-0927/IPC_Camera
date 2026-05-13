/**
 * @FilePath     : hvf_intrusion_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-26 16:20:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-26 16:20:00
 * @Description  : HVF 区域入侵处理器实现
 */

#include "hvf_intrusion_processor.hpp"

#include "internal/base/hvf_detect_common.hpp"

namespace HVFDetectInternal
{
CHVFIntrusionProcessor::CHVFIntrusionProcessor()
{
    reset_area_status_array(m_stFieldStatus);
}

void CHVFIntrusionProcessor::setEnabled(bool bEnable)
{
    m_stAlgoCfg.bEnable = bEnable;
}

void CHVFIntrusionProcessor::setAlgoParamCfg(const Alarm::FieldDetection_S &stAlgoCfg, int nWidth, int nHeight)
{
    dlog_debug("ai_app: 设置区域入侵侦测参数");
    m_stAlgoCfg = stAlgoCfg;
    convert_resolution_and_enable(m_stAlgoCfg, nWidth, nHeight);
}

void CHVFIntrusionProcessor::process(SHVFProcessContext &stContext)
{
#if CAP_EXHIBITION_OSD_PANEL
    prepare_exhibition_panel_frame(stContext.pstPanelFrame, Event::Type_E::INTRUSION, stContext.nWidth, stContext.nHeight);
#endif
    if (stContext.stResult.class_num == 0)
    {
        return;
    }

    m_indexManager.cleanupLostTargets(collect_all_track_ids(stContext.stResult));
    for (size_t i = 0; i < stContext.stResult.class_num; ++i)
    {
        /* 当前遍历到的算法类别结果 */
        process_region_detection(&stContext.stResult.object_class[i],
                                 m_stAlgoCfg.aRule,
                                 m_stFieldStatus,
                                 m_indexManager,
                                 m_alarmStateMachine,
                                 Event::Type_E::INTRUSION,
                                 "区域入侵侦测",
                                 stContext
#if CAP_EXHIBITION_OSD_PANEL
                                 ,
                                 nullptr
#endif
        );
    }
}

bool CHVFIntrusionProcessor::isEnabled() const
{
    return m_stAlgoCfg.bEnable;
}
} // namespace HVFDetectInternal
