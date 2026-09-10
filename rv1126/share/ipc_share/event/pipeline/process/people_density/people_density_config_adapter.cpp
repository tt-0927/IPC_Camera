/**
 * @FilePath     : people_density_config_adapter.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-26 15:00:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 15:00:00
 * @Description  : 人员密度配置适配器实现
 */

#include "people_density_config_adapter.hpp"

#include "IpcRet.h"
#include "dlog.h"

#if CAP_AI_PEOPLE_DENSITY_PIPELINE

namespace AiPipeline_NS
{
namespace PeopleDensity_NS
{
namespace
{
/**
 * @brief   : 判断四边形是否自交
 * @param    {const std::vector<Geometry_NS::Point_S> &} vecQuad：四边形四点
 * @return   {bool} true：自交
 * @note    : 只检查不相邻对边是否相交，与人流统计适配器同一判定口径
 */
bool is_quadrilateral_self_intersecting(const std::vector<Geometry_NS::Point_S> &vecQuad)
{
    if (vecQuad.size() != 4U)
    {
        return true;
    }

    /* 对边对：(0-1, 2-3) 与 (1-2, 3-0) */
    if (Geometry_NS::segments_intersect(vecQuad[0], vecQuad[1], vecQuad[2], vecQuad[3]) ||
        Geometry_NS::segments_intersect(vecQuad[1], vecQuad[2], vecQuad[3], vecQuad[0]))
    {
        return true;
    }

    return false;
}
} // namespace

int CPeopleDensityConfigAdapter::adapt(const Alarm::PeopleDensityDetection_S &stConfig,
                                       PeopleDensityConfig_S &stOut) const
{
    stOut = PeopleDensityConfig_S();
    stOut.bEnabled = stConfig.bEnable;
    stOut.nSensitivity = stConfig.nSensitivity;
    /* 报告间隔为 0 时回退默认 60 秒，保持旧处理器行为 */
    stOut.nReportIntervalSec = (stConfig.nReportInterval > 0U) ? stConfig.nReportInterval
                                                               : PEOPLE_DENSITY_DEFAULT_REPORT_INTERVAL_SEC;
    stOut.stAlarmNormal = stConfig.stDensityAlarm.stNormal;
    stOut.stAlarmMedium = stConfig.stDensityAlarm.stMedium;
    stOut.stAlarmSevere = stConfig.stDensityAlarm.stSevere;

    if (stConfig.nSensitivity < 1U || stConfig.nSensitivity > 100U)
    {
        dlog_warn("人员密度配置非法：灵敏度[%u]超出 [1,100]，处理器保持禁用", stConfig.nSensitivity);
        stOut.bEnabled = false;
        return ERR_PARAM;
    }

    if (!adapt_region(stConfig.stDetectRegion, stOut.vecRegion))
    {
        dlog_warn("人员密度配置非法：检测区域无效，处理器保持禁用");
        stOut.bEnabled = false;
        return ERR_PARAM;
    }

    stOut.bRegionValid = true;
    return OK;
}

bool CPeopleDensityConfigAdapter::adapt_region(const Alarm::Region_S &stRegion,
                                               std::vector<Geometry_NS::Point_S> &vecOutRegion) const
{
    /* 人员密度区域固定为四边形 */
    if (stRegion.nPointNum != 4U || stRegion.aPoint.size() < 4U)
    {
        return false;
    }

    if (!stRegion.IsValid())
    {
        return false;
    }

    const FrameSize_S stConfigSize{ PEOPLE_DENSITY_CONFIG_WIDTH, PEOPLE_DENSITY_CONFIG_HEIGHT };
    std::vector<Geometry_NS::Point_S> vecQuad;
    vecQuad.reserve(4U);
    for (unsigned int i = 0; i < 4U; ++i)
    {
        vecQuad.emplace_back(Geometry_NS::normalize_point(stRegion.aPoint[i].fX, stRegion.aPoint[i].fY, stConfigSize));
    }

    if (is_quadrilateral_self_intersecting(vecQuad))
    {
        dlog_warn("人员密度检测区域自交，配置无效");
        return false;
    }

    vecOutRegion = std::move(vecQuad);
    return true;
}
} // namespace PeopleDensity_NS
} // namespace AiPipeline_NS

#endif // CAP_AI_PEOPLE_DENSITY_PIPELINE
