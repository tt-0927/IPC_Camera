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
     * @brief 推送 V2 告警信息。
     * @details 图片通过指针和实际长度传递，调用期间图片内存必须保持有效。
     * @param [in] pAlarmer 告警设备信息，为空时由服务端填充当前设备信息。
     * @param [in] lCommand 告警命令码。
     * @param [in] pAlarmInfo V2 告警结构体指针。
     * @param [in] dwBufLen V2 告警结构体长度。
     * @return 成功返回 OK，失败返回 ERR。
     */
    int push_alarm_v2(const void *pAlarmer, int lCommand, const void *pAlarmInfo, int dwBufLen);

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
#ifdef SCENE_INTELLIGENCE
    /**
     * @brief 注册智能抓拍事件订阅回调。
     * @details 订阅 IPC 任务管理器发布的人脸、行人、机动车和非机动车抓拍事件。
     * @return 无。
     */
    void register_capture_event_subscribers();

    /**
     * @brief 转发 IPC 抓拍事件到 TVSDK 客户端。
     * @param [in] pData IPC 发布的 JSON 数据。
     * @param [in] nDataLength IPC 发布数据长度。
     * @param [in] nActionCode IPC 动作码。
     * @param [in] pUserData 订阅回调用户数据，当前未使用。
     * @return 成功返回 OK，失败返回 ERR。
     */
    int handle_capture_event(const void *pData,
                             int nDataLength,
                             int nActionCode,
                             void *pUserData);

    bool m_bCaptureEventSubscribed = false;
#endif
    std::shared_ptr<CTaskManage> m_pTaskManage;
    bool m_bInit = false;
    std::string m_strUser;
    std::string m_strPassword;
    unsigned int m_udwPort = 0;  /* 0 表示使用 IN_CONTROL_SDK_PROT */
};
