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

#include "NetTVSDKServer.h"
#include "control_manage.h"
#include "stats_protocol.h"
#include "stats_sink.h"

namespace Classroom
{
/* 共享头里的基址与协议文档必须同值：不同值意味着我们在往另一段发报文，
   平台按本仓文档解析会得到一堆彼此不相干的字段——编译期就拦下来 */
static_assert(static_cast<uint32_t>(NET_ALARM_BASE_STATISTICS) == CLASSROOM_STATS_TVSDK_BASE,
              "stats command base drifted from NET_ALARM_BASE_STATISTICS");

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
        return ControlManage::instance()->tvsdk_get_client_count() > 0;
    }

    bool send(StatsTopic_E enTopic, const void *pData, uint32_t u32Len) override
    {
        if (nullptr == pData || 0 == u32Len || !ready())
        {
            return false;
        }
        return ControlManage::instance()->tvsdk_push_alarm(
                   static_cast<int>(statsTopicCmd(enTopic)), pData,
                   static_cast<int>(u32Len)) >= 0;
    }
};
}
