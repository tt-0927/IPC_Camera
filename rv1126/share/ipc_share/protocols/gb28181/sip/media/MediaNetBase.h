/*
 * @Author       : EasonLu
 * @Date         : 2025-04-24 09:06:14
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-27 15:19:28
 * @FilePath     : MediaNetBase.h
 * @Description  : 媒体网络链接基类
 */
#pragma once
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include <memory>
namespace SIP
{
    class MediaNetBase
    {
    public:
        MediaNetBase() = default;
        virtual ~MediaNetBase() = default;

        typedef std::shared_ptr<MediaNetBase> Ptr;

        /** 定义可选的协议类型 */
        enum Protocol
        {
            TCP, /** TCP 连接 */
            UDP, /** UDP 连接 */
            ALL  /** 客户端不开启同时支持 TCP 和 UDP */
        };

        /** 结构体用于存储回调数据 */
        struct CbData_S
        {
            std::string strIP;      /** 发送方 IP 地址 */
            const char *pData;      /** 数据指针 */
            size_t nSize;           /** 数据长度 */
            bool bIsTcp;            /** 是否为 TCP 连接的数据 */
            bool bIsServer;        /** 是否为服务器端数据 */
            bool bAudio = false;          /* 是否广播音频数据 */
        };

        /** 定义数据回调函数类型 */
        using DataCallback = std::function<void(const CbData_S &)>;
        /** 设置回调函数 */
        void setCallback(DataCallback callback)
        {
            std::lock_guard<std::mutex> lock(m_mutexCallback);
            m_fnCallback = callback;
        }

        bool isRunning() const { return m_bRunning; }

        int port() const { return m_nLocalPort; }

        bool isTcp() const { return m_bUseTcp; }

        bool isUdp() const { return m_bUseUdp; }

        virtual bool init(int nPort, Protocol enType) = 0;
        virtual void deinit() = 0;
        virtual int sendData(const char *pData, size_t nSize) = 0;

    protected:
        int m_nLocalPort;                    /** 本地监听的端口号 */
        bool m_bUseTcp = false;              /** TCP 是否启用 */
        bool m_bUseUdp = false;              /** UDP 是否启用 */
        int m_nTcpSock = -1;                 /** TCP 套接字 */
        int m_nUdpSock = -1;                 /** UDP 套接字 */
        DataCallback m_fnCallback;           /** 回调函数指针 */
        std::mutex m_mutexCallback;          /* 回调函数锁 */
        std::atomic_bool m_bRunning = false; /** 运行状态标志 */
        /** 设置 TCP 和 UDP 的接收缓冲区大小 */
        const uint32_t m_nTcpRecvBuffSize = 8192 * 40;
        const uint16_t m_nUdpRecvBuffSize = 8192 / 2;
    };
} // namespace SIP