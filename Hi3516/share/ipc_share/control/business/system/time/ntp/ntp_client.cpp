/*
 * @FilePath: ntp_client.cpp
 * @Author: tianl
 * @Date: 2024-09-28 10:53:47
 * @LastEditors: tianl
 * @LastEditTime: 2024-09-30 11:19:46
 * @Description: ntp校时
 */
#include "ntp_client.h"
#include "log_handler.h"
#include "dlog.h"

CNtpClient::~CNtpClient()
{
    bIsUpdate = true;
    /* 确保线程终止 */
    stop();
    if (m_nSockfd >= 0)
    {
        close(m_nSockfd);
        m_nSockfd = -1;
    }
}

/* 设置ntp参数 */
int CNtpClient::init(System::NTPInfo_S &stNtp, System::TimeZone_E enTimeZone)
{

    /* 设置端口号 */
    if (stNtp.nPort <= 0)
    {
        this->nPort = DEFAULT_NTP_PORT;
        stNtp.nPort = DEFAULT_NTP_PORT;
    }
    else
    {
        this->nPort = stNtp.nPort;
    }

    /* 设置ntp服务器地址 */
    if (stNtp.address.empty())
    {
        this->strServerIp = DEFAULT_NTP_SERVER;
        stNtp.address = DEFAULT_NTP_SERVER;
    }
    else
    {
        if (this->strServerIp != stNtp.address)
        {
            if (0 < m_nSockfd)
            {
                close(m_nSockfd);
                m_nSockfd = -1;
            }
            this->strServerIp = stNtp.address;
        }
    }

    /* 设置校时时间间隔 */
    if (stNtp.nSyncInterval <= 0)
    {
        this->nInterval = DEFAULT_NTP_INTERVAL;
        stNtp.nSyncInterval = DEFAULT_NTP_INTERVAL;
    }
    else
    {
        this->nInterval = stNtp.nSyncInterval;
    }

    this->nTimeZone = static_cast<int>(enTimeZone);

    /* 开启ntp校时 */
    if (!running)
    {
        start();
    }

    bIsUpdate = true;

    return 0;
}

void CNtpClient::deinit()
{
    stop();
}

void CNtpClient::start()
{
    /* 避免重复启动 */
    if (running)
    {
        return;
    }
    running = true;
    /* 开启线程 */
    ntpThread = std::thread(&CNtpClient::run, this);
}

void CNtpClient::stop()
{
    running = false;

    // 发送空UDP包中断阻塞的recv调用
    if (m_nSockfd != -1) 
    {
        // 创建临时socket连接
        struct sockaddr_in tmp_addr;
        memcpy(&tmp_addr, &stServaddr, sizeof(stServaddr));
        
        int tmp_sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (tmp_sock >= 0) 
        {
            connect(tmp_sock, (struct sockaddr *)&tmp_addr, sizeof(tmp_addr));
            send(tmp_sock, "", 0, 0); // 发送空数据包
            close(tmp_sock);
        }
        
        // 关闭主socket
        close(m_nSockfd);
        m_nSockfd = -1;
    }

    // 设置线程超时等待
    if (ntpThread.joinable()) 
    {
        const auto timeout = std::chrono::milliseconds(200); // 200ms等待
        auto start = std::chrono::steady_clock::now();
        
        while (ntpThread.joinable() && std::chrono::steady_clock::now() - start < timeout) 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        if (ntpThread.joinable())
        {
            ntpThread.detach(); // 安全分离线程
            // dlog_warn("NTP线程未能及时退出，已分离");
        }
    }
}

int CNtpClient::test_ntp(System::TestNtp_S stTestNtp)
{
    char aChBuf[BUFSIZE];
    size_t nbytes = BUFSIZE;
    struct sockaddr_in stTestServaddr;
    int nTestFd;
    
    // 创建UDP套接字
    nTestFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (nTestFd < 0) 
    {
        dlog_error("套接字创建失败: %s", strerror(errno));
        return -1;
    }

    // 增加超时时间到 5 秒（原为 3 秒）
    struct timeval timeout = {5, 0};
    if (setsockopt(nTestFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout))) 
    {
        dlog_error("设置超时失败: %s", strerror(errno));
        close(nTestFd);
        return -1;
    }

    // 配置服务器地址
    memset(&stTestServaddr, 0, sizeof(stTestServaddr));
    stTestServaddr.sin_family = AF_INET;
    stTestServaddr.sin_port = htons(stTestNtp.nPort);
    
    // 解析主机地址
    stTestServaddr.sin_addr.s_addr = resolveHost(stTestNtp.address.c_str());
    if (stTestServaddr.sin_addr.s_addr == INADDR_NONE) 
    
    {
        dlog_error("地址解析失败: %s", stTestNtp.address.c_str());
        close(nTestFd);
        return -1;
    }

    // 构造NTP请求
    if (!constructNtpRequest(aChBuf, &nbytes)) 
    {
        dlog_error("构造NTP请求失败");
        close(nTestFd);
        return -1;
    }

    // 添加重试机制
    const int MAX_RETRIES = 3;
    bool success = false;
    
    for (int attempt = 1; attempt <= MAX_RETRIES; ++attempt) 
    {
        // 发送请求
        ssize_t sent = sendto(nTestFd, aChBuf, nbytes, 0,
                            (struct sockaddr*)&stTestServaddr, sizeof(stTestServaddr));
        if (sent != static_cast<ssize_t>(nbytes)) 
        {
            dlog_warn("请求发送失败(尝试 %d/%d): %s", 
                    attempt, MAX_RETRIES, strerror(errno));
            continue;
        }

        // 接收响应
        socklen_t addrLen = sizeof(stTestServaddr);
        ssize_t received = recvfrom(nTestFd, aChBuf, BUFSIZE, 0,
                                  (struct sockaddr*)&stTestServaddr, &addrLen);
        
        // 处理接收结果
        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) 
            {
                dlog_warn("服务器响应超时(尝试 %d/%d): %s:%d", 
                        attempt, MAX_RETRIES, stTestNtp.address.c_str(), stTestNtp.nPort);
            }
            else 
            {
                dlog_warn("响应接收错误(尝试 %d/%d): %s", 
                        attempt, MAX_RETRIES, strerror(errno));
            }
        } 
        else if (received < static_cast<ssize_t>(NTP_HLEN)) 
        {
            dlog_warn("响应数据不完整(尝试 %d/%d): 收到 %zd 字节, 需要 %d 字节", 
                    attempt, MAX_RETRIES, received, NTP_HLEN);
        } 
        else {
            // 解析响应
            struct ntphdr* response = reinterpret_cast<struct ntphdr*>(aChBuf);
            if (response->ntp_mode != 4) {  // NTP服务器模式应为4
                dlog_warn("无效响应模式(尝试 %d/%d): %d (期望值:4)", 
                        attempt, MAX_RETRIES, response->ntp_mode);
            } 
            else if (response->ntp_stratum == 0) {  // Stratum 0表示不可用
                dlog_warn("服务器不可用(尝试 %d/%d): Stratum 0", 
                        attempt, MAX_RETRIES);
            } 
            else {
                success = true;
                break;  // 成功，退出重试循环
            }
        }
        
        // 失败时等待片刻再重试
        if (attempt < MAX_RETRIES) {
            usleep(500000);  // 等待 0.5 秒
        }
    }

    close(nTestFd);
    
    if (success) {
        dlog_info("NTP服务器测试成功 [%s:%d]", 
                 stTestNtp.address.c_str(), stTestNtp.nPort);
        return 0;
    } else {
        dlog_error("NTP服务器测试失败: %s:%d (尝试 %d 次)", 
                 stTestNtp.address.c_str(), stTestNtp.nPort, MAX_RETRIES);
        return -1;
    }
}

void CNtpClient::run()
{
    pthread_setname_np(pthread_self(), "NtpClientRun");

    const int retry_wait_ms = 300; /* 重试间隔细化为300ms */
    const int check_interval_ms = 100; /* 主检查间隔100ms */
    
    while (running)
    {
        // 尝试同步时间
        int sync_result = -1;
        for (int retry = 0; retry < 3 && running; retry++) 
        {
            sync_result = syncTime();
            if (0 == sync_result)
            {
                /* 重置RtspServer上次请求IDR帧时间，防止无法请求出IDR帧 */
                CRtspServer::instance()->reset_lastIdrRequestTime();

                /* 信息日志-系统校时 */
                Log::Info_S stLogInfo;
                stLogInfo.nType = Log::INFOMATION;
                stLogInfo.nAction = Log::TIME_SYNC;
                LogHandler::instance()->write(stLogInfo);
                bIsUpdate = false;
                dlog_info("时间同步成功！");
                
                break;
            }
            else if (running)
            {
                /* 异常日志-时间同步异常 */
                Log::Info_S stLogInfo;
                stLogInfo.nType = Log::EXCEPTION;
                stLogInfo.nAction = Log::TIME_SYNC_ABNORMAL;
                LogHandler::instance()->write(stLogInfo);
                int wait_ms = retry_wait_ms;
                while (wait_ms > 0 && running)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(check_interval_ms));
                    wait_ms -= check_interval_ms;
                }
            }
        }

        // 主等待循环（细粒度检查）
        int wait_ms = nInterval * 60 * 1000; // 分钟转毫秒
        while (wait_ms > 0 && running && !bIsUpdate)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(check_interval_ms));
            wait_ms -= check_interval_ms;
        }
    }
}

/* 同步时间 */
int CNtpClient::syncTime()
{
    char aChBuf[BUFSIZE];
    size_t nbytes = BUFSIZE;

    if (0 > m_nSockfd)
    {
        /* 初始化服务器 */
        stServaddr.sin_family = AF_INET;
        stServaddr.sin_port = htons(nPort);
        stServaddr.sin_addr.s_addr = resolveHost(strServerIp);

        /* 创建通信套接字 */
        m_nSockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (m_nSockfd < 0)
        {
            dlog_error("套接字创建失败！");
            return -1;
        }

        /* 建立连接 */
        if (connect(m_nSockfd, (struct sockaddr *)&stServaddr, sizeof(stServaddr)) != 0)
        {
            dlog_error("连接失败！");
            close(m_nSockfd);
            m_nSockfd = -1;
            return -1;
        }
    }

    /* 配置ntp数据包 */
    if (!constructNtpRequest(aChBuf, &nbytes))
    {
        dlog_error("构造NTP请求失败");
        return -1;
    }

    /* 向ntp服务器发送数据包 */
    send(m_nSockfd, aChBuf, nbytes, 0);

    /* 创建 epoll 实例 */
    int nEpollfd = epoll_create1(0);
    if (nEpollfd == -1)
    {
        dlog_error("epoll_create1失败");
        return -1;
    }

    /* 将套接字添加到 epoll 监控列表中 */
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = m_nSockfd;

    if (epoll_ctl(nEpollfd, EPOLL_CTL_ADD, m_nSockfd, &event) == -1)
    {
        dlog_error("epoll_ctl失败");
        close(nEpollfd);
        return -1;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(m_nSockfd, &readfds);

    /* 用于存储 epoll_wait 的返回结果 */
    struct epoll_event events[1];

    struct timeval stTimeout;
    stTimeout.tv_sec = NTP_TIMEOUT;
    stTimeout.tv_usec = 0;

    /* 设置超时 */
    int nTimeout = stTimeout.tv_sec * 1000 + stTimeout.tv_usec / 1000;

    /* 等待事件发生 */
    int nfds = epoll_wait(nEpollfd, events, 1, nTimeout);
    if (nfds < 0)
    {
        dlog_error("epoll_wait失败");
        close(nEpollfd);
        return -1;
    }
    else if (nfds == 0)
    {
        dlog_error("NTP服务器超时或无响应");
        close(nEpollfd);
        return -1;
    }

    /*  事件发生，处理数据 */
    if (events[0].events & EPOLLIN)
    {
        if ((nbytes = recv(m_nSockfd, aChBuf, BUFSIZE, 0)) < 0)
        {
            dlog_error("接收数据失败");
            close(nEpollfd);
            return -1;
        }

        struct timeval stRecvtv;
        gettimeofday(&stRecvtv, nullptr);
        double dOffset = calculateOffset(reinterpret_cast<struct ntphdr *>(aChBuf), &stRecvtv);

        /* 时区设置 */
        // dOffset += (TIMEZONEBASE + nTimeZone) * TIMESCALE * TIMESCALE;
        dOffset += nTimeZone * TIMESCALE * TIMESCALE;
        updateSystemTime(dOffset);
    }

    /* 关闭 epoll 实例 */
    close(nEpollfd);

    return 0;
}

/* 配置ntp请求数据包 */
bool CNtpClient::constructNtpRequest(void *pBuf, size_t *pSize)
{
    if (!pSize || *pSize < NTP_HLEN)
        return false;

    memset(pBuf, 0, *pSize);
    auto *pNtp = reinterpret_cast<struct ntphdr *>(pBuf);

    pNtp->ntp_li = NTP_LI;
    pNtp->ntp_vn = NTP_VN;
    pNtp->ntp_mode = NTP_MODE;
    pNtp->ntp_stratum = NTP_STRATUM;
    pNtp->ntp_poll = NTP_POLL;
    pNtp->ntp_precision = NTP_PRECISION;

    struct timeval stTv;
    gettimeofday(&stTv, nullptr);

    pNtp->ntp_transts.intpart = htonl(stTv.tv_sec + JAN_1970);
    pNtp->ntp_transts.fracpart = htonl(USEC2FRAC(stTv.tv_usec));
    *pSize = NTP_HLEN;
    return true;
}

double CNtpClient::calculateOffset(const struct ntphdr *pNtp, const struct timeval *pstRecvtv)
{
    double dT1 = 0.0;
    double dT2 = 0.0;
    double dT3 = 0.0;
    double dT4 = 0.0;

    dT1 = NTP_LFIXED2DOUBLE(&pNtp->ntp_orits);
    dT2 = NTP_LFIXED2DOUBLE(&pNtp->ntp_recvts);
    dT3 = NTP_LFIXED2DOUBLE(&pNtp->ntp_transts);
    dT4 = pstRecvtv->tv_sec + pstRecvtv->tv_usec / 1000000.0;

    return ((dT2 - dT1) + (dT3 - dT4)) / 2;
}

/* 更新系统时间 */
void CNtpClient::updateSystemTime(double dOffset)
{
    struct timespec ts;

    /* 获取当前系统时间 */
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        dlog_error("clock_gettime 失败: %s", strerror(errno));
        return;
    }

    /* 计算新时间（处理秒和纳秒） */
    ts.tv_sec += static_cast<time_t>(dOffset);
    double fractional = dOffset - static_cast<time_t>(dOffset);
    ts.tv_nsec += static_cast<long>(fractional * 1e9);

    /* 处理微秒进位/借位 */
    if (ts.tv_nsec >= 1e9)
    {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1e9;
    }
    else if (ts.tv_nsec < 0)
    {
        ts.tv_sec -= 1;
        ts.tv_nsec += 1e9;
    }

    /* 设置系统时间 */
    if (clock_settime(CLOCK_REALTIME, &ts) == -1)
    {
        dlog_error("clock_settime 失败: %s", strerror(errno));
        return;
    }

    /*同步写到RTC（硬件时钟）中*/
    if (system("hwclock -w") == -1)
    {
        dlog_error("system(hwclock -w) 调用失败: %s", strerror(errno));
    }
    else
    {
        dlog_info("系统时间已更新并同步到RTC");
    }
}

/* 解析端口 */
in_addr_t CNtpClient::resolveHost(const std::string &strHost)
{
    in_addr_t stSaddr;
    struct hostent *pHostent;

    if ((stSaddr = inet_addr(strHost.c_str())) == INADDR_NONE)
    {
        if ((pHostent = gethostbyname(strHost.c_str())) == nullptr)
        {
            return INADDR_NONE;
        }

        memmove(&stSaddr, pHostent->h_addr, pHostent->h_length);
    }

    return stSaddr;
}
