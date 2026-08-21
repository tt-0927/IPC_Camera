/***
 * @FilePath     : register_task.cpp
 * @Author       : huangjunda
 * @Date         : 2025-07-08 14:17:20
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-07-08 14:27:19
 * @Description  : 激活操作
 */

#include "register_task.h"
#include "register_convert.h"
#include "register_manage.h"
#include "network_manage.h"
#include "user_manage.h"
#include "time_manage.h"
#include "system_convert.h"
#include "web_server.h"
#include "log_handler.h"
#include "convert_interface.h"
#include "path_define.h"
#include "IpcRet.h"
#include "dlog.h"

// #include "LogHandler.h"

void Task::Register::GetReisterInfo::handle()
{
    dlog_info("获取注册信息");
    ::Register::RegisterInfo_S stRegInfo;
    Convert::read_file(REGISTER_INFO_FILE, stRegInfo);
    result(Convert::to_string(stRegInfo));
}

void Task::Register::SetRegisterEg::handle()
{
    dlog_info("设置注册码");
    int nRet = 0;
    ::Register::ConfigRegisterEg_S stConfigRegEg;
    Convert::to_struct(m_taskData, stConfigRegEg);
    nRet = CRegisterManage::instance()->register_device(stConfigRegEg.strRegisterEg);
    result(nRet);
}

void Task::Register::SetActivationPasswd::handle()
{
    dlog_info("添加admin激活用户密码");
    ::Register::ActivationPasswdInfo_S stActivationInfo;
    ::User::UserInfo_S stUserInfo;
    Convert::to_struct(m_taskData, stActivationInfo);

    ::System::SecurityServices_S stSecurityServicesInfo;
    Convert::read_file(SECURITY_SERVICES_FILE, stSecurityServicesInfo);
    

    /* 获取激活用户和密码 */
    stUserInfo.stAccountInfo.account = stActivationInfo.strUser;
    stUserInfo.stAccountInfo.password = stActivationInfo.strUserPwd;

    stUserInfo.stAccountInfo.nAccountType = 0;
    stUserInfo.stAccountInfo.nSafety = stActivationInfo.nSafety;

    /* 获取所有权限  */
    stUserInfo.stPermissions.stMenuPermission.bPreview = true;
    stUserInfo.stPermissions.stMenuPermission.bPlayback = true;
    stUserInfo.stPermissions.stMenuPermission.bRetrieve = true;
    stUserInfo.stPermissions.stMenuPermission.bAdhibition = true;
    stUserInfo.stPermissions.stMenuPermission.bWebLocalConfig = true;
    stUserInfo.stPermissions.stMenuPermission.bSystemConfig = true;
    stUserInfo.stPermissions.stMenuPermission.bNetworkConfig = true;
    stUserInfo.stPermissions.stMenuPermission.bChannelManage = true;
    stUserInfo.stPermissions.stMenuPermission.bVideoAndAudio = true;
    stUserInfo.stPermissions.stMenuPermission.bEventConfig = true;
    stUserInfo.stPermissions.stMenuPermission.bVideoManage = true;
    stUserInfo.stPermissions.stMenuPermission.bObjectLib = true;
    stUserInfo.stPermissions.stMenuPermission.bFaceConfig = true;
    stUserInfo.stPermissions.stMenuPermission.bVehicleDetecConfig = true;
    stUserInfo.stPermissions.stMenuPermission.bLocalShutdown = true;

    stUserInfo.stPermissions.stOperatePermission.bPTZControl = true;
    stUserInfo.stPermissions.stOperatePermission.bRound = true;
    stUserInfo.stPermissions.stOperatePermission.bTalk = true;
    stUserInfo.stPermissions.stOperatePermission.bRecord = true;
    stUserInfo.stPermissions.stOperatePermission.bRestart = true;
    stUserInfo.stPermissions.stOperatePermission.bSimpleRecovery = true;
    stUserInfo.stPermissions.stOperatePermission.bFullRecovery = true;
    stUserInfo.stPermissions.stOperatePermission.bParameterDerivation = true;
    stUserInfo.stPermissions.stOperatePermission.bUpgrade = true;
    stUserInfo.stPermissions.stOperatePermission.bActionAlarm = true;

    /* 添加激活用户 */
    ::User::UpdateInfo_S stUserUpdateInfo;
    stUserUpdateInfo.stAccountInfo = stUserInfo.stAccountInfo;
    stUserUpdateInfo.stNewUserInfo = stUserInfo;
    int nRet = CUserManage::instance()->update(stUserUpdateInfo,stSecurityServicesInfo);

    if (nRet < 0)
    {
        stActivationInfo.bEnActivated = false;
    }
    else
    {
        dlog_info("用户激活成功！");
        stActivationInfo.bEnActivated = true;
        /* 获取用户默认密码写入 */
        if (stActivationInfo.bEnSameDevice)
        {
            stActivationInfo.strIpcPwd = stActivationInfo.strUserPwd;
        }

        /* 激活用户信息写入文件 */
        Convert::write_file(ACTIVATE_INFO_FILE, stActivationInfo);

        /* 操作日志-激活设备 */
        Log::Info_S stLogInfo;
        stLogInfo.nType = Log::OPERATION;
        stLogInfo.user = m_user;
        stLogInfo.nAction = Log::LOCAL_ACTIVATE_DEVICE;
        LogHandler::instance()->write(stLogInfo);
    }

    result(nRet);
}

void Task::Register::GetActivationInfo::handle()
{
    dlog_info("获取激活信息");
    ::Register::ActivationPasswdInfo_S stActivationInfo;
    Convert::read_file(ACTIVATE_INFO_FILE, stActivationInfo);
    result(Convert::to_string(stActivationInfo));
}

void Task::Register::GetTimeInfo::handle()
{
    dlog_info("激活-获取时间信息");
    std::ifstream file(TIME_CONFIG_FILE);
    ::System::TimeInfo_S stTimeInfo;

    if (!file)
    {
        /* 初始化配置文件信息 */
        stTimeInfo.enTimeZone = ::System::TimeZone_E::UTC_PLUS_8;
        stTimeInfo.enDateFormat = ::System::DateFormat_E::YYYY_MM_DD;
        stTimeInfo.strDateTime = CTimeManage::instance()->get_current_time(::System::Language_E::ENGLISH, stTimeInfo.enDateFormat);
        stTimeInfo.bEnableNTPSync = false;
        Convert::write_file(TIME_CONFIG_FILE, stTimeInfo);
    }
    Convert::read_file(TIME_CONFIG_FILE, stTimeInfo);
    stTimeInfo.strDateTime = CTimeManage::instance()->get_current_time(::System::Language_E::ENGLISH, stTimeInfo.enDateFormat);
    result(Convert::to_string(stTimeInfo));
}

void Task::Register::ManaualConfigTime::handle()
{
    int nRet;
    ::System::TimeInfo_S stTimeInfo;
    dlog_info("激活-手动配置时间");
    Convert::to_struct(m_taskData, stTimeInfo);
    nRet = CTimeManage::instance()->set_time_info(stTimeInfo);
    result(nRet);
}

void Task::Register::ManaualConfigNetWork::handle()
{
    int nRet;
    ::Register::NetWorkInfo_S stNetWorkInfo;
    dlog_info("手动配置网络");
    Convert::to_struct(m_taskData, stNetWorkInfo);
    nRet = CNetworkManage::instance()->register_manual_config(stNetWorkInfo);
    result(nRet);
}

void Task::Register::AutoConfigNetwork::handle()
{
    /* 网络自动配置-注释 */
    // int nRet = CNetManage::instance()->register_auto_config();
    int nRet = 0;
    if (nRet < 0)
    {
        result(ERR_AUTO_CONFIG_NETWORK);
    }
    else
    {
        result(OK_AUTO_CONFIG_NETWORK);
    }
}