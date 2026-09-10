/***
 * @FilePath     : ddns_manage.h
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2025-04-07 10:26:39
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-04-08 10:27:16
 * @Description  : DDNS配置
 */

#pragma once

#include "IpcRet.h"
#include "network_define.h"
#include "Singleton.h"

/* ddns连接命令 */
#define DDNS_CONNECT_COMMAD   "/userdata/cam/tools/ez-ipupdate "
/* ddns服务器类型 */
#define DYNDNS_SERVER         "dyndns"
#define QDNS_SERVER           "qdns"
#define QDNS_SERVER_ADDRESS   "members.3322.org"
#define DYNDNS_SERVER_ADDRESS "members.dyndns.org"

class CDdnsManage : public CSingleton<CDdnsManage>
{
    CDdnsManage();

public:
    virtual ~CDdnsManage() = default;
    /* 允许 Singleton 访问私有构造函数 */
    friend class CSingleton<CDdnsManage>;

    IpcRet_E init();
    IpcRet_E deinit();
    int config_ddns(Network::Ddns_S stDdns);
    int get_ddns_config(Network::Ddns_S &stDdns);

private:
    /* 配置文件 */
    std::string m_configFile;
};