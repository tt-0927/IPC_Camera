/***
 * @FilePath     : pppoe_manage.cpp
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2024-12-02 09:53:20
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-28 09:53:54
 * @Description  : PPPOE拨号上网
 */

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>
#include <thread>
#include "pppoe_manage.h"
#include "dlog.h"

#include "network_manage.h"
#include "network_convert.h"
#include "convert_interface.h"
#include "path_define.h"

extern "C"
{
#include "edukit_network.h"
}

CPppoeManage::CPppoeManage() : m_configFile(PPPOE_CONFIG_FILE)
{
}

IpcRet_E CPppoeManage::init()
{
	Network::Pppoe_S stPppoe;
	int nRet = OK;
	Convert::read_file(m_configFile, stPppoe);
	// config_pppoe(stPppoe);
	/* 开启pppoe拨号 */
	if (stPppoe.bEnablePppoe)
	{
		nRet = start_pppoe(stPppoe);
		if (nRet < OK)
		{
			dlog_error("开启pppoe失败");
			return ERR;
		}
		m_start = true;
	}

	return OK;
}

IpcRet_E CPppoeManage::deinit()
{
	if (!m_start)
	{
		return OK;
	}
	
	if (!stop_pppoe())
	{
		return ERR;
	}
	return OK;
}

/* 配置pppoe */
int CPppoeManage::config_pppoe(Network::Pppoe_S stPppoe)
{
	int nRet;

	/* 更新配置文件 */
	nRet = set_config_file(stPppoe.strUser, stPppoe.strPassword);
	if (nRet < 0)
	{
		dlog_error("设置配置文件失败");
		return -1;
	}

	/* 更新配置文件 */
	nRet = update_secrets_file(stPppoe.strUser, stPppoe.strPassword);

	if (nRet < 0)
	{
		dlog_error("认证文件更新失败");
		return -1;
	}

	/* 关闭dhcp */
	if (stPppoe.bEnablePppoe)
	{
		Network::Info_S stNetInfo;
		CNetworkManage::instance()->get_system_networkInfo(stNetInfo);
		if (stNetInfo.stIp.bEnableDhcp)
		{
			stNetInfo.stIp.bEnableDhcp = false;
		}

		if (stNetInfo.stDns.bEnableAutoDns)
		{
			stNetInfo.stDns.bEnableAutoDns = false;
		}

		CNetworkManage::instance()->set_system_networkInfo(stNetInfo);
	}

	// if (stPppoe.bEnablePppoe)
	//{
	//	nRet = start_pppoe(stPppoe);
	//	if(nRet < 0)
	//	{
	//		dlog_error("开启pppoe失败");
	//		return -1;
	//	}
	//
	//}
	///* 关闭ppoe拨号 */
	// else
	//{
	//	nRet = stop_pppoe();
	// }

	return nRet;
}

int CPppoeManage::get_pppoe_config(Network::Pppoe_S &stPppoe)
{
	Convert::read_file(m_configFile, stPppoe);
	if (stPppoe.bEnablePppoe)
	{
		stPppoe.strIp = get_pppoe_ip();
	}
	else
	{
		stPppoe.strIp = PPPOE_EMPTY_IP;
	}

	return 0;
}

/* 开启pppoe */
int CPppoeManage::start_pppoe(Network::Pppoe_S stPppoe)
{
	int nRet;

	nRet = std::system(PPPOE_CONNECT_CMD);

	if (nRet < 0)
	{
		dlog_error("执行pppoe connect命令失败");
		return -1;
	}

	dlog_debug("成功启用pppoe");

	return 0;
}

/* 关闭pppoe */
int CPppoeManage::stop_pppoe()
{
	int nRet = std::system(PPPOE_STOP_CMD);

	if (nRet < 0)
	{
		dlog_error("ppoe-stop命令关闭失败");
		return -1;
	}

	return 0;
}

std::string CPppoeManage::get_pppoe_ip()
{
	std::string strIP;

	if (!is_interface_exist())
	{
		dlog_error("pppoe网卡不存在");
		return PPPOE_EMPTY_IP;
	}

	std::string strInterface = PPPOE_INTERFACE;
	std::string strDomin = PPPOE_TEST_DOMIN;

	/* 测试网卡能否连接外网 */
	std::string strCmd = "ping -c 2 -I " + strInterface + " -W 2 " + strDomin + " > /dev/null 2>&1";

	if (system(strCmd.c_str()) != 0)
	{
		dlog_error("pppoe网卡测试连接网络失败");
		return PPPOE_EMPTY_IP;
	}
	else
	{
		const char *pInterface = PPPOE_INTERFACE;
		char *achIP = get_interface_ip(pInterface);
		if (achIP == NULL)
		{
			dlog_error("获取pppoe网卡ip失败");
			return PPPOE_EMPTY_IP;
		}
		else
		{
			strIP = achIP;
			dlog_info("获取pppoe网卡ip成功，IP：%s", strIP.c_str());
			return strIP;
		}
	}
}

bool CPppoeManage::is_interface_exist()
{
	std::ifstream netfile("/proc/net/dev");
	if (!netfile)
	{
		return false;
	}

	std::string strLine;
	std::string strInterface = PPPOE_INTERFACE;
	while (std::getline(netfile, strLine))
	{
		if (strLine.find(strInterface + ":") != std::string::npos)
		{
			return true;
		}
	}
	return false;
}

/* 设置配置文件 */
int CPppoeManage::set_config_file(std::string strUser, const std::string strPasswd)
{
	std::ifstream configFile(ETC_PPPOE_CONFIG_FILE);
	if (!configFile.is_open())
	{
		dlog_error("打开配置文件失败");
		return -1;
	}

	std::string strFileContent((std::istreambuf_iterator<char>(configFile)),
							   std::istreambuf_iterator<char>());
	configFile.close();

	char *pDfaultEth = get_default_interface();
	dlog_info("获取默认网卡:%s", pDfaultEth);
	/* 配置相关信息 */
	strFileContent = replaceKeyValue(strFileContent, USER_KEY, strUser);
	strFileContent = replaceKeyValue(strFileContent, ETH_KEY, pDfaultEth);
	strFileContent = replaceKeyValue(strFileContent, FIREWALL_KEY, FIREWALL_VALUE);
	strFileContent = replaceKeyValue(strFileContent, DNSTYPE_KEY, DNSTYPE_VALUE);
	strFileContent = replaceKeyValue(strFileContent, PIDFILE_KEY, "/var/run/pppoe-" + strPasswd + ".pid");
	strFileContent = replaceKeyValue(strFileContent, DEMAND_KEY, DEMAND_VALUE);
	strFileContent = replaceKeyValue(strFileContent, DNS1_KEY, DNS_EMPTY);
	strFileContent = replaceKeyValue(strFileContent, DNS2_KEY, DNS_EMPTY);
	strFileContent = replaceKeyValue(strFileContent, PEERDNS_KEY, PEERDNS_VALUE);

	/* 将配置信息写入配置文件 */
	std::ofstream updatedConfigFile(ETC_PPPOE_CONFIG_FILE);
	if (!updatedConfigFile.is_open())
	{
		dlog_error("更新配置文件失败");
		return -1;
	}
	updatedConfigFile << strFileContent;
	updatedConfigFile.close();

	dlog_info("更新pppoe配置文件成功");

	return 0;
}

std::string CPppoeManage::replaceKeyValue(std::string strFileContent, const std::string &strKey, const std::string &strNewValue)
{
	/* 查找键的位置 */
	size_t pos = strFileContent.find(strKey);
	if (pos != std::string::npos)
	{
		/* 找到后替换键后的内容 */
		size_t endPos = strFileContent.find('\n', pos);
		if (endPos == std::string::npos)
		{
			endPos = strFileContent.length();
		}

		/* 替换键后的内容 */
		strFileContent.replace(pos, endPos - pos, strKey + strNewValue);
	}
	return strFileContent;
}

int CPppoeManage::update_secrets_file(std::string strUser, const std::string strPasswd)
{
	/* pap认证 打开文件 清空文件 */
	std::ofstream papFile(PAP_SECRETS_FILE, std::ios::out | std::ios::trunc);
	if (!papFile.is_open())
	{
		dlog_error("打开认证文件失败；%s", PAP_SECRETS_FILE);
		return -1;
	}

	papFile << strUser << "    *    \"" << strPasswd << "\"    *" << std::endl;

	/*  检查文件写入是否成功 */
	if (!papFile.good())
	{
		dlog_error("写入认证信息失败：%s", PAP_SECRETS_FILE);
		papFile.close();
		return -1;
	}
	papFile.close();

	/* chap认证 打开文件 清空文件 */
	std::ofstream chapFile(CHAP_SECRETS_FILE, std::ios::out | std::ios::trunc);
	if (!chapFile.is_open())
	{
		dlog_error("打开认证文件失败；%s", CHAP_SECRETS_FILE);
		return -1;
	}

	chapFile << strUser << "    *    \"" << strPasswd << "\"    *" << std::endl;

	/*  检查文件写入是否成功 */
	if (!chapFile.good())
	{
		dlog_error("写入认证信息失败：%s", CHAP_SECRETS_FILE);
		chapFile.close();
		return -1;
	}
	chapFile.close();

	dlog_info("认证文件更新成功");
	return 0;
}