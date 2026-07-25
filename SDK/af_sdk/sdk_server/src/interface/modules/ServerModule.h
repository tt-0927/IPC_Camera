/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : ServerModule.h
 * @Description  : HTTP服务器管理模块，负责HTTP服务器的启动、停止和状态管理
 */

#pragma once
#include "NetTVSDKServerInterface.h"

// 前向声明
class CSdkHttpServer;

/**
 * HTTP服务器管理模块类
 * 负责HTTP服务器的启动、停止和状态管理
 */
class ServerModule
{
public:
    /**
     * 构造函数
     */
    ServerModule();

    /**
     * 析构函数
     * @details 自动调用Stop()停止服务器并释放资源
     */
    ~ServerModule();

    // 禁止拷贝
    ServerModule(const ServerModule&) = delete;
    ServerModule& operator=(const ServerModule&) = delete;

    /**
     * 启动HTTP服务器
     * @details 创建HTTP服务器实例，绑定指定端口，开始监听请求
     * @param dwPort 服务器端口号
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL Start(UINT32 dwPort);

    /**
     * 停止HTTP服务器
     * @details 停止HTTP服务器监听，销毁服务器实例
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL Stop();

    /**
     * 检查服务器是否正在运行
     * @return TRUE表示运行中，FALSE表示已停止
     */
    BOOL IsRunning() const;

    /**
     * 获取当前服务器端口
     * @return 端口号，0表示未启动
     */
    UINT32 GetPort() const;

private:
    UINT32 m_dwPort; /* 服务器端口 */
    bool m_bRunning; /* 运行状态标志 */
};
