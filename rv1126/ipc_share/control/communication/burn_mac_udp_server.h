/***
 * @FilePath     : burn_mac_udp_server.h
 * @Author       : xiezhh
 * @Date         : 2025-03-28 09:22:50
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-28 09:24:40
 * @Description  : 用于初始化设备MAC地址的UDP Server
 */

#pragma once

#include "task_manage.h"
#include "Singleton.h"
#include "IpcRet.h"
#include "IOBase.h"
#include "network_define.h"
#include "os_network_multicast.h"

class CBurnMacUdpServer : public CSingleton<CBurnMacUdpServer>
{
    CBurnMacUdpServer() = default;

public:
    ~CBurnMacUdpServer() = default;

    friend class CSingleton<CBurnMacUdpServer>;

    IpcRet_E init();
    IpcRet_E deinit();

    void getMacValid(bool &bMacValid);

private:
    std::vector<std::string> split(const std::string &str, const std::string &delimiter);
    std::string trimmed(const std::string &str);
    void fill_common(std::string &data);
    bool write_macConfigureFile(const std::string &strName, const std::string &strMac);

    static void *recvData(void *pRecvInfo);

    void handleGetOpt(NetworkMulticast_S *pHandle, ::Network::Ip_S *pstIPInfo, char *pIP, int nPort);
    void handleSetOpt(UserRecv_S *pUserRecv, NetworkMulticast_S *pHandle, ::Network::Ip_S *pstIPInfo, char *pIP, int nPort);

    /* UDP Server 句柄 */
    NetworkMulticast_S *m_stuBurnMacHandle;
    /* 是否已经烧录 */
    bool m_bMacValid = false;
    /* 暂存烧录后的mac地址 */
    std::string m_strNewMac;
};
