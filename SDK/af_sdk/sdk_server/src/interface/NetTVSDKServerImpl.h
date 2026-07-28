/**
 * @file NetTVSDKServerImpl.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief NetTVSDKServerImpl 模块接口与类型定义
 * 功能说明：
 * 1. 声明 NetTVSDKServerImpl 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#pragma once
#include <memory>
#include "NetTVSDKServerInterface.h"

/* 前向声明模块类 */
class CServerModule;
class CSessionModule;
class CRouteModule;
class CAlarmModule;
class CDiscoveryResponder;

/**
 * @author tianl (tianl@kfb.cn)
 * @brief Server实现类 - PIMPL协调器
 * @details 管理所有功能模块，协调模块间的交互
 */
class CNetTVSDKServerImpl
{
public:
    CNetTVSDKServerImpl();
    ~CNetTVSDKServerImpl();

    /* 禁止拷贝 */
    CNetTVSDKServerImpl(const CNetTVSDKServerImpl&) = delete;
    CNetTVSDKServerImpl& operator=(const CNetTVSDKServerImpl&) = delete;

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 初始化SDK服务器
     * @param udwPort 服务器端口号
     * @param szUserName 用户名
     * @param szPassword 密码
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL DoInit(UINT32 udwPort, CHAR szUserName[NET_LEN_132], CHAR szPassword[NET_LEN_132]);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 清理SDK服务器资源
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL DoCleanup();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 设置日志
     * @param dwLogLevel 日志级别
     * @param strLogDir 日志目录
     * @param dwLogFileSize 日志文件大小
     * @param dwLogFileNum 日志文件数量
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL DoSetLogToFile(INT32 dwLogLevel, CHAR* strLogDir, INT32 dwLogFileSize, INT32 dwLogFileNum);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 获取SDK版本
     * @return 版本号
     */
    INT32 DoGetSDKVersion();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 获取当前在线客户端数量（活跃会话数）
     * @return 客户端数量
     */
    INT32 DoGetClientCount();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 设置用户名密码
     * @param szUserName 用户名
     * @param szPassword 密码
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL DoSetUserPasswd(CHAR szUserName[NET_LEN_132], CHAR szPassword[NET_LEN_132]);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 推送告警信息
     * @param pAlarmer 告警设备信息
     * @param lCommand 命令码
     * @param pAlarmInfo 告警信息指针
     * @param dwBufLen 缓冲区长度
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL DoPushAlarmInfo(NET_Alarmer_S* pAlarmer, INT32 lCommand, LPVOID pAlarmInfo, INT32 dwBufLen);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 推送通道上下线状态
     * @param pChannelInfo 通道信息
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL DoPushChannelStatusInfo(NET_ChannelInfo_S* pChannelInfo);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 注册设备发现信息回调
     */
    BOOL DoRegisterCb_GetDiscoveryDeviceInfo(NET_CB_GetDiscoveryDeviceInfo cbFunc);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 启动设备发现响应服务
     */
    BOOL DoDiscoveryStart(const CHAR* szInterfaceName);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 停止设备发现响应服务
     */
    BOOL DoDiscoveryStop();

private:
    bool m_bInitialized;  /* 初始化标志 */

    /* 模块智能指针（PIMPL第二层） */
    std::unique_ptr<CServerModule> m_pServerModule;
    std::unique_ptr<CSessionModule> m_pSessionModule;
    std::unique_ptr<CRouteModule> m_pRouteModule;
    std::unique_ptr<CAlarmModule> m_pAlarmModule;

    /* 设备发现 */
    std::unique_ptr<CDiscoveryResponder> m_pDiscoveryResponder;
    NET_CB_GetDiscoveryDeviceInfo m_fnDiscoveryDeviceInfoCallback{nullptr};
};
