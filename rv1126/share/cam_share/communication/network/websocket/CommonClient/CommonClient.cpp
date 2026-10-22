/*
 * @FilePath     : CommonClient.cpp
 * @Author       : cenqt cenqt@kfb.cn
 * @LastEditTime : 2023-10-31 16:16:26
 * @Description  : WebSocket 信息通讯客户端
 */
#include "CommonClient.h"
#include "dlog.h"
#include <functional>

using namespace wss_NS;

struct per_vhost_data__minimal
{
    struct lws_context *context;
    struct lws_vhost *vhost;
    const struct lws_protocols *protocol;
    pthread_t pthread_spam[2];

    lws_sorted_usec_list_t sul;

    pthread_mutex_t lock_ring; /* serialize access to the ring buffer */
    struct lws_ring *ring;     /* ringbuffer holding unsent messages */
    uint32_t tail;

    struct lws_client_connect_info i;
    struct lws *client_wsi;

    int counter;
    char finished;
    char established;
};

static void __minimal_destroy_message(void *_msg)
{
    struct msg *msg = (struct msg *)_msg;

    free(msg->payload);
    msg->payload = NULL;
    msg->len = 0;
}

MSG_STATUS rt_ctl = RT_IDLE;

/* 句柄 */
static struct lws_context *pstContext;
static struct lws *m_pWsi;

static int port;
static int ssl_connection = LCCSCF_ALLOW_INSECURE;
static char *server_address;

/*
 * The retry and backoff policy we want to use for our client connections
 */

static const uint32_t backoff_ms[] = {1000, 2000, 3000, 4000, 5000};

static const lws_retry_bo_t retry = {
    .retry_ms_table = backoff_ms,
    .retry_ms_table_count = LWS_ARRAY_SIZE(backoff_ms),
    .conceal_count = LWS_ARRAY_SIZE(backoff_ms),

    .secs_since_valid_ping = 3,    /* 空闲几秒后强制ping */
    .secs_since_valid_hangup = 10, /* 空闲几秒后挂机 */

    .jitter_percent = 20,
};

CCommonClient::CCommonClient(WebSocketParams_S stWebSocketParams)
    : CWSBase(stWebSocketParams)

{
    m_thread = std::thread(&CCommonClient::run, this);
}

CCommonClient::~CCommonClient()
{
    m_bHeartbeatRunning.store(false);
    m_bRunning.store(false);
    m_thread.join();
}

/*连接线程*/
void *thread_spam(void *d)
{
    printf("%s\n", __FUNCTION__);
    struct per_vhost_data__minimal *vhd = (struct per_vhost_data__minimal *)d;
    struct msg amsg;
    int len = 128, n, whoami = 0;

    for (n = 0; n < (int)LWS_ARRAY_SIZE(vhd->pthread_spam); n++)
        if (pthread_equal(pthread_self(), vhd->pthread_spam[n]))
            whoami = n + 1;

    printf("vhd->finished=%d, rt_ctl=%d, vhd->established=%d\n", vhd->finished, rt_ctl == RT_IDLE, vhd->established);
    while ((!vhd->finished) && (rt_ctl == RT_IDLE))
    {
        /*如果客户端未连接，则不生成输出 */
        if (!vhd->established)
            goto wait;

        pthread_mutex_lock(&vhd->lock_ring);

        /* 只在环形缓冲区中创建if空间 */
        n = (int)lws_ring_get_count_free_elements(vhd->ring);
        if (!n)
        {
            lwsl_user("dropping!\n");
            goto wait_unlock;
        }

        amsg.payload = malloc((unsigned int)(LWS_PRE + len));
        if (!amsg.payload)
        {
            lwsl_user("OOM: dropping\n");
            goto wait_unlock;
        }
        n = lws_snprintf((char *)amsg.payload + LWS_PRE, (unsigned int)len, "hello websocket");
        amsg.len = (unsigned int)n;
        n = (int)lws_ring_insert(vhd->ring, &amsg, 1);
        if (n != 1)
        {
            __minimal_destroy_message(&amsg);
            lwsl_user("dropping!\n");
        }
        else
        {
            lws_cancel_service(vhd->context);
        }

    wait_unlock:
        pthread_mutex_unlock(&vhd->lock_ring);

    wait:
        usleep(100000);
    }

    lwsl_notice("thread_spam %d exiting\n", whoami);
    pthread_exit(NULL);
    return NULL;
}

/* 发送信息 */
int CCommonClient::send_data(const char *pMessage, size_t nLen, WebSocketHandle_P pHandle)
{
    int nRet = 0;

    if (m_pWsi && (lws_callback_on_writable(m_pWsi) > 0))
    {
        unsigned char *buffer = (unsigned char *)malloc(LWS_SEND_BUFFER_PRE_PADDING + strlen(pMessage) + LWS_SEND_BUFFER_POST_PADDING);

        if (buffer)
        {
            /*填充消息内容*/
            memcpy(buffer + LWS_SEND_BUFFER_PRE_PADDING, pMessage, strlen(pMessage));

            /*发送消息*/
            int ret = lws_write(m_pWsi, buffer + LWS_SEND_BUFFER_PRE_PADDING, strlen(pMessage), LWS_WRITE_TEXT);
            if (ret < 0)
            {
                perror("FAIL: ");
                fprintf(stderr, "Failed to send message\n");
                nRet = -1;
            }

            free(buffer);
        }
    }

    return nRet;
}

static void sul_connect_attempt(struct lws_sorted_usec_list *sul)
{
    struct per_vhost_data__minimal *vhd =
        lws_container_of(sul, struct per_vhost_data__minimal, sul);

    vhd->i.context = vhd->context;
    vhd->i.port = port; 
    vhd->i.address = server_address;
    vhd->i.path = "/";
    vhd->i.host = vhd->i.address;
    vhd->i.origin = vhd->i.address;
    vhd->i.ssl_connection = ssl_connection;
    vhd->i.protocol = "http-only";

    vhd->i.pwsi = &vhd->client_wsi;
    vhd->i.retry_and_idle_policy = &retry;

    /*连接失败则重新尝试连接*/
    m_pWsi = lws_client_connect_via_info(&vhd->i);
    if (!m_pWsi)
    {
        lws_sul_schedule(vhd->context, 0, &vhd->sul, sul_connect_attempt, 10 * LWS_US_PER_SEC);
    }
}

/* 通讯线程 */
void CCommonClient::run()
{
    // // 断线重连延迟时间（单位：毫秒）
    // unsigned int delay = 5000;

    // // 延迟一段时间后再次尝试连接
    // usleep(delay * 1000);
    bool bIsSetHeartbeat = false;
    std::thread heartbeatThread;
    struct lws_context_creation_info info;

    const char *p;
    int n = 0;

    /* 设置日志等级 */
    lws_set_log_level(m_stWebSocketParams.stWebsocketExParam.nLogLevel, m_stWebSocketParams.stWebsocketExParam.logCallback);

    port = m_stWebSocketParams.stWebsocketNeedParam.nPort;
    server_address = (char *)malloc(strlen(m_stWebSocketParams.stWebsocketNeedParam.strServerIP.c_str()));
    memcpy(
        server_address,
        m_stWebSocketParams.stWebsocketNeedParam.strServerIP.c_str(),
        strlen(m_stWebSocketParams.stWebsocketNeedParam.strServerIP.c_str()));

    memset(&info, 0, sizeof(info));

    lwsl_user("LWS minimal ws client\n");

    struct lws_protocols protocols[] = {
        {m_stWebSocketParams.stWebsocketNeedParam.strProtocolName.c_str(),
         CCommonClient::callback,
         0,
         m_stWebSocketParams.stWebsocketExParam.nRxBufferSize,
         m_stWebSocketParams.stWebsocketExParam.nID,
         this,
         m_stWebSocketParams.stWebsocketExParam.nTxPacketSize},
        LWS_PROTOCOL_LIST_TERM,
    };

    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.fd_limit_per_thread = 1 + 1 + 1;

    pstContext = lws_create_context(&info);
    if (!pstContext)
    {
        lwsl_err("lws init failed\n");
        return;
    }

    // /* 安排一个定时任务 */
    // lws_sul_schedule(pstContext, 0, &mco.sul, connect_client, 1000);

    if (m_stWebSocketParams.stWebsocketExParam.nHeartbeatTime > 0 &&
        m_stWebSocketParams.stWebsocketExParam.heartbeatCallback != nullptr)
    {
        bIsSetHeartbeat = true;
        /* 创建心跳线程 */
        heartbeatThread = std::thread(&CCommonClient::sendHeartbeatThread, this);
    }

    while (m_bRunning.load())
    {
        // if (!pstContext)
        // {
        //     lws_service(pstContext, 1000);
        // }
        lws_service(pstContext, 1000);
    }

    if (bIsSetHeartbeat)
    {
        /* 等待线程结束 */
        heartbeatThread.join();
    }

    lws_context_destroy(pstContext);
    // lwsl_user("Completed\n");
}

/* 发送心跳线程 */
void wss_NS::CCommonClient::sendHeartbeatThread()
{
    if (m_stWebSocketParams.stWebsocketExParam.nHeartbeatTime <= 0 ||
        m_stWebSocketParams.stWebsocketExParam.heartbeatCallback == nullptr)
    {
        return;
    }

    // printf("send HeartBeat......\n");

    /* 心跳发送间隔 */
    std::chrono::milliseconds sleepDuration(m_stWebSocketParams.stWebsocketExParam.nHeartbeatTime);

    /* 心跳回调函数 */
    HeartbeatCallbackFunc heartbeatCallback = m_stWebSocketParams.stWebsocketExParam.heartbeatCallback;

    /* 初始化数据 */
    int nIinitialBufferSize = 1024;
    char *pchData = new char[nIinitialBufferSize];
    memset(pchData, 0, nIinitialBufferSize);

    while (m_bHeartbeatRunning.load())
    {
        HeartbeatCallbackParam_S stInfo;
        stInfo.nInLen = nIinitialBufferSize;
        stInfo.pchData = pchData;

        /* 调用心跳回调，获取需要发送的数据 */
        heartbeatCallback(&stInfo);

        /* 判断是否需要扩大发送空间 */
        if (stInfo.nOutLen > stInfo.nInLen)
        {
            if (stInfo.pchData)
            {
                delete[] stInfo.pchData;
                stInfo.pchData = nullptr;
                pchData = nullptr;
            }

            nIinitialBufferSize = stInfo.nOutLen;
            pchData = new char[nIinitialBufferSize];
            memset(pchData, 0, nIinitialBufferSize);

            stInfo.clear();
            // stInfo.nInLen  = nIinitialBufferSize;
            // stInfo.pchData = pchData;
            // /* 重新调用心跳回调，获取需要发送的数据 */
            // heartbeatCallback(&stInfo);
        }

        if (stInfo.nOutLen > 0)
        {
            /* 发送心跳信息 */
            this->send_data(stInfo.pchData, stInfo.nOutLen);
            // printf("send HeartBeat message: %s\n", stInfo.pchData);
        }

        std::this_thread::sleep_for(sleepDuration);
    }

    delete[] pchData;
    pchData = nullptr;
}

/* 通讯回调函数 */
int CCommonClient::callback(
    struct lws *pWsi,
    lws_callback_reasons enReason,
    void *pUser,
    void *pIn,
    size_t nLen)
{
    /* 非0，会主动断开连接 */
    int nRet = 0;
    void *retval;
    int n, m, r = 0;

    struct per_vhost_data__minimal *vhd =
        (struct per_vhost_data__minimal *)
            lws_protocol_vh_priv_get(lws_get_vhost(pWsi),
                                     lws_get_protocol(pWsi));

    const struct msg *pmsg;

// /*平台网线断开后重连操作*/
// #if 0
//     if (pInstance)
//     {
//         /* 状态回调函数 */
//         StatusCallbackFunc statusCallback;
//         statusCallback = pInstance->m_stWebSocketParams.stWebsocketExParam.statusCallback;
//         if (statusCallback)
//         {
//             StatusCallbackParam_S stParamInfo;
//             stParamInfo.enStatus   = WS_DISCONNECT;
//             stParamInfo.pHandle    = pWsi;
//             stParamInfo.pUserParam = pInstance->m_stWebSocketParams.stWebsocketExParam.pUser;

//             /* 调用回调函数 */
//             nRet = statusCallback(stParamInfo);
//         }
//     }
// #endif
    // printf("websocket连接状态:%d\n", enReason);
    switch (enReason)
    {
    /* LWS初始化一个新的协议触发 */
    case LWS_CALLBACK_PROTOCOL_INIT:
    {
        // printf("LWS_CALLBACK_PROTOCOL_INIT\n");
        vhd = (struct per_vhost_data__minimal *)lws_protocol_vh_priv_zalloc(
            lws_get_vhost(pWsi),
            lws_get_protocol(pWsi),
            sizeof(struct per_vhost_data__minimal));
        vhd->context = lws_get_context(pWsi);
        vhd->protocol = lws_get_protocol(pWsi);
        vhd->vhost = lws_get_vhost(pWsi);

        vhd->ring = lws_ring_create(sizeof(struct msg), 8, __minimal_destroy_message);
        if (!vhd->ring)
            return 1;

        pthread_mutex_init(&vhd->lock_ring, NULL);

        /*开始创建上下文的线程*/
        for (n = 0; n < (int)LWS_ARRAY_SIZE(vhd->pthread_spam); n++)
            if (pthread_create(&vhd->pthread_spam[n], NULL, thread_spam, vhd))
            {
                lwsl_err("thread creation failed\n");
                r = 1;
                goto init_fail;
            }

        sul_connect_attempt(&vhd->sul);
    }
    break;

    case LWS_CALLBACK_PROTOCOL_DESTROY:
        // printf("LWS_CALLBACK_PROTOCOL_DESTROY\n");
    init_fail:
        vhd->finished = 1;
        for (n = 0; n < (int)LWS_ARRAY_SIZE(vhd->pthread_spam); n++)
            pthread_join(vhd->pthread_spam[n], &retval);

        if (vhd->ring)
            lws_ring_destroy(vhd->ring);

        lws_sul_cancel(&vhd->sul);
        pthread_mutex_destroy(&vhd->lock_ring);

        return r;

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
    {
        // dlog_debug("LWS_CALLBACK_CLIENT_CONNECTION_ERROR\n");
        lwsl_err("CLIENT_CONNECTION_ERROR: %s\n", pIn ? (char *)pIn : "(null)");
        vhd->client_wsi = NULL;
        lws_sul_schedule(vhd->context, 0, &vhd->sul, sul_connect_attempt, LWS_US_PER_SEC);
        CCommonClient *pInstance = nullptr;
        if (vhd)
        {
            /* 获取用户自定义参数 */
            if (vhd->protocol)
            {
                pInstance = (CCommonClient *)vhd->protocol->user;
                if (vhd->protocol->name)
                {
                    char achData[265] = {0};
                    // snprintf(achData, sizeof(achData) - 1, "[%s]协议的客户端[%p]连接成功\n", vhd->protocol->name, pWsi);
                    lwsl_user(achData);
                }
            }
        }
        if (pInstance)
        {
            /* 状态回调函数 */
            StatusCallbackFunc statusCallback;
            statusCallback = pInstance->m_stWebSocketParams.stWebsocketExParam.statusCallback;
            if (statusCallback)
            {
                StatusCallbackParam_S stParamInfo;
                stParamInfo.enStatus = WS_DISCONNECT;
                stParamInfo.pHandle = pWsi;
                stParamInfo.pUserParam = pInstance->m_stWebSocketParams.stWebsocketExParam.pUser;
                /* 调用回调函数 */
                nRet = statusCallback(stParamInfo);
            }
        }

        break;
    }

    case LWS_CALLBACK_CLIENT_ESTABLISHED:
    {
        dlog_debug("websocket连接服务端成功");
        /*连接建立成功，发送消息*/
        /*请求LWS_CALLBACK_CLIENT_WRITEABLE回调*/
        lws_callback_on_writable(pWsi);

        CCommonClient *pInstance = nullptr;

        if (vhd)
        {
            /* 获取用户自定义参数 */
            if (vhd->protocol)
            {
                pInstance = (CCommonClient *)vhd->protocol->user;

                if (vhd->protocol->name)
                {
                    char achData[265] = {0};
                    // snprintf(achData, sizeof(achData) - 1, "[%s]协议的客户端[%p]连接成功\n", vhd->protocol->name, pWsi);
                    lwsl_user(achData);
                }
            }
        }

        if (pInstance)
        {
            /* 状态回调函数 */
            StatusCallbackFunc statusCallback;
            statusCallback = pInstance->m_stWebSocketParams.stWebsocketExParam.statusCallback;
            if (statusCallback)
            {
                StatusCallbackParam_S stParamInfo;
                stParamInfo.enStatus = WS_CONNECT;
                stParamInfo.pHandle = pWsi;
                stParamInfo.pUserParam = pInstance->m_stWebSocketParams.stWebsocketExParam.pUser;

                /* 调用回调函数 */
                nRet = statusCallback(stParamInfo);
            }
        }

        break;
    }

    case LWS_CALLBACK_CLIENT_WRITEABLE:
        // printf("LWS_CALLBACK_CLIENT_WRITEABLE\n");
        // lwsl_user("Send a msg to Server.\n");
        rt_ctl = RT_BUSY;
        pthread_mutex_lock(&vhd->lock_ring);
        pmsg = (const struct msg *)lws_ring_get_element(vhd->ring, &vhd->tail);
        if (!pmsg)
        {
            goto skip;
        }

        m = lws_write(m_pWsi,
                      ((unsigned char *)pmsg->payload) + LWS_PRE,
                      pmsg->len,
                      LWS_WRITE_TEXT);
        if (m < (int)pmsg->len)
        {
            pthread_mutex_unlock(&vhd->lock_ring);
            lwsl_err("ERROR %d writing to ws socket\n", m);
            return -1;
        }

        lws_ring_consume_single_tail(vhd->ring, &vhd->tail, 1);

        /* more to do for us? */
        if (lws_ring_get_element(vhd->ring, &vhd->tail))
        {
            lws_callback_on_writable(pWsi);
        }

    skip:
        pthread_mutex_unlock(&vhd->lock_ring);
        break;

    case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
        if (vhd && vhd->client_wsi && vhd->established)
            lws_callback_on_writable(vhd->client_wsi);
        break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
    {
        /*接收到服务端发送的消息*/
        // printf("Received message: %s\n", pIn ? (char *)pIn : "null");
        rt_ctl = RT_IDLE;

        char *pchRecvData = new char[nLen + 1];
        memset(pchRecvData, 0, nLen + 1);
        memcpy(pchRecvData, pIn, nLen);
        pchRecvData[nLen] = '\0';
        CCommonClient *pInstance = nullptr;

        if (vhd)
        {
            if (vhd->protocol)
            {
                pInstance = (CCommonClient *)vhd->protocol->user;
            }
        }

        if (pInstance)
        {
            DataCallbackFunc dataCallback;
            dataCallback = pInstance->m_stWebSocketParams.stWebsocketExParam.dataCallback;
            if (dataCallback)
            {
                DataCallbackParam_S stParamInfo;
                stParamInfo.pData = pchRecvData;
                stParamInfo.nDataLen = nLen;
                stParamInfo.pHandle = pWsi;
                stParamInfo.pUserParam = pInstance->m_stWebSocketParams.stWebsocketExParam.pUser;

                /* 调用回调函数 */
                nRet = dataCallback(stParamInfo);
            }
        }

        delete[] pchRecvData;
        pchRecvData = nullptr;
    }
    break;

    case LWS_CALLBACK_CLOSED:
        /* WebSocket连接关闭*/
        printf("WebSocket connection closed\n");
        break;

    case LWS_CALLBACK_CLIENT_CLOSED:
    {
        dlog_debug("websocket服务端断开连接");
        m_pWsi = NULL;
        vhd->client_wsi = NULL;
        vhd->established = 0;
        lws_sul_schedule(vhd->context, 0, &vhd->sul, sul_connect_attempt, LWS_US_PER_SEC);
        CCommonClient *pInstance = nullptr;
        if (vhd)
        {
            /* 获取用户自定义参数 */
            if (vhd->protocol)
            {
                pInstance = (CCommonClient *)vhd->protocol->user;
                if (vhd->protocol->name)
                {
                    char achData[265] = {0};
                    // snprintf(achData, sizeof(achData) - 1, "[%s]协议的客户端[%p]连接成功\n", vhd->protocol->name, pWsi);
                    lwsl_user(achData);
                }
            }
        }
        if (pInstance)
        {
            /* 状态回调函数 */
            StatusCallbackFunc statusCallback;
            statusCallback = pInstance->m_stWebSocketParams.stWebsocketExParam.statusCallback;
            if (statusCallback)
            {
                StatusCallbackParam_S stParamInfo;
                stParamInfo.enStatus = WS_DISCONNECT;
                stParamInfo.pHandle = pWsi;
                stParamInfo.pUserParam = pInstance->m_stWebSocketParams.stWebsocketExParam.pUser;
                /* 调用回调函数 */
                nRet = statusCallback(stParamInfo);
            }
        }
    }    
    break;

    default:
        break;
    }
#if 0
    /*如果应用只使用WebSocket协议，而不需要处理HTTP协议，就不需要执行下面的语句*/
	return lws_callback_http_dummy(pWsi, enReason, pUser, pIn, nLen);
#else
    return nRet;
#endif
}
