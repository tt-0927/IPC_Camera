/**
 * @file DeviceConfigBusiness.h
 * @brief Device config business
 */
#pragma once

#include <string>
#include <cstring>

#include "Singleton.h"
#include "NetTVSDKServerInterface.h"
#include "NetTVConfigCbExecute.h"
#include "DeviceInfoConvert.h"
#include "CapabilityInfoConvert.h"
#include "AlarmInfoConvert.h"
#include "SDKConvert.h"
#include "NetSdkLog.h"
#include <sstream>




class CDeviceConfigBusiness : public CSingleton<CDeviceConfigBusiness>
{
    CDeviceConfigBusiness() {}
public:
    ~CDeviceConfigBusiness() {}
    friend class CSingleton<CDeviceConfigBusiness>;

public:
    std::string GetDevConfig(const std::string& req_data, const std::string& url_param);
    std::string SetDevConfig(const std::string& req_data, const std::string& url_param);

private:
    template<typename T_CFG>
    std::string HandleGetConfig(INT32 channelId, INT32 command)
    {
        T_CFG stCfg;
        memset(&stCfg, 0, sizeof(T_CFG));
        NSDK_LOG_INFO("GetDevConfig callback START");
        NSDK_LOG_INFO("[SDK] stCfg address=%p, sizeof(T_CFG)=%zu\n", (void*)&stCfg, sizeof(T_CFG));

        int nRespCode = NetSDK_ExecuteCb_GetDevConfig(channelId, command, &stCfg);
        NSDK_LOG_INFO("[SDK] after callback: stCfg address=%p\n", (void*)&stCfg);
        if (nRespCode != NET_TV_E_SUCCEED)
        {
            NSDK_LOG_WARN("GetDevConfig callback failed, cmd=%d, ret=%d", command, nRespCode);
        }
        NSDK_LOG_INFO("GetDevConfig callback cmd=%d, ret=%d", command, nRespCode);
        NSDK_LOG_INFO("GetDevConfig callback END");
        return SDKConvert::to_respString(nRespCode, stCfg);
    }

    template<typename T_CFG>
    std::string HandleSetConfig(INT32 channelId, INT32 command, const std::string& req_data)
    {
        if (req_data.empty())
        {
            return SDKConvert::to_respString(NET_TV_E_INVALID_PARAM);
        }

        T_CFG stCfg;
        memset(&stCfg, 0, sizeof(T_CFG));

        Json::Object* pRoot = Json::init(req_data);
        if (!pRoot)
        {
            return SDKConvert::to_respString(NET_TV_E_INVALID_PARAM);
        }

        SDKConvert::deal(pRoot, stCfg, true);
        Json::deinit(pRoot);

        int nRespCode = NetSDK_ExecuteCb_SetDevConfig(channelId, command, &stCfg);
        if (nRespCode != NET_TV_E_SUCCEED)
        {
            NSDK_LOG_WARN("SetDevConfig callback failed, cmd=%d, ret=%d", command, nRespCode);
        }

        return SDKConvert::to_respString((NET_TV_COMMON_ECODE_E)nRespCode);
    }

    int ParseIntParam(const std::string& url_param, const std::string& key, int defaultVal = 0);
    std::string ParseStringParam(const std::string& url_param, const std::string& key, const std::string& defaultVal = "");
    std::string UrlDecode(const std::string& value);
    std::string HandleGetLogList(INT32 channelId, INT32 command, const std::string& url_param);
    std::string HandleGetRecordFileList(INT32 channelId, INT32 command, const std::string& url_param);
};
