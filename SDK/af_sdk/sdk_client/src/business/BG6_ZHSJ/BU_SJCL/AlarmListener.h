/**
 * @file AlarmListener.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-08-31
 *
 * @brief AlarmListener 模块接口与类型定义。
 * 功能说明：
 * 1. 声明 AlarmListener 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 *
 * @par 修改记录
 * 2026-08-28 qinjt：补充抓拍图片生命周期说明并统一接口注释格式。
 * 2026-08-31 qinjt：补充监听线程和健康监控线程的安全停止约束。
 */
#pragma once

#include <tvsdkhttplib.h>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <memory>
#include <functional>
#include "NetTVSDKClientInterface.h"

using namespace tvsdk;

/**
 * @brief 管理客户端 AlarmListen 长连接、告警解析和抓拍图片生命周期。
 */
class CAlarmListener
{
public:
    /**
     * @brief 创建客户端告警监听器。
     * @param [in] strHost 设备主机地址。
     * @param [in] nPort 设备 HTTP 端口。
     * @param [in] strUser 用户名。
     * @param [in] strPass 密码。
     * @return 无返回值。
     */
    CAlarmListener(
        const std::string& strHost,
        int nPort,
        const std::string& strUser,
        const std::string& strPass);

    /**
     * @brief 销毁告警监听器并停止监听线程。
     * @param 无。
     * @return 无返回值。
     */
    ~CAlarmListener();

    /**
     * @brief 启动 AlarmListen 长连接。
     * @param [in] pUserHandle 用户句柄。
     * @param [in] strSessionId 当前登录会话标识。
     * @return true 表示线程启动成功，false 表示启动失败。
     */
    bool StartListen(void* pUserHandle, const std::string& strSessionId);

    /**
     * @brief 更新告警监听会话标识。
     * @param [in] strNewSessionId 重连后获得的新会话标识。
     * @return 无返回值。
     */
    void UpdateSessionId(const std::string& strNewSessionId)
    {
        std::lock_guard<std::mutex> stLock(m_stSessionIdMutex);
        m_strSessionId = strNewSessionId;
    }

    /**
     * @brief 主动中断当前报警连接，以便监听线程使用新会话重新建立连接。
     * @param 无。
     * @return 无返回值。
     */
    void ForceReconnect()
    {
        std::lock_guard<std::mutex> stLock(m_stClientMutex);
        if (m_pClient)
        {
            m_pClient->stop();
        }
    }

    /**
     * @brief 停止 AlarmListen 长连接和相关线程。
     * @param 无。
     * @return 无返回值。
     */
    void Stop();

    /**
     * @brief 查询告警监听线程是否正在运行。
     * @param 无。
     * @return true 表示正在运行，false 表示已停止。
     */
    bool IsRunning() const
    {
        return m_bRunning;
    }

    /**
     * @brief 会话过期回调类型。
     * @details 服务端返回 401 时通知上层重新登录。
     */
    using SessionExpiredCallback = std::function<void()>;
    /**
     * @brief 设置会话过期回调。
     * @param [in] fnCallback 会话过期时调用的回调函数。
     * @return 无返回值。
     */
    void SetSessionExpiredCallback(SessionExpiredCallback fnCallback)
    {
        m_fnSessionExpiredCallback = fnCallback;
    }
    /**
     * @brief 设置告警回调和回调用户数据。
     * @param [in] fnCallback 告警回调函数。
     * @param [in,out] pUserData 回调用户数据。
     * @return 无返回值。
     */
    void SetCallback(NET_AlarmCallBack fnCallback, void* pUserData)
    {
        m_fnAlarmCallback = fnCallback;
        m_pAlarmUserData = pUserData;
    }
    /**
     * @brief 设置通道状态回调和回调用户数据。
     * @param [in] fnCallback 通道状态回调函数。
     * @param [in,out] pUserData 回调用户数据。
     * @return 无返回值。
     */
    void SetChannelStatusCallback(
        NET_ChannelStatusCallBack fnCallback,
        void* pUserData)
    {
        m_fnChannelStatusCallback = fnCallback;
        m_pChannelStatusUserData = pUserData;
    }

private:
    /**
     * @brief 执行告警监听循环。
     * @param 无。
     * @return 无返回值。
     */
    void AlarmLoop();

    /**
     * @brief 监控告警连接健康状态并在必要时触发恢复。
     * @param 无。
     * @return 无返回值。
     */
    void HealthMonitorLoop();

private:
    /**
     * @brief 设备主机地址。
     */
    std::string m_strHost;

    /**
     * @brief 设备 HTTP 服务端口。
     */
    int m_nPort;

    /**
     * @brief 设备登录用户名。
     */
    std::string m_strUsername;

    /**
     * @brief 设备登录密码。
     */
    std::string m_strPassword;

    /**
     * @brief 当前告警监听使用的会话标识。
     */
    std::string m_strSessionId;
    /**
     * @brief 保护 m_strSessionId 的跨线程读写。
     */
    std::mutex m_stSessionIdMutex;

    /**
     * @brief 回调用户句柄，用于上层识别当前监听器。
     */
    void* m_hUser = nullptr;

    /**
     * @brief 保护 m_pClient 的跨线程访问。
     */
    std::mutex m_stClientMutex;

    /**
     * @brief 当前告警监听使用的 HTTP 客户端。
     */
    std::shared_ptr<httplib::Client> m_pClient;

    /**
     * @brief 告警监听线程对象。
     */
    std::thread m_stThread;

    /**
     * @brief 告警监听线程运行状态。
     */
    std::atomic<bool> m_bRunning{false};

    /**
     * @brief 告警回调函数。
     */
    NET_AlarmCallBack m_fnAlarmCallback = nullptr;

    /**
     * @brief 告警回调用户数据。
     */
    void* m_pAlarmUserData = nullptr;

    /**
     * @brief 通道状态回调函数。
     */
    NET_ChannelStatusCallBack m_fnChannelStatusCallback = nullptr;

    /**
     * @brief 通道状态回调用户数据。
     */
    void* m_pChannelStatusUserData = nullptr;

    /**
     * @brief 会话过期回调函数。
     */
    SessionExpiredCallback m_fnSessionExpiredCallback = nullptr;

    /**
     * @brief 连接健康监控统计信息。
     */
    std::atomic<int> m_nReceivedHeartbeatCount{0};
    std::atomic<int> m_nReceivedAlarmCount{0};
    std::atomic<int64_t> m_lLastDataTimeMilliseconds{0};
    std::atomic<bool> m_bFirstDataReceived{false};
    std::chrono::steady_clock::time_point m_stConnectionStartTime{};
    std::chrono::steady_clock::time_point m_stLastAlarmTime{};
    std::chrono::steady_clock::time_point m_stLastStatisticTime{};

    /**
     * @brief 健康监控线程及其运行状态。
     * @details 当 read_timeout 无法解除读取阻塞时，由该线程主动中断连接。
     */
    std::thread m_stHealthMonitorThread;
    std::atomic<bool> m_bHealthMonitorRunning{false};
    std::atomic<int> m_nReconnectCount{0};

    /**
     * @brief 健康监控恢复限流参数。
     */
    static constexpr int NETSDK_ALARM_MAX_RECOVERIES_PER_WINDOW = 5;
    static constexpr int NETSDK_ALARM_RECOVERY_WINDOW_SEC = 300;
    int m_nRecoveryCountInWindow = 0;
    std::chrono::steady_clock::time_point m_stRecoveryWindowStart{};
};
