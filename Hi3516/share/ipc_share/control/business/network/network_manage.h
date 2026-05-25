/***
 * @FilePath     : network_manage.h
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2024-11-02 16:30:03
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-24 16:52:32
 * @Description  : 网络管理
 */

#pragma once

#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <cstring> 
#include <iostream>
#include <functional>
#include <thread>
#include <map>
#include <mutex>
#include <condition_variable>
#include "IpcRet.h"
#include "network_define.h"
#include "register_define.h"
#include "Singleton.h"
#include "path_define.h"
#include "convert_interface.h"
#include "wifi_manage.h" 

#define ETH0_INTERFACE "eth0"  /* 网卡 */

#define ETH0_DEFAULT_IP     "192.168.1.168" /* 网卡默认IP */
#define ETH0_DEFAULT_MASK   "255.255.255.0"   /* 网卡默认子网掩码 */
#define ETH0_DEFAULT_GATWAY "192.168.1.254"   /* 网卡默认网关 */
#define DEFAULT_MTU          1500             /* 默认MTU大小 */
#define DEFAULT_MAIN_DNS    "114.114.114.114" /* 默认主要DNS */
#define DEFAULT_STANDBY_DNS "8.8.8.8"         /* 默认次要DNS */

#define MAX_COMMAND_LENGTH 256  /* 最大命令长度 */ 

// #define NETWORK_INTERFACES     ("/etc/network/interfaces")     /* 网络配置文件 */
// #define NETWORK_INTERFACES_TMP ("/etc/network/interfaces.tmp") /* 网络配置文件信息临时存放 */

/* 物理地址最大字节 */
#define MAC_ADDR_LEN (64)
/* mac文件 */
#define MAC_FILE_PATH  "/opt/cam/.config/design_data/setmac"

/*符合规定的MAC地址前三位*/
#define MAC_0 (0x30)
#define MAC_1 (0x3A)
#define MAC_2 (0xBA)

/* 配置本地回环接口（lo）的网络配置 */ 
#define NETWORK_CONFIG_LO \
    "auto lo\n" \
    "iface lo inet loopback\n"
/* 网卡eth0的manual模式 */
#define NETWORK_MANUAL_ETH0 \
    "auto eth0\n" \
    "iface eth0 inet manual\n"

#define ETH0_DHCP_STATUS_OPEN  ("#eth0 dhcp-open\n")  /* eth0开启动态获取ip */
#define ETH0_DHCP_STATUS_CLOSE ("#eth0 dhcp-close\n") /* eth0关闭动态获取ip */

#define SET_DHCP_STATUS_OPEN(buffer, interface) \
    snprintf(buffer, MAX_COMMAND_LENGTH, "#%s dhcp-open\n", interface)

#define SET_DHCP_STATUS_CLOSE(buffer, interface) \
    snprintf(buffer, MAX_COMMAND_LENGTH, "#%s dhcp-close\n", interface)

#define DHCP_STATUS_OPEN  (" dhcp-open")     /* 开启动态获取ip */
#define DHCP_STATUS_CLOSE (" dhcp-close")    /* 静态获取ip */

/****命令相关*******/
#define GET_IP_ADDRESS_COMMAND          " | grep -o -E 'inet (addr:)?([0-9]{1,3}[\\.]){3}[0-9]{1,3}' | awk '{print $2}'"                  /* 从网络配置中提取IP地址 */
#define GET_SUBNET_MASK_COMMAND         " | awk '/Mask/{print $4}' | sed 's/Mask://'"                                                     /* 从网络配置中提取子网掩码 */
#define GET_DEFAULT_GATEWAY_COMMAND     "route -n | awk '$1 == \"0.0.0.0\" && $4 ~ /G/ {print $2}'"                                       /* 从网络配置中提取网关 */
#define REMOVE_DEFAULT_GATEWAY_COMMAND  "route del default gw $(route -n | awk '$1 == \"0.0.0.0\" && $4 ~ /G/ {print $2}') dev "                                    /* 从网络配置中删除网关 */
#define IP6_ROUTE_SHOW_DEV              "ip -6 route show dev "                                                                           /* 获取ipv6网关的命令 */
#define START_DHCP_COMMAND              "/sbin/udhcpc -b -i "                                                                             /* 开启dhcp命令 */
#define SET_GW_COMMAND                  "route add default gw "                                                                           /* 设置网关的命令 */
#define CHECK_GATEWAY_COMMAND(interface, gateway) ("ip route show | grep 'default via " + gateway + " dev " + interface + "'")            /* 检查网关是否存在的命令 */
#define CHECK_AUTONEGOTIATION(interface)          ("/opt/cam/tools/ethtool " + std::string(interface) + " | grep Auto-negotiation" ) /* 获取网卡是否开启自动协商的命令 */
#define SET_INTERFACE_DOWN(netcard)               std::system(("ip link set " + std::string(netcard) + " down").c_str())                  /*  指定网卡设置为 down 状态*/ 
#define SET_INTERFACE_UP(netcard)                 std::system(("ip link set " + std::string(netcard) + " up").c_str())                    /*  指定网卡设置为 UP 状态*/    

/***************设置网卡类型的命令********************/
#define ETH_AUTO_NEGOTIATION_ON_CMD(netcard)  std::system((" /opt/cam/tools/ethtool -s " + std::string(netcard) + " autoneg on\n").c_str())
#define ETH_AUTO_NEGOTIATION_OFF_CMD(netcard) std::system((" /opt/cam/tools/ethtool -s " + std::string(netcard) + " autoneg off\n").c_str())
#define ETH_10M_HALF_DUPLEX_CMD(netcard)      std::system((" /opt/cam/tools/ethtool -s " + std::string(netcard) + " speed 10 duplex half\n").c_str())
#define ETH_10M_FULL_DUPLEX_CMD(netcard)      std::system((" /opt/cam/tools/ethtool -s " + std::string(netcard) + " speed 10 duplex full\n").c_str())
#define ETH_100M_HALF_DUPLEX_CMD(netcard)     std::system((" /opt/cam/tools/ethtool -s " + std::string(netcard) + " speed 100 duplex half\n").c_str())
#define ETH_100M_FULL_DUPLEX_CMD(netcard)     std::system((" /opt/cam/tools/ethtool -s " + std::string(netcard) + " speed 100 duplex full\n").c_str())
#define ETH_1000M_HALF_DUPLEX_CMD(netcard)    std::system((" /opt/cam/tools/ethtool -s " + std::string(netcard) + " speed 1000 duplex half\n").c_str())
#define ETH_1000M_FULL_DUPLEX_CMD(netcard)    std::system((" /opt/cam/tools/ethtool -s " + std::string(netcard) + " speed 1000 duplex full\n").c_str())

/*********用于写入到网络配置文件，永久设置网卡类型*************/
#define ETH_AUTO_NEGOTIATION_ON(netcard)  (std::string("/opt/cam/tools/ethtool -s ") + netcard + " autoneg on\n")             /* 网卡自动协商 */
#define ETH_AUTO_NEGOTIATION_OFF(netcard) (std::string("/opt/cam/tools/ethtool -s ") + netcard + " autoneg off\n")            /* 关闭网卡自动协商 */
#define ETH_10M_HALF_DUPLEX(netcard)      (std::string("/opt/cam/tools/ethtool -s ") + netcard + " speed 10 duplex half\n")   /* 网卡10M半双工 */
#define ETH_10M_FULL_DUPLEX(netcard)      (std::string("/opt/cam/tools/ethtool -s ") + netcard + " speed 10 duplex full\n")   /* 网卡10M全双工 */
#define ETH_100M_HALF_DUPLEX(netcard)     (std::string("/opt/cam/tools/ethtool -s ") + netcard + " speed 100 duplex half\n")  /* 网卡100M半双工 */
#define ETH_100M_FULL_DUPLEX(netcard)     (std::string("/opt/cam/tools/ethtool -s ") + netcard + " speed 100 duplex full\n")  /* 网卡100M全双工 */
#define ETH_1000M_HALF_DUPLEX(netcard)    (std::string("/opt/cam/tools/ethtool -s ") + netcard + " speed 1000 duplex half\n") /* 网卡1000M半双工 */
#define ETH_1000M_FULL_DUPLEX(netcard)    (std::string("/opt/cam/tools/ethtool -s ") + netcard + " speed 1000 duplex full\n") /* 网卡1000M全双工 */

#define STR_SPEED_10MB   "10"   /* 速率为10Mb/s */
#define STR_SPEED_100MB  "100"  /* 速率为100Mb/s */
#define STR_SPEED_1000MB "1000" /* 速率为1000Mb/s */
#define STR_DUPLEX_HALF  "half" /* 半双工模式 */ 
#define STR_DUPLEX_FULL  "full" /* 全双工模式 */ 

#define DNS_CONFIG_TMP ("/etc/resolv.tmp")  /* DMS配置文件临时存放 */
#define DNS_CONFIG     ("/etc/resolv.conf") /* DNS配置文件 */

#define NETWORK_DEVICES_PATH "/sys/class/net/" /* 网络设备目录的路径 */
#define MTU_PATH             "/mtu"            /* MTU数据包节点 */
#define SPEED_PATH           "/speed"          /* 网卡速率节点 */
#define DUPLEX_PATH          "/duplex"         /* 网卡双工节点 */

/**
 * @brief DHCP是否开启
 */
typedef enum  
{
    DHCP_UNKNOWN  = -1, /* 未知状态，用于表示无法确定DHCP状态或发生错误 */ 
    DHCP_DISABLED = 0,  /* DHCP被禁用 */
    DHCP_ENABLED  = 1   /* DHCP被启用 */ 
}DhcpStatus_E;

class CNetworkManage : public CSingleton<CNetworkManage>
{
    CNetworkManage() = default;
public:
    virtual ~CNetworkManage() = default;
   /* 允许 Singleton 访问私有构造函数 */ 
    friend class CSingleton<CNetworkManage>; 

	/**
	 * @brief 设置系统网络配置信息
	 * @param stNetInfo 网络配置信息
	 * @return int 
	 */
	int set_system_network(Network::Info_S stNetInfo,std::function<void( int)> result);

    /**
     * @brief 设置system network对象
     * @param stNetInfo 
     * @return int 
     */
    int set_system_networkInfo(Network::Info_S stNetInfo, bool bIsImmediate = true);

    /**
     * @brief 获取系统网络配置信息
     * @param stNetInfo 
     * @return int 
     */
    int get_system_networkInfo(Network::Info_S &stNetInfo);

    /**
     * @brief 激活手动配置网络
     * @param stNetInfo 
     * @return int 
     */
    int register_manual_config(Register::NetWorkInfo_S &stNetInfo);

    /**
     * @brief 激活自动配置网络
     * @param stNetInfo 
     * @return int 
     */
    int register_auto_config();

    /**
     * @brief Mac地址合规性检测
     * @param NULL
     * @return bool 
     */
    bool check_mac_valid();

    /*** 
     * @description : 获取网络端口
     * @author      : huangjunda
     * @return       {*}
     */    
    int get_network_port(Network::PortConfig_S &stPortConfig);

    /*** 
     * @description : 设置网络端口
     * @author      : huangjunda
     * @return       {*}
     */    
    int set_network_port(Network::PortConfig_S stPortConfig);

    /**
     * @brief 获取MAC地址
     * @param interfaceName 
     * @return std::string 
     */
     std::string get_macAddress(const std::string& interfaceName);

      /**
     * @brief 是否需要重启
     * @param stNetInfo 
     * @return true 
     * @return false 
     */
    bool need_reboot(Network::Info_S stNetInfo);
       /**
     * @brief 获取ip和DNS相关信息
     * @param stNetInfo 
     * @return int 
     */
    int get_ip_and_dns(Network::Info_S &stNetInfo);

private:

    /**
     * @brief 开启自动获取dhcp
     * @param strInterface 
     * @return int 
     */
    int start_dhcp(const std::string& strInterface);

    /**
     * @brief 设置网卡的IPV4信息
     * @param interface 
     * @param ip 
     * @param mask 
     * @param gateway 
     * @return int 
     */
    int configure_network(const std::string& interface, const std::string& ip, const std::string& mask, const std::string& gateway);
    
    /**
     * @brief 设置容错信息
     * @param stNetInfo 
     * @return int 
     */
    int set_failover_info(Network::Info_S &stNetInfo);

    /**
     * @brief 设置网卡类型
     * @param enType 网卡类型
     * @param isSet 是否调用命令设置
     * @return int 
     */
    int set_network_type(FILE* pFile,Network::NetTypeMode_E enType,std::string strNetName,bool isSet);

    /**
     * @brief 获取网卡类型
     * @return int 
     */
    Network::NetTypeMode_E get_network_yype(std::string strNetName);

    /**
     * @brief 设置网卡信息
     * @param stIp 网卡信息
     * @return int 
     */
    int set_ethInterfaces(Network::Ip_S stIp, bool bIsImmediate);

    /**
     * @brief 设置DNS相关信息
     * @param stDns 
     * @return int 
     */
    int set_networkDns(Network::Dns_S &stDns);

    /**
     * @brief 获取dns信息 
     * @return int 
     */
    int get_networkDns(Network::Dns_S& stDns);

    /**
     * @brief 获取设备IP
     * @param strNetName 
     * @return string 
     */
    std::string get_dev_ip(std::string strNetName);

    /**
     * @brief 获取设备IP
     * @param interface 
     * @return string 
     */
    std::string getLocalIP(const std::string& interface = "");

    /**
     * @brief 获取设备IP
     * @param ifaceiface 
     * @return string 
     */
    std::string getIPByIfconfig(const std::string& iface);

    /**
     * @brief 获取设备子网掩码
     * @param strNetName 
     * @return string 
     */
    std::string get_dev_mask(std::string strNetName);

    /**
     * @brief 获取设备网关
     * @param strNetName 
     * @return string 
     */
    std::string get_dev_gateway(std::string strNetName);


    /**
     * @brief 获取ipV6地址
     * @param strNetName 
     * @return std::string 
     */
    std::string get_ipv6Address(const std::string& strNetName);

    /**
     * @brief 获取ipv6网关
     * @param strNetName 
     * @return std::string 
     */
    std::string get_ipv6Gateway(const std::string& strNetName);

    /**
     * @brief 判断是否开启DHCP
     * @param strNetName 
     * @return int 
     */
    int get_dhcpStatus(const std::string& strNetName);

    /**
     * @brief 设置IP和dns相关信息
     * @param stIpInfo 
     * @return int 
     */
    int set_ip_and_dns(Network::Info_S &stNetInfo, bool bIsImmediate = true);

 

    /**
     * @brief 获取Mtu大小
     * @param strInterfaceName 
     * @return int 
     */
    int get_mtu(const std::string& strInterfaceName);

    /**
     * @brief 设置mtu大小
     * @param strInterfaceName 
     * @param nMtu 
     * @return int 
     */
    int set_mtu(const std::string& strInterfaceName,int nMtu);

    /**
     * @brief 判断网卡是否开启自动协商
     * @param strInterfaceName 
     * @return true 
     * @return false 
     */
    bool autoneg_enabled(const std::string& strInterfaceName);

    /**
     * @brief 获取网卡速率
     * @param strInterfaceName 
     * @return string 
     */
    std::string get_speed(const std::string& strInterfaceName); 

    /**
     * @brief 获取网卡传输模式
     * @param strInterfaceName 
     * @return string 
     */
    std::string get_duplex(const std::string& strInterfaceName);

    /**
     * @brief 解锁文件
     * @param pFilePath 
     * @return int 
     */
    int unlock_file(const char* pFilePath);

    /**
     * @brief 锁定文件不可修改
     * @param pFilePath 
     * @return int 
     */
    int lock_file(const char* pFilePath);

    /**
     * @brief       : 检查命令是否存在
     * @param        {char*} pCmd
     * @return       {bool}
     */
    bool check_cmd_exists(const char* pCmd);

    /**
     * @brief 检查ip是否被使用
     * @param ip
     * @param interface  
     * @return int 
     */
    int check_ip_usage(const std::string& strIp,const std::string& strInterfaceName);

    /**
     * @brief 获取获取网卡名称列表
     * @return std::string
     */
    std::string get_network_interface();

    /**
     * @brief 去除空字符串
     * @param str 目标字符串
     * @return std::string 去除后的结果
     */
    std::string trim(const std::string& str);

    /**
     * @brief 检测网卡物理连接状态
     * @param strInterfaceName 网卡
     * @return int 返回0处于连接状态,非0处于断开状态
     */
    int check_carrier_link(const std::string& strInterfaceName);
    
    /**
     * @brief 获取网卡
     * @return std::string 网卡
     */
    std::string get_netName_interface();

    /**
     * 写入bond网卡脚本
    */
    void write_bond_script();
    
    // @brief 网卡名称
    std::string strInterfaceName;
    /*  新增同步成员变量 */
    Network::Info_S         m_stInfo;                  /* 网络配置信息 */
    std::mutex              m_networkMutex;            /* 网络设置互斥锁 */
    std::condition_variable m_networkCV;               /* 条件变量 */
    bool                    m_bNetworkSetting = false; /* 设置状态标志 */
};


