/*** 
 * @FilePath     : operation_communicate.h
 * @Author       : huangjunda
 * @Date         : 2025-07-15 15:41:11
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-07-17 09:54:59
 * @Description  : 运维进程通讯
 */

#pragma once

#include "Singleton.h"
#include "IpcRet.h"
#include "IOBase.h"

#include "MaintenanceData.h"
#include "thread/CheckFileThread.h"
#include "thread/ReqNormalThread.h"
#include "thread/UploadThread.h"

#include "mqtt_communicate.h"

class OperationServer : public CSingleton<OperationServer>
{
    OperationServer() = default;

public:
    ~OperationServer() = default;
    friend class CSingleton<OperationServer>;

    int init();
    void deinit();

    int send(std::string data, int nActionCode, void *pHdndler = nullptr);
    int send(const void *pData, int nDataLen, int nActionCode, void *pHdndler = nullptr);
    int send_withHead(std::string data, int nActionCode, void *pHdndler = nullptr);

    std::string get_data(std::string &jsonData);
    void deal_heartbeat(Net::Message_S &stMessage, Net::UserParam_S &stUserParam);
    void deal_status(Net::Message_S &stMessage, Net::UserParam_S &stUserParam);
    void deal_message(Net::Message_S &stMessage, Net::UserParam_S &stUserParam);
    void fill_head(std::string &data, int nActionCode);
    void fill_returnHead(std::string &data, int nActionCode, int nReturn);
    /**
     * @brief 手动请求登录
     */
    void login_maintemamce_communicate();

private:
    void init_maintenace();

private:
    std::shared_ptr<Net::IOBase> m_pHandler = nullptr;
    std::string m_heartbeat;
    /* Mqtt句柄 */
    BlMqtt_S m_pstMqtt;
    /* 检查文件线程 */
    CCheckFileThread m_checkFileThread;
    /* 请求线程 */
    CReqNormalThread m_reqNormalThread;
    /* 文件上传线程 */
    CUploadThread m_uploadThread;
    /* 最新升级包id */
    int m_upgradeId = -1;
    /* 最新升级包下载链接 */
    std::string m_newPackurl;
};
