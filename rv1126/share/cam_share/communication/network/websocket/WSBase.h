/*
 * @FilePath     : WSBase.h
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-09-14 15:52:02
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2023-10-26 20:40:24
 * @Description  : libwebsockets库基类
 */
#ifndef _WS_BSAE_H_
#define _WS_BSAE_H_

#include <iostream>
#include <string>

namespace wss_NS
{
    /* 关闭状态 不是随便定的，必须是 lws-ws-close.h/enum lws_close_status中存在的 */
    typedef enum _WebCloseStatus_
    {
        WS_CLOSE_STATUS_NORMAL           = 1000, /* 表示正常关闭 */
        WS_CLOSE_STATUS_POLICY_VIOLATION = 1008, /* 由于策略违规而关闭 */
    } WebCloseStatus_E;

    /* 通讯句柄 */
    typedef void* WebSocketHandle_P;

    /* 数据回调函数的参数 */
    typedef struct _DataCallbackParam_
    {
        void*             pData;        /* 接收的内容 */
        int               nDataLen;     /* 接收的长度 */
        bool              bIsLastPacket;/* 是否最后一包数据 */
        void*             pUserParam;   /* 用户自定义的参数 */
        WebSocketHandle_P pHandle;      /* 通讯句柄 */

        void clear()
        {
            pData           = nullptr;
            pUserParam      = nullptr;
            bIsLastPacket   = false;
            pHandle         = nullptr;
            nDataLen        = 0;
        }

        _DataCallbackParam_()
        {
            this->clear();
        }
    } DataCallbackParam_S;

    /* 数据回调函数 */
    typedef int (*DataCallbackFunc)(DataCallbackParam_S);

    /* 通讯状态枚举 */
    typedef enum _WebSocketStatus_
    {
        WS_DISCONNECT = 0,
        WS_CONNECT,
        WS_CONNECTING,
        WS_CLOSEING,
    } WebSocketStatus_E;

    /* 发送数据类型枚举 */
    typedef enum _WebSocketWriteProtocol_
    {
        WS_WRITE_TEXT = 0,      /* 文本类型,对应 LWS_WRITE_TEXT */
	    WS_WRITE_BINARY,        /* 二进制流类型,对应 LWS_WRITE_BINARY */
    } WebSocketWriteProtocol_E;

    /* 状态回调函数的参数 */
    typedef struct _StatusCallbackParam_
    {
        WebSocketStatus_E enStatus;   /* 通讯状态 */
        void*             pUserParam; /* 用户自定义的参数 */
        WebSocketHandle_P pHandle;    /* 通讯句柄 */

        void clear()
        {
            pUserParam = nullptr;
            pHandle    = nullptr;
            enStatus   = WS_DISCONNECT;
        }

        _StatusCallbackParam_()
        {
            this->clear();
        }
    } StatusCallbackParam_S;

    /* 状态回调函数 */
    typedef int (*StatusCallbackFunc)(StatusCallbackParam_S);

    /* 心跳回调函数的参数 */
    typedef struct _HeartbeatCallbackParam_
    {
        char* pchData;    /* 发送信息的指针 */
        int   nInLen;     /* 发送信息指针的大小 */
        int   nOutLen;    /* 本次需要发送数据的大小 */
        void* pUserParam; /* 用户自定义的参数 */

        void clear()
        {
            pchData    = nullptr;
            pUserParam = nullptr;
            nInLen     = 0;
            nOutLen    = 0;
        }

        _HeartbeatCallbackParam_()
        {
            this->clear();
        }
    } HeartbeatCallbackParam_S;

    /* 心跳回调函数 */
    typedef int (*HeartbeatCallbackFunc)(HeartbeatCallbackParam_S*);

    /* 日志等级 不是随便定的，必须参考lws-logs.h */
    typedef enum _WebLogLevel_
    {
        WS_LLL_ERR     = (1 << 0),
        WS_LLL_WARN    = (1 << 1),
        WS_LLL_NOTICE  = (1 << 2),
        WS_LLL_INFO    = (1 << 3),
        WS_LLL_DEBUG   = (1 << 4),
        WS_LLL_PARSER  = (1 << 5),
        WS_LLL_HEADER  = (1 << 6),
        WS_LLL_EXT     = (1 << 7),
        WS_LLL_CLIENT  = (1 << 8),
        WS_LLL_LATENCY = (1 << 9),
        WS_LLL_USER    = (1 << 10),
        WS_LLL_THREAD  = (1 << 11),
    } WebLogLevel_E;

    /* 日志回调函数 */
    typedef void (*LogCallbackFunc)(int nLevel, const char* pchLine);

    /* 必备参数 */
    typedef struct _WebSocketNeedParam_
    {
        std::string strServerIP;     /* 服务器IP/域名 */
        std::string strServerPath;   /* 服务器请求路径 */
        int         nPort;           /* 端口号 */
        std::string strProtocolName; /* WebSocket 子协议的名称 */

        void clear()
        {
            strServerIP     = std::string();
            strServerPath   = std::string();
            nPort           = 0;
            strProtocolName = std::string();
        }

        _WebSocketNeedParam_()
        {
            this->clear();
        }

    } WebSocketNeedParam_S;

    /* 可选参数 */
    typedef struct _WebSocketExParam_
    {
        void* pUser;       /* 用户自定义参数 */

        int nRxBufferSize; /* 接收缓冲区的大小。用于设置每个会话的接收缓冲区大小，默认 0 */
        int nTxPacketSize; /* 发送缓冲区的大小。默认 0，等于0大小为 nRxBufferSize*/
        int nID;

        int nHeartbeatTime; /* 心跳时间，单位/ms，默认0 */
        int nLogLevel;      /* WebLogLevel_E 日志等级 默认 WS_LLL_ERR|WS_LLL_WARN|WS_LLL_USER */

        LogCallbackFunc       logCallback;       /* 日志回调函数 */
        DataCallbackFunc      dataCallback;      /* 数据回调函数 */
        StatusCallbackFunc    statusCallback;    /* 状态回调函数 */
        HeartbeatCallbackFunc heartbeatCallback; /* 心跳回调函数 */

        void clear()
        {
            pUser             = nullptr;
            logCallback       = nullptr;
            dataCallback      = nullptr;
            statusCallback    = nullptr;
            heartbeatCallback = nullptr;

            nRxBufferSize = 0;
            nTxPacketSize = 0;
            nID           = 0;

            nHeartbeatTime = 0;
            nLogLevel      = WS_LLL_ERR | WS_LLL_WARN | WS_LLL_USER;
        }

        _WebSocketExParam_()
        {
            this->clear();
        }
    } WebSocketExParam_S;

    /* WebSocket初始化参数 */
    typedef struct _WebSocketParams_
    {
        WebSocketNeedParam_S stWebsocketNeedParam; /* 必备参数 */
        WebSocketExParam_S   stWebsocketExParam;   /* 可选参数 */

        void clear()
        {
            stWebsocketNeedParam.clear();
            stWebsocketExParam.clear();
        }

        _WebSocketParams_()
        {
            this->clear();
        }
    } WebSocketParams_S;

    class CWSBase
    {

    public:

        CWSBase(WebSocketParams_S stWebSocketParams);

        virtual ~CWSBase();

        static std::string to_string(int nLevel);

        /**
         * @brief 发送信息
         * @param [char*] pMessage: 发送内容
         * @param [size_t] nLen: 发送长度
         * @param [WebSocketHandle_P] pHandle: 发送的句柄
         * @return [*] 小于0-失败 大于等于0-成功
         * @note
         */
        virtual int send_data(const char* pMessage, size_t nLen, WebSocketHandle_P pHandle = nullptr) = 0;

        /**
         * @brief 设置断开连接的原因
         * @param [WebSocketHandle_P] pHandle: 客户端句柄
         * @param [WebCloseStatus_E] enCloseStatus: 断开状态码
         * @param [char*] pchReasonBuf: 断开理由
         * @param [size_t] nReasonBufLen: 断开理由长度
         * @return [*] 小于0-失败 大于等于0-成功
         * @note 在回调函数中返回非0，就会断开连接
         */
        virtual int set_closeReason(WebSocketHandle_P pHandle, WebCloseStatus_E enCloseStatus, char* pchReasonBuf, size_t nReasonBufLen);

        /**
         * @brief 获取客户端URL中的参数信息
         * @param [WebSocketHandle_P] pHandle: 客户端句柄
         * @param [char*] pchName: 参数名称
         * @param [char*] pchParameterBuf: 解析出来的参数信息指针
         * @param [int] nParameterBufLen: 参数指针大小
         * @return [*] 小于0-失败 大于等于0-成功
         * @note
         */
        virtual int get_urlArgByName(WebSocketHandle_P pHandle, const char* pchName, char* pchParameterBuf, int nParameterBufLen);

    protected:

        /* 参数 */
        WebSocketParams_S m_stWebSocketParams;

    private:
    };

}    // namespace wss_NS

#endif