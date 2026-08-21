/**
 * @file DeviceControlBusiness.h
 * @brief Device control business
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
