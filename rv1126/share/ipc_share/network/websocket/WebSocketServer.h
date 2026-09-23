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

/* 后端二选一：evws（libevent，目标后端）| libwebsockets（现状，回退保留），
   两后端实现同名同签名接口，切换由 websocket.cmake 的 IPC_WS_BACKEND 决定 */
#if defined(IPC_WS_BACKEND_EVWS)
class EvwsServer;
#else
class LibWSServer;
#endif
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
    /**
     * @brief 设置管理通道Qos的Dscp（转发到当前后端实现，业务层不感知后端类型）
     * @param nDscp
     * @return int <0 失败
     */
    static int set_qos_dscp(const int &nDscp);
private:
    std::vector<char> get_heartbeat();
    void thr_heartbeat();
    void receive(Message_S& stMessage, UserParam_S &stUserParam);
private:
    /// @brief 参数
    Param_S m_stParam;
#if defined(IPC_WS_BACKEND_EVWS)
    std::shared_ptr<EvwsServer> m_server;
#else
    std::shared_ptr<LibWSServer> m_server;
#endif
    int m_nHeartbeatInterval;
    std::mutex m_mutex;
    std::vector<char> m_heartbeat;
    bool m_bExit = false;
    std::thread m_tid;
};

}


