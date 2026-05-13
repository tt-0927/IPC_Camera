/**
 * @file PlaybackBusiness.cpp
 * @brief Playback business implementation
 */

#include "PlaybackBusiness.h"

#include <cstring>

std::string CPlaybackBusiness::GetReplayUrl(const std::string& req_data, const std::string& url_param)
{
    (void)url_param;

    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_TV_E_INVALID_PARAM);
    }

    NET_TV_REPLAY_URL_INFO_S stInfo;
    std::memset(&stInfo, 0, sizeof(stInfo));

    Json::Object* pRoot = Json::init(req_data);
    if (!pRoot)
    {
        return SDKConvert::to_respString(NET_TV_E_INVALID_PARAM);
    }

    SDKConvert::deal(pRoot, stInfo, true);
    Json::deinit(pRoot);

    NSDK_LOG_INFO("GetReplayUrl request: channel=%d, start=%s, end=%s",
                  stInfo.dwChannel,
                  stInfo.szStartTime,
                  stInfo.szEndTime);

    int nRespCode = NetSDK_ExecuteCb_GetReplayUrl(&stInfo);
    if (nRespCode != NET_TV_E_SUCCEED)
    {
        NSDK_LOG_WARN("GetReplayUrl callback failed, ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, stInfo);
}

std::string CPlaybackBusiness::ControlReplay(const std::string& req_data, const std::string& url_param)
{
    (void)url_param;

    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_TV_E_INVALID_PARAM);
    }

    NET_TV_REPLAY_CTRL_INFO_S stInfo;
    std::memset(&stInfo, 0, sizeof(stInfo));

    Json::Object* pRoot = Json::init(req_data);
    if (!pRoot)
    {
        return SDKConvert::to_respString(NET_TV_E_INVALID_PARAM);
    }

    SDKConvert::deal(pRoot, stInfo, true);
    Json::deinit(pRoot);

    NSDK_LOG_INFO("ControlReplay request: channel=%d, ctrlType=%d, session=%s, speed=%.3f, start=%s, end=%s",
                  stInfo.dwChannel,
                  stInfo.dwCtrlType,
                  stInfo.szSessionId,
                  stInfo.fSpeed,
                  stInfo.szStartTime,
                  stInfo.szEndTime);

    int nRespCode = NetSDK_ExecuteCb_ControlReplay(&stInfo);
    if (nRespCode != NET_TV_E_SUCCEED)
    {
        NSDK_LOG_WARN("ControlReplay callback failed, ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, stInfo);
}

std::string CPlaybackBusiness::GetReplayRecordList(const std::string& req_data, const std::string& url_param)
{
    (void)url_param;

    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_TV_E_INVALID_PARAM);
    }

    NET_TV_REPLAY_RECORD_LIST_S stInfo;
    std::memset(&stInfo, 0, sizeof(stInfo));

    Json::Object* pRoot = Json::init(req_data);
    if (!pRoot)
    {
        return SDKConvert::to_respString(NET_TV_E_INVALID_PARAM);
    }

    SDKConvert::deal(pRoot, stInfo, true);
    Json::deinit(pRoot);

    NSDK_LOG_INFO("GetReplayRecordList request: channel=%d, filterByEventType=%d, eventType=%d, date=%s, start=%s, end=%s",
                  stInfo.dwChannel,
                  stInfo.bFilterByEventType,
                  stInfo.dwEventType,
                  stInfo.szDate,
                  stInfo.szStartTime,
                  stInfo.szEndTime);

    int nRespCode = NetSDK_ExecuteCb_GetReplayRecordList(&stInfo);
    if (nRespCode != NET_TV_E_SUCCEED)
    {
        NSDK_LOG_WARN("GetReplayRecordList callback failed, ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, stInfo);
}
