/***
 * @FilePath     : operation_client.h
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2025-05-14 10:02:22
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-07-17 10:03:04
 * @Description  : 运维客户端
 */

#pragma once

#include "task_manage.h"
#include "Singleton.h"
#include "IpcRet.h"
#include "TCPClient.h"
#include "IOBase.h"
#include "common_define.h"

class COperationClient : public CSingleton<COperationClient>
{
    COperationClient() = default;

public:
    ~COperationClient() = default;
    friend class CSingleton<COperationClient>;

    IpcRet_E init();
    IpcRet_E deinit();
    void set_taskManage(std::shared_ptr<CTaskManage> pTaskManage);
    int send(const void *pData, int nDataLen, int nActionCode, void *pHdndler = nullptr);
    void deal_heartbeat(Net::Message_S &stMessage, Net::UserParam_S &stUserParam);
    void deal_status(Net::Message_S &stMessage, Net::UserParam_S &stUserParam);
    void deal_message(Net::Message_S &stMessage, Net::UserParam_S &stUserParam);
    void fill_head(std::string &data, int nActionCode);
    int send_withHead(std::string data, int nActionCode, void *pHdndler = nullptr);
    void set_statusObserver(Common::StatusCallback observer);

private:
    std::shared_ptr<Net::IOBase> m_pHandler = nullptr;
    std::shared_ptr<CTaskManage> m_pTaskManage = nullptr;
    std::string m_heartbeat;
    Common::StatusCallback m_statusObserver;
};
