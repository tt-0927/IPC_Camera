#ifndef _REACH_NETWORK_H__
#define _REACH_NETWORK_H__

#define DNS_CFG "/etc/resolv.conf"
#define WAC_PATH "/etc/nettype.conf"
#define ETH0_INTERFACE ("eth0")
#define ETH1_INTERFACE ("eth1")
#define LOCAL_IP_ETH ("lo")

#ifndef LOCAL_IP
#define LOCAL_IP "127.0.0.1"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    int ReachMacAddr(char *interface_name, char *mac);
    /* 字母大写 */
    int ReachMacAddrCapital(char *interface_name, char *mac);

    int ReachSplitMacAddr(char *src, unsigned char *dst, int num);
    unsigned int ReachGetGateWay(char *interface_name);
    unsigned int ReachGetIPaddr(char *interface_name);
    int ReachGetIPaddrstring(char *interface_name, char *ipaddr);
    unsigned int ReachGetNetmask(char *interface_name);
    unsigned int ReachGetBroadcast(char *interface_name);

    int ReachGetIPaddrchar(char *interface_name, char *ipaddr);
    int ReachGetNetmaskchar(char *interface_name, char *ipaddr);
    int ReachGetGateWaychar(char *interface_name, char *ipaddr);

    int ReachGetDHCP(char *filename);

    int ReachSetIP(char *filename, char *ip, char *netmask, char *gw);

    int ReachSetDHCP(char *filename);

    int ReachSetDns(char *dns);

    int ReachGetDns(char *dns);

    int get_lan_mac(char *macaddr);

    // 获取MAC地址
    int get_mac(char *pMac, int nLen);

    int ReachSetMac(char *mac);

    void net_get_ipv6(char *filename, char *ip);
    int ReachGetIPv6dns(char *dns);
    int ReachGetIPv6gateway(char *gateway);

    /*
     * @description: 设置ipv6网关
     * @param[in]: pDns-网关地址
     * @return: 无
     * @others：其他说明
     */
    int ReachSetIPV6Dns(char *pDns);

    /*
     * 获取IPv6网关
     * @Author: wxz
     * param[in]:pInterfaceName:网卡（eth0）
     * param[in]:nType: 1,寻找link-local单播网关（fe80）  0：其他IP6地址
     * param[in]:pBcast: 网关
     * param[in]:nLength: 网关pBcast长度
     * */
    int net_get_ipv6_bcast(char *pInterfaceName, char nType, char *pBcast, int nLength);

    /*
     * 获取IPv6网路地址
     * @Author: wxz
     * param[in]:pInterfaceName:网卡（eth0）
     * param[in]:nType: 1,寻找link-local单播网关（fe80）  0：其他IP6地址
     * param[in]:pAddr: ipv6地址
     * param[in]:nLength: pAddr长度
     * */
    int net_get_ipv6_ifaddr(char *pInterfaceName, char nType, char *pAddr, int nLength); //, char *prefix, int length2);

    /*
     * 恢复出厂默认IPv6设置
     * @Author: wxz
     * @param pConfigFile ipv6配置文件路径
     * */
    int ipv6_recover_defaultsetting(const char *pConfigFile);

    /*
     * @description: 设置网络ipv4
     * @param[in]: pFileName-网络设置脚本文件
     * @param[in]: pIp-ipv4地址
     * @param[in]: pNetMask-子网掩码地址
     * @param[in]: pGateway-网关
     * @param[in]: pEthName-网卡名称：如eth0,eth1
     * @return: 无
     * @others：其他说明
     */
    int network_set_ipv4(char *pFileName, char *pIp, char *pNetMask, char *pGateway, char *pEthName);

    /*
     * @description: 设置网络ipv6
     * @param[in]: pFileName-网络设置脚本文件
     * @param[in]: pIpv6-ipv6地址
     * @param[in]: pGateway-网关
     * @param[in]: pEthName-网卡名称：如eth0,eth1
     * @return: 无
     * @others：其他说明
     */
    int network_set_ipv6(char *pFileName, char *pIpv6, char *pGateway, char *pEthName);
    /**
     * @brief 获取网卡列表
     * @param pnCount 网卡
     * @return char** 网卡列表
     */
    char **get_network_interfaces(int *pnCount);

    /**
     * @brief 获取默认路由对应的网卡名称
     * @return char*网卡名称
     */
    char *get_default_interface();
    /**
     * @brief 获取指定网卡的IP地址
     * @param pIfname 网卡名称
     * @return char* ip地址
     */
    char *get_interface_ip(const char *pIfname);
    /**
     * @brief 获取主网卡ip
     * @return char* ip地址
     */
    char *get_primary_ip();

#if 0
void InitICMP(void);
int cleanICMP(void);
void SendICMPmessage(void);
#endif
#ifdef __cplusplus
}
#endif
#endif
