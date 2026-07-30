/**
 * @FilePath     : tvsdk_server.h
 * @Description  : TVSDK 服务端封装，对接 NetTVSDKServer.h 的 C 接口
 */

#pragma once

#include <memory>
#include <string>
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
     * @param lCommand 命令码/报警类型（见 NetTVSDKServer.h 中 NET_ALARM_*）
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
    std::shared_ptr<CTaskManage> m_pTaskManage;
    bool m_bInit = false;
    std::string m_strUser;
    std::string m_strPassword;
    unsigned int m_udwPort = 0;  /* 0 表示使用 IN_CONTROL_SDK_PROT */
};
