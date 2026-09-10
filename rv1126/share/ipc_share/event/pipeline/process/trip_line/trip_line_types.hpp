/**
 * @FilePath     : trip_line_types.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 越界事件（TripLine）平台无关类型定义
 */

#pragma once

#if CAP_UNIFIED_EVENT_PIPELINE

#include <cstdint>
#include <vector>

#include "alarm_define.h"
#include "detection_types.hpp"
#include "event_define.h"
#include "normalized_geometry.hpp"

namespace AiPipeline_NS
{
namespace TripLine_NS
{
/* 越界规则最大数量，与旧 BOUND_DETECT_REGION_DEFAULT 一致 */
constexpr uint32_t TRIP_LINE_MAX_RULES = 4;
/* 越界轨迹容器容量上限 */
constexpr uint32_t TRIP_LINE_TRACK_CAPACITY = 128;
/* 轨迹超时清理阈值（毫秒），与旧实现 5000ms 一致 */
constexpr int64_t TRIP_LINE_TRACK_TTL_MS = 5000;
/* 配置坐标基准宽度，现有配置按 1920×1080 下发 */
constexpr uint32_t TRIP_LINE_CONFIG_WIDTH = 1920;
/* 配置坐标基准高度 */
constexpr uint32_t TRIP_LINE_CONFIG_HEIGHT = 1080;

/**
 * @brief   : 检测目标类别位掩码常量
 */
constexpr uint32_t DETECTION_TARGET_HUMAN = 1U << 0;   /* 人体 */
constexpr uint32_t DETECTION_TARGET_VEHICLE = 1U << 1; /* 车辆 */
constexpr uint32_t DETECTION_TARGET_OTHER = 1U << 2;   /* 其他 */

/**
 * @brief   : 单条越界规则配置（归一化坐标）
 * @note    : 平台无关，不含厂商类型；由 trip_line_config_adapter 从 RawTripLineConfig_S 归一化
 */
struct TripLineRuleConfig_S
{
    /* 归一化规则线起点 */
    Geometry_NS::Point_S stLineStart;
    /* 归一化规则线终点 */
    Geometry_NS::Point_S stLineEnd;
    /* 穿越方向：BOTH_WAYS/A_TO_B/B_TO_A */
    Alarm::CrossDirection_E enCrossDirection = Alarm::CrossDirection_E::CROSS_DIRECTION_INVALID;
    /* 灵敏度 [1,100] */
    uint32_t nSensitivity = 50;
    /* 检测目标类别位掩码：bit0=HUMAN, bit1=VEHICLE, bit2=OTHER */
    uint32_t nDetectionTargetMask = 0U;
    /* 规则 ID，在同一 Processor 内唯一 */
    int nRuleId = 0;
    /* 几何是否有效（起点终点不重合） */
    bool bValid = false;
};

/**
 * @brief   : 原始越界规则入参（1920×1080 像素坐标，平台无关）
 * @note    : 业务仓 ConfigAdapter 负责把 Alarm::BoundaryDetection_S 等转为该结构，
 *            再交 trip_line_config_adapter 归一化到 [0,1]
 */
struct RawTripLineRule_S
{
    /* 规则线起点（像素坐标，1920×1080 基准） */
    Geometry_NS::Point_S stLineStart;
    /* 规则线终点（像素坐标，1920×1080 基准） */
    Geometry_NS::Point_S stLineEnd;
    /* 穿越方向 */
    Alarm::CrossDirection_E enCrossDirection = Alarm::CrossDirection_E::CROSS_DIRECTION_INVALID;
    /* 灵敏度 [1,100] */
    uint32_t nSensitivity = 50;
    /* 检测目标类别位掩码 */
    uint32_t nDetectionTargetMask = 0U;
    /* 规则 ID */
    int nRuleId = 0;
};

/**
 * @brief   : 原始越界配置入参（1920×1080 像素坐标，平台无关）
 */
struct RawTripLineConfig_S
{
    /* 是否启用 */
    bool bEnabled = false;
    /* 规则列表 */
    std::vector<RawTripLineRule_S> vecRules;
};

/**
 * @brief   : 越界处理器内部配置，坐标已归一化
 */
struct TripLineConfig_S
{
    /* 是否启用 */
    bool bEnabled = false;
    /* 规则列表，最多 TRIP_LINE_MAX_RULES 条 */
    std::vector<TripLineRuleConfig_S> vecRules;
};

/**
 * @brief   : 越界状态存储 Key：channel + ruleId + trackId
 * @note    : 越界事件类型恒为 LINE_CROSSING，无需事件类型维度隔离；
 *            ruleId 维度隔离保证多规则线不互相污染
 */
struct TripLineTrackKey_S
{
    int nChannelId = 0;
    int nRuleId = 0;
    uint64_t ullTrackId = 0;

    /**
     * @brief   : 判断两个 Key 是否相等
     * @param    {const TripLineTrackKey_S &} stOther：另一个 Key
     * @return   {bool} true：相等
     */
    bool operator==(const TripLineTrackKey_S &stOther) const
    {
        return nChannelId == stOther.nChannelId && nRuleId == stOther.nRuleId && ullTrackId == stOther.ullTrackId;
    }
};

/**
 * @brief   : 越界状态 Key 哈希器
 */
struct TripLineTrackKeyHash_S
{
    /**
     * @brief   : 计算越界状态 Key 哈希
     * @param    {const TripLineTrackKey_S &} stKey：越界状态 Key
     * @return   {size_t} 哈希值
     */
    size_t operator()(const TripLineTrackKey_S &stKey) const
    {
        size_t nHash = std::hash<int>()(stKey.nChannelId);
        nHash ^= std::hash<int>()(stKey.nRuleId) + 0x9e3779b9U + (nHash << 6U) + (nHash >> 2U);
        nHash ^= std::hash<uint64_t>()(stKey.ullTrackId) + 0x9e3779b9U + (nHash << 6U) + (nHash >> 2U);
        return nHash;
    }
};

/**
 * @brief   : 单目标在单规则线下的越界轨迹状态
 * @note    : 语义参考旧 BoundaryTrackStatus_S，时钟拆分：
 *            llLastUpdateMs 用于超时清理（monotonic），
 *            事件触发时间戳由 Processor 从批次元数据取 wall clock
 */
struct TripLineTrackState_S
{
    /* 上一帧的归一化位置 */
    Geometry_NS::Point_S stLastPosition;
    /* 当前帧的归一化位置 */
    Geometry_NS::Point_S stCurrentPosition;
    /* 是否正在跟踪，首帧为 false 时初始化后翻 true，首帧不判跨线 */
    bool bIsTracking = false;
    /* 是否已报警，避免同目标同规则重复触发 */
    bool bAlarmed = false;
    /* 最后更新的 monotonic 时间戳，用于陈旧清理 */
    int64_t llLastUpdateMs = 0;
};
} // namespace TripLine_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
