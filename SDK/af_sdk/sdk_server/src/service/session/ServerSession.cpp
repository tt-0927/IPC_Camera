/**
 * @file ServerSession.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief ServerSession 模块实现
 * 功能说明：
 * 1. 实现 ServerSession 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#include "ServerSession.h"
#include "NetSdkLog.h"
#include <utility>

CServerSession::CServerSession(std::string sessionId)
    : m_strSessionId(std::move(sessionId))
{
    UpdateLastActive();
}

CServerSession::~CServerSession()
{

}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 SetConnected 对应的处理。
 * @param [in] bVal 函数处理参数。
 * @return 无返回值。
 */

void CServerSession::SetConnected(bool bVal)
{
    m_bConnected = bVal;
    if (bVal) UpdateLastActive();
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 BeginAlarmListen 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

uint64_t CServerSession::BeginAlarmListen()
{
    uint64_t ulListenSeq = 0;
    {
        std::lock_guard<std::mutex> lock(m_stMutex);
        ulListenSeq = m_uAlarmListenSequence.load() + 1;
        m_uAlarmListenSequence = ulListenSeq;
    }
    m_stCondition.notify_all();
    return ulListenSeq;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 IsCurrentAlarmListen 对应的数据。
 * @param [in] ulListenSeq 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CServerSession::IsCurrentAlarmListen(uint64_t ulListenSeq) const
{
    return ulListenSeq != 0 && m_uAlarmListenSequence.load() == ulListenSeq;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 MarkDisconnectedIfCurrentAlarmListen 定义的内部处理。
 * @param [in] ulListenSeq 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CServerSession::MarkDisconnectedIfCurrentAlarmListen(uint64_t ulListenSeq)
{
    std::lock_guard<std::mutex> lock(m_stMutex);
    if (ulListenSeq == 0 || m_uAlarmListenSequence.load() != ulListenSeq)
    {
        return false;
    }

    m_bConnected = false;
    m_bPushEnabled = false;
    m_uAlarmListenSequence = ulListenSeq + 1;
    std::queue<std::shared_ptr<const AlarmData_S>> empty;
    std::swap(m_stMessageQueue, empty);
    m_uQueuedMessageBytes = 0;
    m_stCondition.notify_all();
    return true;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 UpdateLastActive 对应的处理。
 * @return 无返回值。
 */

void CServerSession::UpdateLastActive()
{
    std::lock_guard<std::mutex> lock(m_stMutex);
    m_stLastActive = std::chrono::steady_clock::now();
}

std::chrono::steady_clock::time_point CServerSession::GetLastActive() const
{
    std::lock_guard<std::mutex> lock(m_stMutex);
    return m_stLastActive;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 IsTimeout 对应的数据。
 * @param [in] nTimeoutSec 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CServerSession::IsTimeout(int nTimeoutSec) const
{
    /* 如果已断开连接，检查断开时长是否超过阈值 */
    if (!m_bConnected)
	{
        std::lock_guard<std::mutex> lock(m_stMutex);
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - m_stLastActive).count();
        return duration > nTimeoutSec;
    }
    return false;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 IsZombie 对应的数据。
 * @param [in] nTimeoutSec 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CServerSession::IsZombie(int nTimeoutSec) const
{
    /* 即使连接状态为 true，如果太久没有心跳更新，也视为僵尸连接 */
    std::lock_guard<std::mutex> lock(m_stMutex);
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - m_stLastActive).count();
    return duration > nTimeoutSec;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 在持有会话锁时移除队首消息并更新队列字节计数。
 * @param [in,out] uDroppedCount 本次丢弃消息数量。
 * @param [in,out] uDroppedBytes 本次丢弃消息字节数。
 * @return 无返回值。
 */
void CServerSession::RemoveFrontMessageLocked(size_t& uDroppedCount, size_t& uDroppedBytes)
{
    if (m_stMessageQueue.empty())
    {
        return;
    }

    const std::shared_ptr<const AlarmData_S>& front = m_stMessageQueue.front();
    const size_t uPayloadSize = front ? front->GetPayloadSize() : 0U;
    m_uQueuedMessageBytes = uPayloadSize > m_uQueuedMessageBytes
        ? 0U : m_uQueuedMessageBytes - uPayloadSize;
    m_stMessageQueue.pop();
    ++uDroppedCount;
    uDroppedBytes += uPayloadSize;
}

/**
 * @brief 将实时告警加入会话队列，并执行时效、条数及字节数限制。
 * @param [in] pstData 共享只读告警消息。
 * @return true 表示消息已入队；false 表示消息被拒绝。
 */
bool CServerSession::EnqueueMessage(std::shared_ptr<const AlarmData_S> pstData)
{
    if (!pstData)
    {
        NETSDK_LOG_MESSAGE_WARN("告警队列拒绝空消息: session=%s", m_strSessionId.c_str());
        return false;
    }

    const size_t uDataBytes = pstData->GetPayloadSize();
    size_t uDroppedCount = 0;
    size_t uDroppedBytes = 0;
    size_t uQueuedCount = 0;
    size_t uQueuedBytes = 0;
    uint64_t ulTotalDroppedCount = 0;
    uint64_t ulTotalDroppedBytes = 0;
    std::string clientIp;
    bool bDroppedOversizedMessage = false;
    bool bEnqueued = false;

    {
        std::lock_guard<std::mutex> lock(m_stMutex);
        const auto now = std::chrono::steady_clock::now();

        /* 告警为实时数据，超过有效期的旧数据无需继续占用慢客户端队列。 */
        while (!m_stMessageQueue.empty() &&
               std::chrono::duration_cast<std::chrono::seconds>(
                   now - m_stMessageQueue.front()->enqueueTime).count() >= kMaxQueuedAlarmAgeSeconds)
        {
            RemoveFrontMessageLocked(uDroppedCount, uDroppedBytes);
        }

        if (uDataBytes > kMaxQueuedAlarmBytes)
        {
            ++uDroppedCount;
            uDroppedBytes += uDataBytes;
            bDroppedOversizedMessage = true;
        }
        else
        {
            /* 淘汰最旧消息，保留最新告警；同时限制队列条数与总字节数。 */
            while (!m_stMessageQueue.empty() &&
                   (m_stMessageQueue.size() >= kMaxQueuedAlarmCount ||
                    m_uQueuedMessageBytes > kMaxQueuedAlarmBytes - uDataBytes))
            {
                RemoveFrontMessageLocked(uDroppedCount, uDroppedBytes);
            }

            m_stMessageQueue.push(std::move(pstData));
            m_uQueuedMessageBytes += uDataBytes;
            bEnqueued = true;
        }

        m_uDroppedMessageCount += uDroppedCount;
        m_uDroppedMessageBytes += uDroppedBytes;
        uQueuedCount = m_stMessageQueue.size();
        uQueuedBytes = m_uQueuedMessageBytes;
        ulTotalDroppedCount = m_uDroppedMessageCount;
        ulTotalDroppedBytes = m_uDroppedMessageBytes;
        clientIp = m_strClientIp;
    }

    if (uDroppedCount > 0)
    {
        NETSDK_LOG_MESSAGE_WARN(
            "告警队列发生丢弃: session=%s, client=%s, 本次丢弃=%zu条/%zu字节, 累计丢弃=%llu条/%llu字节, 当前队列=%zu条/%zu字节, 原因=%s",
            m_strSessionId.c_str(), clientIp.c_str(), uDroppedCount, uDroppedBytes,
            static_cast<unsigned long long>(ulTotalDroppedCount),
            static_cast<unsigned long long>(ulTotalDroppedBytes), uQueuedCount, uQueuedBytes,
            bDroppedOversizedMessage ? "单条消息超过8MiB" : "消息过期或队列容量达到上限");
    }

    if (bEnqueued)
    {
        m_stCondition.notify_one();
    }
    return bEnqueued;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 DequeueMessage 定义的内部处理。
 * @param [out] pOutMsg 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
bool CServerSession::DequeueMessage(std::shared_ptr<const AlarmData_S>& pOutMsg)
{
    std::lock_guard<std::mutex> lock(m_stMutex);
    if (m_stMessageQueue.empty()) {
        return false;
    }
    pOutMsg = std::move(m_stMessageQueue.front()); /* 仅移动共享指针，无数据拷贝 */
    m_stMessageQueue.pop();
    return true;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 HasMessages 对应的数据。
 * @return 返回该处理的状态或结果。
 */

bool CServerSession::HasMessages()
{
    std::lock_guard<std::mutex> lock(m_stMutex);
    return !m_stMessageQueue.empty();
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 ClearMessageQueue 定义的内部处理。
 * @return 无返回值。
 */

void CServerSession::ClearMessageQueue()
{
    std::lock_guard<std::mutex> lock(m_stMutex);
    std::queue<std::shared_ptr<const AlarmData_S>> empty;
    std::swap(m_stMessageQueue, empty);
    m_uQueuedMessageBytes = 0;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 WaitForData 定义的内部处理。
 * @param [in] nTimeoutMs 函数处理参数。
 * @param [in] ulListenSeq 函数处理参数。
 * @return 无返回值。
 */

void CServerSession::WaitForData(int nTimeoutMs, uint64_t ulListenSeq)
{
    std::unique_lock<std::mutex> lock(m_stMutex);
    m_stCondition.wait_for(lock, std::chrono::milliseconds(nTimeoutMs), [this, ulListenSeq] {
        return !m_stMessageQueue.empty() || (ulListenSeq != 0 && !IsCurrentAlarmListen(ulListenSeq));
    });
}
