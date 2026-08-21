/***
 * @FilePath     : user_define.h
 * @Author       : huangjunda
 * @Date         : 2025-03-28 10:41:11
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-28 10:41:26
 * @Description  :
 */
#pragma once

#include <string>

#include "common_define.h"
#include <vector>

#define USER_DEFAULT_NAME    "admin"
#define USER_DEFAULT_PASSWD  "zfrl@168"

namespace User
{
    typedef enum AccountStatus
    {
        /* 正常 */
        ACCOUNT_STATUS_NORMAL  = 0,
        /* 停用 */
        ACCOUNT_STATUS_FROZEN  = 1,
        /* 离线 */
        ACCOUNT_STATUS_OFFLINE = 2,
        /* 在线 */
        ACCOUNT_STATUS_ONLINE  = 3,
        /* 锁定 */
        ACCOUNT_STATUS_LOCKED  = 4
    } AccountStatus_E;

    /**
     * @brief 用户类型
     */
    typedef enum AccountType
    {
        /* 管理员 */
        ACCOUNT_TYPE_ADMIN = 0,
        /* 操作员 */
        ACCOUNT_TYPE_ORDINARY = 1,
        /* 视频用户 */
        ACCOUNT_TYPE = 2
    } AccountType_E;

    /**
     * @brief 安全性
     */
    typedef enum Safety
    {
        /* 低级 */
        LOW_LEVEL = 1,
        /* 中级 */
        MIDDLE_LEVEL = 2,
        /* 高级 */
        HIGH_LEVEL = 3,
        /* 极高 */
        SKYHIGH_LEVEL = 4
    } Safety_E;

    /**
     * @brief 菜单权限
     */
    typedef struct
    {
        bool bPreview = false;            /* 预览 */
        bool bPlayback = false;           /* 回放 */
        bool bRetrieve = false;           /* 检索 */
        bool bAdhibition = false;         /* 应用 */
        bool bWebLocalConfig = false;     /* WEB端本地配置 */
        bool bSystemConfig = false;       /* 系统配置 */
        bool bNetworkConfig = false;      /* 网络配置 */
        bool bChannelManage = false;      /* 通道管理 */
        bool bVideoAndAudio = false;      /* 视音频 */
        bool bEventConfig = false;        /* 事件配置 */
        bool bVideoManage = false;        /* 录像管理 */
        bool bObjectLib = false;          /* 目标库 */
        bool bFaceConfig = false;           /*人脸功能 */
        bool bVehicleDetecConfig = false; /* 车辆检测配置 */
        bool bLocalShutdown = false;      /* 本地关机 */
    } MenuPermission_S;

    /**
     * @brief 操作权限
     */
    typedef struct
    {
        bool bPTZControl = false;          /* 云台控制 */
        bool bRound = false;               /* 轮询 */
        bool bTalk = false;                /* 对讲 */
        bool bRecord = false;              /* 录像 */
        bool bRestart = false;             /* 重启 */
        bool bSimpleRecovery = false;      /* 简单恢复 */
        bool bFullRecovery = false;        /* 完全恢复 */
        bool bParameterDerivation = false; /* 设备参数导出 */
        bool bUpgrade = false;             /* 升级 */
        bool bActionAlarm = false;         /* 报警联动 */
    } OperatePermission_S;

    /**
     * @brief 用户权限
     */
    typedef struct
    {
        /* 菜单权限 */
        MenuPermission_S stMenuPermission;
        /* 操作权限 */
        OperatePermission_S stOperatePermission;
    } UserPermissions_S;

    typedef struct Search
    {
        /* 账号角色类型 AccountType_E */
        int nAccountType = -1;
    } Search_S;

    /**
     * @brief 用户操作
     */
    typedef struct
    {
        std::string strUsername;  /* 用户名 */
        std::string strOperation; /* 操作 */
    } UserOperation_S;

    typedef struct UserFind
    {
        /* 查找类型 */
        Search_S stSearch;
        /* 页数据信息 */
        Common::PageInfo_S stPageInfo;
    } Find_S;

    /**
     * @brief 在线用户
     */
    typedef struct
    {
        int nOnlineUserId = 0;         /* 用户id 唯一 */
        std::string strUsername;       /* 用户名 */
        int nUserType = 0;             /* 用户类型 */
        std::string strIpAddress;      /* 登陆设备ip */
        std::string strLastActionTime; /* 最后操作时间 */
    } OnlineUser_S;

    typedef struct AccountInfo
    {
        /* 是否是web端操作 */
        bool bWeb = false;
        /* 账号 */
        std::string account = std::string();
        /* 密码 */
        std::string password = std::string();
        /* 图案密码 */
        std::string patternPassword = std::string();
        /* 登录次数 */
        int nLoginCnt = 0;
        /* 账号状态 */
        int nAccountStatus = 0;
        /* 安全性 */
        int nSafety = 0;
        /* 账号角色类型 */
        int nAccountType = 0;
        /* 更新账号密码时间 */
        std::string strFirstLoginTime = "";
        /* 当前时间 */
        std::string strNowTime = "";
        /* 在线用户信息 */
        OnlineUser_S stOnlineUser;
    } AccountInfo_S;

    typedef struct
    {
        /* 管理员密码，新增时使用 */
        std::string password;
    } AdminInfo_S;
    typedef struct UserBindInfo
    {
        /* 账号实名 */
        std::string name = std::string();
        /* 手机号 */
        std::string phoneNumber = std::string();
        /* 头像 */
        std::string logoPath = std::string();
    } BindInfo_S;

    /**
     * @brief 用户登录错误信息
     */
    typedef struct
    {
        int nErrorCount = 0;             /* 错误登录次数 */
        std::string strReleaseTime = ""; /* 解锁时间 */
        std::string strFirstRecordTime = "";  /* 第一次记录时间*/
    } LogErrorInfo_S;

    typedef struct UserInfo
    {
        /* 添加账号时自动生成的唯一id */
        int nId = -1;
        /* 用户账号信息 */
        AccountInfo_S stAccountInfo;
        /* 用户绑定信息 */
        BindInfo_S stBindInfo;
        /* 用户权限 */
        UserPermissions_S stPermissions;
    } UserInfo_S;

    /* 删除用户-推送 */
    typedef struct DeleUser
    {
        int nId = -1;
        std::string account;
    } DeleUser_S;

    /* 修改用户-推送 */
    typedef struct UpdateUserPush
    {
        std::string account;
        std::string pawssd;
    } UpdateUserPush_S;

    typedef struct UserUpdateInsfo
    {
        /* 需要修改的账号 */
        AccountInfo_S stAccountInfo;
        /* 应修改数据 */
        UserInfo_S stNewUserInfo;
        /* 是否检查新旧密码相同 */
        bool bCheckPassword = true;
    } UpdateInfo_S;

} /* namespace User */
