/***
 * @FilePath     : qos_manage.h
 * @Author       : huangjunda
 * @Date         : 2025-04-23 11:26:01
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-04-23 11:28:27
 * @Description  :
 */

#pragma once

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "IpcRet.h"
#include "Singleton.h"
#include "dlog.h"
#include "path_define.h"
#include "network_define.h"
#include "network_convert.h"
#include "convert_interface.h"
#include "LibWSServer.h"
#include "CSmtp.h"

#define QOS_DSCP_MIN 0
#define QOS_DSCP_MAX 63

class CQosManage : public CSingleton<CQosManage>
{
    CQosManage();

public:
    virtual ~CQosManage() = default;
    /* 允许 Singleton 访问私有构造函数 */
    friend class CSingleton<CQosManage>;

    IpcRet_E init();
    IpcRet_E deinit();
    int set_qos_config(Network::QosConfigInfo_S stQosConfigInfo);
    int get_qos_config(Network::QosConfigInfo_S &stQosConfigInfo);

private:
    /* 配置文件 */
    std::string m_configFile;
};