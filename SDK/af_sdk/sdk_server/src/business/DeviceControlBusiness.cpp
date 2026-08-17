/**
 * @file DeviceControlBusiness.cpp
 * @brief Device control business implementation
 */

#include "DeviceControlBusiness.h"

#include <cstring>

namespace
{
int ValidateDeviceControlInfo(const NET_DeviceControlInfo_S& stInfo)
{
    if (stInfo.uChannelID <= 0 ||
        stInfo.uControlType <= 0 ||
        stInfo.uCommand <= 0 ||
        stInfo.uDurationMs < 0)
    {
        return NET_E_INVALID_PARAM;
    }

    if (stInfo.uControlType == NET_DEVICE_CTRL_TYPE_PTZ &&
        (stInfo.uSpeed < NET_MIN_PTZ_SPEED_LEVEL || stInfo.uSpeed > NET_MAX_PTZ_SPEED_LEVEL))
    {
        return NET_E_INVALID_PARAM;
    }

    return NET_E_SUCCEED;
}
}

std::string CDeviceControlBusiness::DeviceControl(const std::string& req_data, const std::string& url_param)
{
    (void)url_param;

    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM);
    }

    NET_DeviceControlInfo_S stInfo;
    std::memset(&stInfo, 0, sizeof(stInfo));

    if (!SDKConvert::from_string(req_data, stInfo))
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM);
    }

    if (stInfo.uSize == 0)
    {
        stInfo.uSize = sizeof(NET_DeviceControlInfo_S);
    }

    int nValidCode = ValidateDeviceControlInfo(stInfo);
    if (nValidCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("DeviceControl request invalid: channel=%d, controlType=%d, command=%d, speed=%d, durationMs=%d",
                      stInfo.uChannelID,
                      stInfo.uControlType,
                      stInfo.uCommand,
                      stInfo.uSpeed,
                      stInfo.uDurationMs);
        return SDKConvert::to_respString(nValidCode, stInfo.uCommand, stInfo);
    }

    NETSDK_LOG_MESSAGE_INFO("DeviceControl request: channel=%d, controlType=%d, command=%d, speed=%d, durationMs=%d",
                  stInfo.uChannelID,
                  stInfo.uControlType,
                  stInfo.uCommand,
                  stInfo.uSpeed,
                  stInfo.uDurationMs);

    int nRespCode = NetSDK_ExecuteCb_DeviceControl(&stInfo);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("DeviceControl callback failed, ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, stInfo.uCommand, stInfo);
}
