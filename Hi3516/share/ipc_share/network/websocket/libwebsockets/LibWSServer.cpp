/**
 * @file LibWSServer.cpp
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2024-10-08
 *
 * @brief
 */
#include "LibWSServer.h"
#include "dlog.h"
#include "IpcRet.h"

std::string LibWSServer::m_filePath = "/opt/course/upload/";
std::string LibWSServer::m_uploadfFileName = "";
int LibWSServer::nManageDscp = 0;

LibWSServer::LibWSServer(Net::Param_S &stParam, Net::MessageCallback fnMessageCallback)
    : m_stParam(stParam), m_fnMessageCallback(fnMessageCallback)
{
    struct lws_protocols protocols[] = {
        {"http-only",
         callback,
         0,
         102400,
         0,
         this,
         102400},
        {"file-upload",
         file_upload_callback,
         0,
         102400,
         0,
         this,
         102400},
        LWS_PROTOCOL_LIST_TERM,
    };
    struct lws_context_creation_info stInfo;

    memset(&stInfo, 0, sizeof(stInfo));
    stInfo.port = m_stParam.stInitParam.nPort;
    stInfo.protocols = protocols;
    stInfo.vhost_name = "localhost";
    stInfo.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;
    /* 开启ssl连接 */
    if(m_stParam.stInitParam.bEnssl && !m_stParam.stInitParam.strCert.empty() && !m_stParam.stInitParam.strKey.empty())
    {
        dlog_info("websocket服务器开启ssl连接");
        stInfo.ssl_cert_filepath = m_stParam.stInitParam.strCert.c_str();
        stInfo.ssl_private_key_filepath = m_stParam.stInitParam.strKey.c_str();
        stInfo.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        stInfo.options |= LWS_SERVER_OPTION_ALLOW_NON_SSL_ON_SSL_PORT;
        stInfo.options |= LWS_SERVER_OPTION_SKIP_SERVER_CANONICAL_NAME;
        stInfo.vhost_name = m_stParam.stInitParam.strVhost.c_str();
    }
    lws_set_log_level(WS_LLL_ERR | WS_LLL_WARN | WS_LLL_USER, nullptr);

    m_pContext = lws_create_context(&stInfo);
    if (nullptr == m_pContext)
    {
        dlog_error("创建失败");
    }
    m_tid = std::thread(&LibWSServer::run, this);
}

LibWSServer::~LibWSServer()
{
    dlog_info("LibWSServer析构");
    disconnect();
}

int LibWSServer::send(const Net::Message_S stMessage)
{
    if (stMessage.pData == nullptr || stMessage.nDataLength <= 0)
    {
        return -1;
    }
    int nRet = 0;
    std::shared_ptr<char[]> pSendData = std::shared_ptr<char[]>(new char[stMessage.nDataLength + LWS_PRE]);
    memset(pSendData.get(), 0, stMessage.nDataLength + LWS_PRE);
    memcpy(pSendData.get() + LWS_PRE, stMessage.pData, stMessage.nDataLength);

    struct lws *pWsi = (stMessage.pHandle) ? (struct lws *)stMessage.pHandle : nullptr;
    std::lock_guard<std::mutex> lock(m_mutex);
    for (VhostHandleInfo_S *pVhostDataInfo : m_connections)
    {
        if (!pVhostDataInfo)
        {
            continue;
        }
        
        for (ClientInfo_S *pClientInfo : pVhostDataInfo->clientInfos)
        {
            if (pWsi && pClientInfo->pWsi != pWsi)
            {
                continue;
            }
            
            MsgInfo_S stInfo;
            stInfo.pData = pSendData;
            stInfo.nLen = stMessage.nDataLength;
            std::lock_guard<std::mutex> lock(pClientInfo->mutex);
            pClientInfo->listMsgInfo.push_back(stInfo);
    
            /* 通知可写 */
            nRet = lws_callback_on_writable(pClientInfo->pWsi);
            if (pWsi)  // 如果是指定的 pWsi，找到后就可以跳出循环
            {
                break;
            }
        }
    }


    return nRet;
}

void LibWSServer::run()
{
    pthread_setname_np(pthread_self(), "LibWSServerRun");

    while (!m_bExit)
    {
        lws_service(m_pContext, 1000);
    }
}
/* 通讯回调函数 */
int LibWSServer::callback(
    struct lws *pWsi,
    lws_callback_reasons enReason,
    void *pUser,
    void *pIn,
    size_t nLen)
{
    /* 非0，会主动断开连接 */
    int nRet = 0;

    VhostHandleInfo_S *pVhostDataInfo = (VhostHandleInfo_S *)lws_protocol_vh_priv_get(lws_get_vhost(pWsi), lws_get_protocol(pWsi));

    switch (enReason)
    {
    case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED:
    {
        dlog_info("WebSocket服务器收到新的客户端连接请求，协议为[%s]",pVhostDataInfo->pProtocol->name);
        break;
    }
    /* LWS初始化一个新的协议触发 */
    case LWS_CALLBACK_PROTOCOL_INIT:
    {
        LibWSServer *pInstance = nullptr;

        /* 创建该协议的句柄数据 */
        pVhostDataInfo = (VhostHandleInfo_S *)lws_protocol_vh_priv_zalloc(lws_get_vhost(pWsi), lws_get_protocol(pWsi), sizeof(VhostHandleInfo_S));
        if (pVhostDataInfo)
        {
            pVhostDataInfo->pContext = lws_get_context(pWsi);
            pVhostDataInfo->pProtocol = lws_get_protocol(pWsi);
            pVhostDataInfo->pVhost = lws_get_vhost(pWsi);
            pVhostDataInfo->clientInfos.clear();

            if (pVhostDataInfo->pProtocol)
            {
                pInstance = (LibWSServer *)pVhostDataInfo->pProtocol->user;
                if (pInstance)
                {
                    std::lock_guard<std::mutex> lock(pInstance->m_mutex);
                    /* 记录通讯协议句柄 */
                    pInstance->m_connections.insert(pVhostDataInfo);
                }

                if (pVhostDataInfo->pProtocol->name)
                {
                    dlog_info("[%s]协议初始化成功\n", pVhostDataInfo->pProtocol->name);
                }
            }
        }

        break;
    }
    /* 特定协议的反初始化 */
    case LWS_CALLBACK_PROTOCOL_DESTROY:
    {
        if (!pVhostDataInfo)
        {
            break;
        }
        LibWSServer *pInstance = (LibWSServer *)pVhostDataInfo->pProtocol->user;
        if (pInstance)
        {
            std::lock_guard<std::mutex> lock(pInstance->m_mutex);
            /* 移除该协议的在线用户句柄列表 */
            auto it = pVhostDataInfo->clientInfos.begin();
            while (it != pVhostDataInfo->clientInfos.end())
            {
                const ClientInfo_S *pClientInfo = *it;

                it = pVhostDataInfo->clientInfos.erase(it);

                delete pClientInfo;
                pClientInfo = nullptr;
            }
            dlog_info("[%s]协议释放成功\n", pVhostDataInfo->pProtocol->name);
        }
        break;
    }
    /* WebSocket连接成功建立后触发 */
    case LWS_CALLBACK_ESTABLISHED:
    {
        LibWSServer *pInstance = nullptr;

        if (pVhostDataInfo)
        {
            ClientInfo_S *pClientInfo = new ClientInfo_S();
            /* 添加到该协议的在线用户句柄列表 */
            pClientInfo->pWsi = pWsi;
            pClientInfo->listMsgInfo.clear();
            pClientInfo->vectorRecvBuf.clear();
            pVhostDataInfo->clientInfos.insert(pClientInfo);
            
            /* 获取用户自定义参数 */
            if (pVhostDataInfo->pProtocol)
            {
                pInstance = (LibWSServer *)pVhostDataInfo->pProtocol->user;

                if (pVhostDataInfo->pProtocol->name)
                {
                    dlog_info("[%s]协议的客户端[%p]连接成功\n", pVhostDataInfo->pProtocol->name, pWsi);
                }
            }
        }

        if (pInstance)
        {

            int client_fd = lws_get_socket_fd(pWsi);

            int manageDscpValue = nManageDscp; 

            // 设置 DSCP
            if (client_fd >= 0) {
                 int tos = manageDscpValue << 2;
                 setsockopt(client_fd, IPPROTO_IP, IP_TOS, (char *)&tos, sizeof(tos));
                 dlog_info("设置管理DSCP,网页socket[%d] DSCP to %d (TOS: %d)\n", client_fd, manageDscpValue, tos);
            }
 
            int nStatus = Net::STATUS_SUCCESS;
            Net::Message_S stMessage;
            stMessage.nActionCode = pInstance->m_stParam.stInitParam.nStatusCode;
            stMessage.pData = &nStatus;
            stMessage.nDataLength = sizeof(nStatus);
            stMessage.pHandle = pWsi;
            stMessage.ip.resize(100);
            lws_get_peer_simple(pWsi, stMessage.ip.data(), stMessage.ip.size());
            /* 修正字符串实际长度 */
            stMessage.ip.resize(strlen(stMessage.ip.c_str()));

            if (pInstance->m_fnMessageCallback)
                pInstance->m_fnMessageCallback(stMessage, pInstance->m_stParam.stUserParam);
        }
        break;
    }
    /* WebSocket连接关闭时触发 */
    case LWS_CALLBACK_CLOSED:
    {
        LibWSServer *pInstance = nullptr;

        if (!pVhostDataInfo)
        {
            break;
        }
        pInstance = (LibWSServer *)pVhostDataInfo->pProtocol->user;
        if (pInstance)
        {
            std::lock_guard<std::mutex> lock(pInstance->m_mutex);

            /* 移除该协议的在线用户句柄列表 */
            auto it = pVhostDataInfo->clientInfos.begin();
            while (it != pVhostDataInfo->clientInfos.end())
            {
                const ClientInfo_S *pClientInfo = *it;

                if (pClientInfo->pWsi == pWsi)
                {
                    it = pVhostDataInfo->clientInfos.erase(it);
                    dlog_info("客户端[%p]断开连接\n",pWsi);

                    delete pClientInfo;
                    pClientInfo = nullptr;
                }
                else
                {
                    ++it;
                }
            }

            /* 获取用户自定义参数 */
            if (pVhostDataInfo->pProtocol)
            {
                pInstance = (LibWSServer *)pVhostDataInfo->pProtocol->user;

                if (pVhostDataInfo->pProtocol->name)
                {
                    dlog_info("[%s]协议的客户端[%p]断开连接\n", pVhostDataInfo->pProtocol->name, pWsi);
                }
            }
        }

        if (pInstance)
        {
            int nStatus = Net::STATUS_DISCONNECT;
            Net::Message_S stMessage;
            stMessage.nActionCode = pInstance->m_stParam.stInitParam.nStatusCode;
            stMessage.pData = &nStatus;
            stMessage.nDataLength = sizeof(nStatus);
            stMessage.pHandle = pWsi;
            stMessage.ip.resize(100);
            lws_get_peer_simple(pWsi, stMessage.ip.data(), stMessage.ip.size());
            /* 修正字符串实际长度 */
            stMessage.ip.resize(strlen(stMessage.ip.c_str()));
            if (pInstance->m_fnMessageCallback)
                pInstance->m_fnMessageCallback(stMessage, pInstance->m_stParam.stUserParam);
        }

        break;
    }
    /* WebSocket连接可以写入数据时触发 */
    case LWS_CALLBACK_SERVER_WRITEABLE:
    {
        if (!pVhostDataInfo)
        {
            dlog_error("pVhostDataInfo is null");
            break;
        }
        for (ClientInfo_S *pClientInfo : pVhostDataInfo->clientInfos)
        {
            if (pClientInfo->pWsi == pWsi)
            {
                std::lock_guard<std::mutex> lock(pClientInfo->mutex);
                auto it = pClientInfo->listMsgInfo.begin();
                while (it != pClientInfo->listMsgInfo.end())
                {
                    const MsgInfo_S &stMsgInfo = *it;

                    /* 不能直接传智能指针进去lws_write() */
                    char *pchSendData = new char[stMsgInfo.nLen + LWS_PRE];
                    memset(pchSendData, 0, stMsgInfo.nLen + LWS_PRE);
                    memcpy(pchSendData, stMsgInfo.pData.get(), stMsgInfo.nLen + LWS_PRE);

                    // dlog_info("[%p]发送数据[%p]：\n%s\n", pClientInfo->pWsi, stMsgInfo.pData.get(), pchSendData + LWS_PRE);

                    /* 注意，我们已经在有效负载中允许了LWS_PRE */
                    int nWrite = lws_write(pWsi, ((unsigned char *)pchSendData) + LWS_PRE, stMsgInfo.nLen, LWS_WRITE_TEXT);
                    if (nWrite < (int)stMsgInfo.nLen)
                    {
                        lwsl_err("向客户端[%p]发送数据-失败\n", pWsi);
                    }

                    /* 从容器中移除item，放在memcpy()后面 */
                    it = pClientInfo->listMsgInfo.erase(it);
                    delete[] pchSendData;
                    pchSendData = nullptr;
                }

                break;
            }
        }

        break;
    }

    /* WebSocket连接接收到数据时触发 */
    case LWS_CALLBACK_RECEIVE:
    {
        if (nullptr == pIn || nLen <= 0 || nullptr == pWsi || nullptr == pVhostDataInfo)
        {
            return nRet;
        }
        if (pVhostDataInfo)
        {
            auto it = pVhostDataInfo->clientInfos.begin();
            while (it != pVhostDataInfo->clientInfos.end())
            {
                ClientInfo_S *pClientInfo = *it;
                if (pClientInfo->pWsi == pWsi)
                {
                    /* 记录当前缓存的大小 */
                    size_t lCurrentSize = pClientInfo->vectorRecvBuf.size();
                    /* 扩展缓存以容纳新数据 --当前已有数据长度+接收的分片长度*/
                    pClientInfo->vectorRecvBuf.resize(lCurrentSize + nLen);
                    /* 将新数据复制到缓存的末尾 */
                    memcpy(pClientInfo->vectorRecvBuf.data() + lCurrentSize, pIn, nLen);
                    /* 判断是否接收完所有分片 */
                    if (lws_remaining_packet_payload(pClientInfo->pWsi) == 0)
                    {
                        /*确保字符串结束符*/
                        pClientInfo->vectorRecvBuf.push_back('\0');
                        LibWSServer *pInstance = nullptr;
                        
                        if (pVhostDataInfo)
                        {
                            if (pVhostDataInfo->pProtocol)
                            {
                                pInstance = (LibWSServer *)pVhostDataInfo->pProtocol->user;
                            }
                        }
                        
                        if (pInstance)
                        {
                        
                            Net::Message_S stMessage;
                            stMessage.pData = pClientInfo->vectorRecvBuf.data();
                            stMessage.nDataLength = pClientInfo->vectorRecvBuf.size();
                            stMessage.pHandle = pClientInfo->pWsi;
                            stMessage.ip.resize(100);
                            lws_get_peer_simple(pWsi, stMessage.ip.data(), stMessage.ip.size());
                            /* 修正字符串实际长度 */
                            stMessage.ip.resize(strlen(stMessage.ip.c_str()));
                            if (pInstance->m_fnMessageCallback)
                                pInstance->m_fnMessageCallback(stMessage, pInstance->m_stParam.stUserParam);
                        }
                        

                        /* 清空缓存，为下一次接收准备 */
                        pClientInfo->vectorRecvBuf.clear();
                        break;
                    }
                }

                ++it;
            }
        }

        break;
    }
    default:
    {
        break;
    }
    }

    return nRet;
}



int LibWSServer::file_upload_callback(
    struct lws *pWsi,
    lws_callback_reasons enReason,
    void *pUser,
    void *pIn,
    size_t nLen)
{
    int nRet = 0;
    auto lwsProtocol = lws_get_protocol(pWsi);
    if (lwsProtocol == nullptr)
    {
        dlog_error("lwsProtocol [%p].", lwsProtocol);
        return -1;
    }
    LibWSServer *pWsServer= (LibWSServer *)lwsProtocol->user;
    if (!pWsServer)
    {
        dlog_error("pWsServer [%p].", pWsServer);
        return -1;
    }
    switch (enReason) {
    /* WebSocket连接成功建立后触发 */
    case LWS_CALLBACK_ESTABLISHED: {
        // 文件上传连接建立
        dlog_info("File transfer connection established [%p].", pWsi);
        /* 设置文件上传的路径 */
        pWsServer->m_wsUpload.set_file_path(m_filePath);
        pWsServer->m_wsUpload.parse_param(pWsi);
        break;
    }
    /* WebSocket连接可以写入数据时触发 */
    case LWS_CALLBACK_SERVER_WRITEABLE:
    {
        if (nullptr == pWsi)
        {
            break;
        }
        std::string progress =  pWsServer->m_wsUpload.get_progressStr(pWsi);
        /* 不能直接传智能指针进去lws_write() */
        char *pchSendData = new char[progress.size() + LWS_PRE];
        memset(pchSendData, 0, progress.size()+ LWS_PRE);
        memcpy(pchSendData + LWS_PRE, progress.data(), progress.size());
        int nWrite = lws_write(pWsi, ((unsigned char *)pchSendData) + LWS_PRE, progress.size(), LWS_WRITE_TEXT);
        if (nWrite < (int)progress.size())
        {
            lwsl_err("向客户端[%p]发送数据-失败\n", pWsi);
        }
        delete[] pchSendData;
        pchSendData = nullptr;
        break;
    }
    /* WebSocket连接接收到数据时触发 */
    case LWS_CALLBACK_RECEIVE: {
        if (nullptr == pIn || nLen <= 0 || nullptr == pWsi)
        {
            break;
        }
        // 这里可以将接收的数据处理逻辑放到 LWS_CALLBACK_ESTABLISHED 中处理
        // 判断消息是否结束
        if (!lws_frame_is_binary(pWsi))
        {
            std::string data(reinterpret_cast<const char*>(pIn), nLen);
            pWsServer->m_wsUpload.store_param(pWsi, data);
            if (lws_remaining_packet_payload(pWsi) != 0)
            {
                break;
            }
            std::string param = pWsServer->m_wsUpload.get_param(pWsi);
            pWsServer->m_wsUpload.del_param(pWsi);
            pWsServer->m_wsUpload.parse_param(pWsi, param.c_str(), param.length());
            Net::Message_S stMessage;
            stMessage.pData = param.c_str();
            stMessage.nDataLength = param.length();
            stMessage.pHandle = pWsi;
            stMessage.ip.resize(100);
            lws_get_peer_simple(pWsi, stMessage.ip.data(), stMessage.ip.size());
            /* 修正字符串实际长度 */
            stMessage.ip.resize(strlen(stMessage.ip.c_str()));
            if (pWsServer->m_fnMessageCallback)
                pWsServer->m_fnMessageCallback(stMessage, pWsServer->m_stParam.stUserParam);
            lws_callback_on_writable(pWsi);
        }
        else
        {
            int nRet = pWsServer->m_wsUpload.write_data(pWsi, reinterpret_cast<const char*>(pIn), nLen);
            if (lws_remaining_packet_payload(pWsi) != 0)
            {
                break;
            }
            if (pWsServer->m_wsUpload.is_eof(pWsi))
            {
                pWsServer->m_wsUpload.merge(pWsi);
                m_uploadfFileName = pWsServer->m_wsUpload.get_upload_filename(pWsi);
                lws_callback_on_writable(pWsi);
            }
        }
        break;
    }
    case LWS_CALLBACK_CLOSED: {
        pWsServer->m_wsUpload.erase(pWsi);
        dlog_info("Connection closed\n");
        break;
    }
    default:
        break;
    }

    return nRet;
}


void LibWSServer::disconnect()
{
    dlog_info("LibWSServer断开连接");
    m_bExit = true;

    if (m_tid.joinable())
    {
        m_tid.join();
    }

    if (m_pContext)
    {
        //std::lock_guard<std::mutex> lock(m_mutex);
        /* 断开所有客户端连接 */ 
        for (auto pVhostDataInfo : m_connections)
        {
            if (pVhostDataInfo)
            {
                for (auto pClientInfo : pVhostDataInfo->clientInfos)
                {
                    if (pClientInfo)
                    {
                        lws_callback_on_writable(pClientInfo->pWsi);
                        lws_close_reason(pClientInfo->pWsi, LWS_CLOSE_STATUS_NORMAL, (unsigned char *)"Normal closure", 13);
                    }
                }
                pVhostDataInfo->clientInfos.clear();
            }
        }

        lws_context_destroy(m_pContext);
        m_pContext = nullptr;
    }


}

void LibWSServer::set_file_upload_path(const std::string &strFilePath)
{
    m_filePath = strFilePath;
}

std::string LibWSServer::get_upload_filename()
{
    return m_uploadfFileName;
}


int LibWSServer::setQosDscp(const int &nDscp)
{
    if (QOS_DSCP_MIN <= nDscp &&  QOS_DSCP_MAX >= nDscp)
    {
        LibWSServer::nManageDscp = nDscp;
        return OK;
    }
    return ERR_PARAM;
}