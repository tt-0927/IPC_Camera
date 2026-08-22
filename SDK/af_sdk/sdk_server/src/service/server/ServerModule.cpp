/**
 * @file CServerModule.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CServerModule 模块实现
 * 功能说明：
 * 1. 实现 CServerModule 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : CServerModule.cpp
 * @Description  : HTTP服务器管理模块实现，负责HTTP服务器的启动、停止和状态管理
 */

#include "ServerModule.h"
#include "SdkHttpServer.h"
#include "NetSdkLog.h"

/**
 * 构造函数
 */
CServerModule::CServerModule()
    : m_uPort(0)
    , m_bRunning(false)
{
    NETSDK_LOG_MESSAGE_DEBUG("CServerModule created");
}

/**
 * 析构函数
 * @details 自动调用Stop()停止服务器并释放资源
 */
CServerModule::~CServerModule()
{
    if (m_bRunning)
    {
        Stop();
    }
    NETSDK_LOG_MESSAGE_DEBUG("CServerModule destroyed");
}

/**
 * 启动HTTP服务器
 * @details 创建HTTP服务器实例，绑定指定端口，开始监听请求
 * @param dwPort 服务器端口号
 * @return TRUE表示成功，FALSE表示失败
 */
BOOL CServerModule::Start(UINT32 dwPort)
{
    if (m_bRunning)
    {
        NETSDK_LOG_MESSAGE_WARN("HTTP Server already running on port %u", m_uPort);
        return NET_FALSE;
    }

    NETSDK_LOG_MESSAGE_INFO("Starting HTTP Server on port %u...", dwPort);

    if (CSdkHttpServer::instance()->startServer(static_cast<uint32_t>(dwPort)) != 0)
    {
        NETSDK_LOG_MESSAGE_ERROR("Failed to start HTTP server on port %u", dwPort);
        return NET_FALSE;
    }

    m_uPort = dwPort;
    m_bRunning = true;
    NETSDK_LOG_MESSAGE_INFO("HTTP Server started successfully on port %u", dwPort);
    return NET_TRUE;
}

/**
 * 停止HTTP服务器
 * @details 停止HTTP服务器监听，销毁服务器实例
 * @return TRUE表示成功，FALSE表示失败
 */
BOOL CServerModule::Stop()
{
    if (!m_bRunning)
    {
        NETSDK_LOG_MESSAGE_DEBUG("HTTP Server not running, skip stop");
        return NET_TRUE;
    }

    NETSDK_LOG_MESSAGE_INFO("Stopping HTTP Server on port %u...", m_uPort);

    CSdkHttpServer::instance()->StopServer();
    CSdkHttpServer::DestroyInstance();

    m_bRunning = false;
    m_uPort = 0;
    NETSDK_LOG_MESSAGE_INFO("HTTP Server stopped successfully");
    return NET_TRUE;
}

/**
 * 检查服务器是否正在运行
 * @return TRUE表示运行中，FALSE表示已停止
 */
BOOL CServerModule::IsRunning() const
{
    return m_bRunning;
}

/**
 * 获取当前服务器端口
 * @return 端口号，0表示未启动
 */
UINT32 CServerModule::GetPort() const
{
    return m_uPort;
}
