/*
 * @FilePath     : sdk_new/sdk_server/src/business/Common/DeviceBusiness.h
 * @Author       : ITC
 * @Date         : 2026-08-19
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-19
 * @Description  : 设备级业务处理模块（通用，跨设备共用）
 *                 收口设备级操作（非配置读写、非硬件控制、非能力集查询）：
 *                 1. 日志查询/导出（NET_FIND_LOG/NET_EXPORT_LOG）
 *                 2. 升级状态/版本查询、设置升级（NET_GET_UPGRADESTATUS/NET_GET_UPGRADEVERSION/NET_SET_UPGRADE）
 *                 参考海康 NET_DVR_FindLog/NET_DVR_Upgrade、大华 CLIENT_QueryDeviceLog/CLIENT_Upgrade，
 *                 均为独立于 GetDevConfig/SetDevConfig 的设备级操作。
 */

#pragma once

#include <string>
#include <cstring>

#include "Singleton.h"
#include "NetTVSDKServerInterface.h"
#include "NetTVConfigCbExecute.h"
#include "DeviceInfoConvert.h"
#include "SDKConvert.h"
#include "NetSdkLog.h"
#include "UrlParamUtils.h"
#include "NetTVSDKHttpUrl.h"

/**
 * @brief 设备级业务处理类
 * @details 单例模式，处理通用设备级操作（日志查询/导出、升级状态/版本/设置）
 */
class CDeviceBusiness : public CSingleton<CDeviceBusiness>
{
    CDeviceBusiness() {} /* 私有构造函数 */
public:
    ~CDeviceBusiness() {}
    friend class CSingleton<CDeviceBusiness>;

public:
    /**
     * @brief 查询/导出设备日志 (NET_FIND_LOG/NET_EXPORT_LOG)
     * @details 从URL参数解析查询条件（类型、动作、时间范围、页码），
     *          调用 executeGetDevConfigCb 获取日志列表。
     *          参考海康 NET_DVR_FindLog、大华 CLIENT_QueryDeviceLog。
     * @param nChannelId 通道号（设备级操作，仅用于响应）
     * @param nCommand 命令码（NET_FIND_LOG 或 NET_EXPORT_LOG）
     * @param url_param URL参数（Type/Action/StartTime/EndTime/CurPage/PageSize）
     * @return JSON格式的响应数据
     */
    std::string HandleGetLogList(INT32 nChannelId, INT32 nCommand, const std::string& url_param);

    /**
     * @brief 获取设备升级状态 (NET_GET_UPGRADESTATUS)
     * @details 查询当前固件升级进度/结果，参考海康 NET_DVR_GetUpgradeState
     * @param nChannelId 通道号（设备级操作，仅用于响应）
     * @param nCommand 命令码
     * @return JSON格式的响应数据
     */
    std::string HandleGetUpgradeStatus(INT32 nChannelId, INT32 nCommand);

    /**
     * @brief 获取设备升级版本信息 (NET_GET_UPGRADEVERSION)
     * @details 查询设备固件版本信息
     * @param nChannelId 通道号（设备级操作，仅用于响应）
     * @param nCommand 命令码
     * @return JSON格式的响应数据
     */
    std::string HandleGetUpgradeVersion(INT32 nChannelId, INT32 nCommand);

    /**
     * @brief 设置设备升级 (NET_SET_UPGRADE)
     * @details 解析JSON请求体中的升级信息，触发设备固件升级。
     *          参考海康 NET_DVR_Upgrade、大华 CLIENT_Upgrade。
     * @param nChannelId 通道号（设备级操作）
     * @param nCommand 命令码
     * @param req_data 请求数据（JSON格式，含升级文件路径等信息）
     * @return JSON格式的响应数据
     */
    std::string HandleSetUpgrade(INT32 nChannelId, INT32 nCommand, const std::string& req_data);
};
