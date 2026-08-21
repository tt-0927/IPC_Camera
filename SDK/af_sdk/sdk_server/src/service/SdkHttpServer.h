/**
 * @file SdkHttpServer.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-04
 * 
 * @brief sdk HTTP服务通信端
 */
#pragma once
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <iostream>

#include "Singleton.h"

// 启用端口复用(SO_REUSEADDR)，允许服务端在TIME_WAIT状态下快速重启绑定同一端口
// Enable port reuse (SO_REUSEADDR), allowing the server to quickly rebind the same port during TIME_WAIT
// #define SDK_HTTP_DISABLE_PORT_REUSE

#include "tvsdkhttplib.h"
#include "HttpBasicCommand.hpp"
#include "RouteRegistry.h"


#define DEFAULT_HTTP_SERVER_HOST		"0.0.0.0"		/* http服务默认ip */
#define DEFAULT_HTTP_SERVER_POST		80				/* http服务默认端口号 */
class CSdkHttpServer : public CSingleton<CSdkHttpServer>
{

	CSdkHttpServer();
public:
	
	~CSdkHttpServer();    
	friend class CSingleton<CSdkHttpServer>;

public:
	/**
	 * @brief 启动 HTTP 服务器
	 * @param host 监听地址（默认 "0.0.0.0"）
	 * @param port 监听端口（如 80
	 * @return 0=启动成功，< 0 端口占用/初始化失败
	 */
	int startServer(uint32_t nPort = DEFAULT_HTTP_SERVER_POST);
	int StopServer();
	void InitSession();
private:
	uint16_t m_nPort;                  	// 服务端口
    httplib::Server m_server;         	// 服务器实例
    std::thread m_serverThread;      	// 服务器运行线程
    bool m_running;                		// 服务器运行状态
};