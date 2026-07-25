#include "RecordFrameBusiness.h"

namespace
{

bool HasText(const CHAR* text)
{
    return text && text[0] != '\0';
}

int ValidateRecordFrameCond(NET_TV_RECORD_FRAME_STREAM_COND_S& stCond)
{
    if (stCond.dwSize == 0)
    {
        stCond.dwSize = sizeof(NET_TV_RECORD_FRAME_STREAM_COND_S);
    }

    if (stCond.dwChannel <= 0 || !HasText(stCond.szStartTime) || !HasText(stCond.szEndTime))
    {
        return NET_TV_E_INVALID_PARAM;
    }

    if (stCond.dwMediaType == 0)
    {
        stCond.dwMediaType = NET_TV_RECORD_FRAME_MEDIA_VIDEO;
    }

    if (stCond.dwMediaType != NET_TV_RECORD_FRAME_MEDIA_VIDEO &&
        stCond.dwMediaType != NET_TV_RECORD_FRAME_MEDIA_AUDIO)
    {
        return NET_TV_E_INVALID_PARAM;
    }

    if (stCond.dwTcpPort > 65535)
    {
        return NET_TV_E_INVALID_PARAM;
    }

    return NET_TV_E_SUCCEED;
}

} // namespace

std::string CRecordFrameBusiness::StartRecordFrameStream(const std::string& req_data, const std::string& url_param)
{
    (void)url_param;

    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_TV_E_INVALID_PARAM);
    }

    NET_TV_RECORD_FRAME_STREAM_COND_S stCond;
    std::memset(&stCond, 0, sizeof(stCond));
    if (!SDKConvert::from_string(req_data, stCond))
    {
        return SDKConvert::to_respString(NET_TV_E_INVALID_PARAM);
    }

    int nValidCode = ValidateRecordFrameCond(stCond);
    if (nValidCode != NET_TV_E_SUCCEED)
    {
        return SDKConvert::to_respString(nValidCode);
    }

    if (stCond.dwTcpPort > 0 &&
        !tvsdk::RecordFrameServer::instance()->is_running() &&
        !tvsdk::RecordFrameServer::instance()->start(static_cast<int>(stCond.dwTcpPort)))
    {
        return SDKConvert::to_respString(NET_TV_E_SYSCALL_FALIED);
    }

    NET_TV_RECORD_FRAME_STREAM_INFO_S stInfo;
    std::memset(&stInfo, 0, sizeof(stInfo));
    NET_TV_COMMON_ECODE_E nRespCode = tvsdk::RecordFrameServer::instance()->open_stream(stCond, stInfo);
    if (nRespCode == NET_TV_E_SUCCEED && stInfo.dwTcpPort == 0)
    {
        stInfo.dwTcpPort = static_cast<UINT32>(tvsdk::RecordFrameServer::instance()->port());
    }

    return SDKConvert::to_respString(nRespCode, stInfo);
}

std::string CRecordFrameBusiness::StopRecordFrameStream(const std::string& req_data, const std::string& url_param)
{
    (void)url_param;

    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_TV_E_INVALID_PARAM);
    }

    NET_TV_RECORD_FRAME_STOP_INFO_S stInfo;
    std::memset(&stInfo, 0, sizeof(stInfo));
    if (!SDKConvert::from_string(req_data, stInfo))
    {
        return SDKConvert::to_respString(NET_TV_E_INVALID_PARAM);
    }

    if (stInfo.dwSize == 0)
    {
        stInfo.dwSize = sizeof(NET_TV_RECORD_FRAME_STOP_INFO_S);
    }

    if (!HasText(stInfo.szStreamId))
    {
        return SDKConvert::to_respString(NET_TV_E_INVALID_PARAM, stInfo);
    }

    NET_TV_COMMON_ECODE_E nRespCode = tvsdk::RecordFrameServer::instance()->close_stream(stInfo.szStreamId);
    return SDKConvert::to_respString(nRespCode, stInfo);
}
