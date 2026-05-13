#pragma once
#include "NetTVSDKServerInterface.h"
#include <string>

// 前向声明
class CSessionManager;

/**
 * @brief 会话和鉴权管理模块
 * @details 负责HTTP鉴权信息设置和会话管理
 */
class SessionModule
{
public:
    SessionModule();
    ~SessionModule();

    // 禁止拷贝
    SessionModule(const SessionModule&) = delete;
    SessionModule& operator=(const SessionModule&) = delete;

    /**
     * @brief 设置HTTP鉴权信息
     * @param realm HTTP认证域
     * @param username 用户名
     * @param password 密码
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL SetAuthInfo(const std::string& realm, 
                    const std::string& username, 
                    const std::string& password);

    /**
     * @brief 更新用户密码
     * @param username 用户名
     * @param password 新密码
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL UpdatePassword(const std::string& username, 
                       const std::string& password);

    /**
     * @brief 获取活跃会话数量
     * @return 当前活跃的会话数
     */
    size_t GetActiveSessionCount() const;

    /**
     * @brief 清理所有会话和鉴权资源
     */
    void Cleanup();

private:
    std::string m_strRealm;      // 认证域
    std::string m_strUsername;   // 用户名
    std::string m_strPassword;   // 密码
    bool m_bInitialized;         // 初始化标志
};
