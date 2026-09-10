/**
 * @FilePath     : people_density_config_adapter.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-26 15:00:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 15:00:00
 * @Description  : 人员密度配置适配器（1920×1080 配置 -> 归一化内部配置）
 */

#pragma once

#if CAP_AI_PEOPLE_DENSITY_PIPELINE

#include <cstdint>

#include "people_density_types.hpp"

namespace AiPipeline_NS
{
namespace PeopleDensity_NS
{
/* 人员密度配置适配器，无状态，可安全复用 */
class CPeopleDensityConfigAdapter
{
public:
    /**
     * @brief   : 将外部人员密度配置适配为归一化内部配置
     * @param    {const Alarm::PeopleDensityDetection_S &} stConfig：外部配置（1920×1080 基准）
     * @param    {PeopleDensityConfig_S &} stOut：输出归一化配置
     * @return   {int} OK：成功 ERR_PARAM：灵敏度/区域非法（输出配置保持禁用）
     * @note    : 区域固定四边形，需非自交；上报间隔为 0 时回退默认 60 秒
     */
    int adapt(const Alarm::PeopleDensityDetection_S &stConfig, PeopleDensityConfig_S &stOut) const;

private:
    /**
     * @brief   : 适配检测区域为归一化四边形
     * @param    {const Alarm::Region_S &} stRegion：外部区域配置
     * @param    {std::vector<Geometry_NS::Point_S> &} vecOutRegion：输出归一化四点
     * @return   {bool} true：区域有效 false：点数/有效性/自交校验失败
     */
    bool adapt_region(const Alarm::Region_S &stRegion, std::vector<Geometry_NS::Point_S> &vecOutRegion) const;
};
} // namespace PeopleDensity_NS
} // namespace AiPipeline_NS

#endif // CAP_AI_PEOPLE_DENSITY_PIPELINE
