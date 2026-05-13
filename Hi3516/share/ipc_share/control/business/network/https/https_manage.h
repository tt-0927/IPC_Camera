/*** 
 * @FilePath     : https_manage.h
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2024-11-27 09:47:34
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-08-20 16:35:17
 * @Description  : 开启关闭https功能
 */

#pragma once

#include "Singleton.h"
#include "network_define.h"
#include "path_define.h"
#include "IpcRet.h"

#define HTTP_PORT_KEY                            "http port"
#define HTTPS_PORT_KEY                           "https port"
#define NGINX_CONFIG_FILE       THIRD_PATRY_PATH "nginx/conf/nginx.conf"                                    /* nginx服务器配置文件 */
#define NGINX_HTTP_CONFIG_FILE  THIRD_PATRY_PATH "nginx/conf/nginx.conf.http"                               /* nginx配置http */
#define NGINX_HTTPS_CONFIG_FILE THIRD_PATRY_PATH "nginx/conf/nginx.conf.https"                              /* nginx配置https */
#define NGINX_START_COMMAND     THIRD_PATRY_PATH "nginx/sbin/nginx -p " THIRD_PATRY_PATH "nginx/"           /* 启动nginx命令 */
#define NGINX_RELOAD_COMMAND    THIRD_PATRY_PATH "nginx/sbin/nginx -p " THIRD_PATRY_PATH "nginx/ -s reload" /* 重新加载nginx命令 */
#define NGINX_FIND_PID                           "pgrep nginx"                                              /* 查找nginx程序进程号 */

class CHttpsManage : public CSingleton<CHttpsManage>
{
    CHttpsManage();
public:
    ~CHttpsManage() = default;
    friend class CSingleton<CHttpsManage>;

	/**
	 * @brief 配置https
	 * @param stHttpsConfigInfo 
	 * @return int 
	 */
	int config_https(Network::HttpsConfigInfo_S stHttpsConfigInfo,std::function<void( int)> result);

	/**
	 * @brief https初始化
	 * @return int 
	 */
	IpcRet_E init();

	/**
	 * @brief https去初始化
	 * @return int 
	 */
	IpcRet_E deinit();

	/**
	 * @brief 获取https配置信息
	 * @param stHttpsConfigInfo 
	 */
	void get_httpsInfo(Network::HttpsConfigInfo_S &stHttpsConfigInfo);

	/**
	 * @brief 配置端口
	 * @param nHttpPort http端口 
	 * @param nHttpsPort https端口
	 * @return int 
	 */
	int config_port(int nHttpPort,int nHttpsPort);
	/**
	 * @brief 获取httpPort端口
	 * @return int 
	 */
	int get_httpPort();

private:
	/**
	 * @brief 开启https
	 * @param strCertPath 证书路径
	 * @param strKeyPath 私钥路径
	 * @return int 
	 */
	int enable_https(const std::string& strCertPath, const std::string& strKeyPath);

	/**
	 * @brief 禁用https
	 * @return int 
	 */
	int disable_https();

	/**
	 * @brief https配置文件添加证书文件和私钥
	 * @param strCertPath 
	 * @param strKeyPath 
	 * @return int 
	 */
	int https_add_certfile(const std::string& strCertPath, const std::string& strKeyPath);

	/**
	 * @brief 拷贝文件
	 * @param strSource 源文件 
	 * @param strDestination 目标文件
	 * @return true 成功
	 * @return false 失败
	 */
	bool copyFile(const std::string& strSource, const std::string& strDestination);

	/**
	 * @brief 重新加载nginx服务器配置
	 * @return int 
	 */
	int reload_nginx();

	/**
	 * @brief 替换特定注释行的端口号
	 * @param strFilePath 配置文件
	 * @param strKey 注释关键字
	 * @param nNewPort 新的端口号
	 * @return int 
	 */
	int modify_port(const std::string& strFilePath,const std::string& strKey,int nNewPort);
	
private:
	/**
	 * @brief 配置文件
	 */
	std::string m_configFile;
	std::string m_portConfigFile;

	int m_nHttpPort = 80;
};