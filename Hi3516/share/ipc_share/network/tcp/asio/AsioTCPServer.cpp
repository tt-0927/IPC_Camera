/**
 * @file AsioTCPServer.cpp
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2025-01-06
 *
 * @brief
 */

#include "AsioTCPServer.h"
#include "AsioTcpSesssion.h"

AsioTCPServer::AsioTCPServer(int nPort)
    : m_acceptor(m_ioContext)
{
    try
    {
        asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), nPort);
        /* 启用 SO_REUSEADDR 选项，允许地址重用 */
        asio::socket_base::reuse_address option(true);
        /* 打开 acceptor，使用指定的协议 */
        m_acceptor.open(endpoint.protocol());
        /* 设置 SO_REUSEADDR 选项 */
        m_acceptor.set_option(option);
        /* 绑定到指定的端点 */
        m_acceptor.bind(endpoint);
        /* 开始监听连接 */
        m_acceptor.listen();
    }
    catch (const std::exception &e)
    {
        dlog_error("Failed to initialize acceptor: %s", e.what());
        dlog_error("Error listening on port: %d", nPort);
        /* 启动失败直接退出 */
        exit(-1);
    }

    dlog_info("Listening on address: %s, port: %d", m_acceptor.local_endpoint().address().to_string().c_str(), nPort);
}
AsioTCPServer::~AsioTCPServer()
{
    stop();
}
/**
 * @brief 启动TCP服务器，开始接受连接
 * @return 返回0表示成功
 */
int AsioTCPServer::start()
{
    accept();
    std::thread([this]() {
            try
            {
                m_ioContext.run();
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
        }).detach();
    return 0;
}

/**
 * @brief 停止TCP服务器，取消所有连接并关闭
 * @return 返回0表示成功
 */
int AsioTCPServer::stop()
{
    try
    {
        m_acceptor.cancel();
        m_acceptor.close();
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
 * @brief 接收数据的函数
 *
 * @param pData 指向接收数据的缓冲区
 * @param nDataLen 接收数据的长度
 * @return 返回接收结果
 */
int AsioTCPServer::receive(void *pData, int nDataLen)
{
    return 0;
}

/**
 * @brief 发送数据
 * @param pData 要发送的数据指针
 * @param nDataLen 数据长度
 * @param pHandle 会话句柄
 * @return 返回0表示成功，-1表示失败
 */
int AsioTCPServer::send(const void *pData, int nDataLen, void *pHandle)
{
    if (pHandle == nullptr)
    {
        dlog_error("pHandle  is nullptr");
        return -1;
    }
    AsioTcpSesssion *pSession = static_cast<AsioTcpSesssion *>(pHandle);
    return pSession->send(pData, nDataLen);
}

/**
 * @brief 异步接受新的TCP连接
 */
void AsioTCPServer::accept()
{
    m_acceptor.async_accept(
        [this](const asio::error_code &error, asio::ip::tcp::socket socket)
        {
            if (!error)
            {
                /* 创建一个新的TcpAdapter会话对象，使用传入的socket进行初始化 */
                std::shared_ptr<TcpAdapter> session = std::make_shared<AsioTcpSesssion>(std::move(socket));
                /* 设置会话的回调函数 */
                session->set_callback(m_callback);
                /* 启动会话 */
                session->start();
                /* 将新会话添加到会话管理器中 */
                m_sessions[session.get()] = session;
                /* 调用回调函数通知新连接 */
                m_callback.on_connect(session.get());
                /* 继续接受新的连接 */
                accept();
            }
            else
            {
                /* 错误处理 */
                std::cout << "accept error: " << error.message() << std::endl;
            }
        });
}