/*** 
 * @FilePath     : https_manage.cpp
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2024-11-27 09:47:34
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-08-20 17:42:49
 * @Description  : 开启关闭https功能
 */

#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <unistd.h> 
#include <limits.h> 
#include "https_manage.h"
#include "ca_manage.h"
#include "dlog.h"
#include "web_ssl_server.h"
#include "NetDefine.h"
#include "network_convert.h"
#include "convert_interface.h"

CHttpsManage::CHttpsManage() 
: m_configFile(HTTPS_CONFIG_FILE),
m_portConfigFile(PORT_CONFIG_FILE)
{
	Network::PortConfig_S stPortConfig;
	Convert::read_file(m_portConfigFile, stPortConfig);
	m_nHttpPort = stPortConfig.nHttpPort;
}

IpcRet_E CHttpsManage::init()
{
	Network::HttpsConfigInfo_S stHttpsConfigInfo;
	Convert::read_file(m_configFile, stHttpsConfigInfo);
	if(stHttpsConfigInfo.bEnHttps)
	{
		/* 开启websocket ssl加密服务器*/
		CWebSslServer::instance()->init(stHttpsConfigInfo.strCertPath,stHttpsConfigInfo.strKeyPath);
	}

	// FILE *pFp = popen(NGINX_FIND_PID, "r");
    // if (NULL != pFp)
	// {
    //     char achPid[256] = {0};
    //     if (NULL == fgets(achPid, sizeof(achPid), pFp))
	// 	{

	// 		dlog_debug("启动nginx服务");
    //         std::system(NGINX_START_COMMAND);
    //     }
    //     pclose(pFp);
    // }

	return OK;
}

IpcRet_E CHttpsManage::deinit()
{	
	Network::HttpsConfigInfo_S stHttpsConfigInfo;
	Convert::read_file(m_configFile, stHttpsConfigInfo);
	if(stHttpsConfigInfo.bEnHttps)
	{
		/* 关闭websocket ssl加密服务器*/
		CWebSslServer::instance()->deinit();
	}
	return OK;
}

void CHttpsManage::get_httpsInfo(Network::HttpsConfigInfo_S &stHttpsConfigInfo)
{
	Convert::read_file(m_configFile, stHttpsConfigInfo);
}

/* 配置https */
int CHttpsManage::config_https(Network::HttpsConfigInfo_S stHttpsConfigInfo,std::function<void( int)> result)
{
	auto thrRun = [this](Network::HttpsConfigInfo_S stHttpsConfigInfo,std::function<void(int)> result)
    {
		int nRet = 0;
		/* 启用https */
		if(stHttpsConfigInfo.bEnHttps)
		{
			/* 是否使用本地设备的证书 */
			if (stHttpsConfigInfo.strCertPath.find(CA_DEVICE_PATH) != std::string::npos)
			{
				stHttpsConfigInfo.strKeyPath = CA_DEVICE_KEY;
			}
			else
			{

			}

			if(enable_https(stHttpsConfigInfo.strCertPath,stHttpsConfigInfo.strKeyPath) < 0)
			{
				dlog_error("https开启失败");
				nRet = -1;
			}

			if(CWebSslServer::instance()->is_init())
			{
				CWebSslServer::instance()->deinit();
			}
			
			/* 开启websocket ssl加密服务器*/
			CWebSslServer::instance()->init(stHttpsConfigInfo.strCertPath,stHttpsConfigInfo.strKeyPath);
		}
		/* 禁用https */
		else
		{
			if(disable_https() < 0)
			{
				dlog_error("https禁用失败");
				nRet = -1;
			}

			if(CWebSslServer::instance()->is_init())
			{
				CWebSslServer::instance()->deinit();
			}
			
		}

		if(reload_nginx() < 0)
		{
			dlog_error("nginx配置加载失败");
			nRet = -1;
		}

		if(nRet == 0)
		{
			Convert::write_file(m_configFile, stHttpsConfigInfo);
		}
		
		result(nRet);
	};
	std::thread thr(thrRun,stHttpsConfigInfo, result);
    thr.detach();
	
	return 0;
}

/* 配置端口 */
int CHttpsManage::config_port(int nHttpPort,int nHttpsPort)
{
	/* 配置http端口 */
	modify_port(NGINX_CONFIG_FILE,HTTP_PORT_KEY,nHttpPort);
	modify_port(NGINX_HTTP_CONFIG_FILE,HTTP_PORT_KEY,nHttpPort);
	modify_port(NGINX_HTTPS_CONFIG_FILE,HTTP_PORT_KEY,nHttpPort);
	
	/* 配置https端口 */
	modify_port(NGINX_CONFIG_FILE,HTTPS_PORT_KEY,nHttpsPort);
	modify_port(NGINX_HTTPS_CONFIG_FILE,HTTPS_PORT_KEY,nHttpsPort);

	if(reload_nginx() < 0)
	{
		dlog_error("nginx配置加载失败");
		return -1;
	}

	return 0;
}

int CHttpsManage::get_httpPort()
{
	Network::PortConfig_S stPortConfig;
	Convert::read_file(m_portConfigFile, stPortConfig);
	//dlog_debug("获取http端口号:%d",stPortConfig.nHttpPort);
	
	return stPortConfig.nHttpPort;
}


int CHttpsManage::enable_https(const std::string& strCertPath, const std::string& strKeyPath)
{
	dlog_info("开启https");
	if(https_add_certfile(strCertPath,strKeyPath) < 0)
	{
		dlog_error("修改配置文件失败");
		return -1;
	}

	if(!copyFile(NGINX_HTTPS_CONFIG_FILE,NGINX_CONFIG_FILE))
	{
		dlog_error("复制文件失败");
		return -1;
	}

	return 0;
}

int CHttpsManage::disable_https()
{
	dlog_info("关闭https");

	if(!copyFile(NGINX_HTTP_CONFIG_FILE,NGINX_CONFIG_FILE))
	{
		dlog_error("复制文件失败");
		return -1;
	}

	return 0;
}

/* https配置文件添加相关证书文件 */
int CHttpsManage::https_add_certfile(const std::string& strCertPath, const std::string& strKeyPath)
{
	std::ifstream infile(NGINX_HTTPS_CONFIG_FILE);
    std::string strLine;
    std::string strConfig;

	if (!infile.is_open()) 
	{
		dlog_error("打开文件失败%s",NGINX_HTTPS_CONFIG_FILE);
        return -1;
    }

    while (std::getline(infile, strLine)) 
	{
        /* 使用正则表达式替换证书路径和私钥路径 */ 
        if (strLine.find("ssl_certificate_key") != std::string::npos) 
		{
            strLine = "        ssl_certificate_key " + strKeyPath + ";";
        }
		else if (strLine.find("ssl_certificate") != std::string::npos) 
		{
            strLine = "        ssl_certificate " + strCertPath + ";";
        }
        strConfig += strLine + "\n";
    }
    infile.close();

    /* 将修改后的配置写回到文件中*/ 
    std::ofstream outfile(NGINX_HTTPS_CONFIG_FILE);
    if (outfile.is_open()) 
	{
        outfile << strConfig;
        outfile.close();
        dlog_info("https配置文件已更新:%s",NGINX_HTTPS_CONFIG_FILE);
    }
	else 
	{
        dlog_error("打开文件失败%s",NGINX_HTTPS_CONFIG_FILE);
    }

	return 0;
}


bool CHttpsManage::copyFile(const std::string& strSource, const std::string& strDestination)
{
	  /* 打开源文件 */ 
    std::ifstream src(strSource, std::ios::binary);
    if (!src.is_open()) 
	{
		dlog_error("打开文件失败%s",strSource.c_str());
        return false;
    }

    /*  打开目标文件 */
    std::ofstream dest(strDestination, std::ios::binary);
    if (!dest.is_open()) 
	{
        dlog_error("打开文件失败%s",strDestination.c_str());
        return false;
    }

    /* 复制文件内容 */ 
    dest << src.rdbuf();  

    return true;
}

int CHttpsManage::reload_nginx()
{
 	/* 执行 `nginx -s reload` 命令以重新加载 Nginx 配置 */ 
    int nResult = std::system(NGINX_RELOAD_COMMAND);

    if (nResult == 0) 
	{
        dlog_info("重新加载nginx配置文件成功");
		return 0;
    }
	else 
	{
        dlog_error("重新加载nginx配置文件失败");
		return -1;
    }
}

int CHttpsManage::modify_port(const std::string& strFilePath,const std::string& strKey,int nNewPort)
{
	std::ifstream inputFile(strFilePath);
    if (!inputFile.is_open()) 
	{
		dlog_error("打开文件失败: %s",strFilePath.c_str());
        return -1;
    }

    std::string modifiedContent;
    std::string line;
	std::string regexPattern = R"(listen\s+(\[::\]:)?\d+\s*(\s+ssl)?;\s*#\s*)" + strKey;
    std::regex listenRegex(regexPattern);

    while (std::getline(inputFile, line)) 
	{
        /* 检查是否匹配目标 listen 行 */ 
        if (std::regex_search(line, listenRegex)) 
		{
			/* 保留原来的缩进 */
			std::string indent = line.substr(0, line.find_first_not_of(" \t"));

			/* 判断是否有 IPv6 前缀 */
			bool bIsIPv6 = (line.find("[::]:") != std::string::npos);

			/* 动态处理 ssl 关键字 */ 
            size_t sslPos = line.find("ssl");
            std::string sslPart = (sslPos != std::string::npos) ? " ssl" : "";

            /* 构造新的 listen 行 */
			if (bIsIPv6)
			{
				line = indent + "listen [::]:" + std::to_string(nNewPort) + sslPart + "; # " + strKey;
			}
			else {
				line = indent + "listen " + std::to_string(nNewPort) + sslPart + "; # " + strKey;
			}
        }
        modifiedContent += line + "\n";
    }
    inputFile.close();

    /* 写回文件 */ 
    std::ofstream outputFile(strFilePath);
    if (!outputFile.is_open()) 
	{
        dlog_error("打开文件失败: %s",strFilePath.c_str());
        return -1;
    }
    outputFile << modifiedContent;
    outputFile.close();

	return 0;
}
