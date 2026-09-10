/**
 * @FilePath     : normalized_geometry.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : 归一化几何内核实现
 */

#include "normalized_geometry.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace AiPipeline_NS
{
namespace Geometry_NS
{
bool is_valid_frame_size(const FrameSize_S &stSize)
{
    return stSize.nWidth > 0U && stSize.nHeight > 0U;
}

bool is_valid_normalized_rect(const NormalizedRect_S &stRect)
{
    /* 归一化坐标必须是有限值且位于 [0,1] 内 */
    if (!std::isfinite(stRect.dLeft) || !std::isfinite(stRect.dTop) || !std::isfinite(stRect.dRight) || !std::isfinite(stRect.dBottom))
    {
        return false;
    }

    if (stRect.dLeft < 0.0 || stRect.dTop < 0.0 || stRect.dRight > 1.0 || stRect.dBottom > 1.0)
    {
        return false;
    }

    return stRect.dRight > stRect.dLeft && stRect.dBottom > stRect.dTop;
}

Point_S normalize_point(double dX, double dY, const FrameSize_S &stSize)
{
    if (!is_valid_frame_size(stSize) || !std::isfinite(dX) || !std::isfinite(dY))
    {
        return Point_S();
    }

    Point_S stPoint;
    stPoint.dX = dX / static_cast<double>(stSize.nWidth);
    stPoint.dY = dY / static_cast<double>(stSize.nHeight);
    return stPoint;
}

NormalizedRect_S normalize_rect(double dLeft, double dTop, double dRight, double dBottom, const FrameSize_S &stSize)
{
    NormalizedRect_S stRect;
    if (!is_valid_frame_size(stSize) || !std::isfinite(dLeft) || !std::isfinite(dTop) || !std::isfinite(dRight) || !std::isfinite(dBottom))
    {
        return stRect;
    }

    stRect.dLeft = dLeft / static_cast<double>(stSize.nWidth);
    stRect.dTop = dTop / static_cast<double>(stSize.nHeight);
    stRect.dRight = dRight / static_cast<double>(stSize.nWidth);
    stRect.dBottom = dBottom / static_cast<double>(stSize.nHeight);
    return stRect;
}

Common::RectInfo_S denormalize_rect(const NormalizedRect_S &stRect, const FrameSize_S &stSize)
{
    Common::RectInfo_S stPixels;
    if (!is_valid_normalized_rect(stRect) || !is_valid_frame_size(stSize))
    {
        return stPixels;
    }

    /* 归一化坐标乘以基准分辨率得到像素坐标，采用四舍五入避免系统性截断偏移 */
    stPixels.nX1 = static_cast<int>(std::llround(stRect.dLeft * static_cast<double>(stSize.nWidth)));
    stPixels.nY1 = static_cast<int>(std::llround(stRect.dTop * static_cast<double>(stSize.nHeight)));
    stPixels.nX2 = static_cast<int>(std::llround(stRect.dRight * static_cast<double>(stSize.nWidth)));
    stPixels.nY2 = static_cast<int>(std::llround(stRect.dBottom * static_cast<double>(stSize.nHeight)));
    return stPixels;
}

double cross_product(const Point_S &stA, const Point_S &stB, const Point_S &stC)
{
    return (stB.dX - stA.dX) * (stC.dY - stA.dY) - (stB.dY - stA.dY) * (stC.dX - stA.dX);
}

namespace
{
/**
 * @brief   : 判断两个线段包围盒是否相交
 * @return   {bool} true：相交
 */
bool is_bounding_box_intersecting(const Point_S &stA1, const Point_S &stA2, const Point_S &stB1, const Point_S &stB2)
{
    return std::max(stA1.dX, stA2.dX) >= std::min(stB1.dX, stB2.dX) && std::max(stB1.dX, stB2.dX) >= std::min(stA1.dX, stA2.dX) &&
           std::max(stA1.dY, stA2.dY) >= std::min(stB1.dY, stB2.dY) && std::max(stB1.dY, stB2.dY) >= std::min(stA1.dY, stA2.dY);
}

/**
 * @brief   : 叉积符号，epsilon 范围内视为 0
 * @param    {double} dValue：叉积值
 * @return   {int} -1/0/1
 */
int cross_sign(double dValue)
{
    if (std::fabs(dValue) <= GEOMETRY_EPSILON)
    {
        return 0;
    }
    return dValue > 0.0 ? 1 : -1;
}
} // namespace

bool segments_intersect(const Point_S &stP1, const Point_S &stP2, const Point_S &stP3, const Point_S &stP4)
{
    /* 包围盒快速排除 */
    if (!is_bounding_box_intersecting(stP1, stP2, stP3, stP4))
    {
        return false;
    }

    const int nD1 = cross_sign(cross_product(stP3, stP4, stP1));
    const int nD2 = cross_sign(cross_product(stP3, stP4, stP2));
    const int nD3 = cross_sign(cross_product(stP1, stP2, stP3));
    const int nD4 = cross_sign(cross_product(stP1, stP2, stP4));

    /* 一般相交：两侧分布 */
    if (((nD1 > 0 && nD2 < 0) || (nD1 < 0 && nD2 > 0)) && ((nD3 > 0 && nD4 < 0) || (nD3 < 0 && nD4 > 0)))
    {
        return true;
    }

    /* 共线特例：四点叉积均为 0 时用包围盒判定 */
    if (nD1 == 0 && nD2 == 0 && nD3 == 0 && nD4 == 0)
    {
        return true;
    }

    return false;
}

bool point_in_polygon(const Point_S &stPoint, const std::vector<Point_S> &vecPolygon)
{
    const size_t nPointCount = vecPolygon.size();
    if (nPointCount < 3U)
    {
        return false;
    }

    /* 射线法：从测试点向右作水平射线，统计与多边形边的交点数量 */
    int nCrossCount = 0;
    for (size_t i = 0; i < nPointCount; ++i)
    {
        const Point_S &stPoint1 = vecPolygon[i];
        const Point_S &stPoint2 = vecPolygon[(i + 1) % nPointCount];

        const double dDeltaY = stPoint2.dY - stPoint1.dY;
        /* 跳过水平边，避免交点重复计数 */
        if (std::fabs(dDeltaY) <= GEOMETRY_EPSILON)
        {
            continue;
        }

        /* 判断测试点是否位于边的 Y 范围内 */
        if (((stPoint1.dY > stPoint.dY) != (stPoint2.dY > stPoint.dY)))
        {
            /* 计算水平射线与边的交点 X 坐标 */
            const double dXIntersect = (stPoint2.dX - stPoint1.dX) * (stPoint.dY - stPoint1.dY) / dDeltaY + stPoint1.dX;
            if (stPoint.dX < dXIntersect)
            {
                ++nCrossCount;
            }
        }
    }

    return (nCrossCount % 2) == 1;
}

Alarm::CrossDirection_E detect_cross_direction(const Point_S &stLastPos,
                                               const Point_S &stCurrentPos,
                                               const Point_S &stLineStart,
                                               const Point_S &stLineEnd)
{
    Alarm::CrossDirection_E enDirection = Alarm::CrossDirection_E::CROSS_DIRECTION_INVALID;

    /* 轨迹段与规则线包围盒不相交时直接返回无效 */
    if (!is_bounding_box_intersecting(stLastPos, stCurrentPos, stLineStart, stLineEnd))
    {
        return enDirection;
    }

    /* 轨迹段与规则线未实际相交时返回无效 */
    if (!segments_intersect(stLastPos, stCurrentPos, stLineStart, stLineEnd))
    {
        return enDirection;
    }

    /* 以规则线起终点作为固定方向，判断轨迹从线的一侧移动到另一侧 */
    const double dCrossLast = cross_product(stLineStart, stLineEnd, stLastPos);
    const double dCrossCurrent = cross_product(stLineStart, stLineEnd, stCurrentPos);
    const int nSignLast = cross_sign(dCrossLast);
    const int nSignCurrent = cross_sign(dCrossCurrent);

    if (nSignLast < 0 && nSignCurrent > 0)
    {
        enDirection = Alarm::CrossDirection_E::A_TO_B;
    }
    else if (nSignLast > 0 && nSignCurrent < 0)
    {
        enDirection = Alarm::CrossDirection_E::B_TO_A;
    }
    else if (nSignLast == 0 || nSignCurrent == 0)
    {
        /* 起点或终点落在线上时无法稳定区分方向，返回双向供上层忽略 */
        enDirection = Alarm::CrossDirection_E::BOTH_WAYS;
    }

    return enDirection;
}
} // namespace Geometry_NS
} // namespace AiPipeline_NS
