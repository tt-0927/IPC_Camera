/*
 * @FilePath     : TranspondServer.h
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-09-14 16:00:32
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2023-09-25 20:31:18
 * @Description  : 转发服务器-测试用
 */

#ifndef _WS_TRANSPOND_SERVER_H_
#define _WS_TRANSPOND_SERVER_H_

#include <atomic>
#include <libwebsockets.h>
#include <mutex>
#include <set>
#include <thread>

#include "WSBase.h"

namespace wss_NS
{
    class CTranspondServer : public CWSBase
    {

    protected:

        /* 为每条消息创建一个 */
        typedef struct _MsgInfo_
        {
            char*  pData; /* 数据 */
            size_t nLen;  /* 数据长度 */
        } MsgInfo_S;

        /* 为每个连接到我们的客户端创建其中一个 */
        typedef struct _ClientInfo_
        {
            struct lws* pWsi;  /* WebSocket连接句柄 */
            int         nLast; /* 发送的最后一条信息号码 */
        } ClientInfo_S;

        /* 为使用我们协议的每个虚拟主机创建一个句柄信息 */
        typedef struct _VhostHandleInfo_
        {
            struct lws_context*         pContext;
            struct lws_vhost*           pVhost;
            const struct lws_protocols* pProtocol;

            std::set<ClientInfo_S*> clientDataSet; /* 客户端信息列表集合 */

            MsgInfo_S stMsgInfo;                   /* 数据信息 */
            int       nCurrent;                    /* 我们正在缓存的当前消息号 */

        } VhostHandleInfo_S;

    public:

        CTranspondServer(WebSocketParams_S stWebSocketParams);
        ~CTranspondServer();


        /**
         * @brief 发送信息
         * @param [char*] pMessage: 发送内容
         * @param [size_t] nLen: 发送长度
         * @param [WebSocketHandle_P] pHandle: 发送的句柄
         * @return [*] 小于0-失败 大于等于0-成功
         * @note
         */
        int send_data(const char* pMessage, size_t nLen, WebSocketHandle_P pHandle = nullptr);

    private:

        /**
         * @brief 通讯线程
         * @return [*] 无
         * @note
         */
        void run();


        /**
         * @brief 创建数据空间
         * @param [void] *pMsg: 数据
         * @param [int] nLen: 空间大小
         * @return [*]
         * @note
         */
        static void create_message(void* pMsg, int nLen);

        /**
         * @brief 释放数据
         * @param [void] *pMsg: 数据
         * @return [*]
         * @note
         */
        static void release_message(void* pMsg);



        /**
         * @brief 通讯回调函数
         * @param [lws*] pWsi: 通讯句柄
         * @param [enum lws_callback_reasons] enReason: 通讯码枚举
         * @param [void*] pUser: 用户自定义参数
         * @param [void*] pIn: 数据内容
         * @param [size_t] nLen: 数据长度
         * @return [*]
         * @note
         */
        static int callback(struct lws* pWsi, enum lws_callback_reasons enReason, void* pUser, void* pIn, size_t nLen);

    private:

        /* 存储所有WebSocket连接的集合 */
        std::set<struct lws*> m_connections;

        /* 线程运行标志 */
        std::atomic<bool> m_bRunning = { true };

        /* 线程 */
        std::thread m_thread;
    };



};    // namespace wss_NS

#endif
