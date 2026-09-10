/**
 * @file BlTCPServer.cpp
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2024-10-05
 *
 * @brief
 */
#include "BlTCPServer.h"

#include <cstdint>
#include <stdarg.h>
#include <stdio.h>
#include <string>

#include "IpcRet.h"
#include "dlog.h"


BlTCPServer::BlTCPServer(Net::Param_S& stParam, Net::MessageCallback fnMessageCallback)
    : m_stParam(stParam), m_fnMessageCallback(fnMessageCallback)
{
    InparamServerNet stNetServerParm;

    memset(&stNetServerParm, 0, sizeof(InparamServerNet_t));

    stNetServerParm.cmdfun            = callback_dealCmd;
    stNetServerParm.heartmsg          = callback_heartMsg;
    stNetServerParm.statusFun         = callback_netStatus;
    stNetServerParm.logFun            = callback_logMsg;
    stNetServerParm.overtime          = 1000;
    stNetServerParm.asynchronous      = 1;
    stNetServerParm.asynchronous_time = 20;
    stNetServerParm.nPort             = m_stParam.stInitParam.nPort;
    stNetServerParm.support_ipv6      = 0;
    stNetServerParm.param             = this;
    m_server                          = sdkserver_init_net(stNetServerParm);
    sdk_set_heartbitCode(m_stParam.stInitParam.nHearbeatCode);
    sdk_set_heartbeatInterval(m_stParam.stInitParam.nHeartbeatInterval);
    if (m_server)
    {
        dlog_trace("通讯服务器初始化成功");
    }
    else
    {
        dlog_error("通讯服务器初始化失败");
    }
}

BlTCPServer::~BlTCPServer()
{
}

void BlTCPServer::set_heartbeat(const void* pData, size_t nLength)
{
    if (!pData)
    {
        return;
    }
    std::unique_lock<std::mutex> mtx(m_mutex);
    m_heartbeat.resize(nLength);
    memcpy(m_heartbeat.data(), pData, nLength);
}

IpcRet_E BlTCPServer::send(const Net::Message_S stMessage)
{
    int nRet = 0;

    IpcRet_E enRetCode = OK;

    /*发送命令*/
    if (NULL == stMessage.pHandle)
    {
        nRet = netserver_send_allClient(
            m_server, (char*)const_cast<void*>(stMessage.pData), stMessage.nDataLength, stMessage.nActionCode);
    }
    else
    {
        if (m_clientSet.find(stMessage.pHandle) == m_clientSet.end())
        {
            return ERR;
        }

        int nStatus = sdk_get_clientStatus(m_server, stMessage.pHandle);
        if (nStatus == SDK_NET_CONNECT)
        {
            nRet = net_send_msg(stMessage.pHandle, (char*)const_cast<void*>(stMessage.pData), stMessage.nDataLength, stMessage.nActionCode);
        }
        else
        {
            nRet = -1;
        }
    }

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

int BlTCPServer::callback_dealCmd(NetCallbackMsg_t* pstMsg)
{
    if (pstMsg == NULL || pstMsg->recvvalue == NULL || pstMsg->sOperHandle == NULL || pstMsg->InParam == NULL)
    {
        dlog_error("传入参数异常");
        return ERR_PARAM;
    }
    BlTCPServer* pParam = (BlTCPServer*)pstMsg->InParam;

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

int BlTCPServer::callback_netStatus(Net_Status_t enStatus, Sdk_Net_Handle_t phNetHandle, void* pInParam)
{
    if (pInParam == NULL)
    {
        return ERR_PARAM;
    }
    BlTCPServer* pParam  = (BlTCPServer*)pInParam;
    int          nStatus = Net::STATUS_DISCONNECT;
    switch (enStatus)
    {
        case SDK_NET_DISCONNECT:
            pParam->m_clientSet.erase(phNetHandle);
            nStatus = Net::STATUS_DISCONNECT;
            break;
        case SDK_NET_CONNECT:
            pParam->m_clientSet.insert(phNetHandle);
            nStatus = Net::STATUS_SUCCESS;
            break;
        case SDK_NET_ERROR:
            pParam->m_clientSet.erase(phNetHandle);
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

int BlTCPServer::callback_heartMsg(char* pchMessege, int nLen, Sdk_Net_Handle_t pHandle, void* pInparam, int* pnOutLen)
{
    if (pchMessege == NULL || pInparam == NULL)
    {
        dlog_error("获取心跳的参数无效");
        return ERR_PARAM;
    }
    BlTCPServer*                 pParam = (BlTCPServer*)pInparam;
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
int BlTCPServer::callback_logMsg(const char* pchFormat, ...)
{
    char arryBuff[512] = { 0 };

    va_list args;
    va_start(args, pchFormat);
    vsnprintf(arryBuff, sizeof(arryBuff), pchFormat, args);
    va_end(args);

    dlog_info("%s", arryBuff);
    return 0;
}
