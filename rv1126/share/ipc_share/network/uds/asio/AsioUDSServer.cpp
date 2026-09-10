/**
 * @FilePath     : AsioUDSServer.cpp
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2025-01-06
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-31 11:15:09
 * @Description  : UDS服务端类
 */

#include "AsioUDSServer.h"
#include "AsioUDSSesssion.h"

#include <future>

AsioUDSServer::AsioUDSServer(int nPort)
    : m_acceptor(m_ioContext)
{
    try
    {
        std::string path = Net::UDS_PATH;
        path += '/' + std::to_string(nPort);
        /* 创建一个新的UDS acceptor */
        asio::local::stream_protocol::endpoint endpoint(path.c_str());
        std::remove(path.c_str());
        /* 打开 acceptor，使用指定的协议 */
        m_acceptor.open(endpoint.protocol());
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
}
AsioUDSServer::~AsioUDSServer()
{
    stop();
}
/**
 * @brief 启动UDS服务器，开始接受连接
 * @return 返回0表示成功
 */
int AsioUDSServer::start()
{
    if (!m_running.exchange(true))
    {
        accept();
        m_ioThread = std::thread(
            [this]()
            {
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
            });
    }
    return 0;
}

/**
 * @brief 停止UDS服务器，取消所有连接并关闭
 * @return 返回0表示成功
 */
int AsioUDSServer::stop()
{
    if (m_running.exchange(false))
    {
        try
        {
            m_acceptor.cancel();
            m_acceptor.close();
            m_ioContext.stop();

            /* 等待IO线程结束（带超时） */
            if (m_ioThread.joinable())
            {
                auto timeout = std::chrono::milliseconds(2000);
                auto future = std::async(std::launch::async,
                                         [&]()
                                         {
                                             m_ioThread.join();
                                         });

                if (future.wait_for(timeout) == std::future_status::timeout)
                {
                    dlog_error("服务器IO线程超时未退出");
                }
            }
        }
        catch (const asio::system_error &e)
        {
            dlog_error("Asio system error: %s (Error code: %d)", e.what(), e.code().value());
            return -e.code().value();
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
    }
    return 0;
}

void AsioUDSServer::stop_session(void *pHandle)
{
    if (pHandle == nullptr)
    {
        dlog_error("pHandle  is nullptr");   
        return;
    }
    auto session = (UDSAdapter *)pHandle;
    auto it = m_sessions.find(session);
    if (it == m_sessions.end())
    {
        return;
    }
    session->stop();
    m_sessions.erase(it);
    m_callback.on_close(pHandle);
}

/**
 * @brief 接收数据的函数
 *
 * @param pData 指向接收数据的缓冲区
 * @param nDataLen 接收数据的长度
 * @return 返回接收结果
 */
int AsioUDSServer::receive(void *pData, int nDataLen)
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
int AsioUDSServer::send(const void *pData, int nDataLen, void *pHandle)
{
    if (pHandle == nullptr)
    {
        dlog_error("pHandle  is nullptr");
        return -1;
    }
    auto session = static_cast<UDSAdapter *>(pHandle);
    auto it = m_sessions.find(session);
    if (it == m_sessions.end())
    {
        return -1;
    }
    return session->send(pData, nDataLen);
}

bool AsioUDSServer::is_connected()
{
    return m_sessions.size() > 0;
}

/**
 * @brief 异步接受新的UDS连接
 */
void AsioUDSServer::accept()
{
    if (!m_running.load()) {
        return;
    }

    m_acceptor.async_accept(
        [this](const asio::error_code &error, asio::local::stream_protocol::socket socket)
        {
            if (!m_running.load())
            {
                return;
            }
            if (!error)
            {
                /* 创建一个新的UDSAdapter会话对象，使用传入的socket进行初始化 */
                std::shared_ptr<UDSAdapter> session = std::make_shared<AsioUDSSesssion>(std::move(socket));
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