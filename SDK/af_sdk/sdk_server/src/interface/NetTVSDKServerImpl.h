#pragma once
#include <memory>
#include "NetTVSDKServerInterface.h"

// 前向声明模块类
class CServerModule;
class CSessionModule;
class CRouteModule;
class CAlarmModule;
class CDiscoveryResponder;

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
     * @param uPort 服务器端口号
     * @param strUserName 用户名
     * @param strPassword 密码
     * @param strDeviceName 设备名称
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL DoInit(UINT32 uPort, CHAR strUserName[NET_LEN_132], CHAR strPassword[NET_LEN_132], CHAR strDeviceName[NET_LEN_132]);

    /**
     * @brief 清理SDK服务器资源
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL DoCleanup();

    /**
     * @brief 设置日志
     * @param nLogLevel 日志级别
     * @param pstrLogDir 日志目录
     * @param nLogFileSize 日志文件大小
     * @param nLogFileNum 日志文件数量
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL DoSetLogToFile(INT32 nLogLevel, CHAR* pstrLogDir, INT32 nLogFileSize, INT32 nLogFileNum);

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
     * @param strUserName 用户名
     * @param strPassword 密码
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL DoSetUserPasswd(CHAR strUserName[NET_LEN_132], CHAR strPassword[NET_LEN_132]);

    /**
     * @brief 推送告警信息
     * @param pAlarmer 告警设备信息
     * @param nCommand 命令码
     * @param pAlarmInfo 告警信息指针
     * @param nBufLen 缓冲区长度
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL DoPushAlarmInfo(NET_Alarmer_S* pAlarmer, INT32 nCommand, LPVOID pAlarmInfo, INT32 nBufLen);

    /**
     * @brief 推送通道上下线状态
     * @param pChannelInfo 通道信息
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL DoPushChannelStatusInfo(NET_ChannelInfo_S* pChannelInfo);

    /**
     * @brief 注册设备发现信息回调
     */
    BOOL DoRegisterCb_GetDiscoveryDeviceInfo(NET_CB_GetDiscoveryDeviceInfo cbFunc);

    /**
     * @brief 注册免登录网络配置回调
     */
    BOOL DoRegisterCb_SetNetwork(NET_CB_SetNetwork cbFunc);

    /**
     * @brief 启动设备发现响应服务
     */
    BOOL DoDiscoveryStart(const CHAR* pstrInterfaceName);

    /**
     * @brief 停止设备发现响应服务
     */
    BOOL DoDiscoveryStop();

private:
    bool m_bInitialized;  // 初始化标志

    // 模块智能指针（PIMPL第二层）
    std::unique_ptr<CServerModule> m_pServerModule;
    std::unique_ptr<CSessionModule> m_pSessionModule;
    std::unique_ptr<CRouteModule> m_pRouteModule;
    std::unique_ptr<CAlarmModule> m_pAlarmModule;

    // 设备发现
    std::unique_ptr<CDiscoveryResponder> m_pDiscoveryResponder;
    NET_CB_GetDiscoveryDeviceInfo m_cbDiscoveryDeviceInfo{nullptr};
    NET_CB_SetNetwork m_cbSetNetwork{nullptr};
};
