/**
 * @file ServerSession.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-08-31
 *
 * @brief ServerSession 模块实现
 * 功能说明：
 * 1. 实现 ServerSession 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 *
 * @par 修改记录
 * 2026-08-28 qinjt：统一本次告警队列优化涉及的命名和函数注释，使用共享不可变负载降低复制开销。
 * 2026-08-31 qinjt：为会话超时状态读取增加互斥保护，避免连接状态和活跃时间并发读取不一致。
 */
#include "ServerSession.h"
#include "NetSdkLog.h"

#include <utility>

/**
 * @brief 创建客户端会话并初始化最后活跃时间。
 * @param [in] strSessionId 会话标识。
 * @return 无返回值。
 */
CServerSession::CServerSession(std::string strSessionId)
    : m_strSessionId(std::move(strSessionId))
{
    UpdateLastActive();
}

/**
 * @brief 销毁客户端会话对象。
 * @param 无。
 * @return 无返回值。
 */
CServerSession::~CServerSession() = default;
/**
 * @brief 设置当前会话的连接状态，并在建立连接时刷新活跃时间。
 * @param [in] bConnected true 表示连接已建立，false 表示连接已断开。
 * @return 无返回值。
 */
void CServerSession::SetConnected(bool bConnected)
{
    m_bConnected = bConnected;
    if (bConnected)
    {
        UpdateLastActive();
    }
}
/**
 * @brief 为新的 AlarmListen 长连接生成序号并唤醒旧连接。
 * @param 无。
 * @return 新生成的 AlarmListen 序号。
 */
uint64_t CServerSession::BeginAlarmListen()
{
    uint64_t uListenSequence = 0;
    {
        std::lock_guard<std::mutex> stLock(m_stMutex);
        uListenSequence = m_uAlarmListenSequence.load() + 1;
        m_uAlarmListenSequence = uListenSequence;
    }
    m_stCondition.notify_all();
    return uListenSequence;
}
/**
 * @brief 判断指定的 AlarmListen 序号是否仍为当前有效连接。
 * @param [in] uListenSequence 待校验的 AlarmListen 序号。
 * @return true 表示序号有效，false 表示序号无效或已被新连接替换。
 */
bool CServerSession::IsCurrentAlarmListen(uint64_t uListenSequence) const
{
    return uListenSequence != 0 && m_uAlarmListenSequence.load() == uListenSequence;
}
/**
 * @brief 在序号匹配时标记连接断开并清空未发送告警。
 * @param [in] uListenSequence 待处理连接的 AlarmListen 序号。
 * @return true 表示已完成断开处理，false 表示连接已过期或参数无效。
 */
bool CServerSession::MarkDisconnectedIfCurrentAlarmListen(uint64_t uListenSequence)
{
    std::lock_guard<std::mutex> stLock(m_stMutex);
    if (uListenSequence == 0 || m_uAlarmListenSequence.load() != uListenSequence)
    {
        return false;
    }

    m_bConnected = false;
    m_bPushEnabled = false;
    m_uAlarmListenSequence = uListenSequence + 1;
    std::queue<AlarmDataPtr> stEmptyQueue;
    std::swap(m_stMessageQueue, stEmptyQueue);
    m_stCondition.notify_all();
    return true;
}
/**
 * @brief 更新会话最后活跃时间。
 * @param 无。
 * @return 无返回值。
 */
void CServerSession::UpdateLastActive()
{
    std::lock_guard<std::mutex> stLock(m_stMutex);
    m_stLastActive = std::chrono::steady_clock::now();
}
/**
 * @brief 判断断开状态持续时间是否超过指定阈值。
 * @param [in] nTimeoutSec 超时时间，单位为秒。
 * @return true 表示已超过阈值，false 表示未超时或当前已连接。
 */
bool CServerSession::IsTimeout(int nTimeoutSec) const
{
    std::lock_guard<std::mutex> stLock(m_stMutex);
    if (m_bConnected)
    {
        return false;
    }

    const std::chrono::steady_clock::time_point stNow = std::chrono::steady_clock::now();
    const std::chrono::steady_clock::duration stDuration = stNow - m_stLastActive;
    const long long lDurationSec =
        std::chrono::duration_cast<std::chrono::seconds>(stDuration).count();
    return lDurationSec > nTimeoutSec;
}
/**
 * @brief 判断会话是否长时间没有活跃数据。
 * @param [in] nTimeoutSec 僵尸会话判断阈值，单位为秒。
 * @return true 表示会话已成为僵尸会话，false 表示会话仍活跃。
 */
bool CServerSession::IsZombie(int nTimeoutSec) const
{
    std::lock_guard<std::mutex> stLock(m_stMutex);
    const std::chrono::steady_clock::time_point stNow = std::chrono::steady_clock::now();
    const std::chrono::steady_clock::duration stDuration = stNow - m_stLastActive;
    const long long lDurationSec =
        std::chrono::duration_cast<std::chrono::seconds>(stDuration).count();
    return lDurationSec > nTimeoutSec;
}
/**
 * @brief 将一条不可变告警负载加入会话队列并唤醒等待线程。
 * @param [in] pstAlarmData 待入队的告警负载共享指针。
 * @return 无返回值。
 */
void CServerSession::EnqueueMessage(AlarmDataPtr pstAlarmData)
{
    /*
     * 共享负载为空时直接丢弃，避免发送线程出队后解引用空指针，并保持队列中只保存有效告警。
     */
    if (!pstAlarmData)
    {
        return;
    }

    std::lock_guard<std::mutex> stLock(m_stMutex);
    if (m_stMessageQueue.size() >= NETSDK_SESSION_MESSAGE_QUEUE_MAX_SIZE)
    {
        m_stMessageQueue.pop();
    }
    m_stMessageQueue.push(std::move(pstAlarmData));
    m_stCondition.notify_one();
}
/**
 * @brief 从会话队列中转移一条告警负载，避免再次复制完整消息。
 * @param [out] pstAlarmData 输出的告警负载共享指针。
 * @return true 表示成功出队，false 表示队列为空。
 */
bool CServerSession::DequeueMessage(AlarmDataPtr& pstAlarmData)
{
    std::lock_guard<std::mutex> stLock(m_stMutex);
    if (m_stMessageQueue.empty())
    {
        return false;
    }
    pstAlarmData = std::move(m_stMessageQueue.front());
    m_stMessageQueue.pop();
    return true;
}
/**
 * @brief 判断会话队列中是否存在待发送告警。
 * @param 无。
 * @return true 表示队列非空，false 表示队列为空。
 */
bool CServerSession::HasMessages()
{
    std::lock_guard<std::mutex> stLock(m_stMutex);
    return !m_stMessageQueue.empty();
}
/**
 * @brief 清空会话队列中的全部待发送告警。
 * @param 无。
 * @return 无返回值。
 */
void CServerSession::ClearMessageQueue()
{
    std::lock_guard<std::mutex> stLock(m_stMutex);
    std::queue<AlarmDataPtr> stEmptyQueue;
    std::swap(m_stMessageQueue, stEmptyQueue);
}
/**
 * @brief 等待告警入队、监听序号变化或等待超时。
 * @param [in] nTimeoutMs 最大等待时间，单位为毫秒。
 * @param [in] uListenSequence 当前 AlarmListen 序号，0 表示不校验序号。
 * @return 无返回值。
 */
void CServerSession::WaitForData(int nTimeoutMs, uint64_t uListenSequence)
{
    std::unique_lock<std::mutex> stLock(m_stMutex);
    m_stCondition.wait_for(stLock, std::chrono::milliseconds(nTimeoutMs), [this, uListenSequence] {
        return !m_stMessageQueue.empty() ||
               (uListenSequence != 0 && !IsCurrentAlarmListen(uListenSequence));
    });
}
