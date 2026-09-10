/**
 * @file AsioTCPClient.cpp
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2025-01-08
 *
 * @brief
 * 实现一个基于ASIO库的TCP客户端，提供连接、发送和接收数据的功能。
 */
#include "AsioTCPClient.h"
#include <iostream>

/**
 * @brief 构造函数，初始化TCP客户端并连接到指定的服务器地址和端口。
 * @param host 目标IP。
 * @param nPort 端口。
 */
AsioTCPClient::AsioTCPClient(std::string host, int nPort)
    : m_socket(m_ioContext), m_resolver(m_ioContext), m_host(host), m_nPort(nPort)
{
    try
    {
        
        auto endpoints = m_resolver.resolve(m_host, std::to_string(nPort));
        asio::connect(m_socket, endpoints);
    }
    catch (const asio::system_error &e)
    {
        dlog_error("Asio system error: %s (Error code: %d)", e.what(), e.code().value());
    }
    catch (const std::bad_alloc &e)
    {
        dlog_error("Memory allocation failed: %s", e.what());
    }
    catch (const std::exception &e)
    {
        dlog_error("Standard exception: %s", e.what());
    }
    catch (...)
    {
        dlog_error("Unknown exception caught!");
    }
}

/**
 * @brief 析构函数，停止TCP客户端并释放资源。
 */
AsioTCPClient::~AsioTCPClient()
{
    stop();
}

/**
 * @brief 启动IO上下文以处理异步操作。
 * @return 返回0表示成功。
 */
int AsioTCPClient::start()
{
    std::thread([this]() {
        try 
        {
            m_ioContext.run();
        } 
        catch (const asio::system_error& e)
        {
            dlog_error("Asio system error: %s (Error code: %d)", e.what(), e.code().value());
        }
        catch (const std::bad_alloc& e)
        {
            dlog_error("Memory allocation failed: %s", e.what());
        }
        catch (const std::exception& e)
        {
            dlog_error("Standard exception: %s", e.what());
        }
        catch (...)
        {
            dlog_error("Unknown exception caught!");
        } 
    }).detach();
    return 0;
}

/**
 * @brief 停止TCP客户端，优雅地关闭socket连接。
 * @return 返回0表示成功。
 */
int AsioTCPClient::stop()
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
        m_ioContext.stop();
    }
    catch (const asio::system_error &e)
    {
        dlog_error("Asio system error: %s (Error code: %d)", e.what(), e.code().value());
        return -e.code().value();
    }
    catch (const std::bad_alloc &e)
    {
        dlog_error("Memory allocation failed: %s", e.what());
        return -1;
    }
    catch (const std::exception &e)
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
 * @brief 发送数据到服务器。
 * @param pData 指向要发送数据的指针。
 * @param nDataLen 数据的长度。
 * @param pHandle 额外的句柄参数（未使用）。
 * @return 返回0表示成功，返回-1表示失败。
 */
int AsioTCPClient::send(const void *pData, int nDataLen, void *pHandle)
{
    try
    {
        asio::write(m_socket, asio::buffer(pData, nDataLen));
    }
    catch (const asio::system_error &e)
    {
        //dlog_error("Asio system error: %s (Error code: %d)", e.what(), e.code().value());
        return -e.code().value();
    }
    catch (const std::bad_alloc &e)
    {
        dlog_error("Memory allocation failed: %s", e.what());
        return -1;
    }
    catch (const std::exception &e)
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
 * @brief 尝试重新连接到服务器。
 * @return 返回0表示成功，返回-1表示失败。
 */
int AsioTCPClient::reconnect()
{
    std::error_code ec;

    // 如果 socket 是打开状态，则优雅地关闭它
    if (m_socket.is_open())
    {
        m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        m_socket.close(ec);
    }
    if (ec && ec != asio::error::not_connected)
    {
        // 如果关闭过程中出现非预期错误，返回错误码
        return -1;
    }
    try
    {
        auto endpoints = m_resolver.resolve(m_host, std::to_string(m_nPort));
        asio::connect(m_socket, endpoints);
    }
    catch (const asio::system_error &e)
    {
        //dlog_error("Asio system error: %s (Error code: %d)", e.what(), e.code().value());
        return -e.code().value();
    }
    catch (const std::bad_alloc &e)
    {
        dlog_error("Memory allocation failed: %s", e.what());
        return -1;
    }
    catch (const std::exception &e)
    {
        dlog_error("Standard exception: %s", e.what());
        return -1;
    }
    catch (...)
    {
        dlog_error("Unknown exception caught!");
        return -1;
    }

    if (!m_socket.is_open())
    {
        return -1;
    }
    return 0;
}

/**
 * @brief 接收数据
 * @param pData 指向接收数据缓冲区的指针
 * @param nDataLen 接收数据的长度
 * @return 返回实际接收到的数据长度
 */
int AsioTCPClient::receive(void *pData, int nDataLen)
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
    catch (const asio::system_error &e)
    {
        dlog_error("Asio system error: %s (Error code: %d)", e.what(), e.code().value());
        return -e.code().value();
    }
    catch (const std::bad_alloc &e)
    {
        dlog_error("Memory allocation failed: %s", e.what());
        return -1;
    }
    catch (const std::exception &e)
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
