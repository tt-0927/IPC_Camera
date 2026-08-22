/*
 * @FilePath     : sdk_new/sdk_server/src/business/BG6_ZHSJ/BU_SJCL/NvrBusiness.h
 * @Author       : ITC
 * @Date         : 2026-08-19
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-19
 * @Description  : NVR独有配置业务处理（BG6_ZHSJ/BU_SJCL部门专用）
 *                 负责处理NVR特有功能的业务请求，独立于Common通用配置：
 *                 - 设备规模信息查询（GetDeviceInfo：设备类型/报警端口数/通道数）
 *                 - RTSP实时预览地址获取（GetRtspUrl）
 *                 - 通道信息查询（GetChannelInfo）
 *                 - 通道列表查询（GetChannelList）
 *                 - 录像文件列表查询（GetRecordFileList）
 *                 后续新增的NVR独有配置接口统一收口于此类，
 *                 避免Common/DeviceConfigBusiness.cpp随NVR功能扩展而膨胀。
 */

#pragma once

#include <string>
#include <cstring>

#include "Singleton.h"
#include "NetTVSDKServerInterface.h"
#include "NetTVNvrConfigCbExecute.h"
#include "NetTVNvrDeviceCbExecute.h"
#include "NetTVConfigCbExecute.h"
#include "DeviceInfoConvert.h"
#include "BG6_ZHSJ/BU_SJCL/RecordInfoConvert.h"
#include "SDKConvert.h"
#include "NetSdkLog.h"
#include "UrlParamUtils.h"

/**
 * NVR独有配置业务处理类
 * @details 单例模式，负责处理NVR特有功能的配置获取和设置请求。
 *          与Common/DeviceConfigBusiness解耦，NVR独有接口统一在此扩展。
 */
class CNvrBusiness : public CSingleton<CNvrBusiness>
{
    CNvrBusiness() {}
public:
    ~CNvrBusiness() {}
    friend class CSingleton<CNvrBusiness>;

public:
    /**
     * 获取设备规模信息
     * @details 走 NVR 侧专用回调 executeGetDeviceInfoCb，填充 NET_DeviceInfo_S
     *          （设备类型/报警输入端口数/报警输出端口数/通道数）。
     *          该结构体为 NVR 规模信息，归 NVR 侧，区别于通用设备基本信息
     *          NET_DeviceBasicInfo_S（后者收口于 Common 设备回调）。
     * @param req_data 请求数据（未使用）
     * @param url_param URL参数（未使用）
     * @return JSON格式的响应数据
     */
    std::string GetDeviceInfo(const std::string& req_data, const std::string& url_param);

    /**
     * 获取RTSP实时预览地址
     * @details RTSP为NVR独有配置，走NVR专用回调执行函数（NetTVNvrConfigCb）
     * @param nChannelId 通道号
     * @param nCommand 命令码
     * @return JSON格式的响应数据
     */
    std::string HandleGetRtspUrl(INT32 nChannelId, INT32 nCommand);

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
    std::string HandleGetChannelInfo(INT32 nChannelId, INT32 nCommand);

    /**
     * 获取录像文件列表
     * @details 从URL参数中解析查询条件（通道号、类型、日期、时间范围、文件名等），调用SDK获取录像文件列表
     * @param nChannelId 通道号
     * @param nCommand 命令码
     * @param url_param URL参数
     * @return JSON格式的响应数据
     */
    std::string HandleGetRecordFileList(INT32 nChannelId, INT32 nCommand, const std::string& url_param);
};
