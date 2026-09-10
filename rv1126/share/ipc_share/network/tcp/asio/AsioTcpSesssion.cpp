/**
 * @file AsioTcpSesssion.cpp
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2025-01-08
 * 
 * @brief 
 */


#include "AsioTcpSesssion.h"
#include <iostream>

AsioTcpSesssion::AsioTcpSesssion(asio::ip::tcp::socket socket)
    : m_socket(std::move(socket))
{
}
AsioTcpSesssion::~AsioTcpSesssion()
{
    stop();
}

/**
 * @brief 启动函数，通常用于初始化资源或启动服务
 * @return 返回0表示成功
 */
int AsioTcpSesssion::start() 
{
    return 0;
}

/**
 * @brief 停止函数，用于关闭资源或停止服务
 * @return 返回0表示成功
 */
int AsioTcpSesssion::stop() 
{
    try
    {
        std::error_code ec;
        if (m_socket.is_open())
        {
            asio::ip::tcp::no_delay option(true);
            m_socket.set_option(option);
            m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
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
int AsioTcpSesssion::send(const void *pData, int nDataLen, void *pHandle) 
{
    int nSendLen = 0;
    try
    {
        /* code */
        nSendLen = asio::write(m_socket, asio::buffer(pData, nDataLen));
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
    return nSendLen;
}

/**
 * @brief 接收数据
 * @param pData 指向接收数据缓冲区的指针
 * @param nDataLen 接收数据的长度
 * @return 返回实际接收到的数据长度
 */
int AsioTcpSesssion::receive(void *pData, int nDataLen)
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

std::string AsioTcpSesssion::get_ip()
{    
    std::string ip;
    try {
        if (m_socket.is_open())
        {
            ip = m_socket.remote_endpoint().address().to_string();
        }
    }
    catch (const std::exception& e) {
        dlog_error("Standard exception: %s", e.what());
    }
    catch (...) {
        dlog_error("Unknown exception caught!");
    }
    return ip;
}
