/**
 * @file DeviceCapabilityBusiness.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-01-30
 *
 * @brief 设备能力集业务实现
 */

#include "DeviceCapabilityBusiness.h"
#include "NetTVSDKHttpUrl.h"
#include "BG6_ZHSJ/BU_SJGZ/IpcBusiness.h"

std::string CDeviceCapabilityBusiness::GetDeviceCapability(const std::string& req_data, const std::string& url_param)
{
    NETSDK_LOG_MESSAGE_DEBUG("GetDeviceCapability url_param: %s", url_param.c_str());

    // 解析URL参数
    int nChannelId = ParseIntParam(url_param, NET_API_PARAM_CHANNEL, NET_API_PARAM_NVRCHN);
    int nCommand = ParseIntParam(url_param, NET_API_PARAM_COMMAND, 0);

    switch (nCommand)
    {
        case NET_CAP_VIDEO_ENCODE:
            return HandleVideoEncode(nChannelId, nCommand);

        // 后续扩展
        case NET_CAP_OSD:
            return CIpcBusiness::instance()->HandleOsd(nChannelId, nCommand);
        // case NET_CAP_SMART:
        //     return HandleSmart(nChannelId);
        // case NET_CAP_IMAGE:
        //     return HandleImage(nChannelId);
        case NET_CAP_AUDIO:
             return HandleAudioEncode(nChannelId, nCommand);

        default:
            NETSDK_LOG_MESSAGE_WARN("Unsupported capability nCommand: %d", nCommand);
            return SDKConvert::to_respString(NET_E_CMD_NOT_SUPPORT, nCommand);
    }
}

std::string CDeviceCapabilityBusiness::HandleVideoEncode(int nChannelId, int nCommand)
{
    if (nChannelId < 0)
    {
        NETSDK_LOG_MESSAGE_WARN("HandleVideoEncode: invalid channel=%d", nChannelId);
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    int nRespCode = NET_E_FAILED;
    NET_VideoEncodeCap_S stCap;
    memset(&stCap, 0, sizeof(NET_VideoEncodeCap_S));

    nRespCode = executeGetVideoEncodeCapCb(nChannelId, &stCap);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_DEBUG("视频编码能力集回调执行失败! ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, nCommand, nChannelId, stCap);
}

std::string CDeviceCapabilityBusiness::HandleAudioEncode(int nChannelId, int nCommand)
{
    if (nChannelId < 0)
    {
        NETSDK_LOG_MESSAGE_WARN("HandleAudioEncode: invalid channel=%d", nChannelId);
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    int nRespCode = NET_E_FAILED;
    NET_AudioCap_S stCap;
    memset(&stCap, 0, sizeof(NET_AudioCap_S));

    nRespCode = executeGetAudioCapCb(nChannelId, &stCap);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_DEBUG("音频编码能力集回调执行失败! ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, nCommand, nChannelId, stCap);
}
