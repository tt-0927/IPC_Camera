/**
 * @file AsioTcpSesssion.h
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2025-01-08
 * 
 * @brief 
 */

#pragma once

#include "asio/ip/tcp.hpp"
#include "asio/read.hpp"   
#include "asio/write.hpp"    
#include "TcpAdapter.h"
class AsioTcpSesssion : public TcpAdapter
{
public:
    AsioTcpSesssion() = delete;
    AsioTcpSesssion(asio::ip::tcp::socket socket);
    ~AsioTcpSesssion();
    
    /**
     * @brief 启动函数，通常用于初始化资源或启动服务
     * @return 返回0表示成功
     */
    int start() override;

    /**
     * @brief 停止函数，用于关闭资源或停止服务
     * @return 返回0表示成功
     */
    int stop() override;

    /**
     * @brief 发送数据函数，将数据通过socket发送
     * @param pData 指向要发送的数据的指针
     * @param nDataLen 要发送的数据长度
     * @param pHandle 可选参数，通常用于传递句柄或其他上下文信息
     * @return 返回0表示成功，返回-1表示失败
     */
    int send(const void *pData, int nDataLen, void *pHandle = nullptr) override;

    /**
     * @brief 接收数据
     * @param pData 指向接收数据缓冲区的指针
     * @param nDataLen 接收数据的长度
     * @return 返回实际接收到的数据长度
     */
    int receive(void *pData, int nDataLen)  override;

    /**
     * @brief 获取ip地址
     * @return ip地址字符串
     */
    std::string get_ip() override;
private:
    /* socket */
    asio::ip::tcp::socket m_socket;
};
