/*
 * @FilePath     : CommonServer.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-09-14 15:52:12
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-01 16:10:44
 * @Description  : WebSocket 信息通讯服务器
 */
#include "CommonServer.h"

#include <functional>
#include "dlog.h"

using namespace wss_NS;

CCommonServer::CCommonServer(WebSocketParams_S stWebSocketParams)
    : CWSBase(stWebSocketParams)

{
    m_connections.clear();

    m_thread = std::thread(&CCommonServer::run, this);
}

CCommonServer::~CCommonServer()
{
    m_bHeartbeatRunning.store(false);
    m_bRunning.store(false);
    m_thread.join();
}

/* 发送信息 */
int CCommonServer::send_data(const char* pMessage, size_t nLen, WebSocketHandle_P pHandle)
{
    int nRet = 0;

    std::shared_ptr<char[]> pSendData = std::shared_ptr<char[]>(new char[nLen + LWS_PRE]);
    memset(pSendData.get(), 0, nLen + LWS_PRE);
    memcpy(pSendData.get() + LWS_PRE, pMessage, nLen);

    if (pHandle)
    {
        struct lws* pWsi = (struct lws*)pHandle;

        CCommonServer::VhostHandleInfo_S* pVhostDataInfo = (CCommonServer::VhostHandleInfo_S*)lws_protocol_vh_priv_get(lws_get_vhost(pWsi), lws_get_protocol(pWsi));
        if (pVhostDataInfo)
        {
            for (CCommonServer::ClientInfo_S* pClientInfo : pVhostDataInfo->clientDataSet)
            {
                if (pClientInfo->pWsi == pWsi)
                {
                    MsgInfo_S stInfo;
                    stInfo.pData = pSendData;
                    stInfo.nLen  = nLen;

                    pClientInfo->listMsgInfo.push_back(stInfo);

                    /* 通知可写 */
                    nRet = lws_callback_on_writable(pClientInfo->pWsi);
                    break;
                }
            }
        }
    }
    else
    {
        for (CCommonServer::VhostHandleInfo_S* pVhostDataInfo : m_connections)
        {
            if (pVhostDataInfo)
            {
                for (auto clientData : pVhostDataInfo->clientDataSet)
                {
                    MsgInfo_S stInfo;
                    stInfo.pData = pSendData;
                    stInfo.nLen  = nLen;
                    clientData->listMsgInfo.push_back(stInfo);

                    /* 通知可写 */
                    nRet = lws_callback_on_writable(clientData->pWsi);
                }
            }
        }
    }


    return nRet;
}

/* 通讯线程 */
void CCommonServer::run()
{
    bool        bIsSetHeartbeat = false;
    std::thread heartbeatThread;

    /* 句柄此信息 */
    struct lws_context_creation_info stInfo;

    /* 句柄 */
    struct lws_context* pstContext = nullptr;

    struct lws_protocols protocols[] = {
        {m_stWebSocketParams.stWebsocketNeedParam.strProtocolName.c_str(),
         CCommonServer::callback,
         0,
         m_stWebSocketParams.stWebsocketExParam.nRxBufferSize,
         m_stWebSocketParams.stWebsocketExParam.nID,
         this,
         m_stWebSocketParams.stWebsocketExParam.nTxPacketSize},
        LWS_PROTOCOL_LIST_TERM,
    };

    memset(&stInfo, 0, sizeof(stInfo));
    stInfo.port       = m_stWebSocketParams.stWebsocketNeedParam.nPort;
    stInfo.protocols  = protocols;
    stInfo.vhost_name = "localhost";
    stInfo.options    = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

    /* 设置日志等级 */
    lws_set_log_level(m_stWebSocketParams.stWebsocketExParam.nLogLevel, m_stWebSocketParams.stWebsocketExParam.logCallback);

    pstContext = lws_create_context(&stInfo);
    if (nullptr == pstContext)
    {
        printf("创建失败");
        return;
    }

    if (m_stWebSocketParams.stWebsocketExParam.nHeartbeatTime > 0 &&
        m_stWebSocketParams.stWebsocketExParam.heartbeatCallback != nullptr)
    {
        bIsSetHeartbeat = true;
        /* 创建心跳线程 */
        heartbeatThread = std::thread(&CCommonServer::sendHeartbeatThread, this);
    }

    while (m_bRunning.load())
    {
        lws_service(pstContext, 1000);
    }

    if (bIsSetHeartbeat)
    {
        /* 等待线程结束 */
        heartbeatThread.join();
    }

    lws_context_destroy(pstContext);
}

/* 发送心跳线程 */
void wss_NS::CCommonServer::sendHeartbeatThread()
{
    if (m_stWebSocketParams.stWebsocketExParam.nHeartbeatTime <= 0 ||
        m_stWebSocketParams.stWebsocketExParam.heartbeatCallback == nullptr)
    {
        return;
    }

    /* 心跳发送间隔 */
    std::chrono::milliseconds sleepDuration(m_stWebSocketParams.stWebsocketExParam.nHeartbeatTime);

    /* 心跳回调函数 */
    HeartbeatCallbackFunc heartbeatCallback = m_stWebSocketParams.stWebsocketExParam.heartbeatCallback;

    /* 初始化数据 */
    int   nIinitialBufferSize = 1024;
    char* pchData             = new char[nIinitialBufferSize];
    memset(pchData, 0, nIinitialBufferSize);

    while (m_bHeartbeatRunning.load())
    {
        HeartbeatCallbackParam_S stInfo;
        stInfo.nInLen       = nIinitialBufferSize;
        stInfo.pchData      = pchData;
        stInfo.pUserParam   = m_stWebSocketParams.stWebsocketExParam.pUser;

        /* 调用心跳回调，获取需要发送的数据 */
        heartbeatCallback(&stInfo);

        /* 判断是否需要扩大发送空间 */
        if (stInfo.nOutLen > stInfo.nInLen)
        {
            if (stInfo.pchData)
            {
                delete[] stInfo.pchData;
                stInfo.pchData = nullptr;
                pchData        = nullptr;
            }

            nIinitialBufferSize = stInfo.nOutLen;
            pchData             = new char[nIinitialBufferSize];
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
        }


        std::this_thread::sleep_for(sleepDuration);
    }

    delete[] pchData;
    pchData = nullptr;
}

/* 通讯回调函数 */
int CCommonServer::callback(
    struct lws*          pWsi,
    lws_callback_reasons enReason,
    void*                pUser,
    void*                pIn,
    size_t               nLen)
{
    /* 非0，会主动断开连接 */
    int nRet = 0;

    CCommonServer::VhostHandleInfo_S* pVhostDataInfo = (CCommonServer::VhostHandleInfo_S*)lws_protocol_vh_priv_get(lws_get_vhost(pWsi), lws_get_protocol(pWsi));

    switch (enReason)
    {
        /* LWS初始化一个新的协议触发 */
        case LWS_CALLBACK_PROTOCOL_INIT:
        {
            CCommonServer* pInstance = nullptr;

            /* 创建该协议的句柄数据 */
            pVhostDataInfo = (CCommonServer::VhostHandleInfo_S*)lws_protocol_vh_priv_zalloc(lws_get_vhost(pWsi), lws_get_protocol(pWsi), sizeof(CCommonServer::VhostHandleInfo_S));
            if (pVhostDataInfo)
            {
                pVhostDataInfo->pContext  = lws_get_context(pWsi);
                pVhostDataInfo->pProtocol = lws_get_protocol(pWsi);
                pVhostDataInfo->pVhost    = lws_get_vhost(pWsi);
                pVhostDataInfo->clientDataSet.clear();

                if (pVhostDataInfo->pProtocol)
                {
                    pInstance = (CCommonServer*)pVhostDataInfo->pProtocol->user;
                    if (pInstance)
                    {
                        /* 记录通讯协议句柄 */
                        pInstance->m_connections.insert(pVhostDataInfo);
                    }

                    if (pVhostDataInfo->pProtocol->name)
                    {
                        lwsl_user("[%s]协议初始化成功\n", pVhostDataInfo->pProtocol->name);
                    }
                }
            }


            break;
        }
        /* 特定协议的反初始化 */
        case LWS_CALLBACK_PROTOCOL_DESTROY:
        {
            if (pVhostDataInfo)
            {
                /* 移除该协议的在线用户句柄列表 */
                auto it = pVhostDataInfo->clientDataSet.begin();
                while (it != pVhostDataInfo->clientDataSet.end())
                {
                    const ClientInfo_S* pClientInfo = *it;

                    it = pVhostDataInfo->clientDataSet.erase(it);

                    delete pClientInfo;
                    pClientInfo = nullptr;
                }

                lwsl_user("[%s]协议释放成功\n", pVhostDataInfo->pProtocol->name);
            }
            break;
        }
        /* WebSocket连接成功建立后触发 */
        case LWS_CALLBACK_ESTABLISHED:
        {
            CCommonServer* pInstance = nullptr;

            if (pVhostDataInfo)
            {
                ClientInfo_S* pClientInfo = new ClientInfo_S();
                /* 添加到该协议的在线用户句柄列表 */
                pClientInfo->pWsi         = pWsi;
                pClientInfo->listMsgInfo.clear();
                pClientInfo->vectorRecvBuf.clear();

                pVhostDataInfo->clientDataSet.insert(pClientInfo);

                /* 获取用户自定义参数 */
                if (pVhostDataInfo->pProtocol)
                {
                    pInstance = (CCommonServer*)pVhostDataInfo->pProtocol->user;

                    if (pVhostDataInfo->pProtocol->name)
                    {
                        lwsl_user("[%s]协议的客户端[%p]连接成功\n", pVhostDataInfo->pProtocol->name, pWsi);
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
                    stParamInfo.enStatus   = WS_CONNECT;
                    stParamInfo.pHandle    = pWsi;
                    stParamInfo.pUserParam = pInstance->m_stWebSocketParams.stWebsocketExParam.pUser;

                    /* 调用回调函数 */
                    nRet = statusCallback(stParamInfo);
                }
            }
            break;
        }
        /* WebSocket连接关闭时触发 */
        case LWS_CALLBACK_CLOSED:
        {
            CCommonServer* pInstance = nullptr;

            if (pVhostDataInfo)
            {
                /* 移除该协议的在线用户句柄列表 */
                auto it = pVhostDataInfo->clientDataSet.begin();
                while (it != pVhostDataInfo->clientDataSet.end())
                {
                    const ClientInfo_S* pClientInfo = *it;


                    if (pClientInfo->pWsi == pWsi)
                    {
                        it = pVhostDataInfo->clientDataSet.erase(it);

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
                    pInstance = (CCommonServer*)pVhostDataInfo->pProtocol->user;

                    if (pVhostDataInfo->pProtocol->name)
                    {
                        lwsl_user("[%s]协议的客户端[%p]断开连接\n", pVhostDataInfo->pProtocol->name, pWsi);
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
                    stParamInfo.enStatus   = WS_DISCONNECT;
                    stParamInfo.pHandle    = pWsi;
                    stParamInfo.pUserParam = pInstance->m_stWebSocketParams.stWebsocketExParam.pUser;

                    /* 调用回调函数 */
                    nRet = statusCallback(stParamInfo);
                }
            }

            break;
        }
        /* WebSocket连接可以写入数据时触发 */
        case LWS_CALLBACK_SERVER_WRITEABLE:
        {

            if (!pVhostDataInfo)
            {
                break;
            }

            for (CCommonServer::ClientInfo_S* pClientInfo : pVhostDataInfo->clientDataSet)
            {
                if (pClientInfo->pWsi == pWsi)
                {
                    auto it = pClientInfo->listMsgInfo.begin();
                    while (it != pClientInfo->listMsgInfo.end())
                    {
                        const MsgInfo_S& stMsgInfo = *it;



                        /* 不能直接传智能指针进去lws_write() */
                        char* pchSendData = new char[stMsgInfo.nLen + LWS_PRE];
                        memset(pchSendData, 0, stMsgInfo.nLen + LWS_PRE);
                        memcpy(pchSendData, stMsgInfo.pData.get(), stMsgInfo.nLen + LWS_PRE);

                        // printf("发送数据[%p]：\n%s\n", stMsgInfo.pData.get(), pchSendData + LWS_PRE);

                        /* 注意，我们已经在有效负载中允许了LWS_PRE */
                        int nWrite = lws_write(pWsi, ((unsigned char*)pchSendData) + LWS_PRE, stMsgInfo.nLen, LWS_WRITE_TEXT);


                        /* 从容器中移除item，放在memcpy()后面 */
                        it = pClientInfo->listMsgInfo.erase(it);
                        delete[] pchSendData;
                        pchSendData = nullptr;

                        if (nWrite < (int)stMsgInfo.nLen)
                        {
                            lwsl_err("向客户端[%p]发送数据-失败\n", pWsi);

                            /* 写入失败 */
                            continue;
                        }
                    }

                    break;
                }
            }

            break;
        }

        /* WebSocket连接接收到数据时触发 */
        case LWS_CALLBACK_RECEIVE:
        {
            if (nullptr == pIn || nLen <=0 || nullptr == pWsi || nullptr == pVhostDataInfo)
            {
                return nRet;
            }

            if (pVhostDataInfo)
            {
                auto it = pVhostDataInfo->clientDataSet.begin();
                while (it != pVhostDataInfo->clientDataSet.end())
                {
                    ClientInfo_S* pClientInfo = *it;
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
                            CCommonServer* pInstance = nullptr;

                            if (pVhostDataInfo)
                            {
                                if (pVhostDataInfo->pProtocol)
                                {
                                    pInstance = (CCommonServer*)pVhostDataInfo->pProtocol->user;
                                }
                            }

                            if (pInstance)
                            {
                                DataCallbackFunc dataCallback;
                                dataCallback = pInstance->m_stWebSocketParams.stWebsocketExParam.dataCallback;
                                if (dataCallback)
                                {
                                    DataCallbackParam_S stParamInfo;
                                    stParamInfo.pData      = pClientInfo->vectorRecvBuf.data();
                                    stParamInfo.nDataLen   = pClientInfo->vectorRecvBuf.size();
                                    stParamInfo.pHandle    = pClientInfo->pWsi;
                                    stParamInfo.pUserParam = pInstance->m_stWebSocketParams.stWebsocketExParam.pUser;

                                    /* 调用回调函数 */
                                    nRet = dataCallback(stParamInfo);
                                }
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
