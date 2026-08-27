

#include "TCPClientExample.h"
#include "TCPServerExample.h"
#include "UDSClientExample.h"
#include "UDSServerExample.h"
#include "WebSocketServerExample.h"

#include <unistd.h>
#include <iostream>
#include <random>   

#include "dlog.h"

void tcpc();
void tcps();
void udsc();
void udss();
void wss();
int main(int argc, char* argv[])
{
    /* 不出初始化，不设置没有info以下的打印 */
    setLogLevel(LOG_TRACE);
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "tcpc")
        {
            tcpc();
        }
        else if (std::string(argv[i]) == "tcps")
        {
            tcps();
        }
        else if (std::string(argv[i]) == "udss")
        {
            udss();
        }
        else if (std::string(argv[i]) == "udsc")
        {
            udsc();
        }
        else if (std::string(argv[i]) == "wss")
        {
            wss();
        }
    }
    return 0;
}

void tcps()
{
    dlog_info("创建tcp服务端");
    TCPServerExample server;
    server.init();
    std::string str = "这里是服务端，我修改了心跳";
    server.set_heartbeat(str.data(), str.size());
        // 创建随机数生成器，基于随机设备
    std::random_device rd;
    std::mt19937 gen(rd());  // 使用梅森旋转算法生成随机数

    // 设置范围 [min, max]
    int min = 1, max = 1;
    std::uniform_int_distribution<> dis(min, max);
    while (1)
    {

        // 生成随机数
        int random_num = dis(gen);
        str.clear();
        str.resize(random_num, '1');
        server.send(str, 2003);
        usleep(3000*1000);
    }
}
void tcpc()
{
    
    dlog_info("创建tcp客户端");
    TCPClientExample client;
    client.init();

    std::string data = "这里是客户端 这里是客户端 收到请回答 收到请回答 over";
    dlog_info("客户端发送消息ret[%d]", client.send(data, 0));

    sleep(1);
    std::string str = "这里是客户端，我修改了心跳";
    client.set_heartbeat(str.data(), str.size());
    while (1)
    {
        dlog_info("客户端发送消息2003 ret[%d]", client.send(data, 2003));
        sleep(6);
    }
    while (1) sleep(1);
}

    // 创建随机数生成器，基于随机设备
std::random_device rd;
std::mt19937 gen(rd());  // 使用梅森旋转算法生成随机数

// 设置范围 [min, max]
int min = 666, max = 2*1024*1024;
std::uniform_int_distribution<> dis(min, max);
int random_num = 0;
void udss()
{
    dlog_info("创建uds服务端");
    UDSServerExample server;
    server.init();
    std::string str = "这里是服务端，我修改了心跳";
    server.set_heartbeat(str.data(), str.size());
    while (1)
    {

        // 生成随机数
        random_num = dis(gen);
        str.clear();
        str.resize(random_num, '1');
        server.send(str, 2003);
        usleep(3*1000);
    }
}
void udsc()
{
    std::vector<UDSClientExample> clients;
    for (int i = 0; i < 10; ++i)
    {
        dlog_info("创建uds客户端%d", i);
        UDSClientExample client;
        client.init();
        clients.push_back(client);
    }
    dlog_info("创建uds客户端");
    UDSClientExample client;
    client.init();

    std::string data = "这里是客户端 这里是客户端 收到请回答 收到请回答 over";
    dlog_info("客户端发送消息ret[%d]", client.send(data, 0));

    std::string str = "这里是客户端，我修改了心跳";
    client.set_heartbeat(str.data(), str.size());
    while (1)
    {
        dlog_info("客户端发送消息2003 ret[%d]", client.send(data, 2003));
        sleep(6);
    }
    while (1) sleep(1);
}
void wss()
{
    dlog_info("创建websocket服务端");
    WebSocketServerExample server;
    server.init();
    sleep(10);
    std::string str = "这里是服务端，我修改了心跳";
    server.set_heartbeat(str.data(), str.size());
    while (1) sleep(1);
}