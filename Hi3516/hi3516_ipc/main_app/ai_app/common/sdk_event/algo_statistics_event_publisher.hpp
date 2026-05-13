/**
 * @FilePath     : algo_statistics_event_publisher.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 19:42:45
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-29 14:22:38
 * @Description  : AI 算法 SDK 统计事件推送公共类
 */

#pragma once

#include "algo_sdk_event_publisher_base.hpp"
#include "event_alarm/statistics/event_statistics_reporter.hpp"

namespace AiAppCommon
{
class CAlgoStatisticsEventPublisher : public CAlgoSdkEventPublisherBase,
                                      public EventStatistics_NS::IEventStatisticsReporter
{
public:
    /**
     * @brief   : 上报统计事件到 SDK 客户端
     * @param    {EventStatistics_NS::Report_S} &stReport：统计事件报告
     * @return   {void}
     */
    void report(const EventStatistics_NS::Report_S &stReport) override;

    /**
     * @brief   : 判断当前是否需要构造统计图片负载
     * @return   {bool} true：存在 SDK 客户端 false：无客户端，业务侧可跳过图片编码
     */
    bool shouldBuildHeavyPayload() const override;
};
} // namespace AiAppCommon
