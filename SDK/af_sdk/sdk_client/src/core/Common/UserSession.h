/**
 * @file UserSession.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief UserSession 模块接口与类型定义
 * 功能说明：
 * 1. 声明 UserSession 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#pragma once

#include <tvsdkhttplib.h>
#include <string>
#include <thread>
#include <atomic>
#include <map>
#include <mutex>
#include <memory>
#include <iostream>

#include "NetTVSDKClientInterface.h"
#include "BG6_ZHSJ/ClientAlarmManager.h"

using namespace tvsdk;

/* 用户句柄类型 */
typedef void* LPUSER_HANDLE;

/* 会话断开回调函数类型 */
using OnSessionLostCallback = std::function<void(LPUSER_HANDLE)>;

struct CommandRequest_S
{
    /* 基础信息 */
    std::string method;         /* "GET", "POST", "PUT", "DELETE" */
    std::string url;            /* 基础 URL */

    /* 数据部分 */
    std::string jsonBody;       /* JSON 字符串 body */
    std::map<std::string, std::string> queryParams; /* URL 查询参数 ?key=val */

    /* 二进制拓展 (如果需要上传文件) */
    const char* binData = nullptr;
    size_t binSize = 0;

    /* 构造函数简化使用 */
    CommandRequest_S(const std::string& m, const std::string& u) : method(m), url(u) {}
    CommandRequest_S() = default;
};

class CUserSession : public std::enable_shared_from_this<CUserSession>
{
public:
    CUserSession(LPUSER_HANDLE userHand,  const std::string& host, int port,
                 const std::string& user, const std::string& pass,
                 int hbInterval, int maxRetry,
                 int connectTimeout, int receiveTimeout,
                 OnSessionLostCallback callback);
    ~CUserSession();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 初始化并验证登录（带Digest认证）
     * @return 成功返回true，失败返回false
     */
    bool ConnectAndLogin();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 启动心跳线程
     */
    void StartHeartbeat();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 停止会话（线程安全）
     * @details 停止所有线程，关闭客户端连接，设置离线状态
     */
    void Stop();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 重连循环（内部使用）
     * @details 使用指数退避策略重新登录，成功后恢复心跳和报警监听
     */
    void ReconnectLoop();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 发送请求到设备
     * @param [in] req 请求参数
     * @param [out] outRespBody 响应体输出
     * @return 成功返回true，失败返回false
     */
    bool SendRequest(const CommandRequest_S& req, std::string& outRespBody);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 设置报警回调函数
     * @param [in] cb 报警回调函数指针
     * @param [in] userData 用户数据
     */
    void SetAlarmCallback(NET_AlarmCallBack cb, void* userData);

    /**
     * @brief 设置动态图片 V2 告警回调函数。
     * @param [in] pCallback V2 告警回调函数。
     * @param [in] pUserData 回调用户数据。
     * @return 无。
     */
    void SetAlarmCallbackV2(NET_AlarmCallBackV2 pCallback, void* pUserData);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 设置通道状态回调函数
     * @param [in] cb 通道状态回调函数指针
     * @param [in] userData 用户数据
     */
    void SetChannelStatusCallback(NET_ChannelStatusCallBack cb, void* userData);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 开始监听报警消息
     * @return 成功返回true，失败返回false
     */
    bool StartAlarmListen();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 停止监听报警消息
     * @return 成功返回true，失败返回false
     */
    bool StopAlarmListen();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 获取在线状态
     * @return 在线返回true，离线返回false
     */
    bool IsOnline() const { return m_bOnline; }

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 获取用户句柄
     * @return 用户句柄
     */
    LPUSER_HANDLE GetUserId() const { return m_hUser; }

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 获取会话ID
     * @return 会话ID
     */
    std::string GetSessionId() const { return m_strSessionId; }

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 获取设备主机地址
     * @return 主机地址
     */
    std::string GetHost() const { return m_strHost; }

private:
    /**
 * @author tianl (tianl@kfb.cn)
     * @brief SSE长连接循环线程函数（备用心跳方式）
     */
    void SseLoop();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 心跳保活循环线程函数
     * @details 定时发送心跳包检测连接状态
     */
    void HeartbeatLoop();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 报警监听由ClientAlarmManager管理
     */
    /* AlarmLoop managed by ClientAlarmManager */

private:
    LPUSER_HANDLE m_hUser;
    std::string m_strHost;
    int m_nPort;
    std::string m_strUsername;
    std::string m_strPassword;
    std::string m_strSessionId;

    int m_nHeartbeatInterval;
    int m_nMaxRetry;

    std::atomic<bool> m_bRunning{false};
    std::atomic<bool> m_bOnline{false};

    /* 命令发送锁 (保护 CmdClient 串行发送) */
    std::mutex m_stCommandMutex;

    std::thread m_stSseThread;
    std::thread m_stHeartbeatThread;

    int m_nConnectionTimeout;      /* 连接超时（秒） */
    int m_nReceiveTimeout;     /* 接收超时（秒） */

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 双客户端隔离设计
     */
    std::unique_ptr<httplib::Client> m_pCommandClient; /* 短连接：发命令 */
    std::unique_ptr<httplib::Client> m_pSseClient; /* 长连接：SSE心跳 */

    OnSessionLostCallback m_fnSessionLostCallback;        /* 会话断开 回调处理 */


    /* Reconnect members */
    std::atomic<bool> m_bReconnecting{false};  /* 重连中标志 */
    std::atomic<int> m_nReconnectDelay{1};       /* 当前重连延迟（秒） */
    std::thread m_stReconnectThread;              /* 重连线程 */
    std::mutex m_stReconnectMutex;                /* 重连互斥锁 */

    /* Alarm members */
    std::shared_ptr<CClientAlarmManager> m_pAlarmManager;
};
