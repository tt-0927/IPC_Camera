/**
 * @FilePath     : multicast_udp_server.h
 * @Author       : huangjunda
 * @Date         : 2025-03-28 17:13:53
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-10-13 15:48:33
 * @Description  : 多播 UDP 服务器
 */

#pragma once

#include <fstream>
#include <thread>
#include <atomic>
#include "task_manage.h"
#include "Singleton.h"
#include "IpcRet.h"
#include "IOBase.h"
#include "network_define.h"
#include "network_manage.h"
#include "system_manage.h"
#include "data_length.h"
#include "action_code.h"
#include "share_define.h"
#include "xml_base.h"
#include "os_network_multicast.h"

/* 多播默认端口 */
#define MULIT_PORT_DEFAULT            (8005)
/* 多播默认地址 */
#define MULIT_ADDR_DEFAULT            "239.255.255.251"
/* 多播默认子网掩码 */
#define MULIT_MASK_DEFAULT            "255.255.255.0"
/* 多播默认网关 */
#define MULIT_GATEWAY_DEFAULT         "224.0.0.254"
/* 多播需要的UUID路径 */
#define MULTICAST_UUID_PATH "/proc/sys/kernel/random/uuid"
/* 多播搜索命令 */
#define MULTICAST_CMD_SEARCH "SEARCH * UPGRADE"
/* 多播配置命令 */
#define MULTICAST_CMD_CONFIG "CONFIGNET"
/* 多播重启命令 */
#define MULTICAST_CMD_REBOOT "CAMERA_REBOOT"
/* 多播接收客户端ID */
#define MULTICAST_RECEIVE_CLIENT_ID "Client ID"
/* 多播搜索回应命令 */
#define MULTICAST_ANSWER_SEARCH "REPLY OK \r\n\
VERSION:%s\r\n\
DEVICE_MODEL:%s\r\n\
%s\r\n\
Device ID:%s\r\n\
SN:vf292b080a5\r\n\
Uptime=%ld\r\n\
DHCP=%d\r\n\
IP=%s\r\n\
MASK=%s\r\n\
GATEWAY=%s\r\n\
MAC=%s\r\n\
FDNS=%s"

class CMulticastUdpServer : public CSingleton<CMulticastUdpServer>
{
    CMulticastUdpServer() = default;

public:
    ~CMulticastUdpServer() = default;

    friend class CSingleton<CMulticastUdpServer>;

    IpcRet_E init();
    IpcRet_E deinit();

private:
    static void *network_multicast_handle(void *pParam);
    void         search_response(void *pParam);
    void         config_response(void *pParam);
    void         reboot_response(void *pParam);
    void        *multicast_sendLoop();

    NetworkMulticast_S *m_stuMulticastHandle; /* UDP Server 句柄 */
    std::thread         m_sendThread;         /* 线程对象 */
    std::string         m_strUuid;            /* 设备UUID */
    time_t              m_nStartTime;         /* 服务启动时间 */
};
