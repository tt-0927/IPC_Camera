/**
 * @file DeviceControlBusiness.cpp
 * @brief Device control business implementation
 */

#include "DeviceControlBusiness.h"

#include <cstring>

namespace
{
int ValidateDeviceControlInfo(const NET_TV_DEVICE_CONTROL_INFO_S& stInfo)
{
    if (stInfo.dwChannelID <= 0 ||
        stInfo.dwControlType <= 0 ||
        stInfo.dwCommand <= 0 ||
        stInfo.dwDurationMs < 0)
    {
        return NET_TV_E_INVALID_PARAM;
    }

    if (stInfo.dwControlType == NET_TV_DEVICE_CTRL_TYPE_PTZ &&
        (stInfo.dwSpeed < NET_TV_MIN_PTZ_SPEED_LEVEL || stInfo.dwSpeed > NET_TV_MAX_PTZ_SPEED_LEVEL))
    {
        return NET_TV_E_INVALID_PARAM;
    }

    return NET_TV_E_SUCCEED;
}
}

std::string CDeviceControlBusiness::DeviceControl(const std::string& req_data, const std::string& url_param)
{
    (void)url_param;

    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_TV_E_INVALID_PARAM);
    }

    NET_TV_DEVICE_CONTROL_INFO_S stInfo;
    std::memset(&stInfo, 0, sizeof(stInfo));

    if (!SDKConvert::from_string(req_data, stInfo))
    {
        return SDKConvert::to_respString(NET_TV_E_INVALID_PARAM);
    }

    if (stInfo.dwSize == 0)
    {
        stInfo.dwSize = sizeof(NET_TV_DEVICE_CONTROL_INFO_S);
    }

    int nValidCode = ValidateDeviceControlInfo(stInfo);
    if (nValidCode != NET_TV_E_SUCCEED)
    {
        NSDK_LOG_WARN("DeviceControl request invalid: channel=%d, controlType=%d, command=%d, speed=%d, durationMs=%d",
                      stInfo.dwChannelID,
                      stInfo.dwControlType,
                      stInfo.dwCommand,
                      stInfo.dwSpeed,
                      stInfo.dwDurationMs);
        return SDKConvert::to_respString(nValidCode, stInfo);
    }

    NSDK_LOG_INFO("DeviceControl request: channel=%d, controlType=%d, command=%d, speed=%d, durationMs=%d",
                  stInfo.dwChannelID,
                  stInfo.dwControlType,
                  stInfo.dwCommand,
                  stInfo.dwSpeed,
                  stInfo.dwDurationMs);

    int nRespCode = NetSDK_ExecuteCb_DeviceControl(&stInfo);
    if (nRespCode != NET_TV_E_SUCCEED)
    {
        NSDK_LOG_WARN("DeviceControl callback failed, ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, stInfo);
}
