/**
 * @file DeviceCapabilityBusiness.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief DeviceCapabilityBusiness 模块接口与类型定义
 * 功能说明：
 * 1. 声明 DeviceCapabilityBusiness 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#pragma once
#include <string>
#include <functional>

#include "NetTVCapabilityCbExecute.h"
#include "CapabilityInfoConvert.h"
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
 * @author tianl (tianl@kfb.cn)
     * @brief 获取设备能力集 - 统一入口
     * @param req_data 请求数据(JSON)
     * @param url_param URL参数字符串
     * @return JSON响应字符串
     */
    std::string GetDeviceCapability(const std::string& req_data, const std::string& url_param);

private:
    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 处理视频编码能力集 (NET_TV_CAP_VIDEO_ENCODE)
     */
    std::string HandleVideoEncode(int channelId);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 处理音频编码能力集 (NET_TV_CAP_AUDIO)
     */
    std::string HandleAudioEncode(int channelId);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 处理OSD参数能力集 (NET_TV_CAP_OSD)
     */
    std::string HandleOsd(int channelId);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 从URL参数中解析整型值
     */
    int ParseIntParam(const std::string& url_param, const std::string& key, int defaultVal = 0);
};
