#include "ServerModule.h"
#include "SdkHttpServer.h"
#include "NetSdkLog.h"

ServerModule::ServerModule()
    : m_dwPort(0)
    , m_bRunning(false)
{
    NSDK_LOG_DEBUG("ServerModule created");
}

ServerModule::~ServerModule()
{
    if (m_bRunning)
    {
        Stop();
    }
    NSDK_LOG_DEBUG("ServerModule destroyed");
}

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

BOOL ServerModule::IsRunning() const
{
    return m_bRunning;
}

UINT32 ServerModule::GetPort() const
{
    return m_dwPort;
}
