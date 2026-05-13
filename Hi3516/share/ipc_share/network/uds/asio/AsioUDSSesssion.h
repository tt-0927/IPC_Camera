/**
 * @file AsioUDSSesssion.h
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2025-01-08
 * 
 * @brief 
 */

#pragma once
#include <thread>
#include "UDSAdapter.h"

#include "asio/local/stream_protocol.hpp"
#include "asio/read.hpp"   
#include "asio/write.hpp" 

class AsioUDSSesssion : public UDSAdapter
{
public:
    AsioUDSSesssion() = delete;
    AsioUDSSesssion(asio::local::stream_protocol::socket socket);
    ~AsioUDSSesssion();
    
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
     * @brief 从Unix域套接字接收数据
     * 
     * 该函数用于从Unix域套接字接收指定长度的数据，并将数据存储到指定的缓冲区中。
     * 
     * @param pData 指向接收数据的缓冲区的指针
     * @param nDataLen 接收数据的最大长度
     * @return int 返回实际接收到的数据长度，如果发生错误则返回-1
     */
    int receive(void *pData, int nDataLen) override;
    bool is_connected() override;
private:
    /* socket */
    asio::local::stream_protocol::socket m_socket;
};
