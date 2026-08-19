/**
 * @file SdkHttpServer.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief SdkHttpServer 模块接口与类型定义
 * 功能说明：
 * 1. 声明 SdkHttpServer 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
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

/* 启用端口复用(SO_REUSEADDR)，允许服务端在TIME_WAIT状态下快速重启绑定同一端口 */
/* Enable port reuse (SO_REUSEADDR), allowing the server to quickly rebind the same port during TIME_WAIT */
/* #define SDK_HTTP_DISABLE_PORT_REUSE */

#include "tvsdkhttplib.h"
#include "HttpBasicCommand.hpp"
#include "RouteRegistry.h"


constexpr char NETSDK_HTTP_DEFAULT_HOST[] = "0.0.0.0";
constexpr uint16_t NETSDK_HTTP_DEFAULT_PORT = 80U;
constexpr std::size_t NETSDK_HTTP_THREAD_POOL_SIZE = 16U;
constexpr int NETSDK_HTTP_READ_TIMEOUT_SECONDS = 300;
constexpr int NETSDK_HTTP_WRITE_TIMEOUT_SECONDS = 30;
constexpr int NETSDK_HTTP_KEEP_ALIVE_MAX_COUNT = 100;
constexpr int NETSDK_HTTP_START_RETRY_COUNT = 50;
constexpr int NETSDK_HTTP_START_RETRY_INTERVAL_MILLISECONDS = 10;
constexpr int NETSDK_HTTP_WINDOWS_ADDRESS_IN_USE_ERROR = 10048;
class CSdkHttpServer : public CSingleton<CSdkHttpServer>
{

	CSdkHttpServer();
public:

	~CSdkHttpServer();
	friend class CSingleton<CSdkHttpServer>;

public:
	/**
 * @author tianl (tianl@kfb.cn)
	 * @brief 启动 HTTP 服务器
	 * @param host 监听地址（默认 "0.0.0.0"）
	 * @param port 监听端口（如 80
	 * @return 0=启动成功，< 0 端口占用/初始化失败
	 */
	int startServer(uint32_t uPort = NETSDK_HTTP_DEFAULT_PORT);
	int StopServer();
	void InitSession();
private:
	uint16_t m_nPort{0U};
    httplib::Server m_stServer;         	/* 服务器实例 */
    std::thread m_stServerThread;      	/* 服务器运行线程 */
    std::mutex m_stLifecycleMutex;
    std::atomic<bool> m_bRunning{false};
};
