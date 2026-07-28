/**
 * @file SdkHttpServer.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief SdkHttpServer 模块实现
 * 功能说明：
 * 1. 实现 SdkHttpServer 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#include "SdkHttpServer.h"
#include "NetSdkLog.h"
#include "NetTVSDKHttpUrl.h"
#include "SessionManager.h"

CSdkHttpServer::CSdkHttpServer() = default;

CSdkHttpServer::~CSdkHttpServer()
{
	StopServer();
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 startServer 对应的处理。
 * @param [in] uPort 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

int CSdkHttpServer::startServer(uint32_t uPort)
{
	std::lock_guard<std::mutex> stLock(m_stLifecycleMutex);
	int nRet = -1;
	m_nPort = static_cast<uint16_t>(uPort);
	if(m_bRunning.load())
	{
		NETSDK_LOG_MESSAGE_DEBUG("服务器已经启动！");
		return nRet;
	}

	/* 会话管理初始化 */
	InitSession();

	/* 加载所有注册的路由 */
	for (const auto& route : CRouteRegistry::getRoutes())
	{
        switch (route.method)
		{
            case HttpMethod_E::GET:
                m_stServer.Get(route.url, route.handler);
                break;
            case HttpMethod_E::PUT:
                if (route.useContentReader) {
                    m_stServer.Put(route.url, route.contentReaderHandler);
                } else {
                    m_stServer.Put(route.url, route.handler);
                }
                break;
			case HttpMethod_E::POST:
                if (route.useContentReader) {
                    m_stServer.Post(route.url, route.contentReaderHandler);
                } else {
                    m_stServer.Post(route.url, route.handler);
                }
                break;
			default:
			break;
        }
    }

	/* 读超时也覆盖固件上传 body 的接收过程，升级包较大时 10s 容易导致客户端报接收失败。 */
	m_stServer.set_read_timeout(NETSDK_HTTP_READ_TIMEOUT_SECONDS, 0);

	/* write_timeout 必须大于心跳间隔（8s），否则 content_provider 长时间不写数据会被断开 */
	/* AlarmListen 长连接：空闲时每 8s 发一次心跳，write_timeout 设 30s 留有余量 */
	m_stServer.set_write_timeout(NETSDK_HTTP_WRITE_TIMEOUT_SECONDS, 0);

	/* 长连接配置：AlarmListen 由 content_provider 自己通过心跳保活。 */
	/* HTTP keep-alive 只保留短请求复用，避免 NVR 反复重连后旧连接长期占用资源。 */
	m_stServer.set_keep_alive_max_count(NETSDK_HTTP_KEEP_ALIVE_MAX_COUNT);
	m_stServer.set_keep_alive_timeout(NETSDK_HTTP_WRITE_TIMEOUT_SECONDS);

	/* 减少网络延迟 */
	m_stServer.set_tcp_nodelay(true);

	/*
	 * httplib requires the factory to return a raw TaskQueue pointer and assumes
	 * ownership after the factory returns. std::unique_ptr cannot be used for
	 * this callback signature; the server releases the ThreadPool during stop.
	 */
	m_stServer.new_task_queue = [] { return new httplib::ThreadPool(NETSDK_HTTP_THREAD_POOL_SIZE); };

	NETSDK_LOG_MESSAGE_INFO("正在绑定端口 %s:%d...", NETSDK_HTTP_DEFAULT_HOST, m_nPort);
	bool bBindOk = m_stServer.bind_to_port(NETSDK_HTTP_DEFAULT_HOST, m_nPort);
    if (!bBindOk)
    {
        int error_code = errno;
        std::error_code ec(error_code, std::system_category());

        /* 端口冲突特殊提示 */
        if (error_code == EADDRINUSE || error_code == NETSDK_HTTP_WINDOWS_ADDRESS_IN_USE_ERROR)
        {
            NETSDK_LOG_MESSAGE_ERROR("❌ 端口绑定失败: 端口 %d 已被占用！", m_nPort);
            NETSDK_LOG_MESSAGE_ERROR("   可能原因: 另一个程序或服务已在使用此端口");
            NETSDK_LOG_MESSAGE_ERROR("   解决方案: 1) 停止占用端口的程序  2) 使用其他端口启动服务");
        }
        else
        {
            NETSDK_LOG_MESSAGE_ERROR("❌ 端口绑定失败: %s (错误码: %d)", ec.message().c_str(), error_code);
        }

        m_bRunning.store(false);
        return -1;
    }

	NETSDK_LOG_MESSAGE_DEBUG("端口绑定成功，开始监听...");
	m_bRunning.store(true);
	m_stServerThread = std::thread([this]()
	{
		NETSDK_LOG_MESSAGE_DEBUG("HTTP服务线程已启动");
		bool ret = m_stServer.listen_after_bind();

		if (!ret)
		{
			int error_code = errno;
			std::error_code ec(error_code, std::system_category());
			NETSDK_LOG_MESSAGE_ERROR("listen_after_bind失败: %s (错误码: %d)", ec.message().c_str(), error_code);
		}

		m_bRunning.store(false);
		NETSDK_LOG_MESSAGE_INFO("SDK HTTP服务器已停止");
	});

	int nRetries = NETSDK_HTTP_START_RETRY_COUNT;
    while (!m_stServer.is_running() && nRetries > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(NETSDK_HTTP_START_RETRY_INTERVAL_MILLISECONDS));
        --nRetries;
    }

	if (m_stServer.is_running())
    {
		NETSDK_LOG_MESSAGE_INFO("✅ SDK HTTP服务器启动成功! 监听地址: %s:%d", NETSDK_HTTP_DEFAULT_HOST, m_nPort);
        nRet = 0; /* 成功 */
    }
    else
    {
        NETSDK_LOG_MESSAGE_ERROR("❌ 服务器启动失败!");
        NETSDK_LOG_MESSAGE_ERROR("   状态检查: is_running() 返回 false");
        NETSDK_LOG_MESSAGE_ERROR("   可能原因: 1) 端口已被占用  2) 监听失败  3) 线程启动异常");
        NETSDK_LOG_MESSAGE_ERROR("   当前端口: %d", m_nPort);

        /* 如果启动失败，需要清理线程 */
        m_stServer.stop();
        if(m_stServerThread.joinable()) m_stServerThread.join();
		m_bRunning.store(false);
        nRet = -1;
    }

	return nRet;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 StopServer 对应的处理。
 * @return 返回该处理的状态或结果。
 */

int CSdkHttpServer::StopServer()
{
	std::lock_guard<std::mutex> stLock(m_stLifecycleMutex);
	m_bRunning.store(false);
	m_stServer.stop();
	if (m_stServerThread.joinable())
	{
		m_stServerThread.join();
	}
	NETSDK_LOG_MESSAGE_DEBUG("SDK服务已停止");

	return 0;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 InitSession 对应的处理。
 * @return 无返回值。
 */

void CSdkHttpServer::InitSession()
{
	NETSDK_LOG_MESSAGE_DEBUG("会话管理相关初始化");
	/* 登录接口 */
	m_stServer.Post(NET_API_PATH_BASIC_LOGIN, [](const httplib::Request& req, httplib::Response& res)
	{
    	CSessionManager::instance()->HttpCommandLogin(req, res);
	});

	/* 注销接口 */
	m_stServer.Post(NET_API_PATH_BASIC_LOGOUT, [](const httplib::Request& req, httplib::Response& res)
	{
    	CSessionManager::instance()->HttpCommandLout(req, res);
	});

	/* 保活接口 */
	m_stServer.Get(NET_API_PATH_BASIC_KEEPLIVE, [](const httplib::Request& req, httplib::Response& res)
	{
    	CSessionManager::instance()->HttpCommandKeepAlive(req, res);
	});

	/* 报警监听推送接口 */
	m_stServer.Get(NET_API_PATH_ALARMEVENT_LISTEN, [](const httplib::Request& req, httplib::Response& res)
    {
        CSessionManager::instance()->HttpCommandAlarmListen(req, res);
    });
}
