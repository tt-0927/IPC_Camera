/**
 * @FilePath     : network_manage.cpp
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2024-11-02 16:30:03
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-15 13:51:59
 * @Description  : 网络管理
 */

#include "network_manage.h"
#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include <thread>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <regex>
#include "dlog.h"
#include "log_handler.h"
#include "rtsp_server.h"
// #include "gpio_ctrl.h"

/* 设置系统网络配置信息 */
int CNetworkManage::set_system_network(Network::Info_S stNetInfo, std::function<void(int)> result)
{
	auto thrRun = [this](Network::Info_S stNetInfo, std::function<void(int)> result)
	{
		int nRet;
		// 步骤1：获取互斥锁并等待设置完成
		std::unique_lock<std::mutex> lock(m_networkMutex);

		// 等待直到没有设置操作在进行
		m_networkCV.wait(lock, [this]()
						 { return !m_bNetworkSetting; });

		// 标记设置操作开始
		m_bNetworkSetting = true;
		lock.unlock(); // 释放锁，允许其他线程检查状态

		/* 设置IPv4和DNS相关信息 */
		nRet = set_ip_and_dns(stNetInfo);

		/* 写入配置文件 */
		if(nRet == 0)
		{
			Convert::write_file(NETWORK_CONFIG_FILE, stNetInfo);
			m_stInfo = stNetInfo;
		}

		/* 重启RtspServer */
		CRtspServer::instance()->reboot();

		lock.lock();
		m_bNetworkSetting = false;
		lock.unlock();
		m_networkCV.notify_all(); // 唤醒所有等待线程

		result(nRet);
	};

	std::thread thr(thrRun, stNetInfo, result);
	thr.detach();

	return 0;
}

/* 设置系统网络配置信息 */
int CNetworkManage::set_system_networkInfo(Network::Info_S stNetInfo, bool bIsImmediate)
{
	int nRet;
	
	/* 设置IPV4和DNS相关信息 */
	nRet = set_ip_and_dns(stNetInfo, bIsImmediate);
	/* 写入配置文件 */
	if(nRet == 0)
	{
		Convert::write_file(NETWORK_CONFIG_FILE, stNetInfo);
		m_bNetworkSetting = true;
		m_stInfo = stNetInfo;

		if(bIsImmediate)
		{
			if(stNetInfo.stIp.bEnableDhcp)
			{
				stNetInfo.stIp.ipv4Ip = get_dev_ip(stNetInfo.stIp.netName);
			}

			if(!stNetInfo.stIp.ipv4Ip.empty())
			{
				CRtspServer::instance()->updateNetworkConfig(stNetInfo);
			}
			
		}
	}
	
	return nRet;
}

/* 获取系统网络配置信息 */
int CNetworkManage::get_system_networkInfo(Network::Info_S &stNetInfo)
{
	dlog_debug("开始获取系统网络配置");
	std::string stInterface;
	/* 配置文件是否存在 */
	bool bFileExist = false;
	struct stat buffer;
	Network::Info_S stConfigNetInfo;

	stInterface = get_network_interface();

	std::string network_config_file_path = NETWORK_CONFIG_FILE;
	#if CAP_NETWORK_WIFI
	if(CWifiManager::instance()-> isWifiConnectedAndWiredDisconnected())//WiFi连接，有线断开
	{
		network_config_file_path = NETWORK_WIFI_CONFIG_FILE;
		stInterface = "wlan0";
		m_bNetworkSetting = false;
	}
	else {
		m_bNetworkSetting = false;
	}
	#endif

	if (stInterface.empty())
	{
		return -1;
	}

	if (stat(network_config_file_path.c_str(), &buffer) == 0)
	{
		dlog_info("网络配置文件存在 %s",network_config_file_path.c_str());
		if (!m_bNetworkSetting)
		{
			Convert::read_file(network_config_file_path.c_str(), stConfigNetInfo);
			m_stInfo = stConfigNetInfo;
			m_bNetworkSetting = true;
		}
		else
		{
			stConfigNetInfo = m_stInfo;
		}
		bFileExist = true;
	}

	stNetInfo.stIp.netName = stInterface;
	/* 获取网卡类型 */
	stNetInfo.stIp.enType = get_network_yype(stInterface);

	/* 获取IP信息和DNS信息 */
	if (!bFileExist)
	{
		dlog_info("从设备接口文件中读取网络信息并写入配置文件");
		get_ip_and_dns(stNetInfo);
		if (OK != Convert::write_file(network_config_file_path.c_str(), stNetInfo))
        {
            dlog_error("写入设备信息配置文件失败");
            return ERR;
        }
		//set_system_networkInfo(stNetInfo);
	}
	else
	{
		if (m_bNetworkSetting)
		{
			dlog_info("从结构体中读取网络信息");
		}
		else
		{
			dlog_info("配置文件中读取网络信息");
		}
		stNetInfo.stIp = stConfigNetInfo.stIp;
		if(stNetInfo.stIp.bEnableDhcp)
		{
			stNetInfo.stIp.ipv4Ip = get_dev_ip(stNetInfo.stIp.netName);
			stNetInfo.stIp.ipv4Mask = get_dev_mask(stNetInfo.stIp.netName);
			stNetInfo.stIp.ipv4Gateway = get_dev_gateway(stNetInfo.stIp.netName);
		}
		/* 获取IPV6相关信息 */
		stNetInfo.stIp.ipv6Ip = get_ipv6Address(stNetInfo.stIp.netName);
		stNetInfo.stIp.ipv6Gateway = get_ipv6Gateway(stNetInfo.stIp.netName);
		/* mac地址 */
		stNetInfo.stIp.physicalAddress = get_macAddress(stNetInfo.stIp.netName);

		get_networkDns(stNetInfo.stDns);
		
		/* 多播 */
		stNetInfo.stIp.bEnableMulticast = stConfigNetInfo.stIp.bEnableMulticast;
		stNetInfo.stIp.multicastAddress = stConfigNetInfo.stIp.multicastAddress;
	}

	/* 防止获取到的IP为空 */
	if(stNetInfo.stIp.ipv4Ip.empty())
	{
		stNetInfo.stIp.ipv4Ip = getLocalIP(stNetInfo.stIp.netName);

		if(stNetInfo.stIp.ipv4Ip.empty())
		{
			stNetInfo.stIp.ipv4Ip = getIPByIfconfig(stNetInfo.stIp.netName);

		}
	}
	dlog_debug("结束获取系统网络配置");

	return 0;
}

/* 激活自动配置网络 */
int CNetworkManage::register_auto_config()
{
	int nResult;
	Network::Info_S stNetInfo;
	std::string stInterface = get_network_interface();
	if (stInterface.size() == 0)
	{
		return -1;
	}

	stNetInfo.stIp.netName = stInterface;
	/* 网卡初始化参数 */
	stNetInfo.stIp.nMtu = DEFAULT_MTU;
	/* 检查是否已连接到路由器 */
	const std::string strGateway = get_dev_gateway(stNetInfo.stIp.netName);
	std::string strCommand = "ping -c 1 " + strGateway + " > /dev/null 2>&1";

	nResult = system(strCommand.c_str());
	if (nResult != 0)
	{
		dlog_error("无法连接到路由器，请检查网络连接");
		return -1;
	}

	/* 启用 DHCP 和 自动获取DNS */
	stNetInfo.stDns.bEnableAutoDns = true;
	stNetInfo.stIp.bEnableDhcp = true;

	nResult = set_ip_and_dns(stNetInfo);
	if (nResult < 0)
	{
		dlog_error("自动配置网络失败");
		return -1;
	}
	/* 写入配置文件 */
	Convert::write_file(NETWORK_CONFIG_FILE, stNetInfo);
	m_stInfo = stNetInfo;

	dlog_info("自动配置网络成功");

	return 0;
}

/* 激活手动配置网络 */
int CNetworkManage::register_manual_config(Register::NetWorkInfo_S &stRegNetInfo)
{
	int nRet;
	Network::Info_S stNetInfo;

	std::string stInterface = get_network_interface();

	/* 主网卡初始化参数 */
	stNetInfo.stIp.nMtu = DEFAULT_MTU;
	stNetInfo.stIp.enType = Network::AUTO;

	stNetInfo.stIp.netName = stInterface;
	stNetInfo.stIp.bEnableDhcp = stRegNetInfo.stRegIp.bEnableDhcp;
	stNetInfo.stIp.ipv4Ip = stRegNetInfo.stRegIp.strIpv4Ip;
	stNetInfo.stIp.ipv4Mask =  stRegNetInfo.stRegIp.strIpv4Mask;
	stNetInfo.stIp.ipv4Gateway =  stRegNetInfo.stRegIp.strIpv4Gateway;

	/* 是否自动获取DNS */
	stNetInfo.stDns.bEnableAutoDns = stRegNetInfo.bEnAutoDns;
	if (!stNetInfo.stDns.bEnableAutoDns)
	{
		stNetInfo.stDns.main = stRegNetInfo.strDnsMain;
		stNetInfo.stDns.standby = stRegNetInfo.strDnsStandby;
	}

	nRet = set_ip_and_dns(stNetInfo);
	if (nRet < 0)
	{
		dlog_error("设置网卡信息和DNS失败");
		return -1;
	}

	/* 写入配置文件 */
	Convert::write_file(NETWORK_CONFIG_FILE, stNetInfo);
	m_stInfo = stNetInfo;

	return 0;
}

int CNetworkManage::start_dhcp(const std::string &strInterface)
{
	std::string strCommand = START_DHCP_COMMAND + strInterface;

	/* 使用 system 函数执行命令 */
	int nResult = std::system(strCommand.c_str());
	if (nResult == 0)
	{
		dlog_debug("开启dhcp成功");
	}
	else
	{
		dlog_error("开启dhcp失败");
		return -1;
	}

	FILE *pFp = popen("pgrep -f 'udhcpc -b -i'", "r");
	if (pFp == nullptr)
	{
		dlog_error("无法获取 udhcpc 进程 ID", "");
		return -1;
	}

	char achBuffer[MAX_COMMAND_LENGTH];
	while (fgets(achBuffer, sizeof(achBuffer), pFp) != nullptr)
	{
		/* 读取进程 ID */
		int nPid = std::stoi(achBuffer);
		/* 停止进程 */
		std::string strKillCommand = "kill " + std::to_string(nPid);
		int nKillResult = std::system(strKillCommand.c_str());
		if (nKillResult != 0)
		{
			dlog_error("停止进程 %d 失败", nPid);
			pclose(pFp);
			return -1;
		}
	}

	pclose(pFp);
	dlog_debug("已成功停止 dhcpcd 进程");

	return 0;
}

/* 配置网络 */
int CNetworkManage::configure_network(const std::string &strInterface, const std::string &strIp, const std::string &strMask, const std::string &strGateway)
{
	int nRet;
	if (strIp.empty() || strInterface.empty() || strMask.empty() || strGateway.empty())
	{
		nRet = -1;
		dlog_error("设置静态网络失败！网卡名、IP、子网掩码、网关信息之一为空");
		return nRet;
	}
	else
	{
		nRet = check_ip_usage(strIp, strInterface);
	}

	/* 清空网卡ip */
	// std::string strIpFlashCommand = "ip addr flush dev " + strInterface;
	// nRet = std::system(strIpFlashCommand.c_str());
	// if (nRet < 0)
	// {
	// 	dlog_error("在网卡 [%s]设置网络IP失败，IP:[%s] 清空ip失败", strInterface.c_str(), strIp.c_str());
	// 	return nRet;
	// }

	/* 设置IP地址和子网掩码 */
	std::string strIpCommand = "ifconfig " + strInterface + " " + strIp + " netmask " + strMask;
	int nResult = std::system(strIpCommand.c_str());
	if (nResult != 0)
	{
		dlog_error("在网卡 [%s] 设置IP地址: %s和子网掩码: %s 失败", strInterface.c_str(), strIp.c_str(), strMask.c_str());
		return -1;
	}

	/* 删除默认网关的命令 */
	std::string strDeleteGetWayCommand = REMOVE_DEFAULT_GATEWAY_COMMAND + strInterface;
	nResult = std::system(strDeleteGetWayCommand.c_str());
	if (nResult != 0)
	{
		dlog_error("网卡[%s] 删除默认网关失败", strInterface.c_str());
		return nResult;
	}

	/* 设置网关 */
	std::string gatewayCommand = SET_GW_COMMAND + strGateway + " " + strInterface;
	int gwResult = std::system(gatewayCommand.c_str());
	if (gwResult != 0)
	{
		dlog_error("在网卡 [%s] 设置网关: %s 失败", strInterface.c_str(), strGateway.c_str());
		return -1;
	}

	dlog_info("在网卡 [%s] 成功设置网络，ip： %s  mask：%s GateWay：%s", strInterface.c_str(), strIp.c_str(), strMask.c_str(), strGateway.c_str());
	return 0;
}

/* 获取网卡类型 */
Network::NetTypeMode_E CNetworkManage::get_network_yype(std::string strNetName)
{
	dlog_debug("获取网卡类型");
	if (autoneg_enabled(strNetName))
	{
		dlog_debug("网卡是自动协商");
		return Network::AUTO;
	}
	else
	{
		std::string strSpeed = get_speed(strNetName);
		std::string strDuplex = get_duplex(strNetName);

		if (strSpeed.empty())
		{
			dlog_error("获取速率失败");
			return Network::UNKON_TYPE;
		}

		if (strDuplex.empty())
		{
			dlog_error("获取传输模式失败");
			return Network::UNKON_TYPE;
		}

		dlog_info("获取到的速率:%s,获取到的传输模式%s", strSpeed.c_str(), strDuplex.c_str());

		/* 10M半双工 */
		if (strSpeed == STR_SPEED_10MB && strDuplex == STR_DUPLEX_HALF)
		{
			dlog_info("网卡是10M半双工");
			return Network::SPEED_10M_HALF_DUPLEX;

			/* 10M全双工 */
		}
		else if (strSpeed == STR_SPEED_10MB && strDuplex == STR_DUPLEX_FULL)
		{
			dlog_info("网卡是10M全双工");
			return Network::SPEED_10M_FULL_DUPLEX;

			/* 100M半双工 */
		}
		else if (strSpeed == STR_SPEED_100MB && strDuplex == STR_DUPLEX_HALF)
		{
			dlog_info("网卡是100M半双工");
			return Network::SPEED_100M_HALF_DUPLEX;

			/* 100M全双工 */
		}
		else if (strSpeed == STR_SPEED_100MB && strDuplex == STR_DUPLEX_FULL)
		{
			dlog_info("网卡是100M全双工");
			return Network::SPEED_100M_FULL_DUPLEX;

			/* 100M半双工 */
		}
		else if (strSpeed == STR_SPEED_1000MB && strDuplex == STR_DUPLEX_HALF)
		{
			dlog_info("网卡是100M半双工");
			return Network::SPEED_1000M_HALF_DUPLEX;

			/* 1000M全双工 */
		}
		else if (strSpeed == STR_SPEED_1000MB && strDuplex == STR_DUPLEX_FULL)
		{
			dlog_info("网卡是1000M全双工");
			return Network::SPEED_1000M_FULL_DUPLEX;

			/* 获取失败 */
		}
		else
		{
			dlog_error("找不到对应的类型");
			return Network::UNKON_TYPE;
		}
	}
}

/* 设置网卡类型 */
int CNetworkManage::set_network_type(FILE *pFile, Network::NetTypeMode_E enType, std::string strNetName, bool isSet)
{
	dlog_debug("设置网卡类型");

	if (pFile == nullptr)
	{
		dlog_error("文件指针为空，无法写入网卡类型信息。");
		return -1;
	}

	/* 根据不同的网卡类型设置 */
	switch (enType)
	{
	case Network::AUTO:
		ETH_AUTO_NEGOTIATION_ON_CMD(strNetName.c_str());
		fprintf(pFile, "    %s\n", ETH_AUTO_NEGOTIATION_ON(strNetName).c_str());
		break;
	case Network::SPEED_10M_HALF_DUPLEX:
		ETH_AUTO_NEGOTIATION_OFF_CMD(strNetName.c_str());
		ETH_10M_HALF_DUPLEX_CMD(strNetName.c_str());
		fprintf(pFile, "    %s\n", ETH_10M_HALF_DUPLEX(strNetName).c_str());
		break;
	case Network::SPEED_10M_FULL_DUPLEX:
		ETH_AUTO_NEGOTIATION_OFF_CMD(strNetName.c_str());
		ETH_10M_FULL_DUPLEX_CMD(strNetName.c_str());
		fprintf(pFile, "    %s\n", ETH_10M_FULL_DUPLEX(strNetName).c_str());
		break;
	case Network::SPEED_100M_HALF_DUPLEX:
		ETH_AUTO_NEGOTIATION_OFF_CMD(strNetName.c_str());
		ETH_100M_HALF_DUPLEX_CMD(strNetName.c_str());
		fprintf(pFile, "    %s\n", ETH_100M_HALF_DUPLEX(strNetName).c_str());
		break;
	case Network::SPEED_100M_FULL_DUPLEX:
		ETH_AUTO_NEGOTIATION_OFF_CMD(strNetName.c_str());
		ETH_100M_FULL_DUPLEX_CMD(strNetName.c_str());
		fprintf(pFile, "    %s\n", ETH_100M_FULL_DUPLEX(strNetName).c_str());
		break;
	case Network::SPEED_1000M_HALF_DUPLEX:
		ETH_AUTO_NEGOTIATION_OFF_CMD(strNetName.c_str());
		ETH_1000M_HALF_DUPLEX_CMD(strNetName.c_str());
		fprintf(pFile, "    %s\n", ETH_1000M_HALF_DUPLEX(strNetName).c_str());
		break;
	case Network::SPEED_1000M_FULL_DUPLEX:
		ETH_AUTO_NEGOTIATION_OFF_CMD(strNetName.c_str());
		ETH_1000M_FULL_DUPLEX_CMD(strNetName.c_str());
		fprintf(pFile, "    %s\n", ETH_1000M_FULL_DUPLEX(strNetName).c_str());
		break;
	default:
		dlog_error("未知的网络类型。");
		return -1;
	}
	if (isSet)
	{
		/* 设置后需要重新启用网卡 */
		SET_INTERFACE_DOWN(strNetName.c_str());
		SET_INTERFACE_UP(strNetName.c_str());
	}

	return 0;
}

int CNetworkManage::get_ip_and_dns(Network::Info_S &stNetInfo)
{
	/* 获取IPV4相关信息 */
	stNetInfo.stIp.ipv4Ip = get_dev_ip(stNetInfo.stIp.netName);

	stNetInfo.stIp.ipv4Mask = get_dev_mask(stNetInfo.stIp.netName);
	stNetInfo.stIp.ipv4Gateway = get_dev_gateway(stNetInfo.stIp.netName);

	/* 获取IPV6相关信息 */
	stNetInfo.stIp.ipv6Ip = get_ipv6Address(stNetInfo.stIp.netName);
	stNetInfo.stIp.ipv6Gateway = get_ipv6Gateway(stNetInfo.stIp.netName);

	/* 获取dhcp状态 */
	if (get_dhcpStatus(stNetInfo.stIp.netName.c_str()) == DHCP_ENABLED)
	{
		dlog_info("网卡[%s]已开启dhcp", stNetInfo.stIp.netName.c_str());
		stNetInfo.stIp.bEnableDhcp = true;
	}
	else if (get_dhcpStatus(stNetInfo.stIp.netName.c_str()) == DHCP_DISABLED)
	{
		dlog_info("网卡[%s]已关闭dhcp", stNetInfo.stIp.netName.c_str());
		stNetInfo.stIp.bEnableDhcp = false;
	}
	else
	{
		if (stNetInfo.stIp.ipv4Ip.empty())
		{
			stNetInfo.stIp.bEnableDhcp = false;
		}
	}

	/* 获取MAC地址 */
	stNetInfo.stIp.physicalAddress = get_macAddress(stNetInfo.stIp.netName);

	/* 获取mtu */
	stNetInfo.stIp.nMtu = get_mtu(stNetInfo.stIp.netName);

	dlog_info("获取网卡[%s]的IPV4信息--ip：%s, 子网掩码：%s, 网关：%s", stNetInfo.stIp.netName.c_str(), stNetInfo.stIp.ipv4Ip.c_str(), stNetInfo.stIp.ipv4Mask.c_str(), stNetInfo.stIp.ipv4Gateway.c_str());
	dlog_info("获取网卡[%s]的IPV6信息--ip：%s, 网关：%s", stNetInfo.stIp.netName.c_str(), stNetInfo.stIp.ipv6Ip.c_str(), stNetInfo.stIp.ipv6Gateway.c_str());
	dlog_info("获取网卡[%s]的MTU大小：%d, MAC地址:%s", stNetInfo.stIp.netName.c_str(), stNetInfo.stIp.nMtu, stNetInfo.stIp.physicalAddress.c_str());

	/* 获取DNS */
	get_networkDns(stNetInfo.stDns);

	dlog_info("获取网卡的DNS信息 首选DNS：%s, 次选DNS:%s", stNetInfo.stDns.main.c_str(), stNetInfo.stDns.standby.c_str());

	return 0;
}

/* 设置IP和dns相关信息 */
int CNetworkManage::set_ip_and_dns(Network::Info_S &stNetInfo, bool bIsImmediate)
{
	int nRet = -1;

	/* 自动获取DNS */
	if (stNetInfo.stDns.bEnableAutoDns && bIsImmediate)
	{
		dlog_debug("开启自动获取DNS");

		/* 解锁文件 */
		if (unlock_file(DNS_CONFIG) != 0) 
		{
			dlog_info("无法解锁文件 %s 删除文件", DNS_CONFIG);
		}
		
		/* 删除自定义DNS设置 */
		if(std::remove(DNS_CONFIG) != 0)
		{
			dlog_error("删除DNS配置文件失败:%s",DNS_CONFIG);
		}
		dlog_info("删除DNS配置文件成功:%s",DNS_CONFIG);
	}
	/* 手动获取DNS */
	else if (!stNetInfo.stDns.bEnableAutoDns)
	{
		dlog_debug("不开启自动获取DNS");
		/*设置DNS*/
		nRet = set_networkDns(stNetInfo.stDns);
		if (nRet < 0)
		{
			dlog_debug("手动设置DNS信息失败");
			return nRet;
		}
	}

	/*设置网卡ipv4*/
	nRet = set_ethInterfaces(stNetInfo.stIp, bIsImmediate);
	if (nRet < 0)
	{
		dlog_debug("设置网卡ipv4相关信息失败");
		return nRet;
	}

	if (stNetInfo.stDns.bEnableAutoDns && bIsImmediate)
	{
		get_networkDns(stNetInfo.stDns);
	}

	return nRet;
}

/* 手动设置DNS */
int CNetworkManage::set_networkDns(Network::Dns_S &stDns)
{
	int nRet;

	/* 解锁文件 */
	if (unlock_file(DNS_CONFIG) != 0)
	{
		dlog_info("无法解锁文件 %s 尝试删除文件", DNS_CONFIG);
		/* 删除自定义DNS设置 */
		if (std::remove(DNS_CONFIG) != 0)
		{
			dlog_error("删除DNS配置文件失败或文件不存在:%s", DNS_CONFIG);
		}
	}

	/* 打开文件，"w" 模式表示清空并写入 */
	FILE *pFile = fopen(DNS_CONFIG, "w");

	/* 检查文件是否成功打开 */
	if (!pFile)
	{
		dlog_error("无法打开或创建 DNS 配置文件: %s", DNS_CONFIG);
		return -1;
	}

	/* 手动DNS配置标志 */
	nRet = fprintf(pFile, "#ManualConfig\n");
	if (nRet < 0)
	{
		dlog_error("写入文件失败");
		fclose(pFile);
		return -1;
	}

	/* 写入主要 DNS 服务器 */
	if (stDns.main.empty())
	{
		stDns.main = DEFAULT_MAIN_DNS;
	}
	nRet = fprintf(pFile, "nameserver %s\n", stDns.main.c_str());
	if (nRet < 0)
	{
		dlog_error("写入主要DNS服务器失败");
		fclose(pFile);
		return -1;
	}

	/* 写入次要 DNS 服务器 */
	if (stDns.standby.empty())
	{
		stDns.standby = DEFAULT_STANDBY_DNS;
	}
	nRet = fprintf(pFile, "nameserver %s\n", stDns.standby.c_str());
	if (nRet < 0)
	{
		dlog_error("写入次要DNS服务器失败");
		fclose(pFile);
		return -1;
	}

	/* 关闭文件 */
	fclose(pFile);

	/* 锁定文件 */
	if (lock_file(DNS_CONFIG) != 0)
	{
		dlog_error("无法锁定文件 %s", DNS_CONFIG);
		return -1;
	}

	dlog_info("主要DNS服务器：%s 次要DNS服务器：%s", stDns.main.c_str(), stDns.standby.c_str());
	dlog_info("成功更新 DNS 配置文件为 [%s]", DNS_CONFIG);

	return 0;
}

int CNetworkManage::get_networkDns(Network::Dns_S &stDns)
{
	const std::string strDnsConfigFile = DNS_CONFIG;

	std::ifstream file(strDnsConfigFile);

	/* 检查文件是否成功打开 */
	if (!file.is_open())
	{
		dlog_error("无法打开DNS配置文件: %s", strDnsConfigFile.c_str());
		/* 文件不存在或无法打开，设置enable为false */
		stDns.bEnableAutoDns = false;
	}

	std::string strFirstLine;
	bool isFirstLine = true;
	int nDnsIndex = 0;

	/* 读取文件并提取DNS服务器信息 */
	std::string strLine;
	while (std::getline(file, strLine))
	{
		/* 是否自动配置DNS */
		if (isFirstLine)
		{
			/*检查第一行是否包含 "#ManualConfig" */
			if (strLine.find("#ManualConfig") != std::string::npos)
			{
				stDns.bEnableAutoDns = false;
			}
			else
			{
				stDns.bEnableAutoDns = true;
			}
			isFirstLine = false;
		}

		std::istringstream iss(strLine);
		std::string keyword;
		if (std::getline(iss, keyword, ' ') && keyword == "nameserver")
		{
			std::string dnsAddress;
			if (std::getline(iss, dnsAddress))
			{
				dnsAddress = trim(dnsAddress);

				size_t commentPos = dnsAddress.find('#');
				if (commentPos != std::string::npos)
				{
					dnsAddress = dnsAddress.substr(0, commentPos);
				}

				dnsAddress = trim(dnsAddress);

				size_t spacePos = dnsAddress.find_first_of(" \t");
				if (spacePos != std::string::npos)
				{
					dnsAddress = dnsAddress.substr(0, spacePos);
				}

				dnsAddress = trim(dnsAddress);
				if (!dnsAddress.empty())
				{
					if (nDnsIndex == 0)
					{
						stDns.main = dnsAddress;
					}
					else if (nDnsIndex == 1)
					{
						stDns.standby = dnsAddress;
					}
					nDnsIndex++;
				}
			}
		}
	}

	file.close();

	return 0;
}

/* 设置网卡的信息 */
#if 1
int CNetworkManage::set_ethInterfaces(Network::Ip_S stIp, bool bIsImmediate)
{
    int nRet = -1;
	char achCommand[MAX_COMMAND_LENGTH] = {0};
    FILE *pFile = NULL;

	if (stIp.netName.empty())
	{
		stIp.netName = ETH0_INTERFACE;
	}

    /* 打开启动脚本临时文件 */
    pFile = fopen(NETWORK_INIT_SCRIPT_TMP, "w+");
    if (NULL == pFile)
	{
        dlog_error("打开启动脚本临时文件失败");
        return nRet;
    }

    /* 写入启动脚本基础结构 */
    fprintf(pFile, "#!/bin/sh\n\n");
    fprintf(pFile, "case \"$1\" in\n");
    fprintf(pFile, "  start)\n");

    /* === 核心配置区域开始 === */
    /* 1. 设置MAC地址（在网卡启用前） */
    std::string strMacFileName = MAC_FILE_PATH;
    std::ifstream fs(strMacFileName);
    if (fs.is_open())
    {
        std::string strMac((std::istreambuf_iterator<char>(fs)), {});
        fprintf(pFile, "    %s\n", strMac.c_str());
        fs.close();
    }
    else
    {
        fprintf(pFile, "    ifconfig %s hw ether %s\n", stIp.netName.c_str(), stIp.physicalAddress.c_str());
    }

    /* 2. 设置MTU（在网卡启用前） */
    int nEffectiveMtu = (set_mtu(stIp.netName, stIp.nMtu) == 0) ? stIp.nMtu : DEFAULT_MTU;
    fprintf(pFile, "    ifconfig %s mtu %d\n", 
            stIp.netName.c_str(), nEffectiveMtu);

	/* 3. 设置本地回环 */
    fprintf(pFile, "    ifconfig lo 127.0.0.1\n");

    /* 3. 静态IP配置 */
    if (!stIp.bEnableDhcp)
	{
		/* 判断ip是否被使用 */
		nRet = check_ip_usage(stIp.ipv4Ip, stIp.netName);
		if (nRet < 0)
		{
			/* IP冲突异常日志 */
			Log::Info_S stLogInfo;
			stLogInfo.nType = Log::EXCEPTION;
			stLogInfo.nAction = Log::IP_CONFLICT;
			LogHandler::instance()->write(stLogInfo);
			dlog_error("在网卡 [%s]设置网络IP失败，IP:[%s]已被使用", stIp.netName.c_str(), stIp.ipv4Ip.c_str());
			return nRet;
		}

		SET_DHCP_STATUS_CLOSE(achCommand, stIp.netName.c_str());
		fprintf(pFile, "    %s", achCommand);
        fprintf(pFile, "    ifconfig %s %s netmask %s\n", 
                stIp.netName.c_str(), stIp.ipv4Ip.c_str(), stIp.ipv4Mask.c_str());

		fprintf(pFile, "    if ! ip route show | grep -q \"default via %s dev %s\"; then\n",
        stIp.ipv4Gateway.c_str(), stIp.netName.c_str());
		fprintf(pFile, "      route add default gw %s %s\n",
				stIp.ipv4Gateway.c_str(), stIp.netName.c_str());
		fprintf(pFile, "    else\n");
		fprintf(pFile, "      echo \"IPv4默认路由已存在，无需添加\"\n");
		fprintf(pFile, "    fi\n");
		/* 5.启用网卡 */
    	fprintf(pFile, "    ifconfig %s up\n", stIp.netName.c_str());
		/* 6. 设置默认Ipv6网关 */
		fprintf(pFile, "    if ! ip -6 route show | grep -q \"default via fe80::1 dev %s\"; then\n",stIp.netName.c_str());
		fprintf(pFile, "     ip -6 route add default via fe80::1 dev %s\n", stIp.netName.c_str());
		fprintf(pFile, "    else\n");
		fprintf(pFile, "      echo \"IPv6默认路由已存在，无需添加\"\n");
		fprintf(pFile, "    fi\n");
    } 
    /* 4. 动态IP配置 */
    else
	{
		SET_DHCP_STATUS_OPEN(achCommand, stIp.netName.c_str());
		fprintf(pFile, "    %s", achCommand);
        fprintf(pFile, "    udhcpc -b -i %s\n", stIp.netName.c_str());
    }

    /* 7. 特殊网络类型配置 */
    // set_network_type(pFile, stIp.enType, stIp.netName.c_str(), true);
    /* === 核心配置区域结束 === */

	/* 8. 启动 telnet 服务 */
#if CAP_NETWORK_TELNET_SERVICE
	/* 使用能力宏控制 telnetd 启动，避免直接绑定具体型号，便于后续设备画像统一扩展。 */
    fprintf(pFile, "    telnetd &\n");
#endif

	/* 9. 启动 ftp 服务 */
#if CAP_NETWORK_FTP_SERVICE
	/* 使用能力宏控制 uftpd 启动，保持网络服务开关与设备画像配置一致。 */
    fprintf(pFile, "    uftpd -o \"ftp=21 tftp=69 writable\" /\n");
#endif
	/* 10.判断是否启动ssh服务 */

    /* 脚本结束部分 */
    fprintf(pFile, "    ;;\n");
    fprintf(pFile, "  stop)\n");
    fprintf(pFile, "    ifconfig %s down\n", stIp.netName.c_str());
    fprintf(pFile, "    ;;\n");
    fprintf(pFile, "  *)\n");
    fprintf(pFile, "    $0 start\n");
    fprintf(pFile, "    ;;\n");
    fprintf(pFile, "esac\n\n");
    fprintf(pFile, "exit 0\n");

    if (pFile)
	{
        fclose(pFile);
    }

    /* 设置文件权限 */
    chmod(NETWORK_INIT_SCRIPT_TMP, 0755);

    /* 替换原启动脚本 */
    if (std::remove(NETWORK_INIT_SCRIPT) != 0 && errno != ENOENT)
	{
        dlog_error("删除原启动脚本失败 [%s]", NETWORK_INIT_SCRIPT);
        return -1;
    }

    if (std::rename(NETWORK_INIT_SCRIPT_TMP, NETWORK_INIT_SCRIPT) != 0)
	{
        dlog_error("重命名启动脚本失败 [%s] -> [%s]", 
                  NETWORK_INIT_SCRIPT_TMP, NETWORK_INIT_SCRIPT);
        return -1;
    }
	system("sync");

	if (bIsImmediate)
	{
		dlog_debug("网络配置立即生效");
		system(NETWORK_INIT_SCRIPT " start");
	}
	else
	{
		dlog_debug("网络配置重启后生效");
	}

    return 0;
}
#else
int CNetworkManage::set_ethInterfaces(Network::Ip_S stIp)
{
	int nRet = -1;
	char achCommand[MAX_COMMAND_LENGTH] = {0};
	FILE *pFile = NULL;
	/* 打开临时文件 */
	pFile = fopen(NETWORK_INTERFACES_TMP, "w+");
	if (NULL == pFile)
	{
		dlog_error("打开网络配置信息文件失败");
		return nRet;
	}
	/* 写入本地网卡配置信息 */
	fprintf(pFile, NETWORK_CONFIG_LO);

	/* 静态设置网卡 */
	if (!stIp.bEnableDhcp)
	{
		/* 配置网络 */
		nRet = configure_network(stIp.netName, stIp.ipv4Ip, stIp.ipv4Mask, stIp.ipv4Gateway);
		if (nRet < 0)
		{
			dlog_error("设置静态网络信息失败");
			return nRet;
		}

		SET_DHCP_STATUS_CLOSE(achCommand, stIp.netName.c_str());
		fprintf(pFile, "%s", achCommand);
		fprintf(pFile, "auto %s\niface %s inet static\n",
				stIp.netName.c_str(), stIp.netName.c_str());
		dlog_debug("拿到的ip:%s", stIp.ipv4Ip.c_str());
		/*ip*/
		fprintf(pFile, "address %s\n", stIp.ipv4Ip.c_str());
		/*netmask*/
		fprintf(pFile, "netmask %s\n", stIp.ipv4Mask.c_str());
		/*gateway*/
		fprintf(pFile, "gateway %s\n", stIp.ipv4Gateway.c_str());
	}
	/* 动态设置网卡 */
	else
	{
		SET_DHCP_STATUS_OPEN(achCommand, stIp.netName.c_str());
		fprintf(pFile, "%s", achCommand);
		/*设置回环*/
		fprintf(pFile, "auto %s\niface %s inet dhcp\n", stIp.netName.c_str(), stIp.netName.c_str());
		/* 开启dhcp */
		if (check_carrier_link(stIp.netName) == 0)
		{
			nRet = start_dhcp(stIp.netName.c_str());
			if (nRet < 0)
			{
				dlog_error("DHCP开启失败");
				return -1;
			}
		}
	}

	/* 设置物理地址 */
	std::string strMacFileName;
	strMacFileName = MAC_FILE_PATH;
	std::fstream fs{strMacFileName, fs.in};
	if (fs.is_open())
	{
		std::string strMac((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
		/*mac*/
		fprintf(pFile, "%s\n", strMac.c_str());
		fs.close();
	}
	else
	{
		/*mac*/
		fprintf(pFile, "pre-up ifconfig %s hw ether %s\n", stIp.netName.c_str(), stIp.physicalAddress.c_str());
	}

	/* 设置mtu大小 */
	if (set_mtu(stIp.netName, stIp.nMtu) == 0)
	{
		fprintf(pFile, "pre-up /sbin/ifconfig %s mtu %d\n", stIp.netName.c_str(), stIp.nMtu);
	}
	else
	{
		fprintf(pFile, "pre-up /sbin/ifconfig %s mtu %d\n", stIp.netName.c_str(), DEFAULT_MTU);
	}

	/* 设置eth0的网卡类型 */
	set_network_type(pFile, stIp.enType, stIp.netName.c_str(), true);

	if (pFile)
	{
		fclose(pFile);
		pFile = NULL;
	}

	/* 删除网络文件 */
	if (std::remove(NETWORK_INTERFACES) != 0)
	{
		if (errno != ENOENT)
		{
			dlog_error("无法删除文件 [%s]", NETWORK_INTERFACES);
			return -1;
		}
	}

	/* 重命名临时文件到网络文件 */
	if (std::rename(NETWORK_INTERFACES_TMP, NETWORK_INTERFACES) != 0)
	{
		dlog_error("无法重命名文件从 [%s] 到 [%s]", NETWORK_INTERFACES_TMP, NETWORK_INTERFACES);
		return -1;
	}

	return 0;
}
#endif

int CNetworkManage::get_dhcpStatus(const std::string &strNetName)
{
	char achReadBuf[MAX_COMMAND_LENGTH] = {0};
	int nRet = -1;
	std::ifstream file;

	/* 打开文件 */
	file.open(NETWORK_INIT_SCRIPT);
	if (!file.is_open())
	{
		dlog_error("无法打开网络文件 [%s]", NETWORK_INIT_SCRIPT);
		return -1;
	}

	/* 读取文件直到末尾 */
	while (file.getline(achReadBuf, sizeof(achReadBuf)))
	{
		std::string line(achReadBuf);
		/*  查找strNetName以及dhcp-open或dhcp-close */
		if (line.find(strNetName + DHCP_STATUS_OPEN) != std::string::npos || line.find("iface " + strNetName + " inet dhcp") != std::string::npos)
		{
			/* DHCP打开 */
			nRet = DHCP_ENABLED;
			break;
		}
		else if (line.find(strNetName + DHCP_STATUS_CLOSE) != std::string::npos || line.find("iface " + strNetName + " inet static") != std::string::npos)
		{
			/* DHCP关闭 */
			nRet = DHCP_DISABLED;
			break;
		}
		else
		{ 
			/* 默认关闭DHCP */
			nRet = DHCP_DISABLED;
		}
	}

	file.close();
	return nRet;
}

/* 获取设备ip */
std::string CNetworkManage::get_dev_ip(std::string strNetName)
{
	int nSockfd;
	struct ifreq ifr;

	// 创建套接字
	nSockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (nSockfd < 0)
	{
		dlog_error("创建套接字失败");
		return "";
	}

	// 设置接口名称
	std::strncpy(ifr.ifr_name, strNetName.c_str(), IFNAMSIZ);

	// 获取接口地址
	if (ioctl(nSockfd, SIOCGIFADDR, &ifr) < 0)
	{
		std::cerr << "Failed to get interface address" << std::endl;
		close(nSockfd);
		return "";
	}

	/* 获取IP地址 */
	struct sockaddr_in *pAddr = reinterpret_cast<struct sockaddr_in *>(&ifr.ifr_addr);
	char chIp[INET_ADDRSTRLEN];
	std::strcpy(chIp, inet_ntoa(pAddr->sin_addr));

	close(nSockfd);

	return chIp;
}

std::string CNetworkManage::getIPByIfconfig(const std::string& iface) 
{
    // 执行 ifconfig 命令
    std::string cmd = "ifconfig " + iface + " 2>/dev/null";
    
    std::array<char, 4096> buffer;
    std::string result;
    
    std::unique_ptr<FILE, decltype(&pclose)> pipe(
        popen(cmd.c_str(), "r"), pclose);
    
    if (!pipe) 
	{
		return "";
	}
    
    while (fgets(buffer.data(), buffer.size(), pipe.get())) 
	{
        result += buffer.data();
    }
    
    // 正则匹配 inet 后的 IP 地址
    // 匹配 inet 192.168.1.100 或 inet addr:192.168.1.100 等格式
    std::regex ip_regex(R"(inet\s+(?:addr:)?(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}))");
    std::smatch match;
    
    if (std::regex_search(result, match, ip_regex)) 
	{
        return match[1].str();  // 返回第一个捕获组（IP）
    }
    
    return "";
}

std::string CNetworkManage::getLocalIP(const std::string& interface)
{
    struct ifaddrs *ifaddr = nullptr;
    struct ifaddrs *ifa = nullptr;
    std::string ip;

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return "";
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr) continue;

        // 过滤出IPv4
        if (ifa->ifa_addr->sa_family != AF_INET)
		{
			continue;
		}

        // 网卡必须是UP状态
        if (!(ifa->ifa_flags & IFF_UP))
		{
			continue;
		}

        // 过滤回环
        if (ifa->ifa_flags & IFF_LOOPBACK)
		{
			continue;
		}

        std::string name = ifa->ifa_name;

        // 如果指定网卡
        if (!interface.empty())
        {
            if (name != interface)
			{
				continue;
			}
                
        }
        else
        {
            // 自动模式：过滤虚拟网卡
            if (name.find("docker") == 0 ||
                name.find("br") == 0 ||
                name.find("virbr") == 0 ||
                name.find("veth") == 0 ||
                name.find("tun") == 0)
            {
                continue;
            }
        }

        // 取IP
        char addrStr[INET_ADDRSTRLEN] = {0};
        void* addr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;

        if (inet_ntop(AF_INET, addr, addrStr, sizeof(addrStr)))
        {
            ip = addrStr;
            break;
        }
    }

    freeifaddrs(ifaddr);
    return ip;
}

/*获取设备子掩码*/
std::string CNetworkManage::get_dev_mask(std::string strNetName)
{
	struct ifreq ifr;
	int nSockfd = socket(AF_INET, SOCK_DGRAM, 0);

	if (nSockfd < 0)
	{
		dlog_error("socket");
		return "";
	}

	strncpy(ifr.ifr_name, strNetName.c_str(), IFNAMSIZ);

	if (ioctl(nSockfd, SIOCGIFNETMASK, &ifr) == -1)
	{
		dlog_error("ioctl");
		close(nSockfd);
		return "";
	}

	struct sockaddr_in *pSin = (struct sockaddr_in *)&ifr.ifr_addr;
	char mask[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &pSin->sin_addr, mask, INET_ADDRSTRLEN);

	close(nSockfd);
	return std::string(mask);
}

/*获取设备网关*/
std::string CNetworkManage::get_dev_gateway(std::string strNetName)
{
	std::string strCommand = GET_DEFAULT_GATEWAY_COMMAND;
	FILE *pipe = popen(strCommand.c_str(), "r");
	if (pipe == nullptr)
	{
		dlog_error("打开管道错误");
		return "";
	}
	char buffer[128];
	std::string strGateway;
	while (!feof(pipe))
	{
		if (fgets(buffer, 128, pipe) != nullptr)
			strGateway += buffer;
	}
	pclose(pipe);

	strGateway.erase(strGateway.find_last_not_of("\n") + 1);

	size_t pos = strGateway.find("addr:");
	if (pos != std::string::npos)
	{
		strGateway.erase(pos, 5);
	}

	return strGateway;
}

/* 获取mtu大小 */
int CNetworkManage::get_mtu(const std::string &strInterfaceName)
{
	/* mtu节点 */
	std::string strFilePath = NETWORK_DEVICES_PATH + strInterfaceName + MTU_PATH;
	std::ifstream file(strFilePath);

	if (!file.is_open())
	{
		dlog_error("无法打开文件: %s", strFilePath.c_str());
		return -1;
	}

	int nMtuValue;
	file >> nMtuValue;

	if (file.fail())
	{
		dlog_error("读取 MTU 值失败。") return -1;
	}

	file.close();
	return nMtuValue;
}

int CNetworkManage::set_mtu(const std::string &strInterfaceName, int nMtu)
{
	int nSockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (nSockfd < 0)
	{
		dlog_error("打开网络套接字失败");
		return -1;
	}

	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, strInterfaceName.c_str(), IFNAMSIZ - 1);

	ifr.ifr_mtu = nMtu;
	if (ioctl(nSockfd, SIOCSIFMTU, &ifr) < 0)
	{
		dlog_error("设置mtu失败");
		close(nSockfd);
		return -1;
	}

	close(nSockfd);
	return 0;
}

std::string CNetworkManage::get_ipv6Address(const std::string &strNetName)
{
	struct ifaddrs *ifaddr;
	/* IPv6 地址字符串缓冲区 */
	char addrStr[INET6_ADDRSTRLEN];

	/* 获取网络接口地址信息 */
	if (getifaddrs(&ifaddr) == -1)
	{
		dlog_error("getifaddrs failed");
		return "";
	}

	/* 遍历网络接口 */
	for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == nullptr || (ifa->ifa_flags & IFF_UP) == 0)
		{
			continue;
		}

		/* 检查是否为IPv6 */
		if (ifa->ifa_addr->sa_family == AF_INET6 && strNetName == ifa->ifa_name)
		{
			/* 转换IPv6地址为字符串 */
			if (inet_ntop(AF_INET6, &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr, addrStr, sizeof(addrStr)) != nullptr)
			{
				freeifaddrs(ifaddr);
				return std::string(addrStr);
			}
		}
	}

	freeifaddrs(ifaddr);
	return "";
}

std::string CNetworkManage::get_ipv6Gateway(const std::string &strNetName)
{
	FILE *fp;
	char path[1035];
	std::string gateway;

	/* 使用 `ip -6 route` 命令获取路由信息 */
	std::string strcommand = IP6_ROUTE_SHOW_DEV + strNetName;
	fp = popen(strcommand.c_str(), "r");
	if (fp == nullptr)
	{
		dlog_error("popen failed");
		return "";
	}

	while (fgets(path, sizeof(path), fp) != nullptr)
	{
		/* 查找网关信息 */
		if (strstr(path, "via") != nullptr)
		{
			char *token = strtok(path, " ");
			while (token != nullptr)
			{
				if (strcmp(token, "via") == 0)
				{
					token = strtok(nullptr, " ");
					if (token != nullptr)
					{
						gateway = token;
						break;
					}
				}
				token = strtok(nullptr, " ");
			}
			break;
		}
	}

	pclose(fp);
	return gateway;
}

std::string CNetworkManage::get_macAddress(const std::string &interfaceName)
{
	struct ifreq ifreq;
	int socketFD;

	/* 创建套接字 */
	if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		dlog_error("socket");
		return "";
	}

	/* 清空 ifreq 并设置网卡名称 */
	std::memset(&ifreq, 0, sizeof(ifreq));
	std::strncpy(ifreq.ifr_name, interfaceName.c_str(), IFNAMSIZ - 1);

	/* 获取 MAC 地址 */
	if (ioctl(socketFD, SIOCGIFHWADDR, &ifreq) < 0)
	{
		dlog_error("ioctl");
		close(socketFD);
		return "";
	}

	close(socketFD);

	/* 将 MAC 地址格式化为字符串 */
	char macBuffer[18];
	std::snprintf(macBuffer, sizeof(macBuffer), "%02X:%02X:%02X:%02X:%02X:%02X",
				  (unsigned char)ifreq.ifr_hwaddr.sa_data[0],
				  (unsigned char)ifreq.ifr_hwaddr.sa_data[1],
				  (unsigned char)ifreq.ifr_hwaddr.sa_data[2],
				  (unsigned char)ifreq.ifr_hwaddr.sa_data[3],
				  (unsigned char)ifreq.ifr_hwaddr.sa_data[4],
				  (unsigned char)ifreq.ifr_hwaddr.sa_data[5]);

	return std::string(macBuffer);
}

/**
 * @brief       : Mac地址合规性检测
 * @author      : zhouzirui
 * @return       {bool} ERROR, false; SUCCESS, true
 */
bool CNetworkManage::check_mac_valid()
{
    /* ifreq结构体常用来配置和获取ip地址 */
    struct ifreq stIfreq;
    memset(&stIfreq, 0, sizeof(stIfreq));
    int nSocket = 0;

    if ((nSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        dlog_error("socket error!");
        return -1;
    }
    strcpy(stIfreq.ifr_name, ETH0_INTERFACE);

    if (ioctl(nSocket, SIOCGIFHWADDR, &stIfreq) < 0)
    {
        dlog_error("ioctl error!");
        return -1;
    }
    close(nSocket);

    if (MAC_0 != stIfreq.ifr_hwaddr.sa_data[0] ||
        MAC_1 != stIfreq.ifr_hwaddr.sa_data[1] ||
        MAC_2 != stIfreq.ifr_hwaddr.sa_data[2])
    {
        return -1;
    }

    return 0;
}

int CNetworkManage::get_network_port(Network::PortConfig_S &stPortConfig)
{
	if (Convert::read_file(PORT_CONFIG_FILE, stPortConfig))
	{
		return ERR;
	}
	return OK;
}

int CNetworkManage::set_network_port(Network::PortConfig_S stPortConfig)
{
	if (Convert::write_file(PORT_CONFIG_FILE, stPortConfig))
	{
		return ERR;
	}
	return OK;
}

/* 判断网卡是否开启自动协商模式 */
bool CNetworkManage::autoneg_enabled(const std::string &strInterfaceName)
{
	/* 使用宏构建命令字符串 */
	std::string strCommand = CHECK_AUTONEGOTIATION(strInterfaceName.c_str());

	/* 执行命令并捕获输出 */
	FILE *pFp = popen(strCommand.c_str(), "r");
	if (pFp == nullptr)
	{
		dlog_error("执行命令失败");
		return false;
	}

	/* 读取命令输出 */
	char achPath[MAX_COMMAND_LENGTH];
	bool bEnAutoNegOn = false;
	while (fgets(achPath, sizeof(achPath), pFp) != nullptr)
	{
		/*  输出类似 "Auto-negotiation: on" 或 "Auto-negotiation: off" */
		if (std::string(achPath).find("Auto-negotiation: on") != std::string::npos)
		{
			bEnAutoNegOn = true;
			break;
		}
	}

	pclose(pFp);

	return bEnAutoNegOn;
}

/* 获取网卡传输速率 */
std::string CNetworkManage::get_speed(const std::string &strInterfaceName)
{
	/* Speed节点 */
	std::string strFilePath = NETWORK_DEVICES_PATH + strInterfaceName + SPEED_PATH;
	std::ifstream file(strFilePath);

	if (!file.is_open())
	{
		dlog_error("无法打开文件: %s", strFilePath.c_str());
		return "";
	}

	std::string strSpeedValue;
	file >> strSpeedValue;

	if (file.fail())
	{
		dlog_error("读取网卡速率失败。");
		return "";
	}

	file.close();
	return strSpeedValue;
}

/* 获取网卡传输模式 */
std::string CNetworkManage::get_duplex(const std::string &strInterfaceName)
{
	/* Mode节点 */
	std::string strFilePath = NETWORK_DEVICES_PATH + strInterfaceName + DUPLEX_PATH;
	std::ifstream file(strFilePath);

	if (!file.is_open())
	{
		dlog_error("无法打开文件: %s", strFilePath.c_str());
		return "";
	}

	std::string strDuplex;
	file >> strDuplex;

	if (file.fail())
	{
		dlog_error("读取网卡模式失败。");
		return "";
	}

	file.close();
	return strDuplex;
}

int CNetworkManage::unlock_file(const char *pFilePath)
{
	/* 执行解锁操作 */
	if (check_cmd_exists("chattr")) {
        char achCommand[MAX_COMMAND_LENGTH];
        snprintf(achCommand, sizeof(achCommand), "chattr -i %s", pFilePath);
        return std::system(achCommand);
    } else {
        /*使用 chmod 替代: 设置读写权限（所有用户）*/
        return chmod(pFilePath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    }
}

int CNetworkManage::lock_file(const char *pFilePath)
{
	/* 执行锁定操作 */
	if (check_cmd_exists("chattr")) {
        char achCommand[MAX_COMMAND_LENGTH];
        snprintf(achCommand, sizeof(achCommand), "chattr +i %s", pFilePath);
        return std::system(achCommand);
    } else {
        /*使用 chmod 替代: 设置只读权限（所有用户）*/
        return chmod(pFilePath, S_IRUSR | S_IRGRP | S_IROTH);
    }
}

bool CNetworkManage::check_cmd_exists(const char* pCmd)
{
	std::string checkCmd = std::string("which ") + pCmd + " > /dev/null 2>&1";
    return (system(checkCmd.c_str()) == 0);
}

/* 判断ip是否被使用 */
int CNetworkManage::check_ip_usage(const std::string &strIp, const std::string &strInterfaceName)
{
	std::string stIp;
	std::string strIpListCommand = "ip addr show " + strInterfaceName + " | grep 'inet ' | awk '{print $2}' | cut -d/ -f1";
	std::unique_ptr<FILE, decltype(&pclose)> ipPipe(popen(strIpListCommand.c_str(), "r"), pclose);
	if (!ipPipe)
	{
		dlog_error("获取ip失败");
		return -1;
	}

	char achBuffer[128];
	while (fgets(achBuffer, sizeof(achBuffer), ipPipe.get()) != nullptr)
	{
		std::string ip(achBuffer);
		ip.erase(ip.find_last_not_of(" \n\r\t") + 1); // 去除末尾的换行和空格
		stIp = ip;
	}

	/* 构建 arping 命令 */
	std::string strCommand = "ping -c 1 -W 1 " + stIp;

	/* 打开管道以读取命令输出 */
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(strCommand.c_str(), "r"), pclose);
	if (!pipe)
	{
		dlog_error("无法执行 arping 命令");
		return -1;
	}

	/* 读取命令输出内容 */
	std::memset(achBuffer, 0, sizeof(achBuffer));
	std::string strResult;
	while (fgets(achBuffer, sizeof(achBuffer), pipe.get()) != nullptr)
	{
		strResult += achBuffer;
	}

	/* 检查输出是否包含 "1 received" */
	if (strResult.find("1 received") != std::string::npos)
	{
		dlog_error("IP被使用");
		return IpcRet_E::ERR_IP_COLLIDE;
	}

	return 0;
}

std::string CNetworkManage::get_network_interface()
{
	FILE *fp = nullptr;
	char aBuf[256];
	std::string interface;

	if ((fp = popen("awk 'NR>2{print $1}' /proc/net/dev | cut -d':' -f1", "r")) == nullptr)
	{
		dlog_error("Fail to popen");
		return interface;
	}

	while (fgets(aBuf, sizeof(aBuf), fp) != nullptr)
	{

		int nDataLength = strlen(aBuf) - 1;
		if (nDataLength >= 0 && aBuf[nDataLength] == '\n')
		{
			aBuf[nDataLength] = '\0';
		}

		/* 过滤掉本地回环接口 "lo" */
		if (strcmp(aBuf, "lo") == 0)
		{
			continue;
		}

		/* 过滤usb网卡 */
		if (strcmp(aBuf, "usb0") == 0)
		{
			continue;
		}
		/* === 新增代码：过滤无线网卡 "wlan0" cyc=== */
        if (strcmp(aBuf, "wlan0") == 0)
        {
            continue;
        }

		interface = aBuf;
	}

	pclose(fp);
	return interface;
}

std::string CNetworkManage::trim(const std::string &str)
{
	auto start = str.find_first_not_of(" \t\n\r\f\v");
	auto end = str.find_last_not_of(" \t\n\r\f\v");

	if (start == std::string::npos)
	{
		return ""; // 空字符串
	}

	return str.substr(start, end - start + 1);
}

int CNetworkManage::check_carrier_link(const std::string &strInterfaceName)
{
	int nRet = 0;
	int nSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (nSocket < 0)
	{
		dlog_error("网卡socket打开失败");
		return -1;
	}

	struct ifreq ifr;
	struct ethtool_value eval = {.cmd = ETHTOOL_GLINK};
	strncpy(ifr.ifr_name, strInterfaceName.c_str(), IFNAMSIZ);
	ifr.ifr_data = (char *)&eval;

	if (ioctl(nSocket, SIOCETHTOOL, &ifr) != -1)
	{
		nRet = (eval.data == 0);
	}

	close(nSocket);
	return nRet;
}

std::string CNetworkManage::get_netName_interface()
{
	FILE *pFp = fopen("/proc/net/route", "r");
	if (!pFp)
	{
		perror("fopen");
		return "";
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
	std::string strResult(pIface ? pIface : "");
	free(pIface);
	fclose(pFp);
	return strResult;
}

bool CNetworkManage::need_reboot(Network::Info_S stNetInfo)
{
	// bool bReboot = false;
	Network::Info_S stOldNetInfo;
	get_system_networkInfo(stOldNetInfo);

	/* 静态和动态IP */
	if(stOldNetInfo.stIp.bEnableDhcp != stNetInfo.stIp.bEnableDhcp)
	{
		dlog_debug("=========>网卡[%s]获取IP方式不一致 需要重启",stOldNetInfo.stIp.netName.c_str());
		return true;
	}
	
	/* IP地址 */
	if(stOldNetInfo.stIp.ipv4Ip != stNetInfo.stIp.ipv4Ip)
	{
		dlog_debug("=========>网卡[%s]IP地址不一致 当前[%s] 目标[%s] 需要重启",stOldNetInfo.stIp.netName.c_str(),stOldNetInfo.stIp.ipv4Ip.c_str(),stNetInfo.stIp.ipv4Ip.c_str());
		return true;
	}

	/* ipv4掩码 */
	if(stOldNetInfo.stIp.ipv4Mask != stNetInfo.stIp.ipv4Mask)
	{
		dlog_debug("=========>网卡[%s]子网掩码不一致 当前[%s] 目标[%s] 需要重启",stOldNetInfo.stIp.netName.c_str(),stOldNetInfo.stIp.ipv4Mask.c_str(),stNetInfo.stIp.ipv4Mask.c_str());
		return true;
	}

	/* ipv4网关 */
	if(stOldNetInfo.stIp.ipv4Gateway != stNetInfo.stIp.ipv4Gateway)
	{
		dlog_debug("=========>网卡[%s]网关不一致 当前[%s] 目标[%s] 需要重启",stOldNetInfo.stIp.netName.c_str(),stOldNetInfo.stIp.ipv4Gateway.c_str(),stNetInfo.stIp.ipv4Gateway.c_str());
		return true;
	}

	
	return false;
}