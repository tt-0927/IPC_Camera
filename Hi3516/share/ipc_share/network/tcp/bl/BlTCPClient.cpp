/**
 * @file BlTCPClient.cpp
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2024-10-05
 *
 * @brief
 */
#include "BlTCPClient.h"

#include <cstdint>
#include <stdarg.h>
#include <stdio.h>
#include <string>

#include "IpcRet.h"
#include "dlog.h"


BlTCPClient::BlTCPClient(Net::Param_S& stParam, Net::MessageCallback fnMessageCallback)
    : m_stParam(stParam), m_fnMessageCallback(fnMessageCallback)
{
    InparamClientNet_t stNetClientParam;
    memset(&stNetClientParam, 0, sizeof(InparamClientNet_t));
    stNetClientParam.cmdfun            = callback_dealCmd;
    stNetClientParam.heartmsg          = callback_heartMsg;
    stNetClientParam.statusFun         = callback_netStatus;
    stNetClientParam.logFun            = callback_logMsg;
    stNetClientParam.overtime          = 1000;
    stNetClientParam.asynchronous      = 1;
    stNetClientParam.asynchronous_time = 20;
    stNetClientParam.nReconnect        = 1;
    strncpy(stNetClientParam.ip, m_stParam.stInitParam.ip.c_str(), m_stParam.stInitParam.ip.length() + 1);
    stNetClientParam.nPort = m_stParam.stInitParam.nPort;
    stNetClientParam.param = this;
    m_client               = sdkclient_init_net(stNetClientParam);
    sdk_set_heartbitCode(m_stParam.stInitParam.nHearbeatCode);
    sdk_set_heartbeatInterval(m_stParam.stInitParam.nHeartbeatInterval);

    if (m_client)
    {
        dlog_trace("通讯客户端初始化成功");
    }
    else
    {
        dlog_error("通讯客户端初始化失败");
    }
}

BlTCPClient::~BlTCPClient()
{
}

void BlTCPClient::set_heartbeat(const void* pData, size_t nLength)
{
    if (!pData)
    {
        return;
    }
    std::unique_lock<std::mutex> mtx(m_mutex);
    m_heartbeat.resize(nLength);
    memcpy(m_heartbeat.data(), pData, nLength);
}

IpcRet_E BlTCPClient::send(const Net::Message_S stMessage)
{
    int nRet = 0;

    IpcRet_E enRetCode = OK;

    /*发送命令*/
    nRet = net_send_msg(m_client, (char*)const_cast<void*>(stMessage.pData), stMessage.nDataLength, stMessage.nActionCode);

    if (nRet >= 0)
    {
        enRetCode = OK;
        // dlog_trace("发送成功, Code: [%d], Data: \n%s", stInfo.nCode, stInfo.pDate);
    }
    else
    {
        enRetCode = ERR_SEND;
        // dlog_error("发送失败, Code: [%d], Data: \n%s", stInfo.nCode, stInfo.pDate);
    }

    return enRetCode;
}

int BlTCPClient::callback_dealCmd(NetCallbackMsg_t* pstMsg)
{
    if (pstMsg == NULL || pstMsg->recvvalue == NULL || pstMsg->sOperHandle == NULL || pstMsg->InParam == NULL)
    {
        dlog_error("传入参数异常");
        return ERR_PARAM;
    }
    BlTCPClient* pParam = (BlTCPClient*)pstMsg->InParam;

    Net::Message_S stMessage;
    stMessage.nActionCode = pstMsg->Code;
    if (pstMsg->Code == SDK_NET_HEARTBIT_CMD)
    {
        stMessage.nActionCode = pParam->m_stParam.stInitParam.nHearbeatCode;
    }
    stMessage.pData       = pstMsg->recvvalue;
    stMessage.nDataLength = pstMsg->nLen;
    stMessage.pHandle     = pstMsg->sOperHandle;

    Net::UserParam_S stUserParam;
    if (pParam->m_fnMessageCallback)
    {
        pParam->m_fnMessageCallback(stMessage, stUserParam);
    }

    return 0;
}

int BlTCPClient::callback_netStatus(Net_Status_t enStatus, Sdk_Net_Handle_t phNetHandle, void* pInParam)
{
    if (pInParam == NULL)
    {
        return ERR_PARAM;
    }
    BlTCPClient* pParam  = (BlTCPClient*)pInParam;
    int          nStatus = Net::STATUS_DISCONNECT;
    switch (enStatus)
    {
        case SDK_NET_DISCONNECT:
            nStatus = Net::STATUS_DISCONNECT;
            break;
        case SDK_NET_CONNECT:
            nStatus = Net::STATUS_SUCCESS;
            break;
        case SDK_NET_ERROR:
            nStatus = Net::STATUS_ERROR;
            break;
        default:
            break;
    }
    Net::Message_S stMessage;
    stMessage.nActionCode = pParam->m_stParam.stInitParam.nStatusCode;
    stMessage.pData       = &nStatus;
    stMessage.nDataLength = sizeof(nStatus);
    stMessage.pHandle     = phNetHandle;

    Net::UserParam_S stUserParam;
    if (pParam->m_fnMessageCallback)
    {
        pParam->m_fnMessageCallback(stMessage, stUserParam);
    }
    return 0;
}

int BlTCPClient::callback_heartMsg(char* pchMessege, int nLen, Sdk_Net_Handle_t pHandle, void* pInparam, int* pnOutLen)
{
    if (pchMessege == NULL || pInparam == NULL)
    {
        dlog_error("获取心跳的参数无效");
        return ERR_PARAM;
    }
    BlTCPClient* pParam = (BlTCPClient*)pInparam;

    std::unique_lock<std::mutex> mtx(pParam->m_mutex);
    *pnOutLen = pParam->m_heartbeat.size();
    if (size_t(nLen) < pParam->m_heartbeat.size())
    {
        memcpy(pchMessege, pParam->m_heartbeat.data(), nLen);
    }
    else
    {
        memcpy(pchMessege, pParam->m_heartbeat.data(), pParam->m_heartbeat.size());
    }


    return 0;
}

/**
 * @brief 回调函数-网络库日志信息
 * @param [char] *pchFormat: 信息
 * @return [*]
 */
int BlTCPClient::callback_logMsg(const char* pchFormat, ...)
{
    char arryBuff[512] = { 0 };

    va_list args;
    va_start(args, pchFormat);
    vsnprintf(arryBuff, sizeof(arryBuff), pchFormat, args);
    va_end(args);

    dlog_info("%s", arryBuff);
    return 0;
}
