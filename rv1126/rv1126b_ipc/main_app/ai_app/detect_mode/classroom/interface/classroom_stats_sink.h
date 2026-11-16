/**
 * @FilePath     : classroom_stats_sink.h
 * @Author       : leiyy
 * @Date         : 2026-11-02 10:00:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-02 10:00:00
 * @Description  : 统计出口的 TVSDK 适配传输层
 */

#pragma once

#include <cstdint>

#include "event_linkage_dict.h"
#include "stats_protocol.h"
#include "stats_sink.h"

namespace Classroom
{
/**
 * @brief TVSDK 推送实现：客户端数为 0 时报"通道不可用"
 * @note  `ready()` 被 CStatsHub 每个节拍问一次（只是读一个计数，无锁）；
 *        不可用时不产包，避免每拍敲一个没人接的服务
 */
class CTvSdkStatsSink final : public IStatsSink
{
public:
    bool ready() const override
    {
        return EventLinkageDict::tvsdk_client_count() > 0;
    }

    bool send(StatsTopic_E enTopic, const void *pData, uint32_t u32Len) override
    {
        if (nullptr == pData || 0 == u32Len || !ready())
        {
            return false;
        }
        /* 框架门：客户端闸 + 诊断 + 返回信号（与告警门同一套纪律）。
           >=0 = 平台侧接受；-1 = 两扇门之间客户端掉线/推送失败 */
        return EventLinkageDict::push_tvsdk_classroom_stats_alarm(
                   static_cast<int>(statsTopicCmd(enTopic)), pData,
                   static_cast<int>(u32Len)) >= 0;
    }
};
}
