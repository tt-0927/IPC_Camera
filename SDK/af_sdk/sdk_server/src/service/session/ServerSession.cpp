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
 * @param [in] val 函数处理参数。
 * @return 无返回值。
 */

void CServerSession::SetConnected(bool val)
{
    m_bConnected = val;
    if (val) UpdateLastActive();
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 BeginAlarmListen 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

uint64_t CServerSession::BeginAlarmListen()
{
    uint64_t listenSeq = 0;
    {
        std::lock_guard<std::mutex> lock(m_stMutex);
        listenSeq = m_uAlarmListenSequence.load() + 1;
        m_uAlarmListenSequence = listenSeq;
    }
    m_stCondition.notify_all();
    return listenSeq;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 IsCurrentAlarmListen 对应的数据。
 * @param [in] listenSeq 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CServerSession::IsCurrentAlarmListen(uint64_t listenSeq) const
{
    return listenSeq != 0 && m_uAlarmListenSequence.load() == listenSeq;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 MarkDisconnectedIfCurrentAlarmListen 定义的内部处理。
 * @param [in] listenSeq 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CServerSession::MarkDisconnectedIfCurrentAlarmListen(uint64_t listenSeq)
{
    std::lock_guard<std::mutex> lock(m_stMutex);
    if (listenSeq == 0 || m_uAlarmListenSequence.load() != listenSeq)
    {
        return false;
    }

    m_bConnected = false;
    m_bPushEnabled = false;
    m_uAlarmListenSequence = listenSeq + 1;
    std::queue<std::shared_ptr<const AlarmData_S>> empty;
    std::swap(m_stMessageQueue, empty);
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 IsTimeout 对应的数据。
 * @param [in] timeoutSec 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CServerSession::IsTimeout(int timeoutSec) const
{
    /* 如果已断开连接，检查断开时长是否超过阈值 */
    if (!m_bConnected)
	{
        std::lock_guard<std::mutex> lock(m_stMutex);
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - m_stLastActive).count();
        return duration > timeoutSec;
    }
    return false;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 IsZombie 对应的数据。
 * @param [in] timeoutSec 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CServerSession::IsZombie(int timeoutSec) const
{
    /* 即使连接状态为 true，如果太久没有心跳更新，也视为僵尸连接 */
    std::lock_guard<std::mutex> lock(m_stMutex);
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - m_stLastActive).count();
    return duration > timeoutSec;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 EnqueueMessage 定义的内部处理。
 * @param [in] data 函数处理参数。
 * @return 无返回值。
 */

void CServerSession::EnqueueMessage(std::shared_ptr<const AlarmData_S> data)
{
    std::lock_guard<std::mutex> lock(m_stMutex);
    /* 队列上限 100 条，防止客户端长时断线时内存无限增长 */
    /* 超出时丢弃最旧的一条（滑动窗口，保留最新数据） */
    if (m_stMessageQueue.size() >= 100)
    {
        m_stMessageQueue.pop(); /* 丢弃最旧的一条 */
    }
    m_stMessageQueue.push(std::move(data)); /* 共享指针入队，无数据拷贝 */
    m_stCondition.notify_one(); /* 唤醒正在等待的 content_provider */
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 DequeueMessage 定义的内部处理。
 * @param [out] outMsg 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CServerSession::DequeueMessage(std::shared_ptr<const AlarmData_S>& outMsg)
{
    std::lock_guard<std::mutex> lock(m_stMutex);
    if (m_stMessageQueue.empty()) {
        return false;
    }
    outMsg = std::move(m_stMessageQueue.front()); /* 仅移动共享指针，无数据拷贝 */
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
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 WaitForData 定义的内部处理。
 * @param [in] timeoutMs 函数处理参数。
 * @param [in] listenSeq 函数处理参数。
 * @return 无返回值。
 */

void CServerSession::WaitForData(int timeoutMs, uint64_t listenSeq)
{
    std::unique_lock<std::mutex> lock(m_stMutex);
    m_stCondition.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this, listenSeq] {
        return !m_stMessageQueue.empty() || (listenSeq != 0 && !IsCurrentAlarmListen(listenSeq));
    });
}
