/*
 * @FilePath     : ntp_example.cpp
 * @Author       : tianl
 * @Date         : 2024-09-28 10:55:35
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-04-10 11:10:30
 * @Description  : NTP测试用例
 */

#include "time_manage.h"
#include <csignal>

// using namespace ntp;

// Ntp::CNtpClient NtpClient;

// 信号处理函数
// void exitHandler(int signum)
// {
//     std::cout << "Caught signal: " << signum << ". Stopping NTP Client..." << std::endl;
//     NtpClient.stop(); // 调用 CNtpClient 的 stop 方法
//     exit(-1);         // 退出程序
// }

int main(int argc, char *argv[])
{
    CTimeManage::instance()->init();

    // // 检查命令行参数
    // if (argc != 5)
    // {
    //     std::cerr << "用法: " << argv[0] << " <server_ip> <port> <interval> <timeZone>" << std::endl;
    //     return 1;
    // }

    // // 设置信号处理函数
    // signal(SIGINT, exitHandler);
    // signal(SIGTERM, exitHandler);

    // std::string serverIp = argv[1];                            // 获取服务器IP
    // uint16_t port = static_cast<uint16_t>(std::stoi(argv[2])); // 获取端口号
    // int interval = std::stoi(argv[3]);                         // 获取校准时间间隔
    // int nTimeZone = std::stoi(argv[4]);                        // 获取时区

    // // 初始化 CNtpClient
    // if (0 != NtpClient.Initialize(serverIp, port, interval, nTimeZone))
    // {
    //     std::cerr << "Failed to init ntpServer" << std::endl;
    // }

    // // 启动 CNtpClient
    // NtpClient.start();

    // while (NtpClient.running == true)
    // {
    //     sleep(1);
    // }

    // NtpClient.stop();
    while(1);

    return 0;
}
