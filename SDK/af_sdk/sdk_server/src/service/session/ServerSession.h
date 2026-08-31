/**
 * @file ServerSession.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-08-31
 *
 * @brief ServerSession 模块接口与类型定义
 * 功能说明：
 * 1. 声明 ServerSession 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 *
 * @par 修改记录
 * 2026-08-28 qinjt：统一本次告警队列优化涉及的命名和函数注释，补充不可变告警负载说明。
 * 2026-08-31 qinjt：补充会话超时状态读取的线程安全约束。
 */
#pragma once

#include <cstddef>
#include <string>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <chrono>
#include <queue>
#include <memory>
#include <condition_variable>
#include <vector>
#include <tvsdkhttplib.h>

using namespace tvsdk;

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 单个客户端会话对象
 * 管理客户端的连接状态、鉴权状态、心跳保活以及消息推送队列
 */
class CServerSession : public std::enable_shared_from_this<CServerSession>
{
public:
    /**
     * @brief 创建一个客户端会话对象。
     * @param [in] strSessionId 会话标识。
     * @return 无返回值。
     */
    explicit CServerSession(std::string strSessionId);

    /**
     * @brief 销毁客户端会话对象并释放会话队列资源。
     * @param 无。
     * @return 无返回值。
     */
    ~CServerSession();

    /**
     * @brief 禁止复制客户端会话对象。
     * @param [in] stOtherSession 待复制的会话对象。
     * @return 无返回值。
     */
    CServerSession(const CServerSession&) = delete;

    /**
     * @brief 禁止赋值客户端会话对象。
     * @param [in] stOtherSession 待赋值的会话对象。
     * @return 当前会话对象引用。
     */
    CServerSession& operator=(const CServerSession&) = delete;

    /**
     * @brief 获取会话标识。
     * @param 无。
     * @return 当前会话标识。
     */
    std::string GetSessionId() const
    {
        return m_strSessionId;
    }

    /**
     * @brief 获取客户端 IP 地址。
     * @param 无。
     * @return 客户端 IP 地址。
     */
    std::string GetClientIP() const
    {
        return m_strClientIp;
    }

    /**
     * @brief 设置客户端 IP 地址。
     * @param [in] strClientIp 客户端 IP 地址。
     * @return 无返回值。
     */
    void SetClientIP(const std::string& strClientIp)
    {
        m_strClientIp = strClientIp;
    }

    /**
     * @brief 查询客户端是否已登录。
     * @param 无。
     * @return true 表示已登录，false 表示未登录。
     */
    bool IsLogined() const
    {
        return m_bLoggedIn;
    }

    /**
     * @brief 设置客户端登录状态。
     * @param [in] bLogined true 表示已登录，false 表示未登录。
     * @return 无返回值。
     */
    void SetLogined(bool bLogined)
    {
        m_bLoggedIn = bLogined;
    }

    /**
     * @brief 查询客户端是否存在活动连接。
     * @param 无。
     * @return true 表示已连接，false 表示未连接。
     */
    bool IsConnected() const
    {
        return m_bConnected;
    }

    /**
     * @brief 设置客户端连接状态。
     * @param [in] bConnected true 表示已连接，false 表示未连接。
     * @return 无返回值。
     */
    void SetConnected(bool bConnected);

    /**
     * @brief 查询客户端是否已订阅告警推送。
     * @param 无。
     * @return true 表示已订阅，false 表示未订阅。
     */
    bool IsPushEnabled() const
    {
        return m_bPushEnabled;
    }

    /**
     * @brief 设置客户端告警推送订阅状态。
     * @param [in] bPushEnabled true 表示启用，false 表示关闭。
     * @return 无返回值。
     */
    void SetPushEnabled(bool bPushEnabled)
    {
        m_bPushEnabled = bPushEnabled;
    }

    /**
     * @brief 为新的 AlarmListen 长连接分配递增序号。
     * @param 无。
     * @return 新连接对应的监听序号。
     */
    uint64_t BeginAlarmListen();

    /**
     * @brief 判断给定序号是否仍是当前 AlarmListen 长连接。
     * @param [in] uListenSequence 待校验的监听序号。
     * @return true 表示是当前连接，false 表示不是当前连接。
     */
    bool IsCurrentAlarmListen(uint64_t uListenSequence) const;

    /**
     * @brief 获取当前 AlarmListen 长连接序号。
     * @param 无。
     * @return 当前监听序号。
     */
    uint64_t GetAlarmListenSeq() const
    {
        return m_uAlarmListenSequence.load();
    }

    /**
     * @brief 仅在监听序号匹配时标记连接断开并清空队列。
     * @param [in] uListenSequence 待断开的监听序号。
     * @return true 表示处理成功，false 表示序号已过期或参数无效。
     */
    bool MarkDisconnectedIfCurrentAlarmListen(uint64_t uListenSequence);

    /**
     * @brief 更新会话最后活跃时间。
     * @param 无。
     * @return 无返回值。
     */
    void UpdateLastActive();

    /**
     * @brief 检查断开状态持续时间是否超过阈值。
     * @param [in] nTimeoutSec 超时时间，单位为秒。
     * @return true 表示已超时，false 表示未超时或当前仍连接。
     */
    bool IsTimeout(int nTimeoutSec) const;

    /**
     * @brief 检查会话是否长时间没有活跃数据。
     * @param [in] nTimeoutSec 僵尸判断阈值，单位为秒。
     * @return true 表示是僵尸会话，false 表示仍活跃。
     */
    bool IsZombie(int nTimeoutSec) const;

    /**
     * @author tianl (tianl@kfb.cn)
     * @brief 判断是否到达发送心跳的时间，并在满足条件时更新时间戳。
     * @param [in] nIntervalSec 心跳发送最小间隔，单位为秒。
     * @return true 表示应发送心跳；false 表示尚未到达发送时间。
     */
    bool ShouldSendHeartbeat(int nIntervalSec)
    {
        std::lock_guard<std::mutex> stLock(m_stMutex);
        const std::chrono::steady_clock::time_point stNow = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(stNow - m_stLastHeartbeatSent).count() >= nIntervalSec)
        {
            m_stLastHeartbeatSent = stNow;
            return true;
        }
        return false;
    }

    /**
     * @brief 单个 multipart 附件的只读传输描述。
     * @note 入队后负载不可变，多个客户端会话可以共享同一份附件数据。
     */
    struct Attachment_S
    {
        std::string strContentType;
        std::string strData;
        std::string strName;
        std::string strFilename;
    };

    /**
     * @brief 一个告警消息的不可变共享负载。
     * @note 负载包含 JSON 文本和 multipart 附件，队列只保存其共享指针。
     */
    struct AlarmData_S
    {
        std::string strJson;
        std::vector<Attachment_S> aAttachments;
    };

    /*
     * 告警负载在入队后只读。多个会话共享同一份负载，队列只保存智能指针，
     * 避免每个客户端分别复制 JSON 和附件数据。
     */
    using AlarmDataPtr = std::shared_ptr<const AlarmData_S>;

    /**
     * @brief 将告警负载加入发送队列并唤醒等待线程。
     * @param [in] pstAlarmData 不可变告警负载共享指针。
     * @return 无返回值。
     */
    void EnqueueMessage(AlarmDataPtr pstAlarmData);

    /**
     * @brief 从发送队列转移一条告警负载。
     * @param [out] pstAlarmData 输出的告警负载共享指针。
     * @return true 表示成功出队，false 表示队列为空。
     */
    bool DequeueMessage(AlarmDataPtr& pstAlarmData);

    /**
     * @brief 判断发送队列是否包含消息。
     * @param 无。
     * @return true 表示队列非空，false 表示队列为空。
     */
    bool HasMessages();

    /**
     * @brief 清空发送队列中的所有过期告警。
     * @param 无。
     * @return 无返回值。
     */
    void ClearMessageQueue();

    /**
     * @brief 等待新告警、监听序号变化或超时返回。
     * @param [in] nTimeoutMs 最大等待时间，单位为毫秒。
     * @param [in] uListenSequence 当前监听序号，默认值为 0 表示不校验序号。
     * @return 无返回值。
     */
    void WaitForData(int nTimeoutMs, uint64_t uListenSequence = 0);

private:
    static constexpr std::size_t NETSDK_SESSION_MESSAGE_QUEUE_MAX_SIZE = 100;

    const std::string m_strSessionId;
    std::string m_strClientIp;

    std::atomic<bool> m_bLoggedIn{false};
    std::atomic<bool> m_bConnected{false};
    std::atomic<bool> m_bPushEnabled{false};
    std::atomic<uint64_t> m_uAlarmListenSequence{0};

    /**
     * @brief 会话最后活跃时间，用于超时清理。
     */
    std::chrono::steady_clock::time_point m_stLastActive;

    /**
     * @brief 当前 AlarmListen 连接最后发送心跳的时间。
     */
    std::chrono::steady_clock::time_point m_stLastHeartbeatSent{};

    /**
     * @brief 告警消息队列，用于 AlarmListen 长连接推送。
     */
    std::queue<AlarmDataPtr> m_stMessageQueue;

    /**
     * @brief 告警队列、活跃时间和条件变量的互斥锁。
     */
    mutable std::mutex m_stMutex;

    /**
     * @brief 告警入队时唤醒 AlarmListen 内容提供线程。
     */
    std::condition_variable m_stCondition;
};
