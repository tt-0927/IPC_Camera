/***
 * @FilePath     : ipc_multicast_server.cpp
 * @Author       : huangjunda
 * @Date         : 2025-06-23 16:54:33
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-09-06 10:56:54
 * @Description  : 多播服务
 */

#include "ipc_multicast_server.h"

#include <cstring>
#include "share_define.h"
#include "system_manage.h"
#include "network_manage.h"
#include "user_manage.h"
#include "log_handler.h"
#include "dlog.h"
#include <arpa/inet.h>

/***
 * @description : 初始化
 * @author      : huangjunda
 * @return       {IpcRet_E}
 */
IpcRet_E CIpcMulticastServer::init()
{
    /* 获取服务启动时间 */
    struct timespec stuTime = {0};
    clock_gettime(CLOCK_MONOTONIC, &stuTime);
    m_nStartTime = stuTime.tv_sec;

    Network::Info_S stInfo;
    CNetworkManage::instance()->get_system_networkInfo(stInfo);

    m_pHandle = (NetworkMulticast_S *)malloc(sizeof(NetworkMulticast_S));
    memset(m_pHandle, 0, sizeof(NetworkMulticast_S));

    /* 组播接收客户端消息 */
    m_pHandle->entype = MULTICAST;
    strncpy(m_pHandle->amcast_ip, stInfo.stIp.multicastAddress.c_str(), LENGTH16);
    m_pHandle->nmcast_port = MULTICAST_PORT;
    m_pHandle->stsrc_ip_info.port = MULTICAST_PORT;

    /* 单播发送参数设置 */
    m_pHandle->stunque_addr.sin_family = AF_INET;
    m_pHandle->stunque_addr.sin_port = htons(MULTICAST_PORT);
    m_pHandle->stunque_addr.sin_addr.s_addr = INADDR_NONE;

    /* 广播发送参数设置 */
    m_pHandle->stbroadcast_addr.sin_family = AF_INET;
    m_pHandle->stbroadcast_addr.sin_port = htons(MULTICAST_PORT);
    m_pHandle->stbroadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;

    m_pHandle->stuser_recv_handle.user = this;
    m_pHandle->fnhandledata = &CIpcMulticastServer::multicast_handle;

    m_pHandle->nExit = 1;

    /* 是否运行服务器 */
    if (stInfo.stIp.bEnableMulticast && 0 != stInfo.stIp.multicastAddress.length())
    {
        start(stInfo.stIp.multicastAddress.c_str());
    }

    dlog_debug("成功初始化多播服务器");

    return OK;
}

/***
 * @description : 去初始化
 * @author      : huangjunda
 * @return       {IpcRet_E}
 */  
IpcRet_E CIpcMulticastServer::deinit()
{
    stop();

    return OK;
}

/***
 * @description : 启动服务
 * @author      : huangjunda
 * @param        {char *} pAddress
 * @return       {*}
 */
void CIpcMulticastServer::start(const char * pAddress)
{
    if (!m_pHandle->nExit)
    {
        /* 多播地址有变化，需要重启服务 */
        if (0 != strncmp(m_pHandle->amcast_ip, pAddress, LENGTH16))
        {
            stop();
            strncpy(m_pHandle->amcast_ip, pAddress, LENGTH16);
            m_pHandle->nExit = 0;
            if (0 != os_networkmulticast_init(m_pHandle))
            {
                dlog_debug("初始化多播服务器失败");
                return;
            }
        }
        else
        {
            dlog_debug("多播服务器正在运行中，无法重复启动");
        }
        
        return;
    }
    
    strncpy(m_pHandle->amcast_ip, pAddress, LENGTH16);
    m_pHandle->nExit = 0;
    if (0 != os_networkmulticast_init(m_pHandle))
    {
        dlog_debug("初始化多播服务器失败");
        return;
    }

    return;
}

/***
 * @description : 停止服务
 * @author      : huangjunda
 * @return       {*}
 */
void CIpcMulticastServer::stop()
{
    if (m_pHandle->nExit)
    {
        dlog_debug("多播服务器没有运行，无法停止");
        return;
    }
    
    m_pHandle->nExit = 1;
    if (0 != os_networkmulticast_exit(m_pHandle))
    {
        dlog_debug("初始化多播服务器失败");
        return;
    }

    return;
}

/***
 * @description : 多播处理
 * @author      : huangjunda
 * @param        {void} *pParam
 * @return       {*}
 */
void *CIpcMulticastServer::multicast_handle(void *pParam)
{
    UserRecv_S *pRecvParam = (UserRecv_S *)pParam;
    CIpcMulticastServer *pThis = (CIpcMulticastServer *)pRecvParam->user;
    COMM_BUFFER *pRecvBuffer = reinterpret_cast<COMM_BUFFER *>(pRecvParam->data);
    // pThis->print_search_params(pParam);

    /* 添加来源过滤：只处理发送端发出的请求（nMisc=0），忽略设备端响应（nMisc=1） */
    if (MULTICAST_SERVICE_TYPE::MULTICAST_CLIENT != pRecvBuffer->commHead.nMisc)
    {
        // dlog_debug("忽略其他服务器的消息");
        return NULL;
    }

    // dlog_info("pRecvBuffer->commHead.nCommand: %d", pRecvBuffer->commHead.nCommand);
    switch (pRecvBuffer->commHead.nCommand)
    {
    /* 登录 */
    case NET_CMD_TYPE::NET_LOGON_SERVER:
        /* code */
        break;
    /* 保持活动 */
    case NET_CMD_TYPE::NET_KEEP_ALIVE:
        /* code */
        break;
    /* 更新文件 */
    case NET_CMD_TYPE::NET_UPDATE_FILE:
        /* code */
        break;
    /* 对讲 */
    case NET_CMD_TYPE::NET_TALKBACK:
        /* code */
        break;
    /* 对讲停止 */
    case NET_CMD_TYPE::NET_TALKBACK_STOP:
        /* code */
        break;
    /* 搜索 */
    case NET_CMD_TYPE::NET_SEARCH_SERVER:
        pThis->search_handle(pParam);
        break;
    /* 配置 */
    case NET_CMD_TYPE::NET_CONFIG_SERVER:
        // pThis->print_config_params(pParam);
        pThis->config_handle(pParam);
        break;
    /* 修改MAC */
    case NET_CMD_TYPE::NET_MODIFY_MAC:
        /* code */
        break;
    default:
        break;
    }

    return NULL;
}

void CIpcMulticastServer::message_response(void *pParam)
{
    UserRecv_S *pRecvParam = (UserRecv_S *)pParam;
    COMM_BUFFER *pRecvBuffer = reinterpret_cast<COMM_BUFFER *>(pRecvParam->data);

    /* 返回头信息 */
    pRecvBuffer->commHead.nFlag = HH_MAGIC_FLAG;
    pRecvBuffer->commHead.nLogonID = 0;
    pRecvBuffer->commHead.nPriority = 0;
    pRecvBuffer->commHead.nMisc = MULTICAST_SERVICE_TYPE::MULTICAST_SERVER;
    pRecvBuffer->commHead.nErrorCode = ERR_CODE::ERR_SUCCESS;
    // pRecvBuffer->commHead.nCommand == NET_CMD_TYPE::NET_SEARCH_SERVER;

    switch(pRecvBuffer->commHead.nCommand)
    {
    case NET_CMD_TYPE::NET_SEARCH_SERVER:
        pRecvBuffer->commHead.nBufSize = sizeof(SEARCH_SER_INFO);
        break;
    case NET_CMD_TYPE::NET_CONFIG_SERVER:
        pRecvBuffer->commHead.nBufSize = sizeof(ACCEPT_CONFIG) + sizeof(NETIP_CFG);
        break;
    }

    /* 消息回复发送两个包 */
    /* 1. 发送单播响应给请求的客户端 */
    m_pHandle->stunque_addr.sin_addr.s_addr = inet_addr(pRecvParam->stNetwork.ip);
    os_networkunque_send((const char *)pRecvBuffer, pRecvBuffer->commHead.nBufSize + sizeof(COMM_HEAD), m_pHandle);
    /* 2. 发送广播响应 */
    os_networkbroadcast_send((const char *)pRecvBuffer, pRecvBuffer->commHead.nBufSize + sizeof(COMM_HEAD), m_pHandle);
}

/***
 * @description : 搜索处理
 * @author      : huangjunda
 * @param        {void} *pParam
 * @return       {*}
 */
void CIpcMulticastServer::search_handle(void *pParam)
{
    UserRecv_S *pRecvParam = (UserRecv_S *)pParam;
    COMM_BUFFER *pRecvBuffer = reinterpret_cast<COMM_BUFFER *>(pRecvParam->data);

    /* 获取当前时间点 */
    struct timespec stuTime = {0};
    clock_gettime(CLOCK_MONOTONIC, &stuTime);

    /* 初始化搜索信息 */
    SEARCH_SER_INFO stSearchInfo;
    memset(&stSearchInfo, 0, sizeof(SEARCH_SER_INFO));

    /* 获取设备信息 */
    System::DeviceInfo_S stDeviceInfo;
    SystemManage::instance()->get_device_info(stDeviceInfo);

    /* 获取网络信息 */
    Network::Info_S stNetworkInfo;
    CNetworkManage::instance()->get_system_networkInfo(stNetworkInfo);

    /* 获取端口信息 */
    Network::PortConfig_S stPortConfig;
    CNetworkManage::instance()->get_network_port(stPortConfig);

    /* 返回搜索信息 */
    char achVersion[LENGTH32] = {0};
    stSearchInfo.nDeviceType = 0x01;
    stSearchInfo.nDeviceId = stDeviceInfo.deviceID;
    strncpy(stSearchInfo.szDevName, stDeviceInfo.deviceName.c_str(), LENGTH64);
    stSearchInfo.nChannelNum = 1;
    snprintf(achVersion, LENGTH32, "V%s", stDeviceInfo.systemVersion.c_str());
    strncpy(stSearchInfo.szSoftVer, achVersion, LENGTH32);
    memset(achVersion, 0, sizeof(achVersion));
    snprintf(achVersion, LENGTH32, "V%s", stDeviceInfo.pluginVersion.c_str());
    strncpy(stSearchInfo.szWebPageVer, achVersion, LENGTH32);
    stSearchInfo.nRunTime = stuTime.tv_sec - m_nStartTime;
    strncpy(stSearchInfo.szSerial, stDeviceInfo.serialNumber.c_str(), LENGTH64);
    strncpy(stSearchInfo.szModel, stDeviceInfo.strUnitTpye.c_str(), LENGTH64);
    stSearchInfo.bEncrypt = '1';
    // strncpy(stSearchInfo.szRes3, stDeviceInfo.strUnitTpye.c_str(), sizeof(stSearchInfo.szRes3));
    // strncpy(stSearchInfo.szRes1, stDeviceInfo.strUnitTpye.c_str(), sizeof(stSearchInfo.szRes1));
    /* 网络信息 */
    stSearchInfo.ipLocal = inet_addr(stNetworkInfo.stIp.ipv4Ip.c_str());
    /* 处理MAC地址 */
    sscanf(stNetworkInfo.stIp.physicalAddress.c_str(), "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
           &stSearchInfo.macAddr[0], &stSearchInfo.macAddr[1],
           &stSearchInfo.macAddr[2], &stSearchInfo.macAddr[3],
           &stSearchInfo.macAddr[4], &stSearchInfo.macAddr[5]);
    stSearchInfo.wPortWeb = stPortConfig.nHttpPort;
    stSearchInfo.wPortListen = 8800;
    stSearchInfo.ipSubMask = inet_addr(stNetworkInfo.stIp.ipv4Mask.c_str());
    stSearchInfo.ipGateway = inet_addr(stNetworkInfo.stIp.ipv4Gateway.c_str());
    stSearchInfo.ipMultiAddr = htonl(INADDR_ANY);
    stSearchInfo.ipDnsAddr = inet_addr(stNetworkInfo.stDns.main.c_str());
    stSearchInfo.s4GStrength = -1;
    stSearchInfo.wMultiPort = 0;
    // strncpy(stSearchInfo.szRes2, stDeviceInfo.strUnitTpye.c_str(), sizeof(stSearchInfo.szRes2));

    /* 填充回复内容 */
    memcpy(pRecvBuffer->commBuf, &stSearchInfo, sizeof(stSearchInfo));
    message_response(pParam);
}

/***
 * @description : 配置处理
 * @author      : huangjunda
 * @param        {void} *pParam
 * @return       {*}
 */
void CIpcMulticastServer::config_handle(void *pParam)
{
    UserRecv_S *pRecvParam = (UserRecv_S *)pParam;
    COMM_BUFFER *pRecvBuffer = reinterpret_cast<COMM_BUFFER *>(pRecvParam->data);

    /* 获取用户信息 */
    User::UserInfo_S stUserInfo;
    stUserInfo.stAccountInfo.account = "admin";
    CUserManage::instance()->get_itemInfo(stUserInfo);
    
    /* 获取本地网络配置 */
    Network::Info_S stNetInfo;
    CNetworkManage::instance()->get_system_networkInfo(stNetInfo);

    /* 许可配置 */
    ACCEPT_CONFIG stAcceptConfig;
    memcpy(&stAcceptConfig, pRecvBuffer->commBuf, sizeof(ACCEPT_CONFIG));

    /* 用MAC地址判断客户端是否配置的是本设备 */
    char achMac[18] = {0};
    snprintf(achMac, sizeof(achMac), "%02X:%02X:%02X:%02X:%02X:%02X",
            stAcceptConfig.byMac[0], stAcceptConfig.byMac[1],
            stAcceptConfig.byMac[2], stAcceptConfig.byMac[3],
            stAcceptConfig.byMac[4], stAcceptConfig.byMac[5]);
    /* 转换成 std::string 再比较，忽略大小写 */
    std::string strRecvMac = achMac;
    std::string strLocalMac = stNetInfo.stIp.physicalAddress;
    /* 全部转大写（或小写），保证格式一致 */
    std::transform(strRecvMac.begin(), strRecvMac.end(), strRecvMac.begin(), ::toupper);
    std::transform(strLocalMac.begin(), strLocalMac.end(), strLocalMac.begin(), ::toupper);
    if (strRecvMac != strLocalMac)
    {
        // dlog_debug("不是本机MAC，不需要配置信息");
        return;
    }

    /* 鉴权 */
    encrypt_string(stAcceptConfig.szUserName);
    encrypt_string(stAcceptConfig.szUserPassword);
    if (0 != stUserInfo.stAccountInfo.account.compare(stAcceptConfig.szUserName))
    {
        dlog_warn("多播鉴权用户名错误");
        return;
    }
    if (0 != stUserInfo.stAccountInfo.account.compare(stAcceptConfig.szUserName))
    {
        dlog_warn("多播鉴权密码错误");
        return;
    }
    dlog_debug("多播鉴权成功");

    // dlog_info("stAcceptConfig.nCommand: %d", stAcceptConfig.nCommand);
    switch (stAcceptConfig.nCommand)
    {
    /* 配置网络 */
    case MULTI_COMM::MULTI_COMM_NETIP:
        config_network_handle(pParam);
        break;
    /* 配置MAC */
    case MULTI_COMM::MULTI_COMM_MAC:
        /* code */
        break;
    /* 默认配置 */
    case MULTI_COMM::MULTI_COMM_DEFAULT:
        /* code */
        break;
    /* 配置序列号 */
    case MULTI_COMM::MULTI_COMM_ID:
        /* code */
        break;
    /* 恢复出厂配置 */
    case MULTI_COMM::MULTI_COMM_RESTORE_FACTORY:
        config_reset_handle(pParam);
        break;
    /* 重启 */
    case MULTI_COMM::MULTI_COMM_REBOOT:
        config_reboot_handle(pParam);
        break;
    /* 云台配置 */
    case MULTI_COMM::MULTI_COMM_PTZ:
        /* code */
        break;
    default:
        break;
    }
}

/***
 * @description : 配置网络处理
 * @author      : huangjunda
 * @param        {void} *pParam
 * @return       {*}
 */
void CIpcMulticastServer::config_network_handle(void *pParam)
{
    UserRecv_S *pRecvParam = (UserRecv_S *)pParam;
    COMM_BUFFER *pRecvBuffer = reinterpret_cast<COMM_BUFFER *>(pRecvParam->data);

    /* 网络IP配置 */
    NETIP_CFG stNetIpConfig;
    memcpy(&stNetIpConfig, pRecvBuffer->commBuf + sizeof(ACCEPT_CONFIG), sizeof(NETIP_CFG));

    /* 获取本地网络配置 */
    Network::Info_S stNetInfo;
    CNetworkManage::instance()->get_system_networkInfo(stNetInfo);

    if (stNetIpConfig.dhcp)
    {
        stNetInfo.stIp.bEnableDhcp = true;
    }
    else
    {
        struct in_addr addr;
        stNetInfo.stIp.bEnableDhcp = false;
        addr.s_addr = htonl(stNetIpConfig.nIpAddr);
        stNetInfo.stIp.ipv4Ip = inet_ntoa(addr);
        addr.s_addr = htonl(stNetIpConfig.nGateway);
        stNetInfo.stIp.ipv4Gateway = inet_ntoa(addr);
        addr.s_addr = htonl(stNetIpConfig.nSubmask);
        stNetInfo.stIp.ipv4Mask = inet_ntoa(addr);
    }
    multicast_write_log(pRecvParam->stNetwork.ip, Log::Type_E::OPERATION, Log::Action_E::REMOTE_CONFIG);
    message_response(pParam);
    CNetworkManage::instance()->set_system_networkInfo(stNetInfo);
}

/***
 * @description : 配置恢复出厂设置处理
 * @author      : huangjunda
 * @param        {void} *pParam
 * @return       {*}
 */
void CIpcMulticastServer::config_reset_handle(void *pParam)
{
    UserRecv_S *pRecvParam = (UserRecv_S *)pParam;
    multicast_write_log(pRecvParam->stNetwork.ip, Log::Type_E::OPERATION, Log::Action_E::REMOTE_FACTORY_RESET);
    message_response(pParam);
    SystemManage::instance()->system_reset_complete([](int nRet) {});
}

/***
 * @description : 配置重启处理
 * @author      : huangjunda
 * @param        {void} *pParam
 * @return       {*}
 */
void CIpcMulticastServer::config_reboot_handle(void *pParam)
{
    UserRecv_S *pRecvParam = (UserRecv_S *)pParam;
    multicast_write_log(pRecvParam->stNetwork.ip, Log::Type_E::OPERATION, Log::Action_E::REMOTE_REBOOT);
    message_response(pParam);
    SystemManage::instance()->system_reboot([](int nRet) {});
}

void CIpcMulticastServer::multicast_write_log(char *pIp, int nType, int nAction)
{
    Log::Info_S stLogInfo;
    stLogInfo.user = "multicast";
    stLogInfo.nType = nType;
    stLogInfo.nAction = nAction;
    stLogInfo.host = pIp;
    LogHandler::instance()->write(stLogInfo);
    dlog_debug("%s:%s", stLogInfo.user.c_str(), to_string((Log::Action_E)stLogInfo.nAction).c_str());
}

/***
 * @description : 打印搜索参数
 * @author      : huangjunda
 * @param        {void*} pParam
 * @return       {*}
 */
void CIpcMulticastServer::print_search_params(void *pParam)
{
    UserRecv_S *pRecvParam = (UserRecv_S *)pParam;
    COMM_BUFFER *pRecvBuffer = reinterpret_cast<COMM_BUFFER *>(pRecvParam->data);
    dlog_info("pRecvParam->code: %d", pRecvParam->code);
    dlog_info("pRecvParam->data: %s", pRecvParam->data);
    dlog_info("pRecvParam->lenght: %d", pRecvParam->lenght);
    dlog_info("pRecvParam->user: %s", pRecvParam->user);
    dlog_info("pRecvParam->stNetwork.ip: %s", pRecvParam->stNetwork.ip);
    dlog_info("pRecvParam->stNetwork.port: %d", pRecvParam->stNetwork.port);
    dlog_info("pRecvParam->stDstNet.ip: %s", pRecvParam->stDstNet.ip);
    dlog_info("pRecvParam->stDstNet.port: %d\n", pRecvParam->stDstNet.port);

    dlog_info("pRecvBuffer->commHead.commBuf: %s", pRecvBuffer->commBuf);
    dlog_info("pRecvBuffer->commHead.nFlag: %d", pRecvBuffer->commHead.nFlag);
    dlog_info("pRecvBuffer->commHead.nCommand: %d", pRecvBuffer->commHead.nCommand);
    dlog_info("pRecvBuffer->commHead.nLogonID: %d", pRecvBuffer->commHead.nLogonID);
    dlog_info("pRecvBuffer->commHead.nPriority: %d", pRecvBuffer->commHead.nPriority);
    dlog_info("pRecvBuffer->commHead.nMisc: %d", pRecvBuffer->commHead.nMisc);
    dlog_info("pRecvBuffer->commHead.nErrorCode: %d", pRecvBuffer->commHead.nErrorCode);
    dlog_info("pRecvBuffer->commHead.nBufSize: %d\n", pRecvBuffer->commHead.nBufSize);

    SEARCH_SER_INFO *stSearchInfo = (SEARCH_SER_INFO *)pRecvBuffer->commBuf;
    dlog_info("stSearchInfo->nDeviceType: %d", stSearchInfo->nDeviceType);
    dlog_info("stSearchInfo->nDeviceId: %d", stSearchInfo->nDeviceId);
    dlog_info("stSearchInfo->szDevName: %s", stSearchInfo->szDevName);
    dlog_info("stSearchInfo->nChannelNum: %d", stSearchInfo->nChannelNum);
    dlog_info("stSearchInfo->szSoftVer: %s", stSearchInfo->szSoftVer);
    dlog_info("stSearchInfo->szWebPageVer: %s", stSearchInfo->szWebPageVer);
    dlog_info("stSearchInfo->nRunTime: %d", stSearchInfo->nRunTime);
    dlog_info("stSearchInfo->szSerial: %s", stSearchInfo->szSerial);
    dlog_info("stSearchInfo->szModel: %s", stSearchInfo->szModel);
    dlog_info("stSearchInfo->bEncrypt: %c", stSearchInfo->bEncrypt);
    dlog_info("stSearchInfo->szRes3: %s", stSearchInfo->szRes3);
    dlog_info("stSearchInfo->szRes1: %s\n", stSearchInfo->szRes1);

    /* IP地址字段转换为可读格式 */
    struct in_addr addr;
    addr.s_addr = stSearchInfo->ipLocal;
    dlog_info("stSearchInfo->ipLocal: %s", inet_ntoa(addr));
    /* MAC地址格式化为字符串 */
    char achMac[18];
    snprintf(achMac, sizeof(achMac), "%02X:%02X:%02X:%02X:%02X:%02X",
            stSearchInfo->macAddr[0], stSearchInfo->macAddr[1],
            stSearchInfo->macAddr[2], stSearchInfo->macAddr[3],
            stSearchInfo->macAddr[4], stSearchInfo->macAddr[5]);
    dlog_info("stSearchInfo->macAddr: %s", achMac);
    dlog_info("stSearchInfo->wPortWeb: %d", stSearchInfo->wPortWeb);
    dlog_info("stSearchInfo->wPortListen: %d", stSearchInfo->wPortListen);
    /* 子网掩码转换 */
    addr.s_addr = stSearchInfo->ipSubMask;
    dlog_info("stSearchInfo->ipSubMask: %s", inet_ntoa(addr));
    /* 网关转换 */
    addr.s_addr = stSearchInfo->ipGateway;
    dlog_info("stSearchInfo->ipGateway: %s", inet_ntoa(addr));
    /* 组播地址转换 */
    addr.s_addr = stSearchInfo->ipMultiAddr;
    dlog_info("stSearchInfo->ipMultiAddr: %s", inet_ntoa(addr));
    /* DNS地址转换 */
    addr.s_addr = stSearchInfo->ipDnsAddr;
    dlog_info("stSearchInfo->ipDnsAddr: %s", inet_ntoa(addr));
    dlog_info("stSearchInfo->wMultiPort: %d", stSearchInfo->wMultiPort);
    dlog_info("stSearchInfo->s4GStrength: %d", stSearchInfo->s4GStrength);
    dlog_info("stSearchInfo->szRes2: %s\n", stSearchInfo->szRes2);
}

/***
 * @description : 打印配置参数
 * @author      : huangjunda
 * @param        {void*} pParam
 * @return       {*}
 */
void CIpcMulticastServer::print_config_params(void *pParam)
{
    UserRecv_S *pRecvParam = (UserRecv_S *)pParam;
    COMM_BUFFER *pRecvBuffer = reinterpret_cast<COMM_BUFFER *>(pRecvParam->data);
    dlog_info("pRecvParam->code: %d", pRecvParam->code);
    dlog_info("pRecvParam->data: %s", pRecvParam->data);
    dlog_info("pRecvParam->lenght: %d", pRecvParam->lenght);
    dlog_info("pRecvParam->user: %s", pRecvParam->user);
    dlog_info("pRecvParam->stNetwork.ip: %s", pRecvParam->stNetwork.ip);
    dlog_info("pRecvParam->stNetwork.port: %d", pRecvParam->stNetwork.port);
    dlog_info("pRecvParam->stDstNet.ip: %s", pRecvParam->stDstNet.ip);
    dlog_info("pRecvParam->stDstNet.port: %d\n", pRecvParam->stDstNet.port);

    dlog_info("pRecvBuffer->commHead.commBuf: %s", pRecvBuffer->commBuf);
    dlog_info("pRecvBuffer->commHead.nFlag: %d", pRecvBuffer->commHead.nFlag);
    dlog_info("pRecvBuffer->commHead.nCommand: %d", pRecvBuffer->commHead.nCommand);
    dlog_info("pRecvBuffer->commHead.nLogonID: %d", pRecvBuffer->commHead.nLogonID);
    dlog_info("pRecvBuffer->commHead.nPriority: %d", pRecvBuffer->commHead.nPriority);
    dlog_info("pRecvBuffer->commHead.nMisc: %d", pRecvBuffer->commHead.nMisc);
    dlog_info("pRecvBuffer->commHead.nErrorCode: %d", pRecvBuffer->commHead.nErrorCode);
    dlog_info("pRecvBuffer->commHead.nBufSize: %d\n", pRecvBuffer->commHead.nBufSize);

    /* 打印许可配置 */
    ACCEPT_CONFIG stAcceptConfig;
    memcpy(&stAcceptConfig, pRecvBuffer->commBuf, sizeof(ACCEPT_CONFIG));
    encrypt_string(stAcceptConfig.szUserName);
    encrypt_string(stAcceptConfig.szUserPassword);
    dlog_info("stAcceptConfig.szUserName: %s", stAcceptConfig.szUserName);
    dlog_info("stAcceptConfig.szUserPassword: %s", stAcceptConfig.szUserPassword);
    dlog_info("stAcceptConfig.nCommand: %d", stAcceptConfig.nCommand);
    /* MAC地址格式化为字符串 */
    char achMac[18];
    snprintf(achMac, sizeof(achMac), "%02X:%02X:%02X:%02X:%02X:%02X",
            stAcceptConfig.byMac[0], stAcceptConfig.byMac[1],
            stAcceptConfig.byMac[2], stAcceptConfig.byMac[3],
            stAcceptConfig.byMac[4], stAcceptConfig.byMac[5]);
    dlog_info("stAcceptConfig.byMac: %s", achMac);
    dlog_info("stAcceptConfig.res: %s", stAcceptConfig.res);

    /* 打印网络IP配置 */
    NETIP_CFG stNetIpConfig;
    memcpy(&stNetIpConfig, pRecvBuffer->commBuf + sizeof(ACCEPT_CONFIG), sizeof(NETIP_CFG));
    dlog_info("stNetIpConfig.dhcp: %d", stNetIpConfig.dhcp);
    /* IP地址字段转换为可读格式 */
    struct in_addr addr;
    addr.s_addr = htonl(stNetIpConfig.nIpAddr);
    dlog_info("stNetIpConfig.nIpAddr: %s", inet_ntoa(addr));
    /* 网关转换 */
    addr.s_addr = htonl(stNetIpConfig.nGateway);
    dlog_info("stNetIpConfig.nGateway: %s", inet_ntoa(addr));
    /* 子网掩码转换 */
    addr.s_addr = htonl(stNetIpConfig.nSubmask);
    dlog_info("stNetIpConfig.nSubmask: %s", inet_ntoa(addr));
    dlog_info("stNetIpConfig.res: %s", stNetIpConfig.res);
}

/***
 * @description : 加密字符串
 * @author      : huangjunda
 * @param        {char} *achText
 * @return       {int}
 */
int CIpcMulticastServer::encrypt_string(char *achText)
{
    const char *pMask;
    char       achChar;
    int        nChar;

    pMask = XOR_ENCRYPT;
    nChar = 0;

    while (*achText)
    {
        achChar = *achText ^ *pMask;

        /* 不要生成包含换行符或制表符的加密文本 */
        // if (achChar && !iswspace(achChar)) 
        if (achChar) 
        {
            *achText = achChar;
        }

        /* 将所有指针递增 */
        pMask++;
        achText++;
        nChar++;

        /* 如果指针位于长度末尾，则对其进行加密掩码 */
        if (*pMask == '\0') {
            pMask = XOR_ENCRYPT;
        }
    }

    return nChar;
}