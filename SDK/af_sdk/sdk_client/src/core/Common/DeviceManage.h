/**
 * @file DeviceManage.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief DeviceManage 模块接口与类型定义
 * 功能说明：
 * 1. 声明 DeviceManage 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
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
 * @author tianl (tianl@kfb.cn)
     * @brief 登录接口
     * @return 成功返回 User登录句柄
     */
    LPUSER_HANDLE Login(const std::string& host, int port,
              const std::string& user, const std::string& pass);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 注销设备登录
     * @param [in] pHandle 用户登录句柄
     * @return 成功返回true，失败返回false
     */
    bool Logout(LPUSER_HANDLE pHandle);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 清理所有资源
     * @details 停止所有线程，释放所有会话
     */
    void Cleanup();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 设置SDK初始化状态
     * @param [in] bInit 是否已初始化
     */
    void SetInitialized(bool bInit) {m_bInitialized = bInit;}

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 获取SDK初始化状态
     * @return 已初始化返回true，未初始化返回false
     */
    bool IsInitialized() const {return m_bInitialized;}

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 获取用户会话
     * @param [in] pHandle 用户登录句柄
     * @return 成功返回会话指针，失败返回nullptr
     */
    std::shared_ptr<CUserSession> GetSession(LPUSER_HANDLE pHandle);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 设置全局接收超时时间
     * @param [in] timeoutSec 超时时间（秒）
     */
    void SetGlobalRevTimeout(int timeoutSec);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 设置全局连接参数
     * @param [in] waitTime 等待时间（秒）
     * @param [in] tryTimes 尝试次数
     */
    void SetGlobalConnectTime(int waitTime, int tryTimes);
    /* void SetExceptionCallBack(NET_ExceptionCallBack_PF cb, LPVOID pUser); */

private:
    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 生成用户ID
     * @return 用户ID
     */
    int GenerateUserId();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 生成用户句柄
     * @return 用户句柄
     */
    LPUSER_HANDLE GenerateHandle();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 会话丢失回调处理
     * @param [in] pHandle 用户登录句柄
     */
    void OnSessionLost(LPUSER_HANDLE pHandle);

private:
    std::mutex m_stDeviceMapMutex;
    std::map<LPUSER_HANDLE, std::shared_ptr<CUserSession>> m_stSessions; /* 使用智能指针管理生命周期 */
    std::atomic<int> m_nNextUserId{0};
    std::atomic<bool> m_bInitialized;

    std::mutex m_stThreadMutex;
    std::list<std::thread> m_stWorkerThreads;

    int m_nConnectionTimeout = 5;      /* 默认连接超时 5s */
    int m_nReceiveTimeout = 60;     /* 默认接收超时 60s */
    int m_nHeartbeatInterval = 5;          /* 默认保活间隔 5s */
    int m_nMaxRetry = 3;            /* 默认重试次数 3次 */
};




