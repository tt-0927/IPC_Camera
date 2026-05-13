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

    // 注销
    bool Logout(LPUSER_HANDLE pHandle);
    void Cleanup();

    void SetInitialized(bool bInit) {isInitialized_ = bInit;}
    bool IsInitialized() const {return isInitialized_;}

    std::shared_ptr<CUserSession> GetSession(LPUSER_HANDLE pHandle);

    void SetGlobalRevTimeout(int timeoutSec);
    void SetGlobalConnectTime(int waitTime, int tryTimes);
    // void SetExceptionCallBack(NET_TV_ExceptionCallBack_PF cb, LPVOID pUser);

private:
    int GenerateUserId();
    LPUSER_HANDLE GenerateHandle();
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




