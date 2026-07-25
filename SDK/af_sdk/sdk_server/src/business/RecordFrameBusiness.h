#pragma once

#include <cstring>
#include <string>

#include "Singleton.h"
#include "NetTVSDKServerInterface.h"
#include "RecordFrameServer.h"
#include "DeviceInfoConvert.h"
#include "SDKConvert.h"
#include "NetSdkLog.h"

class CRecordFrameBusiness : public CSingleton<CRecordFrameBusiness>
{
    CRecordFrameBusiness() {}
public:
    ~CRecordFrameBusiness() {}
    friend class CSingleton<CRecordFrameBusiness>;

public:
    std::string StartRecordFrameStream(const std::string& req_data, const std::string& url_param);
    std::string StopRecordFrameStream(const std::string& req_data, const std::string& url_param);
};
