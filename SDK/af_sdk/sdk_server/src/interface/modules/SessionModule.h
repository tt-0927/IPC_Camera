/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : SessionModule.h
 * @Description  : 会话和鉴权管理模块，负责HTTP鉴权信息设置和会话管理
 */

#pragma once
#include "NetTVSDKServerInterface.h"
#include <string>

// 前向声明
class CSessionManager;

/**
 * 会话和鉴权管理模块类
 * 负责HTTP鉴权信息设置和会话管理
 */
class SessionModule
{
public:
    /**
     * 构造函数
     */
    SessionModule();

    /**
     * 析构函数
     * @details 自动调用Cleanup()清理会话和鉴权资源
     */
    ~SessionModule();

    // 禁止拷贝
    SessionModule(const SessionModule&) = delete;
    SessionModule& operator=(const SessionModule&) = delete;

    /**
     * 设置HTTP鉴权信息
     * @details 初始化HTTP Digest认证域、用户名和密码，配置鉴权处理器
     * @param realm HTTP认证域
     * @param username 用户名
     * @param password 密码
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL SetAuthInfo(const std::string& realm, 
                    const std::string& username, 
                    const std::string& password);

    /**
     * 更新用户密码
     * @details 在已初始化鉴权信息的基础上，更新用户名和密码
     * @param username 用户名
     * @param password 新密码
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL UpdatePassword(const std::string& username, 
                       const std::string& password);

    /**
     * 获取活跃会话数量
     * @details 查询会话管理器中当前活跃的HTTP会话数量
     * @return 当前活跃的会话数
     */
    size_t GetActiveSessionCount() const;

    /**
     * 清理所有会话和鉴权资源
     * @details 销毁会话管理器和鉴权处理器实例，重置初始化标志和鉴权信息
     */
    void Cleanup();

private:
    std::string m_strRealm; /* 认证域 */
    std::string m_strUsername; /* 用户名 */
    std::string m_strPassword; /* 密码 */
    bool m_bInitialized; /* 初始化标志 */
};
