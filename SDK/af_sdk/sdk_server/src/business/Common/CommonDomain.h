/*
 * @FilePath     : sdk_new/sdk_server/src/business/Common/CommonDomain.h
 * @Author       : chenchl
 * @Date         : 2026-08-20
 * @LastEditors  : chenchl
 * @LastEditTime : 2026-08-20
 * @Description  : 通用配置域（全设备通用，所有 BG 群编译时必含）
 *                 收口通用命令：设备基本信息、存储信息、NTP、网络、安全服务、SSH、
 *                 日志查询/服务器、升级状态/版本/设置、音频配置。
 *                 这些命令无论录播/NVR/编码器都需要，不属于任何部门独有。
 *                 特殊处理函数 HandleGetDeviceBasicInfo/HandleSetDeviceBasicInfo/HandleGetStorageInfo
 *                 走设备回调（NetTVDeviceCb），不走配置回调（NetTVConfigCb）。
 */
#pragma once

#include "ConfigDomainBase.h"
#include "Singleton.h"

/**
 * 通用配置域
 * @details 单例，注册通用设备级命令，查表分发。
 *          无论哪个 BG 群编译，此域始终编入。
 */
class CCommonDomain : public CConfigDomainBase, public CSingleton<CCommonDomain>
{
    friend class CSingleton<CCommonDomain>;
    CCommonDomain();
public:
    ~CCommonDomain() {}

private:
    /* ===== 特殊处理函数（走设备回调，不走配置回调） ===== */

    /* 获取设备基本信息（通用身份：型号/序列号/固件/MAC等） */
    static std::string HandleGetDeviceBasicInfo(INT32 nChannelId, INT32 nCommand,
                                                const std::string& req_data,
                                                const std::string& url_param);

    /* 设置设备基本信息（仅 strDeviceName 可写） */
    static std::string HandleSetDeviceBasicInfo(INT32 nChannelId, INT32 nCommand,
                                                const std::string& req_data,
                                                const std::string& url_param);

    /* 获取设备存储信息（NVR/录播等有硬盘的设备专用） */
    static std::string HandleGetStorageInfo(INT32 nChannelId, INT32 nCommand,
                                            const std::string& req_data,
                                            const std::string& url_param);
};
