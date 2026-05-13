#pragma once
#include "NetTVSDKServerInterface.h"

// 前向声明
class CSdkHttpServer;

/**
 * @brief HTTP服务器管理模块
 * @details 负责HTTP服务器的启动、停止和状态管理
 */
class ServerModule
{
public:
    ServerModule();
    ~ServerModule();

    // 禁止拷贝
    ServerModule(const ServerModule&) = delete;
    ServerModule& operator=(const ServerModule&) = delete;

    /**
     * @brief 启动HTTP服务器
     * @param dwPort 服务器端口号
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL Start(UINT32 dwPort);

    /**
     * @brief 停止HTTP服务器
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL Stop();

    /**
     * @brief 检查服务器是否正在运行
     * @return TRUE表示运行中，FALSE表示已停止
     */
    BOOL IsRunning() const;

    /**
     * @brief 获取当前服务器端口
     * @return 端口号，0表示未启动
     */
    UINT32 GetPort() const;

private:
    UINT32 m_dwPort;      // 服务器端口
    bool m_bRunning;      // 运行状态标志
};
