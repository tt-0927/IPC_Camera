/**
 * @file DeviceCapabilityBusiness.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-01-30
 *
 * @brief 设备能力集业务 - 统一入口，按command分发
 */

#pragma once
#include <string>
#include <functional>

#include "NetTVCapabilityCbExecute.h"
#include "VisualSecurity/CapabilityInfoConvert.h"
#include "SDKConvert.h"
#include "NetSdkLog.h"
#include "Singleton.h"

class CDeviceCapabilityBusiness : public CSingleton<CDeviceCapabilityBusiness>
{
    CDeviceCapabilityBusiness()
    {

    }
public:

    ~CDeviceCapabilityBusiness()
    {

    }
    friend class CSingleton<CDeviceCapabilityBusiness>;

public:

    /**
     * @brief 获取设备能力集 - 统一入口
     * @param req_data 请求数据(JSON)
     * @param url_param URL参数字符串
     * @return JSON响应字符串
     */
    std::string GetDeviceCapability(const std::string& req_data, const std::string& url_param);

private:
    /**
     * @brief 处理视频编码能力集 (NET_CAP_VIDEO_ENCODE)
     */
    std::string HandleVideoEncode(int channelId);

    /**
     * @brief 处理音频编码能力集 (NET_CAP_AUDIO)
     */
    std::string HandleAudioEncode(int channelId);

    /**
     * @brief 处理OSD参数能力集 (NET_CAP_OSD)
     */
    std::string HandleOsd(int channelId);

    /**
     * @brief 从URL参数中解析整型值
     */
    int ParseIntParam(const std::string& url_param, const std::string& key, int defaultVal = 0);
};
