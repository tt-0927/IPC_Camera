/**
 * @file DeviceConfigBusiness.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : chenchl
 * @LastEditTime : 2026-08-20
 *
 * @brief 设备配置业务处理模块（一级路由）
 * @details 重构后不再包含巨大的 switch-case，改为"询问式路由"：
 *          依次询问各配置域（Common/SJGZ/SJCL）是否处理该命令码，
 *          第一个命中的域处理并返回。各域内部自己管理命令码→处理函数映射。
 *
 * 架构：
 *   DeviceConfigBusiness (一级路由，~15行)
 *     ├── CCommonDomain  (通用: 设备基础/存储/NTP/网络/日志/升级)
 *     ├── CSjgzDomain    (视频/报警/AI: 流/OSD/图像/报警/人脸/AI配置)
 *     └── CSjclDomain    (NVR/录播: 录像/通道/RTSP/文件)
 *
 * 各域继承 CConfigDomainBase，内部用 unordered_map<cmd, handler> 查表分发。
 * 条件编译：SJGZ/SJCL 域可通过 BU_SJGZ_EXCLUDE/BU_SJCL_EXCLUDE 宏排除。
 */
#pragma once

#include <string>
#include <cstring>

#include "Singleton.h"
#include "NetTVSDKServerInterface.h"
#include "NetSdkLog.h"
#include "UrlParamUtils.h"
#include <sstream>

/**
 * 设备配置业务处理类（一级路由）
 * @details 单例，解析 URL 参数后依次询问各域处理，不直接处理业务逻辑。
 */
class CDeviceConfigBusiness : public CSingleton<CDeviceConfigBusiness>
{
    CDeviceConfigBusiness() {}
public:
    ~CDeviceConfigBusiness() {}
    friend class CSingleton<CDeviceConfigBusiness>;

public:
    /**
     * 获取设备配置
     * @details 解析URL参数中的通道号和命令码，依次询问各域处理
     * @param req_data 请求数据（未使用）
     * @param url_param URL参数（包含channel和nCommand）
     * @return JSON格式的响应数据
     */
    std::string GetDevConfig(const std::string& req_data, const std::string& url_param);

    /**
     * 设置设备配置
     * @details 解析URL参数中的通道号和命令码，依次询问各域处理
     * @param req_data 请求数据（JSON格式，包含配置信息）
     * @param url_param URL参数（包含channel和nCommand）
     * @return JSON格式的响应数据
     */
    std::string SetDevConfig(const std::string& req_data, const std::string& url_param);
};
