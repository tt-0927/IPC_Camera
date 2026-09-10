/**
 * @file register_define.h
 * @author tianl (tianl@kfb.cn)
 * @date 2024-10-30
 *
 * @brief 注册激活配置数据定义
 */

#pragma once

#include <string>
#include <vector>

namespace Register
{
    /**
     * @brief 有效期
     */
    typedef enum
    {
        AT_EXPIRED  = -2, /* 已过期 */
        AT_NULL     = -1, /* 未注册/激活 */
        AT_ONE_WEEK = 0,  /* 一周 */
        AT_ONE_MONTH,     /* 一月 */
        AT_TWO_MONTH,     /* 两月 */
        AT_THREE_MONTH,   /* 三月 */
        AT_HALF_YEAR,     /* 半年 */
        AT_FOREVER        /* 永久 */
    } ActivationTime_E;

    /**
     * @brief 注册信息
     */
    typedef struct _RegisterInfo_S_
    {
        std::string strDevID;                       /* 设备id,从硬件读出来 一共16位 */
        std::string strMachinSn;                    /* 机器码，根据设置id自己组成对应格式机器码 */
        std::string strRegisterEg;                  /* 根据机器码生成的注册码，有试用版本注册码和永久注册码 */
        std::string strStartTime;                   /* 注册时间 */
        std::string strLatestCheckTime;             /* 最后一次检查时间 */
        long long lnLifeTimer = 0;                  /* 可用时长 分钟*/
        ActivationTime_E enActionTime = AT_NULL;    /* 有效期 */
    } RegisterInfo_S;

    /**
     * @brief 设置注册码
     */
    typedef struct
    {
        std::string strRegisterEg; /* 根据机器码生成的注册码，有试用版本注册码和永久注册码 */
    } ConfigRegisterEg_S;

    /**
     * @brief 激活密码配置
     */
    typedef struct
    {
        bool        bEnActivated  = false;   /* 设备是否已激活 */
        std::string strUser       = "admin"; /* 用户名 */
        std::string strUserPwd;              /* 用户密码 */
        int         nSafety       = 0;       /* 安全性 */
        std::string strIpcPwd;               /* 摄像机激活密码 */
        bool        bEnSameDevice = false;   /* 是否与设备密码保持一致 */
    } ActivationPasswdInfo_S;

    /**
     * @brief 配置模式
     */
    typedef enum
    {
        AUTOMODE = 1 /* 自动配置 */
    } ConfigMode_E;


    /**
     * @brief 自动配置状态
     */
    typedef struct
    {
        bool bEnNetworkStatus = false; /* 网络配置状态 */
        bool bEnChannelStatus = false; /* 通道配置状态 */
    } AutoConfigStatus_S;

    typedef struct
    {
        bool        bEnableDhcp = false; /* 是否开启dhcp */
        std::string strIpv4Ip;           /* ipv4地址 */
        std::string strIpv4Mask;         /* ipv4掩码 */
        std::string strIpv4Gateway;      /* ipv4网关 */
    }RegisterIp_S;

    /**
     * @brief 手动配置网络信息
     */
    typedef struct
    {
        RegisterIp_S stRegIp;             /* 网卡信息 */
        bool         bEnAutoDns  = false; /* 是否开启自动获取DNS服务器 */
        std::string  strDnsMain;          /* 首选dns */
        std::string  strDnsStandby;       /* 备选nds */
    } NetWorkInfo_S;
}
