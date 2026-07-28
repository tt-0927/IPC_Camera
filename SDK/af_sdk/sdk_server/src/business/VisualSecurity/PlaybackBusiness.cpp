/**
 * @file PlaybackBusiness.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief PlaybackBusiness 模块实现
 * 功能说明：
 * 1. 实现 PlaybackBusiness 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */


#include "PlaybackBusiness.h"

#include <cstring>

namespace
{
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 HasText 对应的数据。
 * @param [in] text 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
static bool HasText(const CHAR* text)
{
    return text != NULL && text[0] != '\0';
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 ValidateReplayCtrlInfo 定义的内部处理。
 * @param [in,out] stInfo 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static int ValidateReplayCtrlInfo(NET_ReplayCtrlInfo_S& stInfo)
{
    if (stInfo.dwChannel <= 0)
    {
        return NET_E_INVALID_PARAM;
    }

    switch (stInfo.dwCtrlType)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 GetReplayUrl 对应的数据。
 * @param [in] req_data 函数处理参数。
 * @param [in] url_param 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

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
                  stInfo.dwChannel,
                  stInfo.szStartTime,
                  stInfo.szEndTime);

    int nRespCode = NetSDK_ExecuteCb_GetReplayUrl(&stInfo);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("GetReplayUrl callback failed, ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, stInfo);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 ControlReplay 定义的内部处理。
 * @param [in] req_data 函数处理参数。
 * @param [in] url_param 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

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
                  stInfo.dwChannel,
                  stInfo.dwCtrlType,
                  stInfo.szStartTime,
                  stInfo.szEndTime,
                  stInfo.szSessionId);

    int nValidCode = ValidateReplayCtrlInfo(stInfo);
    if (nValidCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("ControlReplay request invalid: channel=%d, ctrlType=%d, replayType=%d, session=%s, speed=%.3f, seek=%d, start=%s, end=%s",
                      stInfo.dwChannel,
                      stInfo.dwCtrlType,
                      stInfo.nReplayType,
                      stInfo.szSessionId,
                      stInfo.fSpeed,
                      stInfo.nSeekTime,
                      stInfo.szStartTime,
                      stInfo.szEndTime);
        return SDKConvert::to_respString(nValidCode);
    }

    NETSDK_LOG_MESSAGE_INFO("ControlReplay request: channel=%d, ctrlType=%d, replayType=%d, session=%s, speed=%.3f, seek=%d, start=%s, end=%s",
                  stInfo.dwChannel,
                  stInfo.dwCtrlType,
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
                  stInfo.dwChannel,
                  stInfo.dwCtrlType,
                  stInfo.szUrl,
                  stInfo.szStartTime,
                  stInfo.szEndTime);

    return SDKConvert::to_respString(nRespCode, stInfo);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 GetReplayRecordList 对应的数据。
 * @param [in] req_data 函数处理参数。
 * @param [in] url_param 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

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
                  stInfo.dwChannel,
                  stInfo.bFilterByEventType,
                  stInfo.dwEventType,
                  stInfo.szDate,
                  stInfo.szStartTime,
                  stInfo.szEndTime);

    int nRespCode = NetSDK_ExecuteCb_GetReplayRecordList(&stInfo);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("GetReplayRecordList callback failed, ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, stInfo);
}
