/**
 * @FilePath     : event_statistics_report.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 09:13:14
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 10:40:47
 * @Description  : 事件统计上报通用数据结构
 */

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "common_define.h"
#include "event_define.h"

namespace EventStatistics_NS
{
/* 统计上报业务类型，后续新增统计事件时在这里扩展，不再为每类事件新增 Report 字段 */
enum class StatisticsType_E
{
    UNKNOWN = 0,
    PEOPLE_FLOW = 1,
    PEOPLE_DENSITY = 2,
};

/* 单目标快照类型 */
enum class SnapshotType_E
{
    ENTER = 0,
    LEAVE = 1,
    REGION_CURRENT = 2,
};

/* 单目标统计快照 */
struct TargetSnapshot_S
{
    /* 目标跟踪 ID，无 track_id 时保持默认值 */
    int nTrackId = -1;
    /* 规则 ID 或规则下标，当前业务无外部规则 ID 时使用规则下标 */
    int nRuleId = -1;
    /* 目标快照类型，用于区分进入、离开或区域当前目标 */
    SnapshotType_E enSnapshotType = SnapshotType_E::ENTER;
    /* 目标框坐标，使用当前算法分辨率坐标系 */
    Common::RectInfo_S stRect;
    /* 当前快照生成时间，单位毫秒 */
    long long llTimestampMs = 0;
    /* 目标方向，跨线类事件可填业务方向枚举值，非方向类事件保持 0 */
    int nDirection = 0;
    /* 扩展字段，仅承载非核心统计信息 */
    std::map<std::string, std::string> mapExtras;
};

/* 统计上报图片负载，业务侧填充 JPEG 二进制数据，协议适配层负责拷贝到目标协议结构 */
struct ImagePayload_S
{
    /* JPEG 二进制数据 */
    std::vector<unsigned char> vecJpeg;
    /* 图片宽度，未知时保持 0 */
    int nWidth = 0;
    /* 图片高度，未知时保持 0 */
    int nHeight = 0;
    /* 图片标签，如 panorama */
    std::string strTag;
};

/* 统一统计上报结构，公共字段扁平化，避免随统计类型增加持续膨胀专用 payload */
struct Report_S
{
    /* 当前上报统计类型 */
    StatisticsType_E enStatisticsType = StatisticsType_E::UNKNOWN;
    /* 主事件类型 */
    Event::Type_E enEventType = Event::Type_E::UNKNOWN;
    /* 当前人数对应的报警事件类型，未命中等级报警时可等于主事件类型 */
    Event::Type_E enAlarmEventType = Event::Type_E::UNKNOWN;
    /* 通道号 */
    int nChnId = 0;
    /* 规则 ID 或规则下标 */
    int nRuleId = 0;
    /* 当前帧时间戳，单位毫秒 */
    long long llFrameTimestampMs = 0;
    /* 单业务递增上报序号 */
    uint32_t nReportSeq = 0;
    /* 累计进入人数，人流统计使用 */
    uint32_t nEnterCount = 0;
    /* 累计离开人数，人流统计使用 */
    uint32_t nLeaveCount = 0;
    /* 累计通行总人数，人流统计使用 */
    uint32_t nTotalCount = 0;
    /* 当前区域内人数，人员密度统计使用 */
    uint32_t nCurrentPeopleCount = 0;
    /* 平均停留时间，单位秒，人员密度统计使用 */
    uint32_t nAverageStayTimeSec = 0;
    /* 当前报告涉及的目标快照 */
    std::vector<TargetSnapshot_S> vecTargets;
    /* 统计上报全景图，人员密度周期上报使用 */
    ImagePayload_S stPanoramaImage;
    /* 统计上报目标图列表，人流统计/人脸抓拍等需要目标小图的场景使用 */
    std::vector<ImagePayload_S> vecTargetImages;
    /* 扩展字段，仅用于摘要或后续兼容 */
    std::map<std::string, std::string> mapExtras;

    /**
     * @brief   : 清空统计上报内容
     * @return   {void}
     */
    void clear()
    {
        *this = Report_S();
    }
};
} // namespace EventStatistics_NS
