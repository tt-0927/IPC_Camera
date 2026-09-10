/**
 * @file Snmp.h
 * @author tianl (tianl@kfb.cn)
 * @date 2024-10-16
 * 
 * @brief SNMP类定义
 */

#pragma once
#include <string>
#include <stdio.h>
#include <signal.h>
#include <cstdlib>
#include <atomic>
#include <thread>

#include <agent_pp/snmp_group.h>
#include <agent_pp/system_group.h>
#include <agent_pp/snmp_target_mib.h>
#include <agent_pp/snmp_notification_mib.h>
#include <agent_pp/snmp_community_mib.h>
#include <agent_pp/notification_originator.h>
#include <agent_pp/snmp_proxy_mib.h>
#include <agent_pp/vacm.h>
#include <agent_pp/v3_mib.h>

#include <snmp_pp/oid_def.h>
#include <snmp_pp/octet.h>
#include <snmp_pp/mp_v3.h>
#include <snmp_pp/log.h>
#include "dlog.h"
#include "Singleton.h"
#include "network_define.h"
#include "system_manage.h"

using namespace Snmp_pp;
using namespace Agentpp;
/*  服务器发送端口 */
#define SERVER_PORT 161
/* 	代理软件发送trap消息端口 */
#define TRAP_PORT 162

/* V1/V2版本读权限组 */
#define V1V2GROUP_READ   "v1v2group_read"
/* V1/V2版本读写权限组 */
#define V1V2GROUP_WRITE  "v1v2group_write"
/* V3版本读权限组 */
#define V3GROUP_READ     "v3group_read"
/* V3版本读写权限组 */
#define V3GROUP_WRITE    "v3group_write"

/* 读视图 */
#define READ_VIEW       "ReadView"
/* 写视图 */
#define WRITE_VIEW      "WriteView"
/* 通知视图 */
#define NOTIFY_VIEW     "NotifyView"
/* 树节点 */
#define OIDTREE "1.3"
/* 系统信息OID */
#define SYSGROUP_OID "1.3.6.1.4.1.4976"
/* 系统描述信息 */
#define SYSDES "ITC rk3576 NVR SNMP message of sysDesr"



/**
 * @brief Snmp代理进程数据
 */
typedef struct {
    /// @brief MIB管理信息库
    Mib* pMib;
    ///请求列表               
    RequestList* pReqList;

    /// @brief  访问策略
    Vacm* pVacm;

    /// @brief V3版本用户信息表
    UsmUserTable *pUut;

    /// @brief V3版本的安全加密设置
    v3MP *pV3mp;
    /// @brief 请求
    Request* pReq;   
} SnmpData_S;

class CSnmpManage : public CSingleton<CSnmpManage>
{
    CSnmpManage();
private:
    /// @brief snmp配置信息
    Network::SnmpConfig_S stSnmpConf;
	
    /// @brief snmp代理数据
    SnmpData_S stSnmpData;

    /// @brief SNMP代理线程
    std::unique_ptr<std::thread> snmpThread;

    /// @brief 表示代理是否在运行 
    std::atomic<bool> bEnableRunning;

     /* 设备信息 */
     ::System::DeviceInfo_S m_deviceInfo;
    /* trap地址 */
     std::string m_mibTrapAddress; 

    /**
	 * @brief 配置文件
	 */
	std::string m_configFile;

   /**
    * '@brief 设置V1/V2版本用户
    * @return int 0成功，-1失败
    */
    int setSnmpV1V2User();

    /**
     * @brief 设置V3版本用户
     * @param engineID SNMP驱动ID
     * @return int 0成功，-1失败
     */
    int setSnmpV3User(const NS_SNMP OctetStr& engineID);

    /**
     * @brief 启动代理线程
     */
    int startSnmpAgent();

    /**
     * @brief MIB信息管理库初始化
     * @param engineID  SNMP驱动ID
     * @return int 0成功，-1失败
     */
    int mibInit(Mib& mib,const NS_SNMP OctetStr& engineID); 

   /**
    * @brief 设置V1/V2版本读用户
    * @return int 0成功，-1失败
    */
   int initReadCommunities();

   /**
    * @brief 设置V1/V2版本读写用户
    * @return int 0成功，-1失败
    */
   int initWriteCommunities();

public:
    
	 ~CSnmpManage() = default;
    friend class CSingleton<CSnmpManage>;

    /**
     * @brief snmp服务器初始化
     * @return int 
     */
    int init();
    /**
     * @brief 获取Snmp配置信息
     * @param stSnmpNewconfig 
     */
    void getSnmpInfo(Network::SnmpConfig_S& stSnmpConfig);
    /**
     * @brief 初始化SNMP代理 并启动代理
     * @param stSnmpNewconfig 代理信息
     * @return int 0成功，-1失败
     */
    int initializeSnmp(const Network::SnmpConfig_S& stSnmpNewconfig);

     /**
     * @brief 关闭代理线程
     */
    void stopSnmpAgent(); 
    /**
     * @brief 获取设备信息
     * @return ::System::DeviceInfo_S 
     */
    ::System::DeviceInfo_S getDeviceInfo();
    /**
     * @brief 获取Trap 地址
     * @return std::string 
     */
    std::string getTrapAdress();

};


