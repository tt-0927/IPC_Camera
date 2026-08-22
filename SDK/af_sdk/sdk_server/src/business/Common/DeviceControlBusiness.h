/*
 * @FilePath     : sdk_new/sdk_server/src/business/Common/DeviceControlBusiness.h
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 * 
 * @brief DeviceControlBusiness 模块接口与类型定义
 * 功能说明：
 * 1. 声明 DeviceControlBusiness 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 * @Description  : 设备控制业务模块接口定义
 */

#pragma once

#include <string>

#include "Singleton.h"
#include "NetTVSDKServerInterface.h"
#include "NetTVDeviceCbExecute.h"
#include "DeviceInfoConvert.h"
#include "SDKConvert.h"
#include "NetSdkLog.h"

/**
 * @brief 设备控制业务类
 * @details 单例模式，提供设备硬件控制（云台、声光、雨刷、重启、重置等）的业务处理
 */
class CDeviceControlBusiness : public CSingleton<CDeviceControlBusiness>
{
    CDeviceControlBusiness() {}
public:
    ~CDeviceControlBusiness() {}
    friend class CSingleton<CDeviceControlBusiness>;

public:
    /**
     * @brief 处理设备控制请求
     * @param req_data 请求JSON数据，包含NET_DeviceControlInfo_S字段
     * @param url_param URL附加参数（预留）
     * @return 响应JSON字符串，包含处理结果码和结构体数据
     */
    std::string DeviceControl(const std::string& req_data, const std::string& url_param);
};
