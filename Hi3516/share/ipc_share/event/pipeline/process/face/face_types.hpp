/**
 * @FilePath     : face_types.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 人脸侦测事件平台无关类型定义
 */

#pragma once

#if CAP_UNIFIED_EVENT_PIPELINE

#include <cstdint>
#include <vector>

#include "event_define.h"
#include "normalized_geometry.hpp"

namespace AiPipeline_NS
{
namespace Face_NS
{
/* 配置坐标基准宽度，现有配置按 1920×1080 下发 */
constexpr uint32_t FACE_CONFIG_WIDTH = 1920;
/* 配置坐标基准高度 */
constexpr uint32_t FACE_CONFIG_HEIGHT = 1080;

/**
 * @brief   : 原始人脸侦测配置入参（1920×1080 像素坐标，平台无关）
 * @note    : 业务仓 ConfigAdapter 负责把 Alarm::FaceDetection_S 转为该结构，
 *            再交 face_config_adapter 归一化到 [0,1]
 */
struct RawFaceConfig_S
{
    /* 是否启用 */
    bool bEnabled = false;
    /* 是否启用动态分析 OSD */
    bool bDynamicAnalysisEnable = false;
    /* 灵敏度 [1,100] */
    uint32_t nSensitivity = 50;
    /* 单区域多边形顶点（像素坐标，1920×1080 基准），至少 3 点 */
    std::vector<Geometry_NS::Point_S> vecPolygon;
};

/**
 * @brief   : 人脸侦测处理器内部配置，坐标已归一化
 * @note    : 平台无关，不含厂商类型；由 face_config_adapter 从 RawFaceConfig_S 归一化
 */
struct FaceConfig_S
{
    /* 是否启用 */
    bool bEnabled = false;
    /* 是否启用动态分析 OSD */
    bool bDynamicAnalysisEnable = false;
    /* 灵敏度 [1,100] */
    uint32_t nSensitivity = 50;
    /* 归一化单区域多边形顶点，至少 3 点 */
    std::vector<Geometry_NS::Point_S> vecPolygon;
    /* 几何是否有效 */
    bool bValid = false;
};
} // namespace Face_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
