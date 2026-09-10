#pragma once

#include "IOBase.h"
#include <thread>
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "Heartbeat.h"

class UDSAdapter;
namespace Net
{

class UDSClient : public IOBase
{
public:
    UDSClient() = default;
    UDSClient(Param_S &stParam);
    ~UDSClient() override;
    int send(const Message_S stMessage) override;
    int receive(Message_S &stMessage) override;
    void set_heartbeat(const void *pData, size_t nLength) override;
    void heartbeat_status(bool bOK);
private:
    void reconnect();
    void receive();
    void receive(Message_S& stMessage, UserParam_S &stUserParam);
    void throw_status(int nStatus);
    /**
     * @brief   : 清理函数
     */
    void cleanup();
private:
    /// @brief 参数
    Param_S m_stParam;
    std::shared_ptr<UDSAdapter> m_client;
    std::shared_ptr<Heartbeat> m_heartbeat;
    std::thread m_receiveThr;
    std::mutex m_mutex;
    std::thread m_tid;
    std::atomic_bool m_bExit{false};
    std::atomic_bool m_bReconnect{false};
    /* 条件变量用于退出 */
    std::condition_variable m_cv;
};

}


