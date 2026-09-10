/**
 * @FilePath     : event_statistics_reporter.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-25 09:13:14
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 10:41:04
 * @Description  : 事件统计上报抽象接口
 */

#pragma once

#include "event_statistics_report.hpp"

namespace EventStatistics_NS
{
class IEventStatisticsReporter
{
public:
    /**
     * @brief   : 析构统计上报接口
     * @return   {void}
     */
    virtual ~IEventStatisticsReporter() = default;

    /**
     * @brief   : 上报统计快照
     * @param    {const Report_S &} stReport：统计快照
     * @return   {void}
     */
    virtual void report(const Report_S &stReport) = 0;

    /**
     * @brief   : 判断业务侧是否需要构造图片等重型上报负载
     * @return   {bool} true：需要构造 false：当前无输出消费者，可跳过编码
     */
    virtual bool shouldBuildHeavyPayload() const
    {
        return true;
    }
};

class CNullEventStatisticsReporter : public IEventStatisticsReporter
{
public:
    /**
     * @brief   : 空统计上报实现，用于未接入具体输出通道时占位
     * @param    {const Report_S &} stReport：统计快照
     * @return   {void}
     */
    void report(const Report_S &stReport) override
    {
        (void)stReport;
    }

    /**
     * @brief   : 空上报器无需构造图片负载
     * @return   {bool} false：无需构造
     */
    bool shouldBuildHeavyPayload() const override
    {
        return false;
    }
};
} // namespace EventStatistics_NS
