/**
 * @file AlarmModule.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CAlarmModule 模块接口与类型定义
 * 功能说明：
 * 1. 声明 CAlarmModule 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#pragma once
#include "NetTVSDKServerInterface.h"

/* 前向声明 */
class CSessionModule;

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 告警推送管理模块
 * @details 负责告警信息的推送和统计
 */
class CAlarmModule
{
public:
    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 构造函数
     * @param pSessionModule 会话模块指针（用于获取会话信息）
     */
    explicit CAlarmModule(CSessionModule* pSessionModule);
    ~CAlarmModule();

    /* 禁止拷贝 */
    CAlarmModule(const CAlarmModule&) = delete;
    CAlarmModule& operator=(const CAlarmModule&) = delete;

    /**
 * @author tianl (tianl@kfb.cn)
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
 * @author tianl (tianl@kfb.cn)
     * @brief 推送通道上下线状态到所有客户端
     * @param pChannelInfo 通道状态信息
     * @return TRUE表示成功，FALSE表示失败
     */
    BOOL PushChannelStatusInfo(NET_TV_CHANNEL_INFO_S* pChannelInfo);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 获取告警推送总次数
     * @return 推送次数
     */
    INT64 GetPushCount() const;

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 重置推送计数
     */
    void ResetPushCount();

private:
    CSessionModule* m_pSessionModule;  /* 会话模块引用 */
    INT64 m_lPushCount;              /* 推送计数 */
};
