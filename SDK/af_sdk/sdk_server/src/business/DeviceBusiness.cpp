/**
 * @file DeviceBusiness.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief DeviceBusiness 模块实现
 * 功能说明：
 * 1. 实现 DeviceBusiness 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#include "DeviceBusiness.h"
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 GetDeviceInfo 对应的数据。
 * @param [in] req_data 函数处理参数。
 * @param [in] url_param 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

std::string CDeviceBusiness::GetDeviceInfo(const std::string& req_data, const std::string& url_param)
{
    int nRespCode = 0;
    std::string strResp;
    NET_DeviceInfo_S stInfo;
    memset(&stInfo, 0, sizeof(NET_DeviceInfo_S));

    nRespCode = NetSDK_ExecuteCb_DeviceInfo(&stInfo);
    if(nRespCode != 0)
    {
        NETSDK_LOG_MESSAGE_DEBUG("设备信息回调执行失败!");
    }

    strResp = SDKConvert::to_respString(nRespCode,stInfo);
    return strResp;
}