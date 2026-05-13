/*
 * @FilePath     : TranspondServer.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-09-14 15:52:12
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2023-10-16 18:33:39
 * @Description  : 转发服务器-测试用
 */
#include "TranspondServer.h"

#include <functional>


using namespace wss_NS;

CTranspondServer::CTranspondServer(WebSocketParams_S stWebSocketParams)
    : CWSBase(stWebSocketParams)

{
    m_thread = std::thread(&CTranspondServer::run, this);
}

CTranspondServer::~CTranspondServer()
{
    m_bRunning.store(false);
    m_thread.join();
}

/* 发送信息 */
int CTranspondServer::send_data(const char* pMessage, size_t nLen, WebSocketHandle_P pHandle)
{
    return -1;
}

/* 通讯线程 */
void CTranspondServer::run()
{
    /* 句柄此信息 */
    struct lws_context_creation_info stInfo;

    /* 句柄 */
    struct lws_context* pstContext = nullptr;

    struct lws_protocols protocols[] = {
        {m_stWebSocketParams.stWebsocketNeedParam.strProtocolName.c_str(),
         CTranspondServer::callback,
         sizeof(CTranspondServer::ClientInfo_S),
         m_stWebSocketParams.stWebsocketExParam.nRxBufferSize,
         m_stWebSocketParams.stWebsocketExParam.nID,
         NULL,
         m_stWebSocketParams.stWebsocketExParam.nTxPacketSize},
        LWS_PROTOCOL_LIST_TERM,
    };


    memset(&stInfo, 0, sizeof(stInfo));
    stInfo.port       = m_stWebSocketParams.stWebsocketNeedParam.nPort;
    stInfo.protocols  = protocols;
    stInfo.vhost_name = "localhost";
    stInfo.options    = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

    pstContext = lws_create_context(&stInfo);
    if (!pstContext)
    {
        printf("创建失败");
        return;
    }
    while (m_bRunning.load())
    {
        lws_service(pstContext, 1000);
    }

    lws_context_destroy(pstContext);
}

/* 创建数据空间 */
void CTranspondServer::create_message(void* pMsg, int nLen)
{
    if (nullptr == pMsg || nLen <= 0)
    {
        return;
    }
    CTranspondServer::MsgInfo_S* pstMsgInfo = (CTranspondServer::MsgInfo_S*)pMsg;

    pstMsgInfo->nLen  = nLen;
    pstMsgInfo->pData = (char*)malloc(LWS_PRE + nLen);
    memset(pstMsgInfo->pData, 0, nLen);
}

/* 释放数据 */
void CTranspondServer::release_message(void* pMsg)
{
    if (nullptr == pMsg)
    {
        return;
    }
    CTranspondServer::MsgInfo_S* pstMsgInfo = (CTranspondServer::MsgInfo_S*)pMsg;

    free(pstMsgInfo->pData);
    pstMsgInfo->pData = NULL;
    pstMsgInfo->nLen  = 0;
}

/* 通讯回调函数 */
int CTranspondServer::callback(
    lws*                 pWsi,
    lws_callback_reasons enReason,
    void*                pUser,
    void*                pIn,
    size_t               nLen)
{

    CTranspondServer::ClientInfo_S*      pClientInfo    = (CTranspondServer::ClientInfo_S*)pUser;
    CTranspondServer::VhostHandleInfo_S* pVhostDataInfo = (CTranspondServer::VhostHandleInfo_S*)lws_protocol_vh_priv_get(lws_get_vhost(pWsi), lws_get_protocol(pWsi));

    switch (enReason)
    {
        /* LWS初始化一个新的协议触发 */
        case LWS_CALLBACK_PROTOCOL_INIT:
        {
            /* 创建该协议的句柄数据 */
            pVhostDataInfo            = (CTranspondServer::VhostHandleInfo_S*)lws_protocol_vh_priv_zalloc(lws_get_vhost(pWsi), lws_get_protocol(pWsi), sizeof(CTranspondServer::VhostHandleInfo_S));
            pVhostDataInfo->pContext  = lws_get_context(pWsi);
            pVhostDataInfo->pProtocol = lws_get_protocol(pWsi);
            pVhostDataInfo->pVhost    = lws_get_vhost(pWsi);

            pVhostDataInfo->clientDataSet.clear();
            pVhostDataInfo->nCurrent = 0;

            printf("协议初始化\n");
            break;
        }
        /* WebSocket连接成功建立后触发 */
        case LWS_CALLBACK_ESTABLISHED:
        {
            if (pVhostDataInfo && pClientInfo)
            {
                /* 添加到该协议的句柄列表 */
                pClientInfo->pWsi  = pWsi;
                pClientInfo->nLast = pVhostDataInfo->nCurrent;

                pVhostDataInfo->clientDataSet.insert(pClientInfo);
            }
            printf("客户端连接\n");

            break;
        }
        /* WebSocket连接关闭时触发 */
        case LWS_CALLBACK_CLOSED:
        {
            if (pVhostDataInfo)
            {
                pVhostDataInfo->clientDataSet.erase(pClientInfo);
            }
            printf("客户端断开连接\n");
        }

        /* WebSocket连接可以写入数据时触发 */
        case LWS_CALLBACK_SERVER_WRITEABLE:
        {

            if (!pVhostDataInfo || !pClientInfo)
            {
                break;
            }

            if (!pVhostDataInfo->stMsgInfo.pData)
            {
                break;
            }

            if (pClientInfo->nLast == pVhostDataInfo->nCurrent)
            {
                break;
            }

            printf("向客户端发送数据\n");
            /* 注意，我们已经在有效负载中允许了LWS_PRE */
            int nWrite = lws_write(pWsi, ((unsigned char*)pVhostDataInfo->stMsgInfo.pData) + LWS_PRE, pVhostDataInfo->stMsgInfo.nLen, LWS_WRITE_TEXT);
            if (nWrite < (int)pVhostDataInfo->stMsgInfo.nLen)
            {
                printf("向客户端发送数据-失败\n");
                /* 写入失败 */
                return -1;
            }

            pClientInfo->nLast = pVhostDataInfo->nCurrent;
            break;
        }
        /* WebSocket连接接收到数据时触发 */
        case LWS_CALLBACK_RECEIVE:
        {
            if (!pVhostDataInfo || !pClientInfo)
            {
                break;
            }

            if (pVhostDataInfo->stMsgInfo.pData)
            {
                /* 释放上一次的空间 */
                release_message(&pVhostDataInfo->stMsgInfo);
            }

            /* 开始创建空间 */
            create_message(&(pVhostDataInfo->stMsgInfo), nLen);
            if (!pVhostDataInfo->stMsgInfo.pData)
            {
                /* 创建失败 */
                break;
            }

            memcpy((char*)pVhostDataInfo->stMsgInfo.pData + LWS_PRE, pIn, nLen);
            pVhostDataInfo->nCurrent++;

            printf("接收到数据 %s\n", (char*)pVhostDataInfo->stMsgInfo.pData + LWS_PRE);

            /* 遍历与当前虚拟主机关联的WebSocket会话列表 */
            for (const auto& item : pVhostDataInfo->clientDataSet)
            {
                if (item)
                {
                    lws_callback_on_writable(item->pWsi);
                }
            }

            break;
        }
        default:
        {
            break;
        }
    }
    return 0;
}
