/**
 * @file PlaybackBusiness.cpp
 * @brief Playback business implementation
 */

#include "PlaybackBusiness.h"

#include <cstring>

namespace
{
bool HasText(const CHAR* text)
{
    return text != NULL && text[0] != '\0';
}

int ValidateReplayCtrlInfo(NET_ReplayCtrlInfo_S& stInfo)
{
    if (stInfo.uChannel <= 0)
    {
        return NET_E_INVALID_PARAM;
    }

    switch (stInfo.uCtrlType)
    {
        case NET_REPLAY_CTRL_START:
            if (!HasText(stInfo.szStartTime) || !HasText(stInfo.szEndTime))
            {
                return NET_E_INVALID_PARAM;
            }
            if (stInfo.fSpeed <= 0.0f)
            {
                stInfo.fSpeed = 1.0f;
            }
            break;
        case NET_REPLAY_CTRL_STOP:
        case NET_REPLAY_CTRL_PAUSE:
        case NET_REPLAY_CTRL_RESUME:
            if (!HasText(stInfo.szSessionId))
            {
                return NET_E_INVALID_PARAM;
            }
            break;
        case NET_REPLAY_CTRL_SET_SPEED:
            if (!HasText(stInfo.szSessionId))
            {
                return NET_E_INVALID_PARAM;
            }
            if (stInfo.nReplayType == NET_REPLAY_PLATFORM_CTRL_NONE)
            {
                stInfo.nReplayType = NET_REPLAY_PLATFORM_CTRL_SPEED;
            }
            if (stInfo.nReplayType != NET_REPLAY_PLATFORM_CTRL_SPEED || stInfo.fSpeed <= 0.0f)
            {
                return NET_E_INVALID_PARAM;
            }
            break;
        case NET_REPLAY_CTRL_SET_SEEK:
            if (!HasText(stInfo.szSessionId))
            {
                return NET_E_INVALID_PARAM;
            }
            if (stInfo.nReplayType == NET_REPLAY_PLATFORM_CTRL_NONE &&
                HasText(stInfo.szStartTime) && HasText(stInfo.szEndTime))
            {
                stInfo.nReplayType = NET_REPLAY_PLATFORM_CTRL_JUMP_TIME;
            }
            switch (stInfo.nReplayType)
            {
                case NET_REPLAY_PLATFORM_CTRL_NONE:
                    break;
                case NET_REPLAY_PLATFORM_CTRL_BACKWARD_30S:
                case NET_REPLAY_PLATFORM_CTRL_FORWARD_30S:
                    if (stInfo.nSeekTime <= 0)
                    {
                        stInfo.nSeekTime = 30;
                    }
                    break;
                case NET_REPLAY_PLATFORM_CTRL_PERSON_EVENT:
                case NET_REPLAY_PLATFORM_CTRL_VEHICLE_EVENT:
                case NET_REPLAY_PLATFORM_CTRL_PERSON_VEHICLE_EVENT:
                case NET_REPLAY_PLATFORM_CTRL_CANCEL_EVENT:
                    break;
                case NET_REPLAY_PLATFORM_CTRL_JUMP_TIME:
                    if (!HasText(stInfo.szStartTime) || !HasText(stInfo.szEndTime))
                    {
                        return NET_E_INVALID_PARAM;
                    }
                    break;
                default:
                    return NET_E_INVALID_PARAM;
            }
            break;
        default:
            return NET_E_INVALID_PARAM;
    }

    return NET_E_SUCCEED;
}
}

std::string CPlaybackBusiness::GetReplayUrl(const std::string& req_data, const std::string& url_param)
{
    (void)url_param;

    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM);
    }

    NET_ReplayUrlInfo_S stInfo;
    std::memset(&stInfo, 0, sizeof(stInfo));

    Json::Object* pRoot = Json::init(req_data);
    if (!pRoot)
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM);
    }

    SDKConvert::deal(pRoot, stInfo, true);
    Json::deinit(pRoot);

    NETSDK_LOG_MESSAGE_INFO("GetReplayUrl request: channel=%d, start=%s, end=%s",
                  stInfo.uChannel,
                  stInfo.szStartTime,
                  stInfo.szEndTime);

    int nRespCode = NetSDK_ExecuteCb_GetReplayUrl(&stInfo);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("GetReplayUrl callback failed, ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, 0, stInfo);
}

std::string CPlaybackBusiness::ControlReplay(const std::string& req_data, const std::string& url_param)
{
    (void)url_param;

    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM);
    }

    NET_ReplayCtrlInfo_S stInfo;
    std::memset(&stInfo, 0, sizeof(stInfo));

    Json::Object* pRoot = Json::init(req_data);
    if (!pRoot)
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM);
    }

    SDKConvert::deal(pRoot, stInfo, true);
    Json::deinit(pRoot);

    NETSDK_LOG_MESSAGE_INFO("ControlReplay after JSON parse: channel=%d, ctrlType=%d, startTime=[%s], endTime=[%s], sessionId=[%s]",
                  stInfo.uChannel,
                  stInfo.uCtrlType,
                  stInfo.szStartTime,
                  stInfo.szEndTime,
                  stInfo.szSessionId);

    int nValidCode = ValidateReplayCtrlInfo(stInfo);
    if (nValidCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("ControlReplay request invalid: channel=%d, ctrlType=%d, replayType=%d, session=%s, speed=%.3f, seek=%d, start=%s, end=%s",
                      stInfo.uChannel,
                      stInfo.uCtrlType,
                      stInfo.nReplayType,
                      stInfo.szSessionId,
                      stInfo.fSpeed,
                      stInfo.nSeekTime,
                      stInfo.szStartTime,
                      stInfo.szEndTime);
        return SDKConvert::to_respString((NET_COMMON_ECODE_E)nValidCode);
    }

    NETSDK_LOG_MESSAGE_INFO("ControlReplay request: channel=%d, ctrlType=%d, replayType=%d, session=%s, speed=%.3f, seek=%d, start=%s, end=%s",
                  stInfo.uChannel,
                  stInfo.uCtrlType,
                  stInfo.nReplayType,
                  stInfo.szSessionId,
                  stInfo.fSpeed,
                  stInfo.nSeekTime,
                  stInfo.szStartTime,
                  stInfo.szEndTime);

    int nRespCode = NetSDK_ExecuteCb_ControlReplay(&stInfo);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("ControlReplay callback failed, ret=%d", nRespCode);
    }
    NETSDK_LOG_MESSAGE_INFO("ControlReplay after callback: channel=%d, ctrlType=%d, url=[%s], start=[%s], end=[%s]",
                  stInfo.uChannel,
                  stInfo.uCtrlType,
                  stInfo.szUrl,
                  stInfo.szStartTime,
                  stInfo.szEndTime);

    return SDKConvert::to_respString(nRespCode, 0, stInfo);
}

std::string CPlaybackBusiness::GetReplayRecordList(const std::string& req_data, const std::string& url_param)
{
    (void)url_param;

    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM);
    }

    NET_ReplayRecordList_S stInfo;
    std::memset(&stInfo, 0, sizeof(stInfo));

    Json::Object* pRoot = Json::init(req_data);
    if (!pRoot)
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM);
    }

    SDKConvert::deal(pRoot, stInfo, true);
    Json::deinit(pRoot);

    NETSDK_LOG_MESSAGE_INFO("GetReplayRecordList request: channel=%d, filterByEventType=%d, eventType=%d, date=%s, start=%s, end=%s",
                  stInfo.uChannel,
                  stInfo.bFilterByEventType,
                  stInfo.uEventType,
                  stInfo.szDate,
                  stInfo.szStartTime,
                  stInfo.szEndTime);

    int nRespCode = NetSDK_ExecuteCb_GetReplayRecordList(&stInfo);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("GetReplayRecordList callback failed, ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, 0, stInfo);
}
