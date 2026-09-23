/**
 * @file EvwsServer.cpp
 * @brief WebSocket 服务端（libevent evws 后端），协议行为对齐 libwebsockets/LibWSServer.cpp
 */

#include "EvwsServer.h"

#include "Json.h"
#include "dlog.h"
#include "IpcRet.h"
#include "WsQuery.h"

#include <event2/bufferevent_ssl.h>
#include <event2/thread.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

std::string EvwsServer::m_filePath = "/opt/course/upload/";
std::string EvwsServer::m_imagePath = "/opt/course/face/";
std::string EvwsServer::m_uploadfFileName = "";
int EvwsServer::nManageDscp = 0;

EvwsServer::EvwsServer(Net::Param_S &stParam, Net::MessageCallback fnMessageCallback)
    : m_stParam(stParam), m_fnMessageCallback(fnMessageCallback)
{
    /* bufferevent 锁与 event_active 跨线程调用依赖 threading 支持；重复启用安全 */
    evthread_use_pthreads();

    m_pBase = event_base_new();
    if (nullptr == m_pBase)
    {
        dlog_error("event_base_new 失败");
    }

    if (!setup_http())
    {
        dlog_error("websocket服务器初始化失败 port[%d]", m_stParam.stInitParam.nPort);
    }

    m_tid = std::thread(&EvwsServer::run, this);
}

EvwsServer::~EvwsServer()
{
    dlog_info("EvwsServer析构");
    disconnect();
}

bool EvwsServer::setup_http()
{
    if (nullptr == m_pBase)
    {
        return false;
    }

    m_pHttp = evhttp_new(m_pBase);
    if (nullptr == m_pHttp)
    {
        dlog_error("evhttp_new 失败");
        return false;
    }

    m_pWakeEvent = event_new(m_pBase, -1, EV_PERSIST, EvwsServer::on_wake, this);
    if (nullptr == m_pWakeEvent)
    {
        dlog_error("wake event 创建失败");
    }
    else
    {
        event_add(m_pWakeEvent, nullptr);
    }

    /* 消息派发事件：evws 回调（bufferevent 锁内）入队后经 event_active 触发，
       在无锁上下文处理业务，规避回调链中 evws_send 的同线程锁重入死锁 */
    m_pDispatchEvent = event_new(m_pBase, -1, EV_PERSIST, EvwsServer::on_dispatch, this);
    if (nullptr == m_pDispatchEvent)
    {
        dlog_error("dispatch event 创建失败");
    }
    else
    {
        event_add(m_pDispatchEvent, nullptr);
    }

    /* 上传路径独立路由，对应 lws 的 file-upload 子协议 */
    evhttp_set_cb(m_pHttp, "/file-upload", EvwsServer::on_upload_request, this);
    /* 其余路径按消息会话处理，对应 lws 的 http-only 子协议 */
    evhttp_set_gencb(m_pHttp, EvwsServer::on_message_request, this);

#if defined(EVWS_HAVE_OPENSSL)
    if (m_stParam.stInitParam.bEnssl && !m_stParam.stInitParam.strCert.empty() && !m_stParam.stInitParam.strKey.empty())
    {
        dlog_info("websocket服务器开启ssl连接");
        m_pSslCtx = create_ssl_ctx();
        if (nullptr == m_pSslCtx)
        {
            dlog_error("SSL_CTX 创建失败，TLS 端口不可用");
        }
        else
        {
            /* 注入 SSL filter 后该 evhttp 上的全部连接均为 TLS；
               与 lws 的 ALLOW_NON_SSL_ON_SSL_PORT 不同，明文连接将被拒绝 */
            evhttp_set_bevcb(m_pHttp, EvwsServer::ssl_bev_cb, this);
        }
    }
#endif

    /* 对齐 lws 行为：LibWSServer 仅使用 port（未设置 iface），绑定所有接口；
       Param_S.ip 在旧后端即未生效，此处不按其收窄绑定范围，避免外部
       （前端直连设备 IP）连接被拒 */
    if (evhttp_bind_socket(m_pHttp, nullptr, (ev_uint16_t) m_stParam.stInitParam.nPort) < 0)
    {
        dlog_error("evhttp_bind_socket 失败 port[%d]", m_stParam.stInitParam.nPort);
        return false;
    }
    return true;
}

#if defined(EVWS_HAVE_OPENSSL)
SSL_CTX *EvwsServer::create_ssl_ctx()
{
    SSL_CTX *pCtx = SSL_CTX_new(TLS_server_method());
    if (nullptr == pCtx)
    {
        return nullptr;
    }

    /* TLS 最低协议版本：消费 NetService_S.NetServiceVersion 配置
       （0=TLS1_1 1=TLS1_2 2=TLS1_3，语义为"允许的最低版本"） */
    int nMinProto = TLS1_1_VERSION;
    switch (m_stParam.stInitParam.nTlsVersion)
    {
    case 1:
        nMinProto = TLS1_2_VERSION;
        break;
    case 2:
        nMinProto = TLS1_3_VERSION;
        break;
    default:
        nMinProto = TLS1_1_VERSION;
        break;
    }
    SSL_CTX_set_min_proto_version(pCtx, nMinProto);
    SSL_CTX_set_max_proto_version(pCtx, TLS_ANY_VERSION);

    if (SSL_CTX_use_certificate_chain_file(pCtx, m_stParam.stInitParam.strCert.c_str()) != 1 ||
        SSL_CTX_use_PrivateKey_file(pCtx, m_stParam.stInitParam.strKey.c_str(), SSL_FILETYPE_PEM) != 1 ||
        SSL_CTX_check_private_key(pCtx) != 1)
    {
        dlog_error("证书/密钥加载失败 cert[%s] key[%s]", m_stParam.stInitParam.strCert.c_str(), m_stParam.stInitParam.strKey.c_str());
        SSL_CTX_free(pCtx);
        return nullptr;
    }
    return pCtx;
}

struct bufferevent *EvwsServer::ssl_bev_cb(struct event_base *pBase, void *pArg)
{
    EvwsServer *pServer = static_cast<EvwsServer *>(pArg);
    SSL *pSsl = SSL_new(pServer->m_pSslCtx);
    if (nullptr == pSsl)
    {
        return nullptr;
    }
    struct bufferevent *pRaw = bufferevent_socket_new(pBase, -1, BEV_OPT_CLOSE_ON_FREE);
    if (nullptr == pRaw)
    {
        SSL_free(pSsl);
        return nullptr;
    }
    /* fd 由 evhttp accept 后经 bufferevent_replacefd 注入 */
    return bufferevent_openssl_filter_new(pBase, pRaw, pSsl, BUFFEREVENT_SSL_ACCEPTING, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
}
#endif /* EVWS_HAVE_OPENSSL */

void EvwsServer::run()
{
    pthread_setname_np(pthread_self(), "EvwsServerRun");

    /* EVLOOP_ONCE + 外层退出检查：不依赖 event_base_loopbreak 的跨线程语义 */
    while (!m_bExit)
    {
        if (nullptr == m_pBase)
        {
            break;
        }
        event_base_loop(m_pBase, EVLOOP_ONCE);
    }
}

void EvwsServer::on_wake(evutil_socket_t fd, short nEvent, void *pArg)
{
    (void) fd;
    (void) nEvent;
    (void) pArg;
}

void EvwsServer::set_conn_dscp(ConnCtx_S *pCtx)
{
    if (nullptr == pCtx || nullptr == pCtx->pEvws)
    {
        return;
    }
    struct bufferevent *pBev = evws_connection_get_bufferevent(pCtx->pEvws);
    if (nullptr == pBev)
    {
        return;
    }
    evutil_socket_t fd = bufferevent_getfd(pBev);
    if (fd >= 0 && nManageDscp > 0)
    {
        int nTos = nManageDscp << 2;
        setsockopt(fd, IPPROTO_IP, IP_TOS, (char *) &nTos, sizeof(nTos));
        dlog_info("设置管理DSCP,网页socket[%d] DSCP to %d (TOS: %d)", (int) fd, nManageDscp, nTos);
    }
}

void EvwsServer::on_upload_request(struct evhttp_request *pReq, void *pArg)
{
    EvwsServer *pServer = static_cast<EvwsServer *>(pArg);
    if (nullptr == pServer || nullptr == pReq)
    {
        return;
    }

    ConnCtx_S *pCtx = new ConnCtx_S();
    pCtx->pServer = pServer;
    pCtx->bUpload = true;
    /* evws_new_session 成功后 request 即被释放，query 必须在升级前提取 */
    pCtx->mapQuery = WsQuery::parse(WsQuery::extract_query(evhttp_request_get_uri(pReq)));

    struct evws_connection *pEvws = evws_new_session(pReq, EvwsServer::on_ws_msg, pCtx, BEV_OPT_THREADSAFE);
    if (nullptr == pEvws)
    {
        /* 非 WebSocket 升级请求：evws_new_session 已回 400 */
        dlog_warn("file-upload 升级失败");
        pServer->release_ctx(pCtx);
        return;
    }
    pCtx->pEvws = pEvws;
    evws_connection_set_closecb(pEvws, EvwsServer::on_ws_close, pCtx);

    /* 以下对应 lws file-upload 协议的 LWS_CALLBACK_ESTABLISHED */
#if !CAP_AI_FACE_COMPARE
    pServer->m_wsUpload.set_file_path(m_filePath);
#endif
    pServer->m_wsUpload.parse_param(pCtx, pCtx->mapQuery);

    pServer->register_conn(pEvws, pCtx);
}

void EvwsServer::on_message_request(struct evhttp_request *pReq, void *pArg)
{
    EvwsServer *pServer = static_cast<EvwsServer *>(pArg);
    if (nullptr == pServer || nullptr == pReq)
    {
        return;
    }

    ConnCtx_S *pCtx = new ConnCtx_S();
    pCtx->pServer = pServer;
    pCtx->mapQuery = WsQuery::parse(WsQuery::extract_query(evhttp_request_get_uri(pReq)));

    struct evws_connection *pEvws = evws_new_session(pReq, EvwsServer::on_ws_msg, pCtx, BEV_OPT_THREADSAFE);
    if (nullptr == pEvws)
    {
        dlog_warn("websocket 升级失败");
        pServer->release_ctx(pCtx);
        return;
    }
    pCtx->pEvws = pEvws;
    evws_connection_set_closecb(pEvws, EvwsServer::on_ws_close, pCtx);

    pServer->register_conn(pEvws, pCtx);

    /* 对应 lws http-only 协议的 LWS_CALLBACK_ESTABLISHED：通知业务层新连接 */
    pServer->emit_status(pCtx, Net::STATUS_SUCCESS);
}

void EvwsServer::register_conn(struct evws_connection *pEvws, ConnCtx_S *pCtx)
{
    pCtx->strIp.clear();
    struct bufferevent *pBev = evws_connection_get_bufferevent(pEvws);
    if (nullptr != pBev)
    {
        evutil_socket_t fd = bufferevent_getfd(pBev);
        struct sockaddr_storage ssAddr;
        socklen_t nAddrLen = sizeof(ssAddr);
        if (fd >= 0 && getpeername(fd, reinterpret_cast<struct sockaddr *>(&ssAddr), &nAddrLen) == 0)
        {
            char achIp[INET6_ADDRSTRLEN] = { 0 };
            if (ssAddr.ss_family == AF_INET)
            {
                inet_ntop(AF_INET, &reinterpret_cast<struct sockaddr_in *>(&ssAddr)->sin_addr, achIp, sizeof(achIp));
            }
            else if (ssAddr.ss_family == AF_INET6)
            {
                inet_ntop(AF_INET6, &reinterpret_cast<struct sockaddr_in6 *>(&ssAddr)->sin6_addr, achIp, sizeof(achIp));
            }
            pCtx->strIp = achIp;
        }
    }
    set_conn_dscp(pCtx);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_connections.insert(pCtx);
}

void EvwsServer::emit_status(ConnCtx_S *pCtx, Net::Status_E enStatus)
{
    int nStatus = (int) enStatus;
    Net::Message_S stMessage;
    stMessage.nActionCode = m_stParam.stInitParam.nStatusCode;
    stMessage.pData = &nStatus;
    stMessage.nDataLength = sizeof(nStatus);
    stMessage.pHandle = pCtx->pEvws;
    stMessage.ip = pCtx->strIp;
    if (m_fnMessageCallback)
        m_fnMessageCallback(stMessage, m_stParam.stUserParam);
}

void EvwsServer::emit_message(ConnCtx_S *pCtx, const void *pData, size_t nLen)
{
    /* lws 版接收侧以 '\0' 结尾上送，nDataLength 含 '\0'，此处保持一致 */
    std::string strData(static_cast<const char *>(pData), nLen);
    Net::Message_S stMessage;
    stMessage.pData = strData.c_str();
    stMessage.nDataLength = (int) strData.size() + 1;
    stMessage.pHandle = pCtx->pEvws;
    stMessage.ip = pCtx->strIp;
    if (m_fnMessageCallback)
        m_fnMessageCallback(stMessage, m_stParam.stUserParam);
}

void EvwsServer::on_ws_msg(struct evws_connection *pConn, int nType, const unsigned char *pData, size_t nLen, void *pArg)
{
    (void) pConn;
    ConnCtx_S *pCtx = static_cast<ConnCtx_S *>(pArg);
    if (nullptr == pCtx || nullptr == pCtx->pServer || nullptr == pData || 0 == nLen)
    {
        return;
    }
    EvwsServer *pServer = pCtx->pServer;

    /* 本回调运行在 bufferevent 锁内（ws.c read_cb），处理逻辑一律转投
       dispatch_pending，避免回调链中的 evws_send 同线程重入锁 */
    pServer->retain_ctx(pCtx);
    {
        PendingMsg_S stPending;
        stPending.pCtx = pCtx;
        stPending.bBinary = (WS_TEXT_FRAME != nType);
        stPending.strData.assign(reinterpret_cast<const char *>(pData), nLen);
        std::lock_guard<std::mutex> lock(pServer->m_pendingMutex);
        pServer->m_pending.push_back(std::move(stPending));
    }
    if (nullptr != pServer->m_pDispatchEvent)
    {
        event_active(pServer->m_pDispatchEvent, 0, 0);
    }
}

void EvwsServer::on_dispatch(evutil_socket_t fd, short nEvent, void *pArg)
{
    (void) fd;
    (void) nEvent;
    static_cast<EvwsServer *>(pArg)->dispatch_pending();
}

void EvwsServer::dispatch_pending()
{
    std::vector<PendingMsg_S> vecPending;
    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        vecPending.swap(m_pending);
    }

    for (PendingMsg_S &stMsg : vecPending)
    {
        ConnCtx_S *pCtx = stMsg.pCtx;
        /* 连接在派发前已关闭：closecb 会置空 pEvws，跳过业务处理 */
        if (nullptr == pCtx->pEvws)
        {
            release_ctx(pCtx);
            continue;
        }

        if (pCtx->bUpload)
        {
            if (stMsg.bBinary)
            {
                handle_upload_binary(pCtx, reinterpret_cast<const unsigned char *>(stMsg.strData.data()), stMsg.strData.size());
            }
            else
            {
                handle_upload_text(pCtx, reinterpret_cast<const unsigned char *>(stMsg.strData.data()), stMsg.strData.size());
            }
        }
        else
        {
            /* 消息会话：与传输类型无关，统一上送业务层（对齐 http-only 协议的 RECEIVE） */
            emit_message(pCtx, stMsg.strData.data(), stMsg.strData.size());
        }
        release_ctx(pCtx);
    }
}

void EvwsServer::retain_ctx(ConnCtx_S *pCtx)
{
    if (nullptr != pCtx)
    {
        pCtx->nRef.fetch_add(1, std::memory_order_relaxed);
    }
}

void EvwsServer::release_ctx(ConnCtx_S *pCtx)
{
    if (nullptr != pCtx && pCtx->nRef.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        delete pCtx;
    }
}

void EvwsServer::on_ws_close(struct evws_connection *pConn, void *pArg)
{
    ConnCtx_S *pCtx = static_cast<ConnCtx_S *>(pArg);
    if (nullptr == pCtx)
    {
        return;
    }
    /* 回调时连接已进入释放流程：先置空句柄，让待派发队列丢弃未处理消息，
       之后任何路径都不得再触碰 evws 接口 */
    pCtx->pEvws = nullptr;

    EvwsServer *pServer = pCtx->pServer;
    if (nullptr != pServer)
    {
        if (pCtx->bUpload)
        {
            pServer->m_wsUpload.erase(pCtx);
            dlog_info("上传连接断开 %p", pConn);
        }
        else
        {
            pServer->emit_status(pCtx, Net::STATUS_DISCONNECT);
            dlog_info("客户端[%p]断开连接", pConn);
        }

        {
            std::lock_guard<std::mutex> lock(pServer->m_mutex);
            pServer->m_connections.erase(pCtx);
        }
        pServer->release_ctx(pCtx);
        return;
    }
    delete pCtx;
}

void EvwsServer::handle_upload_text(ConnCtx_S *pCtx, const unsigned char *pData, size_t nLen)
{
    /* evws 已将分片累积为整条消息，无需再判断帧边界 */
    const std::string strData(reinterpret_cast<const char *>(pData), nLen);
    m_wsUpload.store_param(pCtx, strData);
    std::string param = m_wsUpload.get_param(pCtx);

#if CAP_AI_FACE_COMPARE
    /* 将人脸图片存到sd卡：按首条 JSON 的 type 字段选择目录 */
    std::string fileType;
    Json::get(param.c_str(), "type", fileType);
    dlog_info("解析到的文件类型 fileType: [%s]", fileType.c_str());
    if (fileType == "image")
    {
        m_wsUpload.set_file_path(m_imagePath);
        dlog_info("设置为图片上传路径: %s", m_imagePath.c_str());
    }
    else
    {
        m_wsUpload.set_file_path(m_filePath);
        dlog_info("设置为通用上传路径: %s", m_filePath.c_str());
    }
#endif

    m_wsUpload.del_param(pCtx);
    m_wsUpload.parse_param(pCtx, param.c_str(), param.length());
    emit_message(pCtx, param.c_str(), param.length());
    push_upload_progress(pCtx);
}

void EvwsServer::handle_upload_binary(ConnCtx_S *pCtx, const unsigned char *pData, size_t nLen)
{
    int nRet = m_wsUpload.write_data(pCtx, reinterpret_cast<const char *>(pData), nLen);
    if (nRet < 0)
    {
        dlog_error("上传数据写盘失败 conn %p", pCtx);
    }
    if (m_wsUpload.is_eof(pCtx))
    {
        m_wsUpload.merge(pCtx);
        m_uploadfFileName = m_wsUpload.get_upload_filename(pCtx);
        push_upload_progress(pCtx);
    }
}

void EvwsServer::push_upload_progress(ConnCtx_S *pCtx)
{
    const std::string strProgress = m_wsUpload.get_progressStr(pCtx);
    if (!strProgress.empty() && nullptr != pCtx->pEvws)
    {
        evws_send_text(pCtx->pEvws, strProgress.c_str());
    }
}

int EvwsServer::send(const Net::Message_S stMessage)
{
    if (nullptr == stMessage.pData || stMessage.nDataLength <= 0)
    {
        return -1;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    int nRet = 0;
    for (ConnCtx_S *pCtx : m_connections)
    {
        /* 上传连接不在消息广播范围内（对应 lws 双协议的隔离语义） */
        if (pCtx->bUpload)
        {
            continue;
        }
        if (stMessage.pHandle && pCtx->pEvws != stMessage.pHandle)
        {
            continue;
        }
        nRet = send_to_conn(pCtx, stMessage);
        if (stMessage.pHandle)
        {
            break;
        }
    }
    return nRet;
}

int EvwsServer::send_to_conn(ConnCtx_S *pCtx, const Net::Message_S &stMessage)
{
    if (nullptr == pCtx || nullptr == pCtx->pEvws)
    {
        return -1;
    }

    /* 背压检查：output 积压超限丢弃本轮发送，连续超限主动断开 */
    struct bufferevent *pBev = evws_connection_get_bufferevent(pCtx->pEvws);
    if (nullptr != pBev)
    {
        const size_t nPending = evbuffer_get_length(bufferevent_get_output(pBev));
        if (nPending > kMaxPendingOutputBytes)
        {
            ++pCtx->nOverrunCnt;
            dlog_warn("慢客户端积压 %zu 字节，丢弃发送 conn %p 次数 %d", nPending, pCtx, pCtx->nOverrunCnt);
            if (pCtx->nOverrunCnt >= kMaxOverrunCnt)
            {
                dlog_warn("连续积压超限，断开慢客户端 conn %p", pCtx);
                evws_close(pCtx->pEvws, WS_CR_NORMAL);
            }
            return -1;
        }
        pCtx->nOverrunCnt = 0;
    }

    /* 现网载荷均为 JSON 文本（TEXT 语义）；evws_send_text 以 '\0' 截断 */
    const std::string strPayload(static_cast<const char *>(stMessage.pData), stMessage.nDataLength);
    evws_send_text(pCtx->pEvws, strPayload.c_str());
    return 0;
}

void EvwsServer::disconnect()
{
    dlog_info("EvwsServer断开连接");
    m_bExit = true;

    /* 唤醒阻塞中的 loop，使线程尽快退出 */
    if (nullptr != m_pBase && nullptr != m_pWakeEvent)
    {
        event_active(m_pWakeEvent, 0, 0);
    }
    if (m_tid.joinable())
    {
        m_tid.join();
    }

    if (nullptr != m_pHttp)
    {
        /* evhttp_free 会清理残留 WS 会话并触发 closecb（DISCONNECT 通知随之发出） */
        evhttp_free(m_pHttp);
        m_pHttp = nullptr;
    }

    /* 释放尚未派发的消息（loop 已退出，队列不会再生产/消费） */
    {
        std::vector<PendingMsg_S> vecPending;
        {
            std::lock_guard<std::mutex> lock(m_pendingMutex);
            vecPending.swap(m_pending);
        }
        for (PendingMsg_S &stMsg : vecPending)
        {
            release_ctx(stMsg.pCtx);
        }
    }
#if defined(EVWS_HAVE_OPENSSL)
    if (nullptr != m_pSslCtx)
    {
        SSL_CTX_free(m_pSslCtx);
        m_pSslCtx = nullptr;
    }
#endif
    if (nullptr != m_pBase)
    {
        event_base_free(m_pBase);
        m_pBase = nullptr;
    }
}

void EvwsServer::set_file_upload_path(const std::string &strFilePath)
{
    m_filePath = strFilePath;
}

std::string EvwsServer::get_upload_filename()
{
    return m_uploadfFileName;
}

int EvwsServer::setQosDscp(const int &nDscp)
{
    const int QOS_DSCP_MIN = 0;
    const int QOS_DSCP_MAX = 63;
    if (QOS_DSCP_MIN <= nDscp && QOS_DSCP_MAX >= nDscp)
    {
        EvwsServer::nManageDscp = nDscp;
        return OK;
    }
    return ERR_PARAM;
}
