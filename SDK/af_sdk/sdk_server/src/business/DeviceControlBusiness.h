/**
 * @file DeviceControlBusiness.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief DeviceControlBusiness 模块接口与类型定义
 * 功能说明：
 * 1. 声明 DeviceControlBusiness 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */

#pragma once

#include <string>

#include "Singleton.h"
#include "NetTVSDKServerInterface.h"
#include "NetTVDeviceCbExecute.h"
#include "DeviceInfoConvert.h"
#include "SDKConvert.h"
#include "NetSdkLog.h"

class CDeviceControlBusiness : public CSingleton<CDeviceControlBusiness>
{
    CDeviceControlBusiness() {}
public:
    ~CDeviceControlBusiness() {}
    friend class CSingleton<CDeviceControlBusiness>;

public:
    std::string DeviceControl(const std::string& req_data, const std::string& url_param);
};
