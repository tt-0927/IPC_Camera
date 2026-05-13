/***
 * @FilePath     : user_convert.cpp
 * @Author       : huangjunda
 * @Date         : 2025-03-28 11:04:34
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-28 11:05:22
 * @Description  :
 */

#include "user_convert.h"

#include "common_convert.h"
#include "convert.h" /* 这个要放在UserDefineConvert的后面 */

void Convert::deal(Json::Object *pRootJson, User::MenuPermission_S &stMenuPermission, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Preview", stMenuPermission.bPreview);
    convert.field(pRootJson, "Playback", stMenuPermission.bPlayback);
    convert.field(pRootJson, "Retrieve", stMenuPermission.bRetrieve);
    convert.field(pRootJson, "Adhibition", stMenuPermission.bAdhibition);
    convert.field(pRootJson, "WebLocalConfig", stMenuPermission.bWebLocalConfig);
    convert.field(pRootJson, "SystemConfig", stMenuPermission.bSystemConfig);
    convert.field(pRootJson, "NetworkConfig", stMenuPermission.bNetworkConfig);
    convert.field(pRootJson, "ChannelManage", stMenuPermission.bChannelManage);
    convert.field(pRootJson, "VideoAndAudio", stMenuPermission.bVideoAndAudio);
    convert.field(pRootJson, "EventConfig", stMenuPermission.bEventConfig);
    convert.field(pRootJson, "VideoManage", stMenuPermission.bVideoManage);
    convert.field(pRootJson, "ObjectLib", stMenuPermission.bObjectLib);
    convert.field(pRootJson, "VehicleDetecConfig", stMenuPermission.bVehicleDetecConfig);
    convert.field(pRootJson, "LocalShutdown", stMenuPermission.bLocalShutdown);
}

void Convert::deal(Json::Object *pRootJson, User::OperatePermission_S &stOperatePermission, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "PTZControl", stOperatePermission.bPTZControl);
    convert.field(pRootJson, "Round", stOperatePermission.bRound);
    convert.field(pRootJson, "Talk", stOperatePermission.bTalk);
    convert.field(pRootJson, "Record", stOperatePermission.bRecord);
    convert.field(pRootJson, "Restart", stOperatePermission.bRestart);
    convert.field(pRootJson, "SimpleRecovery", stOperatePermission.bSimpleRecovery);
    convert.field(pRootJson, "FullRecovery", stOperatePermission.bFullRecovery);
    convert.field(pRootJson, "ParameterDerivation", stOperatePermission.bParameterDerivation);
    convert.field(pRootJson, "Upgrade", stOperatePermission.bUpgrade);
    convert.field(pRootJson, "ActionAlarm", stOperatePermission.bActionAlarm);
}

void Convert::deal(Json::Object *pRootJson, User::UserPermissions_S &stPermissions, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);

    convert.structure(pRootJson, "MenuPermission", stPermissions.stMenuPermission);
    convert.structure(pRootJson, "OperatePermission", stPermissions.stOperatePermission);
}

/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, User::Search_S &stSearch, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "AccountType", stSearch.nAccountType);
}

/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, User::Find_S &stFind, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, stFind.stSearch);
    convert.structure(pRootJson, stFind.stPageInfo);
}

/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, User::AccountInfo_S &stAccountInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Account", stAccountInfo.account);
    convert.field(pRootJson, "Password", stAccountInfo.password);
    convert.field(pRootJson, "patternPassword", stAccountInfo.patternPassword);
    convert.field(pRootJson, "LoginCnt", stAccountInfo.nLoginCnt);
    convert.field(pRootJson, "AccountStatus", stAccountInfo.nAccountStatus);
    convert.field(pRootJson, "AccountType", stAccountInfo.nAccountType);
    convert.field(pRootJson, "Safety", stAccountInfo.nSafety);
    convert.field(pRootJson, "NowTime", stAccountInfo.strNowTime);

    convert.structure(pRootJson, stAccountInfo.stOnlineUser);
}
void Convert::deal(Json::Object *pRootJson, User::AdminInfo_S &stAdminInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "AdminPassword", stAdminInfo.password);
}

/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, User::BindInfo_S &stBindInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Name", stBindInfo.name);
    convert.field(pRootJson, "PhoneNumber", stBindInfo.phoneNumber);
    convert.field(pRootJson, "LogoPath", stBindInfo.logoPath);
}

/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, User::UserInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Id", stInfo.nId);
    convert.structure(pRootJson, stInfo.stAccountInfo);
    convert.structure(pRootJson, stInfo.stBindInfo);
    convert.structure(pRootJson, stInfo.stPermissions);
}

/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, std::vector<User::UserInfo_S> &vecInfos, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Infos", vecInfos);
}

/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, User::UpdateInfo_S &stUpdateInfo, bool bOutStruct)
{

    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, stUpdateInfo.stAccountInfo);
    convert.structure(pRootJson, "Update", stUpdateInfo.stNewUserInfo);
    convert.field(pRootJson, "CheckPassword", stUpdateInfo.bCheckPassword);
}

void Convert::deal(Json::Object *pRootJson, User::LogErrorInfo_S &stLogErrorInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "ErrorCount", stLogErrorInfo.nErrorCount);
    convert.field(pRootJson, "ReleaseTime", stLogErrorInfo.strReleaseTime);
}

void Convert::deal(Json::Object *pRootJson, User::OnlineUser_S &stOnlineUser, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "OnlineUserId", stOnlineUser.nOnlineUserId);
    convert.field(pRootJson, "Username", stOnlineUser.strUsername);
    convert.field(pRootJson, "UserType", stOnlineUser.nUserType);
    convert.field(pRootJson, "IpAddress", stOnlineUser.strIpAddress);
    convert.field(pRootJson, "LastActionTime", stOnlineUser.strLastActionTime);
}

void Convert::deal(Json::Object *pRootJson, std::vector<User::OnlineUser_S> &vecOlineUsers, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "OnlineUser", vecOlineUsers);
}

void Convert::deal(Json::Object *pRootJson, User::UserOperation_S &stUserOperation, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Username", stUserOperation.strUsername);
    convert.field(pRootJson, "Operation", stUserOperation.strOperation);
}

void Convert::deal(Json::Object *pRootJson, User::DeleUser_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "nId", stInfo.nId);
    convert.field(pRootJson, "DeleteAccount", stInfo.account);
}

void Convert::deal(Json::Object *pRootJson, User::UpdateUserPush_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Account", stInfo.account);
    convert.field(pRootJson, "Pawssd", stInfo.pawssd);
}