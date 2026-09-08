/**
 * @FilePath     : people_density_types.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-26 15:00:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 15:00:00
 * @Description  : 共享人员密度业务类型定义
 */

#pragma once

#if CAP_AI_PEOPLE_DENSITY_PIPELINE

#include <cstdint>
#include <vector>

#include "alarm_define.h"
#include "detection_types.hpp"
#include "normalized_geometry.hpp"

namespace AiPipeline_NS
{
namespace PeopleDensity_NS
{
/* 首轮单规则，输出仍显式携带规则 ID 以预留扩展 */
constexpr uint64_t PEOPLE_DENSITY_DEFAULT_RULE_ID = 0;
/* 配置坐标基准宽度，现有配置按 1920×1080 下发 */
constexpr uint32_t PEOPLE_DENSITY_CONFIG_WIDTH = 1920;
/* 配置坐标基准高度 */
constexpr uint32_t PEOPLE_DENSITY_CONFIG_HEIGHT = 1080;
/* 上报间隔配置为 0 时的回退默认值（秒），与旧实现默认一致 */
constexpr uint32_t PEOPLE_DENSITY_DEFAULT_REPORT_INTERVAL_SEC = 60;

/* 内部人员密度配置，坐标已完成归一化转换 */
struct PeopleDensityConfig_S
{
    bool bEnabled = false;                                /* 是否启用 */
    uint32_t nSensitivity = 50;                           /* 灵敏度 [1,100] */
    uint32_t nReportIntervalSec = 60;                     /* 周期上报间隔，单位秒 */
    std::vector<Geometry_NS::Point_S> vecRegion;          /* 归一化检测区域，固定 4 点 */
    bool bRegionValid = false;                            /* 区域几何是否有效 */
    Alarm::PopulationAlarmRule_S stAlarmNormal;           /* 密度普通报警 */
    Alarm::PopulationAlarmRule_S stAlarmMedium;           /* 密度中度报警 */
    Alarm::PopulationAlarmRule_S stAlarmSevere;           /* 密度严重报警 */
};
} // namespace PeopleDensity_NS
} // namespace AiPipeline_NS

#endif // CAP_AI_PEOPLE_DENSITY_PIPELINE
