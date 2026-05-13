/**
 * @FilePath     : hvf_loitering_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-26 16:20:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-26 16:20:00
 * @Description  : HVF 徘徊侦测处理器实现
 */

#include "hvf_loitering_processor.hpp"

#include "internal/base/hvf_detect_common.hpp"

namespace HVFDetectInternal
{
CHVFLoiteringProcessor::CHVFLoiteringProcessor()
{
    reset_area_status_array(m_stLoiteringStatus);
#if CAP_EXHIBITION_OSD_PANEL
    reset_panel_enter_time_array(m_stPanelDurationStartTime);
#endif
}

void CHVFLoiteringProcessor::setEnabled(bool bEnable)
{
    m_stAlgoCfg.bEnable = bEnable;
}

void CHVFLoiteringProcessor::setAlgoParamCfg(const Alarm::LoiteringDetection_S &stAlgoCfg, int nWidth, int nHeight)
{
    dlog_debug("ai_app: 设置徘徊侦测参数");
    m_stAlgoCfg = stAlgoCfg;
    convert_resolution_and_enable(m_stAlgoCfg, nWidth, nHeight);
}

void CHVFLoiteringProcessor::process(SHVFProcessContext &stContext)
{
#if CAP_EXHIBITION_OSD_PANEL
    prepare_exhibition_panel_frame(stContext.pstPanelFrame, Event::Type_E::LOITERING_DETECT, stContext.nWidth, stContext.nHeight);
#endif
    /* 当前帧人体类别结果 */
    const ot_aidetect_object_of_one_class *pstObjectClass = find_object_class(stContext.stResult, OT_AIDETECT_CLASS_HUMAN);
    /* 当前帧仍存在的跟踪目标集合 */
    const std::set<int> stCurrentTrackIds = collect_track_ids(pstObjectClass);
    /* 上一轮仍保留在索引管理器中的跟踪目标集合 */
    const std::set<int> stPreviousTrackIds = m_indexManager.getCurrentTrackIds();

    for (int nTrackId : stPreviousTrackIds)
    {
        if (stCurrentTrackIds.find(nTrackId) != stCurrentTrackIds.end())
        {
            continue;
        }

        /* 丢失目标在状态数组中的内部索引 */
        const int nInternalIndex = m_indexManager.getIndexByTrackId(nTrackId);
        if (nInternalIndex < 0 || nInternalIndex >= m_indexManager.getMaxTargets())
        {
            continue;
        }

        for (size_t i = 0; i < LOITERING_DETECT_REGION_DEFAULT; ++i)
        {
            m_stLoiteringStatus[i][nInternalIndex] = AreaStatus_S();
#if CAP_EXHIBITION_OSD_PANEL
            m_stPanelDurationStartTime[i][nInternalIndex] = 0;
#endif
        }
    }

    m_indexManager.cleanupLostTargets(stCurrentTrackIds);
    process_region_detection(pstObjectClass,
                             m_stAlgoCfg.aRule,
                             m_stLoiteringStatus,
                             m_indexManager,
                             m_alarmStateMachine,
                             Event::Type_E::LOITERING_DETECT,
                             "徘徊侦测",
                             stContext
#if CAP_EXHIBITION_OSD_PANEL
                             ,
                             m_stPanelDurationStartTime
#endif
    );
}

bool CHVFLoiteringProcessor::isEnabled() const
{
    return m_stAlgoCfg.bEnable;
}
} // namespace HVFDetectInternal
