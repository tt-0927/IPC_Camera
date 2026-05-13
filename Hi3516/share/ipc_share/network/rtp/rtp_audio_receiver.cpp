/*** 
 * @FilePath     : rtp_audio_receiver.cpp
 * @Author       : cyc
 * @Date         : 2025-07-15 20:00:17
 * @LastEditors  : cyc
 * @LastEditTime : 2025-07-30 09:24:52
 * @Description  : 解析rtp音频包
 */

#include "rtp_audio_receiver.h"
#include <iostream>
#include <sstream>
#include <regex>
#include <cstring>
#include "dlog.h"
#include <fcntl.h>
#include "IpcRet.h"

RtpAudioReceiver::RtpAudioReceiver()
{
    m_running.store(false);
}

RtpAudioReceiver::~RtpAudioReceiver() 
{

}

int RtpAudioReceiver::init(const std::string& strUrl)
{
    return parseUrl(strUrl);
}

void RtpAudioReceiver::setDataCallback(const DataCallback& callback) 
{
    m_dataCallback = callback;
}

bool RtpAudioReceiver::start() 
{
    /* 如果线程在跑停止线程 */
    if (m_running.load()) 
    {
        m_running.store(false);
    }

    /* 创建UDP套接字 */ 
    m_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket < 0) {
        dlog_error("failed to create socket");
        return false;
    }

    /* 设置套接字选项 */ 
    int opt = 1;
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        dlog_error("Failed to set socket options");
        close(m_socket);
        m_socket = -1;
        return false;
    }

    /* 绑定地址和端口 */ 
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_port);
    serverAddr.sin_addr.s_addr = inet_addr(m_ip.c_str());

    if (bind(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) 
    {
        dlog_error("Failed to bind socket to ip:%s,port:%d",m_ip.c_str(),m_port);
        close(m_socket);
        m_socket = -1;
        return false;
    }

    /* 启动接收线程 */ 
    m_running.store(true);
    m_receiveThread = std::thread(&RtpAudioReceiver::receiveThread, this);

    return true;
}

void RtpAudioReceiver::stop() 
{
    if (!m_running.load()) 
    {
        dlog_debug("rtp audio receiver not start");
        return;
    }

    /* 停止线程 */ 
    m_running.store(false);
    
    /* 关闭套接字（会导致recvfrom返回） */ 
    if (m_socket != -1) 
    {
        close(m_socket);
        m_socket = -1;
    }

    /* 等待线程退出 */ 
    if (m_receiveThread.joinable()) 
    {
        m_receiveThread.join();
    }
}

bool RtpAudioReceiver::isRunning() const 
{
    return m_running.load();
}

int RtpAudioReceiver::parseUrl(const std::string& strUrl)
{
    /* 重置旧值 */
    m_ip.clear();
    m_port = 0;

    /* 正则匹配 rtp://ip:port */
    std::regex urlPattern(R"(^rtp://(\d{1,3}(?:\.\d{1,3}){3}):(\d{1,5})$)");
    std::smatch matches;

    if (std::regex_match(strUrl, matches, urlPattern) && matches.size() == 3)
    {
        m_ip   = matches[1].str();
        m_port = static_cast<uint16_t>(std::stoi(matches[2].str()));
    }
    else
    {
        /* 解析失败：直接返回，不抛异常 */
        dlog_warn("Invalid RTP URL format: %s", strUrl.c_str());
        return ERR;
    }
    return OK;
}

void RtpAudioReceiver::receiveThread() 
{
    pthread_setname_np(pthread_self(), "RtpAudioReceive");

    constexpr size_t BUFFER_SIZE = 2048;
    std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(BUFFER_SIZE);

    sockaddr_in clientAddr{};
    socklen_t clientAddrLen = sizeof(clientAddr);
    int nFlags = fcntl(m_socket, F_GETFL, 0);
    fcntl(m_socket, F_SETFL, nFlags | O_NONBLOCK);


    while (m_running.load()) 
    {
        /* 接收数据 */ 
        ssize_t bytesRead = recvfrom
        (
            m_socket, 
            buffer.get(), 
            BUFFER_SIZE, 
            0, 
            reinterpret_cast<sockaddr*>(&clientAddr), 
            &clientAddrLen
        );

        if (bytesRead < 0) 
        {
            /* 检查是否因套接字关闭而中断 */ 
            if (!m_running.load()) 
            {
                break;
            }
            usleep(10);
            continue;
        }

        /* 调用回调函数 */ 
        if (m_dataCallback && bytesRead > 0) 
        {
            /* 音频数据位于RTP头部之后 */
            uint8_t* payloadData = buffer.get() + RTP_FIXED_HEADER_LENGTH; 
            /* 音频数据长度 */
            size_t payloadLen = bytesRead - RTP_FIXED_HEADER_LENGTH; 
            m_dataCallback(payloadData, payloadLen);
        }
    }
}

uint32_t RtpAudioReceiver::parseRtpTimestamp(const uint8_t* pData) const 
{
    /* RTP时间戳位于第4-7字节（从0开始计数） */ 
    return (static_cast<uint32_t>(pData[4]) << 24) |
           (static_cast<uint32_t>(pData[5]) << 16) |
           (static_cast<uint32_t>(pData[6]) << 8) |
           static_cast<uint32_t>(pData[7]);
}    