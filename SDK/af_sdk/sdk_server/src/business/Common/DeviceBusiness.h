/**
 * @file DeviceBusiness.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief DeviceBusiness 模块接口与类型定义
 * 功能说明：
 * 1. 声明 DeviceBusiness 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#pragma once
#include <string>
#include <functional>

#include "NetTVDeviceCbExecute.h"
#include "DeviceInfoConvert.h"
#include "SDKConvert.h"
#include "NetSdkLog.h"
#include "Singleton.h"

class CDeviceBusiness : public CSingleton<CDeviceBusiness>
{

	CDeviceBusiness()
    {

    }
public:

	~CDeviceBusiness()
    {

    }
	friend class CSingleton<CDeviceBusiness>;

public:

    /* 设备信息查询 */
    std::string GetDeviceInfo(const std::string& req_data, const std::string& url_param);


    private:

};
