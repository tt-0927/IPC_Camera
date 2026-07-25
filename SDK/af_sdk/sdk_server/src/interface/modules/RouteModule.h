/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : RouteModule.h
 * @Description  : 路由注册管理模块，负责HTTP路由的注册和业务单例的生命周期管理
 */

#pragma once
#include <stddef.h>
#include "NetTVSDKServerInterface.h"

// 前向声明
class CRouteRegistry;

/**
 * 路由注册管理模块类
 * 负责HTTP路由的注册和业务单例的生命周期管理
 */
class RouteModule
{
public:
    /**
     * 构造函数
     */
    RouteModule();

    /**
     * 析构函数
     * @details 自动调用ClearRoutes()清理路由和业务单例
     */
    ~RouteModule();

    // 禁止拷贝
    RouteModule(const RouteModule&) = delete;
    RouteModule& operator=(const RouteModule&) = delete;

    /**
     * 注册所有业务路由
     * @details 依次注册设备、能力集、配置、设备控制、视频、升级相关路由
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL RegisterAllRoutes();

    /**
     * 清理所有路由和业务单例
     * @details 清除路由注册表，销毁所有业务单例实例
     */
    void ClearRoutes();

    /**
     * 获取已注册的路由数量
     * @return 路由数量
     */
    size_t GetRouteCount() const;

private:
    /**
     * 注册设备相关路由
     * @details 注册设备信息获取路由
     */
    void RegisterDeviceRoutes();

    /**
     * 注册能力集相关路由
     * @details 注册设备能力集获取路由
     */
    void RegisterCapabilityRoutes();

    /**
     * 注册配置相关路由
     * @details 注册设备配置获取和设置路由
     */
    void RegisterConfigRoutes();

    /**
     * 注册设备控制相关路由
     * @details 注册设备控制路由
     */
    void RegisterDeviceControlRoutes();

    /**
     * 注册视频相关路由
     * @details 注册录像回放URL获取、回放控制、录像列表获取、录像帧流启动和停止路由
     */
    void RegisterVideoRoutes();

    /**
     * 注册升级相关路由
     * @details 注册固件上传路由（PUT方法，直接处理二进制数据）
     */
    void RegisterUpgradeRoutes();

    size_t m_routeCount; /* 已注册路由计数 */
};
