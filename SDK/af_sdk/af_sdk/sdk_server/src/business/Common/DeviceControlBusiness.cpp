/*
 * @FilePath     : sdk_new/sdk_server/src/business/Common/DeviceControlBusiness.cpp
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 * @Description  : 设备控制业务模块实现
 */

#include "DeviceControlBusiness.h"

#include <cstring>

namespace
{
/**
 * @brief 校验设备控制参数有效性
 * @param stInfo 设备控制参数结构体
 * @return NET_E_SUCCEED 成功，NET_E_INVALID_PARAM 参数无效
 */
int ValidateDeviceControlInfo(const NET_DeviceControlInfo_S& stInfo)
{
    /* 基础字段合法性校验 */
    if (stInfo.uChannelID <= 0 ||
        stInfo.uControlType <= 0 ||
        stInfo.uCommand <= 0 ||
        stInfo.uDurationMs < 0)
    {
        return NET_E_INVALID_PARAM;
    }

    /* 云台控制需要校验速度等级范围 */
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

    /* 请求体为空判断 */
    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM);
    }

    /* 解析JSON请求为结构体 */
    NET_DeviceControlInfo_S stInfo;
    std::memset(&stInfo, 0, sizeof(stInfo));

    if (!SDKConvert::from_string(req_data, stInfo))
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM);
    }

    /* 兼容旧版本未设置结构体大小的情况 */
    if (stInfo.uSize == 0)
    {
        stInfo.uSize = sizeof(NET_DeviceControlInfo_S);
    }

    /* 参数合法性校验 */
    int nValidCode = ValidateDeviceControlInfo(stInfo);
    if (nValidCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("DeviceControl request invalid: channel=%d, controlType=%d, nCommand=%d, speed=%d, durationMs=%d",
                      stInfo.uChannelID,
                      stInfo.uControlType,
                      stInfo.uCommand,
                      stInfo.uSpeed,
                      stInfo.uDurationMs);
        return SDKConvert::to_respString(nValidCode, stInfo.uCommand, stInfo);
    }

    /* 记录业务事件日志 */
    NETSDK_LOG_MESSAGE_INFO("DeviceControl request: channel=%d, controlType=%d, nCommand=%d, speed=%d, durationMs=%d",
                  stInfo.uChannelID,
                  stInfo.uControlType,
                  stInfo.uCommand,
                  stInfo.uSpeed,
                  stInfo.uDurationMs);

    /* 执行设备控制回调（由上层注册） */
    int nRespCode = executeDeviceControlCb(&stInfo);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("DeviceControl callback failed, ret=%d", nRespCode);
    }

    /* 构造响应并返回 */
    return SDKConvert::to_respString(nRespCode, stInfo.uCommand, stInfo);
}
