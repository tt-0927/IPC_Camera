/***
 * @FilePath     : burn_mac_udp_server.cpp
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2025-01-20 09:22:51
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-28 09:26:04
 * @Description  :
 */

#include "burn_mac_udp_server.h"
#include "dlog.h"
#include "common_define.h"
#include "action_code.h"
#include "network_manage.h"
#include "system_manage.h"
#include "network_convert.h"
#include "convert_interface.h"
#include "web_server.h"
#include "Json.h"

#include <iostream>
#include <fstream>
#include <sstream>

IpcRet_E CBurnMacUdpServer::init()
{
    int nRet = 0;

    /* 获取网卡信息 */
    ::Network::Info_S stInfo;
    CNetworkManage::instance()->get_system_networkInfo(stInfo);

    /* Mac地址合规性检测 */
    m_bMacValid = CNetworkManage::instance()->check_mac_valid();

    dlog_debug("开始初始化烧录UDP服务器");
    dlog_debug("初始化烧录网卡Mac地址通信：%s IP：%s 状态：%d", stInfo.stIp.netName.c_str(), stInfo.stIp.ipv4Ip.c_str(), nRet);

    m_stuBurnMacHandle = (NetworkMulticast_S *)malloc(sizeof(NetworkMulticast_S));
    memset(m_stuBurnMacHandle, 0, sizeof(NetworkMulticast_S));
    m_stuBurnMacHandle->entype = UNICAST;
    m_stuBurnMacHandle->stsrc_ip_info.port = OUT_BURN_MAC_PORT;
    strcpy(m_stuBurnMacHandle->stsrc_ip_info.ip, stInfo.stIp.ipv4Ip.c_str());
    m_stuBurnMacHandle->fnhandledata = &CBurnMacUdpServer::recvData;
    m_stuBurnMacHandle->nasync_deal = 1;
    m_stuBurnMacHandle->stuser_recv_handle.user = this;

    nRet = os_networkunque_init(m_stuBurnMacHandle);
    if (0 != nRet)
    {
        dlog_debug("初始化烧录服务器失败");
        return ERR;
    }

    return OK;
}

IpcRet_E CBurnMacUdpServer::deinit()
{
    os_networkunque_exit(m_stuBurnMacHandle);
    return OK;
}

void CBurnMacUdpServer::getMacValid(bool &bMacValid)
{
    bMacValid = m_bMacValid;
}

std::vector<std::string> CBurnMacUdpServer::split(const std::string &str, const std::string &delimiter)
{
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = str.find(delimiter);

    while (end != std::string::npos)
    {
        tokens.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }

    // 添加最后一个子串
    tokens.push_back(str.substr(start));
    return tokens;
}

std::string CBurnMacUdpServer::trimmed(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t\n\r\0", 0);
    if (first == std::string::npos)
        return "";

    size_t last = str.find_last_not_of(" \t\n\r\0");
    return str.substr(first, last - first + 1);
}

void CBurnMacUdpServer::fill_common(std::string &data)
{
    Json::Object *pJsonRoot = Json::init();
    Json::add(pJsonRoot, "ActionCode", AC_GET_CHECK_MAC_VALID);
    Json::add(pJsonRoot, "DeviceName", "RV1106");
    Json::add(pJsonRoot, "UserName", "admin");
    Json::add(pJsonRoot, "Return", 0);

    Json::Object *pJsonData = Json::init(data);
    if (pJsonData)
    {
        Json::add(pJsonRoot, "Data", pJsonData);
    }

    data = Json::to_string(pJsonRoot);
    Json::deinit(pJsonRoot);
}

bool CBurnMacUdpServer::write_macConfigureFile(const std::string &strName, const std::string &strMac)
{
    std::string strFileName;
    strFileName = MAC_FILE_PATH;

    /* 组装文本 */
    std::ostringstream oss;
    oss << "ifconfig " << strName << " hw ether " << strMac;

    std::string strResult = oss.str();

    std::fstream fs{strFileName, fs.trunc | fs.in | fs.out};
    if (!fs.is_open())
    {
        return false;
    }

    fs.write(strResult.data(), strResult.length());
    fs.flush();
    fs.close();

    return true;
}

void *CBurnMacUdpServer::recvData(void *pRecvInfo)
{
    UserRecv_S *pUserRecv = (UserRecv_S *)pRecvInfo;
    CBurnMacUdpServer *pThis = (CBurnMacUdpServer *)pUserRecv->user;

    /* 获取网卡信息 */
    ::Network::Info_S stInfo;
    CNetworkManage::instance()->get_system_networkInfo(stInfo);

    dlog_info("目标IP：%s", pUserRecv->stDstNet.ip);

    if (pThis->m_bMacValid)
    {
        dlog_debug("Mac地址已经烧录完成");
        // return NULL;
    }

    std::string strLocalIP = pThis->trimmed(stInfo.stIp.ipv4Ip);
    std::string strDstIP = pThis->trimmed(std::string(pUserRecv->stDstNet.ip, strlen(pUserRecv->stDstNet.ip)));
    if (0 == strLocalIP.compare(strDstIP))
    {
        /* 拿到对应UDP句柄 */
        NetworkMulticast_S *pHandle = NULL;
        pHandle = pThis->m_stuBurnMacHandle;

        if (pHandle == NULL)
        {
            dlog_error("烧录MAC地址：无法获取UDP Server句柄");
            return NULL;
        }

        /*查询指令*/
        if (pUserRecv->lenght == 8 &&
            pUserRecv->data[0] == 0xAA &&
            pUserRecv->data[1] == 0x00 &&
            pUserRecv->data[2] == 0x07 &&
            pUserRecv->data[5] == 0x0A &&
            pUserRecv->data[7] == 0xAD)
        {
            dlog_debug("网卡：%s 接收到查询指令", stInfo.stIp.netName.c_str());

            pThis->handleGetOpt(pHandle, &stInfo.stIp, pUserRecv->stNetwork.ip, pUserRecv->stNetwork.port);
        }
        /* 设置指令 */
        else if (pUserRecv->lenght == 14 &&
                    pUserRecv->data[0] == 0xAA &&
                    pUserRecv->data[1] == 0x00 &&
                    pUserRecv->data[2] == 0x0D &&
                    pUserRecv->data[5] == 0x0C &&
                    pUserRecv->data[13] == 0xAD)
        {
            dlog_debug("网卡：%s 接收到设置指令", stInfo.stIp.netName.c_str());

            pThis->handleSetOpt(pUserRecv, pHandle, &stInfo.stIp, pUserRecv->stNetwork.ip, pUserRecv->stNetwork.port);
        }
    }

    return NULL;
}

void CBurnMacUdpServer::handleGetOpt(NetworkMulticast_S *pHandle, ::Network::Ip_S *pstIPInfo, char *pIP, int nPort)
{
    if (pHandle == NULL || pstIPInfo == NULL || pIP == NULL || nPort <= 0)
    {
        dlog_error("传入的参数异常，无法处理");
        return;
    }

    /* 返回的数据 */
    unsigned char achData[14] = {0};

    /* 头 */
    achData[0] = 0xAA;
    achData[1] = 0x00;
    achData[2] = 0x0D;
    achData[3] = 0x00;
    achData[4] = 0x00;
    achData[5] = 0x0B;

    std::vector<std::string> vecMac = split(pstIPInfo->physicalAddress, ":");
    if (vecMac.size() < 6)
    {
        dlog_error("分割mac地址失败： %s", pstIPInfo->physicalAddress.c_str()) return;
    }

    /* 如果之前修改过，返回修改过的mac */
    if ((pstIPInfo->netName.compare(ETH0_INTERFACE) == 0) &&
        m_bMacValid && !m_strNewMac.empty())
    {
        vecMac = split(m_strNewMac, ":");
    }

    dlog_info("返回网卡：【%s】的Mac地址：【%s:%s:%s:%s:%s:%s】",
             pstIPInfo->netName.c_str(),
             vecMac[0].c_str(),
             vecMac[1].c_str(),
             vecMac[2].c_str(),
             vecMac[3].c_str(),
             vecMac[4].c_str(),
             vecMac[5].c_str());

    /* mac地址 */
    achData[6] = static_cast<uint8_t>(std::stoul(vecMac[0], nullptr, 16));
    achData[7] = static_cast<uint8_t>(std::stoul(vecMac[1], nullptr, 16));
    achData[8] = static_cast<uint8_t>(std::stoul(vecMac[2], nullptr, 16));
    achData[9] = static_cast<uint8_t>(std::stoul(vecMac[3], nullptr, 16));
    achData[10] = static_cast<uint8_t>(std::stoul(vecMac[4], nullptr, 16));
    achData[11] = static_cast<uint8_t>(std::stoul(vecMac[5], nullptr, 16));

    /* 尾 */
    achData[12] = (achData[1] + achData[2] + achData[3] + achData[4] + achData[5] +
                   achData[6] + achData[7] + achData[8] + achData[9] + achData[10] + achData[11]) &
                  0xFF;
    achData[13] = 0xAD;

    /* 发回去 */
    os_networkunque_ip_send((const char *)achData, 14, pHandle, pIP, nPort);
}

/* 设置网络结果回调 */
void setInterfaceResultCB(int nRet)
{
}

/* 重启的结果回调 */
void rebootResultCB(int nRet)
{
}

void CBurnMacUdpServer::handleSetOpt(UserRecv_S *pUserRecv, NetworkMulticast_S *pHandle, ::Network::Ip_S *pstIPInfo, char *pIP, int nPort)
{
    if (pUserRecv == NULL || pHandle == NULL || pstIPInfo == NULL || pIP == NULL || nPort <= 0)
    {
        dlog_error("传入的参数异常，无法处理");
        return;
    }

    bool bIsOk = true;

    /* 再此之前判断是否已经是我们所需要的mac地址了 */
    if (pstIPInfo->physicalAddress.find("30:3a:ba:") == std::string::npos &&
        pstIPInfo->physicalAddress.find("30:3A:BA:") == std::string::npos)
    {
        char achMac[MAC_ADDR_LEN] = {0};
        snprintf(achMac, sizeof(achMac), "%x:%x:%x:%x:%x:%x",
                 pUserRecv->data[6], pUserRecv->data[7], pUserRecv->data[8],
                 pUserRecv->data[9], pUserRecv->data[10], pUserRecv->data[11]);

        dlog_debug("网卡：%s 接收到写入mac指令，新的mac地址：%s", pstIPInfo->netName.c_str(), achMac);

        /* 判断当前发送过来的mac地址是否等于 0.0.0.0.0.0 */
        if (strcmp(achMac, "0.0.0.0.0.0") == 0)
        {
            dlog_error("无法设置MAC地址为空，传输过来的MAC值：%s", achMac);
            return;
        }

        std::string strNewMac(achMac, strlen(achMac));

        /* 开始写出mac地址到指定的文件 */
        m_bMacValid = true;
        m_strNewMac = strNewMac;

        /* 写出mac地址成功 */
        if (write_macConfigureFile(pstIPInfo->netName, strNewMac))
        {
            /* 通知Web烧录Mac地址成功 */
            ::Network::CheckMacValid_S stRetInfo;
            stRetInfo.bMacValid = m_bMacValid;
            std::string strJson = Convert::to_string(stRetInfo);
            fill_common(strJson);

            CWebServer::instance()->send(strJson.c_str(), strJson.length(), AC_GET_CHECK_MAC_VALID);
        }
        else
        {
            bIsOk = false;
        }
    }
    else
    {
        dlog_debug("当前的mac地址【%s】已经是正确的了，无需重新烧录", pstIPInfo->physicalAddress.c_str());
    }

    if (bIsOk)
    {
        /* 把物理地址设置到启动脚本中 */
        Network::Info_S stNetInfo;
        CNetworkManage::instance()->get_system_networkInfo(stNetInfo);
        stNetInfo.stIp.physicalAddress = m_strNewMac;
        if (OK == CNetworkManage::instance()->set_system_networkInfo(stNetInfo))
        {
            /* 返回数据 */
            unsigned char achData[8] = {0};
            achData[0] = 0xAA;
            achData[1] = 0x00;
            achData[2] = 0x07;
            achData[3] = 0x00;
            achData[4] = 0x00;
            achData[5] = 0xA5;
            achData[6] = achData[1] + achData[2] + achData[3] + achData[4] + achData[5];
            achData[7] = 0xAD;
            os_networkunque_ip_send((const char *)achData, 8, pHandle, pIP, nPort);
            /* 重启 */
            SystemManage::instance()->system_reboot([](int nRet) {});
        }
    }
}
