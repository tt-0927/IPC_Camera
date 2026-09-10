/**
 * @FilePath     : AsioUDSClient.cpp
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2025-01-08
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-31 11:13:00
 * @Description  : 实现一个基于ASIO库的UDS客户端，提供连接、发送和接收数据的功能。
 */

#include "AsioUDSClient.h"
#include <iostream>
#include <future>

/**
 * @brief 构造函数，初始化UDS客户端并连接到指定的服务器地址和端口。
 * @param host 目标IP。
 * @param nPort 端口。
 */
AsioUDSClient::AsioUDSClient(std::string host, int nPort)
    : m_socket(m_ioContext)
    , m_host(host)
    , m_nPort(nPort)
{
    try {
        std::string path = Net::UDS_PATH;
        path +=  '/' + std::to_string(nPort);
        asio::local::stream_protocol::endpoint endpoint(path.c_str());
        m_socket.connect(endpoint);
        m_bIsConnected = true;
    } catch (const std::exception& e) {
        dlog_error("connect error: %s %s:%d", e.what(), host.c_str(), nPort);
        m_bIsConnected = false;
    }
}

/**
 * @brief 析构函数，停止UDS客户端并释放资源。
 */
AsioUDSClient::~AsioUDSClient()
{
    stop();
}

/**
 * @brief 启动IO上下文以处理异步操作。
 * @return 返回0表示成功。
 */
int AsioUDSClient::start()
{
    std::thread([this]() {
        try 
        {
            m_ioContext.run();
        }
        catch (const asio::system_error& e)
        {
            m_bIsConnected = false;
            dlog_error("Asio system error: %s port %d (Error code: %d)", e.what(), m_nPort, e.code().value());
        }
        catch (const std::bad_alloc& e)
        {
            m_bIsConnected = false;
            dlog_error("Memory allocation failed: %s", e.what());
        }
        catch (const std::exception& e)
        {
            m_bIsConnected = false;
            dlog_error("Standard exception: %s", e.what());
        } 
        catch (...)
        {
            m_bIsConnected = false;
            dlog_error("Unknown exception caught!");
        }
    }).detach();
    return 0;
}

/**
 * @brief 停止UDS客户端，优雅地关闭socket连接。
 * @return 返回0表示成功。
 */
int AsioUDSClient::stop()
{
    try
    {
        std::error_code ec;
        if (m_socket.is_open())
        {
            m_socket.shutdown(asio::local::stream_protocol::socket::shutdown_both, ec);
            m_socket.close(ec);
        }
        m_ioContext.stop();
        m_bIsConnected = false;
    }
    catch (const asio::system_error& e)
    {
        dlog_error("Asio system error: %s port %d (Error code: %d)", e.what(), m_nPort, e.code().value());
        return -1;
    }
    catch (const std::bad_alloc& e)
    {
        dlog_error("Memory allocation failed: %s port %d", e.what(), m_nPort);
        return -1;
    }
    catch (const std::exception& e)
    {
        dlog_error("Standard exception: %s port %d", e.what(), m_nPort);
        return -1;
    }
    catch (...)
    {
        dlog_error("Unknown exception caught! port %d", m_nPort);
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
int AsioUDSClient::send(const void *pData, int nDataLen, void *pHandle)
{
    try
    {
        asio::write(m_socket, asio::buffer(pData, nDataLen));
    }
    catch (const asio::system_error& e)
    {
        if (e.code().value() == 107)
        {
            /* 没连上 */
            return -1;
        } 
        dlog_error("Asio system error: %s port %d (Error code: %d)", e.what(), m_nPort, e.code().value());
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
 * @brief 尝试重新连接到服务器。
 * @return 返回0表示成功，返回-1表示失败。
 */
int AsioUDSClient::reconnect()
{
    m_bIsConnected = false;
    try {           
        std::error_code ec;

        // 如果 socket 是打开状态，则优雅地关闭它
        if (m_socket.is_open())
        {
            m_socket.shutdown(asio::local::stream_protocol::socket::shutdown_both, ec);
            m_socket.close(ec);
        }
        if (ec && ec != asio::error::not_connected)
        {
            // 如果关闭过程中出现非预期错误，返回错误码
            return -1;
        }
        std::string path = Net::UDS_PATH;
        path +=  '/' + std::to_string(m_nPort);
        asio::local::stream_protocol::endpoint endpoints(path.c_str());
        m_socket.connect(endpoints, ec);
        if (ec) {
            // dlog_error("Socket connect failed: %s (Error code: %d)", ec.message().c_str(), ec.value());
            return -ec.value();
        }
    } 
    catch (const asio::system_error& e)
    {
        dlog_error("Asio system error: %s port %d (Error code: %d)", e.what(), m_nPort, e.code().value());
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
    if (!m_socket.is_open())
    {
        return -1;
    }
    m_bIsConnected = true;
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
int AsioUDSClient::receive(void *pData, int nDataLen)
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
        dlog_error("Asio system error: %s port %d (Error code: %d)", e.what(), m_nPort, e.code().value());
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

bool AsioUDSClient::is_connected()
{
    return m_bIsConnected;
}

