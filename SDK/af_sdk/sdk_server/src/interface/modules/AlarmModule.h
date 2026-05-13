#pragma once
#include "NetTVSDKServerInterface.h"

// 前向声明
class SessionModule;

/**
 * @brief 告警推送管理模块
 * @details 负责告警信息的推送和统计
 */
class AlarmModule
{
public:
    /**
     * @brief 构造函数
     * @param pSessionModule 会话模块指针（用于获取会话信息）
     */
    explicit AlarmModule(SessionModule* pSessionModule);
    ~AlarmModule();

    // 禁止拷贝
    AlarmModule(const AlarmModule&) = delete;
    AlarmModule& operator=(const AlarmModule&) = delete;

    /**
     * @brief 推送告警信息到所有客户端
     * @param pAlarmer 告警设备信息
     * @param lCommand 命令码（报警类型）
     * @param pAlarmInfo 具体告警结构体指针
     * @param dwBufLen pAlarmInfo 长度
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL PushAlarmInfo(NET_TV_ALARMER_S* pAlarmer,
                      INT32 lCommand,
                      LPVOID pAlarmInfo,
                      INT32 dwBufLen);

    /**
     * @brief 推送通道上下线状态到所有客户端
     * @param pChannelInfo 通道状态信息
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL PushChannelStatusInfo(NET_TV_CHANNEL_INFO_S* pChannelInfo);

    /**
     * @brief 获取告警推送总次数
     * @return 推送次数
     */
    INT64 GetPushCount() const;

    /**
     * @brief 重置推送计数
     */
    void ResetPushCount();

private:
    SessionModule* m_pSessionModule;  // 会话模块引用
    INT64 m_pushCount;              // 推送计数
};
