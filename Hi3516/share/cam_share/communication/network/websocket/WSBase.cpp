/*
 * @FilePath     : WSBase.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-10-26 21:07:47
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2023-10-27 09:50:44
 * @Description  : libwebsockets库基类
 */
#include "WSBase.h"

#include <libwebsockets.h>

using namespace wss_NS;

wss_NS::CWSBase::CWSBase(WebSocketParams_S stWebSocketParams)
    : m_stWebSocketParams(stWebSocketParams)
{
}

wss_NS::CWSBase::~CWSBase()
{
}

std::string wss_NS::CWSBase::to_string(int nLevel)
{
    std::string strLevel;
    if (nLevel & WS_LLL_ERR)
    {
        strLevel += "WS_LLL_ERR|";
    }
    if (nLevel & WS_LLL_WARN)
    {
        strLevel += "WS_LLL_WARN|";
    }
    if (nLevel & WS_LLL_NOTICE)
    {
        strLevel += "WS_LLL_NOTICE|";
    }
    if (nLevel & WS_LLL_INFO)
    {
        strLevel += "WS_LLL_INFO|";
    }
    if (nLevel & WS_LLL_DEBUG)
    {
        strLevel += "WS_LLL_DEBUG|";
    }
    if (nLevel & WS_LLL_PARSER)
    {
        strLevel += "WS_LLL_PARSER|";
    }
    if (nLevel & WS_LLL_HEADER)
    {
        strLevel += "WS_LLL_HEADER|";
    }
    if (nLevel & WS_LLL_EXT)
    {
        strLevel += "WS_LLL_EXT|";
    }
    if (nLevel & WS_LLL_CLIENT)
    {
        strLevel += "WS_LLL_CLIENT|";
    }
    if (nLevel & WS_LLL_LATENCY)
    {
        strLevel += "WS_LLL_LATENCY|";
    }
    if (nLevel & WS_LLL_USER)
    {
        strLevel += "WS_LLL_USER|";
    }
    if (nLevel & WS_LLL_THREAD)
    {
        strLevel += "WS_LLL_THREAD|";
    }

    if (!strLevel.empty() && strLevel.back() == '|')
    {
        /* 删除最后一个字符 */
        strLevel.erase(strLevel.size() - 1);
    }

    return strLevel;
}

/* 设置断开连接的原因 */
int wss_NS::CWSBase::set_closeReason(WebSocketHandle_P pHandle, WebCloseStatus_E enCloseStatus, char* pchReasonBuf, size_t nReasonBufLen)
{
    struct lws* pWsi = (struct lws*)pHandle;

    enum lws_close_status enStatus = (enum lws_close_status)enCloseStatus;

    if (pWsi)
    {
        lws_close_reason(pWsi, enStatus, (unsigned char *)pchReasonBuf, nReasonBufLen);
        return 0;
    }

    return -1;
}

/* 获取客户端URL中的参数信息 */
int wss_NS::CWSBase::get_urlArgByName(WebSocketHandle_P pHandle, const char* pchName, char* pchParameterBuf, int nParameterBufLen)
{
    struct lws* pWsi = (struct lws*)pHandle;

    if (nullptr == pWsi ||
        nullptr == pchName ||
        nullptr == pchParameterBuf ||
        0 >= nParameterBufLen)
    {
        return -1;
    }

    /* 获取URL中的参数 */
    if (lws_get_urlarg_by_name(pWsi, pchName, pchParameterBuf, nParameterBufLen))
    {
        return 0;
    }
    else
    {
        return -1;
    }
}
