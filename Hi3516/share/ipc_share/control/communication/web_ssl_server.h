/**
 * @file web_ssl_server.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-01-14
 * 
 * @brief 
 */

#pragma once

#include "task_manage.h"
#include "Singleton.h"
#include "IpcRet.h"
#include "WebSocketServer.h"
#include "IOBase.h"


class CWebSslServer : public CSingleton<CWebSslServer>
{
    CWebSslServer() = default;
public:
    ~CWebSslServer() = default;
    friend class CSingleton<CWebSslServer>;

    /**
     * @brief 通过证书和私钥初始化ssl服务端
     * @param strCert 证书
     * @param strKey  私钥
     * @return int 
     */
    int init(std::string strCert,std::string strKey);
    void deinit();
    void set_taskManage(std::shared_ptr<CTaskManage> pTaskManage);
    int send(const void *pData, int nDataLen, int nActionCode, void *pHandle = nullptr);
    void set_heartbeat(const void *pData, size_t nLength);
    void deal_heartbeat(Net::Message_S& stMessage, Net::UserParam_S &stUserParam);
    void deal_status(Net::Message_S& stMessage, Net::UserParam_S &stUserParam);
    void deal_message(Net::Message_S& stMessage, Net::UserParam_S &stUserParam);

    /**
     * @brief 获取登录设备客户端的ip
     * @return std::string 客户端IP
     */
    std::string get_loginclient_ip();

    /**
     * @brief 服务器是否初始化
     * @return true 已经初始化
     * @return false 未初始化
     */
    bool is_init();
private:
    std::shared_ptr<Net::WebSocketServer> m_pHandler = nullptr;
    std::shared_ptr<CTaskManage> m_pTaskManage = nullptr;
    std::string m_heartbeat;

    /* 登录设备IP */
    std::string m_LoginDeviceIp;
    /* 是否初始化 */
    bool m_EnInit = false;
};
