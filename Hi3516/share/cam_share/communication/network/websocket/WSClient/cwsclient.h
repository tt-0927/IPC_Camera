/*
 * @FilePath     : cwsclient.h
 * @Author       : Xiezhh
 * @Date         : 2024-08-01
 * @Description  : 继承至CWSBase类的一个websocket客户端可以更加灵活的连接和断开ws
 *                 支持Http Bearer鉴权。
 *                 注意：断开连接后立即调用connect()，lws_sul_schedule函数没办法在指定时间内执行回调（大概延迟15-30秒），
 *                 如果需要立马重新连接建议重新初始化或重新new一个CWSClient。
 */
#ifndef CWSCLIENT_H
#define CWSCLIENT_H

#include <libwebsockets.h>
#include "cwsclientcommon.h"
#include "WSBase.h"

#include <thread>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <vector>

namespace wss_NS {

    class CWSClient : public CWSBase {
    public:
        CWSClient();
        ~CWSClient();

        /**
         * @brief  设置参数
         * @param  [WSClientParame] 参数
         * @return [int] 0: 设置成功，1：线程已运行，2：参数异常
         * @author Xiezhh
         */
        int setParame(WSClientParame stParame);

        /**
         * @brief  初始化
         * @author Xiezhh
         * @return [bool] true: 开始运行线程，false：线程已运行
         * @note 开启线程，初始化lws
         */
        bool init();
        /**
         * @brief  反初始化
         * @author Xiezhh
         * @note 设置线程停止标志，退出线程时释放lws上下文
         */
        void unInit();

        /**
         * @brief Lws事件循环线程是否运行中
         * @return [bool] true：运行中，false：未运行
         * @author Xiezhh
         */
        bool getRuningFlag();

        /**
         * @brief  连接websocket
         * @return [bool] true：成功将连接添加进入队列中，false：失败
         * @author Xiezhh
         * @note 当只有在lws上下文成功创建的时候才会连接.
         */
        bool connect();

        /**
         * @brief  断开连接websocket
         * @return [bool] true：断开连接，false：失败
         * @author Xiezhh
         */
        bool disConnect();

        /**
         * @brief  是否连接
         * @return [bool] true：成功连接，false：未连接
         * @author Xiezhh
         */
        bool isConnect();

        /**
         * @brief  发送数据
         * @param [const char*]         数据
         * @param [size_t]              数据大小
         * @param [WebSocketHandle_P]   通信句柄
         * @return [int] 0：成功，其它失败
         * @author Xiezhh
         */
        int send_data(const char* pMessage, size_t nLen, WebSocketHandle_P pHandle = nullptr) override;

        /**
         * @brief  发送数据
         * @param [uint8_t*] 数据
         * @param [size_t]   数据大小
         * @param [int]      数据类型（SendType）
         * @return [int] 0：成功，其它失败
         * @author Xiezhh
         */
        int sendData(uint8_t* pData, size_t nLen, WebSocketWriteProtocol_E enSendType);

    private:
        /**
         * @brief  给成员变量设置默认值
         * @author Xiezhh
         */
        void reset();

        /**
         * @brief  初始化子协议数组和连接信息结构体
         * @author Xiezhh
         */
        void initConnectInfo();

        /**
         * @brief 连接，Lws事件循环线程
         * @param [CWSClient*] 当前类指针
         * @author Xiezhh
         * @note 调用init初始化，线程执行函数
         */
        static void run(CWSClient* pThis);


        /**
         * @brief 设置 Lws事件循环线程运行标志
         * @param [bool] true：运行，false：停止运行
         * @author Xiezhh
         */
        void setRuningFlag(const bool & bRuningFlag);

        /**
         * @brief 获取连接标志
         * @return [WebSocketStatus_E] 状态枚举
         * @author Xiezhh
         */
        WebSocketStatus_E getConnectStatus();

        /**
         * @brief 设置连接标志
         * @param [WebSocketStatus_E] 状态枚举
         * @author Xiezhh
         */
        void setConnectStatus(const WebSocketStatus_E &enStatus);

        /**
         * @brief 插入发送环形缓冲区
         * @param [const char*] 数据
         * @param [size_t]      数据大小
         * @param [WebSocketWriteProtocol_E] 发送数据类型
         * @author Xiezhh
         */
        int insertWriteRingBuffer(const char* pData, size_t nLen, WebSocketWriteProtocol_E enSendType);

        /**
         * @brief 插入发送环形缓冲区（重载）
         * @param [uint8_t*]    数据
         * @param [size_t]      数据大小
         * @param [WebSocketWriteProtocol_E] 发送数据类型
         * @author Xiezhh
         */
        int insertWriteRingBuffer(uint8_t* pData, size_t nLen, WebSocketWriteProtocol_E enSendType);

        /* 静态回调函数 */
    private:
        /**
         * @brief  lws事件回调
         * @param [lws *] 客户端句柄
         * @param [lws_callback_reasons] 事件类型
         * @param [void *] 用户数据（无用）
         * @param [void *] 事件数据
         * @param [size_t] 事件数据长度
         * @return [int] 返回非0值会直接断连 
         * @author Xiezhh
         */
        static int wsEventHandleCallBack(struct lws *pWsi, enum lws_callback_reasons enReason, void *pUser, void *pIn, size_t pLen);
        
        /**
         * @brief  连接websocekt
         * @param [lws_sorted_usec_list *] 事件列表
         * @author Xiezhh
         * @note 添加一个100ms后执行的连接事件到事件列表中
         */
        static void wsConnect(struct lws_sorted_usec_list *pSul);

        /* 事件具体的处理函数 */
    private:
        int lwsProtocolInit(struct lws *pWsi, void *pUser, void *pIn, size_t nLen);
        int lwsProtocolDestroy(struct lws *pWsi, void *pUser, void *pIn, size_t nLen);
        int lwsClientAppendHandShakeHeader(struct lws *pWsi, void *pUser, void *pIn, size_t nLen);
        int lwsConnectError(struct lws *pWsi, void *pUser, void *pIn, size_t nLen);
        int lwsConnectEstableished(struct lws *pWsi, void *pUser, void *pIn, size_t nLen);
        int lwsConnectClose(struct lws *pWsi, void *pUser, void *pIn, size_t nLen);
        int lwsWaitCancelled(struct lws *pWsi, void *pUser, void *pIn, size_t nLen);
        int lwsWriteable(struct lws *pWsi, void *pUser, void *pIn, size_t nLen);
        int lwsReceive(struct lws *pWsi, void *pUser, void *pIn, size_t nLen);

    private:
        /* 初始化参数 */
        WSClientParame m_stParame;

        /* 连接状态互斥锁 */
        std::shared_mutex m_mutexConnectStatus;
        /* 连接状态 */
        std::atomic< WebSocketStatus_E > m_enConnectStatus = WS_DISCONNECT;

        /* 连接互斥锁 */
        std::mutex m_mutexConnect;

        /* LWS事件循环线程 */
        std::thread m_thread;

        /* 是否正在运行标志互斥锁 */
        std::shared_mutex m_mutexRuning;
        /* 是否正在运行标志 */
        std::atomic<bool> m_bIsRuning = false;

        /* lws的参数和成员 */
    private:
        /* 上下文 */
        struct lws_context *m_pContext = NULL;
        /* 虚拟主机 */
        struct lws_vhost *m_pVhost = NULL;
        /* 协议栈 */
        struct lws_protocols m_pProtocols[2];

        /* 环形发送队列 */
        struct lws_ring *m_pTxRingBuffer = NULL;
        /* 环形缓冲区互斥锁 */
        std::mutex m_txRingBufferMutex;
        /*  */
        uint32_t m_nTail = 0;

        /* 连接事件列表 */
        lws_sorted_usec_list_t m_stSul;

        /* 连接信息 */
        struct lws_client_connect_info m_stConnectInfo;
        /* 客户端连接上之后的句柄 */
        struct lws *m_pClientWsi = NULL;
    };

}

#endif //CWSCLIENT_H