/**
 * @file DeviceManage.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-10
 * 
 * @brief 登录设备管理
 */
#include "DeviceManage.h"
#include "NetTVSDKHttpUrl.h"
#include "SDKConvert.h"
#include "NetSdkLog.h"
#include "NetTVSDKClientInterface.h"

using namespace tvsdk;

/**
 * @brief 构造函数
 * @details 初始化设备管理器，设置初始状态为未初始化，用户ID计数器从1开始
 */
CDeviceManage::CDeviceManage() : isInitialized_(false),
								idCounter_(1)
{

}

/**
 * @brief 析构函数
 * @details 清理所有会话资源，调用Cleanup()停止所有线程和释放会话
 */
CDeviceManage::~CDeviceManage()
{
	NSDK_LOG_INFO("~CDeviceManage() ");
    Cleanup();
}

/**
 * @brief 登录设备
 * @param [IN] host 设备IP地址
 * @param [IN] port 设备端口号
 * @param [IN] user 用户名
 * @param [IN] pass 密码
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
												hbInterval_, maxRetry_,connectTimeout_,receiveTimeout_,lostCallback);
    if (session->ConnectAndLogin()) 
	{
        session->StartHeartbeat();
        std::lock_guard<std::mutex> lock(mapMutex_);
        sessions_[hUser] = session;
        
		NSDK_LOG_INFO("LogIn sucessful! Device: %s:%d UserHand: %p", host.c_str(),port, hUser);

        return hUser;
    }

	NSDK_LOG_INFO("LogIn  ERROR Device: %s:%d", host.c_str(),port);
    return NULL;
}


/**
 * @brief 注销设备登录
 * @param [IN] pHandle 用户登录句柄
 * @return 成功返回true，失败返回false
 * @details 从会话列表中移除并停止指定会话，释放资源
 */
bool CDeviceManage::Logout(LPUSER_HANDLE pHandle) 
{
    std::shared_ptr<CUserSession> session = nullptr;

    {
        std::lock_guard<std::mutex> lock(mapMutex_);
        auto it = sessions_.find(pHandle);
        if (it != sessions_.end()) 
		{
            session = it->second;
            sessions_.erase(it);
        }
    }

    if (session) 
	{
        session->Stop();

		NSDK_LOG_INFO("LogOut sucessful! UserHand: %p", pHandle);

        return true;
    }
    return false;
}

/**
 * @brief 生成用户ID
 * @return 用户ID
 * @details 使用原子计数器生成唯一用户ID，线程安全
 */
int CDeviceManage::GenerateUserId() 
{
    return idCounter_.fetch_add(1);
}


/**
 * @brief 生成用户句柄
 * @return 用户句柄
 * @details 将原子计数器值转换为指针类型作为句柄，线程安全
 */
LPUSER_HANDLE CDeviceManage::GenerateHandle() 
{
	int id = idCounter_.fetch_add(1);
	return (LPUSER_HANDLE)(intptr_t)id;
}
/**
 * @brief 获取用户会话
 * @param [IN] pHandle 用户登录句柄
 * @return 成功返回会话指针，失败返回nullptr
 * @details 根据句柄从会话列表中查找对应的会话，线程安全
 */
std::shared_ptr<CUserSession> CDeviceManage::GetSession(LPUSER_HANDLE pHandle) 
{
    NSDK_LOG_INFO("[DeviceManage] GetSession called, handle=%p (as int=%d)", pHandle, (int)(intptr_t)pHandle);
	std::lock_guard<std::mutex> lock(mapMutex_);
	auto it = sessions_.find(pHandle);
    if (it != sessions_.end()) {
        NSDK_LOG_INFO("[DeviceManage] Session found for handle=%p", pHandle);
        return it->second;
    } else {
        NSDK_LOG_WARN("[DeviceManage] Session NOT found for handle=%p", pHandle);
        return nullptr;
    }
}

/**
 * @brief 设置全局接收超时时间
 * @param [IN] timeoutSec 超时时间（秒）
 */
void CDeviceManage::SetGlobalRevTimeout(int timeoutSec) 
{
    receiveTimeout_ = timeoutSec;
}

/**
 * @brief 设置全局连接参数
 * @param [IN] waitTime 等待时间（秒）
 * @param [IN] tryTimes 尝试次数
 * @details 设置心跳间隔、最大重试次数和连接超时时间
 */
void CDeviceManage::SetGlobalConnectTime(int waitTime, int tryTimes) 
{
    hbInterval_ = waitTime;
    maxRetry_ = tryTimes;
    connectTimeout_ = waitTime;  // 同时设置连接超时
}

/**
 * @brief 会话丢失回调处理
 * @param [IN] pHandle 用户登录句柄
 * @details 仅记录日志，不删除session。session会通过ReconnectLoop自动重连，
 *          重连成功后仍在map中可继续使用。若在此删除session，重连成功后handle将无法找到对应session
 */
void CDeviceManage::OnSessionLost(LPUSER_HANDLE pHandle) 
{
//     NSDK_LOG_INFO("Handling session lost for User: %p", pHandle);

//    std::lock_guard<std::mutex> lock(threadMutex_);
    
//     // 清理已经结束的线程，防止列表无限增长
//     for (auto it = workThreads_.begin(); it != workThreads_.end(); ) {
//         if (it->joinable()) {
//              // 这里的检查比较棘手，std::thread 没有直接的 "is_finished" 接口。
//              // 简单的做法是：仅在 Cleanup 时统一 join。
//              // 或者如果这是一个长时间运行的服务，建议引入标志位或 future。
//              // 鉴于这是客户端 SDK，且掉线是低频事件，我们可以暂不通过复杂的逻辑清理，
//              // 重点解决 Crash 问题。
//              ++it; 
//         } else {
//             // 如果线程不可 join（理论上存进去的都是可 join 的），移除
//             it = workThreads_.erase(it);
//         }
//     }

//     // 创建线程并存入列表
//     workThreads_.emplace_back([this, pHandle]() 
//     {
//         // 缩短等待时间，或者如果逻辑允许，去掉 sleep
//         // std::this_thread::sleep_for(std::chrono::milliseconds(100)); 

//         std::shared_ptr<CUserSession> sessionToRelease = nullptr;
//         {
//             // 检查单例是否还在初始化状态（可选，增加安全性）
//             if (!this->isInitialized_) return;

//             std::lock_guard<std::mutex> lock(mapMutex_);
//             auto it = sessions_.find(pHandle);
//             if (it != sessions_.end()) 
//             {
//                 sessionToRelease = it->second; 
//                 sessions_.erase(it);
//             }
//         }

//         if (sessionToRelease) 
//         {
//             // Stop 可能会阻塞等待 SSE 线程退出，这是安全的
//             sessionToRelease->Stop(); 
//             NSDK_LOG_INFO("User %p removed from manager.", pHandle);
//         }
//     });
    // 仅记录日志，不删除 session
    // session 自己会通过 ReconnectLoop 重连，重连成功后仍然在 map 里
    // 如果在此删除 session，重连成功后 handle 就找不到对应的 session，所有命令调用失败
    NSDK_LOG_WARN("[DeviceManage] Session lost notification for User-%p (session remains in map for reconnect)", pHandle);
}

/**
 * @brief 清理所有资源
 * @details 设置初始化状态为false，等待工作线程结束，停止所有会话并释放资源
 */
void CDeviceManage::Cleanup() 
{
	isInitialized_ = false;
	NSDK_LOG_INFO("DeviceManage Cleanup executing...");

	{
        std::lock_guard<std::mutex> lock(threadMutex_);
        for (auto& t : workThreads_) {
            if (t.joinable()) {
                t.join(); // 等待线程结束
            }
        }
        workThreads_.clear();
    }
	
	std::map<LPUSER_HANDLE, std::shared_ptr<CUserSession>> dyingSessions;
	{
		std::lock_guard<std::mutex> lock(mapMutex_);
		dyingSessions.swap(sessions_);
	}

	for (auto& kv : dyingSessions) 
	{
		if (kv.second) kv.second->Stop();
	}
	dyingSessions.clear();
}