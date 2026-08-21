/**
 * @file PlaybackBusiness.h
 * @brief Playback business
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
