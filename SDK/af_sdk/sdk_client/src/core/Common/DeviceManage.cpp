/**
 * @file DeviceManage.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief DeviceManage 模块实现
 * 功能说明：
 * 1. 实现 DeviceManage 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#include "DeviceManage.h"
#include "NetTVSDKHttpUrl.h"
#include "SDKConvert.h"
#include "NetSdkLog.h"
#include "NetTVSDKClientInterface.h"

using namespace tvsdk;

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 构造函数
 * @details 初始化设备管理器，设置初始状态为未初始化，用户ID计数器从1开始
 */
CDeviceManage::CDeviceManage() : m_bInitialized(false),
								m_nNextUserId(1)
{

}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 析构函数
 * @details 清理所有会话资源，调用Cleanup()停止所有线程和释放会话
 */
CDeviceManage::~CDeviceManage()
{
	NETSDK_LOG_MESSAGE_INFO("~CDeviceManage() ");
    Cleanup();
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 登录设备
 * @param [in] host 设备IP地址
 * @param [in] port 设备端口号
 * @param [in] user 用户名
 * @param [in] pass 密码
 * @return 成功返回用户句柄，失败返回NULL
 * @details 创建用户会话，连接设备并登录，启动心跳线程，将会话存入管理列表
 */
LPUSER_HANDLE CDeviceManage::Login(const std::string& host, int port,
                         const std::string& user, const std::string& pass)
{
    LPUSER_HANDLE hUser = GenerateHandle();
	auto lostCallback = [this](LPUSER_HANDLE h)
	{
        this->OnSessionLost(h);
    };
    auto session = std::make_shared<CUserSession>(hUser, host, port, user, pass,
												m_nHeartbeatInterval, m_nMaxRetry,m_nConnectionTimeout,m_nReceiveTimeout,lostCallback);
    if (session->ConnectAndLogin())
	{
        session->StartHeartbeat();
        std::lock_guard<std::mutex> lock(m_stDeviceMapMutex);
        m_stSessions[hUser] = session;

		NETSDK_LOG_MESSAGE_INFO("LogIn sucessful! Device: %s:%d UserHand: %p", host.c_str(),port, hUser);

        return hUser;
    }

	NETSDK_LOG_MESSAGE_INFO("LogIn  ERROR Device: %s:%d", host.c_str(),port);
    return NULL;
}


/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注销设备登录
 * @param [in] pHandle 用户登录句柄
 * @return 成功返回true，失败返回false
 * @details 从会话列表中移除并停止指定会话，释放资源
 */
bool CDeviceManage::Logout(LPUSER_HANDLE pHandle)
{
    std::shared_ptr<CUserSession> session = nullptr;

    {
        std::lock_guard<std::mutex> lock(m_stDeviceMapMutex);
        auto it = m_stSessions.find(pHandle);
        if (it != m_stSessions.end())
		{
            session = it->second;
            m_stSessions.erase(it);
        }
    }

    if (session)
	{
        session->Stop();

		NETSDK_LOG_MESSAGE_INFO("LogOut sucessful! UserHand: %p", pHandle);

        return true;
    }
    return false;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 生成用户ID
 * @return 用户ID
 * @details 使用原子计数器生成唯一用户ID，线程安全
 */
int CDeviceManage::GenerateUserId()
{
    return m_nNextUserId.fetch_add(1);
}


/**
 * @author tianl (tianl@kfb.cn)
 * @brief 生成用户句柄
 * @return 用户句柄
 * @details 将原子计数器值转换为指针类型作为句柄，线程安全
 */
LPUSER_HANDLE CDeviceManage::GenerateHandle()
{
	int id = m_nNextUserId.fetch_add(1);
	return (LPUSER_HANDLE)(intptr_t)id;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 获取用户会话
 * @param [in] pHandle 用户登录句柄
 * @return 成功返回会话指针，失败返回nullptr
 * @details 根据句柄从会话列表中查找对应的会话，线程安全
 */
std::shared_ptr<CUserSession> CDeviceManage::GetSession(LPUSER_HANDLE pHandle)
{
    NETSDK_LOG_MESSAGE_INFO("[DeviceManage] GetSession called, handle=%p (as int=%d)", pHandle, (int)(intptr_t)pHandle);
	std::lock_guard<std::mutex> lock(m_stDeviceMapMutex);
	auto it = m_stSessions.find(pHandle);
    if (it != m_stSessions.end()) {
        NETSDK_LOG_MESSAGE_INFO("[DeviceManage] Session found for handle=%p", pHandle);
        return it->second;
    } else {
        NETSDK_LOG_MESSAGE_WARN("[DeviceManage] Session NOT found for handle=%p", pHandle);
        return nullptr;
    }
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 设置全局接收超时时间
 * @param [in] timeoutSec 超时时间（秒）
 */
void CDeviceManage::SetGlobalRevTimeout(int timeoutSec)
{
    m_nReceiveTimeout = timeoutSec;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 设置全局连接参数
 * @param [in] waitTime 等待时间（秒）
 * @param [in] tryTimes 尝试次数
 * @details 设置心跳间隔、最大重试次数和连接超时时间
 */
void CDeviceManage::SetGlobalConnectTime(int waitTime, int tryTimes)
{
    m_nHeartbeatInterval = waitTime;
    m_nMaxRetry = tryTimes;
    m_nConnectionTimeout = waitTime;  /* 同时设置连接超时 */
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 会话丢失回调处理
 * @param [in] pHandle 用户登录句柄
 * @details 仅记录日志，不删除session。session会通过ReconnectLoop自动重连，
 *          重连成功后仍在map中可继续使用。若在此删除session，重连成功后handle将无法找到对应session
 */
void CDeviceManage::OnSessionLost(LPUSER_HANDLE pHandle)
{
    /* 仅记录日志，不删除 session */
    /* session 自己会通过 ReconnectLoop 重连，重连成功后仍然在 map 里 */
    /* 如果在此删除 session，重连成功后 handle 就找不到对应的 session，所有命令调用失败 */
    NETSDK_LOG_MESSAGE_WARN("[DeviceManage] Session lost notification for User-%p (session remains in map for reconnect)", pHandle);
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 清理所有资源
 * @details 设置初始化状态为false，等待工作线程结束，停止所有会话并释放资源
 */
void CDeviceManage::Cleanup()
{
	m_bInitialized = false;
	NETSDK_LOG_MESSAGE_INFO("DeviceManage Cleanup executing...");

	{
        std::lock_guard<std::mutex> lock(m_stThreadMutex);
        for (auto& t : m_stWorkerThreads) {
            if (t.joinable()) {
                t.join(); /* 等待线程结束 */
            }
        }
        m_stWorkerThreads.clear();
    }

	std::map<LPUSER_HANDLE, std::shared_ptr<CUserSession>> dyingSessions;
	{
		std::lock_guard<std::mutex> lock(m_stDeviceMapMutex);
		dyingSessions.swap(m_stSessions);
	}

	for (auto& kv : dyingSessions)
	{
		if (kv.second) kv.second->Stop();
	}
	dyingSessions.clear();
}
