#include "cwsclient.h"
#include <iostream>
#include <chrono>
#include <cstddef>

using namespace std;
using namespace wss_NS;

/* 重连连接间隔 */
static const uint32_t g_backoffMS[] = {1000, 2000};

/* 重连结构体 */
static const lws_retry_bo_t g_retry = {
    /* 重连间隔数组 */
    .retry_ms_table = g_backoffMS,
    /* 数组长度 */
    .retry_ms_table_count = LWS_ARRAY_SIZE(g_backoffMS),
    /* 连接次数 */
    .conceal_count = 1,
    /* 空闲几秒后强制ping */
    .secs_since_valid_ping = 3,    
    /* 空闲几秒后挂机 */
    .secs_since_valid_hangup = 8, 

    .jitter_percent = 0,
};

CWSClient::CWSClient() : CWSBase(WebSocketParams_S())
{
    m_stWebSocketParams.clear();

    reset();
}

CWSClient::~CWSClient()
{

}

int CWSClient::setParame(WSClientParame stParame)
{
    if(getRuningFlag())
    {
        return 1;
    }

    if(stParame.stParames.stWebsocketNeedParam.strServerIP.empty() ||
       stParame.stParames.stWebsocketNeedParam.strServerPath.empty() ||
       stParame.stParames.stWebsocketNeedParam.nPort == 0 ||
       stParame.stParames.stWebsocketNeedParam.strProtocolName.empty())
    {
        return 2;
    }

    m_stParame = stParame;
    m_stWebSocketParams = m_stParame.stParames;
    return 0;
}

bool CWSClient::init()
{
    if(getRuningFlag())
    {
        return false;
    }
    lwsl_user("Start init...");
    initConnectInfo();
    m_thread = std::thread(&CWSClient::run, this);
    return true;
}

void CWSClient::unInit()
{
    if(!getRuningFlag())
    {
        return;
    }
    lwsl_user("Start unInit...");
    setRuningFlag(false);
    if(m_thread.joinable())
    {
        m_thread.join();
    }
}

bool CWSClient::connect()
{
    std::unique_lock<std::mutex> lock(m_mutexConnect);
    if(getConnectStatus() == WS_DISCONNECT)
    {
        if(m_pContext)
        {
            lwsl_user("Start connecting the websocket...");

            setConnectStatus(WS_CONNECTING);
            /* 10ms开始连接 */
            lws_sul_schedule(m_pContext, 0, &m_stSul, &CWSClient::wsConnect, LWS_US_PER_SEC / 100);
            return true;
        }
    }
    return false;
}

bool CWSClient::disConnect()
{
    std::unique_lock<std::mutex> lock(m_mutexConnect);
    if(getConnectStatus() == WS_CONNECT)
    {
        if(m_pClientWsi)
        {
            lwsl_user("Start disconnected...");

            setConnectStatus(WS_CLOSEING);
            /* 通知 */
            lws_cancel_service(m_pContext);
            /* 关闭通知 */
            lws_close_reason(m_pClientWsi, LWS_CLOSE_STATUS_NORMAL, NULL, 0);
            /* 直接断开ws */
            // lws_set_timeout(m_pClientWsi, PENDING_TIMEOUT_AWAITING_PROXY_RESPONSE, LWS_TO_KILL_ASYNC);
            return true;
        }
    }
    return false;
}

bool CWSClient::isConnect()
{
    return getConnectStatus() == WS_CONNECT;
}

int CWSClient::send_data(const char* pMessage, size_t nLen, WebSocketHandle_P pHandle)
{
    return insertWriteRingBuffer(pMessage, nLen, WS_WRITE_TEXT);
}

int CWSClient::sendData(uint8_t* pData, size_t nLen, WebSocketWriteProtocol_E enSendType)
{
    return insertWriteRingBuffer(pData, nLen, enSendType);
}

void CWSClient::reset()
{
    /* 初始化成员 */
    m_pContext = NULL;
    m_pVhost = NULL;
    m_pTxRingBuffer = NULL;
    m_nTail = 0;
	m_pClientWsi = NULL;
    memset(&m_stSul, 0, sizeof(m_stSul));
}

void CWSClient::initConnectInfo()
{
    /* 协议栈 */
    m_pProtocols[0] = {
        m_stWebSocketParams.stWebsocketNeedParam.strProtocolName.c_str(),
        &CWSClient::wsEventHandleCallBack,
        0,
        m_stWebSocketParams.stWebsocketExParam.nRxBufferSize,
        m_stWebSocketParams.stWebsocketExParam.nID,
        this,
        m_stWebSocketParams.stWebsocketExParam.nTxPacketSize
    };
    m_pProtocols[1] = LWS_PROTOCOL_LIST_TERM;
    
    /* 连接参数 */
    memset(&m_stConnectInfo, 0, sizeof m_stConnectInfo); 
	m_stConnectInfo.port        = m_stWebSocketParams.stWebsocketNeedParam.nPort;
	m_stConnectInfo.address     = m_stWebSocketParams.stWebsocketNeedParam.strServerIP.c_str();
	m_stConnectInfo.path        = m_stWebSocketParams.stWebsocketNeedParam.strServerPath.c_str();
	m_stConnectInfo.host        = m_stConnectInfo.address;
	m_stConnectInfo.origin      = m_stConnectInfo.address;
	m_stConnectInfo.protocol    = m_stWebSocketParams.stWebsocketNeedParam.strProtocolName.c_str();
	m_stConnectInfo.pwsi        = &m_pClientWsi;
    m_stConnectInfo.userdata    = this;
    m_stConnectInfo.retry_and_idle_policy = &g_retry;
    m_stConnectInfo.ssl_connection = !m_stParame.bSSL ? 0 : 
                                     (LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | 
                                      LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK   | LCCSCF_ALLOW_INSECURE);
}

void CWSClient::run(CWSClient* pThis)
{
    pThis->setRuningFlag(true);

    /* 设置日志等级 */
    lws_set_log_level(pThis->m_stWebSocketParams.stWebsocketExParam.nLogLevel, 
                        pThis->m_stWebSocketParams.stWebsocketExParam.logCallback);

    /* 创建上下文 */
    struct lws_context_creation_info stInfo;
    memset(&stInfo, 0, sizeof stInfo); 
	stInfo.port = CONTEXT_PORT_NO_LISTEN;
	stInfo.protocols = pThis->m_pProtocols;
	stInfo.fd_limit_per_thread = 1 + 1 + 1;
    stInfo.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    pThis->m_pContext = lws_create_context(&stInfo);

    /* 设置连接参数的上下文 */
    pThis->m_stConnectInfo.context = pThis->m_pContext;
    /* 开始事件循环 */
    if(pThis->m_pContext)
    {
        int nRet = 0;
        while(nRet >= 0 && pThis->getRuningFlag())
        {
            nRet = lws_service(pThis->m_pContext, 0);
        }
        lws_context_destroy(pThis->m_pContext);
    }
    
    pThis->setRuningFlag(false);
}

bool CWSClient::getRuningFlag()
{
    std::shared_lock<std::shared_mutex> locker(m_mutexRuning);
    return m_bIsRuning.load();
}

void CWSClient::setRuningFlag(const bool & bRuningFlag)
{
    std::unique_lock<std::shared_mutex> locker(m_mutexRuning);
    m_bIsRuning.store(bRuningFlag);
}

WebSocketStatus_E CWSClient::getConnectStatus()
{
    std::shared_lock<std::shared_mutex> locker(m_mutexConnectStatus);
    return m_enConnectStatus.load();
}

void CWSClient::setConnectStatus(const WebSocketStatus_E &enStatus)
{
    std::unique_lock<std::shared_mutex> locker(m_mutexConnectStatus);
    m_enConnectStatus.store(enStatus);
}

int CWSClient::insertWriteRingBuffer(const char* pData, size_t nLen, WebSocketWriteProtocol_E enSendType)
{
    if(!m_pTxRingBuffer || !pData || nLen <= 0 || 
        getConnectStatus() != WS_CONNECT ||
        enSendType != WS_WRITE_TEXT ||
        enSendType != WS_WRITE_BINARY)
    {
        return 1;
    }
    std::unique_lock<std::mutex> locker(m_txRingBufferMutex);
    int nRet = (int)lws_ring_get_count_free_elements(m_pTxRingBuffer);
    if(nRet)
    {
        WSMsg stMsg;
        stMsg.enType = enSendType;
        stMsg.nLen = nLen;

        /* 一定要添加LWS_PRE头，该头并不会发送 */
        size_t nBufferSize = LWS_PRE + nLen;
        stMsg.pBuffer = (uint8_t*)malloc(nBufferSize);
        memset(stMsg.pBuffer, 0, nBufferSize);
        memcpy(stMsg.pBuffer + LWS_PRE, pData, nLen);

        nRet = (int)lws_ring_insert(m_pTxRingBuffer, &stMsg, 1);
        if (nRet != 1) {
            lwsl_err("-->lws_ring_insert: 无法添加入缓冲区");
            nRet = 2;
            deleteWSMsg(&stMsg);
        }
        else {
            nRet = 0;
            lws_cancel_service(m_pContext);
        }
        return nRet;
    }
    else
    {
        lwsl_err("-->lws_ring_get_count_free_elements: 缓冲区已满");
    }
    return 1;
}

int CWSClient::insertWriteRingBuffer(uint8_t* pData, size_t nLen, WebSocketWriteProtocol_E enSendType)
{
    if(!m_pTxRingBuffer || !pData || nLen <= 0 || 
        getConnectStatus() != WS_CONNECT ||
        enSendType < WS_WRITE_TEXT ||
        enSendType > WS_WRITE_BINARY)
    {
        return 1;
    }
    std::unique_lock<std::mutex> locker(m_txRingBufferMutex);
    int nRet = (int)lws_ring_get_count_free_elements(m_pTxRingBuffer);
    if(nRet)
    {
        WSMsg stMsg;
        stMsg.enType = enSendType;
        stMsg.nLen = nLen;

        /* 一定要添加LWS_PRE头，该头并不会发送 */
        size_t nBufferSize = LWS_PRE + nLen;
        stMsg.pBuffer = (uint8_t*)malloc(nBufferSize);
        memset(stMsg.pBuffer, 0, nBufferSize);
        memcpy(stMsg.pBuffer + LWS_PRE, pData, nLen);

        nRet = (int)lws_ring_insert(m_pTxRingBuffer, &stMsg, 1);
        if (nRet != 1) {
            lwsl_err("-->lws_ring_insert: 无法添加入缓冲区");
            nRet = 2;
            deleteWSMsg(&stMsg);
        }
        else {
            nRet = 0;
            lws_cancel_service(m_pContext);
        }
        return nRet;
    }
    else
    {
        lwsl_err("-->lws_ring_get_count_free_elements: 缓冲区已满");
    }
    return 1;
}

int CWSClient::wsEventHandleCallBack(struct lws *pWsi, enum lws_callback_reasons enReason, void *pUser, void *pIn, size_t nLen)
{
    // lwsl_user("-->wsEventHandleCallBack: %d", (int)enReason);
    /* pUser是在连接参数中设置userdata字段 */
    if(!pUser)
    {
        /* 如果拿不到那么来协议栈这里拿 */
        const struct lws_protocols * pProtocols = lws_get_protocol(pWsi);
        if(pProtocols)
        {
            pUser = pProtocols->user;
        }
    }

    if(pUser)
    {
        CWSClient *pThis = (CWSClient*)pUser;

        switch (enReason){
        /* http当连接上了就会调用协议初始化，这个时候WS还没有连接成功 */
        case LWS_CALLBACK_PROTOCOL_INIT:
            if(pThis->lwsProtocolInit(pWsi, pUser, pIn, nLen) != 0)
            {
                return 1;
            }
            break;
        /* 协议注销 */
        case LWS_CALLBACK_PROTOCOL_DESTROY:
            pThis->lwsProtocolDestroy(pWsi, pUser, pIn, nLen);
            break;
        /* 添加Http鉴权头 */
        case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
            pThis->lwsClientAppendHandShakeHeader(pWsi, pUser, pIn, nLen);
            break;
        /* 连接失败 */
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            pThis->lwsConnectError(pWsi, pUser, pIn, nLen);
		    break;
        /* 连接成功 */
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            pThis->lwsConnectEstableished(pWsi, pUser, pIn, nLen);
		    break;
        /* 断开连接 */
        case LWS_CALLBACK_CLOSED:
        case LWS_CALLBACK_CLIENT_CLOSED:
        case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
            pThis->lwsConnectClose(pWsi, pUser, pIn, nLen);
		    break;
        /* 有地方调用了 lws_cancel_service() */
        case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
        {
            int nRet = pThis->lwsWaitCancelled(pWsi, pUser, pIn, nLen);
            if(nRet != 0)
            {
                return nRet;
            }
        }
            break;
        /* 可写数据 */
        case LWS_CALLBACK_CLIENT_WRITEABLE:
        {
            int nRet = pThis->lwsWriteable(pWsi, pUser, pIn, nLen);
		    if(nRet != 0)
            {
                return nRet;
            }
        }
            break;
        /* 数据可读 */
        case LWS_CALLBACK_CLIENT_RECEIVE:
            pThis->lwsReceive(pWsi, pUser, pIn, nLen);
            break;
        default:
            break;
        }
    }
    return lws_callback_http_dummy(pWsi, enReason, pUser, pIn, nLen);
}

void CWSClient::wsConnect(struct lws_sorted_usec_list *pSul)
{
    if(pSul)
    {
        /* 根据pSul进行偏移量获取WSClient*指针 */
        CWSClient *pThis = reinterpret_cast<CWSClient*>(reinterpret_cast<char*>(pSul) - offsetof(CWSClient, m_stSul));
        if(pThis && pThis->m_pContext)
        {
            lws_client_connect_via_info(&pThis->m_stConnectInfo);
        }
    }
}

int CWSClient::lwsProtocolInit(struct lws *pWsi, void *pUser, void *pIn, size_t nLen)
{
    m_pVhost = lws_get_vhost(pWsi);

    /* 创建环形发送缓冲区 */
    {
        std::unique_lock<std::mutex> locker(m_txRingBufferMutex);
        m_pTxRingBuffer = lws_ring_create(sizeof(struct WSMsg), 8, deleteWSMsg);
    }

    if(!m_pTxRingBuffer)
    {
        return 1;
    }
    return 0;
}

int CWSClient::lwsProtocolDestroy(struct lws *pWsi, void *pUser, void *pIn, size_t nLen)
{
    /* 设置线程停止标志 */
    setRuningFlag(false);

    /* 销毁环形发送缓冲区 */
    {
        std::unique_lock<std::mutex> locker(m_txRingBufferMutex);
        if (m_pTxRingBuffer) {
            lws_ring_destroy(m_pTxRingBuffer);
        }
    }
    
    lws_sul_cancel(&m_stSul);
    return 0;
}

int CWSClient::lwsClientAppendHandShakeHeader(struct lws *pWsi, void *pUser, void *pIn, size_t nLen)
{
    if(m_stParame.bBearerAuth)
    {
        const char* key = "Authorization:";
        std::string value = "Bearer; ";
        value.append(m_stParame.strToken);

        const unsigned char* uKyey = (const unsigned char*)key;
        const unsigned char* uValue = (const unsigned char*)value.c_str();
        int valueLen = value.length();

        struct lws_process_html_args *args = (struct lws_process_html_args *)pIn;
        lws_add_http_header_by_name(pWsi, uKyey, uValue, valueLen, (unsigned char **)&args->p, (unsigned char *)args->p + args->max_len);
    }
    return 0;
}

int CWSClient::lwsConnectError(struct lws *pWsi, void *pUser, void *pIn, size_t nLen)
{
    lwsl_err("CLIENT_CONNECTION_ERROR: %s\n", pIn ? (char*)pIn : "(null)");
    setConnectStatus(WS_DISCONNECT);
    m_pClientWsi = NULL;
    return 0;
}

int CWSClient::lwsConnectEstableished(struct lws *pWsi, void *pUser, void *pIn, size_t nLen)
{
    setConnectStatus(WS_CONNECT);

    if(m_stWebSocketParams.stWebsocketExParam.statusCallback)
    {
        StatusCallbackParam_S stParam;
        stParam.clear();
        stParam.enStatus = WS_CONNECT;
        stParam.pUserParam = m_stWebSocketParams.stWebsocketExParam.pUser;
        stParam.pHandle = pWsi;
        m_stWebSocketParams.stWebsocketExParam.statusCallback(stParam);
    }
    return 0;
}

int CWSClient::lwsWaitCancelled(struct lws *pWsi, void *pUser, void *pIn, size_t nLen)
{
    int nRet = 0;
    WebSocketStatus_E enStatus = getConnectStatus();

    if(enStatus == WS_CLOSEING)
    {
        /* 非0，主动断开websocket链接 */
        nRet = -1;
    }
    else if(enStatus == WS_CONNECT && m_pClientWsi)
    {
        /* 通知可写 */
        lws_callback_on_writable(m_pClientWsi);
    }
    return nRet;
}

int CWSClient::lwsConnectClose(struct lws *pWsi, void *pUser, void *pIn, size_t nLen)
{
    setConnectStatus(WS_DISCONNECT);
    m_pClientWsi = NULL;

    /* 清空ring buffer */
    {
        std::unique_lock<std::mutex> locker(m_txRingBufferMutex);
        const struct WSMsg* pMsg = (const struct WSMsg*)lws_ring_get_element(m_pTxRingBuffer, &m_nTail);
        while(pMsg)
        {
            /* 释放元素 */
            lws_ring_consume_single_tail(m_pTxRingBuffer, &m_nTail, 1);
            /* 获取下一个元素 */
            pMsg = (const struct WSMsg*)lws_ring_get_element(m_pTxRingBuffer, &m_nTail);
        }
    }

    if(m_stWebSocketParams.stWebsocketExParam.statusCallback)
    {
        StatusCallbackParam_S stParam;
        stParam.clear();
        stParam.enStatus = WS_DISCONNECT;
        stParam.pUserParam = m_stWebSocketParams.stWebsocketExParam.pUser;
        stParam.pHandle = pWsi;
        m_stWebSocketParams.stWebsocketExParam.statusCallback(stParam);
    }
    return 0;
}

int CWSClient::lwsWriteable(struct lws *pWsi, void *pUser, void *pIn, size_t nLen)
{
    WebSocketStatus_E enStatus = getConnectStatus();
    if (enStatus == WS_CONNECT && m_pTxRingBuffer)
    {
        std::unique_lock<std::mutex> locker(m_txRingBufferMutex);
        const struct WSMsg* pMsg = (const struct WSMsg*)lws_ring_get_element(m_pTxRingBuffer, &m_nTail);
        if (pMsg)
        {
            /* 写数据 */
            int wirteSize = lws_write(pWsi, pMsg->pBuffer + LWS_PRE, pMsg->nLen, (enum lws_write_protocol)pMsg->enType);
            if (wirteSize < pMsg->nLen) 
            {
                lwsl_err("ERROR %d writing to ws socket\n", wirteSize);
            }
            else
            {
                lws_ring_consume_single_tail(m_pTxRingBuffer, &m_nTail, 1);
                /* 是否还有未发送的消息 */
                if (lws_ring_get_element(m_pTxRingBuffer, &m_nTail))
                {
                    /* 通知可写 */
                    lws_callback_on_writable(pWsi);
                }
            }
        }
    }
    else if(enStatus == WS_CLOSEING)
    {
        return -1;
    }
    return 0;
}

int CWSClient::lwsReceive(struct lws *pWsi, void *pUser, void *pIn, size_t nLen)
{
    if(m_stWebSocketParams.stWebsocketExParam.dataCallback)
    {
        DataCallbackParam_S stParam;
        stParam.clear();
        stParam.pData = pIn;
        stParam.nDataLen = nLen;
        stParam.bIsLastPacket = lws_remaining_packet_payload(pWsi) == 0;
        stParam.pUserParam = m_stWebSocketParams.stWebsocketExParam.pUser;
        stParam.pHandle = pWsi;

        m_stWebSocketParams.stWebsocketExParam.dataCallback(stParam);
    }
    return 0;
}
