/**
 * @FilePath     : processor_output.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 15:00:00
 * @Description  : AI 事件处理器类型化输出契约定义
 */

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "detection_types.hpp"
#include "event_define.h"

namespace AiPipeline_NS
{
/* 报告内目标快照草稿，坐标保持归一化，由平台输出适配层转换 */
struct ReportTargetDraft_S
{
    uint64_t ullTrackId = 0; /* 目标 Track ID */
    int nSnapshotType = 0;   /* 快照类型，与 EventStatistics_NS::SnapshotType_E 对齐：0 进入 1 离开 */
    int nDirection = 0;      /* 穿越方向，Alarm::CrossDirection_E 数值 */
    NormalizedRect_S stRect; /* 归一化目标框 */
};

/* 统计报告草稿，对应现有 EventStatistics_NS::Report_S 的共享前结构 */
struct StatisticsReportDraft_S
{
    int nChannelId = 0;                          /* 通道号 */
    int nRuleId = 0;                             /* 规则 ID */
    int64_t llTimestampMs = 0;                   /* wall clock 毫秒时间戳 */
    uint32_t nReportSeq = 0;                     /* 递增上报序号 */
    int nStatisticsType = 0;                     /* 统计类型，与 EventStatistics_NS::StatisticsType_E 对齐，默认 UNKNOWN 按人流统计兼容处理 */
    uint32_t nEnterCount = 0;                    /* 累计进入人数 */
    uint32_t nLeaveCount = 0;                    /* 累计离开人数 */
    uint32_t nTotalCount = 0;                    /* 累计通行总人数 */
    uint32_t nCurrentStayCount = 0;              /* 当前滞留人数 */
    uint32_t nCurrentPeopleCount = 0;            /* 当前区域内人数，人员密度使用 */
    uint32_t nAverageStayTimeSec = 0;            /* 平均停留时间（秒），人员密度使用 */
    Event::Type_E enAlarmEventType = Event::Type_E::UNKNOWN; /* 当前人数命中的报警等级事件，人员密度使用 */
    std::vector<ReportTargetDraft_S> vecTargets; /* 本报告涉及的目标快照 */
    bool bForceReport = false;                   /* 是否跨线强制上报 */
    bool bNeedPanorama = false;                  /* 是否需要全景图 */
};

/* 事件条件，平台输出适配层据此驱动联动状态机 */
struct EventCondition_S
{
    Event::Type_E enEventType = Event::Type_E::UNKNOWN; /* 事件类型 */
    bool bConditionMet = false;                         /* 当前条件是否满足 */
    int nChannelId = 0;                                 /* 通道号 */
    int nRuleId = 0;                                    /* 规则 ID */
    int64_t llWallTimestampMs = 0;                      /* wall clock 毫秒时间戳 */
    int64_t llMonotonicTimestampMs = 0;                 /* monotonic 毫秒时间戳 */
    uint32_t nEnterCount = 0;                           /* 累计进入人数 */
    uint32_t nLeaveCount = 0;                           /* 累计离开人数 */
    uint32_t nTotalCount = 0;                           /* 累计通行总人数 */
    uint32_t nCurrentStayCount = 0;                     /* 当前滞留人数 */
    uint32_t nCurrentPeopleCount = 0;                   /* 当前区域内人数，人员密度使用 */

    /* ====== 触发目标信息（TVSDK 结构化字段回补，后向兼容安全默认值） ====== */
    /* 标识本条 EventCondition 是否携带触发目标信息
     * 缺省 false：人流/密度等不关心目标的处理器不受影响，保持原有行为 */
    bool bHasTarget = false;
    /* 平台无关目标类型语义：0=未知 1=人 2=车 3=人脸
     * 业务仓 OutputExecutor 侧再映射到 TVSDK objType，共享层不含 TVSDK 语义 */
    int nObjectType = 0;
    /* 目标检测置信度，范围 [0,1]，缺省 0 */
    float fConfidence = 0.0F;
    /* 触发目标的归一化矩形框，OutputExecutor 用 native_result_size 反归一化到像素框 */
    NormalizedRect_S stTargetRect;
    /* 触发目标 Track ID，缺省 0 */
    uint64_t ullTargetId = 0;
};

/**
 * @brief   : 将 ObjectType_E 映射到平台无关中性语义整数值
 * @param    {ObjectType_E} enType：共享层语义目标类型
 * @return   {int} 中性整数值：0=未知 1=人 2=车 3=人脸
 * @note    : 业务仓 OutputExecutor 侧再映射到 TVSDK objType，
 *            共享层不含任何 TVSDK 语义
 */
inline int to_neutral_object_type(ObjectType_E enType)
{
    switch (enType)
    {
    case ObjectType_E::PERSON:
        return 1;
    case ObjectType_E::VEHICLE:
        return 2;
    case ObjectType_E::FACE:
        return 3;
    default:
        return 0;
    }
}

/* OSD 叠加草稿，坐标保持归一化 */
struct OverlayItem_S
{
    ObjectType_E enType = ObjectType_E::UNKNOWN; /* 目标类型 */
    NormalizedRect_S stRect;                     /* 归一化矩形 */
    std::optional<uint64_t> optTrackId;          /* 可选 Track ID */
};

/* 图片请求，通过报告序号与统计报告关联，不使用裸指针 */
struct ImageRequest_S
{
    uint32_t nReportSeq = 0;     /* 关联的报告序号 */
    int nStatisticsType = 0;     /* 统计类型，与 EventStatistics_NS::StatisticsType_E 对齐，默认 UNKNOWN 按人流统计兼容处理 */
    bool bPanorama = false;      /* true：全景图 false：目标图 */
    NormalizedRect_S stCropRect; /* 归一化裁剪框，全景图忽略 */
    int nOrder = 0;              /* 请求顺序，目标图按序消费 */
};

/* 单帧处理器总输出：四种低耦合值对象集合 */
struct ProcessorOutput_S
{
    std::vector<StatisticsReportDraft_S> vecStatisticsDrafts; /* 统计报告草稿 */
    std::vector<EventCondition_S> vecEventConditions;         /* 事件条件 */
    std::vector<OverlayItem_S> vecOverlayItems;               /* OSD 叠加草稿 */
    std::vector<ImageRequest_S> vecImageRequests;             /* 图片请求 */

    /**
     * @brief   : 清空全部输出
     * @return   {void}
     */
    void clear()
    {
        vecStatisticsDrafts.clear();
        vecEventConditions.clear();
        vecOverlayItems.clear();
        vecImageRequests.clear();
    }
};
} // namespace AiPipeline_NS
