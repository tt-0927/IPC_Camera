/**
 * @FilePath     : multicast_udp_server.cpp
 * @Author       : huangjunda
 * @Date         : 2025-03-28 17:23:18
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-18 16:33:39
 * @Description  : 多播 UDP 服务器
 */

#include "multicast_udp_server.h"

void *CMulticastUdpServer::multicast_sendLoop()
{
    /* 获取网卡信息 */
    ::Network::Info_S stInfo;
    CNetworkManage::instance()->get_system_networkInfo(stInfo);

    while (!this->m_stuMulticastHandle->nExit)
    {
        os_networkmulticast_send(stInfo.stIp.ipv4Ip.c_str(), stInfo.stIp.ipv4Ip.size(), this->m_stuMulticastHandle);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return NULL;
}

IpcRet_E CMulticastUdpServer::init()
{
    dlog_debug("开始初始化组播服务器");
    int nRet = 0;
    struct timespec stuTime = { 0, 0 };

    /* 获取设备UUID */
    std::ifstream file(MULTICAST_UUID_PATH);
    if (file.is_open())
    {
        std::getline(file, m_strUuid);
        /* 移除字符串中所有的'-' */
        m_strUuid.erase(std::remove(m_strUuid.begin(), m_strUuid.end(), '-'), m_strUuid.end());
        file.close();
    }
    else
    {
        dlog_error("组播打开设备uuid文件失败")
        return ERR;
    }
    
    /* 获取服务启动时间 */
    clock_gettime(CLOCK_MONOTONIC, &stuTime);
    m_nStartTime = stuTime.tv_sec;

    m_stuMulticastHandle = (NetworkMulticast_S *)malloc(sizeof(NetworkMulticast_S));
    memset(m_stuMulticastHandle, 0, sizeof(NetworkMulticast_S));
    m_stuMulticastHandle->entype = MULTICAST;
    strncpy(m_stuMulticastHandle->amcast_ip, MULIT_ADDR_DEFAULT, LENGTH16);
    m_stuMulticastHandle->nmcast_port = MULIT_PORT_DEFAULT;
    m_stuMulticastHandle->stsrc_ip_info.port = MULIT_PORT_DEFAULT;
    m_stuMulticastHandle->stuser_recv_handle.user = this;
    m_stuMulticastHandle->fnhandledata = &CMulticastUdpServer::network_multicast_handle;

    nRet = os_networkmulticast_init(m_stuMulticastHandle);
    if (0 != nRet)
    {
        dlog_debug("初始化组播服务器失败");
        return ERR;
    }

    m_stuMulticastHandle->nExit = 0;
    /* 启动线程，将业务逻辑封装到 multicast_sendLoop */
    // m_sendThread = std::thread(&CMulticastUdpServer::multicast_sendLoop, this);

    return OK;
}

IpcRet_E CMulticastUdpServer::deinit()
{
    this->m_stuMulticastHandle->nExit = 1;
    os_networkmulticast_exit(m_stuMulticastHandle);
    // if (m_sendThread.joinable())
    // {
    //     m_sendThread.join(); // 等待线程退出
    // }
    return OK;
}

void *CMulticastUdpServer::network_multicast_handle(void *pParam)
{
    UserRecv_S *pRecvParam = (UserRecv_S *)pParam;
    CMulticastUdpServer *pThis = (CMulticastUdpServer *)pRecvParam->user;
    std::string strData(pRecvParam->data);

    /* 搜索 */
    if (std::string::npos != strData.find(MULTICAST_CMD_SEARCH))
    {
        pThis->search_response(pParam);
    }
    /* 配置 */
    else if (std::string::npos != strData.find(MULTICAST_CMD_CONFIG))
    {
        pThis->config_response(pParam);
    }
    /* 重启 */
    else if (std::string::npos != strData.find(MULTICAST_CMD_REBOOT))
    {
        pThis->reboot_response(pParam);
    }

    return NULL;
}

void CMulticastUdpServer::search_response(void *pParam)
{
    UserRecv_S *pRecvParam = (UserRecv_S *)pParam;
    char *pClientId = NULL;
    char achMsg[LENGTH512] = {0};
    struct timespec stuTime = {.tv_sec = 0, .tv_nsec = 0};
    ::Network::Info_S stInfo;

    /* 获取网卡信息 */
    CNetworkManage::instance()->get_system_networkInfo(stInfo);
    /* 获取搜索工具ID */
    pClientId = strstr(pRecvParam->data, MULTICAST_RECEIVE_CLIENT_ID);
    /* 获取当前时间点 */
    clock_gettime(CLOCK_MONOTONIC, &stuTime);

    snprintf(achMsg, sizeof(achMsg), MULTICAST_ANSWER_SEARCH, SYSTEM_VERSION, DEVICE_CODE, pClientId,
             m_strUuid.c_str(), (long int)(stuTime.tv_sec - m_nStartTime), stInfo.stIp.bEnableDhcp ? 1 : 0,
             stInfo.stIp.ipv4Ip.c_str(), stInfo.stIp.ipv4Mask.c_str(), stInfo.stIp.ipv4Gateway.c_str(),
             stInfo.stIp.physicalAddress.c_str(), stInfo.stDns.main.c_str());
    dlog_info("%s", achMsg);
    os_networkmulticast_send(achMsg, strlen(achMsg), this->m_stuMulticastHandle);
}

void CMulticastUdpServer::config_response(void *pParam)
{
    UserRecv_S *pRecvParam = (UserRecv_S *)pParam;
    std::string strData(pRecvParam->data);
    ::Network::Info_S stInfo;
    char achOldIp[LENGTH16]   = {0};
    char achNewIp[LENGTH16]   = {0};
    char achMask[LENGTH16]    = {0};
    char achGateway[LENGTH16] = {0};
    char achDns[LENGTH16]     = {0};
    int  nDhcp                =  0;

    /* 获取网卡信息 */
    CNetworkManage::instance()->get_system_networkInfo(stInfo);

    /* 获取当前搜索工具要修改的IP */
    if (false == xml_get_charNode1("/CONFIGNET/oldIP/", achOldIp, (char *)strData.c_str(), sizeof(achOldIp)))
    {
        dlog_error("获取搜索工具配置IP信息失败");
        return;
    }

    /* 判断要修改的IP与本机IP是否相同,不相同则过滤掉 */
    if (0 != stInfo.stIp.ipv4Ip.compare(achOldIp))
    {
        return;
    }

    /* 获取当前搜索工具设置的DHCP */
    if (false == xml_get_intNode1("/CONFIGNET/DHCP/", &nDhcp, (char *)strData.c_str()))
    {
        dlog_error("获取搜索工具配置DHCP信息失败");
        return;
    }

    /* 获取返回字段信息 */
    if (0 == nDhcp)
    {
        xml_get_charNode1("/CONFIGNET/newIP/", achNewIp, (char *)strData.c_str(), sizeof(achNewIp));
        xml_get_charNode1("/CONFIGNET/gateway/", achGateway, (char *)strData.c_str(), sizeof(achGateway));
        
        stInfo.stIp.ipv4Ip = achNewIp;
        stInfo.stIp.ipv4Gateway = achGateway;
    }
    xml_get_charNode1("/CONFIGNET/mask/", achMask, (char *)strData.c_str(), sizeof(achMask));
    xml_get_charNode1("/CONFIGNET/DNS/", achDns, (char *)strData.c_str(), sizeof(achDns));
    stInfo.stIp.ipv4Mask = achMask;
    stInfo.stDns.main = achDns;
    stInfo.stIp.bEnableDhcp = nDhcp ? true : false;

    /* 设置网卡信息 */
    CNetworkManage::instance()->set_system_networkInfo(stInfo, false);
}

void CMulticastUdpServer::reboot_response(void *pParam)
{
    UserRecv_S *pRecvParam = (UserRecv_S *)pParam;
    std::string strData(pRecvParam->data);
    char achDeviceId[LENGTH64] = {0};

    /* 获取当前搜索工具返回的摄像机UUID */
    if (false == xml_get_charNode1("/CAMERA_REBOOT/DeviceID/", achDeviceId, (char *)strData.c_str(), sizeof(achDeviceId)))
    {
        dlog_error("获取搜索工具摄像机UUID信息失败");
        return;
    }

    /* 判断发送的摄像机UUID与本机设备UUID是否相同,不相同则过滤掉 */
    if (0 != m_strUuid.compare(achDeviceId))
    {
        return;
    }
    
    /* 直接生效,不需要重启 */
    // dlog_debug("组播搜索工具配置摄像机网络信息直接生效,不需要重启");
    SystemManage::instance()->system_reboot([](int nRet) {});
}