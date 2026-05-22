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
#include "dlog.h"

namespace AiAppCommon
{
void CAlgoStatisticsEventPublisher::report(const EventStatistics_NS::Report_S &stReport)
{
    dlog_info("[统计推送诊断] publisher::report 被调用: 类型[%d] 规则[%d] 进入[%d] 离开[%d] 总数[%d]",
              static_cast<int>(stReport.enStatisticsType), stReport.nRuleId,
              stReport.nEnterCount, stReport.nLeaveCount, stReport.nTotalCount);

    if (!hasClient())
    {
        dlog_warn("[统计推送诊断] publisher::hasClient()=false, 无TVSDK客户端, 丢弃统计报告");
        return;
    }

    dlog_info("[统计推送诊断] publisher::hasClient()=true, 转发到TVSDK reporter");
    /* 统计协议转换继续复用共享联动适配器，业务公共层只统一入口和客户端判断 */
    CTvSdkEventStatisticsReporter stTvSdkReporter;
    stTvSdkReporter.report(stReport);
}

bool CAlgoStatisticsEventPublisher::shouldBuildHeavyPayload() const
{
    return hasClient();
}
} // namespace AiAppCommon
