/**
 * @FilePath     : normalized_geometry.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : 归一化几何内核，平台无关的坐标校验与线段/多边形判断
 */

#pragma once

#include <cstdint>
#include <vector>

#include "alarm_define.h"
#include "detection_types.hpp"

namespace AiPipeline_NS
{
namespace Geometry_NS
{
/* 统一浮点比较精度 */
constexpr double GEOMETRY_EPSILON = 1e-9;

/* 平面点，使用 double 避免旧实现叉积截断精度丢失 */
struct Point_S
{
    double dX = 0.0; /* X 坐标 */
    double dY = 0.0; /* Y 坐标 */

    /**
     * @brief   : 判断两个点是否相等
     * @param    {const Point_S &} stOther：另一个点
     * @return   {bool} true：相等
     */
    bool operator==(const Point_S &stOther) const
    {
        return dX == stOther.dX && dY == stOther.dY;
    }

    /**
     * @brief   : 判断两个点是否不等
     * @param    {const Point_S &} stOther：另一个点
     * @return   {bool} true：不等
     */
    bool operator!=(const Point_S &stOther) const
    {
        return !(*this == stOther);
    }
};

/**
 * @brief   : 校验帧分辨率元数据
 * @param    {const FrameSize_S &} stSize：帧分辨率
 * @return   {bool} true：合法 false：非法
 */
bool is_valid_frame_size(const FrameSize_S &stSize);

/**
 * @brief   : 校验归一化矩形
 * @param    {const NormalizedRect_S &} stRect：归一化矩形
 * @return   {bool} true：合法 false：非法
 * @note    : 归一化坐标要求有限值且位于 [0,1]，右/下边界大于左/上边界
 */
bool is_valid_normalized_rect(const NormalizedRect_S &stRect);

/**
 * @brief   : 像素坐标转换为归一化坐标
 * @param    {double} dX：像素 X 坐标
 * @param    {double} dY：像素 Y 坐标
 * @param    {const FrameSize_S &} stSize：坐标基准分辨率
 * @return   {Point_S} 归一化点，非法输入返回 [0,0]
 */
Point_S normalize_point(double dX, double dY, const FrameSize_S &stSize);

/**
 * @brief   : 像素矩形转换为归一化矩形
 * @param    {double} dLeft：左边界像素坐标
 * @param    {double} dTop：上边界像素坐标
 * @param    {double} dRight：右边界像素坐标
 * @param    {double} dBottom：下边界像素坐标
 * @param    {const FrameSize_S &} stSize：坐标基准分辨率
 * @return   {NormalizedRect_S} 归一化矩形，非法输入返回空矩形
 */
NormalizedRect_S normalize_rect(double dLeft, double dTop, double dRight, double dBottom, const FrameSize_S &stSize);

/**
 * @brief   : 归一化矩形转换为像素矩形
 * @param    {const NormalizedRect_S &} stRect：归一化矩形
 * @param    {const FrameSize_S &} stSize：坐标基准分辨率
 * @return   {Common::RectInfo_S} 像素矩形，非法输入返回空矩形
 */
Common::RectInfo_S denormalize_rect(const NormalizedRect_S &stRect, const FrameSize_S &stSize);

/**
 * @brief   : 计算向量 AB 与 AC 的叉积
 * @param    {const Point_S &} stA：公共起点
 * @param    {const Point_S &} stB：向量终点
 * @param    {const Point_S &} stC：向量终点
 * @return   {double} 叉积值，符号表示 C 相对 AB 的侧向
 */
double cross_product(const Point_S &stA, const Point_S &stB, const Point_S &stC);

/**
 * @brief   : 判断两条线段是否相交
 * @param    {const Point_S &} stP1：线段一起点
 * @param    {const Point_S &} stP2：线段一终点
 * @param    {const Point_S &} stP3：线段二起点
 * @param    {const Point_S &} stP4：线段二终点
 * @return   {bool} true：相交（含端点接触） false：不相交
 */
bool segments_intersect(const Point_S &stP1, const Point_S &stP2, const Point_S &stP3, const Point_S &stP4);

/**
 * @brief   : 判断点是否在多边形内（射线法）
 * @param    {const Point_S &} stPoint：待判断点
 * @param    {const std::vector<Point_S> &} vecPolygon：多边形顶点，至少 3 点
 * @return   {bool} true：在内部 false：在外部
 */
bool point_in_polygon(const Point_S &stPoint, const std::vector<Point_S> &vecPolygon);

/**
 * @brief   : 判断轨迹段相对规则线的穿越方向
 * @param    {const Point_S &} stLastPos：轨迹上一位置
 * @param    {const Point_S &} stCurrentPos：轨迹当前位置
 * @param    {const Point_S &} stLineStart：规则线起点
 * @param    {const Point_S &} stLineEnd：规则线终点
 * @return   {Alarm::CrossDirection_E} 穿越方向
 * @note    : 语义与现有 tripLineDetection 保持一致：起点叉积为负、终点为正为 A_TO_B，
 *            相反为 B_TO_A；任一点落在线上返回 BOTH_WAYS；不相交返回 INVALID
 */
Alarm::CrossDirection_E detect_cross_direction(const Point_S &stLastPos,
                                               const Point_S &stCurrentPos,
                                               const Point_S &stLineStart,
                                               const Point_S &stLineEnd);
} // namespace Geometry_NS
} // namespace AiPipeline_NS
