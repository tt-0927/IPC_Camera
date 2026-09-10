/**
 * @file BlTCPServer.h
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2024-10-05
 * 
 * @brief 
 */

#pragma once
#include "sdk_network.h"
#include "NetDefine.h"
#include <cstring>
#include "IpcRet.h"
#include <mutex>
#include <vector>
#include <set>

class BlTCPServer
{
public:

    BlTCPServer(Net::Param_S &stParam, Net::MessageCallback fnMessageCallback);
    ~BlTCPServer();
    IpcRet_E send(const Net::Message_S stMessage);
    void set_heartbeat(const void *pData, size_t nLength);
private:

    /**
     * @brief 回调函数，通信内容处理
     * @return [*] 成功 >= int::0   其他失败
     * @note
     */
    static int callback_dealCmd(NetCallbackMsg_t* pstMsg);

    /**
     * @brief 回调函数，网络连接状态
     * @return [*] 成功 >= int::0   其他失败
     * @note
     */
    static int callback_netStatus(Net_Status_t enStatus, Sdk_Net_Handle_t phNetHandle, void* pInParam);

    /**
     * @brief 回调函数-心跳函数
     * @return [*] 成功 >= int::0   其他失败
     * @note
     */
    static int callback_heartMsg(char* pchMessege, int nLen, Sdk_Net_Handle_t pHandle, void* pInparam, int* pnOutLen);

    /**
     * @brief 回调函数-日志函数
     * @return [*] 成功 >= int::0   其他失败
     * @note
     */
    static int callback_logMsg(const char* pchFormat, ...);

private:
    /*网络句柄*/
    Sdk_ServerNet_Handle_t m_server = nullptr;
    std::set<Sdk_Net_Handle_t> m_clientSet;
    /*初始化参数*/
    Net::Param_S m_stParam;
    Net::MessageCallback m_fnMessageCallback;
    
    std::mutex m_mutex;
    std::vector<char> m_heartbeat;
};
