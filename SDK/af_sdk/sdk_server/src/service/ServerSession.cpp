#include "ServerSession.h"
#include "NetSdkLog.h"

CServerSession::CServerSession(std::string sessionId)
    : m_sessionId(std::move(sessionId))
{
    UpdateLastActive();
}

CServerSession::~CServerSession()
{
    
}

void CServerSession::SetConnected(bool val)
{
    m_isConnected = val;
    if (val) UpdateLastActive();
}

void CServerSession::UpdateLastActive()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastActive = std::chrono::steady_clock::now();
}

bool CServerSession::IsTimeout(int timeoutSec) const
{
    // 如果已断开连接，检查断开时长是否超过阈值
    if (!m_isConnected) 
	{
        std::lock_guard<std::mutex> lock(m_mutex);
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastActive).count();
        return duration > timeoutSec;
    }
    return false;
}

bool CServerSession::IsZombie(int timeoutSec) const
{
    // 即使连接状态为 true，如果太久没有心跳更新，也视为僵尸连接
    std::lock_guard<std::mutex> lock(m_mutex);
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastActive).count();
    return duration > timeoutSec;
}

void CServerSession::EnqueueMessage(const AlarmData& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // 队列上限 100 条，防止客户端长时断线时内存无限增长
    // 超出时丢弃最旧的一条（滑动窗口，保留最新数据）
    if (m_msgQueue.size() >= 100)
    {
        m_msgQueue.pop(); // 丢弃最旧的一条
    }
    m_msgQueue.push(data);
}

bool CServerSession::DequeueMessage(AlarmData& outMsg)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_msgQueue.empty()) {
        return false;
    }
    outMsg = m_msgQueue.front();
    m_msgQueue.pop();
    return true;
}

bool CServerSession::HasMessages()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_msgQueue.empty();
}

void CServerSession::ClearMessageQueue()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::queue<AlarmData> empty;
    std::swap(m_msgQueue, empty);
}