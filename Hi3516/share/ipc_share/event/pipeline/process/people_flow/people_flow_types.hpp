/**
 * @FilePath     : people_flow_types.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : 共享人流统计业务类型定义
 */

#pragma once

#if CAP_AI_PEOPLE_STATISTICS

#include <cstdint>
#include <functional>
#include <unordered_map>

#include "alarm_define.h"
#include "detection_types.hpp"
#include "normalized_geometry.hpp"

namespace AiPipeline_NS
{
namespace PeopleFlow_NS
{
/* 首轮单规则，状态 Key 与输出仍显式携带规则 ID 以预留扩展 */
constexpr uint64_t PEOPLE_FLOW_DEFAULT_RULE_ID = 0;
/* Track 状态容器容量上限，满载时先清理超时项再限频降级 */
constexpr uint32_t PEOPLE_FLOW_TRACK_CAPACITY = 64;
/* Track 缺失后保留时长，与旧实现 5 秒超时一致 */
constexpr int64_t PEOPLE_FLOW_TRACK_TTL_MS = 5000;
/* 单报告目标图数量上限 */
constexpr uint32_t PEOPLE_FLOW_MAX_TARGET_IMAGES_PER_REPORT = 4;
/* 配置坐标基准宽度，现有配置按 1920×1080 下发 */
constexpr uint32_t PEOPLE_FLOW_CONFIG_WIDTH = 1920;
/* 配置坐标基准高度 */
constexpr uint32_t PEOPLE_FLOW_CONFIG_HEIGHT = 1080;

/* 归一化人流统计规则 */
struct PeopleFlowRuleConfig_S
{
    Geometry_NS::Point_S stLineStart;                                           /* 归一化规则线起点 */
    Geometry_NS::Point_S stLineEnd;                                             /* 归一化规则线终点 */
    Alarm::CrossDirection_E enEnterDirection = Alarm::CrossDirection_E::A_TO_B; /* 配置进入方向 */
    std::vector<Geometry_NS::Point_S> vecRegion;                                /* 归一化检测区域，固定 4 点 */
    bool bValid = false;                                                        /* 规则几何是否有效 */
};

/* 内部人流统计配置，坐标已完成归一化转换 */
struct PeopleFlowConfig_S
{
    bool bEnabled = false;            /* 是否启用 */
    uint32_t nSensitivity = 50;       /* 灵敏度 [1,100] */
    uint32_t nReportIntervalSec = 60; /* 周期上报间隔，单位秒 */
    Alarm::PeopleFlowStatisticsType_E enStatisticsType = Alarm::PeopleFlowStatisticsType_E::PEOPLE_FLOW_STAT_TOTAL; /* 统计类型 */
    PeopleFlowRuleConfig_S stRule;                                                                                  /* 归一化规则 */
    bool bTimedResetEnabled = false;                /* 是否启用定时清零 */
    Common::Time_S stResetTime;                     /* 定时清零时间点 */
    Alarm::PopulationAlarmRule_S stStayAlarmNormal; /* 滞留普通报警 */
    Alarm::PopulationAlarmRule_S stStayAlarmMedium; /* 滞留中度报警 */
    Alarm::PopulationAlarmRule_S stStayAlarmSevere; /* 滞留严重报警 */
};

/* 每帧人流统计摘要，供帧级诊断与外部消费 */
struct PeopleFlowFrameSummary_S
{
    int nChannelId = 0;               /* 通道号 */
    int nRuleId = 0;                  /* 规则 ID */
    uint64_t ullFrameId = 0;          /* 帧标识 */
    uint32_t nFrameEnterCount = 0;    /* 本帧进入人数 */
    uint32_t nFrameLeaveCount = 0;    /* 本帧离开人数 */
    uint32_t nTotalEnterCount = 0;    /* 累计进入人数 */
    uint32_t nTotalLeaveCount = 0;    /* 累计离开人数 */
    uint32_t nCurrentStayCount = 0;   /* 当前滞留人数 */
};

/* 轨迹状态真实 Key：Channel + Rule + Track ID */
struct TrackKey_S
{
    int nChannelId = 0;      /* 通道号 */
    int nRuleId = 0;         /* 规则 ID */
    uint64_t ullTrackId = 0; /* Track ID，0 仍是合法值 */

    /**
     * @brief   : 判断两个轨迹 Key 是否相等
     * @param    {const TrackKey_S &} stOther：另一个 Key
     * @return   {bool} true：相等
     */
    bool operator==(const TrackKey_S &stOther) const
    {
        return nChannelId == stOther.nChannelId && nRuleId == stOther.nRuleId && ullTrackId == stOther.ullTrackId;
    }
};

/* 轨迹 Key 哈希器 */
struct TrackKeyHash_S
{
    /**
     * @brief   : 计算轨迹 Key 哈希
     * @param    {const TrackKey_S &} stKey：轨迹 Key
     * @return   {size_t} 哈希值
     */
    size_t operator()(const TrackKey_S &stKey) const
    {
        size_t nHash = std::hash<int>()(stKey.nChannelId);
        nHash ^= std::hash<int>()(stKey.nRuleId) + 0x9e3779b9U + (nHash << 6U) + (nHash >> 2U);
        nHash ^= std::hash<uint64_t>()(stKey.ullTrackId) + 0x9e3779b9U + (nHash << 6U) + (nHash >> 2U);
        return nHash;
    }
};

/* 单目标轨迹状态 */
struct TrackState_S
{
    Geometry_NS::Point_S stLastPosition;    /* 上一帧位置 */
    Geometry_NS::Point_S stCurrentPosition; /* 当前帧位置 */
    bool bHasLast = false;                  /* 是否已有上一帧位置，首帧为 false */
    int64_t llLastUpdateMs = 0;             /* 最后更新 monotonic 毫秒时间戳 */
};
} // namespace PeopleFlow_NS
} // namespace AiPipeline_NS

#endif // CAP_AI_PEOPLE_STATISTICS
