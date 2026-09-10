/***
 * @FilePath     : ddns_manage.cpp
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2025-04-07 10:26:39
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-04-08 10:27:03
 * @Description  : DDNS配置
 */

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>
#include <thread>
#include "ddns_manage.h"
#include "dlog.h"

#include "network_manage.h"
#include "network_convert.h"
#include "convert_interface.h"
#include "path_define.h"
#include "edukit_network.h"

CDdnsManage::CDdnsManage() : m_configFile(DDNS_CONFIG_FILE)
{
}

IpcRet_E CDdnsManage::init()
{
    Network::Ddns_S stDdns;
    if (Convert::read_file(m_configFile, stDdns))
    {
        Convert::write_file(m_configFile, stDdns);
    }
    int nRet = config_ddns(stDdns);
    if (nRet < OK)
    {
        return ERR;
    }
    
    return OK;
}

IpcRet_E CDdnsManage::deinit()
{
    return OK;
}

int CDdnsManage::config_ddns(Network::Ddns_S stDdns)
{
    char *pDfaultEth = get_default_interface();
    dlog_info("获取默认网卡:%s", pDfaultEth);
    if (pDfaultEth == NULL)
    {
        dlog_error("获取默认网卡失败");
        pDfaultEth = ETH0_INTERFACE;
        // return -1;
    }

    std::ostringstream cmd;
    std::string strInterface;
    std::string strServiceType;

    if (!stDdns.bEnableDdns)
    {
        return 0;
    }

    strInterface = pDfaultEth;
    if (stDdns.nDdnsType == Network::QDNS_TYPE)
    {
        strServiceType = QDNS_SERVER;
        stDdns.address = QDNS_SERVER_ADDRESS;
    }
    else if (stDdns.nDdnsType == Network::DYNDNS_TYPE)
    {
        strServiceType = DYNDNS_SERVER;
        stDdns.address = DYNDNS_SERVER_ADDRESS;
    }

    /* 封装ddns执行命令 */
    cmd << DDNS_CONNECT_COMMAD
        << " -i " << strInterface
        << " -h " << stDdns.realmName
        << " -S " << strServiceType
        << " -u " << stDdns.user << ":" << stDdns.password;

    int nRet = system(cmd.str().c_str());

    if (nRet != 0)
    {
        dlog_error("ddns连接命令执行失败");
        return -1;
    }

    Convert::write_file(m_configFile, stDdns);
    return 0;
}

int CDdnsManage::get_ddns_config(Network::Ddns_S &stDdns)
{
    Convert::read_file(m_configFile, stDdns);
    return 0;
}