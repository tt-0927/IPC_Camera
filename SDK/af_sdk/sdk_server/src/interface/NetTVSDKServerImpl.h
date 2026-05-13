#pragma once
#include <memory>
#include "NetTVSDKServerInterface.h"

// 前向声明模块类
class ServerModule;
class SessionModule;
class RouteModule;
class AlarmModule;

/**
 * @brief Server实现类 - PIMPL协调器
 * @details 管理所有功能模块，协调模块间的交互
 */
class CNetTVSDKServerImpl
{
public:
    CNetTVSDKServerImpl();
    ~CNetTVSDKServerImpl();

    // 禁止拷贝
    CNetTVSDKServerImpl(const CNetTVSDKServerImpl&) = delete;
    CNetTVSDKServerImpl& operator=(const CNetTVSDKServerImpl&) = delete;

    /**
     * @brief 初始化SDK服务器
     * @param udwPort 服务器端口号
     * @param szUserName 用户名
     * @param szPassword 密码
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL DoInit(UINT32 udwPort, CHAR szUserName[NET_TV_LEN_132], CHAR szPassword[NET_TV_LEN_132]);

    /**
     * @brief 清理SDK服务器资源
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL DoCleanup();

    /**
     * @brief 设置日志
     * @param dwLogLevel 日志级别
     * @param strLogDir 日志目录
     * @param dwLogFileSize 日志文件大小
     * @param dwLogFileNum 日志文件数量
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL DoSetLogToFile(INT32 dwLogLevel, CHAR* strLogDir, INT32 dwLogFileSize, INT32 dwLogFileNum);

    /**
     * @brief 获取SDK版本
     * @return 版本号
     */
    INT32 DoGetSDKVersion();

    /**
     * @brief 获取当前在线客户端数量（活跃会话数）
     * @return 客户端数量
     */
    INT32 DoGetClientCount();

    /**
     * @brief 设置用户名密码
     * @param szUserName 用户名
     * @param szPassword 密码
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL DoSetUserPasswd(CHAR szUserName[NET_TV_LEN_132], CHAR szPassword[NET_TV_LEN_132]);

    /**
     * @brief 推送告警信息
     * @param pAlarmer 告警设备信息
     * @param lCommand 命令码
     * @param pAlarmInfo 告警信息指针
     * @param dwBufLen 缓冲区长度
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL DoPushAlarmInfo(NET_TV_ALARMER_S* pAlarmer, INT32 lCommand, LPVOID pAlarmInfo, INT32 dwBufLen);

    /**
     * @brief 推送通道上下线状态
     * @param pChannelInfo 通道信息
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL DoPushChannelStatusInfo(NET_TV_CHANNEL_INFO_S* pChannelInfo);

private:
    bool m_bInitialized;  // 初始化标志
    
    // 模块智能指针（PIMPL第二层）
    std::unique_ptr<ServerModule> m_pServerModule;
    std::unique_ptr<SessionModule> m_pSessionModule;
    std::unique_ptr<RouteModule> m_pRouteModule;
    std::unique_ptr<AlarmModule> m_pAlarmModule;
};
