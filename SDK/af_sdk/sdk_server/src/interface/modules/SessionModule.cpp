/**
 * @file SessionModule.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CSessionModule 模块实现
 * 功能说明：
 * 1. 实现 CSessionModule 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : CSessionModule.cpp
 * @Description  : 会话和鉴权管理模块实现，负责HTTP鉴权信息设置和会话管理
 */

#include "SessionModule.h"
#include "SessionManager.h"
#include "HttpAuthHandler.h"
#include "NetSdkLog.h"

/**
 * 构造函数
 */
CSessionModule::CSessionModule()
    : m_strRealm("")
    , m_strUsername("")
    , m_strPassword("")
    , m_bInitialized(false)
{
    NETSDK_LOG_MESSAGE_DEBUG("CSessionModule created");
}

/**
 * 析构函数
 * @details 自动调用Cleanup()清理会话和鉴权资源
 */
CSessionModule::~CSessionModule()
{
    Cleanup();
    NETSDK_LOG_MESSAGE_DEBUG("CSessionModule destroyed");
}

/**
 * 设置HTTP鉴权信息
 * @details 初始化HTTP Digest认证域、用户名和密码，配置鉴权处理器
 * @param realm HTTP认证域
 * @param username 用户名
 * @param password 密码
 * @return TRUE表示成功，FALSE表示失败
 */
BOOL CSessionModule::SetAuthInfo(const std::string& realm,
                                const std::string& username,
                                const std::string& password)
{
    if (realm.empty() || username.empty() || password.empty())
    {
        NETSDK_LOG_MESSAGE_ERROR("SetAuthInfo: Invalid parameters (empty string)");
        return NET_TV_FALSE;
    }

    m_strRealm = realm;
    m_strUsername = username;
    m_strPassword = password;

    NETSDK_LOG_MESSAGE_INFO("Setting auth info: realm=%s, user=%s", realm.c_str(), username.c_str());

    CHttpAuthHandler::instance()->set_auth_info(realm.c_str(), username.c_str(), password.c_str());

    m_bInitialized = true;
    return NET_TV_TRUE;
}

/**
 * 更新用户密码
 * @details 在已初始化鉴权信息的基础上，更新用户名和密码
 * @param username 用户名
 * @param password 新密码
 * @return TRUE表示成功，FALSE表示失败
 */
BOOL CSessionModule::UpdatePassword(const std::string& username,
                                   const std::string& password)
{
    if (username.empty() || password.empty())
    {
        NETSDK_LOG_MESSAGE_ERROR("UpdatePassword: Invalid parameters (empty string)");
        return NET_TV_FALSE;
    }

    if (!m_bInitialized)
    {
        NETSDK_LOG_MESSAGE_ERROR("UpdatePassword: Auth not initialized");
        return NET_TV_FALSE;
    }

    m_strUsername = username;
    m_strPassword = password;

    NETSDK_LOG_MESSAGE_INFO("Updating password for user: %s", username.c_str());

    CHttpAuthHandler::instance()->set_auth_info(m_strRealm.c_str(), username.c_str(), password.c_str());

    return NET_TV_TRUE;
}

/**
 * 获取活跃会话数量
 * @details 查询会话管理器中当前活跃的HTTP会话数量
 * @return 当前活跃的会话数
 */
size_t CSessionModule::GetActiveSessionCount() const
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
void CSessionModule::Cleanup()
{
    if (!m_bInitialized)
    {
        return;
    }

    NETSDK_LOG_MESSAGE_INFO("Cleaning up CSessionModule...");

    /* 清理会话管理器 */
    if (CSessionManager::instance())
    {
        CSessionManager::DestroyInstance();
        NETSDK_LOG_MESSAGE_DEBUG("SessionManager destroyed");
    }

    /* 清理鉴权处理器 */
    if (CHttpAuthHandler::instance())
    {
        CHttpAuthHandler::DestroyInstance();
        NETSDK_LOG_MESSAGE_DEBUG("HttpAuthHandler destroyed");
    }

    m_bInitialized = false;
    m_strRealm.clear();
    m_strUsername.clear();
    m_strPassword.clear();

    NETSDK_LOG_MESSAGE_INFO("CSessionModule cleanup completed");
}
