/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : RecordFrameBusiness.cpp
 * @Description  : 录像帧业务处理实现，负责处理录像帧流的启动和停止请求
 */

#include "RecordFrameBusiness.h"

namespace
{

/**
 * @brief 判断字符串是否非空
 * @param text 字符串指针
 * @return true表示字符串非空，false表示字符串为空或指针为空
 */
bool HasText(const CHAR* text)
{
    return text && text[0] != '\0';
}

/**
 * @brief 验证录像帧流启动条件参数
 * @details 检查通道号、时间范围、媒体类型、端口号等参数的有效性，设置默认值
 * @param stCond 流启动条件结构体
 * @return 错误码，NET_E_SUCCEED表示成功，其他值表示失败
 */
int ValidateRecordFrameCond(NET_RecordFrameStreamCond_S& stCond)
{
    if (stCond.uSize == 0)
    {
        stCond.uSize = sizeof(NET_RecordFrameStreamCond_S);
    }

    if (stCond.uChannel <= 0 || !HasText(stCond.szStartTime) || !HasText(stCond.szEndTime))
    {
        return NET_E_INVALID_PARAM;
    }

    if (stCond.uMediaType == 0)
    {
        stCond.uMediaType = NET_RECORD_FRAME_MEDIA_VIDEO;
    }

    if (stCond.uMediaType != NET_RECORD_FRAME_MEDIA_VIDEO &&
        stCond.uMediaType != NET_RECORD_FRAME_MEDIA_AUDIO)
    {
        return NET_E_INVALID_PARAM;
    }

    if (stCond.uTcpPort > 65535)
    {
        return NET_E_INVALID_PARAM;
    }

    return NET_E_SUCCEED;
}

} // namespace

/**
 * @brief 处理启动录像帧流请求
 * @details 解析请求数据（JSON格式），验证参数有效性，启动RecordFrameServer（如果未运行），
 *          调用RecordFrameServer::open_stream开启流，返回流信息
 * @param req_data 请求数据（JSON格式），包含流启动条件（通道、时间范围、媒体类型等）
 * @param url_param URL参数（未使用）
 * @return 响应数据（JSON格式），包含流信息（流ID、端口、媒体类型等）
 */
std::string CRecordFrameBusiness::StartRecordFrameStream(const std::string& req_data, const std::string& url_param)
{
    (void)url_param;

    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM);
    }

    NET_RecordFrameStreamCond_S stCond;
    std::memset(&stCond, 0, sizeof(stCond));
    if (!SDKConvert::from_string(req_data, stCond))
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM);
    }

    int nValidCode = ValidateRecordFrameCond(stCond);
    if (nValidCode != NET_E_SUCCEED)
    {
        return SDKConvert::to_respString((NET_COMMON_ECODE_E)nValidCode);
    }

    if (stCond.uTcpPort > 0 &&
        !tvsdk::CRecordFrameServer::instance()->is_running() &&
        !tvsdk::CRecordFrameServer::instance()->start(static_cast<int>(stCond.uTcpPort)))
    {
        return SDKConvert::to_respString(NET_E_SYSCALL_FALIED);
    }

    NET_RecordFrameStreamInfo_S stInfo;
    std::memset(&stInfo, 0, sizeof(stInfo));
    NET_COMMON_ECODE_E nRespCode = tvsdk::CRecordFrameServer::instance()->open_stream(stCond, stInfo);
    if (nRespCode == NET_E_SUCCEED && stInfo.uTcpPort == 0)
    {
        stInfo.uTcpPort = static_cast<UINT32>(tvsdk::CRecordFrameServer::instance()->port());
    }

    return SDKConvert::to_respString(nRespCode, 0, stInfo);
}

/**
 * @brief 处理停止录像帧流请求
 * @details 解析请求数据（JSON格式），验证流ID有效性，调用RecordFrameServer::close_stream关闭流，返回操作结果
 * @param req_data 请求数据（JSON格式），包含流ID
 * @param url_param URL参数（未使用）
 * @return 响应数据（JSON格式），包含操作结果
 */
std::string CRecordFrameBusiness::StopRecordFrameStream(const std::string& req_data, const std::string& url_param)
{
    (void)url_param;

    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM);
    }

    NET_RecordFrameStopInfo_S stInfo;
    std::memset(&stInfo, 0, sizeof(stInfo));
    if (!SDKConvert::from_string(req_data, stInfo))
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM);
    }

    if (stInfo.uSize == 0)
    {
        stInfo.uSize = sizeof(NET_RecordFrameStopInfo_S);
    }

    if (!HasText(stInfo.szStreamId))
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, 0, stInfo);
    }

    NET_COMMON_ECODE_E nRespCode = tvsdk::CRecordFrameServer::instance()->close_stream(stInfo.szStreamId);
    return SDKConvert::to_respString(nRespCode, 0, stInfo);
}
