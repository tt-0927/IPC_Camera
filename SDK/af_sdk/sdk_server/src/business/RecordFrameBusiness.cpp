/**
 * @file RecordFrameBusiness.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief RecordFrameBusiness 模块实现
 * 功能说明：
 * 1. 实现 RecordFrameBusiness 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#include "RecordFrameBusiness.h"

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
    return text && text[0] != '\0';
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 ValidateRecordFrameCond 定义的内部处理。
 * @param [in,out] stCond 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static int ValidateRecordFrameCond(NET_TV_RECORD_FRAME_STREAM_COND_S& stCond)
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

} /* namespace */

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
        !tvsdk::CRecordFrameServer::instance()->is_running() &&
        !tvsdk::CRecordFrameServer::instance()->start(static_cast<int>(stCond.dwTcpPort)))
    {
        return SDKConvert::to_respString(NET_TV_E_SYSCALL_FALIED);
    }

    NET_TV_RECORD_FRAME_STREAM_INFO_S stInfo;
    std::memset(&stInfo, 0, sizeof(stInfo));
    NET_TV_COMMON_ECODE_E nRespCode = tvsdk::CRecordFrameServer::instance()->open_stream(stCond, stInfo);
    if (nRespCode == NET_TV_E_SUCCEED && stInfo.dwTcpPort == 0)
    {
        stInfo.dwTcpPort = static_cast<UINT32>(tvsdk::CRecordFrameServer::instance()->port());
    }

    return SDKConvert::to_respString(nRespCode, stInfo);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 StopRecordFrameStream 对应的处理。
 * @param [in] req_data 函数处理参数。
 * @param [in] url_param 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

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

    NET_TV_COMMON_ECODE_E nRespCode = tvsdk::CRecordFrameServer::instance()->close_stream(stInfo.szStreamId);
    return SDKConvert::to_respString(nRespCode, stInfo);
}
