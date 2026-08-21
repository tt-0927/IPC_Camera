/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : ServerModule.cpp
 * @Description  : HTTP服务器管理模块实现，负责HTTP服务器的启动、停止和状态管理
 */

#include "ServerModule.h"
#include "SdkHttpServer.h"
#include "NetSdkLog.h"

/**
 * 构造函数
 */
ServerModule::ServerModule()
    : m_dwPort(0)
    , m_bRunning(false)
{
    NSDK_LOG_DEBUG("ServerModule created");
}

/**
 * 析构函数
 * @details 自动调用Stop()停止服务器并释放资源
 */
ServerModule::~ServerModule()
{
    if (m_bRunning)
    {
        Stop();
    }
    NSDK_LOG_DEBUG("ServerModule destroyed");
}

/**
 * 启动HTTP服务器
 * @details 创建HTTP服务器实例，绑定指定端口，开始监听请求
 * @param dwPort 服务器端口号
 * @return TRUE表示成功，FALSE表示失败
 */
BOOL ServerModule::Start(UINT32 dwPort)
{
    if (m_bRunning)
    {
        NSDK_LOG_WARN("HTTP Server already running on port %u", m_dwPort);
        return FALSE;
    }

    NSDK_LOG_INFO("Starting HTTP Server on port %u...", dwPort);

    if (CSdkHttpServer::instance()->startServer(static_cast<uint32_t>(dwPort)) != 0)
    {
        NSDK_LOG_ERROR("Failed to start HTTP server on port %u", dwPort);
        return FALSE;
    }

    m_dwPort = dwPort;
    m_bRunning = true;
    NSDK_LOG_INFO("HTTP Server started successfully on port %u", dwPort);
    return TRUE;
}

/**
 * 停止HTTP服务器
 * @details 停止HTTP服务器监听，销毁服务器实例
 * @return TRUE表示成功，FALSE表示失败
 */
BOOL ServerModule::Stop()
{
    if (!m_bRunning)
    {
        NSDK_LOG_DEBUG("HTTP Server not running, skip stop");
        return TRUE;
    }

    NSDK_LOG_INFO("Stopping HTTP Server on port %u...", m_dwPort);

    CSdkHttpServer::instance()->StopServer();
    CSdkHttpServer::DestroyInstance();

    m_bRunning = false;
    m_dwPort = 0;
    NSDK_LOG_INFO("HTTP Server stopped successfully");
    return TRUE;
}

/**
 * 检查服务器是否正在运行
 * @return TRUE表示运行中，FALSE表示已停止
 */
BOOL ServerModule::IsRunning() const
{
    return m_bRunning;
}

/**
 * 获取当前服务器端口
 * @return 端口号，0表示未启动
 */
UINT32 ServerModule::GetPort() const
{
    return m_dwPort;
}
