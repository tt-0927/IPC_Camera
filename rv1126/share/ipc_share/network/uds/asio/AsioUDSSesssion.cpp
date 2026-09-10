/**
 * @file AsioUDSSesssion.cpp
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2025-01-08
 * 
 * @brief 
 */


#include "AsioUDSSesssion.h"
#include <iostream>

AsioUDSSesssion::AsioUDSSesssion(asio::local::stream_protocol::socket socket)
    : m_socket(std::move(socket))
{
}
AsioUDSSesssion::~AsioUDSSesssion()
{
    stop();
}

/**
 * @brief 启动函数，通常用于初始化资源或启动服务
 * @return 返回0表示成功
 */
int AsioUDSSesssion::start() 
{
    return 0;
}

/**
 * @brief 停止函数，用于关闭资源或停止服务
 * @return 返回0表示成功
 */
int AsioUDSSesssion::stop() 
{
    try
    {
        std::error_code ec;

        // 如果 socket 是打开状态，则优雅地关闭它
        if (m_socket.is_open())
        {
            m_socket.shutdown(asio::local::stream_protocol::socket::shutdown_both, ec);
            m_socket.close(ec);
        }
    }
    catch (const asio::system_error& e)
    {
        dlog_error("Asio system error: %s (Error code: %d)", e.what(), e.code().value());
        return -e.code().value();
    }
    catch (const std::bad_alloc& e)
    {
        dlog_error("Memory allocation failed: %s", e.what());
        return -1;
    }
    catch (const std::exception& e)
    {
        dlog_error("Standard exception: %s", e.what());
        return -1;
    }
    catch (...)
    {
        dlog_error("Unknown exception caught!");
        return -1;
    }
    
    return 0;
}

/**
 * @brief 发送数据函数，将数据通过socket发送
 * @param pData 指向要发送的数据的指针
 * @param nDataLen 要发送的数据长度
 * @param pHandle 可选参数，通常用于传递句柄或其他上下文信息
 * @return 返回0表示成功，返回-1表示失败
 */
int AsioUDSSesssion::send(const void *pData, int nDataLen, void *pHandle) 
{
    try
    {
        asio::write(m_socket, asio::buffer(pData, nDataLen));
    }
    catch (const asio::system_error& e)
    {
        dlog_error("Asio system error: %s (Error code: %d)", e.what(), e.code().value());
        return -e.code().value();
    }
    catch (const std::bad_alloc& e)
    {
        dlog_error("Memory allocation failed: %s", e.what());
        return -1;
    }
    catch (const std::exception& e)
    {
        dlog_error("Standard exception: %s", e.what());
        return -1;
    }
    catch (...)
    {
        dlog_error("Unknown exception caught!");
        return -1;
    }
    return 0;
}


/**
 * @brief 从Unix域套接字接收数据
 * 
 * 该函数用于从Unix域套接字接收指定长度的数据，并将数据存储到指定的缓冲区中。
 * 
 * @param pData 指向接收数据的缓冲区的指针
 * @param nDataLen 接收数据的最大长度
 * @return int 返回实际接收到的数据长度，如果发生错误则返回-1
 */
int AsioUDSSesssion::receive(void *pData, int nDataLen)
{
    if (pData == nullptr || nDataLen <= 0)
    {
        dlog_error("pData is empty");
        return -1;
    }
    int nRecvLen = 0;
    try
    {
        nRecvLen = asio::read(m_socket, asio::buffer(pData, nDataLen));
    }
    catch (const asio::system_error& e)
    {
        dlog_error("Asio system error: %s (Error code: %d)", e.what(), e.code().value());
        return -e.code().value();
    }
    catch (const std::bad_alloc& e)
    {
        dlog_error("Memory allocation failed: %s", e.what());
        return -1;
    }
    catch (const std::exception& e)
    {
        dlog_error("Standard exception: %s", e.what());
        return -1;
    }
    catch (...)
    {
        dlog_error("Unknown exception caught!");
        return -1;
    }
    
    return nRecvLen;
}

bool AsioUDSSesssion::is_connected()
{
    return true;
}
