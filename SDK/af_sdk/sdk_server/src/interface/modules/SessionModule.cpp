#include "SessionModule.h"
#include "SessionManager.h"
#include "HttpAuthHandler.h"
#include "NetSdkLog.h"

SessionModule::SessionModule()
    : m_strRealm("")
    , m_strUsername("")
    , m_strPassword("")
    , m_bInitialized(false)
{
    NSDK_LOG_DEBUG("SessionModule created");
}

SessionModule::~SessionModule()
{
    Cleanup();
    NSDK_LOG_DEBUG("SessionModule destroyed");
}

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
