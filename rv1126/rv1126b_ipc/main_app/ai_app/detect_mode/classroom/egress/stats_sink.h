/*
 * @Author: leiyy leiyy@kfb.cn
 * @Date: 2026-11-04 09:34:06
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2026-11-04 16:57:20
 * @FilePath: /TV-3881TJY/rv1126b_ipc/main_app/ai_app/detect_mode/classroom/egress/stats_sink.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/**
 * @FilePath     : stats_sink.h
 * @Author       : leiyy
 * @Date         : 2026-11-02 10:00:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-02 10:00:00
 * @Description  : 统计出口端口（核心只认 topic，不认命令码/传输）
 */

#pragma once

#include <cstdint>
#include <functional>

#include "stats_protocol.h"

namespace Classroom
{
/**
 * @brief 轻量推送函数：参数是完整报文（头+载荷）与命令码
 * @return true=平台侧已接受；false=没送达（无客户端/服务不可用）
 * @note 保留它以让网关/用例不必实现一个类；核心持有的是下面的接口指针。
 */
typedef std::function<bool(uint32_t u32Cmd, const void *pData, uint32_t u32Len)> CStatsPushFn;

class IStatsSink
{
public:
    virtual ~IStatsSink() = default;
    /* 通道当前是否可用（无客户端时应返回 false） */
    virtual bool ready() const = 0;
    /* 发一条载荷；topic 由传输层映射到具体命令码 */
    virtual bool send(StatsTopic_E enTopic, const void *pData, uint32_t u32Len) = 0;
};

/* 函数适配器：把 CStatsPushFn 包装成接口。持有者负责生命周期（CStatsHub 内嵌一份） */
class CFnStatsSink final : public IStatsSink
{
public:
    CFnStatsSink() = default;
    explicit CFnStatsSink(CStatsPushFn fnPush) : m_fnPush(std::move(fnPush)) {}

    void setPushFn(CStatsPushFn fnPush) { m_fnPush = std::move(fnPush); }

    bool ready() const override { return static_cast<bool>(m_fnPush); }

    bool send(StatsTopic_E enTopic, const void *pData, uint32_t u32Len) override
    {
        if (!m_fnPush || nullptr == pData || 0 == u32Len)
        {
            return false;
        }
        return m_fnPush(statsTopicCmd(enTopic), pData, u32Len);
    }

private:
    CStatsPushFn m_fnPush;
};
}
