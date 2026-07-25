/**
 * @file DeviceBusiness.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-12
 * 
 * @brief 设备通用业务
 */
#include "DeviceBusiness.h"
  
std::string CDeviceBusiness::GetDeviceInfo(const std::string& req_data, const std::string& url_param)
{
    int nRespCode = 0;
    std::string strResp;
    NET_DeviceInfo_S stInfo;
    memset(&stInfo, 0, sizeof(NET_DeviceInfo_S));
    
    nRespCode = NetSDK_ExecuteCb_DeviceInfo(&stInfo);
    if(nRespCode != 0)
    {
        NSDK_LOG_DEBUG("设备信息回调执行失败!");
    }
    
    strResp = SDKConvert::to_respString(nRespCode,stInfo);
    return strResp;
}