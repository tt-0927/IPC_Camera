/**************************************************************************************************
 *													注意事项
 *
 *					1.
 *					2.
 *																																					write by zm
 ****************************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <asm/types.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/time.h>
#include <ifaddrs.h>

#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
// #include <linux/icmp.h>
#include <strings.h>
#include <netinet/ip_icmp.h>
#include <stdlib.h>

#include "share_socket.h"

// #include "reach_network.h"
#include "edukit_network.h"
#include "dlog.h"

#define BUFSIZE 8192
#define IPADDR_LEN 16

int network_order_getInfo(char *command, char *freadBuf, int len)
{
	if (freadBuf == NULL || command == NULL)
	{
		printf("network_order_getInfo \n");
		return -1;
	}
	FILE *fd = popen(command, "r");
	if (fd == NULL)
	{
		printf("%s is fail\n", command);
		return -1;
	}
	int ret = fread(freadBuf, 1, len, fd);
	// printf("freadBuf:%s\n", freadBuf);
	if (ret <= 0)
	{
		// printf("fread catforkInfo%d\n", ret);
		pclose(fd);
		return -1;
	}
	pclose(fd);
	return ret;
}

int ReachSetIPV6(const char *filename, char *ip, char *gw, int net);

struct route_info
{
	u_int dstAddr;
	u_int srcAddr;
	u_int gateWay;
	char ifName[IF_NAMESIZE];
};

/*
 * 将MAC地址的字符串转换成字符数组中
 *
 * 如:  src = "00:11:22:33:44:55" --->
 *		dst[0] = 0x00 dst[1] = 0x11 dst[2] = 0x22;
 *
 */

int ReachMacAddr(char *interface_name, char *mac)
{
	struct ifreq ifreq;
	int sock;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket ");
		return 2;
	}
	strcpy(ifreq.ifr_name, interface_name);
	if (ioctl(sock, SIOCGIFHWADDR, &ifreq) < 0)
	{
		perror("ioctl ");
		return 3;
	}

	sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
			(unsigned char)ifreq.ifr_hwaddr.sa_data[0],
			(unsigned char)ifreq.ifr_hwaddr.sa_data[1],
			(unsigned char)ifreq.ifr_hwaddr.sa_data[2],
			(unsigned char)ifreq.ifr_hwaddr.sa_data[3],
			(unsigned char)ifreq.ifr_hwaddr.sa_data[4],
			(unsigned char)ifreq.ifr_hwaddr.sa_data[5]);
	if (sock > 0)
	{
		close(sock);
	}
	return 0;
}

int ReachSplitMacAddr(char *src, unsigned char *dst, int num)
{
	char *p;
	char *q = src;
	int val = 0, i = 0;

	for (i = 0; i < num; i++)
	{
		p = strstr(q, ":");

		if (!p)
		{
			val = strtol(q, 0, 16);
			dst[i] = val;

			if (i == num - 1)
			{
				continue;
			}
			else
			{
				return -1;
			}
		}

		*(p++) = '\0';
		val = strtol(q, 0, 16);
		dst[i] = val;
		q = p;
	}

	return 0;
}

int ReachMacAddrCapital(char *interface_name, char *mac)
{
	struct ifreq ifreq;
	int sock;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket ");
		return 2;
	}
	strcpy(ifreq.ifr_name, interface_name);
	if (ioctl(sock, SIOCGIFHWADDR, &ifreq) < 0)
	{
		perror("ioctl ");
		return 3;
	}

	sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X",
			(unsigned char)ifreq.ifr_hwaddr.sa_data[0],
			(unsigned char)ifreq.ifr_hwaddr.sa_data[1],
			(unsigned char)ifreq.ifr_hwaddr.sa_data[2],
			(unsigned char)ifreq.ifr_hwaddr.sa_data[3],
			(unsigned char)ifreq.ifr_hwaddr.sa_data[4],
			(unsigned char)ifreq.ifr_hwaddr.sa_data[5]);
	if (sock > 0)
	{
		close(sock);
	}
	return 0;
}

#if 0
static int readNlSock(int sockFd, char *bufPtr, int seqNum, int pId)
{
	struct nlmsghdr *nlHdr;
	int readLen = 0, msgLen = 0;

	do {
		/* Recieve response from the kernel */
		if((readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0)) < 0) {
			dlog_error("readNlSock:SOCK READ: ");
			return -1;
		}

		nlHdr = (struct nlmsghdr *)bufPtr;

		/* Check if the header is valid */
		if((NLMSG_OK(nlHdr, readLen) == 0) || (nlHdr->nlmsg_type == NLMSG_ERROR)) {
			dlog_error("Error in recieved packet");
			return -1;
		}

		/* Check if the its the last message */
		if(nlHdr->nlmsg_type == NLMSG_DONE) {
			break;
		} else {
			/* Else move the pointer to buffer appropriately */
			bufPtr += readLen;
			msgLen += readLen;
		}

		/* Check if its a multi part message */
		if((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0) {
			/* return if its not */
			break;
		}
	} while((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));

	return msgLen;
}


/* For parsing the route info returned */
static void parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo, char *gateway)
{
	struct rtmsg *rtMsg;
	struct rtattr *rtAttr;
	int rtLen;
	char *tempBuf = NULL;

	tempBuf = (char *)r_malloc(100);
	rtMsg = (struct rtmsg *)NLMSG_DATA(nlHdr);

	/* If the route is not for AF_INET or does not belong to main routing table
	then return. */
	if((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN)) {
		return;
	}


	/* get the rtattr field */
	rtAttr = (struct rtattr *)RTM_RTA(rtMsg);
	rtLen = RTM_PAYLOAD(nlHdr);

	for(; RTA_OK(rtAttr, rtLen); rtAttr = RTA_NEXT(rtAttr, rtLen)) {
		switch(rtAttr->rta_type) {
			case RTA_OIF:
				if_indextoname(*(int *)RTA_DATA(rtAttr), rtInfo->ifName);
				break;

			case RTA_GATEWAY:
				rtInfo->gateWay = *(u_int *)RTA_DATA(rtAttr);
				break;

			case RTA_PREFSRC:
				rtInfo->srcAddr = *(u_int *)RTA_DATA(rtAttr);
				break;

			case RTA_DST:
				rtInfo->dstAddr = *(u_int *)RTA_DATA(rtAttr);
				break;
		}
	}

	if(strstr((char *)inet_ntoa(*(struct in_addr *)(rtInfo->dstAddr)), "0.0.0.0")) {
		dlog_debug("gateWay:[%s]\n", (char *)inet_ntoa(*(struct in_addr *)rtInfo->gateWay));
		sprintf(gateway, "%s", (char *)inet_ntoa(*(struct in_addr *)rtInfo->gateWay));
	}

	//printRoute(rtInfo);
	free(tempBuf);
	return;
}
#endif

unsigned int ReachGetGateWay(char *interface_name)
{
	FILE *fp;
	char buf[512];
	uint32_t gateway = 0;
	// route
	fp = fopen("/proc/net/route", "r");

	if (fp == NULL)
	{
		dlog_error("open file /proc/net/route failed");
		return -1;
	}

	while (fgets((char *)buf, sizeof(buf), fp) != NULL)
	{
		unsigned long dest, gate;
		char buffer[20] = {0};
		dest = gate = 0;
		sscanf(buf, "%s%lx%lx", buffer, &dest, &gate);

		if (dest == 0 && gate != 0 && (0 == strcmp(buffer, (char *)interface_name)))
		{
			gateway = gate;
			struct in_addr *addr = (struct in_addr *)&gateway;
			// dlog_debug("    GateWay:%s\n", inet_ntoa(*addr));

			break;
		}
	}

	fclose(fp);
	fp = NULL;
	return gateway;
}

unsigned int ReachGetIPaddr(char *interface_name)
{
	int s;
	unsigned int ip;

	if ((s = RH_Socket(__FILE__, (char *)__func__, PF_INET, SOCK_STREAM, 0)) < 0)
	{
		dlog_error("ReachGetIPaddr:Socket:s:%d\n", s);
		return -1;
	}

	struct ifreq ifr;

	strcpy(ifr.ifr_name, interface_name);

	if (ioctl(s, SIOCGIFADDR, &ifr) < 0)
	{
		dlog_error("ReachGetIPaddr:ioctl error");
		return -1;
	}

	struct sockaddr_in *ptr;

	ptr = (struct sockaddr_in *)&ifr.ifr_ifru.ifru_addr;

	dlog_debug(" [ReachGetIPaddr] IP:%s\n", inet_ntoa(ptr->sin_addr));

	memcpy(&ip, &ptr->sin_addr, 4);

	RH_Close(__FILE__, (char *)__func__, s);

	return ip;
}

int ReachGetIPaddrstring(char *interface_name, char *ipaddr)
{
	int s;

	if (NULL == ipaddr)
	{
		dlog_error("ReachGetIPaddrstring:NULL == ipaddr");
		return -1;
	}

	if ((s = RH_Socket(__FILE__, (char *)__func__, PF_INET, SOCK_STREAM, 0)) < 0)
	{
		dlog_error("ReachGetIPaddrstring:Socket s:%d\n", s);
		return -1;
	}

	struct ifreq ifr;

	strcpy(ifr.ifr_name, interface_name);

	if (ioctl(s, SIOCGIFADDR, &ifr) < 0)
	{
		//		dlog_error("ReachGetIPaddrstring:ioctl interface_name[%s]", interface_name);
		RH_Close(__FILE__, (char *)__func__, s);
		return -1;
	}

	struct sockaddr_in *ptr;

	ptr = (struct sockaddr_in *)&ifr.ifr_ifru.ifru_addr;

	snprintf(ipaddr, IPADDR_LEN, "%s", inet_ntoa(ptr->sin_addr));

	// dlog_debug(" ReachGetIPaddrstring:[GetIPaddr] IP:%s\n", ipaddr);

	RH_Close(__FILE__, (char *)__func__, s);

	return 0;
}

unsigned int ReachGetNetmask(char *interface_name)
{
	int s;
	unsigned int ip;

	if ((s = RH_Socket(__FILE__, (char *)__func__, PF_INET, SOCK_STREAM, 0)) < 0)
	{
		dlog_error("ReachGetNetmask:Socket");
		return -1;
	}

	struct ifreq ifr;

	strcpy(ifr.ifr_name, interface_name);

	if (ioctl(s, SIOCGIFNETMASK, &ifr) < 0)
	{
		dlog_error("ReachGetNetmask:ioctl");
		RH_Close(__FILE__, (char *)__func__, s);
		return -1;
	}

	struct sockaddr_in *ptr;

	ptr = (struct sockaddr_in *)&ifr.ifr_ifru.ifru_netmask;

	// dlog_debug("ReachGetNetmask:Netmask:%s\n", inet_ntoa(ptr->sin_addr));

	memcpy(&ip, &ptr->sin_addr, 4);

	RH_Close(__FILE__, (char *)__func__, s);

	return ip;
}

unsigned int ReachGetBroadcast(char *interface_name)
{
	int s;
	unsigned int ip;

	if ((s = RH_Socket(__FILE__, (char *)__func__, PF_INET, SOCK_STREAM, 0)) < 0)
	{
		dlog_error("GetBroadcast:Socket");
		return -1;
	}

	struct ifreq ifr;

	strcpy(ifr.ifr_name, interface_name);

	if (ioctl(s, SIOCGIFBRDADDR, &ifr) < 0)
	{
		dlog_error("GetBroadcast:ioctl");
		RH_Close(__FILE__, (char *)__func__, s);
		return -1;
	}

	struct sockaddr_in *ptr;

	ptr = (struct sockaddr_in *)&ifr.ifr_ifru.ifru_broadaddr;

	dlog_debug("GetBroadcast:Broadcast:%s\n", inet_ntoa(ptr->sin_addr));

	memcpy(&ip, &ptr->sin_addr, 4);

	RH_Close(__FILE__, (char *)__func__, s);

	return ip;
}

// 获取MAC地址
int get_lan_mac(char *macaddr)
{
	struct ifreq ifr;
	int sockfd;

	char *name = "eth0";
	if (strlen(name) >= IFNAMSIZ)
		return -1;
	strcpy(ifr.ifr_name, name);
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	// get HWaddr
	if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == -1)
	{
		close(sockfd);
		return -1;
	}
	memcpy(macaddr, ifr.ifr_hwaddr.sa_data, 16);
	// printf("get_lan_mac: %02X:%02X:%02X:%02X:%02X:%02X\n",macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
	close(sockfd);
	return 0;
}

int ReachGetGateWaychar(char *interface_name, char *ipaddr)
{
	FILE *fp;
	char buf[512];
	uint32_t gateway = 0;
	// route
	fp = fopen("/proc/net/route", "r");

	if (fp == NULL)
	{
		printf("open file /proc/net/route failed");
		return -1;
	}

	while (fgets((char *)buf, sizeof(buf), fp) != NULL)
	{
		unsigned long dest, gate;
		char buffer[20] = {0};
		dest = gate = 0;
		sscanf(buf, "%s%lx%lx", buffer, &dest, &gate);

		if (dest == 0 && gate != 0 && (0 == strcmp(buffer, (char *)interface_name)))
		{
			gateway = gate;
			struct in_addr *addr = (struct in_addr *)&gateway;
			printf("GateWay:%s\n", inet_ntoa(*addr));
			ipaddr[0] = ((unsigned char *)&gateway)[0];
			ipaddr[1] = ((unsigned char *)&gateway)[1];
			ipaddr[2] = ((unsigned char *)&gateway)[2];
			ipaddr[3] = ((unsigned char *)&gateway)[3];

			break;
		}
	}

	fclose(fp);
	fp = NULL;
	return 0;
}

int ReachGetIPaddrchar(char *interface_name, char *ipaddr)
{
#if 0
	int s;

	if(NULL == ipaddr) {
		printf("ReachGetIPaddrstring:NULL == ipaddr");
		return -1;
	}

	if((s = RH_Socket(__FILE__, (char *) __func__, PF_INET, SOCK_STREAM, 0)) < 0) {
		printf("ReachGetIPaddrstring:Socket s:%d\n", s);
		RH_Close(__FILE__, (char *)__func__, s);
		return -1;
	}

	struct ifreq ifr;

	strcpy(ifr.ifr_name, interface_name);

	if(ioctl(s, SIOCGIFADDR, &ifr) < 0) {
		printf("ReachGetIPaddrstring:ioctl");
		RH_Close(__FILE__, (char *)__func__, s);
		return -1;
	}

	struct sockaddr_in *ptr;

	ptr = (struct sockaddr_in *) &ifr.ifr_ifru.ifru_addr;

	//snprintf(ipaddr, IPADDR_LEN, "%s", inet_ntoa(ptr->sin_addr));



	//ipaddr = (unsigned char*)&ptr->sin_addr;
	ipaddr[0] = ((unsigned char*)&ptr->sin_addr)[0];
	ipaddr[1] = ((unsigned char*)&ptr->sin_addr)[1];
	ipaddr[2] = ((unsigned char*)&ptr->sin_addr)[2];
	ipaddr[3] = ((unsigned char*)&ptr->sin_addr)[3];


	RH_Close(__FILE__, (char *)__func__, s);

	return 0;
#else
	return ReachGetIPaddrstring(interface_name, ipaddr);
#endif
}

int ReachGetNetmaskchar(char *interface_name, char *ipaddr)
{
	int s;
	unsigned int ip;

	if ((s = RH_Socket(__FILE__, (char *)__func__, PF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("ReachGetNetmask:Socket");
		return -1;
	}

	struct ifreq ifr;

	strcpy(ifr.ifr_name, interface_name);

	if (ioctl(s, SIOCGIFNETMASK, &ifr) < 0)
	{
		printf("ReachGetNetmask:ioctl");
		return -1;
	}

	struct sockaddr_in *ptr;

	ptr = (struct sockaddr_in *)&ifr.ifr_ifru.ifru_netmask;

	// printf("ReachGetNetmask:Netmask:%s\n", inet_ntoa(ptr->sin_addr));

	// memcpy(&ip, &ptr->sin_addr, 4);

	ipaddr[0] = ((unsigned char *)&ptr->sin_addr)[0];
	ipaddr[1] = ((unsigned char *)&ptr->sin_addr)[1];
	ipaddr[2] = ((unsigned char *)&ptr->sin_addr)[2];
	ipaddr[3] = ((unsigned char *)&ptr->sin_addr)[3];

	RH_Close(__FILE__, (char *)__func__, s);

	return 0;
}

#define DEFAULT_ETH0_DNS_CONFIG "/etc/resolv.conf"

#define SET_ETH_STATICIP(file, ip, gateway, netmask) \
	fprintf(file, "ifconfig eth0 %s netmask %s\n"    \
				  "route add default gw %s\n",       \
			ip, netmask, gateway);

#define SET_ETH_DHCP(file) fprintf(file, "/sbin/udhcpc -b -i eth0\n");

#define SET_ETH_DNS(file, dns) fprintf(file, "nameserver %s\n", dns);

#define SET_ETH_STATICIPV6(file, ip, pregateway, gateway, net)  \
	fprintf(file, "ifconfig eth%d %s\n"                         \
				  "route -A inet6 add default gw %s dev eth%d", \
			net, ip, gateway, net);
#define SET_ETH_STATICIPV6_NODELGW(file, ip, gateway, net)      \
	fprintf(file, "ifconfig eth%d %s\n"                         \
				  "route -A inet6 add default gw %s dev eth%d", \
			net, ip, gateway, net);
#define SET_ETH_STATICIPV6_GW(file, ip, net) \
	fprintf(file, "ifconfig eth%d %s", net, ip);
#define SCOPE_LINK_STR "fe80"
#define PROCNET_IFINET6_PATH "/proc/net/if_inet6"

/*
 * 获取IPv6网关
 * @Author: wxz
 * param[in]:pInterfaceName:网卡（eth0）
 * param[in]:nType: 1,寻找link-local单播网关（fe80）  0：其他IP6地址
 * param[in]:pBcast: 网关
 * param[in]:nLength: 网关pBcast长度
 * */
int net_get_ipv6_bcast(char *pInterfaceName, char nType, char *pBcast, int nLength)
{
	FILE *pFp;
	char abuffer[80] = {'\0'};
	char aPath[128] = {'\0'};
	char *pDfstr = "default via ";
	char *p, *q;

	if (!pInterfaceName || !pBcast || nLength <= 0)
		return -1;
	// dlog_info("net_get_ipv6_bcast_LOG nType[%d] \n",nType);
	// snprintf(path, sizeof(path), "/bin/ip -6 route show dev %s", ifname);
	snprintf(aPath, sizeof(aPath), "ip -6 route show dev %s", pInterfaceName);
	pFp = popen(aPath, "r");
	if (!pFp)
		return -1;

	if (lockf(fileno(pFp), F_LOCK, 0) != 0)
	{
		// dlog_error("lockf error");
	}
	while (fgets(abuffer, sizeof(abuffer), pFp))
	{
		// dlog_info("abuffer = %s, nType[%d] \n",abuffer,nType);
		if (!strncmp(abuffer, pDfstr, strlen(pDfstr)))
		{
			p = (char *)&abuffer[strlen(pDfstr)];
			if (p)
			{
				q = strchr(p, ' ');
				if (q)
				{

					if (strncmp(p, SCOPE_LINK_STR, strlen(SCOPE_LINK_STR)) == 0)
					{
						if (nType == 1)
						{
							snprintf(pBcast, (int)(q - p + 1), "%s", p);
							if (lockf(fileno(pFp), F_ULOCK, 0) != 0)
							{
								dlog_info("lockf error");
							}
							pclose(pFp);
							return 0;
						}
					}
					else
					{
						if (nType == 0)
						{
							snprintf(pBcast, (int)(q - p + 1), "%s", p);
							if (lockf(fileno(pFp), F_ULOCK, 0) != 0)
							{
								dlog_info("lockf error");
							}
							pclose(pFp);
							return 0;
						}
					}
				}
			}
		}
	}

	if (lockf(fileno(pFp), F_ULOCK, 0) != 0)
	{
		dlog_info("lockf error");
	}
	pclose(pFp);
	return -1;
}

int ReachGetIPv6gateway(char *gateway)
{
	if (gateway == NULL)
	{
		return -1;
	}
	int ret;
	char command[256] = "route -A \'inet6\' | grep  -w  \"UG\" | awk  \'{ print $2 }\'";
	char freadbuf[64] = {0};

	ret = network_order_getInfo(command, freadbuf, sizeof(freadbuf));
	if (ret > 0)
	{
		freadbuf[strlen(freadbuf) - 1] = '\0';
		memcpy(gateway, freadbuf, sizeof(freadbuf));
	}
	else
	{
		dlog_debug("cjm:no get gw\n");
	}

	return ret;
}

/*
 * 清除所有已绑定的网关，除了默认单播网关（fe80）
 * @Author: wxz
 * */
static int del_ipv6Bcast_addr()
{

	int ret;
	char aCommand[256] = {0};
	char aIpv6Bcast[64] = {0};
	net_get_ipv6_bcast(ETH0_INTERFACE, 0, aIpv6Bcast, sizeof(aIpv6Bcast));
	while (strlen(aIpv6Bcast) > 0)
	{
		snprintf(aCommand, sizeof(aCommand), "route -A inet6 del default gw %s dev eth0", aIpv6Bcast);
		printf("del ipv6Bcast %s\n", aIpv6Bcast);
		if (system(aCommand) < 0)
		{
			dlog_info("del_ipv6Bcast_addr system error");
		}
		memset(aIpv6Bcast, '\0', sizeof(aIpv6Bcast));
		net_get_ipv6_bcast(ETH0_INTERFACE, 0, aIpv6Bcast, sizeof(aIpv6Bcast));
	}

	return 0;
}

/*
 * 清除所有ipv6地址，除了默认单播地址（fe80）
 * @Author: wxz
 * */
static int del_ipv6_addr()
{

	int ret;
	char aCommand[256] = {0};
	char aIpv6[64] = {0};

	net_get_ipv6_ifaddr(ETH0_INTERFACE, 0, aIpv6, sizeof(aIpv6));
	while (strlen(aIpv6) > 0)
	{
		snprintf(aCommand, sizeof(aCommand), "ifconfig eth0 del %s", aIpv6);
		printf("del ipv6 %s\n", aIpv6);
		if (system(aCommand) < 0)
		{
			dlog_info("del_ipv6_addr system error");
		}
		memset(aIpv6, '\0', sizeof(aIpv6));
		net_get_ipv6_ifaddr(ETH0_INTERFACE, 0, aIpv6, sizeof(aIpv6));
	}

	return 0;
}

int ipv6_recover_defaultsetting(const char *pConfigFile)
{
	// 设置前，先清除已添加的ipv6和网关
	del_ipv6_addr();
	del_ipv6Bcast_addr();

	ReachSetIPV6(pConfigFile, "2019:c118:1208:0:6453:1bff:fed7:c001/64", "2019:c118:1208:0:6453::", 0);
	chmod(pConfigFile, S_IRUSR | S_IWUSR | S_IXUSR);
	return 0;
}

int ReachSetIPV6(const char *filename, char *ip, char *gw, int net)
{
	if ((filename == NULL) || (ip == NULL) || (gw == NULL))
	{
		dlog_debug("file == NULL\n");
		return -1;
	}
	if ((strlen(ip) == 0) || (strlen(gw) == 0))
		return -1;

	// 设置前，先清除已添加的ipv6和网关
	del_ipv6_addr();
	del_ipv6Bcast_addr();

	FILE *file = fopen(filename, "w");

	if (file != NULL)
	{

		SET_ETH_STATICIPV6_NODELGW(file, ip, gw, net);

		fclose(file);
		file = NULL;
	}

	if (chown(filename, 0, 0) != 0)
	{
		dlog_info("chown error");
	}
	return 0;
}

int ReachSetIP(char *filename, char *ip, char *netmask, char *gw)
{
	if (filename == NULL)
	{
		dlog_debug("file == NULL\n");
		return -1;
	}

	if (chown(filename, 0, 0) != 0)
	{
		dlog_info("chown error");
	}
	if (chmod(filename, S_IRUSR | S_IWUSR | S_IXUSR) != 0)
	{
		dlog_info("chmod error");
	}
	FILE *file = fopen(filename, "w");

	if (file != NULL)
	{
		SET_ETH_STATICIP(file, ip, gw, netmask);
		fclose(file);
		file = NULL;
	}

	if (chown(filename, 0, 0) != 0)
	{
		dlog_info("chown error");
	}
	if (chmod(filename, S_IRUSR | S_IWUSR | S_IXUSR) != 0)
	{
		dlog_info("chmod error");
	}

	return 0;
}

int ReachSetDHCP(char *filename)
{
	FILE *file = fopen(filename, "w");

	if (file != NULL)
	{
		SET_ETH_DHCP(file);
		fclose(file);
		file = NULL;
	}

	if (chown(filename, 0, 0) != 0)
	{
		dlog_info("chown error");
	}
	if (chmod(filename, S_IRUSR | S_IWUSR | S_IXUSR) != 0)
	{
		dlog_info("chmod error");
	}
	return 0;
}

int ReachGetDHCP(char *filename)
{
	FILE *fp = NULL;
	char buf[512] = {0};
	char *temp = NULL;
	int dhcp = 0;
	fp = fopen(filename, "r");

	if (NULL == fp)
	{
		dlog_error("open file %s failed", filename);
		return 0;
	}

	while (fgets((char *)buf, sizeof(buf), fp) != NULL)
	{
		temp = strstr(buf, "udhcpc");

		if (temp)
		{
			dhcp = 1;
		}
		else
		{
			dhcp = 0;
		}
	}

	fclose(fp);
	fp = NULL;
	return dhcp;
}

int ReachSetDns(char *dns)
{
	FILE *file = fopen(DNS_CFG, "w");

	if (file != NULL)
	{
		SET_ETH_DNS(file, dns);
		fclose(file);
		file = NULL;
	}

	if (chown(DNS_CFG, 0, 0) != 0)
	{
		dlog_info("chown error");
	}
	if (chmod(DNS_CFG, S_IRUSR | S_IWUSR | S_IXUSR) != 0)
	{
		dlog_info("chmod error");
	}
	return 0;
}
static void cpy_ip(char *ip, char *conIp)
{
	int i = 0;
	int j = 0;

	while (conIp[i++])
	{
		if (conIp[i] > 45 && conIp[i] < 58 && 47 != conIp[i])
		{
			ip[j++] = conIp[i];
		}
	}
}

int ReachGetDns(char *dns)
{
	FILE *fp = NULL;
	char buf[512] = {0};
	char *temp = NULL;
	// uint32_t gateway = 0;
	//  route
	fp = fopen(DNS_CFG, "r");

	if (fp == NULL)
	{
		dlog_error("open file %s failed", DNS_CFG);
		return -1;
	}

	while (fgets((char *)buf, sizeof(buf), fp) != NULL)
	{
		temp = strstr(buf, "nameserver ");

		if (temp)
		{
			cpy_ip(dns, buf);
			fclose(fp);
			fp = NULL;
			return 0;
		}
	}

	fclose(fp);
	fp = NULL;
	return -1;
}

// 获取MAC地址
int get_mac(char *pMac, int nLen) // 返回值是实际写入char * mac的字符个数（不包括'\0'）
{
	struct ifreq ifreq; // ifreq结构体常用来配置和获取ip地址
	int nSocket;

	if ((nSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return -1;
	}
	strcpy(ifreq.ifr_name, "eth0"); // Currently, only get eth0

	if (ioctl(nSocket, SIOCGIFHWADDR, &ifreq) < 0)
	{
		perror("ioctl");
		return -1;
	}
	close(nSocket);
	return snprintf(pMac, nLen, "%02X:%02X:%02X:%02X:%02X:%02X", (unsigned char)ifreq.ifr_hwaddr.sa_data[0],
					(unsigned char)ifreq.ifr_hwaddr.sa_data[1], (unsigned char)ifreq.ifr_hwaddr.sa_data[2],
					(unsigned char)ifreq.ifr_hwaddr.sa_data[3], (unsigned char)ifreq.ifr_hwaddr.sa_data[4],
					(unsigned char)ifreq.ifr_hwaddr.sa_data[5]);
}

int ReachSetMac(char *mac)
{
	FILE *file = fopen(WAC_PATH, "w");

	if (file != NULL)
	{
		fprintf(file, "%s\n", mac);
		fclose(file);
		file = NULL;
	}
	return 0;
}

/*
 * 获取IPv6网路地址
 * @Author: wxz
 * param[in]:pInterfaceName:网卡（eth0）
 * param[in]:nType: 1,寻找link-local单播网关（fe80）  0：其他IP6地址
 * param[in]:pAddr: ipv6地址
 * param[in]:nLength: pAddr长度
 * */
int net_get_ipv6_ifaddr(char *pInterfaceName, char nType, char *pAddr, int nLength) //, char *prefix, int length2)
{
	FILE *pFp;
	char aAddr6[46] = {0}, dev_name[21] = {0};
	char aAddr6p[8][5];
	int nLength_of_prefix, nScope_value, nIf_flags, nIf_index;
	struct sockaddr_in6 stSap;
	memset(&stSap, 0, sizeof(struct sockaddr_in6));

	if (!pInterfaceName || !pAddr || nLength <= 0) //|| !prefix || length2 <= 0)
		return -1;

	pFp = fopen(PROCNET_IFINET6_PATH, "r");
	if (!pFp)
		return -1;

	if (lockf(fileno(pFp), F_LOCK, 0) != 0)
	{
		// dlog_error("lockf error");
	}

	while (fscanf(pFp, "%4s%4s%4s%4s%4s%4s%4s%4s %02x %02x %02x %02x %20s\n",
				  aAddr6p[0], aAddr6p[1], aAddr6p[2], aAddr6p[3],
				  aAddr6p[4], aAddr6p[5], aAddr6p[6], aAddr6p[7],
				  &nIf_index, &nLength_of_prefix, &nScope_value, &nIf_flags, dev_name) != EOF)
	{
		if (!strcmp(dev_name, pInterfaceName))
		{
			sprintf(aAddr6, "%s:%s:%s:%s:%s:%s:%s:%s",
					aAddr6p[0], aAddr6p[1], aAddr6p[2], aAddr6p[3],
					aAddr6p[4], aAddr6p[5], aAddr6p[6], aAddr6p[7]);
			{
				// dlog_info("addr6 : %s\n",aAddr6);
				char pTest[64] = {0};
				if (strncmp(aAddr6p[0], SCOPE_LINK_STR, strlen(SCOPE_LINK_STR)) == 0)
				{
					// fe80 Scope:Link
					if (nType == 1)
					{
						inet_pton(AF_INET6, aAddr6, &stSap.sin6_addr);
						inet_ntop(AF_INET6, &stSap.sin6_addr, pAddr, nLength);
						snprintf(pTest, nLength, "%s/%d", pAddr, nLength_of_prefix);
						memcpy(pAddr, pTest, nLength);
						// dlog_info("pAddr=%s, nLength %d\n",pAddr,nLength);
						// snprintf(prefix, length2, "%d", length_of_prefix);
						if (lockf(fileno(pFp), F_ULOCK, 0) != 0)
						{
							dlog_error("lockf error");
						}
						fclose(pFp);
						return 0;
					}
				}
				else
				{
					// 2001 Scope:Global
					if (nType == 0)
					{
						inet_pton(AF_INET6, aAddr6, &stSap.sin6_addr);
						inet_ntop(AF_INET6, &stSap.sin6_addr, pAddr, nLength);
						snprintf(pTest, nLength, "%s/%d", pAddr, nLength_of_prefix);
						memcpy(pAddr, pTest, nLength);
						dlog_info("pAddr=%s, nLength %d\n", pAddr, nLength);
						if (lockf(fileno(pFp), F_ULOCK, 0) != 0)
						{
							dlog_info("lockf error");
						}
						fclose(pFp);
						return 0;
					}
				}
			}
		}
	}
	if (lockf(fileno(pFp), F_ULOCK, 0) != 0)
	{
		dlog_info("lockf error");
	}
	fclose(pFp);
	return -1;
}

// fhs add
void net_get_ipv6(char *filename, char *ip)
{
	FILE *fd;

	char buff[512];
	fd = fopen(filename, "r");
	if (fd == NULL)
		return;
	if (fgets(buff, sizeof(buff), fd) == NULL)
	{
		dlog_error("fgets error");
	}
	memcpy(ip, buff + 14, strlen(buff + 14));
	fclose(fd);
	// printf("fhs add : %s\n",ip);
}

void net_get_ipv6Bcast(char *filename, char *pIpv6Bcast)
{
	FILE *fd;

	char buff[512];
	fd = fopen(filename, "r");
	if (fd == NULL)
	{
		return;
	}
	if (fgets(buff, sizeof(buff), fd) == NULL)
	{
		dlog_error("fgets error");
	}
	memcpy(pIpv6Bcast, buff + 14, strlen(buff + 14));
	fclose(fd);
	// printf("fhs add : %s\n",pIpv6Bcast);
}

int ReachGetIPv6addrstring(char *interface_name, char *ipaddr)
{
	struct ifaddrs *ifa, *ifa_tmp;
	char addr[64] = {0};

	if (getifaddrs(&ifa) == -1)
	{
		perror("getifaddrs failed");
		return -1;
	}

	ifa_tmp = ifa;
	while (ifa_tmp)
	{
		if ((ifa_tmp->ifa_addr) && (ifa_tmp->ifa_addr->sa_family == AF_INET6))
		{

			// create IPv6 string
			struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)ifa_tmp->ifa_addr;
			inet_ntop(AF_INET6, &in6->sin6_addr, addr, sizeof(addr));
			if (strncmp(ifa_tmp->ifa_name, interface_name, strlen(interface_name)) == 0)
			{
				if (strncmp(addr, "::1", strlen("::1")) != 0)
				{
					memcpy(ipaddr, addr, strlen(addr));
					dlog_debug("cjm:ipv6addr:%s\n", addr);
					//					printf("[ipv6addr:%s]\n", addr;
					return 0;
				}
			}
		}
		ifa_tmp = ifa_tmp->ifa_next;
	}

	return -1;
}

static void cpy_ipv6dns(char *ip, char *conIp)
{
	int i = 0;
	int j = 0;

	while (conIp[i++])
	{
		if ((conIp[i] >= '0' && conIp[i] <= ':') || (conIp[i] >= 'a' && conIp[i] <= 'f'))
		{
			ip[j++] = conIp[i];
		}
	}
}

/* 设置ipv6网关 */
int ReachSetIPV6Dns(char *pDns)
{
	FILE *pFile = fopen(DNS_CFG, "a+");

	if (pFile != NULL)
	{
		SET_ETH_DNS(pFile, pDns);
		fclose(pFile);
		pFile = NULL;
	}

	if (chown(DNS_CFG, 0, 0) != 0)
	{
		dlog_error("chown %s failed", DNS_CFG);
	}
	return 0;
}

int ReachGetIPv6dns(char *dns)
{
	if (dns == NULL)
	{
		return -1;
	}

	FILE *fp = NULL;
	char buf[512] = {0};
	char *temp = NULL;
	// uint32_t gateway = 0;
	//  route
	fp = fopen(DNS_CFG, "r");

	if (fp == NULL)
	{
		dlog_error("open file %s failed", DNS_CFG);
		return -1;
	}

	while (fgets((char *)buf, sizeof(buf), fp) != NULL)
	{

		temp = strstr(buf, "nameserver ");
		if (temp)
		{
			temp = NULL;
			if (fgets((char *)buf, sizeof(buf), fp) != NULL)
			{
				if (strstr(buf, ".") != NULL)
				{
					break;
				}
				temp = strstr(buf, "nameserver ");
				if (temp)
				{
					temp += strlen("nameserver ") - 1;
					cpy_ipv6dns(dns, temp);
					printf("[cjm]cpy_ipv6dns %s\n", dns);
					fclose(fp);
					fp = NULL;
					return 0;
				}
			}
			else
			{
				fclose(fp);
				fp = NULL;
				return 0;
			}
		}
	}

	fclose(fp);
	fp = NULL;
	return -1;
}

/*设置网络*/
int network_set_ipv4(char *pFileName, char *pIp, char *pNetMask, char *pGateway, char *pEthName)
{
	if (NULL == pFileName || NULL == pIp ||
		NULL == pNetMask || NULL == pEthName)
	{
		dlog_debug("file == NULL\n");
		return -1;
	}

	if (chown(pFileName, 0, 0) != 0)
	{
		dlog_error("chown %s failed", pFileName);
	}
	if (chmod(pFileName, S_IRUSR | S_IWUSR | S_IXUSR) != 0)
	{
		dlog_error("chmod %s failed", pFileName);
	}

	FILE *pFile = fopen(pFileName, "w");
	if (pFile != NULL)
	{
		fprintf(pFile, "ifconfig %s %s netmask %s\nroute add default gw %s\n",
				pEthName, pIp, pNetMask, pGateway);
		fclose(pFile);
		pFile = NULL;
		if (chown(pFileName, 0, 0) != 0)
		{
			dlog_error("chown %s failed", pFileName);
		}
		if (chmod(pFileName, S_IRUSR | S_IWUSR | S_IXUSR) != 0)
		{
			dlog_error("chmod %s failed", pFileName);
		}
	}
	else
	{
		return -1;
	}

	return 0;
}

/*清除ipv6*/
static int network_clear_ipv6Addr(const char *pEthName)
{
	char achCmd[256] = {0};
	char achIpv6[64] = {0};
	int nLen = 0;
	if (strcmp(pEthName, ETH0_INTERFACE) == 0)
	{
		net_get_ipv6_ifaddr(ETH0_INTERFACE, 0, achIpv6, sizeof(achIpv6));
		nLen = strlen(achIpv6);
		if (nLen > 0)
		{
			snprintf(achCmd, sizeof(achCmd), "ifconfig %s del %s", pEthName, achIpv6);
			printf("del ipv6 %s\n", achIpv6);
			if (system(achCmd) != 0)
			{
				dlog_error("system error\n");
			}

			memset(achIpv6, '\0', sizeof(achIpv6));
			net_get_ipv6_ifaddr(ETH0_INTERFACE, 0, achIpv6, sizeof(achIpv6));
			nLen = strlen(achIpv6);
			if (nLen > 0)
			{
				dlog_error("network_clear_ipv6Addr error\n");
				return -1;
			}
		}
		else
		{
			return -1;
		}
	}
	else if (0 == strcmp(pEthName, ETH1_INTERFACE))
	{
		net_get_ipv6_ifaddr(ETH1_INTERFACE, 0, achIpv6, sizeof(achIpv6));
		nLen = strlen(achIpv6);
		if (nLen > 0)
		{
			snprintf(achCmd, sizeof(achCmd), "ifconfig %s del %s", pEthName, achIpv6);
			printf("del ipv6 %s\n", achIpv6);
			if (system(achCmd) != 0)
			{
				dlog_error("system error\n");
			}

			memset(achIpv6, '\0', sizeof(achIpv6));
			net_get_ipv6_ifaddr(ETH1_INTERFACE, 0, achIpv6, sizeof(achIpv6));
			nLen = strlen(achIpv6);
			if (nLen > 0)
			{
				dlog_error("network_clear_ipv6Addr error\n");
				return -1;
			}
		}
		else
		{
			return -1;
		}
	}

	return 0;
}

/* 清除所有已绑定的网关，除了默认单播网关（fe80）*/
static int network_clear_ipv6BCastAddr(const char *pEthName)
{
	char achCmd[256] = {0};
	char aIpv6Bcast[64] = {0};
	int nLen = 0;
	if (strcmp(pEthName, ETH0_INTERFACE) == 0)
	{
		net_get_ipv6_bcast(ETH0_INTERFACE, 0, aIpv6Bcast, sizeof(aIpv6Bcast));
		nLen = strlen(aIpv6Bcast);
		if (nLen > 0)
		{
			snprintf(achCmd, sizeof(achCmd), "route -A inet6 del default gw %s dev %s", aIpv6Bcast, pEthName);
			dlog_info("del ipv6 %s\n", aIpv6Bcast);
			if (system(achCmd) != 0)
			{
				dlog_error("system error\n");
			}

			memset(aIpv6Bcast, '\0', sizeof(aIpv6Bcast));
			net_get_ipv6_bcast(ETH0_INTERFACE, 0, aIpv6Bcast, sizeof(aIpv6Bcast));
			nLen = strlen(aIpv6Bcast);
			if (nLen > 0)
			{
				dlog_error("network_clear_ipv6Addr error\n");
				return -1;
			}
		}
		else
		{
			dlog_error("network_clear_ipv6Addr error\n");
			return -1;
		}
	}
	else if (0 == strcmp(pEthName, ETH1_INTERFACE))
	{
		net_get_ipv6_bcast(ETH1_INTERFACE, 0, aIpv6Bcast, sizeof(aIpv6Bcast));
		nLen = strlen(aIpv6Bcast);
		if (nLen > 0)
		{
			snprintf(achCmd, sizeof(achCmd), "route -A inet6 del default gw %s dev %s", aIpv6Bcast, pEthName);
			dlog_info("del ipv6 %s\n", aIpv6Bcast);
			if (system(achCmd) != 0)
			{
				dlog_error("system error\n");
			}

			memset(aIpv6Bcast, '\0', sizeof(aIpv6Bcast));
			net_get_ipv6_bcast(ETH1_INTERFACE, 0, aIpv6Bcast, sizeof(aIpv6Bcast));
			nLen = strlen(aIpv6Bcast);
			if (nLen > 0)
			{
				dlog_error("network_clear_ipv6Addr error\n");
				return -1;
			}
		}
		else
		{
			dlog_error("network_clear_ipv6Addr error\n");
			return -1;
		}
	}

	return 0;
}

/*设置ipv6网络*/
int network_set_ipv6(char *pFileName, char *pIpv6, char *pGateway, char *pEthName)
{
	if (NULL == pFileName || NULL == pIpv6 || NULL == pGateway || NULL == pEthName)
	{
		dlog_error("file == NULL\n");
		return -1;
	}
	dlog_info("network_set_ipv6_log ipv6[%s], gateway[%s]\n", pIpv6, pGateway);
	if ((strlen(pIpv6) == 0) || (strlen(pGateway) == 0))
	{
		dlog_error(" IPV6 error or gateway error");
		return -1;
	}

	// 设置前，先清除已添加的ipv6和网关
	network_clear_ipv6Addr(pEthName);
	network_clear_ipv6BCastAddr(pEthName);

	FILE *pFile = fopen(pFileName, "w+");
	if (pFile != NULL)
	{
		fprintf(pFile, "ifconfig %s %s\nroute -A inet6 add default gw %s dev %s\n",
				pEthName, pIpv6, pGateway, pEthName);

		fclose(pFile);
		pFile = NULL;
		if (chown(pFileName, 0, 0) != 0)
		{
			dlog_error("chown error\n");
		}
	}
	else
	{
		dlog_info("open file[%s] error\n", pFileName);
		return -1;
	}
	return 0;
}

char **get_network_interfaces(int *pnCount)
{
	FILE *pFp = popen("awk 'NR>2{print $1}' /proc/net/dev | cut -d':' -f1", "r");
	if (!pFp)
	{
		dlog_error("popen失败");
		perror("popen");
		return NULL;
	}

	char **ppInterfaces = NULL;
	char ifname[IFNAMSIZ];
	int nIdx = 0;

	while (fscanf(pFp, "%16s", ifname) == 1)
	{
		/* 跳过环回接口 */
		if (strcmp(ifname, "lo") == 0)
			continue;

		ppInterfaces = realloc(ppInterfaces, (nIdx + 1) * sizeof(char *));
		ppInterfaces[nIdx] = strdup(ifname);
		nIdx++;
	}

	pclose(pFp);
	*pnCount = nIdx;
	return ppInterfaces;
}

char *get_default_interface()
{
	FILE *pFp = fopen("/proc/net/route", "r");
	if (!pFp)
	{
		perror("fopen");
		return NULL;
	}

	char line[256];
	char *pIface = NULL;

	/* 跳过标题行 */
	fgets(line, sizeof(line), pFp);

	while (fgets(line, sizeof(line), pFp))
	{
		char dev[IFNAMSIZ];
		char dest[IFNAMSIZ];
		char gateway[IFNAMSIZ];

		sscanf(line, "%s %s %s", dev, dest, gateway);

		if (strcmp(dest, "00000000") == 0)
		{
			pIface = strdup(dev);
			break;
		}
	}

	fclose(pFp);
	return pIface;
}

char *get_interface_ip(const char *pIfname)
{
	int nFd = socket(AF_INET, SOCK_DGRAM, 0);
	if (nFd < 0)
	{
		perror("socket");
		return NULL;
	}

	struct ifreq ifr;
	strncpy(ifr.ifr_name, pIfname, IFNAMSIZ);

	if (ioctl(nFd, SIOCGIFADDR, &ifr) == -1)
	{
		close(nFd);
		return NULL;
	}

	close(nFd);

	static char pIp[INET_ADDRSTRLEN];
	struct sockaddr_in *pAddr = (struct sockaddr_in *)&ifr.ifr_addr;
	inet_ntop(AF_INET, &pAddr->sin_addr, pIp, sizeof(pIp));
	return pIp;
}

char *get_primary_ip()
{
	int nCount = 0;
	char **ppInterfaces = get_network_interfaces(&nCount);

	if (nCount == 0)
	{
		fprintf(stderr, "No network interfaces found\n");
		dlog_error("没有网卡接口");
		return NULL;
	}

	/* 如果只有一个网卡，直接使用 */
	if (nCount == 1)
	{
		// dlog_info("只有一个网卡接口");
		char *pIp = get_interface_ip(ppInterfaces[0]);
		free(ppInterfaces[0]);
		free(ppInterfaces);
		return pIp;
	}

	char *pDefaultIface = get_default_interface();
	if (!pDefaultIface)
	{
		dlog_error("没有网卡路由");
		free(ppInterfaces);
		return NULL;
	}

	/* 检查默认网卡是否在接口列表中 */
	for (int i = 0; i < nCount; i++)
	{
		if (strcmp(ppInterfaces[i], pDefaultIface) == 0)
		{
			char *pIp = get_interface_ip(pDefaultIface);
			free(pDefaultIface);
			free(ppInterfaces[i]);
			free(ppInterfaces);
			dlog_info("找到网卡IP:%s", pIp);
			return pIp;
		}
	}

	dlog_error("默认网卡不在网卡列表中");
	free(pDefaultIface);
	free(ppInterfaces);
	return NULL;
}