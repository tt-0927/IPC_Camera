/**
 * @file DeviceCapabilityBusiness.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-01-30
 *
 * @brief 设备能力集业务实现
 */

#include "DeviceCapabilityBusiness.h"
#include "NetTVSDKHttpUrl.h"

std::string CDeviceCapabilityBusiness::GetDeviceCapability(const std::string& req_data, const std::string& url_param)
{
    NETSDK_LOG_MESSAGE_DEBUG("GetDeviceCapability url_param: %s", url_param.c_str());

    // 解析URL参数
    int channelId = ParseIntParam(url_param, NET_API_PARAM_CHANNEL, NET_API_PARAM_NVRCHN);
    int command = ParseIntParam(url_param, NET_API_PARAM_COMMAND, 0);

    switch (command)
    {
        case NET_CAP_VIDEO_ENCODE:
            return HandleVideoEncode(channelId, command);

        // 后续扩展
        case NET_CAP_OSD:
            return HandleOsd(channelId, command);
        // case NET_CAP_SMART:
        //     return HandleSmart(channelId);
        // case NET_CAP_IMAGE:
        //     return HandleImage(channelId);
        case NET_CAP_AUDIO:
             return HandleAudioEncode(channelId, command);

        default:
            NETSDK_LOG_MESSAGE_WARN("Unsupported capability command: %d", command);
            return SDKConvert::to_respString(NET_E_CMD_NOT_SUPPORT, command);
    }
}

std::string CDeviceCapabilityBusiness::HandleVideoEncode(int channelId, int command)
{
    int nRespCode = NET_E_FAILED;
    NET_VideoEncodeCap_S stCap;
    memset(&stCap, 0, sizeof(NET_VideoEncodeCap_S));

    nRespCode = NetSDK_ExecuteCb_GetVideoEncodeCap(channelId, &stCap);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_DEBUG("视频编码能力集回调执行失败! ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, command, channelId, stCap);
}

std::string CDeviceCapabilityBusiness::HandleAudioEncode(int channelId, int command)
{
    int nRespCode = NET_E_FAILED;
    NET_AudioCap_S stCap;
    memset(&stCap, 0, sizeof(NET_AudioCap_S));

    nRespCode = NetSDK_ExecuteCb_GetAudioCap(channelId, &stCap);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_DEBUG("音频编码能力集回调执行失败! ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, command, channelId, stCap);
}

std::string CDeviceCapabilityBusiness::HandleOsd(int channelId, int command)
{
    int nRespCode = NET_E_FAILED;
    NET_OsdCap_S stCap;
    memset(&stCap, 0, sizeof(NET_OsdCap_S));

    nRespCode = NetSDK_ExecuteCb_GetOsdCap(channelId, &stCap);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_DEBUG("OSD能力集回调执行失败! ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, command, channelId, stCap);
}

int CDeviceCapabilityBusiness::ParseIntParam(const std::string& url_param, const std::string& key, int defaultVal)
{
    // 简单解析 "key=value" 格式
    std::string searchKey = key + "=";
    size_t pos = url_param.find(searchKey);
    if (pos == std::string::npos)
    {
        return defaultVal;
    }

    size_t valueStart = pos + searchKey.length();
    size_t valueEnd = url_param.find('&', valueStart);
    if (valueEnd == std::string::npos)
    {
        valueEnd = url_param.length();
    }

    std::string valueStr = url_param.substr(valueStart, valueEnd - valueStart);
    try
    {
        return std::stoi(valueStr);
    }
    catch (...)
    {
        return defaultVal;
    }
}
