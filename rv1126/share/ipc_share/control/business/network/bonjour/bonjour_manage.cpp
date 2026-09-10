/***
 * @FilePath     : bonjour_manage.cpp
 * @Author       : huangjunda
 * @Date         : 2025-04-28 11:06:48
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-04-29 16:05:22
 * @Description  :
 */

#include "bonjour_manage.h"

CBonjourManage::CBonjourManage() : m_configFile(BONJOUR_CONFIG_FILE)
{
}

IpcRet_E CBonjourManage::init()
{
    Network::BonjourConfigInfo_S stBonjourConfigInfo;
    Convert::read_file(BONJOUR_CONFIG_FILE, stBonjourConfigInfo);
    int nRet = set_bonjour_config(stBonjourConfigInfo);
    if (nRet < OK)
    {
        return ERR;
    }

    return OK;
}

IpcRet_E CBonjourManage::deinit()
{
    return OK;
}

int CBonjourManage::set_bonjour_config(Network::BonjourConfigInfo_S stBonjourConfigInfo)
{
    if (stBonjourConfigInfo.strHostName.empty())
    {
        stBonjourConfigInfo.strHostName = BONJOUR_HOST_NAME_DEFAULT;
    }

    if (stBonjourConfigInfo.strServerName.empty())
    {
        stBonjourConfigInfo.strServerName = BONJOUR_SERVER_NAME_DEFAULT;
    }

    if (0 == stBonjourConfigInfo.nPort)
    {
        stBonjourConfigInfo.nPort = OUT_HTTP_DEFAULT_PORT;
    }

    m_stInfo = stBonjourConfigInfo;

    if (m_stInfo.bEnBonjour && !bEnableRunning)
    {
        start_bonjour_server(m_stInfo);
    }
    else if (!m_stInfo.bEnBonjour && bEnableRunning)
    {
        stop_bonjour_server();
    }

    Convert::write_file(m_configFile, m_stInfo);
    return 0;
}

int CBonjourManage::get_bonjour_config(Network::BonjourConfigInfo_S &stBonjourConfigInfo)
{
    Convert::read_file(m_configFile, stBonjourConfigInfo);
    return 0;
}

void CBonjourManage::start_bonjour_server(Network::BonjourConfigInfo_S stBonjourConfigInfo)
{
    if (!stBonjourConfigInfo.bEnBonjour)
    {
        return;
    }
    
    /* 启动代理服务线程 */
    bonjourThread = std::make_unique<std::thread>(
        [](const char *hostname, const char *service, int port)
        {
            service_mdns(hostname, service, port);
        },
        m_stInfo.strHostName.c_str(),
        m_stInfo.strServerName.c_str(),
        m_stInfo.nPort);
    bEnableRunning = true;
}

void CBonjourManage::stop_bonjour_server()
{
    /* 停止代理 */
    if (bEnableRunning && bonjourThread && bonjourThread->joinable())
    {
        bonjourThread->join();
    }
    bEnableRunning = false;
}