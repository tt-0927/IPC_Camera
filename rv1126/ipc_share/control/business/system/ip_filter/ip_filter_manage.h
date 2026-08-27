/*** 
 * @FilePath     : ip_filter_manage.h
 * @Author       : huangjunda
 * @Date         : 2025-08-19 20:51:31
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-08-20 15:33:17
 * @Description  : 
 */

#pragma once

#include "system_define.h"
#include "Singleton.h"
#include "IpcRet.h"

#define NGINX_IP_FILTER_CONFIG_FILE THIRD_PATRY_PATH "nginx/conf/ip_filter.conf"                                /* nginx配置IP地址过滤 */
#define NGINX_RELOAD_COMMAND        THIRD_PATRY_PATH "nginx/sbin/nginx -p " THIRD_PATRY_PATH "nginx/ -s reload" /* 重新加载nginx命令 */

class CIpFilterManage : public CSingleton<CIpFilterManage>
{
    CIpFilterManage();
public:
    ~CIpFilterManage() = default;
    friend class CSingleton<CIpFilterManage>;

    /**
     * @brief IP地址过滤初始化
     * @return IpcRet_E
     */
    IpcRet_E init();

    /**
     * @brief IP地址过滤去初始化
     * @return IpcRet_E
     */
    IpcRet_E deinit();

    /**
     * @brief 获取IP地址过滤信息
	 * @param IpFilterConfigInfo_S &stInfo 
     * @return void
     */
    void get_ip_filter_info(System::IpFilterConfigInfo_S &stInfo);

    /**
     * @brief 设置IP地址过滤信息
	 * @param IpFilterConfigInfo_S stInfo 
     * @return int
     */
    int set_ip_filter_info(System::IpFilterConfigInfo_S stInfo);

    /*** 
     * @brief  添加IP过滤地址
     * @author huangjunda
     * @param  {string} strIp
     * @return {int}
     */    
    int add_ip(const std::string strIp);

    /*** 
     * @brief  删除IP过滤地址
     * @author huangjunda
     * @param  {string} strIp
     * @return {int}
     */ 
    int remove_ip(const std::string strIp);

    /*** 
     * @brief  修改IP过滤地址
     * @author huangjunda
     * @param  {System::IpFilterModify_S} stIpInfo
     * @return {int}
     */ 
    int modify_ip(System::IpFilterModify_S stIpInfo);

private:

    /*** 
     * @brief  更新IP过滤地址文件
     * @author huangjunda
     * @param  {IpFilterConfigInfo_S} stInfo
     * @return {int}
     */    
    int update_ip_filter_config(System::IpFilterConfigInfo_S stInfo);

    /*** 
     * @brief  清空IP过滤地址文件
     * @author huangjunda
     * @param  {IpFilterConfigInfo_S} stInfo
     * @return {int}
     */ 
    int clear_ip_filter_config(System::IpFilterConfigInfo_S stInfo);

    /*** 
     * @brief  重新加载nginx程序
     * @author huangjunda
     * @return {int}
     */ 
    int reload_nginx();

    /* 配置文件 */
    std::string m_strConfigFile;
};
