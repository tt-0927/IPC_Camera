/*** 
 * @FilePath     : rtp_audio_receiver.h
 * @Author       : cyc
 * @Date         : 2025-07-15 20:00:08
 * @LastEditors  : cyc
 * @LastEditTime : 2025-07-30 09:23:38
 * @Description  : 解析rtp音频包
 */

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <cstdint>
#include <memory>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

/* 定义RTP固定头部长度常量 */
#define RTP_FIXED_HEADER_LENGTH  (12)

/**
 * RTP音频包接收类，负责从指定URL接收RTP音频数据
 */
class RtpAudioReceiver
{
    public:
        /*  数据接收回调函数类型定义，参数为数据指针、数据长度 */
        using DataCallback = std::function<void(const uint8_t*, size_t)>;
        
        /** 析构函数 */
        ~RtpAudioReceiver();
        RtpAudioReceiver();

        /*** 
         * @description : 设置数据接收回调函数
         * @author      : cyc
         * @param        {DataCallback&} callback 回调函数
         * @return       {*}
         */        
        void setDataCallback(const DataCallback& callback);

        /*** 
         * @description : 初始化函数
         * @author      : cyc
         * @param        {string&} strUrl
         * @return       成功返回0，失败非0
         */        
        int init(const std::string& strUrl);

        /*** 
         * @description : 启动RTP接收服务
         * @author      : cyc
         * @return       @return 成功返回true，失败返回false
         */        
        bool start();

        /*** 
         * @description : 停止RTP接收服务
         * @author      : cyc
         * @return       {*}
         */        
        void stop();

        /*** 
         * @description : 检查服务是否正在运行
         * @author      : cyc
         * @return       运行中返回true，否则返回false
         */        
        bool isRunning() const;

    private:
        /*** 
         * @description : 解析RTP URL，提取IP地址和端口号
         * @author      : cyc
         * @param        {string&} strUrl
         * @return       成功返回0，失败非0
         */        
        int parseUrl(const std::string& strUrl);

        /*** 
         * @description : UDP接收线程函数
         * @author      : cyc
         * @return       {*}
         */        
        void receiveThread();

        /*** 
         * @description : 解析RTP时间戳
         * @author      : cyc
         * @param        {uint8_t*} pData RTP数据包
         * @return       RTP时间戳
         */        
        uint32_t parseRtpTimestamp(const uint8_t* pData) const;

    private:
        std::string m_ip;                      /* 服务器IP地址  */ 
        uint16_t m_port;                       /* 服务器端口号 */ 
        int m_socket;                          /* UDP套接字 */ 
        std::thread m_receiveThread;           /* 接收线程 */ 
        std::atomic<bool> m_running;           /* 运行状态标志 */ 
        DataCallback m_dataCallback;           /* 数据回调函数 */ 
};