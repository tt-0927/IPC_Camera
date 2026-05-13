/**
 * @FilePath     : event_tvsdk_payload.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-24 09:13:14
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 10:42:03
 * @Description  : 事件联动 TVSDK 推送共享负载定义
 */

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

/* TVSDK 事件负载类型，用于区分后续不同告警结构体填充路径 */
enum class EventTvSdkPayloadType_E
{
    NONE = 0,
    BASIC,
    RULE,
    AI_OBJECT,
    PLATE,
    STATISTICS,
};

/* TVSDK 统计业务子类型，数值与 NET_TV_STATISTICS_TYPE_E 保持一致 */
enum class EventTvSdkStatisticsType_E
{
    PEOPLE_FLOW = 1,
    PEOPLE_DENSITY = 2,
};

/* TVSDK JPEG 图片负载，图片内容统一保存二进制 JPEG，不使用 Base64 */
struct EventTvSdkImage_S
{
    /* JPEG 二进制数据 */
    std::vector<unsigned char> vecJpeg;
    /* 图片宽度，未知时保持 0 */
    int nWidth = 0;
    /* 图片高度，未知时保持 0 */
    int nHeight = 0;
    /* 图片标签，如 panorama/object/plate */
    std::string strTag;
};

/* TVSDK 单目标快照负载，使用基础坐标字段避免依赖业务侧目标结构 */
struct EventTvSdkTarget_S
{
    /* 目标跟踪 ID */
    int nTrackId = -1;
    /* 规则 ID 或规则下标 */
    int nRuleId = -1;
    /* 快照类型，进入/离开/区域当前目标由业务侧约定 */
    int nSnapshotType = 0;
    /* 目标框左上角 X 坐标 */
    int nLeft = 0;
    /* 目标框左上角 Y 坐标 */
    int nTop = 0;
    /* 目标框右下角 X 坐标 */
    int nRight = 0;
    /* 目标框右下角 Y 坐标 */
    int nBottom = 0;
    /* 目标快照时间戳，单位毫秒 */
    long long llTimestampMs = 0;
    /* 人流统计方向或业务扩展方向，未知时保持 0 */
    int nDirection = 0;
    /* 非核心扩展字段，避免为临时属性频繁扩结构 */
    std::map<std::string, std::string> mapExtras;
};

/* TVSDK 统计类负载，第一阶段用于人流统计，后续可复用到人员密度 */
struct EventTvSdkStatisticsPayload_S
{
    /* 统计子类型，取值由 TVSDK 协议 NET_TV_STATISTICS_TYPE_* 对齐 */
    int nStatisticsType = 0;
    /* 规则 ID 或规则下标 */
    int nRuleId = 0;
    /* 报告时间戳，单位毫秒 */
    long long llTimestampMs = 0;
    /* 统计报告序号 */
    uint32_t nReportSeq = 0;
    /* 累计进入人数 */
    uint32_t nEnterCount = 0;
    /* 累计离开人数 */
    uint32_t nLeaveCount = 0;
    /* 累计通行总人数 */
    uint32_t nTotalCount = 0;
    /* 当前区域人数，用于人员密度统计 */
    uint32_t nCurrentPeopleCount = 0;
    /* 平均停留时间，单位秒，用于人员密度统计 */
    uint32_t nAverageStayTimeSec = 0;
    /* 当前报告涉及的目标快照 */
    std::vector<EventTvSdkTarget_S> vecTargets;
    /* 全景 JPEG，第一阶段允许为空 */
    EventTvSdkImage_S stPanoramaImage;
};

/* TVSDK 事件推送统一负载对象，通过 shared_ptr 挂载到事件上下文中传递所有权 */
struct EventTvSdkPayload_S
{
    /* 当前负载类型 */
    EventTvSdkPayloadType_E enType = EventTvSdkPayloadType_E::NONE;
    /* 是否允许状态机缓存该负载，默认不缓存以避免大图长期占用内存 */
    bool bAllowCacheInStateMachine = false;
    /* 通用全景图负载，普通/周界事件后续可复用 */
    EventTvSdkImage_S stPanoramaImage;
    /* 统计类负载 */
    EventTvSdkStatisticsPayload_S stStatistics;
};
