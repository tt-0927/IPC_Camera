/**
 * @FilePath     : algo_statistics_event_publisher.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 19:42:45
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-29 14:22:34
 * @Description  : AI 算法 SDK 统计事件推送公共类实现
 */

#include "algo_statistics_event_publisher.hpp"

#include "event_statistics_tvsdk_reporter.h"

namespace AiAppCommon
{
void CAlgoStatisticsEventPublisher::report(const EventStatistics_NS::Report_S &stReport)
{
    if (!hasClient())
    {
        return;
    }

    /* 统计协议转换继续复用共享联动适配器，业务公共层只统一入口和客户端判断 */
    CTvSdkEventStatisticsReporter stTvSdkReporter;
    stTvSdkReporter.report(stReport);
}

bool CAlgoStatisticsEventPublisher::shouldBuildHeavyPayload() const
{
    return hasClient();
}
} // namespace AiAppCommon
