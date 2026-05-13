/*
 * @FilePath     : CommonClient.cpp
 * @Author       : cenqt cenqt@kfb.cn
 * @LastEditTime : 2023-10-31 16:16:26
 * @Description  : WebSocket 信息通讯客户端
 */

#ifndef _WS_COMMON_CLIENT_H_
#define _WS_COMMON_CLIENT_H_

#include <atomic>
#include <libwebsockets.h>
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <thread>

#include "WSBase.h"


/* one of these created for each message */
struct msg 
{
	void *payload; /* is malloc'd */
	size_t len;
};

typedef enum{
		RT_IDLE = 0,
		RT_BUSY,
		RT_TIMEOUT
}MSG_STATUS;




namespace wss_NS
{
    class CCommonClient : public CWSBase
    {

    public:
        CCommonClient(WebSocketParams_S stWebSocketParams);
        ~CCommonClient();


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
         * @brief 发送心跳线程
         * @return [*] 无
         * @note
         */
        void sendHeartbeatThread();

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
        /* 线程运行标志 */
        std::atomic<bool> m_bRunning          = { true };
        /* 发送心跳线程运行标志 */
        std::atomic<bool> m_bHeartbeatRunning = { true };

        /* 线程 */
        std::thread m_thread;
        
    };



};    // namespace wss_NS

#endif
