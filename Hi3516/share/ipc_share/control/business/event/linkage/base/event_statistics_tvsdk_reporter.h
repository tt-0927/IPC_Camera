/**
 * @FilePath     : event_statistics_tvsdk_reporter.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-25 09:13:14
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 10:41:47
 * @Description  : 事件统计 TVSDK 上报适配器
 */

#pragma once

#include "event_alarm/statistics/event_statistics_reporter.hpp"

class CTvSdkEventStatisticsReporter : public EventStatistics_NS::IEventStatisticsReporter
{
public:
    /**
     * @brief   : 上报统计快照到 TVSDK 客户端
     * @param    {EventStatistics_NS::Report_S} &stReport：统计快照
     * @return   {void}
     */
    void report(const EventStatistics_NS::Report_S &stReport) override;
};
