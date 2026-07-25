#pragma once
#include <stddef.h>
#include "NetTVSDKServerInterface.h"

// 前向声明
class CRouteRegistry;

/**
 * @brief 路由注册管理模块
 * @details 负责HTTP路由的注册和业务单例的生命周期管理
 */
class RouteModule
{
public:
    RouteModule();
    ~RouteModule();

    // 禁止拷贝
    RouteModule(const RouteModule&) = delete;
    RouteModule& operator=(const RouteModule&) = delete;

    /**
     * @brief 注册所有业务路由
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL RegisterAllRoutes();

    /**
     * @brief 清理所有路由和业务单例
     */
    void ClearRoutes();

    /**
     * @brief 获取已注册的路由数量
     * @return 路由数量
     */
    size_t GetRouteCount() const;

private:
    /**
     * @brief 注册设备相关路由
     */
    void RegisterDeviceRoutes();

    /**
     * @brief 注册能力集相关路由
     */
    void RegisterCapabilityRoutes();

    /**
     * @brief 注册配置相关路由
     */
    void RegisterConfigRoutes();

    /**
     * @brief 注册设备控制相关路由
     */
    void RegisterDeviceControlRoutes();

    /**
     * @brief 注册视频相关路由
     */
    void RegisterVideoRoutes();

    /**
     * @brief 注册升级相关路由
     */
    void RegisterUpgradeRoutes();

    size_t m_routeCount;  // 已注册路由计数
};
