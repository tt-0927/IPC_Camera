/**
 * @file PlaybackBusiness.cpp
 * @brief Playback business implementation
 */

#include "PlaybackBusiness.h"
#include "UrlParamUtils.h"
#include "HttpBasicCommand.hpp"

#include <cstring>
#include <vector>
#include <algorithm>

namespace
{
bool HasText(const CHAR* text)
{
    return text != NULL && text[0] != '\0';
}

int ValidateReplayCtrlInfo(NET_ReplayCtrlInfo_S& stInfo)
{
    /* Unified reject logging: the field only sees 102 and cannot tell which check
     * failed (channel / ctrlType / session / speed). Log the reason plus the key
     * fields once, so the next run is diagnosable from a single line. */
    const auto reject = [&stInfo](const CHAR* reason) -> int
    {
        NETSDK_LOG_MESSAGE_WARN("replay validate rejected: reason=%s, channel=%d, ctrlType=%d, replayType=%d, speed=%.3f, seek=%d, session=[%s], start=[%s], end=[%s]",
                                reason,
                                stInfo.uChannel,
                                stInfo.uCtrlType,
                                stInfo.nReplayType,
                                stInfo.fSpeed,
                                stInfo.nSeekTime,
                                stInfo.szSessionId,
                                stInfo.szStartTime,
                                stInfo.szEndTime);
        return NET_E_INVALID_PARAM;
    };

    /* 通道 0 合法：与 sdk_old（6860）的 dwChannel < 0 判定保持一致。
     * 写成 <=0 会导致控制回放/取回放URL/取回放列表在通道 0 时直接被判 102，
     * 客户端连请求都发不出去（表现为 "Control replay start failed! Error=102"）。 */
    if (stInfo.uChannel < 0)
    {
        return reject("channel<0");
    }

    switch (stInfo.uCtrlType)
    {
        case NET_REPLAY_CTRL_START:
            if (!HasText(stInfo.szStartTime) || !HasText(stInfo.szEndTime))
            {
                return reject("start: missing start/end time");
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
                return reject("stop/pause/resume: missing sessionId");
            }
            break;
        case NET_REPLAY_CTRL_SET_SPEED:
            if (!HasText(stInfo.szSessionId))
            {
                return reject("set_speed: missing sessionId");
            }
            if (stInfo.nReplayType == NET_REPLAY_PLATFORM_CTRL_NONE)
            {
                stInfo.nReplayType = NET_REPLAY_PLATFORM_CTRL_SPEED;
            }
            if (stInfo.nReplayType != NET_REPLAY_PLATFORM_CTRL_SPEED || stInfo.fSpeed <= 0.0f)
            {
                return reject("set_speed: bad replayType/speed");
            }
            break;
        case NET_REPLAY_CTRL_SET_SEEK:
            if (!HasText(stInfo.szSessionId))
            {
                return reject("set_seek: missing sessionId");
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
                        return reject("set_seek/jump_time: missing start/end time");
                    }
                    break;
                default:
                    return reject("set_seek: unsupported replayType");
            }
            break;
        default:
            return reject("unsupported ctrlType");
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

    if (stInfo.uChannel < 0)
    {
        NETSDK_LOG_MESSAGE_WARN("GetReplayUrl: invalid channel=%d (channel<0)", stInfo.uChannel);
        return SDKConvert::to_respString(NET_E_INVALID_PARAM);
    }

    NETSDK_LOG_MESSAGE_INFO("GetReplayUrl request: channel=%d, start=%s, end=%s",
                  stInfo.uChannel,
                  stInfo.szStartTime,
                  stInfo.szEndTime);

    int nRespCode = executeGetReplayUrlCb(&stInfo);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("GetReplayUrl callback failed, ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, 0, stInfo);
}

std::string CPlaybackBusiness::ControlReplay(const std::string& req_data, const std::string& url_param)
{
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

    /* szUserIp 为空时自动填充HTTP对端IP（服务端权威来源，客户端/平台无需填写） */
    if (!HasText(stInfo.szUserIp))
    {
        const std::string clientIp = SdkHttpContext::client_ip();
        if (!clientIp.empty())
        {
            snprintf(stInfo.szUserIp, sizeof(stInfo.szUserIp), "%s", clientIp.c_str());
        }
    }

    NETSDK_LOG_MESSAGE_INFO("ControlReplay after JSON parse: channel=%d, ctrlType=%d, startTime=[%s], endTime=[%s], sessionId=[%s], userIp=[%s]",
                  stInfo.uChannel,
                  stInfo.uCtrlType,
                  stInfo.szStartTime,
                  stInfo.szEndTime,
                  stInfo.szSessionId,
                  stInfo.szUserIp);

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

    NETSDK_LOG_MESSAGE_INFO("ControlReplay request: channel=%d, ctrlType=%d, replayType=%d, session=%s, speed=%.3f, seek=%d, start=%s, end=%s, userIp=[%s]",
                  stInfo.uChannel,
                  stInfo.uCtrlType,
                  stInfo.nReplayType,
                  stInfo.szSessionId,
                  stInfo.fSpeed,
                  stInfo.nSeekTime,
                  stInfo.szStartTime,
                  stInfo.szEndTime,
                  stInfo.szUserIp);

    int nRespCode = executeControlReplayCb(&stInfo);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("ControlReplay callback failed, ret=%d", nRespCode);
    }
    NETSDK_LOG_MESSAGE_INFO("ControlReplay after callback: channel=%d, ctrlType=%d, url=[%s], start=[%s], end=[%s], userIp=[%s]",
                  stInfo.uChannel,
                  stInfo.uCtrlType,
                  stInfo.szUrl,
                  stInfo.szStartTime,
                  stInfo.szEndTime,
                  stInfo.szUserIp);

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

    if (stInfo.uChannel < 0)
    {
        NETSDK_LOG_MESSAGE_WARN("GetReplayRecordList: invalid channel=%d (channel<0)", stInfo.uChannel);
        return SDKConvert::to_respString(NET_E_INVALID_PARAM);
    }

    NETSDK_LOG_MESSAGE_INFO("GetReplayRecordList request: channel=%d, filterByEventType=%d, eventType=%d, date=%s, start=%s, end=%s",
                  stInfo.uChannel,
                  stInfo.bFilterByEventType,
                  stInfo.uEventType,
                  stInfo.szDate,
                  stInfo.szStartTime,
                  stInfo.szEndTime);

    int nRespCode = executeGetReplayRecordListCb(&stInfo);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("GetReplayRecordList callback failed, ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, 0, stInfo);
}

/* 分页查询真实录像文件（对应 NET_API_PATH_RECORD_QUERY_FILES）。
 * 入参 NET_RecordFileQuery_S：stFind 检索条件 + stPage 分页（nCurPage 从1起，nPageSize 上限
 * NET_RECORD_QUERY_PAGE_MAX_NUM）。服务端按页回调 executeQueryRecordFilesCb（3588 侧已注册），
 * 回填 pResults 数组与分页统计字段。pResults 由本函数按请求页大小临时分配，回调结束后序列化为
 * JSON Infos 数组返回；避免旧 48 条固定数组上限。 */
std::string CPlaybackBusiness::QueryRecordFiles(const std::string& req_data,
                                                const std::string& url_param)
{
    (void)url_param;
    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, 0);
    }

    NET_RecordFileQuery_S stInfo;
    std::memset(&stInfo, 0, sizeof(stInfo));

    Json::Object* pRoot = Json::init(req_data);
    if (!pRoot)
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, 0);
    }
    SDKConvert::deal(pRoot, stInfo, true);
    Json::deinit(pRoot);

    if (stInfo.stFind.nChnId < 0 || stInfo.stPage.nCurPage <= 0 ||
        stInfo.stPage.nPageSize <= 0 ||
        stInfo.stPage.nPageSize > NET_RECORD_QUERY_PAGE_MAX_NUM)
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, 0);
    }

    std::vector<NET_RecordFindResult_S> results(
        static_cast<size_t>(stInfo.stPage.nPageSize));
    stInfo.pResults = results.data();
    stInfo.nResultCapacity = static_cast<INT32>(results.size());
    stInfo.nResultCount = 0;

    int nRespCode = executeQueryRecordFilesCb(&stInfo);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("QueryRecordFiles callback failed, channel=%d, page=%d, pageSize=%d, ret=%d",
                                stInfo.stFind.nChnId, stInfo.stPage.nCurPage,
                                stInfo.stPage.nPageSize, nRespCode);
    }
    else
    {
        stInfo.nResultCount = (std::max)(
            0, (std::min)(stInfo.nResultCount, stInfo.nResultCapacity));
        stInfo.stPage.nDataTotal = (std::max)(0, stInfo.stPage.nDataTotal);
        stInfo.stPage.nPageTotal =
            stInfo.stPage.nDataTotal / stInfo.stPage.nPageSize;
        if (stInfo.stPage.nDataTotal % stInfo.stPage.nPageSize != 0)
        {
            ++stInfo.stPage.nPageTotal;
        }
        stInfo.stPage.bHasMore =
            (stInfo.stPage.nCurPage < stInfo.stPage.nPageTotal) ? TRUE : FALSE;
    }

    return SDKConvert::to_respString(nRespCode, 0, stInfo);
}
