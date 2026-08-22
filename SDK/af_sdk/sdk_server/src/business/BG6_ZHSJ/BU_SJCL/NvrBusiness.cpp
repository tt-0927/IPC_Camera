/*
 * @FilePath     : sdk_new/sdk_server/src/business/BG6_ZHSJ/BU_SJCL/NvrBusiness.cpp
 * @Author       : ITC
 * @Date         : 2026-08-19
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-19
 * @Description  : NVR独有配置业务处理实现（BG6_ZHSJ/BU_SJCL部门专用）
 */

#include "NvrBusiness.h"

#include <cctype>
#include <cstdlib>
#include <memory>

/* ===================== 设备规模信息（NVR独有） ===================== */

/**
 * 获取设备规模信息
 * @details 走 NVR 侧专用回调 executeGetDeviceInfoCb，填充 NET_DeviceInfo_S
 *          （设备类型/报警输入端口数/报警输出端口数/通道数）。
 *          NET_DeviceInfo_S 为 NVR 规模信息，从 Common/DeviceBusiness 迁入，
 *          归 NVR 侧，区别于通用设备基本信息 NET_DeviceBasicInfo_S。
 * @param req_data 请求数据（未使用）
 * @param url_param URL参数（未使用）
 * @return JSON格式的响应数据
 */
std::string CNvrBusiness::GetDeviceInfo(const std::string& req_data, const std::string& url_param)
{
    (void)req_data;
    (void)url_param;

    int nRespCode = 0;
    NET_DeviceInfo_S stInfo;
    memset(&stInfo, 0, sizeof(NET_DeviceInfo_S));

    nRespCode = executeGetDeviceInfoCb(&stInfo);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_DEBUG("设备信息回调执行失败! ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, 0, stInfo);
}

/* ===================== RTSP实时预览地址 ===================== */

/**
 * 获取RTSP实时预览地址
 * @details RTSP为NVR独有配置，走NVR专用回调执行函数（NetTVNvrConfigCb）
 * @param nChannelId 通道号
 * @param nCommand 命令码
 * @return JSON格式的响应数据
 */
std::string CNvrBusiness::HandleGetRtspUrl(INT32 nChannelId, INT32 nCommand)
{
    NET_RtspUrlInfo_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    NETSDK_LOG_MESSAGE_INFO("GetRtspUrl callback START");
    int nRespCode = executeGetRtspUrlCb(nChannelId, &stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("GetRtspUrl callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }
    NETSDK_LOG_MESSAGE_INFO("GetRtspUrl callback cmd=%d, ret=%d", nCommand, nRespCode);
    NETSDK_LOG_MESSAGE_INFO("GetRtspUrl callback END");
    return SDKConvert::to_respString(nRespCode, nCommand, nChannelId, stCfg);
}

/* ===================== 通道信息（含通道列表） ===================== */

/**
 * 获取通道信息（含通道列表）
 * @details 通道信息与通道列表已合并为同一接口（NET_GET_CHANNEL_INFO）：
 *          - 传 nChannelId：返回单通道信息
 *          - 不传 nChannelId：返回全通道列表
 *          结构体统一为 NET_ChannelList_S
 * @param nChannelId 通道号（0表示获取全通道列表）
 * @param nCommand 命令码
 * @return JSON格式的响应数据
 */
std::string CNvrBusiness::HandleGetChannelInfo(INT32 nChannelId, INT32 nCommand)
{
    auto stCfg = std::make_unique<NET_ChannelList_S>();
    if (!stCfg)
    {
        NETSDK_LOG_MESSAGE_WARN("GetChannelInfo callback alloc failed");
        return SDKConvert::to_respString(NET_E_FAILED, nCommand);
    }

    memset(stCfg.get(), 0, sizeof(NET_ChannelList_S));

    NETSDK_LOG_MESSAGE_INFO("GetChannelInfo callback START, nChannelId=%d", nChannelId);
    int nRespCode = executeGetDevConfigCb(nChannelId, nCommand, stCfg.get());
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("GetChannelInfo callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }
    NETSDK_LOG_MESSAGE_INFO("GetChannelInfo callback cmd=%d, ret=%d", nCommand, nRespCode);
    NETSDK_LOG_MESSAGE_INFO("GetChannelInfo callback END");
    return SDKConvert::to_respString(nRespCode, nCommand, nChannelId, *stCfg);
}

/* ===================== 录像文件列表 ===================== */

/**
 * 获取录像文件列表
 * @details 从URL参数中解析查询条件（通道号、类型、日期、时间范围、文件名等），调用SDK获取录像文件列表
 * @param nChannelId 通道号
 * @param nCommand 命令码
 * @param url_param URL参数
 * @return JSON格式的响应数据
 */
std::string CNvrBusiness::HandleGetRecordFileList(INT32 nChannelId, INT32 nCommand, const std::string& url_param)
{
    NET_RecordFileList_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    stCfg.stFind.nChnId = ParseIntParam(url_param, "ChnId", nChannelId);
    stCfg.stFind.nType = ParseIntParam(url_param, "Type", 0);

    std::string strYear = ParseStringParam(url_param, "Year");
    std::string strMonth = ParseStringParam(url_param, "Month");
    std::string strDate = ParseStringParam(url_param, "Date");
    std::string strStartTime = ParseStringParam(url_param, "StartTime");
    std::string strEndTime = ParseStringParam(url_param, "EndTime");
    std::string strFilename = ParseStringParam(url_param, "Filename");
    snprintf(stCfg.stFind.szYear, sizeof(stCfg.stFind.szYear), "%s", strYear.c_str());
    snprintf(stCfg.stFind.szMonth, sizeof(stCfg.stFind.szMonth), "%s", strMonth.c_str());
    snprintf(stCfg.stFind.szDate, sizeof(stCfg.stFind.szDate), "%s", strDate.c_str());
    snprintf(stCfg.stFind.szStartTime, sizeof(stCfg.stFind.szStartTime), "%s", strStartTime.c_str());
    snprintf(stCfg.stFind.szEndTime, sizeof(stCfg.stFind.szEndTime), "%s", strEndTime.c_str());
    snprintf(stCfg.stFind.szFilename, sizeof(stCfg.stFind.szFilename), "%s", strFilename.c_str());

    NETSDK_LOG_MESSAGE_INFO("GetRecordFileList callback START");
    int nRespCode = executeGetDevConfigCb(nChannelId, nCommand, &stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("GetRecordFileList callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }
    NETSDK_LOG_MESSAGE_INFO("GetRecordFileList callback cmd=%d, ret=%d", nCommand, nRespCode);
    NETSDK_LOG_MESSAGE_INFO("GetRecordFileList callback END");
    return SDKConvert::to_respString(nRespCode, nCommand, stCfg);
}
