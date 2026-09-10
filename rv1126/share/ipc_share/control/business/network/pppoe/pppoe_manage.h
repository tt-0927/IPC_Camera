/***
 * @FilePath     : pppoe_manage.h
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2024-12-02 09:53:20
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-28 09:54:17
 * @Description  :
 */

#pragma once

#include "IpcRet.h"
#include "network_define.h"
#include "Singleton.h"
#include "IpcRet.h"

#define PPPOE_INTERFACE "ppp0"			 /* ppp拨号网卡 */
#define PPPOE_EMPTY_IP "0.0.0.0"		 /* pppoe空ip  */
#define PPPOE_TEST_DOMIN "www.baidu.com" /* 测试域名 */

#define ETC_PPPOE_CONFIG_FILE "/etc/ppp/pppoe.conf" /* pppoe拨号配置文件 */
#define PAP_SECRETS_FILE "/etc/ppp/pap-secrets"		/* pap认证文件 */
#define CHAP_SECRETS_FILE "/etc/ppp/chap-secrets"	/* chap认证文件 */

/* PPPOE配置文件相关项 */
#define USER_KEY "USER="
#define ETH_KEY "ETH="
#define FIREWALL_KEY "FIREWALL="
#define FIREWALL_VALUE "NONE"
#define DNSTYPE_KEY "DNSTYPE="
#define DNSTYPE_VALUE "SERVER"
#define PIDFILE_KEY "PIDFILE="
#define DEMAND_KEY "DEMAND="
#define DEMAND_VALUE "no"
#define DNS1_KEY "DNS1="
#define DNS2_KEY "DNS2="
#define DNS_EMPTY ""
#define PEERDNS_KEY "PEERDNS="
#define PEERDNS_VALUE "yes"

#define PPPOE_CONNECT_CMD "pppoe-connect /etc/ppp/pppoe.conf > /dev/null 2>&1 &" /* 启动pppoe拨号命令 */
#define PPPOE_STOP_CMD "pppoe-stop"												 /* 停止pppoe拨号命令 */

class CPppoeManage : public CSingleton<CPppoeManage>
{
	CPppoeManage();

public:
	virtual ~CPppoeManage() = default;
	/* 允许 Singleton 访问私有构造函数 */
	friend class CSingleton<CPppoeManage>;

	IpcRet_E init();
	IpcRet_E deinit();

	/**
	 * @brief 配置pppoe拨号上网
	 * @param stPppoe pppoe信息
	 * @return int
	 */
	int config_pppoe(Network::Pppoe_S stPppoe);

	/**
	 * @brief 获取pppoe配置信息
	 * @param stPppoe pppoe配置信息
	 * @return int
	 */
	int get_pppoe_config(Network::Pppoe_S &stPppoe);
	/**
	 * @brief 获取pppoe拨号上网的ip
	 * @return std::string
	 */
	std::string get_pppoe_ip();

private:
	/**
	 * @brief 开启pppoe拨号
	 * @param stPppoe pppoe信息
	 * @return int
	 */
	int start_pppoe(Network::Pppoe_S stPppoe);

	/**
	 * @brief 关闭ppoe拨号
	 * @return int
	 */
	int stop_pppoe();

	/**
	 * @brief 设置配置文件
	 * @param strUser pppoe服务器用户
	 * @param strPasswd  pppoe服务器密码
	 * @return int
	 */
	int set_config_file(std::string strUser, const std::string strPasswd);

	/**
	 * @brief 更新认证文件
	 * @param strUser pppoe服务器用户
	 * @param strPasswd pppoe服务器密码
	 * @return int
	 */
	int update_secrets_file(std::string strUser, const std::string strPasswd);

	/**
	 * @brief 替换键值
	 * @param strFileContent
	 * @param strKey 键值
	 * @param strNewValue 要替换的值
	 * @return std::string
	 */
	std::string replaceKeyValue(std::string strFileContent, const std::string &strKey, const std::string &strNewValue);
	/**
	 * @brief 是否存在ppp网卡
	 * @return true
	 * @return false
	 */
	bool is_interface_exist();

private:
	/* 配置文件 */
	std::string m_configFile;
	bool m_start = false;
};