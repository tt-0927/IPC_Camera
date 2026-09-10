/***
 * @FilePath     : user_task.cpp
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-10-06 15:16:08
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-28 15:17:05
 * @Description  :
 */

#include "user_task.h"
#include "user_convert.h"
#include "convert_interface.h"
#include "dlog.h"
#include "user_manage.h"
#include "system_manage.h"
#include "preview_manage.h"
#include "network_utils.h"
#include "log_handler.h"
#include "web_server.h"
#include "path_define.h"
#include "IpcRet.h"
#include "user_define.h"
#include "rtsp_server.h"

using namespace User;

/* web端用户登录 */
void Task::User::Login::handle()
{
    /* 取出账号信息 */
    ::User::AccountInfo_S stAccountInfo;
    Convert::to_struct(m_taskData, stAccountInfo);

    /* 用户数据处理 */
    ::User::UserInfo_S stUserInfo;

    ::System::SecurityServices_S stSecurityServicesInfo;
    Convert::read_file(SECURITY_SERVICES_FILE, stSecurityServicesInfo);
    int nRet = CUserManage::instance()->login(stAccountInfo, stUserInfo, stSecurityServicesInfo);
    if (nRet < 0 && nRet != IpcRet_E::PASS_ERR_EXPIRED && nRet != IpcRet_E::PASS_ERR_FIRST_LOGIN_PWD_CHANGE)
    {
        /* 异常日志 */
        Log::Info_S stLogInfo;
        stLogInfo.nType = Log::EXCEPTION;
        stLogInfo.nAction = Log::UNAUTHORIZED_ACCESS;
        stLogInfo.user = stAccountInfo.account;
        

        /* 同IP重复登录错误，返回带提示信息的JSON */
        if (nRet == IpcRet_E::ERR_REPEAT_LOGIN_IP)
        {
            stLogInfo.nType = Log::OPERATION;
            stLogInfo.nAction = Log::REPEATED_LOGIN;
            std::string res = "{\"Msg\":\"" + std::string(pass_check_msg(nRet)) + "\"}";
            result(res, nRet);
        }
        else
        {
            result(std::string(), nRet);
        }
        LogHandler::instance()->write(stLogInfo);
    }
    else
    {
        /* 操作日志-登录 */
        Log::Info_S stLogInfo;
        stLogInfo.nType = Log::OPERATION;
        stLogInfo.nAction = Log::LOCAL_LOGIN;
        stLogInfo.user = stAccountInfo.account;
        LogHandler::instance()->write(stLogInfo);
        std::string accountInfo = Convert::to_string(stUserInfo);
        result(accountInfo, nRet);
    }
}

/* 获取登录错误信息 */
void Task::User::GetLoginErrorInfo::handle()
{
    dlog_debug("m_taskData %s] ", m_taskData.c_str());
    /* 取出用户信息 */
    ::User::UserInfo_S stUserInfo;
    Convert::to_struct(m_taskData, stUserInfo);

    ::User::LogErrorInfo_S stLogErrorInfo;
    stLogErrorInfo = CUserManage::instance()->get_logErrorInfo(stUserInfo.stAccountInfo.account);
    std::string errorInfo = Convert::to_string(stLogErrorInfo);
    result(errorInfo);
}

/* 获取用户信息 */
void Task::User::Find::handle()
{
    ::User::UserInfo_S stUserInfo;
    Convert::to_struct(m_taskData, stUserInfo);
    int nRet = CUserManage::instance()->get_itemInfo(stUserInfo);
    if (nRet < 0)
    {
        result(std::string(), nRet);
    }
    else
    {
        std::string infos = Convert::to_string(stUserInfo);
        result(infos, nRet);
    }
}

/* 修改用户信息 */
void Task::User::Update::handle()
{
    dlog_debug("m_taskData %s] ", m_taskData.c_str());
    /* 取出账号信息 */
    ::User::UpdateInfo_S stUserUpdateInfo;
    int nRet;
    // Convert::to_struct(m_taskData, stUserUpdateInfo.stAccountInfo);
    // stUserUpdateInfo.stNewUserInfo.stAccountInfo = stUserUpdateInfo.stAccountInfo;
    // int nRet = CUserManage::instance()->get_itemInfo(stUserUpdateInfo.stNewUserInfo);
    // if (nRet < 0 )
    //{
    //     result(nRet);
    //     return;
    // }
    // dlog_info("新密码:%s 旧密码:%s",stUserUpdateInfo.stAccountInfo.password.c_str(), stUserUpdateInfo.stNewUserInfo.stAccountInfo.password.c_str());
    ///* 密码验证 */
    // if (!stUserUpdateInfo.stAccountInfo.password.empty() && stUserUpdateInfo.stAccountInfo.password != stUserUpdateInfo.stNewUserInfo.stAccountInfo.password)
    //{
    //     result(ERR_PASSWORD_WRONG);
    //     return;
    // }
    ///* 取出更新信息 */
    // Convert::to_struct(m_taskData, stUserUpdateInfo.stNewUserInfo);
    ::User::UserInfo_S stOldUserInfo;
    Convert::to_struct(m_taskData, stUserUpdateInfo);
    
    ::System::SecurityServices_S stSecurityServicesInfo;
    Convert::read_file(SECURITY_SERVICES_FILE, stSecurityServicesInfo);
    

    stOldUserInfo.stAccountInfo.account = stUserUpdateInfo.stAccountInfo.account;
    nRet = CUserManage::instance()->get_itemInfo(stOldUserInfo);
    if (nRet < 0)
    {
        result(nRet);
        return;
    }
    /* 验证旧密码 */
    if (!stUserUpdateInfo.stAccountInfo.password.empty())
    {
        dlog_info("旧密码:%s 新密码:%s", stOldUserInfo.stAccountInfo.password.c_str(), stUserUpdateInfo.stAccountInfo.password.c_str());
        if (stOldUserInfo.stAccountInfo.password != stUserUpdateInfo.stAccountInfo.password)
        {
            dlog_error("旧密码验证失败");
            result(ERR_PASSWORD_WRONG);
            return;
        }
    }

    /* 用户数据处理 */
    nRet = CUserManage::instance()->update(stUserUpdateInfo,stSecurityServicesInfo);
    if (nRet < 0)
    {
        std::string res = "{\"Msg\":\"" + std::string(pass_check_msg(nRet)) + "\"}";
        result(res, nRet);
    }
    else
    {
        std::string accountInfo = Convert::to_string(stUserUpdateInfo);
        result(accountInfo, nRet);
    }

    /*如果是管理员账号*/
    if (stUserUpdateInfo.stNewUserInfo.stAccountInfo.nAccountType == ACCOUNT_TYPE_ADMIN && nRet >= 0)
    {
        /*更新ssh账号信息*/
        SystemManage::instance()->add_ssh_admin(stUserUpdateInfo.stNewUserInfo.stAccountInfo.account.c_str(),
                                                stUserUpdateInfo.stNewUserInfo.stAccountInfo.password.c_str());
        /* 更新RtspServer账号信息 */
        CRtspServer::instance()->update_userInfo(stUserUpdateInfo.stNewUserInfo.stAccountInfo.account,
                                                 stUserUpdateInfo.stNewUserInfo.stAccountInfo.password,
                                                 true);
    }
}

/* 更新用户状态 -当前仅仅解锁作用*/
void Task::User::UpdateStatus::handle()
{
    dlog_debug("m_taskData %s] ", m_taskData.c_str());
    ::User::UserInfo_S stUserInfo;
    Convert::to_struct(m_taskData, stUserInfo);

    int nRet = CUserManage::instance()->update_status(stUserInfo,::User::AccountStatus_E::ACCOUNT_STATUS_NORMAL);
    result(nRet);

}

/* 添加用户 */
void Task::User::Add::handle()
{
    dlog_debug("m_taskData %s] ", m_taskData.c_str());
    /* 取出用户信息 */
    ::User::UserInfo_S stUserInfo;
    Convert::to_struct(m_taskData, stUserInfo);

    ::System::SecurityServices_S stSecurityServicesInfo;
    Convert::read_file(SECURITY_SERVICES_FILE, stSecurityServicesInfo);

    /* 用户数据处理 */
    int nRet = CUserManage::instance()->add(stUserInfo,stSecurityServicesInfo);
    if (nRet < 0)
    {
        std::string res = "{\"Msg\":\"" + std::string(pass_check_msg(nRet)) + "\"}";
        result(res, nRet);
    }
    else
    {
        result(nRet);
    }

    /*如果是管理员账号*/
    if (stUserInfo.stAccountInfo.nAccountType == ACCOUNT_TYPE_ADMIN && nRet >= 0)
    {
        /*更新ssh账号信息*/
        SystemManage::instance()->add_ssh_admin(stUserInfo.stAccountInfo.account.c_str(),
                                                stUserInfo.stAccountInfo.password.c_str());
        /* 更新RtspServer账号信息 */
        CRtspServer::instance()->update_userInfo(stUserInfo.stAccountInfo.account,
                                                 stUserInfo.stAccountInfo.password,
                                                 true);
    }
}

/* 删除用户 */
void Task::User::Del::handle()
{
    dlog_debug("m_taskData %s] ", m_taskData.c_str());
    ::User::UserInfo_S stUserInfo;
    Convert::to_struct(m_taskData, stUserInfo);

    int nRet = CUserManage::instance()->del(stUserInfo);
    result(nRet);

    /*如果是管理员账号*/
    if(stUserInfo.stAccountInfo.nAccountType == ACCOUNT_TYPE_ADMIN  && nRet >= 0){
        /*删除ssh账号信息*/
        SystemManage::instance()->del_ssh_admin(stUserInfo.stAccountInfo.account.c_str());
    }
}

/* 获取所有用户 */
void Task::User::GetAllUser::handle()
{
    std::vector<::User::UserInfo_S> userInfos;
    CUserManage::instance()->get_all_user(userInfos);
    result(Convert::to_string(userInfos));
}

/* 管理员密码验证 */
void Task::User::VerificationAdmin::handle()
{
    ::User::AdminInfo_S stAdminInfo;
    Convert::to_struct(m_taskData, stAdminInfo);

    int nRet = CUserManage::instance()->admin_verification(stAdminInfo.password);
    result(nRet);
}

/* 获取在线用户 */
void Task::User::GetOnlineUser::handle()
{
    std::vector<::User::OnlineUser_S> vecOlineUsers;
    vecOlineUsers = CUserManage::instance()->get_online_users();
    result(Convert::to_string(vecOlineUsers));
}

/* 删除在线用户 */
void Task::User::DeleteOnlinUser::handle()
{
    ::User::OnlineUser_S stOnlineUser;
    Convert::to_struct(m_taskData, stOnlineUser);
    int nRet = CUserManage::instance()->delete_online_user(stOnlineUser.nOnlineUserId);
    result(nRet);
}

/* 用户权限验证 */
void Task::User::UserPermissionAuth::handle()
{
    ::User::UserOperation_S stUserOperation;
    Convert::to_struct(m_taskData, stUserOperation);
    int nRet = CUserManage::instance()->user_permssion_auth(stUserOperation);
    result(nRet);
}

void Task::User::DeleUserExit::handle()
{

}

void Task::User::UpdateUserExit::handle()
{
}

/* 更新当前在线用户 任务处理在 web_server 中 */
void Task::User::UpdateLocalOnlineUser::handle()
{
}

/* 重置密码信息 */
void Task::User::ResetPassword::handle()
{
    dlog_debug("m_taskData %s] ", m_taskData.c_str());
    ::User::UserInfo_S stUserInfo;
    Convert::to_struct(m_taskData, stUserInfo);

    ::User::UpdateInfo_S stUserUpdateInfo;
    int nRet;
    
    ::System::SecurityServices_S stSecurityServicesInfo;
    Convert::read_file(SECURITY_SERVICES_FILE, stSecurityServicesInfo);

    stUserUpdateInfo.stAccountInfo.account = stUserInfo.stAccountInfo.account;
    stUserUpdateInfo.stAccountInfo.password = USER_DEFAULT_PASSWD;

    /* 用户数据处理 */
    nRet = CUserManage::instance()->reset_password(stUserUpdateInfo,stSecurityServicesInfo);

    /*如果是管理员账号*/
    if (stUserUpdateInfo.stNewUserInfo.stAccountInfo.nAccountType == ACCOUNT_TYPE_ADMIN && nRet >= 0)
    {
        /*更新ssh账号信息*/
        SystemManage::instance()->add_ssh_admin(stUserUpdateInfo.stNewUserInfo.stAccountInfo.account.c_str(),
                                                stUserUpdateInfo.stNewUserInfo.stAccountInfo.password.c_str());
        /* 更新RtspServer账号信息 */
        CRtspServer::instance()->update_userInfo(stUserUpdateInfo.stNewUserInfo.stAccountInfo.account,
                                                 stUserUpdateInfo.stNewUserInfo.stAccountInfo.password,
                                                 true);
    }

    result( nRet);

}

