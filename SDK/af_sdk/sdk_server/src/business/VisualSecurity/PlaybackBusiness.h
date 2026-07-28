/**
 * @file PlaybackBusiness.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief PlaybackBusiness 模块接口与类型定义
 * 功能说明：
 * 1. 声明 PlaybackBusiness 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */

#pragma once

#include <string>

#include "Singleton.h"
#include "NetTVSDKServerInterface.h"
#include "NetTVConfigCbExecute.h"
#include "DeviceInfoConvert.h"
#include "SDKConvert.h"
#include "NetSdkLog.h"

class CPlaybackBusiness : public CSingleton<CPlaybackBusiness>
{
    CPlaybackBusiness() {}
public:
    ~CPlaybackBusiness() {}
    friend class CSingleton<CPlaybackBusiness>;

public:
    std::string GetReplayUrl(const std::string& req_data, const std::string& url_param);
    std::string ControlReplay(const std::string& req_data, const std::string& url_param);
    std::string GetReplayRecordList(const std::string& req_data, const std::string& url_param);
};
