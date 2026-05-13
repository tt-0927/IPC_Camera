/**
 * @file WebSocketServer.h
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2024-10-08
 * 
 * @brief 
 */
#pragma once

#include "IOBase.h"
#include <thread>
#include <map>
#include <vector>
#include <memory>
#include <mutex>

class LibWSServer;
namespace Net
{

class WebSocketServer : public IOBase
{
public:
    WebSocketServer() = default;
    WebSocketServer(Param_S &stParam);
    ~WebSocketServer() override;
    int send(const Message_S stMessage) override;
    int receive(Message_S &stMessage) override;
    void set_heartbeat(const void *pData, size_t nLength) override;
      /**
     * @brief 断开所有客户端并关闭服务器
     */
    void disconnect();
    /**
     * @brief 设置上传文件路径
     * @param strFilePath 
     */
    void set_file_upload_path(const std::string &strFilePath);
    std::string get_upload_fileName();
private:
    std::vector<char> get_heartbeat();
    void thr_heartbeat();
    void receive(Message_S& stMessage, UserParam_S &stUserParam);
private:
    /// @brief 参数
    Param_S m_stParam;
    std::shared_ptr<LibWSServer> m_server;
    int m_nHeartbeatInterval;
    std::mutex m_mutex;
    std::vector<char> m_heartbeat;
    bool m_bExit = false;
    std::thread m_tid;
};

}


