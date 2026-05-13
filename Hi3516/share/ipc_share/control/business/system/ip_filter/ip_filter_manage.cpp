/*** 
 * @FilePath     : ip_filter_manage.cpp
 * @Author       : huangjunda
 * @Date         : 2025-08-19 20:51:23
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-08-20 15:39:15
 * @Description  : 
 */

#include "ip_filter_manage.h"

#include "convert_interface.h"
#include "path_define.h"

CIpFilterManage::CIpFilterManage()
: m_strConfigFile(IP_FILTER_CONFIG_FILE)
{
}

IpcRet_E CIpFilterManage::init()
{
	System::IpFilterConfigInfo_S stInfo;
	if (Convert::read_file(m_strConfigFile, stInfo))
	{
		Convert::write_file(m_strConfigFile, stInfo);
	}
    return OK;
}

IpcRet_E CIpFilterManage::deinit()
{
    return OK;
}

void CIpFilterManage::get_ip_filter_info(System::IpFilterConfigInfo_S &stInfo)
{
    Convert::read_file(m_strConfigFile, stInfo);
}

int CIpFilterManage::set_ip_filter_info(System::IpFilterConfigInfo_S stInfo)
{
    if (stInfo.bEnable)
    {
        update_ip_filter_config(stInfo);
    }
    else
    {
        clear_ip_filter_config(stInfo);
    }

    Convert::write_file(m_strConfigFile, stInfo);

    return OK;
}

int CIpFilterManage::add_ip(const std::string strIp)
{
    System::IpFilterConfigInfo_S stInfo;
    Convert::read_file(m_strConfigFile, stInfo);

    auto it = std::find_if(stInfo.vecIps.begin(), stInfo.vecIps.end(),
    [&](const System::IpFilterInfo_S& info)
    {
        return info.strIp == strIp;
    });

    if (it != stInfo.vecIps.end())
    {
        dlog_error("该IP过滤地址已存在，无法添加");
        return ERR;
    }

    System::IpFilterInfo_S addIpInfo;
    addIpInfo.bEnable = true;
    addIpInfo.strIp = strIp;
    stInfo.vecIps.push_back(addIpInfo);

    if (stInfo.bEnable)
    {
        update_ip_filter_config(stInfo);
    }

    Convert::write_file(m_strConfigFile, stInfo);

    return OK;
}

int CIpFilterManage::remove_ip(const std::string strIp)
{
    System::IpFilterConfigInfo_S stInfo;
    Convert::read_file(m_strConfigFile, stInfo);

    auto it = std::find_if(stInfo.vecIps.begin(), stInfo.vecIps.end(),
    [&](const System::IpFilterInfo_S& info)
    {
        return info.strIp == strIp;
    });

    if (it == stInfo.vecIps.end())
    {
        dlog_error("该IP过滤地址不存在，无法删除");
        return ERR;
    }

    stInfo.vecIps.erase(it);

    if (stInfo.bEnable)
    {
        update_ip_filter_config(stInfo);
    }
    
    Convert::write_file(m_strConfigFile, stInfo);

    return OK;
}

int CIpFilterManage::modify_ip(System::IpFilterModify_S stIpInfo)
{
    System::IpFilterConfigInfo_S stInfo;
    Convert::read_file(m_strConfigFile, stInfo);

    if (stIpInfo.strOldIp == stIpInfo.strNewIp)
    {
        dlog_error("要修改的IP过滤地址和修改前的IP过滤地址一致，不进行修改");
        return OK;
    }

    /* 检查旧IP是否存在 */
    auto it = std::find_if(stInfo.vecIps.begin(), stInfo.vecIps.end(),
    [&](const System::IpFilterInfo_S& info)
    {
        return info.strIp == stIpInfo.strOldIp;
    });

    if (it == stInfo.vecIps.end())
    {
        dlog_error("待修改的IP过滤地址不存在，无法修改");
        return ERR;
    }

    /* 检查新IP是否冲突 */
    auto exist = std::find_if(stInfo.vecIps.begin(), stInfo.vecIps.end(),
    [&](const System::IpFilterInfo_S& info)
    {
        return info.strIp == stIpInfo.strNewIp;
    });

    if (exist != stInfo.vecIps.end())
    {
        dlog_error("要修改的IP过滤地址已存在，无法修改");
        return ERR;
    }

    it->strIp = stIpInfo.strNewIp;

    if (stInfo.bEnable)
    {
        update_ip_filter_config(stInfo);
    }

    Convert::write_file(m_strConfigFile, stInfo);

    return OK;
}

int CIpFilterManage::update_ip_filter_config(System::IpFilterConfigInfo_S stInfo)
{
    std::ofstream outfile(NGINX_IP_FILTER_CONFIG_FILE, std::ios::trunc);
    if(!outfile.is_open())
    {
        dlog_error("打开文件失败");
        return ERR_OPEN;
    }

    if (System::IPFILTER_DENY == stInfo.eMode)
    {
        for(auto &ipInfo : stInfo.vecIps)
        {
            if (ipInfo.bEnable)
            {
                outfile << "deny " << ipInfo.strIp << ";\n";
            }
        }
        outfile << "allow all;\n";
    }
    else if (System::IPFILTER_ALLOW == stInfo.eMode)
    {
        for (auto &ipInfo : stInfo.vecIps)
        {
            if (ipInfo.bEnable)
            {
                outfile << "allow " << ipInfo.strIp << ";\n";
            }
        }
        outfile << "deny all;\n";
    }
    outfile.close();
    reload_nginx();

    return OK;
}

int CIpFilterManage::clear_ip_filter_config(System::IpFilterConfigInfo_S stInfo)
{
    std::ofstream outfile(NGINX_IP_FILTER_CONFIG_FILE, std::ios::trunc);
    if(!outfile.is_open())
    {
        dlog_error("打开文件失败");
        return ERR_OPEN;
    }
    outfile.close();
    reload_nginx();

    return OK;
}

int CIpFilterManage::reload_nginx()
{
 	/* 执行 `nginx -s reload` 命令以重新加载 Nginx 配置 */
    if (0 != std::system(NGINX_RELOAD_COMMAND)) 
	{
        dlog_error("重新加载nginx配置文件失败");
		return ERR;
    }
	else 
	{
        dlog_info("重新加载nginx配置文件成功");
		return OK;
    }
}