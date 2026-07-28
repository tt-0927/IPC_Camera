/**
 * @file DeviceCapabilityBusiness.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief DeviceCapabilityBusiness 模块实现
 * 功能说明：
 * 1. 实现 DeviceCapabilityBusiness 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#include "DeviceCapabilityBusiness.h"
#include "NetTVSDKHttpUrl.h"
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 GetDeviceCapability 对应的数据。
 * @param [in] req_data 函数处理参数。
 * @param [in] url_param 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

std::string CDeviceCapabilityBusiness::GetDeviceCapability(const std::string& req_data, const std::string& url_param)
{
    NETSDK_LOG_MESSAGE_DEBUG("GetDeviceCapability url_param: %s", url_param.c_str());

    /* 解析URL参数 */
    int channelId = ParseIntParam(url_param, NET_TV_API_PARAM_CHANNEL, 1);
    int command = ParseIntParam(url_param, NET_TV_API_PARAM_COMMAND, 0);

    switch (command)
    {
        case NET_TV_CAP_VIDEO_ENCODE:
            return HandleVideoEncode(channelId);

        /* 后续扩展 */
        case NET_TV_CAP_OSD:
            return HandleOsd(channelId);
        /* case NET_TV_CAP_SMART: */
        /*     return HandleSmart(channelId); */
        /* case NET_TV_CAP_IMAGE: */
        /*     return HandleImage(channelId); */
        case NET_TV_CAP_AUDIO:
             return HandleAudioEncode(channelId);

        default:
            NETSDK_LOG_MESSAGE_WARN("Unsupported capability command: %d", command);
            return SDKConvert::to_respString(NET_TV_E_CMD_NOT_SUPPORT);
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 HandleVideoEncode 定义的内部处理。
 * @param [in] channelId 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

std::string CDeviceCapabilityBusiness::HandleVideoEncode(int channelId)
{
    int nRespCode = NET_TV_E_FAILED;
    NET_VideoEncodeCap_S stCap;
    memset(&stCap, 0, sizeof(NET_VideoEncodeCap_S));

    nRespCode = NetSDK_ExecuteCb_GetVideoEncodeCap(channelId, &stCap);
    if (nRespCode != NET_TV_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_DEBUG("视频编码能力集回调执行失败! ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, stCap);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 HandleAudioEncode 定义的内部处理。
 * @param [in] channelId 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

std::string CDeviceCapabilityBusiness::HandleAudioEncode(int channelId)
{
    int nRespCode = NET_TV_E_FAILED;
    NET_AudioCap_S stCap;
    memset(&stCap, 0, sizeof(NET_AudioCap_S));

    nRespCode = NetSDK_ExecuteCb_GetAudioCap(channelId, &stCap);
    if (nRespCode != NET_TV_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_DEBUG("音频编码能力集回调执行失败! ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, stCap);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 HandleOsd 定义的内部处理。
 * @param [in] channelId 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

std::string CDeviceCapabilityBusiness::HandleOsd(int channelId)
{
    int nRespCode = NET_TV_E_FAILED;
    NET_OsdCap_S stCap;
    memset(&stCap, 0, sizeof(NET_OsdCap_S));

    nRespCode = NetSDK_ExecuteCb_GetOsdCap(channelId, &stCap);
    if (nRespCode != NET_TV_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_DEBUG("OSD能力集回调执行失败! ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, stCap);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 ParseIntParam 对应的数据。
 * @param [in] url_param 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in] defaultVal 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

int CDeviceCapabilityBusiness::ParseIntParam(const std::string& url_param, const std::string& key, int defaultVal)
{
    /* 简单解析 "key=value" 格式 */
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
