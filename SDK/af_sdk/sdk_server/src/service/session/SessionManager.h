/**
 * @file SessionManager.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-08-31
 *
 * @brief SessionManager 模块接口与类型定义。
 * 功能说明：
 * 1. 声明 SDK 服务端会话管理模块的接口。
 * 2. 管理登录会话、AlarmListen 长连接和会话生命周期。
 * 3. 为告警推送提供线程安全的会话查询和消息分发入口。
 *
 * @par 修改记录
 * 2026-08-28 qinjt：统一会话管理接口的命名、注释和告警共享负载声明。
 * 2026-08-31 qinjt：补充共享告警负载按需创建的接口约束。
 */
#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <tvsdkhttplib.h>

#include "ServerSession.h"
#include "Singleton.h"

using namespace tvsdk;

/**
 * @brief 管理 SDK 服务端的登录会话和告警长连接。
 * @details 该类负责会话状态维护、超时清理、告警负载分发以及 HTTP
 *          登录、保活和 AlarmListen 请求处理。
 */
class CSessionManager : public CSingleton<CSessionManager>
{
private:
    /**
     * @brief 创建会话管理器并启动超时清理线程。
     * @param 无。
     * @return 无返回值。
     */
    CSessionManager();

public:
    /**
     * @brief 销毁会话管理器并等待超时清理线程退出。
     * @param 无。
     * @return 无返回值。
     */
    ~CSessionManager();

    friend class CSingleton<CSessionManager>;

    /**
     * @brief 启用指定会话的告警推送。
     * @param [in] strSessionId 待启用推送的会话标识。
     * @return true 表示启用成功，false 表示会话不存在或未登录。
     */
    bool EnablePush(const std::string& strSessionId);

    /**
     * @brief 清理已经超时或失活的会话。
     * @param 无。
     * @return 无返回值。
     */
    void CleanTimeoutSessions();

    /**
     * @brief 标记指定会话断开并清空其待发送告警队列。
     * @param [in] strSessionId 待标记的会话标识。
     * @return 无返回值。
     */
    void MarkDisconnected(const std::string& strSessionId);

    /**
     * @brief 将一条告警负载分发到所有符合条件的会话。
     * @param [in] strJson 告警 JSON 文本。
     * @param [in] aAttachments 告警 multipart 附件列表。
     * @return 成功入队的会话数量。
     */
    std::size_t PushToAll(
        const std::string& strJson,
        const std::vector<CServerSession::Attachment_S>& aAttachments = {});

    /**
     * @brief 获取当前会话数量。
     * @param 无。
     * @return 当前会话数量。
     */
    std::size_t GetSessionCount();

    /**
     * @brief 根据会话标识获取会话共享指针。
     * @param [in] strSessionId 待查询的会话标识。
     * @return 找到时返回会话共享指针，否则返回 nullptr。
     */
    std::shared_ptr<CServerSession> GetSession(const std::string& strSessionId);

    /**
     * @brief 获取所有会话的诊断状态信息。
     * @param 无。
     * @return 包含客户端 IP、登录、连接和订阅状态的诊断字符串。
     */
    std::string GetSessionDiagnosticInfo();

    /**
     * @brief 处理 HTTP 登录请求。
     * @param [in] req HTTP 请求对象。
     * @param [out] res HTTP 响应对象。
     * @return 无返回值。
     */
    void HttpCommandLogin(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief 处理 HTTP 注销请求。
     * @param [in] req HTTP 请求对象。
     * @param [out] res HTTP 响应对象。
     * @return 无返回值。
     */
    void HttpCommandLout(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief 处理 HTTP 会话保活请求。
     * @param [in] req HTTP 请求对象。
     * @param [out] res HTTP 响应对象。
     * @return 无返回值。
     */
    void HttpCommandKeepAlive(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief 处理 HTTP AlarmListen 长连接请求。
     * @param [in] req HTTP 请求对象。
     * @param [out] res HTTP 响应对象。
     * @return 无返回值。
     */
    void HttpCommandAlarmListen(const httplib::Request& req, httplib::Response& res);

private:
    /**
     * @brief 创建并登记一个新的登录会话。
     * @param [out] strSessionId 输出新会话标识。
     * @param [in] strClientIp 登录客户端 IP 地址。
     * @return true 表示创建成功，false 表示创建失败。
     */
    bool Login(std::string& strSessionId, const std::string& strClientIp = std::string());

    /**
     * @brief 注销并移除指定登录会话。
     * @param [in] strSessionId 待注销的会话标识。
     * @return true 表示注销成功，false 表示会话不存在。
     */
    bool Logout(const std::string& strSessionId);

    /**
     * @brief 生成新的会话标识。
     * @param 无。
     * @return 新生成的会话标识。
     */
    static std::string GenerateSessionId();

    /**
     * @brief 周期性执行会话超时清理。
     * @param 无。
     * @return 无返回值。
     */
    void CleanupLoop();

private:
    std::unordered_map<std::string, std::shared_ptr<CServerSession>> m_stSessions;
    std::mutex m_stMutex;
    std::thread m_stCleanerThread;
    std::atomic<bool> m_bRunning{false};

    /* 会话超时时间，单位为秒，默认值为五分钟。 */
    static constexpr int NETSDK_SESSION_TIMEOUT_SEC = 300;
};
