/**
 * @FilePath     : AsioUDSClient.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2025-01-06
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-31 11:12:27
 * @Description  : UDS客户端类，基于ASIO库实现异步网络操作。
 */

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include "IpcRet.h"
#include "NetDefine.h"
#include <thread>

#include "asio/local/stream_protocol.hpp"
#include "asio/io_context.hpp"    
#include "asio/read.hpp"    
#include "asio/write.hpp"    

#include "UDSAdapter.h"
class AsioUDSClient : public UDSAdapter
{
public:
    AsioUDSClient(std::string host, int nPort);
    ~AsioUDSClient();

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
     * @brief 从Unix域套接字接收数据
     * 
     * 该函数用于从Unix域套接字接收指定长度的数据，并将数据存储到指定的缓冲区中。
     * 
     * @param pData 指向接收数据的缓冲区的指针
     * @param nDataLen 接收数据的最大长度
     * @return int 返回实际接收到的数据长度，如果发生错误则返回-1
     */
    int receive(void *pData, int nDataLen)  override;

    bool is_connected() override;
private:    
    /**
     * @brief AsioUDSClient类的成员变量
     */
    asio::io_context m_ioContext;
    /**
     * @brief UDS套接字，用于网络连接
     */
    asio::local::stream_protocol::socket m_socket;
    /**
     * @brief 存储主机地址
     */
    std::string m_host;

    /**
     * @brief 存储端口号
     */
    int m_nPort = 0;

    bool m_bIsConnected = false;
};