/**
 * @file Snmp.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2024-10-16
 * 
 * @brief 
 */

#include<iostream>
#include <agent_pp/snmp_request.h>
#include "snmp_manage.h"
#include "ipc_mib.h"
#include "network_convert.h"
#include "convert_interface.h"
#include "path_define.h"

using namespace  Snmp_pp;
using namespace Agentpp;

CSnmpManage::CSnmpManage()
: m_configFile(SNMP_CONFIG_FILE)
{
    ::SystemManage::instance()->get_device_info(m_deviceInfo);
}

void CSnmpManage::getSnmpInfo(Network::SnmpConfig_S& stSnmpConfig)
{
    Convert::read_file(m_configFile, stSnmpConfig);
}

 System::DeviceInfo_S CSnmpManage::getDeviceInfo()
 {
    return m_deviceInfo;
 }

std::string CSnmpManage::getTrapAdress()
{
    return m_mibTrapAddress;
}

int CSnmpManage::init()
{
    Network::SnmpConfig_S stSnmpConfig;
    Convert::read_file(m_configFile, stSnmpConfig);
    m_mibTrapAddress = stSnmpConfig.strTrapAddress;
    initializeSnmp(stSnmpConfig);
    return 0;
}

/* 初始化配置 */
int CSnmpManage::initializeSnmp(const Network::SnmpConfig_S& stSnmpNewconfig)
{
    this->stSnmpConf = stSnmpNewconfig;

/* 初始化SNMP库打印信息 */
#ifndef _NO_LOGGING
    DefaultLog::log()->set_filter(ERROR_LOG, 5);
    DefaultLog::log()->set_filter(WARNING_LOG, 5);
    DefaultLog::log()->set_filter(EVENT_LOG, 5);
    DefaultLog::log()->set_filter(INFO_LOG, 5);
    DefaultLog::log()->set_filter(DEBUG_LOG, 8);
#endif
    /* 开启snmp代理 */
    if (stSnmpConf.bEnableSnmp)
    {
        /* 检测是否正在运行代理进程 */
        if (bEnableRunning)
        {
            stopSnmpAgent();
        }
        bEnableRunning = true;
        snmpThread = std::make_unique<std::thread>(&CSnmpManage::startSnmpAgent, this);
     /* 关闭snmp代理 */   
    }
    else
    {
        if (bEnableRunning)
        {
            stopSnmpAgent();
        }
    }

    m_mibTrapAddress = stSnmpNewconfig.strTrapAddress;

    Convert::write_file(m_configFile, stSnmpConf);

    return 0;
}

/* 启动snmp代理 */
int CSnmpManage::startSnmpAgent() 
{

    /* snmp套接字初始化 */ 
    int nStatus;
    unsigned short port = stSnmpConf.nSnmpPort;
    Snmp::socket_startup(); 
    Snmpx cSnmpx(nStatus, port);

    if (nStatus == SNMP_CLASS_SUCCESS) 
    {
        dlog_info("SNMP 套接字初始化成功");
    } 
    else 
    {
        dlog_error("SNMP 套接字初始化失败");
        return -1;
    }

    /* 管理信息表初始化 */
    stSnmpData.pMib = new Mib(); 
    stSnmpData.pReqList = new RequestList(stSnmpData.pMib); 

    unsigned int nSnmpEngineBoots = 0;
    OctetStr cEngineId(SnmpEngineID::create_engine_id(port));

    /* 设置snmp启动次数 */ 
    nStatus = stSnmpData.pMib->get_boot_counter(cEngineId, nSnmpEngineBoots);
    if ((nStatus != SNMPv3_OK) && (nStatus < SNMPv3_FILEOPEN_ERROR)) 
    {
        dlog_error("加载snmpEngineBoots计数器错误");
        return -1;
    }

    /* 保存snmp启动次数 */ 
    nSnmpEngineBoots++;
    nStatus = stSnmpData.pMib->set_boot_counter(cEngineId, nSnmpEngineBoots);
    if (nStatus != SNMPv3_OK) 
    {
        dlog_error("保存snmpEngineBoots计数器错误");
        return -1;
    }

    int nStat;
    stSnmpData.pV3mp = new v3MP(cEngineId, nSnmpEngineBoots, nStat);
    /* 注册v3MP */ 
    stSnmpData.pReqList->set_v3mp(stSnmpData.pV3mp);
    cSnmpx.set_mpv3(stSnmpData.pV3mp);

    /* 请求列表关联到snmp代理 */ 
    stSnmpData.pReqList->set_snmp(&cSnmpx);
    /* 列表和MIB库关联 */
    stSnmpData.pMib->set_request_list(stSnmpData.pReqList);
    /* 信息库初始化 */
    mibInit(*(stSnmpData.pMib),cEngineId);
    
    /* 管理信息库添加转发代理 */
    ProxyForwarder* pProxy = new ProxyForwarder(stSnmpData.pMib, "", ProxyForwarder::ALL);
    stSnmpData.pMib->register_proxy(pProxy);
    /* 注册VACM */ 
    stSnmpData.pVacm = new Vacm(*(stSnmpData.pMib));
    stSnmpData.pReqList->set_vacm(stSnmpData.pVacm);
  
    stSnmpData.pVacm->addNewContext("");
   // stSnmpData.pVacm->addNewContext("other");
    if (!stSnmpConf.bEnableSmnpV3)
    {
        dlog_info("SNMP 开始设置V2/V1版本的用户");
        /* V1/V2 读用户组设置   */
        stSnmpData.pVacm->addNewGroup(SNMP_SECURITY_MODEL_V2, stSnmpConf.strReadCommunityName.c_str(),
        V1V2GROUP_READ, storageType_nonVolatile);
        stSnmpData.pVacm->addNewGroup(SNMP_SECURITY_MODEL_V1, stSnmpConf.strReadCommunityName.c_str(),
        V1V2GROUP_READ, storageType_nonVolatile);

        /* V1/V2 读写用户组设置  */
        stSnmpData.pVacm->addNewGroup(SNMP_SECURITY_MODEL_V2, stSnmpConf.strWriteCommunityName.c_str(),
        V1V2GROUP_WRITE, storageType_nonVolatile);
        stSnmpData.pVacm->addNewGroup(SNMP_SECURITY_MODEL_V1, stSnmpConf.strWriteCommunityName.c_str(),
        V1V2GROUP_WRITE, storageType_nonVolatile);

        /* 读用户组权限设置 */ 
        stSnmpData.pVacm->addNewAccessEntry(V1V2GROUP_READ, "",
        SNMP_SECURITY_MODEL_V2,
        SNMP_SECURITY_LEVEL_NOAUTH_NOPRIV,
        match_exact,
        READ_VIEW, "",
        "", storageType_nonVolatile);
        stSnmpData.pVacm->addNewAccessEntry(V1V2GROUP_READ, "",
        SNMP_SECURITY_MODEL_V1,
        SNMP_SECURITY_LEVEL_NOAUTH_NOPRIV,
        match_exact,
        READ_VIEW, "",
        "", storageType_nonVolatile);

        /* 读写用户组权限设置 */ 
        stSnmpData.pVacm->addNewAccessEntry(V1V2GROUP_WRITE, "",
        SNMP_SECURITY_MODEL_V2,
        SNMP_SECURITY_LEVEL_NOAUTH_NOPRIV,
        match_exact,
        READ_VIEW, WRITE_VIEW,
        NOTIFY_VIEW , storageType_nonVolatile);
        stSnmpData.pVacm->addNewAccessEntry(V1V2GROUP_WRITE, "",
        SNMP_SECURITY_MODEL_V1,
        SNMP_SECURITY_LEVEL_NOAUTH_NOPRIV,
        match_exact,
        READ_VIEW, WRITE_VIEW,
        NOTIFY_VIEW, storageType_nonVolatile);
    }
    else
    {
        dlog_info("SNMP 开始设置V3版本的用户");
        /* V3 读用户组设置  */ 
        stSnmpData.pVacm->addNewGroup(SNMP_SECURITY_MODEL_USM, stSnmpConf.strReadSecurityName.c_str(),
            V3GROUP_READ, storageType_nonVolatile); 

        /* V3 读写用户组设置  */
        stSnmpData.pVacm->addNewGroup(SNMP_SECURITY_MODEL_USM, stSnmpConf.strWriteSecurityName.c_str(),
            V3GROUP_WRITE, storageType_nonVolatile);

        /* V3 读用户组权限设置  */
        stSnmpData.pVacm->addNewAccessEntry(V3GROUP_READ, "",
            SNMP_SECURITY_MODEL_USM,
            SNMP_SECURITY_LEVEL_AUTH_PRIV,
            match_exact,
            READ_VIEW, "",
            "", storageType_nonVolatile);

        /* V3 读写用户组权限设置  */
        stSnmpData.pVacm->addNewAccessEntry(V3GROUP_WRITE, "",
            SNMP_SECURITY_MODEL_USM,
            SNMP_SECURITY_LEVEL_AUTH_PRIV,
            match_exact,
            READ_VIEW, WRITE_VIEW,
            NOTIFY_VIEW, storageType_nonVolatile);
   }
    dlog_info("SNMP 用户初始化成功！");
    stSnmpData.pVacm->addNewView(READ_VIEW,
          OIDTREE,
          "", 
          view_included, 
          storageType_nonVolatile);
  
    stSnmpData.pVacm->addNewView(WRITE_VIEW,
          OIDTREE,
          "", 
          view_included, 
          storageType_nonVolatile);

    stSnmpData.pVacm->addNewView(NOTIFY_VIEW,
          OIDTREE,
          "", 
          view_included,
          storageType_nonVolatile);

    stSnmpData.pMib->init();

    /* 发送trap消息 */
    //Vbx* pVbs = 0;
    //coldStartOid coldOid;
    //NotificationOriginator no;
    //UdpAddress dest("127.0.0.1/162");
    //no.add_v1_trap_destination(dest, "defaultV1Trap", "v1trap",  stSnmpConf.strWriteCommunityName.c_str());
    //no.generate(pVbs, 0, coldOid, "", "");

    Request* req;
    while (bEnableRunning) 
    {
        req = stSnmpData.pReqList->receive(2);

        if (req) 
        {
            stSnmpData.pMib->process_request(req);
        } 
        else 
        {
            stSnmpData.pMib->cleanup();
        }
    }

     /* 清理资源 */
    delete stSnmpData.pReqList;
    // delete stSnmpData.pMib;
    delete stSnmpData.pVacm;
    delete stSnmpData.pV3mp;
    Snmp::socket_cleanup();
    return 0; 
}

/* 停止代理 */
void CSnmpManage::stopSnmpAgent() 
{
    bEnableRunning = false;  
    if (snmpThread && snmpThread->joinable())
    {
        snmpThread->join();
    }
}

/* 管理信息库初始化 */
int CSnmpManage::mibInit(Mib& mib,const NS_SNMP OctetStr& engineID)
{
    dlog_info("SNMP 开始初始化MIB管理信息库");
    /* 添加设备信息描述 */ 
    OctetStr descr(SYSDES);
    mib.add(new sysGroup(descr.get_printable(),
            SYSGROUP_OID, 10));
    mib.add(new snmpGroup());

    /* 添加NVR的MIB信息管理库 */
    mib.add(new nvrGroup());

    mib.add(new TestAndIncr(oidSnmpSetSerialNo));
    
    /* 添加管理目标 */ 
    mib.add(new snmp_target_mib());

    /* 添加代理 */ 
    mib.add(new snmp_proxy_mib());

    /* 为MIB添加社区名 */ 
    mib.add(new snmp_community_mib());
    mib.add(new snmp_notification_mib());

    if (!stSnmpConf.bEnableSmnpV3)
    {
         /* 添加V1/V2版本的用户名 */
         setSnmpV1V2User();
    }
    else
    {
        /* 添加V3版本的用户名 */
        setSnmpV3User(engineID);
    }

    stSnmpData.pMib->add(new V3SnmpEngine());
    stSnmpData.pMib->add(new MPDGroup());

    return 0;
}

/* 设置V3版本用户 */
int CSnmpManage::setSnmpV3User(const NS_SNMP OctetStr& engineID)
{
     /* 获取USM对象 */ 
    stSnmpData.pUut = new UsmUserTable();

    /*** 设置V3版本的读安全设置 ***/
    /* MD5认证 DES加密 */
    if (stSnmpConf.stReadSecuritySettings.enAuth == Network::AUTH_MD5 && stSnmpConf.stReadSecuritySettings.enPriv == Network::PRIV_DES)
    {
        stSnmpData.pUut->addNewRow(stSnmpConf.strReadSecurityName.c_str(),SNMP_AUTHPROTOCOL_HMACMD5,SNMP_PRIVPROTOCOL_DES, stSnmpConf.stReadSecuritySettings.strAuthPassword.c_str(), stSnmpConf.stReadSecuritySettings.strPrivPassword.c_str(), engineID, false);

    /* MD5认证 AES加密 */  
    }
    else if (stSnmpConf.stReadSecuritySettings.enAuth == Network::AUTH_MD5 && stSnmpConf.stReadSecuritySettings.enPriv == Network::PRIV_AES)
    {
        
        stSnmpData.pUut->addNewRow(stSnmpConf.strReadSecurityName.c_str(),SNMP_AUTHPROTOCOL_HMACMD5,SNMP_PRIVPROTOCOL_AES128, stSnmpConf.stReadSecuritySettings.strAuthPassword.c_str(), stSnmpConf.stReadSecuritySettings.strPrivPassword.c_str(), engineID, false);

    /* SHA认证 DES加密 */
    }
    else if (stSnmpConf.stReadSecuritySettings.enAuth == Network::AUTH_SHA && stSnmpConf.stReadSecuritySettings.enPriv == Network::PRIV_DES)
    {
        stSnmpData.pUut->addNewRow(stSnmpConf.strReadSecurityName.c_str(),SNMP_AUTHPROTOCOL_HMACSHA,SNMP_PRIVPROTOCOL_DES, stSnmpConf.stReadSecuritySettings.strAuthPassword.c_str(), stSnmpConf.stReadSecuritySettings.strPrivPassword.c_str(), engineID, false);
       
    /* SHA认证 AES加密 */
    }
    else if (stSnmpConf.stReadSecuritySettings.enAuth == Network::AUTH_SHA && stSnmpConf.stReadSecuritySettings.enPriv == Network::PRIV_AES)
    {
        stSnmpData.pUut->addNewRow(stSnmpConf.strReadSecurityName.c_str(),SNMP_AUTHPROTOCOL_HMACSHA,SNMP_PRIVPROTOCOL_AES128, stSnmpConf.stReadSecuritySettings.strAuthPassword.c_str(), stSnmpConf.stReadSecuritySettings.strPrivPassword.c_str(), engineID, false);
       
    }
    else
    {
        dlog_error("SNMP V3 版本设置读安全共同体失败！");
        return -1;
    }
    
    /*** 设置V3版本的读写安全设置 ***/
    /* MD5认证 DES加密 */
    if (stSnmpConf.stWriteSecuritySettings.enAuth == Network::AUTH_MD5 && stSnmpConf.stWriteSecuritySettings.enPriv == Network::PRIV_DES)
    {
        stSnmpData.pUut->addNewRow(stSnmpConf.strWriteSecurityName.c_str(),SNMP_AUTHPROTOCOL_HMACMD5,SNMP_PRIVPROTOCOL_DES, stSnmpConf.stWriteSecuritySettings.strAuthPassword.c_str(), stSnmpConf.stWriteSecuritySettings.strPrivPassword.c_str(), engineID, false);

    /* MD5认证 AES加密  */ 
    }
    else if (stSnmpConf.stWriteSecuritySettings.enAuth == Network::AUTH_MD5 && stSnmpConf.stWriteSecuritySettings.enPriv == Network::PRIV_AES)
    {
        
        stSnmpData.pUut->addNewRow(stSnmpConf.strWriteSecurityName.c_str(),SNMP_AUTHPROTOCOL_HMACMD5,SNMP_PRIVPROTOCOL_AES128, stSnmpConf.stWriteSecuritySettings.strAuthPassword.c_str(), stSnmpConf.stWriteSecuritySettings.strPrivPassword.c_str(), engineID, false);

    /* SHA认证 DES加密 */
    }
    else if (stSnmpConf.stWriteSecuritySettings.enAuth == Network::AUTH_SHA && stSnmpConf.stWriteSecuritySettings.enPriv == Network::PRIV_DES)
    {
        stSnmpData.pUut->addNewRow(stSnmpConf.strWriteSecurityName.c_str(),SNMP_AUTHPROTOCOL_HMACSHA,SNMP_PRIVPROTOCOL_DES, stSnmpConf.stWriteSecuritySettings.strAuthPassword.c_str(), stSnmpConf.stWriteSecuritySettings.strPrivPassword.c_str(), engineID, false);
       
    /* SHA认证 AES加密 */
    }
    else if (stSnmpConf.stWriteSecuritySettings.enAuth == Network::AUTH_SHA && stSnmpConf.stWriteSecuritySettings.enPriv == Network::PRIV_AES)
    {
        stSnmpData.pUut->addNewRow(stSnmpConf.strWriteSecurityName.c_str(),SNMP_AUTHPROTOCOL_HMACSHA,SNMP_PRIVPROTOCOL_AES128, stSnmpConf.stWriteSecuritySettings.strAuthPassword.c_str(), stSnmpConf.stWriteSecuritySettings.strPrivPassword.c_str(), engineID, false);
       
    }
    else
    {
        dlog_error("SNMP V3 版本设置读写安全共同体失败！");
        return -1;
    }
    
    /* 向管理信息库加入V3版本的相关信息 */ 
    stSnmpData.pMib->add(new UsmStats());
    stSnmpData.pMib->add(new usm_mib( stSnmpData.pUut));

    return 0;
}

/* 设置V1/V2版本的用户名 */
int CSnmpManage::setSnmpV1V2User()
{
    if (initReadCommunities() == -1)
    {
        dlog_error("SNMP V2 版本设置读共同体失败！");
    }
    else
    {
        dlog_info("SNMP V2 版本设置读共同体成功！");
    }
    
    
    if (initWriteCommunities() == -1)
    {
        dlog_error("SNMP V2 版本设置读写共同体失败！");
    }
    else
    {
        dlog_info("SNMP V2 版本设置读写共同体成功！");
    }

    return 0;
}

/* 初始化V1/V2版本的读共同体名 */
int CSnmpManage::initReadCommunities() 
{
    if (!stSnmpData.pMib->get_request_list()->get_v3mp()) 
    {
        dlog_error("v3MP必须在snmpCommunityTable之前初始化");
        return -1;
    }

    snmpCommunityEntry* pSnmpCommunityEntry =
            snmpCommunityEntry::get_instance(stSnmpData.pMib);

    if (!pSnmpCommunityEntry) 
    {
        dlog_error("snmpCommunityEntry必须在snmpCommunityTable之前初始化");
        return -1;      
    }
    Oidx ind = Oidx::from_string(stSnmpConf.strReadCommunityName.c_str(), FALSE);
    MibTableRow* r = pSnmpCommunityEntry->find_index(ind);
    if (!r) r = pSnmpCommunityEntry->add_row(ind);
    pSnmpCommunityEntry->set_row(r,
            OctetStr(stSnmpConf.strReadCommunityName.c_str()),
            OctetStr(stSnmpConf.strReadCommunityName.c_str()),
            stSnmpData.pMib->get_request_list()->get_v3mp()->get_local_engine_id(),
            OctetStr(""),
            OctetStr("access"),
            3, 1);
    return 0;
}

/* 初始化V1/V2版本的读写共同体名 */
int CSnmpManage::initWriteCommunities() {
    if (!stSnmpData.pMib->get_request_list()->get_v3mp()) 
    {
        dlog_error("v3MP必须在snmpCommunityTable之前初始化");
        return -1;
    }

    snmpCommunityEntry* pSnmpCommunityEntry =
            snmpCommunityEntry::get_instance(stSnmpData.pMib);

    if (!pSnmpCommunityEntry) 
    {
        dlog_error("snmpCommunityEntry必须在snmpCommunityTable之前初始化");
        return -1;      
    }
    Oidx ind = Oidx::from_string(stSnmpConf.strWriteCommunityName.c_str(), FALSE);
    MibTableRow* r = pSnmpCommunityEntry->find_index(ind);
    if (!r) r = pSnmpCommunityEntry->add_row(ind);
    pSnmpCommunityEntry->set_row(r,
            OctetStr(stSnmpConf.strWriteCommunityName.c_str()),
            OctetStr(stSnmpConf.strWriteCommunityName.c_str()),
            stSnmpData.pMib->get_request_list()->get_v3mp()->get_local_engine_id(),
            OctetStr(""),
            OctetStr("access"),
            3, 1);
    return 0;
}

