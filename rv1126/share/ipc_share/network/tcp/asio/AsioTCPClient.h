/**
 * @file AsioTCPClient.h
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2025-01-06
 * 
 * @brief TCP客户端类，基于ASIO库实现异步网络操作。
 */
#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <thread>

#include "IpcRet.h"
#include "NetDefine.h"

#include "TcpAdapter.h"

#include "asio/ip/tcp.hpp"
#include "asio/read.hpp"   
#include "asio/write.hpp" 
#include "asio/connect.hpp" 
class AsioTCPClient : public TcpAdapter
{
public:
    AsioTCPClient(std::string host, int nPort);
    ~AsioTCPClient();

    /**
     * @brief 启动客户端
     * @return 成功返回0, 失败返回错误代码
     */
    int start() override;

    /**
     * @brief 停止客户端
     * @return 成功返回0, 失败返回错误代码
     */
    int stop() override;

    /**
     * @brief 发送数据
     * @param pData 要发送的数据指针
     * @param nDataLen 数据长度
     * @param pHandle 可选句柄用于处理返回信息
     * @return 成功返回0, 失败返回错误代码
     */
    int send(const void *pData, int nDataLen, void *pHandle = nullptr) override;

    /**
     * @brief 重新连接到服务器
     * @return 成功返回0, 失败返回错误代码
     */
    int reconnect() override;

    /**
     * @brief 接收数据
     * @param pData 指向接收数据缓冲区的指针
     * @param nDataLen 接收数据的长度
     * @return 返回实际接收到的数据长度
     */
    int receive(void *pData, int nDataLen)  override;

private:    
    /**
     * @brief AsioTCPClient类的成员变量
     */
    asio::io_context m_ioContext;
    /**
     * @brief TCP套接字，用于网络连接
     */
    asio::ip::tcp::socket m_socket;
    /**
     * @brief TCP解析器，用于解析主机名和服务
     */
    asio::ip::tcp::resolver m_resolver;
    /**
     * @brief 存储主机地址
     */
    std::string m_host;

    /**
     * @brief 存储端口号
     */
    int m_nPort = 0;
};