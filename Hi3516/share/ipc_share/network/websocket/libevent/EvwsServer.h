/**
 * @file EvwsServer.h
 * @brief WebSocket 服务端（libevent evws 后端）
 *
 * 对外接口与 libwebsockets/LibWSServer.h 保持同名同签名（IPC_WS_BACKEND
 * 切换时 WebSocketServer 层与业务层无感）。内部以 event_base + evhttp 承载
 * WS 升级握手，evws 承载会话收发：
 * - /file-upload 路径 → 上传会话（对应 lws 的 file-upload 子协议）；
 * - 其余路径     → 消息会话（对应 lws 的 http-only 子协议）；
 * - bEnssl 时经 evhttp_set_bevcb 注入 bufferevent_openssl filter（WSS）。
 *
 * 线程模型：单个 loop 线程跑 event_base_loop(EVLOOP_ONCE)；连接集合增删只在
 * loop 线程，send() 可从任意线程调用（evws 以 BEV_OPT_THREADSAFE 创建，
 * 内部锁保护，且依赖 evthread_use_pthreads 已初始化）。
 * 应用层心跳仍由 Net::WebSocketServer 的心跳线程周期广播（与 lws 后端一致），
 * 本类不做心跳定时器。
 */

#pragma once

#include "NetDefine.h"
#include "WsUpload.h"

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/util.h>
#include <event2/ws.h>

#include <atomic>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

/* TLS 依赖 OpenSSL；头文件缺失时仅支持明文，链接期报符号缺失兜底 */
#if defined(__has_include)
#if __has_include(<openssl/ssl.h>)
#define EVWS_HAVE_OPENSSL 1
#include <openssl/ssl.h>
#endif
#endif

struct bufferevent;
struct evhttp;
struct evws_connection;

class EvwsServer
{
public:
    EvwsServer(Net::Param_S &stParam, Net::MessageCallback fnMessageCallback);
    ~EvwsServer();

    int send(const Net::Message_S stMessage);
    /**
     * @brief 断开所有客户端并关闭服务器
     */
    void disconnect();
    /**
     * @brief 设置上传文件路径
     * @param strFilePath
     */
    void set_file_upload_path(const std::string &strFilePath);
    /**
     * @brief 获取上传文件名称
     * @return std::string
     */
    std::string get_upload_filename();
    /**
     * @brief 设置Qos的Dscp
     * @param nDscp
     * @return int
     */
    static int setQosDscp(const int &nDscp);

private:
    /* 连接上下文：evws 消息/关闭回调 arg 的唯一载体 */
    typedef struct _ConnCtx_
    {
        struct evws_connection *pEvws = nullptr; /* evws 会话（关闭即置空） */
        struct EvwsServer *pServer = nullptr;    /* 归属实例（静态回调回取 this） */
        bool bUpload = false;                    /* 是否上传连接（/file-upload） */
        std::string strIp;                       /* 对端 IP */
        int nOverrunCnt = 0;                     /* 连续背压超限次数 */
        /* 引用计数：连接活跃（初始 1）与待派发队列各持一份；归零即释放，
           防止连接在派发前关闭导致队列内悬垂 */
        std::atomic<int> nRef{ 1 };
        /* 握手前从 URI 提取的 query（evws_new_session 后 request 即被释放） */
        std::map<std::string, std::string> mapQuery;
    } ConnCtx_S;

    /* 待派发消息。evws 读回调在 bufferevent 锁内执行用户回调（ws.c 的
       incref_and_lock_/decref_and_unlock_ 区间），回调链中任何 evws_send_*
       都会同线程重入非递归锁导致死锁——回调内只入队，由 on_dispatch 在
       loop 线程、无锁上下文中处理业务。 */
    typedef struct _PendingMsg_
    {
        ConnCtx_S *pCtx = nullptr;
        bool bBinary = false;
        std::string strData;
    } PendingMsg_S;

    void run();
    bool setup_http();
    void register_conn(struct evws_connection *pEvws, ConnCtx_S *pCtx);
    void emit_status(ConnCtx_S *pCtx, Net::Status_E enStatus);
    void emit_message(ConnCtx_S *pCtx, const void *pData, size_t nLen);
    int send_to_conn(ConnCtx_S *pCtx, const Net::Message_S &stMessage);
    void handle_upload_text(ConnCtx_S *pCtx, const unsigned char *pData, size_t nLen);
    void handle_upload_binary(ConnCtx_S *pCtx, const unsigned char *pData, size_t nLen);
    void push_upload_progress(ConnCtx_S *pCtx);
    static void set_conn_dscp(ConnCtx_S *pCtx);
    void retain_ctx(ConnCtx_S *pCtx);
    void release_ctx(ConnCtx_S *pCtx);
    void dispatch_pending();

#if defined(EVWS_HAVE_OPENSSL)
    SSL_CTX *create_ssl_ctx();
    static struct bufferevent *ssl_bev_cb(struct event_base *pBase, void *pArg);
#endif

    /* evhttp 路由与 evws 回调（均在 loop 线程） */
    static void on_upload_request(struct evhttp_request *pReq, void *pArg);
    static void on_message_request(struct evhttp_request *pReq, void *pArg);
    static void on_ws_msg(struct evws_connection *pConn, int nType, const unsigned char *pData, size_t nLen, void *pArg);
    static void on_ws_close(struct evws_connection *pConn, void *pArg);
    /* 待派发队列的消费者：loop 线程、无 bufferevent 锁上下文 */
    static void on_dispatch(evutil_socket_t fd, short nEvent, void *pArg);
    /* 仅用于跨线程唤醒 event_base_loop，退出 disconnect() 时置位后触发 */
    static void on_wake(evutil_socket_t fd, short nEvent, void *pArg);

private:
    /* 慢客户端输出积压上限（字节）：超过则丢弃本轮发送，连续超限断开。
       freshness > completeness：控制通道消息可丢，不可拖垮 loop。 */
    static const size_t kMaxPendingOutputBytes = 1 * 1024 * 1024;
    static const int kMaxOverrunCnt = 3;

    Net::Param_S m_stParam;
    Net::MessageCallback m_fnMessageCallback;
    CWsUpload m_wsUpload;

    struct event_base *m_pBase = nullptr;
    struct evhttp *m_pHttp = nullptr;
    struct event *m_pWakeEvent = nullptr;
    struct event *m_pDispatchEvent = nullptr;
#if defined(EVWS_HAVE_OPENSSL)
    SSL_CTX *m_pSslCtx = nullptr;
#endif

    /* 连接集合：loop 线程增删；send() 外部线程遍历，以 m_mutex 互斥 */
    std::mutex m_mutex;
    std::set<ConnCtx_S *> m_connections;

    /* 待派发消息队列：evws 回调（锁内）生产，on_dispatch（loop 线程）消费 */
    std::mutex m_pendingMutex;
    std::vector<PendingMsg_S> m_pending;

    /* 上传路径与文件名：对齐 LibWSServer 的静态语义 */
    static std::string m_filePath;
    static std::string m_imagePath;
    static std::string m_uploadfFileName;
    /* QOS_DSCP */
    static int nManageDscp;

    std::atomic<bool> m_bExit{ false };
    std::thread m_tid;
};
