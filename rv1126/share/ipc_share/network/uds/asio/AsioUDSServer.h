/**
 * @FilePath     : AsioUDSServer.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2025-01-06
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-31 11:14:21
 * @Description  : UDS服务端类
 */

#pragma once

#include <atomic>
#include <unordered_map>
#include <iostream>
#include <thread>
#include "UDSAdapter.h"
#include "NetDefine.h"

#include "asio/local/stream_protocol.hpp"
#include "asio/io_context.hpp"
#include "asio/read.hpp"
#include "asio/write.hpp"

class AsioUDSServer : public UDSAdapter
{
public:
    AsioUDSServer() = delete;
    AsioUDSServer(int nPort);
    ~AsioUDSServer();
    /**
     * @brief 启动UDS服务器，开始接受连接
     * @return 返回0表示成功
     */
    int start() override;

    /**
     * @brief 停止UDS服务器，取消所有连接并关闭
     * @return 返回0表示成功
     */
    int stop() override;
    void stop_session(void *pHandle) override;

    /**
     * @brief 接收数据的函数
     * 
     * @param pData 指向接收数据的缓冲区
     * @param nDataLen 接收数据的长度
     * @return 返回接收结果
     */
    int receive(void *pData, int nDataLen) override;

    /**
     * @brief 发送数据
     * @param pData 要发送的数据指针
     * @param nDataLen 数据长度
     * @param pHandle 会话句柄
     * @return 返回0表示成功，-1表示失败
     */
    int send(const void *pData, int nDataLen, void *pHandle = nullptr) override;

    bool is_connected() override;
private:
    /**
     * @brief 异步接受新的UDS连接
     */
    void accept();
private:
    /**
     * @brief IO上下文，用于管理异步操作
     */
    asio::io_context m_ioContext;

    /**
     * @brief UDS接收器，用于接受新的UDS连接
     */
    asio::local::stream_protocol::acceptor m_acceptor;

    /**
     * @brief 会话映射，存储所有活动的UDS会话
     * @details 键为UDSAdapter指针，值为UDSAdapter的共享指针
     */
    std::unordered_map<UDSAdapter*, std::shared_ptr<UDSAdapter>> m_sessions;

    /**
     * @brief 标志位，用于指示服务器是否正在停止
     */
    std::atomic<bool> m_running{false};

    /* 管理io_context线程 */
    std::thread m_ioThread;
};