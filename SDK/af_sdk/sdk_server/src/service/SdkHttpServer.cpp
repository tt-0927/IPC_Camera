/**
 * @file SdkHttpServer.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-04
 *
 * @brief
 */

#include "SdkHttpServer.h"
#include "NetSdkLog.h"
#include "NetTVSDKHttpUrl.h"
#include "SessionManager.h"

CSdkHttpServer::CSdkHttpServer()
{
	m_running = false;
}

CSdkHttpServer::~CSdkHttpServer()
{
	StopServer();
}

int CSdkHttpServer::startServer( uint32_t nPort)
{
	int nRet = -1;
	m_nPort = nPort;
	if(m_running)
	{
		NSDK_LOG_DEBUG("服务器已经启动！");
		return nRet;
	}

	/* 会话管理初始化 */
	InitSession();

	/* 加载所有注册的路由 */
	for (const auto& route : CRouteRegistry::getRoutes())
	{
        switch (route.method)
		{
            case HttpMethod::GET: m_server.Get(route.url, route.handler); break;
            case HttpMethod::PUT: m_server.Put(route.url, route.handler); break;
			case HttpMethod::POST: m_server.Post(route.url, route.handler); break;
			default:
			break;
        }
    }

	// 短请求超时（登录、心跳、命令等短连接接口）
	m_server.set_read_timeout(10, 0);     // 读超时10s（适用于短请求）

	// write_timeout 必须大于心跳间隔（8s），否则 content_provider 长时间不写数据会被断开
	// AlarmListen 长连接：空闲时每 8s 发一次心跳，write_timeout 设 30s 留有余量
	m_server.set_write_timeout(30, 0);

	// 长连接配置：AlarmListen 是长连接，必须设置足够大的 keep_alive_timeout
	// 报警周期可能超过 1 分钟，空闲1分钟内没有报警不应该断连
	m_server.set_keep_alive_max_count(10000); // 单个长连接内处理请求数上限
	m_server.set_keep_alive_timeout(300);     // 空闲 300 秒（5分钟）无数据才关闭

	// 减少网络延迟
	m_server.set_tcp_nodelay(true);

	// 线程池大小：AlarmListen 长连接每个客户端占用 1 个线程
	// 摄像机端：1-2 个 NVR 连进来 + 少量短请求，4 个线程足够
	// NVR 端：1-4 个安防平台客户端 + 少量短请求，8 个线程足够
	// 空闲线程不消耗 CPU，每个线程栈内存约 1MB，8 个线程约占 8MB
	m_server.new_task_queue = [] { return new httplib::ThreadPool(8); };

	NSDK_LOG_INFO("正在绑定端口 %s:%d...", DEFAULT_HTTP_SERVER_HOST, m_nPort);
	bool bind_ok = m_server.bind_to_port(DEFAULT_HTTP_SERVER_HOST, m_nPort);
    if (!bind_ok)
    {
        int error_code = errno;
        std::error_code ec(error_code, std::system_category());

        // 端口冲突特殊提示
        if (error_code == EADDRINUSE || error_code == 10048) // EADDRINUSE on Linux, 10048 on Windows
        {
            NSDK_LOG_ERROR("❌ 端口绑定失败: 端口 %d 已被占用！", m_nPort);
            NSDK_LOG_ERROR("   可能原因: 另一个程序或服务已在使用此端口");
            NSDK_LOG_ERROR("   解决方案: 1) 停止占用端口的程序  2) 使用其他端口启动服务");
        }
        else
        {
            NSDK_LOG_ERROR("❌ 端口绑定失败: %s (错误码: %d)", ec.message().c_str(), error_code);
        }

        m_running = false;
        return -1;
    }

	NSDK_LOG_DEBUG("端口绑定成功，开始监听...");
	m_running = true;
	m_serverThread = std::thread([this]()
	{
		NSDK_LOG_DEBUG("HTTP服务线程已启动");
		bool ret = m_server.listen_after_bind();

		if (!ret)
		{
			int error_code = errno;
			std::error_code ec(error_code, std::system_category());
			NSDK_LOG_ERROR("listen_after_bind失败: %s (错误码: %d)", ec.message().c_str(), error_code);
		}

		m_running = false;
		NSDK_LOG_INFO("SDK HTTP服务器已停止");
	});

	int max_retries = 50; // 最多等待 50 * 10ms = 0.5秒
    while (!m_server.is_running() && max_retries > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        max_retries--;
    }

	if (m_server.is_running())
    {
        NSDK_LOG_INFO("✅ SDK HTTP服务器启动成功! 监听地址: %s:%d", DEFAULT_HTTP_SERVER_HOST, m_nPort);
        nRet = 0; // 成功
    }
    else
    {
        NSDK_LOG_ERROR("❌ 服务器启动失败!");
        NSDK_LOG_ERROR("   状态检查: is_running() 返回 false");
        NSDK_LOG_ERROR("   可能原因: 1) 端口已被占用  2) 监听失败  3) 线程启动异常");
        NSDK_LOG_ERROR("   当前端口: %d", m_nPort);

        // 如果启动失败，需要清理线程
        m_server.stop();
        if(m_serverThread.joinable()) m_serverThread.join();
        m_running = false;
        nRet = -1;
    }

	return nRet;
}

int CSdkHttpServer::StopServer()
{
	if (!m_running)
	{
		return 0;
	}

	m_running = false;
	m_server.stop();
	if (m_serverThread.joinable())
	{
		m_serverThread.join();
	}
	NSDK_LOG_DEBUG("SDK服务已停止");

	return 0;
}

void CSdkHttpServer::InitSession()
{
	NSDK_LOG_DEBUG("会话管理相关初始化");
	// 登录接口
	m_server.Post(TVAPI_PATH_BASIC_LOGIN, [](const httplib::Request& req, httplib::Response& res)
	{
    	CSessionManager::instance()->HttpCommandLogin(req, res);
	});

	// 注销接口
	m_server.Post(TVAPI_PATH_BASIC_LOGOUT, [](const httplib::Request& req, httplib::Response& res)
	{
    	CSessionManager::instance()->HttpCommandLout(req, res);
	});

	// 保活接口
	m_server.Get(TVAPI_PATH_BASIC_KEEPLIVE, [](const httplib::Request& req, httplib::Response& res)
	{
    	CSessionManager::instance()->HttpCommandKeepAlive(req, res);
	});

	// 报警监听推送接口
	m_server.Get(TVAPI_PATH_ALARMEVENT_LISTEN, [](const httplib::Request& req, httplib::Response& res)
    {
        CSessionManager::instance()->HttpCommandAlarmListen(req, res);
    });
}