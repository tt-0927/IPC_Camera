/**
 * @FilePath     : web_server.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-10-08 13:46:32
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-10-20 15:00:39
 * @Description  : WebSocket服务器类
 */

#pragma once

#include "task_manage.h"
#include "Singleton.h"
#include "IpcRet.h"
#include "WebSocketServer.h"
#include "IOBase.h"

#include <atomic>

/* 连接信息结构体 */
typedef struct _ConnectionInfo_
{
    void *pHandle = nullptr; /* 通信句柄 */
    std::string ip;          /* 通信IP */
    int userId = 0;          /* 在线用户ID，未登录时为0 */
} ConnectionInfo_S;

/* 待清理的连接信息 */
typedef struct _PendingCleanup_
{
    std::string ip;        /* IP地址 */
    int userId = 0;        /* 用户ID */
    time_t disconnectTime; /* 断开时间 */
} PendingCleanup_S;

class CWebServer : public CSingleton<CWebServer>
{
    CWebServer() = default;
public:
    ~CWebServer() = default;
    friend class CSingleton<CWebServer>;

    IpcRet_E init();
    IpcRet_E deinit();
    void set_taskManage(std::shared_ptr<CTaskManage> pTaskManage);
    int send(const void *pData, int nDataLen, int nActionCode, void *pHandle = nullptr);
    void set_heartbeat(const void *pData, size_t nLength);
    void deal_heartbeat(Net::Message_S& stMessage, Net::UserParam_S &stUserParam);
    void deal_status(Net::Message_S& stMessage, Net::UserParam_S &stUserParam);
    void deal_message(Net::Message_S& stMessage, Net::UserParam_S &stUserParam);

    /**
     * @brief 获取登录设备客户端的ip
     * @return std::string 客户端IP
     */
    std::string get_loginclient_ip();

    /**
     * @brief 获取使用者设备客户端的ip
     * @return std::string 客户端IP
     */
    std::string get_userclient_ip();

    void set_statusObserver(Common::StatusCallback observer);

    /**
     * @brief   : 检查指定IP是否存在活跃的已登录连接
     * @param    {std::string} ip：客户端IP
     * @return   {bool} true：存在活跃已登录连接，false：不存在
     */
    bool hasActiveLoggedInConnectionByIp(const std::string &ip);

    /**
     * @brief   : 按IP移除待下线记录
     * @param    {std::string} ip：客户端IP
     * @return   {void}
     */
    void clearPendingCleanupByIp(const std::string &ip);

private:
    /**
     * @brief   : 启动清理连接信息线程
     */
    void startCleanupThread();

    /**
     * @brief   : 停止清理连接信息线程
     */
    void stopCleanupThread();

    /**
     * @brief   : 清理连接信息线程函数
     */
    void cleanupThreadFunc();

    /**
     * @brief   : 为不同任务构建结果回调
     * @param    {int} nActionCode：任务码
     * @return   {Task::ResultCallback} 任务结果回调
     */
    Task::ResultCallback buildResultCallback(int nActionCode);

    /**
     * @brief   : 登录结果发送前立即绑定在线用户ID
     * @param    {void} *pData：发送数据
     * @param    {int} nDataLen：数据长度
     * @param    {int} nActionCode：任务码
     * @param    {void} *pHandle：websocket连接句柄
     * @return   {int} 0：沿用发送结果，非0：发送失败时的返回值
     */
    int sendWithLoginBind(const void *pData, int nDataLen, int nActionCode, void *pHandle);

    /**
     * @brief   : 添加websocket连接句柄、IP至 句柄 -> 连接信息映射管理
     * @param    {void*} pHandle websocket连接句柄
     * @param    {string&} ip 连接的IP地址
     */
    void addConnection(void* pHandle, const std::string& ip);

    /**
     * @brief   : 从句柄 -> 连接信息映射管理移除websocket连接句柄
     * @param    {void*} pHandle websocket连接句柄
     */
    void removeConnection(void* pHandle);

    /**
     * @brief   : 为连接关联用户ID，并取消该ID的待下线状态
     * @param    {void} *pHandle：websocket连接句柄
     * @param    {int} userId：在线用户ID
     * @return   {bool} true：关联成功，false：关联失败
     */
    bool setUserIdForConnection(void *pHandle, const int userId);

    /**
     * @brief   : 为连接取消关联用户ID，并取消该ID的待下线状态
     * @param    {void} *pHandle websocket连接句柄
     * @param    {int} userId 在线用户ID
     */
    void deleteUserIdForConnection(void *pHandle, const int userId);

    /**
     * @brief   : 检查指定用户ID是否还有其他活动连接
     * @param   {int} userId 要检查的用户ID
     * @return  {bool} true 如果存在其他连接 false 如果不存在
     */
    bool hasOtherConnections(const int userId);

    /**
     * @brief   : 处理用户真正下线
     * @param    {int} userId 在线用户ID
     * @param    {string} &ip 连接的IP地址
     */
    void handleUserOffline(const int userId, const std::string &ip);

    /**
     * @brief   : 用户下线后，关闭对讲/广播任务
     * @param    {string} &ip 下线用户的IP地址
     */
    void closeAudio(const std::string &ip);

private:
    std::shared_ptr<Net::IOBase> m_pHandler = nullptr;
    std::shared_ptr<CTaskManage> m_pTaskManage = nullptr;
    std::string m_heartbeat;

    /* 登录设备IP */
    std::string m_LoginDeviceIp;
    /* 使用者的设备IP */
    std::string m_UserDeviceIp;

    Common::StatusCallback m_statusObserver;

    /* 在线连接管理 */
    /* 句柄 -> 连接信息映射管理 */
    std::map<void *, ConnectionInfo_S> m_mapConnections;
    /* 待清理的连接信息 */
    std::vector<PendingCleanup_S> m_vecPendingCleanup;
    /* 连接管理互斥锁 */
    std::mutex m_connectionMutex;
    /* 清理线程 */
    std::thread m_cleanupThread;
    /* 停止清理标志 */
    std::atomic<bool> m_bStopCleanup;
    /* 清理检查间隔(秒) */
    const int CLEANUP_CHECK_INTERVAL = 1;
    /* 重连宽限期(秒) */
    const int RECONNECT_GRACE_PERIOD = 3;
};
