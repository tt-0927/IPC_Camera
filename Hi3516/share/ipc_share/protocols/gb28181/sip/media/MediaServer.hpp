/*
 * @Author       : EasonLu
 * @Date         : 2025-03-03 10:27:59
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-27 11:23:02
 * @FilePath     : MediaServer.hpp
 * @Description  : 媒体流服务器类
 */
#pragma once

/** 引入所需的头文件 */
#include "MediaNetBase.h"
#include "dlog.h"
#include <arpa/inet.h>
#include <atomic>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_set>
#include <vector>

/** 定义命名空间 SIP */
namespace SIP
{
    /**
     * @brief 媒体流服务器类，支持 TCP 和 UDP
     */
    class MediaServer : public MediaNetBase,
                        public std::enable_shared_from_this<MediaServer>
    {
    public:
        typedef std::shared_ptr<MediaServer> Ptr;

        struct ClientInfo
        {
            std::string strIP;
            int nPort;
            int nSocket = 0;
            /* 重载赋值运算符 */
            ClientInfo &operator=(const ClientInfo &rhs)
            {
                if (this != &rhs)
                {
                    strIP = rhs.strIP;
                    nPort = rhs.nPort;
                    nSocket = rhs.nSocket;
                }
                return *this;
            }
            /* 重载等于运算符 */
            bool operator==(const ClientInfo &rhs) const
            {
                return strIP == rhs.strIP && nPort == rhs.nPort;
            }
        };

        struct ClientInfoHash
        {
            size_t operator()(const ClientInfo &client) const
            {
                return std::hash<std::string>{}(client.strIP) ^
                       (std::hash<int>{}(client.nPort) << 1);
            }
        };

        /**
         * @brief 构造函数
         */
        MediaServer()
            : m_setAllowTargets(), m_setClients()
        {
            /** 初始化 TCP 监听标志 */
            m_bUseTcp = false;
            /** 初始化 UDP 监听标志 */
            m_bUseUdp = false;
            dlog_info("[MediaServer]构造服务器");
        }

        /**
         * @brief 初始化服务器，启动监听
         * @param
         * @param enType 监听的协议类型
         * @return 是否成功启动监听
         */
        bool init(int nPort, Protocol enType)
        {
            m_nLocalPort = nPort;
            m_bThreadRunning = true;
            /** 监听状态标志，初始化为 true */
            bool success = true;
            /** 如果协议类型包含 TCP，则启动 TCP 监听 */
            if (enType == ALL || enType == TCP)
            {
                m_bUseTcp = true;
                success &= startTCPListener();
            }
            /** 如果协议类型包含 UDP，则启动 UDP 监听 */
            if (enType == ALL || enType == UDP)
            {
                m_bUseUdp = true;
                success &= startUDPListener();
            }
            /** 如果启动监听失败，则设置端口号为 -1 */
            if (!success)
            {
                m_nLocalPort = -1;
                m_bThreadRunning = false;
            }

            m_bRunning = success;
            dlog_info("[MediaServer]初始化服务器，端口[%d]协议类型[%d]监听状态[%d]",
                      m_nLocalPort, enType, success);
            /** 返回监听状态 */
            return success;
        }

        /**
         * @brief 反初始化服务器，关闭所有监听
         */
        virtual void deinit() override
        {
            m_bThreadRunning = false;
            /** 如果 TCP 监听已启用，则关闭 TCP 套接字 */
            if (m_bUseTcp && m_nTcpSock > 0)
            {
                close(m_nTcpSock);
                m_nTcpSock = -1;
                m_bUseTcp = false;
            }
            /** 如果 UDP 监听已启用，则关闭 UDP 套接字 */
            if (m_bUseUdp && m_nUdpSock > 0)
            {
                close(m_nUdpSock);
                m_nUdpSock = -1;
                m_bUseUdp = false;
            }
            m_setAllowTargets.clear();
            m_setClients.clear();
            dlog_info("[MediaServer]反初始化，端口[%d]协议类型[%d]",
                      m_nLocalPort, m_bUseTcp ? TCP : UDP);
        }

        /**
         * @brief 析构函数，自动关闭所有监听
         */
        ~MediaServer()
        {
            /** 调用 deinit() 确保资源释放 */
            deinit();
            dlog_info("[MediaServer]析构服务器");
        }

        /**
         * @brief  设置白名单
         * @param  [string] &strAllowIP 白名单的IP
         * @param  [int] nPort 白名单的端口
         * @return [*]
         * @author EasonLu
         * @note   最好在init前调用，init时直接开始监听
         */
        void addAllowTarget(const std::string &strAllowIP, int nPort)
        {
            ClientInfo stClient;
            stClient.strIP = strAllowIP;
            stClient.nPort = nPort;
            m_setAllowTargets.insert(stClient);
            dlog_info("[MediaServer] 添加白名单:[%s:%d]", strAllowIP.c_str(), nPort);
        }

        /**
         * @brief  TCP服务端发送数据
         * @param  [char] *pData 发送数据
         * @param  [size_t] nSize  数据长度
         * @return [*]
         * @author EasonLu
         * @note   设置白名单则只发送白名单，否则发送所有
         */
        virtual int sendData(const char *pData, size_t nSize) override
        {
            if (m_nTcpSock < 0)
            {
                return -1;
            }
            if (nullptr == pData || nSize <= 0)
            {
                return -1;
            }

            for (auto &&client : m_setClients)
            {
                if (client.nSocket > 0)
                {
                    /* 设置参数MSG_NOSIGNAL,防止被服务器重置连接后继续发送数据引起SIGPIPE */
                    send(client.nSocket, pData, nSize, MSG_NOSIGNAL);
                }
            }
            return 0;
        }

    private:
        /* 线程运行标记位 */
        std::atomic_bool m_bThreadRunning = false;

        /* 连接白名单 */
        std::unordered_set<ClientInfo, ClientInfoHash> m_setAllowTargets;
        /* 连接名单 */
        std::unordered_set<ClientInfo, ClientInfoHash> m_setClients;

        /**
         * @brief 启动 TCP 监听
         * @return 是否成功启动 TCP 监听
         */
        bool startTCPListener()
        {
            /** 创建 TCP 套接字 */
            m_nTcpSock = socket(AF_INET, SOCK_STREAM, 0);
            if (m_nTcpSock < 0)
            {
                dlog_error("[MediaServer] 创建TCP套接字失败");
                return false;
            }
            /** 设置 TCP 监听地址 */
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(m_nLocalPort);
            /** 绑定 TCP 套接字 */
            if (bind(m_nTcpSock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            {
                dlog_error("[MediaServer] 绑定TCP端口失败");
                return false;
            }
            /** 开始 TCP 监听 */
            if (listen(m_nTcpSock, 5) < 0)
            {
                dlog_error("[MediaServer] TCP监听端口[%d]失败", m_nLocalPort);
                if (m_nTcpSock >= 0)
                {
                    close(m_nTcpSock);
                    m_nTcpSock = -1;
                }
                return false;
            }
            /** 创建 TCP 监听线程 */
            std::thread([this]()
                        {
                dlog_info("[MediaServer] 开启TCP监听[%d]", m_nLocalPort);
                while (m_bThreadRunning)
                {
                    sockaddr_in clientAddr{};
                    socklen_t clientLen = sizeof(clientAddr);
                    int clientSock = accept(m_nTcpSock, (struct sockaddr *)&clientAddr, &clientLen);
                    if (clientSock < 0)
                    {
                        dlog_warn("[MediaServer] TCP接受连接失败");
                        continue;
                    }
                    ClientInfo stInClient;
                    stInClient.strIP = inet_ntoa(clientAddr.sin_addr);
                    stInClient.nPort = ntohs(clientAddr.sin_port);
                    stInClient.nSocket = clientSock;
                    /* 判断是否设置了白名单 */
                    if (m_setAllowTargets.size() > 0)
                    {
                        auto pFind = m_setAllowTargets.find(stInClient);
                        if (pFind == m_setAllowTargets.end())
                        {
                            close(clientSock);
                            /* 不在白名单中 */
                            dlog_warn("[MediaServer] 连接 [%s:%d] 不在白名单中",
                                        stInClient.strIP.c_str(), stInClient.nPort);
                            continue;
                        }
                    }

                    dlog_info("[MediaServer] 新TCP连接[%s:%d]",
                                stInClient.strIP.c_str(), stInClient.nPort);
                    /* 记录连接 */
                    m_setClients.insert(stInClient);
                    std::thread([this, clientSock, clientAddr]() {
                        std::vector<char> buffer(m_nTcpRecvBuffSize);
                        while (m_bThreadRunning)
                        {
                            buffer.resize(m_nTcpRecvBuffSize);
                            ssize_t len = read(clientSock, buffer.data(), buffer.size());
                            if (len <= 0)
                                break;
                            if (m_fnCallback)
                            {
                                std::lock_guard<std::mutex> lock(m_mutexCallback);
                                m_fnCallback({
                                    inet_ntoa(clientAddr.sin_addr),
                                    buffer.data(),
                                    (size_t)len,
                                    true,
                                    true});
                            }
                        }
                        close(clientSock);
                    }).detach();
                }
                dlog_info("[MediaServer] 关闭TCP监听[%d]", m_nLocalPort); })
                .detach();
            return true;
        }

        /**
         * @brief 启动 UDP 监听
         * @return 是否成功启动 UDP 监听
         */
        bool startUDPListener()
        {
            /** 创建 UDP 套接字 */
            m_nUdpSock = socket(AF_INET, SOCK_DGRAM, 0);
            if (m_nUdpSock < 0)
            {
                dlog_error("[MediaServer] 创建UDP套接字失败");
                return false;
            }
            /** 设置 UDP 监听地址 */
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(m_nLocalPort);
            /** 绑定 UDP 套接字 */
            if (bind(m_nUdpSock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            {
                dlog_error("[MediaServer] 绑定UDP端口[%d]失败", m_nLocalPort);
                if (m_nUdpSock >= 0)
                {
                    close(m_nUdpSock);
                    m_nUdpSock = -1;
                }
                return false;
            }
            /** 创建 UDP 监听线程 */
            std::thread([this]()
                        {
                dlog_info("[MediaServer] 开启UDP监听端口[%d]", m_nLocalPort);
                std::vector<char> buffer(m_nUdpRecvBuffSize);
                sockaddr_in clientAddr{};
                socklen_t clientLen = sizeof(clientAddr);
                while (m_bThreadRunning)
                {
                    buffer.resize(m_nUdpRecvBuffSize);
                    ssize_t len = recvfrom(m_nUdpSock, buffer.data(), buffer.size(), 0, (struct sockaddr *)&clientAddr, &clientLen);
                    if (len > 0 && m_fnCallback)
                    {
                        std::lock_guard<std::mutex> lock(m_mutexCallback);
                        m_fnCallback({
                                inet_ntoa(clientAddr.sin_addr),
                                buffer.data(),
                                (size_t)len,
                                false,
                                true,
                                true});
                    }
                }
                dlog_info("[MediaServer] 关闭UDP监听[%d]", m_nLocalPort); })
                .detach();
            return true;
        }
    };
}
