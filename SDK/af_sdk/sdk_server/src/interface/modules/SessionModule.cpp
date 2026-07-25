/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : SessionModule.cpp
 * @Description  : 会话和鉴权管理模块实现，负责HTTP鉴权信息设置和会话管理
 */

#include "SessionModule.h"
#include "SessionManager.h"
#include "HttpAuthHandler.h"
#include "NetSdkLog.h"

/**
 * 构造函数
 */
SessionModule::SessionModule()
    : m_strRealm("")
    , m_strUsername("")
    , m_strPassword("")
    , m_bInitialized(false)
{
    NSDK_LOG_DEBUG("SessionModule created");
}

/**
 * 析构函数
 * @details 自动调用Cleanup()清理会话和鉴权资源
 */
SessionModule::~SessionModule()
{
    Cleanup();
    NSDK_LOG_DEBUG("SessionModule destroyed");
}

/**
 * 设置HTTP鉴权信息
 * @details 初始化HTTP Digest认证域、用户名和密码，配置鉴权处理器
 * @param realm HTTP认证域
 * @param username 用户名
 * @param password 密码
 * @return TRUE表示成功，FALSE表示失败
 */
BOOL SessionModule::SetAuthInfo(const std::string& realm, 
                                const std::string& username, 
                                const std::string& password)
{
    if (realm.empty() || username.empty() || password.empty())
    {
        NSDK_LOG_ERROR("SetAuthInfo: Invalid parameters (empty string)");
        return FALSE;
    }

    m_strRealm = realm;
    m_strUsername = username;
    m_strPassword = password;

    NSDK_LOG_INFO("Setting auth info: realm=%s, user=%s", realm.c_str(), username.c_str());

    CHttpAuthHandler::instance()->set_auth_info(realm.c_str(), username.c_str(), password.c_str());
    
    m_bInitialized = true;
    return TRUE;
}

/**
 * 更新用户密码
 * @details 在已初始化鉴权信息的基础上，更新用户名和密码
 * @param username 用户名
 * @param password 新密码
 * @return TRUE表示成功，FALSE表示失败
 */
BOOL SessionModule::UpdatePassword(const std::string& username, 
                                   const std::string& password)
{
    if (username.empty() || password.empty())
    {
        NSDK_LOG_ERROR("UpdatePassword: Invalid parameters (empty string)");
        return FALSE;
    }

    if (!m_bInitialized)
    {
        NSDK_LOG_ERROR("UpdatePassword: Auth not initialized");
        return FALSE;
    }

    m_strUsername = username;
    m_strPassword = password;

    NSDK_LOG_INFO("Updating password for user: %s", username.c_str());

    CHttpAuthHandler::instance()->set_auth_info(m_strRealm.c_str(), username.c_str(), password.c_str());
    
    return TRUE;
}

/**
 * 获取活跃会话数量
 * @details 查询会话管理器中当前活跃的HTTP会话数量
 * @return 当前活跃的会话数
 */
size_t SessionModule::GetActiveSessionCount() const
{
    if (!m_bInitialized)
    {
        return 0;
    }

    CSessionManager* pSessionMgr = CSessionManager::instance();
    if (!pSessionMgr)
    {
        return 0;
    }

    return pSessionMgr->GetSessionCount();
}

/**
 * 清理所有会话和鉴权资源
 * @details 销毁会话管理器和鉴权处理器实例，重置初始化标志和鉴权信息
 */
void SessionModule::Cleanup()
{
    if (!m_bInitialized)
    {
        return;
    }

    NSDK_LOG_INFO("Cleaning up SessionModule...");

    // 清理会话管理器
    if (CSessionManager::instance())
    {
        CSessionManager::DestroyInstance();
        NSDK_LOG_DEBUG("SessionManager destroyed");
    }

    // 清理鉴权处理器
    if (CHttpAuthHandler::instance())
    {
        CHttpAuthHandler::DestroyInstance();
        NSDK_LOG_DEBUG("HttpAuthHandler destroyed");
    }

    m_bInitialized = false;
    m_strRealm.clear();
    m_strUsername.clear();
    m_strPassword.clear();
    
    NSDK_LOG_INFO("SessionModule cleanup completed");
}
