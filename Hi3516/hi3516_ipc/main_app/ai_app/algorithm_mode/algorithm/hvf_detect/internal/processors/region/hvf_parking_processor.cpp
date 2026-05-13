/**
 * @FilePath     : hvf_parking_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-26 16:20:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-26 16:20:00
 * @Description  : HVF 停车侦测处理器实现
 */

#include "hvf_parking_processor.hpp"

#include "internal/base/hvf_detect_common.hpp"

namespace HVFDetectInternal
{
CHVFParkingProcessor::CHVFParkingProcessor()
{
    reset_area_status_array(m_stParkingStatus);
}

void CHVFParkingProcessor::setEnabled(bool bEnable)
{
    m_stAlgoCfg.bEnable = bEnable;
}

void CHVFParkingProcessor::setAlgoParamCfg(const Alarm::ParkingDetection_S &stAlgoCfg, int nWidth, int nHeight)
{
    dlog_debug("ai_app: 设置停车侦测参数");
    m_stAlgoCfg = stAlgoCfg;
    convert_resolution_and_enable(m_stAlgoCfg, nWidth, nHeight);
}

void CHVFParkingProcessor::process(SHVFProcessContext &stContext)
{
    if (stContext.stResult.class_num == 0)
    {
        return;
    }

    /* 当前帧车辆类别结果 */
    const ot_aidetect_object_of_one_class *pstObjectClass = find_object_class(stContext.stResult, OT_AIDETECT_CLASS_VEHICLE);
    m_indexManager.cleanupLostTargets(collect_track_ids(pstObjectClass));
    process_region_detection(pstObjectClass,
                             m_stAlgoCfg.aRule,
                             m_stParkingStatus,
                             m_indexManager,
                             m_alarmStateMachine,
                             Event::Type_E::PARKING_DETECT,
                             "停车侦测",
                             stContext
#if CAP_EXHIBITION_OSD_PANEL
                             ,
                             nullptr
#endif
    );
}

bool CHVFParkingProcessor::isEnabled() const
{
    return m_stAlgoCfg.bEnable;
}
} // namespace HVFDetectInternal
