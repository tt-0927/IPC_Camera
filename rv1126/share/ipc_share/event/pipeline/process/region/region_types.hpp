/**
 * @FilePath     : region_types.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 区域事件（入侵/进入/离开）平台无关类型定义
 */

#pragma once

#if CAP_UNIFIED_EVENT_PIPELINE

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

#include "detection_types.hpp"
#include "event_define.h"
#include "normalized_geometry.hpp"

namespace AiPipeline_NS
{
namespace Region_NS
{
/* 区域事件规则最大数量，与旧 FIELD_DETECT_REGION_DEFAULT 一致 */
constexpr uint32_t REGION_MAX_RULES = 4;
/* 区域状态容器容量上限 */
constexpr uint32_t REGION_TRACK_CAPACITY = 128;
/* 轨迹缺失后保留时长，与旧实现 5 秒超时一致 */
constexpr int64_t REGION_TRACK_TTL_MS = 5000;
/* 配置坐标基准宽度，现有配置按 1920×1080 下发 */
constexpr uint32_t REGION_CONFIG_WIDTH = 1920;
/* 配置坐标基准高度 */
constexpr uint32_t REGION_CONFIG_HEIGHT = 1080;

/**
 * @brief   : 检测目标类别位掩码常量
 */
constexpr uint32_t DETECTION_TARGET_HUMAN = 1U << 0;   /* 人体 */
constexpr uint32_t DETECTION_TARGET_VEHICLE = 1U << 1; /* 车辆 */
constexpr uint32_t DETECTION_TARGET_OTHER = 1U << 2;   /* 其他 */

/**
 * @brief   : 原始区域规则入参（1920×1080 像素坐标，平台无关）
 * @note    : 业务仓 ConfigAdapter 负责把 Alarm::FieldDetection_S 等转为该结构，
 *            再交 region_config_adapter 归一化到 [0,1]
 */
struct RawRegionRule_S
{
    /* 事件类型：INTRUSION / ENTER_REGION / LEAVE_REGION */
    Event::Type_E enEventType = Event::Type_E::UNKNOWN;
    /* 多边形顶点（像素坐标，1920×1080 基准），至少 3 点 */
    std::vector<Geometry_NS::Point_S> vecPolygon;
    /* 驻留时间阈值（秒），仅 INTRUSION 使用 */
    uint32_t nTimeThresholdSec = 0;
    /* 灵敏度 [1,100] */
    uint32_t nSensitivity = 50;
    /* 检测目标类别位掩码 */
    uint32_t nDetectionTargetMask = 0U;
    /* 规则 ID */
    int nRuleId = 0;
};

/**
 * @brief   : 原始区域配置入参（1920×1080 像素坐标，平台无关）
 */
struct RawRegionConfig_S
{
    /* 是否启用 */
    bool bEnabled = false;
    /* 规则列表 */
    std::vector<RawRegionRule_S> vecRules;
};

/**
 * @brief   : 单条区域规则配置（归一化坐标）
 * @note    : 平台无关，不含厂商类型；由 region_config_adapter 从 RawRegionConfig_S 归一化
 */
struct RegionRuleConfig_S
{
    /* 事件类型：INTRUSION / ENTER_REGION / LEAVE_REGION */
    Event::Type_E enEventType = Event::Type_E::UNKNOWN;
    /* 归一化多边形顶点，至少 3 点 */
    std::vector<Geometry_NS::Point_S> vecPolygon;
    /* 驻留时间阈值（秒），仅 INTRUSION 使用；ENTER/LEAVE 为 0 */
    uint32_t nTimeThresholdSec = 0;
    /* 灵敏度 [1,100] */
    uint32_t nSensitivity = 50;
    /* 检测目标类别位掩码：bit0=HUMAN, bit1=VEHICLE, bit2=OTHER */
    uint32_t nDetectionTargetMask = 0U;
    /* 规则 ID，在同一事件类型内唯一 */
    int nRuleId = 0;
    /* 几何是否有效 */
    bool bValid = false;
};

/**
 * @brief   : 区域处理器内部配置，坐标已归一化
 */
struct RegionConfig_S
{
    /* 是否启用 */
    bool bEnabled = false;
    /* 规则列表，最多 REGION_MAX_RULES 条 */
    std::vector<RegionRuleConfig_S> vecRules;
};

/**
 * @brief   : 区域状态存储 Key：channel + eventType + ruleId + trackId
 * @note    : 多规则驻留计时互不污染；三事件共用一份存储，事件类型维度隔离状态空间
 */
struct RegionTrackKey_S
{
    int nChannelId = 0;
    Event::Type_E enEventType = Event::Type_E::UNKNOWN;
    int nRuleId = 0;
    uint64_t ullTrackId = 0;

    /**
     * @brief   : 判断两个 Key 是否相等
     * @param    {const RegionTrackKey_S &} stOther：另一个 Key
     * @return   {bool} true：相等
     */
    bool operator==(const RegionTrackKey_S &stOther) const
    {
        return nChannelId == stOther.nChannelId && enEventType == stOther.enEventType &&
               nRuleId == stOther.nRuleId && ullTrackId == stOther.ullTrackId;
    }
};

/**
 * @brief   : 区域状态 Key 哈希器
 */
struct RegionTrackKeyHash_S
{
    /**
     * @brief   : 计算区域状态 Key 哈希
     * @param    {const RegionTrackKey_S &} stKey：区域状态 Key
     * @return   {size_t} 哈希值
     */
    size_t operator()(const RegionTrackKey_S &stKey) const
    {
        size_t nHash = std::hash<int>()(stKey.nChannelId);
        nHash ^= std::hash<int>()(static_cast<int>(stKey.enEventType)) + 0x9e3779b9U + (nHash << 6U) + (nHash >> 2U);
        nHash ^= std::hash<int>()(stKey.nRuleId) + 0x9e3779b9U + (nHash << 6U) + (nHash >> 2U);
        nHash ^= std::hash<uint64_t>()(stKey.ullTrackId) + 0x9e3779b9U + (nHash << 6U) + (nHash >> 2U);
        return nHash;
    }
};

/**
 * @brief   : 单目标在单规则下的区域驻留状态
 * @note    : 语义参考旧 AreaStatus_S，时钟拆分：
 *            llEnterMonotonicMs 用于驻留计时（monotonic），
 *            事件触发时间戳由 Processor 从批次元数据取 wall clock
 */
struct RegionTrackState_S
{
    /* 目标当前是否在区域内 */
    bool bIsInRegion = false;
    /* 进入区域的 monotonic 时间戳（毫秒），用于驻留计时 */
    int64_t llEnterMonotonicMs = 0;
    /* 是否已报警，避免重复触发 */
    bool bAlarmed = false;
    /* 最后更新的 monotonic 时间戳，用于陈旧清理 */
    int64_t llLastUpdateMs = 0;
};
} // namespace Region_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
