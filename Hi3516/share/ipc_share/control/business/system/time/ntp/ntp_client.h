/*
 * @FilePath: ntp_client.h
 * @Author: tianl
 * @Date: 2024-09-28 10:53:59
 * @LastEditors: tianl
 * @LastEditTime: 2024-09-30 11:22:29
 * @Description: ntp校时
 */
#pragma once

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <string>
#include <thread>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>
#include <sys/epoll.h>

#include "Singleton.h"
#include "system_define.h"
#include "rtsp_server.h"

#define VERSION_3 3   /* NTP 协议的版本 3 */
#define MODE_CLIENT 3 /* NTP 工作模式为客户端。 */
#define NTP_LI 0      /* 闰秒指示符 */
#define NTP_VN VERSION_3
#define NTP_MODE MODE_CLIENT
#define NTP_STRATUM 0       /* NTP 服务器的层级 */
#define NTP_POLL 4          /* 轮询间隔的初始值 */
#define NTP_PRECISION -6    /* NTP 服务器的精度值 */
#define NTP_HLEN 48         /* NTP 数据包的长度 */
#define NTP_PORT 123        /* ntp服务器默认端口号 */
#define NTP_TIMEOUT 10      /* 超时时间，单位为秒 */
#define JAN_1970 0x83aa7e80 /*  1970 年 1 月 1 日 0 时 0 分 0 秒的 NTP 时间戳 */
#define BUFSIZE 1500

#define TIMESCALE (60)              /* 时间进制转换单位 */
#define CALIBINT(x) (x * TIMESCALE) /* 校时间隔 */
#define TIMEZONEBASE (-8)           /* UTC基时区 */

#define NTP_CONV_FRAC32(x) (uint64_t)((x) * ((uint64_t)1 << 32))         /* 将一个浮点数转换为 32 位 NTP 分数格式 */
#define NTP_REVE_FRAC32(x) ((double)((double)(x) / ((uint64_t)1 << 32))) /* 将 NTP 32 位分数格式转换为浮点数 */

#define NTP_CONV_FRAC16(x) (uint32_t)((x) * ((uint32_t)1 << 16))         /* 将一个浮点数转换为 16 位 NTP 分数格式 */
#define NTP_REVE_FRAC16(x) ((double)((double)(x) / ((uint32_t)1 << 16))) /* 将 NTP 16 位分数格式转换为浮点数 */

#define USEC2FRAC(x) ((uint32_t)NTP_CONV_FRAC32((x) / 1000000.0)) /* 将微秒转换为 NTP 32 位分数格式 */
#define FRAC2USEC(x) ((uint32_t)NTP_REVE_FRAC32((x) * 1000000.0)) /* 将 NTP 32 位分数格式转换为微秒 */

#define NTP_LFIXED2DOUBLE(x) ((double)(ntohl(((struct l_fixedpt *)(x))->intpart) - JAN_1970 + FRAC2USEC(ntohl(((struct l_fixedpt *)(x))->fracpart)) / 1000000.0)) /* 将 NTP 固定小数格式（l_fixedpt 结构）转换为双精度浮点数 */

#define DEFAULT_NTP_SERVER "time.windows.com" /* 默认ntp服务器 */
#define DEFAULT_NTP_PORT 123                  /* 默认ntp端口 */
#define DEFAULT_NTP_TIMEZONE 8                /* 默认时区 北京时区 */
#define DEFAULT_NTP_INTERVAL 60               /* 默认校时时间 60分钟 */

class CNtpClient : public CSingleton<CNtpClient>
{
public:
    CNtpClient() : running(false), m_nSockfd(-1) {};
    ~CNtpClient();

    /**
     * @brief: 校时时间间隔 默认1分钟
     */
    int nInterval;

    /**
     * @brief: ntp服务器IP地址
     */
    std::string strServerIp;

    /**
     * @brief:  端口号
     */
    int nPort;

    /**
     * @brief:  时区
     */
    // double dTimeZone;
    int nTimeZone;

    /**
     * @brief:  线程开启标志
     */
    bool running;
    /**
     * @brief: 开启ntp校时
     * @return {*}
     */
    void start();

    /**
     * @brief: 关闭ntp校时
     * @return {*}
     */
    void stop();

    /**
     * @brief 初始化ntp校时
     * @param stNtp
     * @param nTimeZone
     * @return int
     */
    int init(System::NTPInfo_S &stNtp, System::TimeZone_E enTimeZone);

    /**
     * @brief 去初始化ntp校时
     * @return {*}
     */
    void deinit();

    /**
    * @brief 测试ntp服务器
    * @param stTestNtp 
    * @return int 
    */
   int test_ntp(System::TestNtp_S stTestNtp);

    bool bIsUpdate;

private:
    struct s_fixedpt
    {
        uint16_t intpart;
        uint16_t fracpart;
    };

    struct l_fixedpt
    {
        uint32_t intpart;
        uint32_t fracpart;
    };

    /**
     * @brief NTP报文头部结构体
     *
     * 该结构体用于表示NTP（网络时间协议）报文的头部信息，包括各种协议字段。
     *
     */
    struct ntphdr
    {
        unsigned int ntp_mode : 3;
        unsigned int ntp_vn : 3;
        unsigned int ntp_li : 2;
        uint8_t ntp_stratum;
        uint8_t ntp_poll;
        int8_t ntp_precision;
        struct s_fixedpt ntp_rtdelay;
        struct s_fixedpt ntp_rtdispersion;
        uint32_t ntp_refid;
        struct l_fixedpt ntp_refts;
        struct l_fixedpt ntp_orits;
        struct l_fixedpt ntp_recvts;
        struct l_fixedpt ntp_transts;
    };

    /**
     * @brief:  通信套接字
     */
    int m_nSockfd;

    struct sockaddr_in stServaddr;

    /**
     * @brief:  ntp校时线程
     */
    std::thread ntpThread;

    /**
     * @brief:构建并填充NTP请求报文.
     * @param {void} *pBuf NTP请求报文
     * @param {size_t} *pSize 报文大小
     * @return {bool} 成功返回true，失败返回false
     */
    bool constructNtpRequest(void *pBuf, size_t *pSize);

    /**
     * @brief: 计算客户端与NTP服务器之间的时间偏移量
     * @param {ntphdr} *pNtp 指向接收到的NTP报文头的指针. 其中包含服务器的时间戳
     * @param {timeval} *pstRecvtv 指向客户端接收NTP报文时的时间结构体
     * @return {double } 返回计算出的时间偏移量，单位为秒
     */
    double calculateOffset(const struct ntphdr *pNtp, const struct timeval *pstRecvtv);

    /**
     * @brief: 更新系统时间
     * @param {double} dOffset 时间偏移量，单位为秒
     * @return {void}
     */
    void updateSystemTime(double dOffset);

    /**
     * @brief:解析主机名并返回对应的网络地址
     * @param {string&} strHost 要解析的主机名
     * @return {in_addr_t} 成功返回主机网络地址；失败返回INADDR_NONE
     */
    in_addr_t resolveHost(const std::string &host);

    /**
     * @brief: run方法
     * @return {*}
     */
    void run();

    /**
     * @brief: 同步ntp时间
     * @return {int} 返回0同步成功
     */
    int syncTime();
};
