/**
 * @file AsioTCPServer.h
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2025-01-06
 *
 * @brief
 */

#pragma once
#include <unordered_map>
#include <iostream>
#include <thread>

#include "TcpAdapter.h"

#include "asio/ip/tcp.hpp"
#include "asio/read.hpp"   
#include "asio/write.hpp"   

class AsioTCPServer : public TcpAdapter
{
public:
    AsioTCPServer() = delete;
    AsioTCPServer(int nPort);
    ~AsioTCPServer();
    /**
     * @brief 启动TCP服务器，开始接受连接
     * @return 返回0表示成功
     */
    int start() override;

    /**
     * @brief 停止TCP服务器，取消所有连接并关闭
     * @return 返回0表示成功
     */
    int stop() override;

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
private:
    /**
     * @brief 异步接受新的TCP连接
     */
    void accept();
private:
    /**
     * @brief IO上下文，用于管理异步操作
     */
    asio::io_context m_ioContext;

    /**
     * @brief TCP接收器，用于接受新的TCP连接
     */
    asio::ip::tcp::acceptor m_acceptor;

    /**
     * @brief 会话映射，存储所有活动的TCP会话
     * @details 键为TcpAdapter指针，值为TcpAdapter的共享指针
     */
    std::unordered_map<TcpAdapter*, std::shared_ptr<TcpAdapter>> m_sessions;
};