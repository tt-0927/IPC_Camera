/**
 * @FilePath     : face_config_adapter.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 人脸侦测配置适配器实现
 */

#include "face_config_adapter.hpp"

#include "IpcRet.h"
#include "dlog.h"

#if CAP_UNIFIED_EVENT_PIPELINE

namespace AiPipeline_NS
{
namespace Face_NS
{
namespace
{
/**
 * @brief   : 判断多边形是否自交
 * @param    {const std::vector<Geometry_NS::Point_S> &} vecPoly：多边形顶点
 * @return   {bool} true：自交
 * @note    : 检查不相邻边是否相交；少于 4 点跳过自交检查
 */
bool is_polygon_self_intersecting(const std::vector<Geometry_NS::Point_S> &vecPoly)
{
    const size_t nSize = vecPoly.size();
    if (nSize < 4U)
    {
        return false;
    }

    /* 检查所有不相邻的边对 */
    for (size_t i = 0; i < nSize; ++i)
    {
        const size_t iNext = (i + 1U) % nSize;
        for (size_t j = i + 2U; j < nSize; ++j)
        {
            /* 跳过首尾相邻的边 */
            if (i == 0U && j == nSize - 1U)
            {
                continue;
            }
            const size_t jNext = (j + 1U) % nSize;
            if (Geometry_NS::segments_intersect(vecPoly[i], vecPoly[iNext], vecPoly[j], vecPoly[jNext]))
            {
                return true;
            }
        }
    }
    return false;
}
} // namespace

int CFaceConfigAdapter::adapt(const RawFaceConfig_S &stRawConfig, FaceConfig_S &stOut) const
{
    stOut = FaceConfig_S();
    stOut.bEnabled = stRawConfig.bEnabled;
    stOut.bDynamicAnalysisEnable = stRawConfig.bDynamicAnalysisEnable;
    stOut.nSensitivity = stRawConfig.nSensitivity;

    /* 校验并归一化单区域多边形 */
    if (!adapt_polygon(stRawConfig, stOut))
    {
        /* 仅在事件已使能但区域无效时 warn；正常禁用（未配置人脸区域）静默，避免噪音 */
        if (stRawConfig.bEnabled)
        {
            dlog_warn("人脸事件已使能但区域无效，处理器保持禁用");
        }
        stOut.bEnabled = false;
        return ERR_PARAM;
    }

    stOut.bValid = true;
    return OK;
}

bool CFaceConfigAdapter::adapt_polygon(const RawFaceConfig_S &stRawConfig, FaceConfig_S &stOutConfig) const
{
    /* 多边形顶点数量校验，至少 3 点 */
    if (stRawConfig.vecPolygon.size() < 3U)
    {
        return false;
    }

    /* 1920×1080 基准归一化 */
    const FrameSize_S stConfigSize{ FACE_CONFIG_WIDTH, FACE_CONFIG_HEIGHT };
    std::vector<Geometry_NS::Point_S> vecNormalized;
    vecNormalized.reserve(stRawConfig.vecPolygon.size());
    for (const auto &stPoint : stRawConfig.vecPolygon)
    {
        vecNormalized.emplace_back(Geometry_NS::normalize_point(stPoint.dX, stPoint.dY, stConfigSize));
    }

    /* 自交检查 */
    if (is_polygon_self_intersecting(vecNormalized))
    {
        dlog_warn("人脸配置区域多边形自交");
        return false;
    }

    stOutConfig.vecPolygon = std::move(vecNormalized);
    return true;
}
} // namespace Face_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
