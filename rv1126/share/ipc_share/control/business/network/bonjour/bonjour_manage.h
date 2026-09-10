/*** 
 * @FilePath     : bonjour_manage.h
 * @Author       : huangjunda
 * @Date         : 2025-04-28 11:06:52
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-05-12 16:37:15
 * @Description  : 
 */

#pragma once

#include <atomic>
#include <thread>

#include "IpcRet.h"
#include "Singleton.h"
#include "dlog.h"
#include "path_define.h"
#include "network_define.h"
#include "network_convert.h"
#include "convert_interface.h"
#include "action_code.h"
#include "mdns.h"

#define BONJOUR_HOST_NAME_DEFAULT "IPC"
#define BONJOUR_SERVER_NAME_DEFAULT "_http._tcp.local."

class CBonjourManage : public CSingleton<CBonjourManage>
{
    CBonjourManage();

public:
    virtual ~CBonjourManage() = default;
    /* 允许 Singleton 访问私有构造函数 */
    friend class CSingleton<CBonjourManage>;

    IpcRet_E init();
    IpcRet_E deinit();
    int set_bonjour_config(Network::BonjourConfigInfo_S stBonjour);
    int get_bonjour_config(Network::BonjourConfigInfo_S &stBonjour);

private:
    void start_bonjour_server(Network::BonjourConfigInfo_S stBonjourConfigInfo);
    void stop_bonjour_server();
    std::string                  m_configFile;   /* 配置文件 */
    Network::BonjourConfigInfo_S m_stInfo;       /* 配置文件信息 */
    std::atomic<bool>            bEnableRunning; /* 表示代理是否在运行 */
    std::unique_ptr<std::thread> bonjourThread;  /* Bonjour代理线程 */
};