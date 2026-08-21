/**
 * @file DeviceManage.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-10
 * 
 * @brief 登录设备管理
 */
#pragma once
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <mutex>
#include <functional>
#include <memory>
#include <algorithm>

#include "UserSession.h"
#include "Singleton.h"

class CDeviceManage : public CSingleton<CDeviceManage>
{
    CDeviceManage();
public:
	
	~CDeviceManage();    
	friend class CSingleton<CDeviceManage>;

public:

    /**
     * @brief 登录接口 
     * @return 成功返回 User登录句柄
     */
    LPUSER_HANDLE Login(const std::string& host, int port, 
              const std::string& user, const std::string& pass);

    /**
     * @brief 注销设备登录
     * @param [IN] pHandle 用户登录句柄
     * @return 成功返回true，失败返回false
     */
    bool Logout(LPUSER_HANDLE pHandle);

    /**
     * @brief 清理所有资源
     * @details 停止所有线程，释放所有会话
     */
    void Cleanup();

    /**
     * @brief 设置SDK初始化状态
     * @param [IN] bInit 是否已初始化
     */
    void SetInitialized(bool bInit) {isInitialized_ = bInit;}

    /**
     * @brief 获取SDK初始化状态
     * @return 已初始化返回true，未初始化返回false
     */
    bool IsInitialized() const {return isInitialized_;}

    /**
     * @brief 获取用户会话
     * @param [IN] pHandle 用户登录句柄
     * @return 成功返回会话指针，失败返回nullptr
     */
    std::shared_ptr<CUserSession> GetSession(LPUSER_HANDLE pHandle);

    /**
     * @brief 设置全局接收超时时间
     * @param [IN] timeoutSec 超时时间（秒）
     */
    void SetGlobalRevTimeout(int timeoutSec);

    /**
     * @brief 设置全局连接参数
     * @param [IN] waitTime 等待时间（秒）
     * @param [IN] tryTimes 尝试次数
     */
    void SetGlobalConnectTime(int waitTime, int tryTimes);
    // void SetExceptionCallBack(NET_TV_ExceptionCallBack_PF cb, LPVOID pUser);

private:
    /**
     * @brief 生成用户ID
     * @return 用户ID
     */
    int GenerateUserId();

    /**
     * @brief 生成用户句柄
     * @return 用户句柄
     */
    LPUSER_HANDLE GenerateHandle();

    /**
     * @brief 会话丢失回调处理
     * @param [IN] pHandle 用户登录句柄
     */
    void OnSessionLost(LPUSER_HANDLE pHandle);

private:
    std::mutex mapMutex_; 
    std::map<LPUSER_HANDLE, std::shared_ptr<CUserSession>> sessions_; // 使用智能指针管理生命周期
    std::atomic<int> idCounter_{0};
    std::atomic<bool> isInitialized_;

    std::mutex threadMutex_;
    std::list<std::thread> workThreads_;

    int connectTimeout_ = 5;      // 默认连接超时 5s
    int receiveTimeout_ = 60;     // 默认接收超时 60s
    int hbInterval_ = 5;          // 默认保活间隔 5s
    int maxRetry_ = 3;            // 默认重试次数 3次
};




