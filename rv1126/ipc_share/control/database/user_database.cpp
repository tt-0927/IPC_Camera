/***
 * @FilePath     : user_database.cpp
 * @Author       : zjc
 * @Date         : 2022-11-05 11:39:33
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-28 11:40:09
 * @Description  : 用户数据库
 */

#include "user_database.h"

#include <iostream>

using namespace Db;
CUserDatabase::CUserDatabase()
    : m_database(USER_DATABASE_PATH, USER_TABLE_NAME)
{
    create();
}

CUserDatabase::~CUserDatabase()
{
}

int CUserDatabase::create()
{
    m_database.add_tableKey(TableKey(USER_FIELD_ACCOUNT, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    m_database.add_tableKey(TableKey(USER_FIELD_PASSWORD, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    m_database.add_tableKey(TableKey(USER_FIELD_PATTREN_PASSWORD, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    m_database.add_tableKey(TableKey(USER_FIELD_ACCOUNT_STATUS, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(USER_FIELD_ACCOUNT_TYPE, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(USER_FIELD_ACCOUNT_SAFETY, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(USER_FIELD_LOGO_FIRST_TIME, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    m_database.add_tableKey(TableKey(USER_FIELD_NAME, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    m_database.add_tableKey(TableKey(USER_FIELD_PHONE_NUMBER, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    m_database.add_tableKey(TableKey(USER_FIELD_LOGIN_CNT, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(USER_FIELD_LOGO_PATH, CDbBase::type_string(MAX_DB_STRING_SIZE)));

    m_database.add_tableKey(TableKey(PERMISSION_FIELD_PREVIEW, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_PLAYBACK, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_RETRIEVE, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_ADHIBITION, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_WEB_LOCAL_CONFIG, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_SYSTEM_CONFIG, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_NETWORK_CONFIG, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_CHANNEL_MANAGE, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_VIDEO_AND_AUDIO, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_EVENT_CONFIG, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_VIDEO_MANAGE, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_OBJECT_LIB, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_FACE_CONFIG, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_VEHICLE_DETECT_CONFIG, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_LOCAL_SHUTDOWN, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_PTZ_CONTROL, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_ROUND, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_TALK, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_RECORD, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_RESTART, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_SIMPLE_RECOVERY, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_FULL_RECOVERY, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_PARAMETER_DERIVATION, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_UPGRADE, CDbBase::type_int()));
    m_database.add_tableKey(TableKey(PERMISSION_FIELD_ACTION_ALARM, CDbBase::type_int()));

    m_database.init();

    /* 去掉默认用户密码设置 */
    User::UserInfo_S stUserInfo;
    stUserInfo.stAccountInfo.account = USER_DEFAULT_NAME;
    stUserInfo.stAccountInfo.password = USER_DEFAULT_PASSWD;
    /*安全等级*/
    stUserInfo.stAccountInfo.nSafety =  User::Safety_E::MIDDLE_LEVEL;

    std::vector<User::UserInfo_S> userInfos;
    find(Element(USER_FIELD_ACCOUNT, stUserInfo.stAccountInfo.account), userInfos);
    if (userInfos.size() == 0)
    {
        /* 默认全部权限 */
        stUserInfo.stPermissions.stMenuPermission.bPreview = true;                /* 预览 */
        stUserInfo.stPermissions.stMenuPermission.bPlayback = true;               /* 回放 */
        stUserInfo.stPermissions.stMenuPermission.bRetrieve = true;               /* 检索 */
        stUserInfo.stPermissions.stMenuPermission.bAdhibition = true;             /* 应用 */
        stUserInfo.stPermissions.stMenuPermission.bWebLocalConfig = true;         /* WEB端本地配置 */
        stUserInfo.stPermissions.stMenuPermission.bSystemConfig = true;           /* 系统配置 */
        stUserInfo.stPermissions.stMenuPermission.bNetworkConfig = true;          /* 网络配置 */
        stUserInfo.stPermissions.stMenuPermission.bChannelManage = true;          /* 通道管理 */
        stUserInfo.stPermissions.stMenuPermission.bVideoAndAudio = true;          /* 视音频 */
        stUserInfo.stPermissions.stMenuPermission.bEventConfig = true;            /* 事件配置 */
        stUserInfo.stPermissions.stMenuPermission.bVideoManage = true;            /* 录像管理 */
        stUserInfo.stPermissions.stMenuPermission.bObjectLib = true;              /* 目标库 */
        stUserInfo.stPermissions.stMenuPermission.bFaceConfig = true;             /* 人脸功能 */
        stUserInfo.stPermissions.stMenuPermission.bVehicleDetecConfig = true;     /* 车辆检测配置 */
        stUserInfo.stPermissions.stMenuPermission.bLocalShutdown = true;          /* 本地关机 */
        stUserInfo.stPermissions.stOperatePermission.bPTZControl = true;          /* 云台控制 */
        stUserInfo.stPermissions.stOperatePermission.bRound = true;               /* 轮询 */
        stUserInfo.stPermissions.stOperatePermission.bTalk = true;                /* 对讲 */
        stUserInfo.stPermissions.stOperatePermission.bRecord = true;              /* 录像 */
        stUserInfo.stPermissions.stOperatePermission.bRestart = true;             /* 重启 */
        stUserInfo.stPermissions.stOperatePermission.bSimpleRecovery = true;      /* 简单恢复 */
        stUserInfo.stPermissions.stOperatePermission.bFullRecovery = true;        /* 完全恢复 */
        stUserInfo.stPermissions.stOperatePermission.bParameterDerivation = true; /* 设备参数导出 */
        stUserInfo.stPermissions.stOperatePermission.bUpgrade = true;             /* 升级 */
        stUserInfo.stPermissions.stOperatePermission.bActionAlarm = true;         /* 报警联动 */
        add(stUserInfo);
    }
    return 0;
}

int CUserDatabase::add(const User::UserInfo_S &stInfo)
{
    Item item;
    item.push_back(Element(USER_FIELD_ACCOUNT, stInfo.stAccountInfo.account));
    item.push_back(Element(USER_FIELD_PASSWORD, stInfo.stAccountInfo.password));
    item.push_back(Element(USER_FIELD_ACCOUNT_STATUS, stInfo.stAccountInfo.nAccountStatus));
    item.push_back(Element(USER_FIELD_PATTREN_PASSWORD, stInfo.stAccountInfo.patternPassword));
    item.push_back(Element(USER_FIELD_ACCOUNT_TYPE, stInfo.stAccountInfo.nAccountType));
    item.push_back(Element(USER_FIELD_ACCOUNT_SAFETY, stInfo.stAccountInfo.nSafety));
    item.push_back(Element(USER_FIELD_LOGO_FIRST_TIME, stInfo.stAccountInfo.strFirstLoginTime));
    item.push_back(Element(USER_FIELD_NAME, stInfo.stBindInfo.name));
    item.push_back(Element(USER_FIELD_PHONE_NUMBER, stInfo.stBindInfo.phoneNumber));
    item.push_back(Element(USER_FIELD_LOGIN_CNT, stInfo.stAccountInfo.nLoginCnt));
    item.push_back(Element(USER_FIELD_LOGO_PATH, stInfo.stBindInfo.logoPath));

    item.push_back(Element(PERMISSION_FIELD_PREVIEW, static_cast<int>(stInfo.stPermissions.stMenuPermission.bPreview)));
    item.push_back(Element(PERMISSION_FIELD_PLAYBACK, static_cast<int>(stInfo.stPermissions.stMenuPermission.bPlayback)));
    item.push_back(Element(PERMISSION_FIELD_RETRIEVE, static_cast<int>(stInfo.stPermissions.stMenuPermission.bRetrieve)));
    item.push_back(Element(PERMISSION_FIELD_ADHIBITION, static_cast<int>(stInfo.stPermissions.stMenuPermission.bAdhibition)));
    item.push_back(Element(PERMISSION_FIELD_WEB_LOCAL_CONFIG, static_cast<int>(stInfo.stPermissions.stMenuPermission.bWebLocalConfig)));
    item.push_back(Element(PERMISSION_FIELD_SYSTEM_CONFIG, static_cast<int>(stInfo.stPermissions.stMenuPermission.bSystemConfig)));
    item.push_back(Element(PERMISSION_FIELD_NETWORK_CONFIG, static_cast<int>(stInfo.stPermissions.stMenuPermission.bNetworkConfig)));
    item.push_back(Element(PERMISSION_FIELD_CHANNEL_MANAGE, static_cast<int>(stInfo.stPermissions.stMenuPermission.bChannelManage)));
    item.push_back(Element(PERMISSION_FIELD_VIDEO_AND_AUDIO, static_cast<int>(stInfo.stPermissions.stMenuPermission.bVideoAndAudio)));
    item.push_back(Element(PERMISSION_FIELD_EVENT_CONFIG, static_cast<int>(stInfo.stPermissions.stMenuPermission.bEventConfig)));
    item.push_back(Element(PERMISSION_FIELD_VIDEO_MANAGE, static_cast<int>(stInfo.stPermissions.stMenuPermission.bVideoManage)));
    item.push_back(Element(PERMISSION_FIELD_OBJECT_LIB, static_cast<int>(stInfo.stPermissions.stMenuPermission.bObjectLib)));
    item.push_back(Element(PERMISSION_FIELD_FACE_CONFIG, static_cast<int>(stInfo.stPermissions.stMenuPermission.bFaceConfig)));
    item.push_back(Element(PERMISSION_FIELD_VEHICLE_DETECT_CONFIG, static_cast<int>(stInfo.stPermissions.stMenuPermission.bVehicleDetecConfig)));
    item.push_back(Element(PERMISSION_FIELD_LOCAL_SHUTDOWN, static_cast<int>(stInfo.stPermissions.stMenuPermission.bLocalShutdown)));

    item.push_back(Element(PERMISSION_FIELD_PTZ_CONTROL, static_cast<int>(stInfo.stPermissions.stOperatePermission.bPTZControl)));
    item.push_back(Element(PERMISSION_FIELD_ROUND, static_cast<int>(stInfo.stPermissions.stOperatePermission.bRound)));
    item.push_back(Element(PERMISSION_FIELD_TALK, static_cast<int>(stInfo.stPermissions.stOperatePermission.bTalk)));
    item.push_back(Element(PERMISSION_FIELD_RECORD, static_cast<int>(stInfo.stPermissions.stOperatePermission.bRecord)));
    item.push_back(Element(PERMISSION_FIELD_RESTART, static_cast<int>(stInfo.stPermissions.stOperatePermission.bRestart)));
    item.push_back(Element(PERMISSION_FIELD_SIMPLE_RECOVERY, static_cast<int>(stInfo.stPermissions.stOperatePermission.bSimpleRecovery)));
    item.push_back(Element(PERMISSION_FIELD_FULL_RECOVERY, static_cast<int>(stInfo.stPermissions.stOperatePermission.bFullRecovery)));
    item.push_back(Element(PERMISSION_FIELD_PARAMETER_DERIVATION, static_cast<int>(stInfo.stPermissions.stOperatePermission.bParameterDerivation)));
    item.push_back(Element(PERMISSION_FIELD_UPGRADE, static_cast<int>(stInfo.stPermissions.stOperatePermission.bUpgrade)));
    item.push_back(Element(PERMISSION_FIELD_ACTION_ALARM, static_cast<int>(stInfo.stPermissions.stOperatePermission.bActionAlarm)));

    item.push_back(Element(DB_COMMON_FIELD_RESERVE1, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE2, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE3, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE4, std::string()));
    return m_database.add(item);
}

int CUserDatabase::find(const Element &elem, std::vector<User::UserInfo_S> &infos)
{
    MatchMethods methods;
    methods.push_back(MatchMethod(elem, FIND_CRITERION_EQ));
    find(methods, infos);

    return 0;
}

int CUserDatabase::find(const MatchMethods &methods, std::vector<User::UserInfo_S> &infos)
{
    std::vector<Item> items;
    m_database.find(methods, items);

    for (Item &item : items)
    {
        User::UserInfo_S stInfo;
        for (Element &pair : item)
        {
            std::string &key = pair.first;
            FieldValue &value = pair.second;
            switch (str2tag(key.c_str()))
            {
            case str2tag(DB_COMMON_FIELD_ID):
                stInfo.nId = mpark::get<int>(value);
                break;
            case str2tag(USER_FIELD_ACCOUNT):
                stInfo.stAccountInfo.account = mpark::get<std::string>(value);
                break;
            case str2tag(USER_FIELD_PASSWORD):
                stInfo.stAccountInfo.password = mpark::get<std::string>(value);
                break;
            case str2tag(USER_FIELD_PATTREN_PASSWORD):
                stInfo.stAccountInfo.patternPassword = mpark::get<std::string>(value);
                break;
            case str2tag(USER_FIELD_ACCOUNT_STATUS):
                stInfo.stAccountInfo.nAccountStatus = mpark::get<int>(value);
                break;
            case str2tag(USER_FIELD_ACCOUNT_TYPE):
                stInfo.stAccountInfo.nAccountType = mpark::get<int>(value);
                break;
            case str2tag(USER_FIELD_ACCOUNT_SAFETY):
                stInfo.stAccountInfo.nSafety = mpark::get<int>(value);
                break;
            case str2tag(USER_FIELD_LOGO_FIRST_TIME):
                stInfo.stAccountInfo.strFirstLoginTime = mpark::get<std::string>(value);
                break;
            case str2tag(USER_FIELD_NAME):
                stInfo.stBindInfo.name = mpark::get<std::string>(value);
                break;
            case str2tag(USER_FIELD_PHONE_NUMBER):
                stInfo.stBindInfo.phoneNumber = mpark::get<std::string>(value);
                break;
            case str2tag(USER_FIELD_LOGIN_CNT):
                stInfo.stAccountInfo.nLoginCnt = mpark::get<int>(value);
                break;
            case str2tag(USER_FIELD_LOGO_PATH):
                stInfo.stBindInfo.logoPath = mpark::get<std::string>(value);
                break;
            case str2tag(PERMISSION_FIELD_PREVIEW):
                stInfo.stPermissions.stMenuPermission.bPreview = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_PLAYBACK):
                stInfo.stPermissions.stMenuPermission.bPlayback = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_RETRIEVE):
                stInfo.stPermissions.stMenuPermission.bRetrieve = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_ADHIBITION):
                stInfo.stPermissions.stMenuPermission.bAdhibition = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_WEB_LOCAL_CONFIG):
                stInfo.stPermissions.stMenuPermission.bWebLocalConfig = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_SYSTEM_CONFIG):
                stInfo.stPermissions.stMenuPermission.bSystemConfig = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_NETWORK_CONFIG):
                stInfo.stPermissions.stMenuPermission.bNetworkConfig = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_CHANNEL_MANAGE):
                stInfo.stPermissions.stMenuPermission.bChannelManage = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_VIDEO_AND_AUDIO):
                stInfo.stPermissions.stMenuPermission.bVideoAndAudio = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_EVENT_CONFIG):
                stInfo.stPermissions.stMenuPermission.bEventConfig = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_VIDEO_MANAGE):
                stInfo.stPermissions.stMenuPermission.bVideoManage = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_OBJECT_LIB):
                stInfo.stPermissions.stMenuPermission.bObjectLib = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_FACE_CONFIG):
                stInfo.stPermissions.stMenuPermission.bFaceConfig = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_VEHICLE_DETECT_CONFIG):
                stInfo.stPermissions.stMenuPermission.bVehicleDetecConfig = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_LOCAL_SHUTDOWN):
                stInfo.stPermissions.stMenuPermission.bLocalShutdown = mpark::get<int>(value);
                break;

            case str2tag(PERMISSION_FIELD_PTZ_CONTROL):
                stInfo.stPermissions.stOperatePermission.bPTZControl = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_ROUND):
                stInfo.stPermissions.stOperatePermission.bRound = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_TALK):
                stInfo.stPermissions.stOperatePermission.bTalk = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_RECORD):
                stInfo.stPermissions.stOperatePermission.bRecord = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_RESTART):
                stInfo.stPermissions.stOperatePermission.bRestart = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_SIMPLE_RECOVERY):
                stInfo.stPermissions.stOperatePermission.bSimpleRecovery = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_FULL_RECOVERY):
                stInfo.stPermissions.stOperatePermission.bFullRecovery = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_PARAMETER_DERIVATION):
                stInfo.stPermissions.stOperatePermission.bParameterDerivation = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_UPGRADE):
                stInfo.stPermissions.stOperatePermission.bUpgrade = mpark::get<int>(value);
                break;
            case str2tag(PERMISSION_FIELD_ACTION_ALARM):
                stInfo.stPermissions.stOperatePermission.bActionAlarm = mpark::get<int>(value);
                break;
            default:
                break;
            }
        }
        infos.push_back(stInfo);
    }
    return 0;
}

int CUserDatabase::update(const Item &item, const MatchMethods &methods)
{
    return m_database.update(item, methods);
}

int CUserDatabase::del(const Item &item)
{
    return m_database.del(item);
}
