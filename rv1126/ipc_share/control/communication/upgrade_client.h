/**
 * @file upgrade_client.h
 * @author lih (lih@kfb.cn)
 * @date 2025-02-17
 * 
 * @brief 
 */

#pragma once


#include "task_manage.h"
#include "Singleton.h"
#include "IpcRet.h"
#include "TCPClient.h"
#include "IOBase.h"
#include "common_define.h"


class CUpgradeClient : public CSingleton<CUpgradeClient>
{
    CUpgradeClient() = default;
public:
    ~CUpgradeClient() = default;
    friend class CSingleton<CUpgradeClient>;

    IpcRet_E init();
    IpcRet_E deinit();
    void set_taskManage(std::shared_ptr<CTaskManage> pTaskManage);
    int send(std::string data, int nActionCode, void *pHdndler = nullptr);
    int send(const void *pData, int nDataLen, int nActionCode, void *pHdndler = nullptr);
    void set_heartbeat(const void *pData, size_t nLength);
    void deal_heartbeat(Net::Message_S& stMessage, Net::UserParam_S &stUserParam);
    void deal_status(Net::Message_S& stMessage, Net::UserParam_S &stUserParam);
    void deal_message(Net::Message_S& stMessage, Net::UserParam_S &stUserParam);
    void fill_head(std::string &strData, int nActionCode);
    bool get_connect();
    void set_statusObserver(Common::StatusCallback observer);
private:
    std::shared_ptr<Net::IOBase> m_pHandler = nullptr;
    std::shared_ptr<CTaskManage> m_pTaskManage = nullptr;
    std::string m_heartbeat;
    bool bEnConnect = false;
    Common::StatusCallback m_statusObserver;
};