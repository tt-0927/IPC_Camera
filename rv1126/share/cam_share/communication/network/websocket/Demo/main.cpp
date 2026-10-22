/*
 * @FilePath     : main.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-09-14 17:27:49
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2023-10-07 16:59:35
 * @Description  :
 */
#include "CommonServer.h"
#include "TranspondServer.h"

int dataCallbackFunc(wss_NS::DataCallbackParam_S stInfo)
{
    if (stInfo.pData && stInfo.pUserParam)
    {
        char achData[2048] = { 0 };
        strncpy(achData, (char*)stInfo.pData, stInfo.nDataLen);
        printf("接收到：[%s]-[%s]\n", (char*)stInfo.pUserParam, achData);
    }

    return 0;
}

int statusCallbackFunc(wss_NS::StatusCallbackParam_S stInfo)
{
    if (stInfo.pUserParam)
    {
        if (stInfo.enStatus == wss_NS::WS_DISCONNECT)
        {
            printf("客户端断开连接[%s]-%p\n", (char*)stInfo.pUserParam, stInfo.pHandle);
        }
        else
        {
            printf("客户端连接[%s]-%p\n", (char*)stInfo.pUserParam, stInfo.pHandle);
        }
    }

    return 0;
}

int heartbeatCallbackFunc(wss_NS::HeartbeatCallbackParam_S* pstInfo)
{
    if (pstInfo == nullptr)
    {
        return -1;
    }

    if (pstInfo->pchData == nullptr)
    {
        return -1;
    }

    strncpy(pstInfo->pchData, "ping", 5);
    pstInfo->nOutLen = 5;
    return 0;
}

void logCallbackFunc(int nLevel, const char* pchLine)
{
    printf("日志信息: [%s] %s", wss_NS::CWSBase::to_string(nLevel).c_str(), pchLine);
    return;
}

int main(int argc, char const* argv[])
{
    wss_NS::WebSocketParams_S stInfo;
    stInfo.stWebsocketNeedParam.nPort           = 7681;
    stInfo.stWebsocketNeedParam.strProtocolName = "http-only";
    stInfo.stWebsocketNeedParam.strServerIP     = "127.0.0.1";

    stInfo.stWebsocketExParam.dataCallback   = dataCallbackFunc;
    stInfo.stWebsocketExParam.statusCallback = statusCallbackFunc;
    stInfo.stWebsocketExParam.pUser          = malloc(1024);
    strcpy((char*)stInfo.stWebsocketExParam.pUser, "123qweasd");

    // stInfo.stWebsocketExParam.nHeartbeatTime    = 3000;
    // stInfo.stWebsocketExParam.heartbeatCallback = heartbeatCallbackFunc;

    // stInfo.stWebsocketExParam.logCallback = logCallbackFunc;
    // stInfo.stWebsocketExParam.nLogLevel   = wss_NS::WebLogLevel_E::WS_LLL_ERR | wss_NS::WebLogLevel_E::WS_LLL_WARN | wss_NS::WebLogLevel_E::WS_LLL_USER | wss_NS::WebLogLevel_E::WS_LLL_INFO;

    wss_NS::CWSBase* pWs = new wss_NS::CCommonServer(stInfo);
    // wss_NS::CWSBase* pWs = new wss_NS::CTranspondServer(stInfo);

    while (1)
    {
        sleep(10);
    }

    return 0;
}
