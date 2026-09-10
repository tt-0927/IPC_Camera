#pragma once

#include "IOBase.h"
#include <thread>
#include <map>
#include <vector>
#include <memory>
#include <mutex>

#include "Heartbeat.h"
#include "safe_queue.h"

class TcpAdapter;
namespace Net
{

class TCPClient : public IOBase
{
public:
    TCPClient() = default;
    TCPClient(Param_S &stParam);
    ~TCPClient() override;
    int send(const Message_S stMessage) override;
    int receive(Message_S &stMessage) override;
    void set_heartbeat(const void *pData, size_t nLength) override;
    void heartbeat_status(bool bOK);
private:
    void reconnect();
    void receive();
    void receive(Message_S& stMessage, UserParam_S &stUserParam);
    void throw_status(int nStatus);
private:
    /// @brief 参数
    Param_S m_stParam;
    std::shared_ptr<TcpAdapter> m_client;
    std::shared_ptr<Heartbeat> m_heartbeat;
    std::thread m_receiveThr;
    std::mutex m_mutex;
    bool m_bExit = false;
    std::thread m_tid;
    bool m_bReconnect = false;
    SafeQueue<Message_S> m_msgQueue;
    std::thread m_queTid;
    
};

}


