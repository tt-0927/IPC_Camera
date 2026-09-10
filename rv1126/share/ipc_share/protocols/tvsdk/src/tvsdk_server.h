/**
 * @FilePath     : tvsdk_server.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-23 11:17:50
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-13 18:07:12
 * @Description  : TVSDK 服务端封装，对接 NetTVSDKServer.h 的 C 接口
 */

#pragma once

#include <mutex>
#include <memory>
#include <string>

#include "NetTVSDKServer.h"
#include "IpcRet.h"


class CTaskManage;

class CTvSdkServer
{
public:
    CTvSdkServer() = default;
    ~CTvSdkServer() = default;

    /**
     * @brief 初始化 TVSDK 服务端（端口、鉴权、注册能力集回调）
     * @return 0 成功，负值失败
     */
    int init();

    /**
     * @brief 去初始化 TVSDK 服务端
     */
    void deinit();

    /**
     * @brief 设置任务管理句柄，用于 Get/Set 配置时走 Task 分发
     */
    void set_taskManage(std::shared_ptr<CTaskManage> pTaskManage);

    /**
     * @brief 推送告警信息给已连接的 SDK 客户端
     * @param pAlarmer 告警设备信息（可为 nullptr，内部会填默认）
     * @param lCommand 命令码/报警类型（见 NetTVSDKCommon.h 中 NET_ALARM_*）
     * @param pAlarmInfo 告警结构体指针，类型由 lCommand 决定
     * @param dwBufLen pAlarmInfo 长度
     * @return 0 成功，负值失败
     */
    int push_alarm(const void *pAlarmer, int lCommand, const void *pAlarmInfo, int dwBufLen);

    /**
     * @brief 获取当前 TVSDK 在线客户端数量（活跃会话数）
     * @return >=0 客户端数量，<0 表示失败
     */
    int get_client_count() const;
    
    /**
     * @brief 是否已初始化
     */
    bool is_init() const { return m_bInit; }

    /**
     * @brief 设置服务端端口（0 表示使用 IN_CONTROL_SDK_PROT）
     */
    void set_port(unsigned int port);

    /**
     * @brief 设置鉴权用户名与密码
     */
    void set_user_passwd(const std::string &user, const std::string &passwd);

private:
    /**
     * @brief   : 在 TVSDK 启动阶段准备告警设备信息缓存
     * @return  : 无
     * @note    : 设备序列号、MAC 和 IP 在告警推送期间保持不变，避免每次告警执行网络命令。
     */
    void prepare_default_alarmer();

    /**
     * @brief 将录像下载任务的异步进度转换为 TVSDK 481 通知。
     */
    void subscribe_record_download_progress();

    std::shared_ptr<CTaskManage> m_pTaskManage;
    /* 已注册 481 订阅的任务管理器，用于避免同一实例重复注册回调。 */
    CTaskManage *m_pRecordDownloadSubscribeManage = nullptr;
    bool m_bInit = false;
    std::string m_strUser;
    std::string m_strPassword;
    unsigned int m_udwPort = 0;  /* 0 表示使用 IN_CONTROL_SDK_PROT */
    /* lock: 保护默认告警设备信息缓存，避免初始化与告警线程并发读取时产生数据竞争 */
    mutable std::mutex m_mtxDefaultAlarmer;
    /* memory: 仅保存固定长度的设备信息，不携带告警图片等大块数据 */
    NET_Alarmer_S m_stDefaultAlarmer{};
    /* 默认告警设备信息是否已经在启动阶段准备完成 */
    bool m_bDefaultAlarmerReady = false;
};
