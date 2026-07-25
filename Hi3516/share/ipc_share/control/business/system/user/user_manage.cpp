/***
 * @FilePath     : user_manage.cpp
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-11-14 10:22:30
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-28 10:23:37
 * @Description  :
 */

#include "user_manage.h"

#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>
#include <algorithm>

#include "task_publish.h"
#include "user_convert.h"
#include "convert_interface.h"
#include "action_code.h"
#include "path_define.h"
#include "user_database.h"
#include "time_manage.h"
#include "web_server.h"
#include "upnp_manage.h"
#include "rtsp_server.h"

using namespace Db;

/**
 * @brief 初始化
 */
IpcRet_E CUserManage::init()
{
    // 获取所有用户
    std::vector<User::UserInfo_S> userInfos;
    CUserDatabase::instance()->find(MatchMethods(), userInfos);
    
    int updatedCount = 0;
    for (const auto &userInfo : userInfos)
    {
        Item item;
        item.push_back(Element(USER_FIELD_ACCOUNT_STATUS, User::AccountStatus_E::ACCOUNT_STATUS_NORMAL));

        MatchMethods methods;
        methods.push_back(MatchMethod(Element(USER_FIELD_ACCOUNT, userInfo.stAccountInfo.account), FIND_CRITERION_EQ));

        if (CUserDatabase::instance()->update(item, methods) == 0)
        {
            updatedCount++;
        }

        /* 检索admin管理员账户密码，更新至RtspServer */
        if (userInfo.stAccountInfo.account == USER_DEFAULT_NAME)
        {
            CRtspServer::instance()->update_userInfo(userInfo.stAccountInfo.account,
                                                     userInfo.stAccountInfo.password,
                                                     true);
        }
    }

    dlog_info("%d个用户账号状态已重置为正常状态", updatedCount);

    /* 自动添加隐藏的itc产测账号 */
    std::vector<User::UserInfo_S> itcUsers;
    CUserDatabase::instance()->find(Element(USER_FIELD_ACCOUNT, "itc"), itcUsers);
    if (itcUsers.empty())
    {
        User::UserInfo_S stItcUser;
        stItcUser.stAccountInfo.account = "itc";
        stItcUser.stAccountInfo.password = "itc@1993";
        stItcUser.stAccountInfo.nAccountType = User::ACCOUNT_TYPE_ADMIN;
        stItcUser.stAccountInfo.nSafety = User::Safety_E::SKYHIGH_LEVEL;
        stItcUser.stBindInfo.name = "ITC";
        stItcUser.stPermissions.stMenuPermission.bPreview = true;
        stItcUser.stPermissions.stMenuPermission.bPlayback = true;
        stItcUser.stPermissions.stMenuPermission.bRetrieve = true;
        stItcUser.stPermissions.stMenuPermission.bAdhibition = true;
        stItcUser.stPermissions.stMenuPermission.bWebLocalConfig = true;
        stItcUser.stPermissions.stMenuPermission.bSystemConfig = true;
        stItcUser.stPermissions.stMenuPermission.bNetworkConfig = true;
        stItcUser.stPermissions.stMenuPermission.bChannelManage = true;
        stItcUser.stPermissions.stMenuPermission.bVideoAndAudio = true;
        stItcUser.stPermissions.stMenuPermission.bEventConfig = true;
        stItcUser.stPermissions.stMenuPermission.bVideoManage = true;
        stItcUser.stPermissions.stMenuPermission.bObjectLib = true;
        stItcUser.stPermissions.stMenuPermission.bVehicleDetecConfig = true;
        stItcUser.stPermissions.stMenuPermission.bLocalShutdown = true;
        stItcUser.stPermissions.stOperatePermission.bPTZControl = true;
        stItcUser.stPermissions.stOperatePermission.bRound = true;
        stItcUser.stPermissions.stOperatePermission.bTalk = true;
        stItcUser.stPermissions.stOperatePermission.bRecord = true;
        stItcUser.stPermissions.stOperatePermission.bRestart = true;
        stItcUser.stPermissions.stOperatePermission.bSimpleRecovery = true;
        stItcUser.stPermissions.stOperatePermission.bFullRecovery = true;
        stItcUser.stPermissions.stOperatePermission.bParameterDerivation = true;
        stItcUser.stPermissions.stOperatePermission.bUpgrade = true;
        stItcUser.stPermissions.stOperatePermission.bActionAlarm = true;
        CUserDatabase::instance()->add(stItcUser);
        dlog_info("隐藏账号 itc 已自动添加");
    }
    
    return OK;
}

/**
 * @brief 反初始化
 */
IpcRet_E CUserManage::deinit()
{
    return OK;
}

void CUserManage::set_pending_password_update_time(const std::string &strAccount, const std::string &strNowTime)
{
    if (strAccount.empty() || strNowTime.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    mPendingPasswordUpdateTime[strAccount] = strNowTime;
}

std::string CUserManage::get_pending_password_update_time(const std::string &strAccount)
{
    if (strAccount.empty()) {
        return "";
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = mPendingPasswordUpdateTime.find(strAccount);
    if (it == mPendingPasswordUpdateTime.end()) {
        return "";
    }

    return it->second;
}

void CUserManage::clear_pending_password_update_time(const std::string &strAccount)
{
    if (strAccount.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    mPendingPasswordUpdateTime.erase(strAccount);
}

std::string CUserManage::get_password_update_time(const User::UpdateInfo_S &stUserUpdateInfo)
{
    /* 根层 Data.NowTime，对应旧的 1003 请求时间字段 */
    const std::string &strRootNowTime = stUserUpdateInfo.stAccountInfo.strNowTime;

    /* Update.NowTime，对应前端把时间放在 Update 子对象中的新结构 */
    const std::string &strUpdateNowTime = stUserUpdateInfo.stNewUserInfo.stAccountInfo.strNowTime;

    /* 登录阶段缓存的业务时间，用于首次登录强制改密、密码过期后的后续改密 */
    const std::string strCachedNowTime = get_pending_password_update_time(stUserUpdateInfo.stAccountInfo.account);

    if (!strRootNowTime.empty()) {
        return strRootNowTime;
    }

    if (!strUpdateNowTime.empty()) {
        return strUpdateNowTime;
    }

    if (!strCachedNowTime.empty()) {
        return strCachedNowTime;
    }

    /* 当前设备时间，作为所有业务时间都缺失时的最后兜底 */
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

/**
 * @brief 用户登录
 */
int CUserManage::login(User::AccountInfo_S stAccountInfo, User::UserInfo_S &stUserInfo, ::System::SecurityServices_S stSecurityServicesInfo)
{
    std::string strClientIp = CWebServer::instance()->get_loginclient_ip();
    if (strClientIp.empty()) strClientIp = UNKNOWN_USER;

    std::string strNowTime = stAccountInfo.strNowTime;
    
    // 检查锁定状态 
    // 返回值: 0-正常, 1-已锁定且未过期, 2-已锁定但已过期(需重置)
    auto checkLockStatus = [&](User::LogErrorInfo_S &errorInfo) -> int {
        // 是否在检测周期(nCheckInterval)外，是，重置计数
        if (errorInfo.nErrorCount > 0 && !errorInfo.strFirstRecordTime.empty() && errorInfo.strReleaseTime.empty()) {
            std::tm tmFirst = {};
            std::istringstream ss(errorInfo.strFirstRecordTime);
            ss >> std::get_time(&tmFirst, "%Y-%m-%d %H:%M:%S"); 
            auto tpFirst = std::chrono::system_clock::from_time_t(std::mktime(&tmFirst));
            
            std::tm tmNow = {};
            std::istringstream sn(strNowTime);
            sn >> std::get_time(&tmNow, "%Y-%m-%d %H:%M:%S");
            auto tpNow = std::chrono::system_clock::from_time_t(std::mktime(&tmNow));

            auto minutes = std::chrono::duration_cast<std::chrono::minutes>(tpNow - tpFirst).count();
            if (minutes >= stSecurityServicesInfo.stLoginLock.nCheckInterval && stSecurityServicesInfo.stLoginLock.nCheckInterval > 0) {
                // 超过检测周期，重置错误信息
                errorInfo.nErrorCount = 0;
                errorInfo.strFirstRecordTime = "";
                return 0; 
            }
        }

        // 是否已经被锁定
        if (errorInfo.nErrorCount >= stSecurityServicesInfo.stLoginLock.nMaxErrorTimes && !errorInfo.strReleaseTime.empty()) {
            if (strNowTime >= errorInfo.strReleaseTime) {
                // 解锁时间已到
                errorInfo.nErrorCount = 0;
                errorInfo.strReleaseTime = "";
                errorInfo.strFirstRecordTime = "";
                return 2; // 解锁
            } else {
                return 1; // 锁定中
            }
        }
        return 0; // 未锁定
    };

    // 记录失败 
    auto recordFailure = [&](std::string strKey) {
        auto &errorInfo = mErrorInfo[strKey];
        
        // 如果是该周期的第一次错误，记录时间
        if (errorInfo.nErrorCount == 0 || errorInfo.strFirstRecordTime.empty()) {
            errorInfo.strFirstRecordTime = strNowTime;
            errorInfo.nErrorCount = 0;
        }

        errorInfo.nErrorCount++;
        dlog_error("[%s] 错误次数: %d", strKey.c_str(), errorInfo.nErrorCount);

        // 达到最大错误次数，进行锁定
        if (errorInfo.nErrorCount >= stSecurityServicesInfo.stLoginLock.nMaxErrorTimes) {
            
            if (strKey.find(PREFIX_IP) == 0) {
                // IP锁定,含不存在的用户)-> 强制 5 分钟
                errorInfo.strReleaseTime = addErrorTime(strNowTime, ::System::LOCK_DURATION_5_MIN); 
                dlog_error("IP [%s] 触发锁定 (5分钟), 解锁时间: %s", strKey.c_str(), errorInfo.strReleaseTime.c_str());
            } 
            else {
                // 已存在的用户-> 使用配置的时间
                errorInfo.strReleaseTime = addErrorTime(strNowTime, stSecurityServicesInfo.stLoginLock.nLockDuration);
                dlog_error("账号 [%s] 触发锁定 (配置时长), 解锁时间: %s", strKey.c_str(), errorInfo.strReleaseTime.c_str());

                // 用户，更新数据库状态为锁定
                if (strKey.find(PREFIX_USER) == 0) {
                    // 获取真实账号名
                    std::string realAccount = strKey.substr(std::string(PREFIX_USER).length());
                    stUserInfo.stAccountInfo.account = realAccount;
                    update_status(stUserInfo, User::AccountStatus_E::ACCOUNT_STATUS_LOCKED); 
                }
            }

        }
    };

    // 检查 IP 锁定状态 (所有用户，包括不存在的用户)
    std::string strIpKey = PREFIX_IP + strClientIp;
    if (stSecurityServicesInfo.stLoginLock.bIllegalLoginEnable) {
        int ipStatus = checkLockStatus(mErrorInfo[strIpKey]);
        if (ipStatus == 1) {
            dlog_error("IP [%s] 处于锁定状态，拒绝访问", strClientIp.c_str());
            return IpcRet_E::ERR_LOGIN_LOCK;
        } else if (ipStatus == 2) {
            dlog_info("IP [%s] 锁定已自动解除", strClientIp.c_str());
        }
    }

    // 检查 账号 锁定状态 (针对已存在的账号)
    std::string strUserKey = PREFIX_USER + stAccountInfo.account;
    if (stSecurityServicesInfo.stLoginLock.bIllegalLoginEnable) {
        int userStatus = checkLockStatus(mErrorInfo[strUserKey]);
        if (userStatus == 1) {
            dlog_error("账号 [%s] 处于锁定状态，拒绝访问", stAccountInfo.account.c_str());
            return IpcRet_E::ERR_LOGIN_LOCK;
        } else if (userStatus == 2) {
            dlog_info("账号 [%s] 锁定已自动解除", stAccountInfo.account.c_str());
            stUserInfo.stAccountInfo.account = stAccountInfo.account;
            update_status(stUserInfo, User::AccountStatus_E::ACCOUNT_STATUS_NORMAL);
        }
    }

    std::vector<User::UserInfo_S> userInfos;
    CUserDatabase::instance()->find(Element(USER_FIELD_ACCOUNT, stAccountInfo.account), userInfos);
    if (userInfos.empty()) {
        dlog_error("用户不存在: %s", stAccountInfo.account.c_str());
        if (stSecurityServicesInfo.stLoginLock.bIllegalLoginEnable) {
            // 用户不存在时，记录为 IP 错误
            recordFailure(strIpKey);
        }
        return IpcRet_E::ERR_USER_NOT_EXIST;
    }

    if (stAccountInfo.password != userInfos[0].stAccountInfo.password) {
        dlog_error("密码错误: User[%s]", stAccountInfo.account.c_str());
        
        if (stSecurityServicesInfo.stLoginLock.bIllegalLoginEnable) {
            // 密码错误时， 账号错误
            recordFailure(strUserKey);
        }
        return IpcRet_E::ERR_PASSWORD_WRONG;
    }

    ::Network::PortMapConfig_S stPortMapConfig;
    CUpnpManage::instance()->get_port_map(stPortMapConfig);
    
    int result = check_password(stAccountInfo.password.c_str(), stAccountInfo.account.c_str(), NULL);
    if (result != IpcRet_E::OK) {

        if(!stSecurityServicesInfo.stPwdPolicy.bAllowLowLevelPwdLogin && stPortMapConfig.bEnablePortMap) {
            dlog_info("外网低等级密码账号不允许访问系统!");
            return IpcRet_E::PASS_ERR_LOW_LEVEL_EXTERNAL_ACCESS;
        }
    }

    mErrorInfo.erase(strUserKey);
    // mErrorInfo.erase(strIpKey);

    stUserInfo = userInfos[0];

    /* itc隐藏用户跳过首次强制改密和密码过期校验 */
    if (stUserInfo.stAccountInfo.account != "itc")
    {
        /*识别用户第一次登陆或者密码不符合规范则强制重置密码*/
        if ((result != IpcRet_E::OK && !stSecurityServicesInfo.stPwdPolicy.bPwdSecurityLevelEnable)
            || stUserInfo.stAccountInfo.strFirstLoginTime.empty())
        {
            set_pending_password_update_time(stUserInfo.stAccountInfo.account, strNowTime);
            return IpcRet_E::PASS_ERR_FIRST_LOGIN_PWD_CHANGE;
        }

        /*检测密码是否已过期-90天周期-不强制重置密码*/
        if (!stUserInfo.stAccountInfo.strFirstLoginTime.empty() && !strNowTime.empty()){
        std::tm tmFirst = {};
        std::istringstream ss(stUserInfo.stAccountInfo.strFirstLoginTime);
        ss >> std::get_time(&tmFirst, "%Y-%m-%d %H:%M:%S");
        if (ss.fail()) return OK;

        std::tm tmNow = {};
        std::istringstream sn(strNowTime);
        sn >> std::get_time(&tmNow, "%Y-%m-%d %H:%M:%S");
        if (sn.fail()) return OK;

        auto tpFirst  = std::chrono::system_clock::from_time_t(std::mktime(&tmFirst));
        auto tpNow    = std::chrono::system_clock::from_time_t(std::mktime(&tmNow));

        auto days = std::chrono::duration_cast<std::chrono::hours>(tpNow - tpFirst).count() / 24;
        if(days >= 90){

            if (mPasswordExpiryReminder.find(stUserInfo.stAccountInfo.account) != mPasswordExpiryReminder.end()) {
                int value = mPasswordExpiryReminder[stUserInfo.stAccountInfo.account];
                if(value == false){//第一次点击登录，识别第一次，则提醒
                    mPasswordExpiryReminder[stUserInfo.stAccountInfo.account] = true;
                    set_pending_password_update_time(stUserInfo.stAccountInfo.account, strNowTime);
                    return IpcRet_E::PASS_ERR_EXPIRED;
                }
                 //value为true，第二次点击登录，识别第二次，不提醒；成功后清除记录

            }
            else{
                //识别第一次，则提醒
                mPasswordExpiryReminder[stUserInfo.stAccountInfo.account] = true;
                set_pending_password_update_time(stUserInfo.stAccountInfo.account, strNowTime);
                 return IpcRet_E::PASS_ERR_EXPIRED;
            }
 
        }
    }
    }

    /* 检查同一IP是否已登录 */
    if (check_ip_online(strClientIp))
    {
        if (CWebServer::instance()->hasActiveLoggedInConnectionByIp(strClientIp))
        {
            dlog_error("IP [%s] 已登录，禁止重复登录", strClientIp.c_str());
            return IpcRet_E::ERR_REPEAT_LOGIN_IP;
        }

        dlog_warn("IP [%s] 存在残留在线状态，清理后允许重新登录", strClientIp.c_str());
        cleanup_online_users_by_ip(strClientIp);
        CWebServer::instance()->clearPendingCleanupByIp(strClientIp);
    }

    stUserInfo.stAccountInfo.nLoginCnt++;
    stAccountInfo.stOnlineUser.strIpAddress = strClientIp;

    if (!stAccountInfo.stOnlineUser.strIpAddress.empty()) {
        /* 保护在线用户列表写入 */
        std::lock_guard<std::mutex> lock(m_mutex);
        if (check_ip_online_locked(stAccountInfo.stOnlineUser.strIpAddress))
        {
            dlog_error("IP [%s] 在写入在线记录前再次检测到在线状态", strClientIp.c_str());
            return IpcRet_E::ERR_REPEAT_LOGIN_IP;
        }

        stUserInfo.stAccountInfo.stOnlineUser.strUsername = stUserInfo.stAccountInfo.account;
        stUserInfo.stAccountInfo.stOnlineUser.nUserType = stUserInfo.stAccountInfo.nAccountType;
        stUserInfo.stAccountInfo.stOnlineUser.strLastActionTime = strNowTime;
        stUserInfo.stAccountInfo.stOnlineUser.strIpAddress = stAccountInfo.stOnlineUser.strIpAddress;
        stUserInfo.stAccountInfo.stOnlineUser.nOnlineUserId = generate_online_userId();
        m_vecOnlineUsers.push_back(stUserInfo.stAccountInfo.stOnlineUser);
    }

    Item item;
    item.push_back(Element(USER_FIELD_LOGIN_CNT, stUserInfo.stAccountInfo.nLoginCnt));
    
    stUserInfo.stAccountInfo.nAccountStatus = (int)User::AccountStatus_E::ACCOUNT_STATUS_ONLINE;
    item.push_back(Element(USER_FIELD_ACCOUNT_STATUS, stUserInfo.stAccountInfo.nAccountStatus));

    MatchMethods methods;
    methods.push_back(MatchMethod(Element(USER_FIELD_ACCOUNT, stUserInfo.stAccountInfo.account), FIND_CRITERION_EQ));
    CUserDatabase::instance()->update(item, methods);

    dlog_info("用户登录成功: %s, IP: %s", stAccountInfo.account.c_str(), strClientIp.c_str());

    /*清除密码过期提醒记录*/
    if (mPasswordExpiryReminder.find(stUserInfo.stAccountInfo.account) != mPasswordExpiryReminder.end())
    {
        mPasswordExpiryReminder[stUserInfo.stAccountInfo.account] = false;
    }
    clear_pending_password_update_time(stUserInfo.stAccountInfo.account);

    return OK;
}

/**
 * @brief 添加用户
 */
int CUserManage::add(User::UserInfo_S stUserInfo,::System::SecurityServices_S stSecurityServicesInfo)
{
    std::vector<User::UserInfo_S> userInfos;
    CUserDatabase::instance()->find(Element(USER_FIELD_ACCOUNT, stUserInfo.stAccountInfo.account), userInfos);
    if (userInfos.size() != 0)
    {
        dlog_info("用户已存在");
        return IpcRet_E::ERR_USER_EXIST;
    }


    /* 判断密码是否符合规范 */
    if(!stSecurityServicesInfo.stPwdPolicy.bPwdSecurityLevelEnable)
    {
        int result = check_password(stUserInfo.stAccountInfo.password.c_str(),stUserInfo.stAccountInfo.account.c_str(),NULL);
        if (result != IpcRet_E::OK)
        {
            dlog_info("密码不符合规范!");
            return result;
        }
    }

    // auto now = std::chrono::system_clock::now();
    // std::time_t t = std::chrono::system_clock::to_time_t(now);
    // std::stringstream ss;
    // ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
    // stUserInfo.stAccountInfo.strFirstLoginTime = ss.str();
    int nRet;
    nRet = CUserDatabase::instance()->add(stUserInfo);
    if (nRet < 0)
    {
        return nRet;
    }
    /* 用户添加成功 */
    else
    {
        return 0;
    }
}

/**
 * @brief 删除用户
 */
int CUserManage::del(User::UserInfo_S stUserInfo)
{
    int nRet = 0;
    if (stUserInfo.stAccountInfo.account == "admin"
        || stUserInfo.stAccountInfo.account == "itc")
    {
        dlog_error("无法删除%s用户", stUserInfo.stAccountInfo.account.c_str());
        return -1;
    }

    std::vector<User::UserInfo_S> userInfos;
    CUserDatabase::instance()->find(Element(USER_FIELD_ACCOUNT, stUserInfo.stAccountInfo.account), userInfos);
    if (userInfos.size() == 0)
    {
        dlog_error("用户不存在");
        return IpcRet_E::ERR_USER_NOT_EXIST;
    }

    nRet = del(stUserInfo.stAccountInfo);
    if (nRet == 0)
    {
        dlog_info("删除用户成功:%s", stUserInfo.stAccountInfo.account.c_str());
        /*移除对应用户登录错误信息*/
        std::string strUserKey = std::string(PREFIX_USER) + stUserInfo.stAccountInfo.account;
        mErrorInfo.erase(strUserKey);

        for (auto it = m_vecOnlineUsers.begin(); it != m_vecOnlineUsers.end();)
        {
            const std::string &strUsername = it->strUsername;
            if (strUsername == stUserInfo.stAccountInfo.account)
            {
                dlog_info("发送在线用户退出命令");
                User::DeleUser_S stDeleUser;
                stDeleUser.nId = it->nOnlineUserId;
                stDeleUser.account = stUserInfo.stAccountInfo.account;
                TaskPublish::instance()->message(AC_DEL_AND_EXIT_USER, Convert::to_string(stDeleUser));
                /* 删除当前元素 */
                it = m_vecOnlineUsers.erase(it);
            }
            else
            {
                ++it;
            }
        }

        return 0;
    }
    else
    {
        return nRet;
    }
}

/**
 * @brief 删除用户
 */
int CUserManage::del(User::AccountInfo_S stAccountInfo)
{
    if (stAccountInfo.account == "admin"
        || stAccountInfo.account == "itc")
    {
        dlog_error("无法删除%s用户", stAccountInfo.account.c_str());
        return -1;
    }

    Item item;
    item.push_back(Element(USER_FIELD_ACCOUNT, stAccountInfo.account));
    return CUserDatabase::instance()->del(item);
}

/**
 * @brief 修改用户
 */
int CUserManage::update(User::UpdateInfo_S stUserUpdateInfo, ::System::SecurityServices_S stSecurityServicesInfo)
{
    std::vector<User::UserInfo_S> userInfos;
    CUserDatabase::instance()->find(Element(USER_FIELD_ACCOUNT, stUserUpdateInfo.stAccountInfo.account), userInfos);
    if (userInfos.size() == 0)
    {
        dlog_error("用户不存在");
        return IpcRet_E::ERR_USER_NOT_EXIST;
    }

    if (userInfos[0].stAccountInfo.nAccountType == User::ACCOUNT_TYPE_ADMIN && stUserUpdateInfo.stNewUserInfo.stAccountInfo.nAccountType != User::ACCOUNT_TYPE_ADMIN)
    {
        dlog_error("用户类型是管理员类型 无法修改");
        return -1;
    }

    /* 是否检查新旧密码相同，只修改权限相关无需检查 */
    if(stUserUpdateInfo.bCheckPassword)
    {
        //新旧密码一样
        if(userInfos[0].stAccountInfo.password == stUserUpdateInfo.stNewUserInfo.stAccountInfo.password)
        {
            dlog_info("密码已存在");
            return IpcRet_E::PASS_ERR_EXIST;
        }
    }

    /* 判断要修改的用户名是否存在 */
    if (stUserUpdateInfo.stAccountInfo.account != stUserUpdateInfo.stNewUserInfo.stAccountInfo.account)
    {
        std::vector<User::UserInfo_S> userInfos2;
        CUserDatabase::instance()->find(Element(USER_FIELD_ACCOUNT, stUserUpdateInfo.stNewUserInfo.stAccountInfo.account), userInfos2);
        if (userInfos2.size() != 0)
        {
            dlog_info("用户已存在");
            return IpcRet_E::ERR_USER_EXIST;
        }
    }

    /* 判断密码是否符合规范 */
    if(!stSecurityServicesInfo.stPwdPolicy.bPwdSecurityLevelEnable)
    {
        int result = check_password(stUserUpdateInfo.stNewUserInfo.stAccountInfo.password.c_str(),stUserUpdateInfo.stNewUserInfo.stAccountInfo.account.c_str(),NULL);
        if (result != IpcRet_E::OK)
        {
            dlog_info("密码不符合规范!");
            return result;
        }
    }

    /* 本次密码修改最终采用的时间字符串 */
    const std::string strPasswordUpdateTime = get_password_update_time(stUserUpdateInfo);
    if (userInfos[0].stAccountInfo.password != stUserUpdateInfo.stNewUserInfo.stAccountInfo.password){
        stUserUpdateInfo.stNewUserInfo.stAccountInfo.strFirstLoginTime = strPasswordUpdateTime;
    }

    Item item;
    item.push_back(Element(USER_FIELD_ACCOUNT, stUserUpdateInfo.stNewUserInfo.stAccountInfo.account));
    item.push_back(Element(USER_FIELD_PASSWORD, stUserUpdateInfo.stNewUserInfo.stAccountInfo.password));
    item.push_back(Element(USER_FIELD_ACCOUNT_TYPE, stUserUpdateInfo.stNewUserInfo.stAccountInfo.nAccountType));
    item.push_back(Element(USER_FIELD_ACCOUNT_SAFETY, stUserUpdateInfo.stNewUserInfo.stAccountInfo.nSafety));
    if(!stUserUpdateInfo.stNewUserInfo.stAccountInfo.strFirstLoginTime.empty())
    {
    item.push_back(Element(USER_FIELD_LOGO_FIRST_TIME, stUserUpdateInfo.stNewUserInfo.stAccountInfo.strFirstLoginTime));          
    }
    item.push_back(Element(USER_FIELD_NAME, stUserUpdateInfo.stNewUserInfo.stBindInfo.name));
    item.push_back(Element(USER_FIELD_PHONE_NUMBER, stUserUpdateInfo.stNewUserInfo.stBindInfo.phoneNumber));
    item.push_back(Element(USER_FIELD_LOGO_PATH, stUserUpdateInfo.stNewUserInfo.stBindInfo.logoPath));

    item.push_back(Element(PERMISSION_FIELD_PREVIEW, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stMenuPermission.bPreview)));
    item.push_back(Element(PERMISSION_FIELD_PLAYBACK, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stMenuPermission.bPlayback)));
    item.push_back(Element(PERMISSION_FIELD_RETRIEVE, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stMenuPermission.bRetrieve)));
    item.push_back(Element(PERMISSION_FIELD_ADHIBITION, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stMenuPermission.bAdhibition)));
    item.push_back(Element(PERMISSION_FIELD_WEB_LOCAL_CONFIG, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stMenuPermission.bWebLocalConfig)));
    item.push_back(Element(PERMISSION_FIELD_SYSTEM_CONFIG, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stMenuPermission.bSystemConfig)));
    item.push_back(Element(PERMISSION_FIELD_NETWORK_CONFIG, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stMenuPermission.bNetworkConfig)));
    item.push_back(Element(PERMISSION_FIELD_CHANNEL_MANAGE, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stMenuPermission.bChannelManage)));
    item.push_back(Element(PERMISSION_FIELD_VIDEO_AND_AUDIO, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stMenuPermission.bVideoAndAudio)));
    item.push_back(Element(PERMISSION_FIELD_EVENT_CONFIG, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stMenuPermission.bEventConfig)));
    item.push_back(Element(PERMISSION_FIELD_VIDEO_MANAGE, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stMenuPermission.bVideoManage)));
    item.push_back(Element(PERMISSION_FIELD_OBJECT_LIB, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stMenuPermission.bObjectLib)));
    item.push_back(Element(PERMISSION_FIELD_VEHICLE_DETECT_CONFIG, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stMenuPermission.bVehicleDetecConfig)));
    item.push_back(Element(PERMISSION_FIELD_LOCAL_SHUTDOWN, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stMenuPermission.bLocalShutdown)));

    item.push_back(Element(PERMISSION_FIELD_PTZ_CONTROL, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stOperatePermission.bPTZControl)));
    item.push_back(Element(PERMISSION_FIELD_ROUND, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stOperatePermission.bRound)));
    item.push_back(Element(PERMISSION_FIELD_TALK, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stOperatePermission.bTalk)));
    item.push_back(Element(PERMISSION_FIELD_RECORD, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stOperatePermission.bRecord)));
    item.push_back(Element(PERMISSION_FIELD_RESTART, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stOperatePermission.bRestart)));
    item.push_back(Element(PERMISSION_FIELD_SIMPLE_RECOVERY, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stOperatePermission.bSimpleRecovery)));
    item.push_back(Element(PERMISSION_FIELD_FULL_RECOVERY, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stOperatePermission.bFullRecovery)));
    item.push_back(Element(PERMISSION_FIELD_PARAMETER_DERIVATION, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stOperatePermission.bParameterDerivation)));
    item.push_back(Element(PERMISSION_FIELD_UPGRADE, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stOperatePermission.bUpgrade)));
    item.push_back(Element(PERMISSION_FIELD_ACTION_ALARM, static_cast<int>(stUserUpdateInfo.stNewUserInfo.stPermissions.stOperatePermission.bActionAlarm)));

    MatchMethods methods;
    methods.push_back(MatchMethod(Element(USER_FIELD_ACCOUNT, stUserUpdateInfo.stAccountInfo.account), FIND_CRITERION_EQ));
    int nRet = CUserDatabase::instance()->update(item, methods);

    if (nRet == 0)
    {
        if (stUserUpdateInfo.stAccountInfo.password != stUserUpdateInfo.stNewUserInfo.stAccountInfo.password)
        {
            clear_pending_password_update_time(stUserUpdateInfo.stAccountInfo.account);
            for (auto it = m_vecOnlineUsers.begin(); it != m_vecOnlineUsers.end();)
            {
                const std::string &strUsername = it->strUsername;
                if (strUsername == stUserUpdateInfo.stAccountInfo.account)
                {
                    dlog_info("发送在线用户退出命令");
                    User::DeleUser_S stDeleUser;
                    stDeleUser.nId = it->nOnlineUserId;
                    stDeleUser.account = stUserUpdateInfo.stAccountInfo.account;
                    TaskPublish::instance()->message(AC_DEL_AND_EXIT_USER, Convert::to_string(stDeleUser));
                    /* 删除当前元素 */
                    it = m_vecOnlineUsers.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    return nRet;
}


/**
 * @brief 更新用户状态
 */
int CUserManage::update_status(User::UserInfo_S stUserUpdateInfo, User::AccountStatus_E enStatus)
{
    std::vector<User::UserInfo_S> userInfos;
    User::UserInfo_S stUserInfo;
    CUserDatabase::instance()->find(Element(USER_FIELD_ACCOUNT, stUserUpdateInfo.stAccountInfo.account), userInfos);
    if (userInfos.size() == 0)
    {
        dlog_error("%s用户不存在", stUserUpdateInfo.stAccountInfo.account.c_str());
        return IpcRet_E::ERR_USER_NOT_EXIST;
    }
    stUserInfo = userInfos[0];

    Item item;
    stUserInfo.stAccountInfo.nAccountStatus = (int)enStatus;
    item.push_back(Element(USER_FIELD_ACCOUNT_STATUS, stUserInfo.stAccountInfo.nAccountStatus));
    
    MatchMethods methods;
    methods.push_back(MatchMethod(Element(USER_FIELD_ACCOUNT, stUserInfo.stAccountInfo.account), FIND_CRITERION_EQ));
    CUserDatabase::instance()->update(item, methods);
    dlog_info("%s用户数据库状态更新成功！状态：%d", stUserInfo.stAccountInfo.account.c_str(), stUserInfo.stAccountInfo.nAccountStatus);


    if (enStatus == User::AccountStatus_E::ACCOUNT_STATUS_OFFLINE || 
        enStatus == User::AccountStatus_E::ACCOUNT_STATUS_ONLINE ||
        enStatus == User::AccountStatus_E::ACCOUNT_STATUS_NORMAL) 
    {
        std::string strUserKey = std::string(PREFIX_USER) + stUserUpdateInfo.stAccountInfo.account;
        
        auto it = mErrorInfo.find(strUserKey);
        if (it != mErrorInfo.end())
        {
            mErrorInfo.erase(it);
            dlog_info("已强制清除用户 [%s] 的内存锁定记录，现在可以登录", stUserUpdateInfo.stAccountInfo.account.c_str());
        }
    }

    return 0;
}

/**
 * @brief 重置密码信息
 */
int CUserManage::reset_password(User::UpdateInfo_S stUserUpdateInfo,::System::SecurityServices_S stSecurityServicesInfo)
{
    std::vector<User::UserInfo_S> userInfos;
    User::UserInfo_S stUserInfo;
    CUserDatabase::instance()->find(Element(USER_FIELD_ACCOUNT, stUserUpdateInfo.stAccountInfo.account), userInfos);
    if (userInfos.size() == 0)
    {
        dlog_error("%s用户不存在", stUserUpdateInfo.stAccountInfo.account.c_str());
        return IpcRet_E::ERR_USER_NOT_EXIST;
    }
    stUserInfo = userInfos[0];

    //用户重置密码后，第一次登陆需要强制修改密码，清除strFirstLoginTime信息
    stUserInfo.stAccountInfo.strFirstLoginTime.clear();
    clear_pending_password_update_time(stUserInfo.stAccountInfo.account);

    Item item;
    stUserInfo.stAccountInfo.password = stUserUpdateInfo.stAccountInfo.password;
    item.push_back(Element(USER_FIELD_PASSWORD, stUserInfo.stAccountInfo.password));
    item.push_back(Element(USER_FIELD_LOGO_FIRST_TIME, stUserInfo.stAccountInfo.strFirstLoginTime));
    
    MatchMethods methods;
    methods.push_back(MatchMethod(Element(USER_FIELD_ACCOUNT, stUserInfo.stAccountInfo.account), FIND_CRITERION_EQ));
    int nRet = CUserDatabase::instance()->update(item, methods);

    if (nRet == 0)
    {
            for (auto it = m_vecOnlineUsers.begin(); it != m_vecOnlineUsers.end();)
            {
                const std::string &strUsername = it->strUsername;
                if (strUsername == stUserUpdateInfo.stAccountInfo.account)
                {
                    dlog_info("发送在线用户退出命令");
                    User::DeleUser_S stDeleUser;
                    stDeleUser.nId = it->nOnlineUserId;
                    stDeleUser.account = stUserUpdateInfo.stAccountInfo.account;
                    TaskPublish::instance()->message(AC_UPDATE_AND_EXIT_USER, Convert::to_string(stDeleUser));
                    /* 删除当前元素 */
                    it = m_vecOnlineUsers.erase(it);
                }
                else
                {
                    ++it;
                }
            }
    }

    dlog_info("%s用户数据库密码重置成功！",stUserUpdateInfo.stAccountInfo.account.c_str());


    return nRet;
}


/**
 * @brief 查找用户信息
 */
int CUserManage::find(User::Find_S stUserFind, std::vector<User::UserInfo_S> &userInfos)
{
    MatchMethods methods;
    Criterion_E enCriterion = FIND_CRITERION_NONE;
    if (stUserFind.stSearch.nAccountType >= 0)
    {
        if (enCriterion != FIND_CRITERION_NONE)
        {
            MatchMethod &lastMethod = methods.back();
            lastMethod.enAndOr = enCriterion;
        }
        methods.push_back(MatchMethod(Element(USER_FIELD_ACCOUNT_TYPE, stUserFind.stSearch.nAccountType), FIND_CRITERION_EQ));
    }

    return CUserDatabase::instance()->find(methods, userInfos);
}

/**
 * @brief 根据账号获取用户信息
 */
int CUserManage::get_itemInfo(User::UserInfo_S &stUserInfo)
{
    std::vector<User::UserInfo_S> userInfos;
    CUserDatabase::instance()->find(Element(USER_FIELD_ACCOUNT, stUserInfo.stAccountInfo.account), userInfos);
    if (userInfos.size() == 0)
    {
        dlog_error("用户不存在");
        return IpcRet_E::ERR_USER_NOT_EXIST;
    }
    stUserInfo = userInfos[0];
    return 0;
}

/* 获取错误信息 */
User::LogErrorInfo_S CUserManage::get_logErrorInfo(std::string strAccount)
{
    User::LogErrorInfo_S stReturnInfo;
    stReturnInfo.nErrorCount = 0;
    stReturnInfo.strFirstRecordTime = "";
    stReturnInfo.strReleaseTime = "";

    // 获取当前请求的客户端IP
    std::string strClientIp = CWebServer::instance()->get_loginclient_ip();
    if (strClientIp.empty()) {
        strClientIp = UNKNOWN_USER;
    }

    std::string strIpKey = std::string(PREFIX_IP) + strClientIp;
    std::string strUserKey = std::string(PREFIX_USER) + strAccount;

    // 查找 Map 中的记录
    std::lock_guard<std::mutex> lock(m_mutex);

    User::LogErrorInfo_S stIpInfo = {0};
    User::LogErrorInfo_S stUserInfo = {0};
    bool bIpExists = false;
    bool bUserExists = false;

    if (mErrorInfo.find(strIpKey) != mErrorInfo.end()) {
        stIpInfo = mErrorInfo[strIpKey];
        bIpExists = true;
    }

    if (!strAccount.empty() && mErrorInfo.find(strUserKey) != mErrorInfo.end()) {
        stUserInfo = mErrorInfo[strUserKey];
        bUserExists = true;
    }

    // IP 已经被锁定 (有解锁时间) -> 优先级最高
    if (bIpExists && !stIpInfo.strReleaseTime.empty()) {
        return stIpInfo;
    }

    //  账号 已经被锁定 -> 优先级次之
    if (bUserExists && !stUserInfo.strReleaseTime.empty()) {
        return stUserInfo;
    }

    //都没锁定，但账号有错误计数 -> 返回账号计数
    if (bUserExists && stUserInfo.nErrorCount > 0) {
        return stUserInfo;
    }

    // 账号无记录，但IP有错误计数 -> 返回IP计数
    if (bIpExists && stIpInfo.nErrorCount > 0) {
        return stIpInfo;
    }

    // 无任何错误记录
    return stReturnInfo;

}

/* 添加错误超时时间 */
std::string CUserManage::addErrorTime(const std::string &strTime,::System::LockDuration_E enDuration)
{
    std::tm tmTime = {};

    std::istringstream iss(strTime);
    iss >> std::get_time(&tmTime, DATE_TIME_FORMAT_YYYYMMDD_DEFAULT);
    if (iss.fail())
    {
        dlog_error("时间解析失败");
        return "";
    }

    std::time_t time = std::mktime(&tmTime);
    if (time == -1)
    {
        dlog_error("时间转换失败");
        return "";
    }

    switch (enDuration)
    {
    case ::System::LOCK_DURATION_5_MIN:
        time += 5 * 60;          // 5  分钟
        break;
    case ::System::LOCK_DURATION_30_MIN:
        time += 30 * 60;         // 30 分钟
        break;
    case ::System::LOCK_DURATION_60_MIN:
        time += 60 * 60;         // 60 分钟
        break;
    case ::System::LOCK_DURATION_1_DAY:
        time += 1440 * 60;       // 1 天 = 24*60*60 秒
        break;
    case ::System::LOCK_DURATION_5_DAY:
        time = 5 * 1440 * 60;      // 5 天 = 24*60*60*5 秒
        break;
    }

    std::tm *updatedTm = std::localtime(&time);

    std::ostringstream oss;
    oss << std::put_time(updatedTm, DATE_TIME_FORMAT_YYYYMMDD_DEFAULT);

    return oss.str();
}

int CUserManage::get_all_user(std::vector<User::UserInfo_S> &userInfos)
{
    MatchMethods emptyMethods;
    CUserDatabase::instance()->find(emptyMethods, userInfos);

    /* 过滤隐藏的itc账号 */
    userInfos.erase(std::remove_if(userInfos.begin(), userInfos.end(),
        [](const User::UserInfo_S &u) { return u.stAccountInfo.account == "itc"; }),
        userInfos.end());

    return 0;
}

std::string CUserManage::get_passwd(std::string strUser)
{
    User::UserInfo_S stUserInfo;
    stUserInfo.stAccountInfo.account = strUser;
    if (get_itemInfo(stUserInfo) != 0)
    {
        dlog_error("获取用户信息失败");
        return "";
    }

    return stUserInfo.stAccountInfo.password;
}

int CUserManage::verifi_user(std::string strUser)
{
    User::UserInfo_S stUserInfo;
    stUserInfo.stAccountInfo.account = strUser;
    if (get_itemInfo(stUserInfo) != 0)
    {
        dlog_error("onvif用户验证失败 用户获取失败");
        return ERR;
    }
    return OK;
}

/* 验证管理员密码 */
int CUserManage::admin_verification(std::string strPassword)
{
    std::vector<User::UserInfo_S> userInfos;
    User::UserInfo_S stUserInfo;
    stUserInfo.stAccountInfo.account = "admin";
    CUserDatabase::instance()->find(Element(USER_FIELD_ACCOUNT, stUserInfo.stAccountInfo.account), userInfos);
    if (userInfos.size() == 0)
    {
        dlog_error("admin用户不存在");
        return IpcRet_E::ERR_USER_NOT_EXIST;
    }
    stUserInfo = userInfos[0];

    if (stUserInfo.stAccountInfo.password == strPassword)
    {
        dlog_info("管理员密码验证成功");
        return 0;
    }
    else
    {
        dlog_error("管理员密码验证失败");
        return -1;
    }
}

std::vector<User::OnlineUser_S> CUserManage::get_online_users()
{
    /* 保护在线用户列表读取 */
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_vecOnlineUsers;
}

int CUserManage::get_online_user_specified(User::OnlineUser_S &UserOnlineInfo, const std::string& strUsername)
{
    /* 保护在线用户列表读取 */
    std::lock_guard<std::mutex> lock(m_mutex);
    /* 保存目标在线用户迭代器 */
    auto it = std::find_if(m_vecOnlineUsers.begin(), m_vecOnlineUsers.end(),
                           [&strUsername](const User::OnlineUser_S& user) {
                               return user.strUsername == strUsername;
                           });
    
    if (it != m_vecOnlineUsers.end()) {
        UserOnlineInfo = *it;  
        return 0;       
    }
    return -1;
}

bool CUserManage::check_ip_online_locked(const std::string &strIp) const
{
    for (const auto &onlineUser : m_vecOnlineUsers)
    {
        if (onlineUser.strIpAddress == strIp)
        {
            return true;
        }
    }
    return false;
}

bool CUserManage::has_online_user_for_account_locked(const std::string &strAccount) const
{
    for (const auto &onlineUser : m_vecOnlineUsers)
    {
        if (onlineUser.strUsername == strAccount)
        {
            return true;
        }
    }
    return false;
}

void CUserManage::refresh_account_online_status(const std::string &strAccount)
{
    if (strAccount.empty())
    {
        return;
    }

    /* 标记账号是否仍有其他在线记录 */
    bool bHasOnlineUser = false;
    {
        /* 保护在线用户列表读取 */
        std::lock_guard<std::mutex> lock(m_mutex);
        bHasOnlineUser = has_online_user_for_account_locked(strAccount);
    }

    /* 保存数据库更新项 */
    Item item;
    /* 保存数据库查询条件 */
    MatchMethods methods;
    /* 保存账号查询结果 */
    std::vector<User::UserInfo_S> userInfos;
    /* 保存目标账号状态 */
    int nAccountStatus = bHasOnlineUser ? User::AccountStatus_E::ACCOUNT_STATUS_ONLINE
                                        : User::AccountStatus_E::ACCOUNT_STATUS_OFFLINE;

    CUserDatabase::instance()->find(Element(USER_FIELD_ACCOUNT, strAccount), userInfos);
    if (userInfos.empty())
    {
        dlog_error("%s用户不存在，无法刷新在线状态", strAccount.c_str());
        return;
    }

    item.push_back(Element(USER_FIELD_ACCOUNT_STATUS, nAccountStatus));
    methods.push_back(MatchMethod(Element(USER_FIELD_ACCOUNT, strAccount), FIND_CRITERION_EQ));
    CUserDatabase::instance()->update(item, methods);
    dlog_info("%s用户状态刷新成功！状态：%d", strAccount.c_str(), nAccountStatus);
}

int CUserManage::cleanup_online_users_by_ip(const std::string &strIp)
{
    if (strIp.empty())
    {
        return 0;
    }

    /* 保存被清理记录涉及的账号 */
    std::vector<std::string> vecAccounts;
    /* 保存清理数量 */
    int nRemovedCount = 0;

    {
        /* 保护在线用户列表写入 */
        std::lock_guard<std::mutex> lock(m_mutex);
        /* 保存在线用户列表迭代器 */
        auto it = m_vecOnlineUsers.begin();
        while (it != m_vecOnlineUsers.end())
        {
            if (it->strIpAddress == strIp)
            {
                vecAccounts.push_back(it->strUsername);
                it = m_vecOnlineUsers.erase(it);
                ++nRemovedCount;
            }
            else
            {
                ++it;
            }
        }
    }

    if (nRemovedCount == 0)
    {
        return 0;
    }

    std::sort(vecAccounts.begin(), vecAccounts.end());
    vecAccounts.erase(std::unique(vecAccounts.begin(), vecAccounts.end()), vecAccounts.end());
    for (const auto &strAccount : vecAccounts)
    {
        refresh_account_online_status(strAccount);
    }

    dlog_warn("已清理IP[%s]的残留在线记录，数量：%d", strIp.c_str(), nRemovedCount);
    return nRemovedCount;
}

int CUserManage::delete_online_user(int nOnlineUserId)
{
    /* 保存被删除在线记录对应账号 */
    std::string strAccount;
    {
        /* 保护在线用户列表写入 */
        std::lock_guard<std::mutex> lock(m_mutex);
        /* 保存目标在线用户迭代器 */
        auto it = std::find_if(m_vecOnlineUsers.begin(),
                               m_vecOnlineUsers.end(),
                               [nOnlineUserId](const User::OnlineUser_S &user)
                               {
                                   return user.nOnlineUserId == nOnlineUserId;
                               });
        if (it == m_vecOnlineUsers.end())
        {
            dlog_info("在线用户ID: %d 删除失败！", nOnlineUserId);
            return 1;
        }

        strAccount = it->strUsername;
        m_vecOnlineUsers.erase(it);
    }

    refresh_account_online_status(strAccount);
    dlog_info("在线用户ID: %d 删除成功！", nOnlineUserId);
    return 0;
}

bool CUserManage::check_ip_online(const std::string &strIp)
{
    /* 保护在线用户列表读取 */
    std::lock_guard<std::mutex> lock(m_mutex);
    return check_ip_online_locked(strIp);
}

/* 生成唯一id */
int CUserManage::generate_online_userId()
{
    /* 保存当前时间点 */
    auto now = std::chrono::system_clock::now();
    /* 保存当前时间戳持续时长 */
    auto duration = now.time_since_epoch();
    /* 保存秒级时间戳 */
    int nSeconds = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(duration).count());

    /* 保存随机数设备 */
    std::random_device rd;
    /* 保存随机数生成器 */
    std::mt19937 gen(rd());
    /* 保存随机数分布 */
    std::uniform_int_distribution<int> dis(1, 9999);

    /* 保存随机部分 */
    int randomPart = dis(gen);
    /* 保存最终在线用户ID */
    int nUniqueId = (nSeconds % 1000000) * 10000 + randomPart;

    return nUniqueId;
}

/* 验证用户操作是否有权限 */
int CUserManage::user_permssion_auth(User::UserOperation_S stUserOperation)
{
    std::vector<User::UserInfo_S> userInfos;
    User::UserInfo_S stUserInfo;
    stUserInfo.stAccountInfo.account = stUserOperation.strUsername;
    CUserDatabase::instance()->find(Element(USER_FIELD_ACCOUNT, stUserInfo.stAccountInfo.account), userInfos);
    if (userInfos.size() == 0)
    {
        dlog_error("用户不存在");
        return IpcRet_E::ERR_USER_NOT_EXIST;
    }
    stUserInfo = userInfos[0];

    /* 预览权限 */
    if (stUserOperation.strOperation == "Preview" && stUserInfo.stPermissions.stMenuPermission.bPreview)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 回放权限 */
    else if (stUserOperation.strOperation == "Playback" && stUserInfo.stPermissions.stMenuPermission.bPlayback)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 检索权限 */
    else if (stUserOperation.strOperation == "Retrieve" && stUserInfo.stPermissions.stMenuPermission.bRetrieve)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 应用权限 */
    else if (stUserOperation.strOperation == "Adhibition" && stUserInfo.stPermissions.stMenuPermission.bAdhibition)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* WEB端本地配置权限 */
    else if (stUserOperation.strOperation == "WebLocalConfig" && stUserInfo.stPermissions.stMenuPermission.bWebLocalConfig)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 系统配置权限 */
    else if (stUserOperation.strOperation == "SystemConfig" && stUserInfo.stPermissions.stMenuPermission.bSystemConfig)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 网络配置权限 */
    else if (stUserOperation.strOperation == "NetworkConfig" && stUserInfo.stPermissions.stMenuPermission.bNetworkConfig)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 通道管理权限 */
    else if (stUserOperation.strOperation == "ChannelManage" && stUserInfo.stPermissions.stMenuPermission.bChannelManage)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 视音频权限 */
    else if (stUserOperation.strOperation == "VideoAndAudio" && stUserInfo.stPermissions.stMenuPermission.bVideoAndAudio)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 事件配置权限 */
    else if (stUserOperation.strOperation == "EventConfig" && stUserInfo.stPermissions.stMenuPermission.bEventConfig)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 录像管理权限 */
    else if (stUserOperation.strOperation == "VideoManage" && stUserInfo.stPermissions.stMenuPermission.bVideoManage)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 目标库权限 */
    else if (stUserOperation.strOperation == "ObjectLib" && stUserInfo.stPermissions.stMenuPermission.bObjectLib)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 车辆检测配置权限 */
    else if (stUserOperation.strOperation == "VehicleDetecConfig" && stUserInfo.stPermissions.stMenuPermission.bVehicleDetecConfig)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 本地关机权限 */
    else if (stUserOperation.strOperation == "LocalShutdown" && stUserInfo.stPermissions.stMenuPermission.bLocalShutdown)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }

    /* 云台控制权限 */
    else if (stUserOperation.strOperation == "PTZControl" && stUserInfo.stPermissions.stOperatePermission.bPTZControl)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 轮询权限 */
    else if (stUserOperation.strOperation == "Round" && stUserInfo.stPermissions.stOperatePermission.bRound)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 对讲权限 */
    else if (stUserOperation.strOperation == "Talk" && stUserInfo.stPermissions.stOperatePermission.bTalk)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 录像权限 */
    else if (stUserOperation.strOperation == "Record" && stUserInfo.stPermissions.stOperatePermission.bRecord)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 重启权限 */
    else if (stUserOperation.strOperation == "Restart" && stUserInfo.stPermissions.stOperatePermission.bRestart)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 简单恢复权限 */
    else if (stUserOperation.strOperation == "SimpleRecovery" && stUserInfo.stPermissions.stOperatePermission.bSimpleRecovery)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 完全恢复权限 */
    else if (stUserOperation.strOperation == "FullRecovery" && stUserInfo.stPermissions.stOperatePermission.bFullRecovery)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 设备参数导出权限 */
    else if (stUserOperation.strOperation == "ParameterDerivation" && stUserInfo.stPermissions.stOperatePermission.bParameterDerivation)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 升级权限 */
    else if (stUserOperation.strOperation == "Upgrade" && stUserInfo.stPermissions.stOperatePermission.bUpgrade)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 报警联动权限 */
    else if (stUserOperation.strOperation == "ActionAlarm" && stUserInfo.stPermissions.stOperatePermission.bActionAlarm)
    {
        return IpcRet_E::OK_USER_PERMISSION;
    }
    /* 返回权限验证失败错误码 */
    else
    {
        return IpcRet_E::ERR_USER_PERMISSION;
    }
}
