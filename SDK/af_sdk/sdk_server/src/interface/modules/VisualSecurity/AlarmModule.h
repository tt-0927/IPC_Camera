/**
 * @brief 告警推送管理模块
 * @author chenchl
 * @date 2025-01-02
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : CAlarmModule.h
 * @Description  : 告警推送管理模块，负责告警信息的推送和统计
 */

#pragma once
#include "NetTVSDKServerInterface.h"

/* 前向声明 */
class CSessionModule;

/**
 * 告警推送管理模块类
 * 负责告警信息的推送和统计
 */
class CAlarmModule
{
public:
    /**
     * 构造函数
     * @param pSessionModule 会话模块指针（用于获取会话信息）
     */
    explicit CAlarmModule(CSessionModule* pSessionModule);

    /**
     * 析构函数
     */
    ~CAlarmModule();

    /* 禁止拷贝 */
    CAlarmModule(const CAlarmModule&) = delete;
    CAlarmModule& operator=(const CAlarmModule&) = delete;

    /**
     * 推送告警信息到所有客户端
     * @details 根据命令码解析告警信息类型（人脸比对、基础告警、规则告警、AI告警、交通告警、异常告警、统计告警等），
     *          将告警信息转换为JSON格式后推送到所有活跃会话客户端
     * @param pAlarmer 告警设备信息
     * @param lCommand 命令码（报警类型）
     * @param pAlarmInfo 具体告警结构体指针
     * @param dwBufLen pAlarmInfo 长度
     * @return TRUE表示成功推送到至少一个客户端，FALSE表示失败
     */
    BOOL PushAlarmInfo(NET_Alarmer_S* pAlarmer,
                      INT32 lCommand,
                      LPVOID pAlarmInfo,
                      INT32 dwBufLen);

    /**
     * @brief 推送动态图片 V2 告警到全部已订阅客户端。
     * @param [in] pAlarmer 告警设备信息。
     * @param [in] lCommand 告警命令码。
     * @param [in] pAlarmInfo V2 告警结构体。
     * @param [in] dwBufLen V2 告警结构体长度。
     * @return 至少推送到一个客户端时返回 TRUE，否则返回 FALSE。
     */
    BOOL PushAlarmInfoV2(NET_Alarmer_S* pAlarmer,
                         INT32 lCommand,
                         LPVOID pAlarmInfo,
                         INT32 dwBufLen);

    /**
     * 推送通道上下线状态到所有客户端
     * @details 将通道状态信息转换为JSON格式后推送到所有活跃会话客户端
     * @param pChannelInfo 通道状态信息
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL PushChannelStatusInfo(NET_ChannelInfo_S* pChannelInfo);

    /**
     * 获取告警推送总次数
     * @return 推送次数
     */
    INT64 GetPushCount() const;

    /**
     * 重置推送计数
     */
    void ResetPushCount();

private:
    CSessionModule* m_pSessionModule; /* 会话模块引用 */
    INT64 m_lPushCount; /* 推送计数 */
};
