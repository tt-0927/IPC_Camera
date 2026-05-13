/*
 * @Author       : EasonLu
 * @Date         : 2025-03-18 17:08:24
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-25 10:43:23
 * @FilePath     : MediaClient.hpp
 * @Description  : TCP/UDP 客户端，支持绑定本地端口并与服务器通信
 */
#pragma once

/** 引入必要的头文件，用于网络编程、多线程、同步等功能 */
#include "MediaNetBase.h"
#include "ModuleLog.h"
#include <arpa/inet.h>
#include <atomic>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

/** 定义命名空间 SIP，封装客户端类 */
namespace SIP
{
    /** 定义 TCP/UDP 客户端类 */
    class MediaClient : public MediaNetBase,
                        public std::enable_shared_from_this<MediaClient>
    {

    public:
        typedef std::shared_ptr<MediaClient> Ptr;

        /**
         * @brief 构造函数，初始化客户端
         * @param remoteIP 远程服务器的 IP
         * @param remotePort 远程服务器的端口
         */
        MediaClient(const std::string &remoteIP, int remotePort)
            : m_strRemoteIP(remoteIP), m_nRemotePort(remotePort)
        {
            MLOG_INFO("[MediaClient]构造客户端，目标IP[%s]端口[%d]",
                      m_strRemoteIP.c_str(), m_nRemotePort);
        }

        /** 析构函数，释放资源 */
        ~MediaClient()
        {
            deinit();
            MLOG_INFO("[MediaClient]析构客户端，目标IP[%s]端口[%d]",
                      m_strRemoteIP.c_str(), m_nRemotePort);
        }

        virtual bool init(int nPort, Protocol enType) override
        {
            MLOG_INFO("[MediaClient]初始化客户端，目标IP[%s]端口[%d]本地端口[%d]协议类型[%d]",
                      m_strRemoteIP.c_str(), m_nRemotePort, nPort, enType);
            m_nLocalPort = nPort;
            m_bRunning = true;
            if (enType == TCP)
            {
                m_bUseTcp = startTCPClient(); /** 启动 TCP 客户端 */
                return m_bUseTcp;
            }
            if (enType == UDP)
            {
                m_bUseUdp = startUDPClient(); /** 启动 UDP 客户端 */
                return m_bUseUdp;
            }
            m_bRunning = false;
            return false;
        }

        /** 关闭客户端并释放资源 */
        virtual void deinit() override
        {
            m_bRunning = false; /** 设置运行状态为 false */
            m_bUseTcp = false;
            m_bUseUdp = false;
            if (m_bUseTcp && m_nTcpSock > 0)
            {
                close(m_nTcpSock); /** 关闭 TCP 套接字 */
                m_nTcpSock = -1;
            }
            if (m_bUseUdp && m_nUdpSock > 0)
            {
                close(m_nUdpSock); /** 关闭 UDP 套接字 */
                m_nUdpSock = -1;
            }
            MLOG_INFO("[MediaClient]反初始化，目标IP[%s]端口[%d]",
                      m_strRemoteIP.c_str(), m_nRemotePort);
        }

        virtual int sendData(const char *pData, size_t nSize) override
        {
            if (m_bUseTcp)
            {
                return sendTCP(pData, nSize);
            }
            if (m_bUseUdp)
            {
                return sendUDP(pData, nSize);
            }
            return -1;
        }

    private:
        std::string m_strRemoteIP = ""; /** 远程服务器 IP */
        int m_nRemotePort = 0;          /** 远程端口号 */
        /**
         * @brief 发送 TCP 数据
         * @param data 要发送的数据
         * @return 是否发送成功
         */
        bool sendTCP(const char *data, size_t size)
        {
            if (m_nTcpSock < 0)
            {
                return false;
            }
            if (data == nullptr || size <= 0)
            {
                return false;
            }
            if( send(m_nTcpSock, data, size, MSG_NOSIGNAL) == (ssize_t)size)
            {
                //printf("========TCP套接字[%d]=============发送数据=======大小[%d]=======SUCESSFULE========\n",m_nTcpSock,size);
                return true;
            }
            else
            {
                //printf("========TCP套接字[%d]=============发送数据=======大小[%d]========ERROR  =======\n",m_nTcpSock,size);
            }
           
            return false;

            // /* 设置参数MSG_NOSIGNAL,防止被服务器重置连接后继续发送数据引起SIGPIPE */
            // return send(m_nTcpSock, data, size, MSG_NOSIGNAL) == (ssize_t)size;
        }

        /**
         * @brief 发送 UDP 数据
         * @param data 要发送的数据
         * @return 是否发送成功
         */
        bool sendUDP(const char *data, size_t size)
        {
            if (m_nUdpSock < 0)
            {
                return false;
            }
            if (m_strRemoteIP.empty())
            {
                return false;
            }
            if (data == nullptr || size <= 0)
            {
                return false;
            }
            sockaddr_in remoteAddr{};
            remoteAddr.sin_family = AF_INET;
            remoteAddr.sin_addr.s_addr = inet_addr(m_strRemoteIP.c_str());
            remoteAddr.sin_port = htons(m_nRemotePort);
            if (remoteAddr.sin_addr.s_addr == INADDR_NONE)
            {
                std::cerr << "Error: Invalid IP address: " << m_strRemoteIP << std::endl;
                return false;
            }
            return sendto(m_nUdpSock, data, size, 0, (struct sockaddr *)&remoteAddr, sizeof(remoteAddr)) == (ssize_t)size;
        }
        /** 启动 TCP 客户端 */
        bool startTCPClient()
        {
            m_nTcpSock = socket(AF_INET, SOCK_STREAM, 0);
            if (m_nTcpSock < 0)
            {
                return false;
            }

            sockaddr_in localAddr{};
            localAddr.sin_family = AF_INET;
            localAddr.sin_addr.s_addr = INADDR_ANY;
            localAddr.sin_port = htons(m_nLocalPort);
            bind(m_nTcpSock, (struct sockaddr *)&localAddr, sizeof(localAddr));

            sockaddr_in serverAddr{};
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_addr.s_addr = inet_addr(m_strRemoteIP.c_str());
            serverAddr.sin_port = htons(m_nRemotePort);
            if (connect(m_nTcpSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
            {
                return false;
            }
            else
            {
                MLOG_INFO("[MediaClient]  连接成功 本地[Tcp]端口[%d] ====> 连接远端Tcp[%s:%d]",
                            m_nLocalPort, m_strRemoteIP.c_str(), m_nRemotePort);
            }

            std::thread([this]()
                        {
                MLOG_INFO("[MediaClient][Tcp][%d] ====> [%s:%d]",
                            m_nLocalPort, m_strRemoteIP.c_str(), m_nRemotePort);
                std::vector<char> buffer(m_nTcpRecvBuffSize);
                while (m_bRunning)
                {
                    ssize_t len = read(m_nTcpSock, buffer.data(), buffer.size());
                    if (len <= 0) break;
                    if (m_fnCallback)
                    {
                        m_fnCallback({
                        m_strRemoteIP, 
                        buffer.data(), 
                        (size_t)len, 
                        true,
                        false});
                    }
                } })
                .detach();
            return true;
        }

        /** 启动 UDP 客户端 */
        bool startUDPClient()
        {
            m_nUdpSock = socket(AF_INET, SOCK_DGRAM, 0);
            if (m_nUdpSock < 0)
            {
                return false;
            }

            sockaddr_in localAddr{};
            localAddr.sin_family = AF_INET;
            localAddr.sin_addr.s_addr = INADDR_ANY;
            localAddr.sin_port = htons(m_nLocalPort);
            bind(m_nUdpSock, (struct sockaddr *)&localAddr, sizeof(localAddr));

            std::thread([this]()
                        {
                MLOG_INFO("[MediaClient][Udp][%d] ====> [%s:%d]",
                        m_nLocalPort, m_strRemoteIP.c_str(), m_nRemotePort);
                std::vector<char> buffer(m_nUdpRecvBuffSize);
                sockaddr_in remoteAddr{};
                socklen_t remoteLen = sizeof(remoteAddr);
                while (m_bRunning)
                {
                    ssize_t len = recvfrom(m_nUdpSock, buffer.data(), buffer.size(), 0, (struct sockaddr *)&remoteAddr, &remoteLen);
                    if (len > 0 && m_fnCallback)
                    {
                        m_fnCallback({
                        inet_ntoa(remoteAddr.sin_addr), 
                        buffer.data(), 
                        (size_t)len, 
                        false,
                        false});
                    }
                } })
                .detach();
            return true;
        }
    };
}
