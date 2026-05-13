/***
 * @FilePath     : upnp_manage.h
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2025-03-19 14:16:09
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-04-14 14:35:05
 * @Description  : upnp管理实现
 */

#pragma once

#include <mutex>
#include <vector>
#include <map>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <cstring>
#include <unordered_map>

#undef UPNP_HAVE_OPTSSDP
#include <upnp.h>
#include <upnptools.h>
#include <upnpdebug.h>
#include <ixml.h>

#include "dlog.h"
#include "Singleton.h"
#include "network_define.h"

/* 所有类型设备 */
#define UPNP_ALL_SERVICE     "ssdp:all"
/* ppp连接服务类型设备 */
#define UPNP_WANPPPC_SERVICE "urn:schemas-upnp-org:service:WANPPPConnection:1"
/* 光纤、以太网直连服务类型设备 */
#define UPNP_WANIPC_SERVICE  "urn:schemas-upnp-org:service:WANIPConnection:1"
/* 网关类型设备 */
#define UPNP_GATEWAY_SERVIC  "urn:schemas-upnp-org:device:InternetGatewayDevice:1"

#define UPNP_ENABLE   "1"                 /* 启用端口映射 */
#define UPNP_DISABLE  "0"                 /* 禁用端口映射 */
#define UPNP_PROTOCOL "TCP"               /* 端口映射协议 */
#define UPNP_DESCRI   "UPnP Port Mapping" /* 端口映射描述 */
#define UPNP_LEASE    "0"                 /* 端口映射有效期-永久 */
#define UPNP_TIMEOUT  3                   /* upnp搜索设备超时 */

/* upnp默认映射端口 */
#define UPNP_HTTP_PORT   80
#define UPNP_RTSP_PORT   554
#define UPNP_HTTPS_PORT  443
#define UPNP_SERVER_PORT 8000
#define UPNP_WEB_SERVER_PORT 9000
/* 空ip */
#define UPNP_EMTPY_IP    "0.0.0.0"

/* upnp ip获取回调 */
using upnp_ip_callBack = std::function<void(const std::string &)>;
/* upnp 端口状态回调 */
using port_check_callback = std::function<void(bool)>;
/* 端口类型描述列表 */
const std::map<int, std::string> PORT_TYPE_DESCRIPTION =
{
    {0, "HTTP Service"},
    {1, "RTSP Stream"},
    {2, "HTTPS Service"},
    {3, "Server Connection"},
    {4, "Platform Command"},
    {5, "Platform Data"}
};
class CUpnpManage : public CSingleton<CUpnpManage>
{
    CUpnpManage();
public:
    typedef enum _UPNP_STATUS_E_
    {
        /* 开启中 */
        OPENING = 0,
        /* 已开启 */
        OPENED = 1,
        /* 关闭中 */
        CLOSING = 2,
        /* 已关闭 */
        CLOSED = 3
    } UPNP_STATUS_E;
public:
    ~CUpnpManage();
    friend class CSingleton<CUpnpManage>;
    /**
     * @brief 打印端口映射状态
     */
    void print_status();
    /**
     * @brief upnp初始化
     * @return int
     */
    int init();
    /**
     * @brief 设置端口映射
     * @param stPortMapConfig 端口映射信息
     * @return int
     */
    int set_port_map(Network::PortMapConfig_S stPortMapConfig);

    /**
     * @brief 获取端口映射
     * @param stPortMapConfig 端口映射信息
     * @return int
     */
    int get_port_map(Network::PortMapConfig_S &stPortMapConfig);

    /**
     * @brief 处理upnp命令返回结果
     * @param pAction
     */
    void process_action_complete(const void *pAction, Network::PortMap_S &stPortMap);

    /**
     * @brief 处理事件
     * @param stEventType 事件类型
     * @param pEvent 数据
     */
    void handle_event(int stEventType, void *pEvent);
    /**
     * @brief upnp反初始化
     * @return int
     */
    int deinit();

private:
    void deinit_thread();
    bool file_exists(const std::string &strFilename);
    /**
     * @brief 添加端口映射
     * @param stPortMap 映射信息
     * @return int
     */
    int add_port_mapping(Network::PortMap_S stPortMap);
    /**
     * @brief 开启设备搜索
     */
    void start_search();
    /**
     * @brief 关闭设备搜索
     */
    void stop_search();
    /**
     * @brief 搜索设备
     * @param nTimeOut 超时时间
     * @return int
     */
    int search_device(int nTimeOut);
    /**
     * @brief upnp初始化
     * @return int
     */
    int init_upnp();

    /**
     * @brief 获取外部ip
     */
    void get_external_ip();
    /**
     * @brief 检查端口映射状态
     * @param nExternalPort
     */
    void check_portMapping_status(int nExternalPort);
    /**
     * @brief 事件回调
     * @param callback
     */
    static int event_callback(int eventType, void *event, void *cookie);

    /**
     * @brief 删除端口映射
     * @param stPortMap 映射信息
     * @return int
     */
    int remove_port_mapping(Network::PortMap_S stPortMap);

    /**
     * @brief 设备发现事件
     * @param pDiscovery
     */
    void process_discovery_event(void *pDiscovery);
    /**
     * @brief 设备离线事件
     * @param pDiscovery
     */
    void process_byebye_event(void *pDiscovery);
    /**
     * @brief 获取xml节点数据
     * @param pDoc xml文档
     * @param strNode 指定节点
     * @return std::string
     */
    std::string get_xml_nodeValue(void *pDoc, std::string strNode);
    /**
     * @brief 解析设备信息
     * @param pDoc
     * @param device
     * @return int
     */
    int parse_device_description(void *pDoc, Network::UpnpConfigInfo_S &stDevice);

    /**
     * @brief 解析Location获取baseURL
     * @param pLocation
     * @return std::string
     */
    std::string extract_baseURL(const char *pLocation);
    /**
     * @brief 解析 sockaddr_storage
     * @param pAddr 目标网络数据结构
     * @param pIp ip地址
     * @param pPort 端口
     * @return int
     */
    int parse_sockaddr(const struct sockaddr_storage *pAddr, char *pIp, uint16_t *pPort);

private:
    /* upnp配置文件 */
    std::string m_upnpFile;
    /* 端口配置文件 */
    std::string m_portFile;
    /* 当前配置 */
    Network::PortMapConfig_S m_currentConfig;
    /* upnp客户端句柄 */
    UpnpClient_Handle m_handle = -1;
    /* 循环控制标志 */
    std::atomic<bool> m_loop{false};
    /* 搜索线程 */
    std::thread m_worker;
    /* 设备列表 */
    std::unordered_map<std::string, Network::UpnpConfigInfo_S> m_mapdevices;
    /* 设备锁 */
    std::mutex m_deviceMutex;
    /* 外部ip */
    std::string m_externalIP = "0.0.0.0";
    /* 端口映射状态 */
    int m_status;
    /* 设备在线状态 */
    std::atomic<bool> m_deviceOnline{false};
    /* UPNP当前状态 */
    UPNP_STATUS_E m_upnpStaus = CLOSED;
};