/**
 * @file DeviceBusiness.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-04
 * 
 * @brief 设备通用业务
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

    // 设备信息查询
    std::string GetDeviceInfo(const std::string& req_data, const std::string& url_param);

  
    private:
 
};
