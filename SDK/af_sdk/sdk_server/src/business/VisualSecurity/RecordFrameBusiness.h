/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : RecordFrameBusiness.h
 * @Description  : 录像帧业务处理类，负责处理录像帧流的启动和停止请求
 */

#pragma once

#include <cstring>
#include <string>

#include "Singleton.h"
#include "NetTVSDKServerInterface.h"
#include "RecordFrameServer.h"
#include "DeviceInfoConvert.h"
#include "SDKConvert.h"
#include "NetSdkLog.h"

/**
 * @brief 录像帧业务处理类
 * @details 负责处理录像帧流的启动和停止HTTP请求，解析请求参数，调用RecordFrameServer处理实际业务逻辑，
 *          采用单例模式，继承自CSingleton
 */
class CRecordFrameBusiness : public CSingleton<CRecordFrameBusiness>
{
    /**
     * @brief 构造函数（私有，单例模式）
     */
    CRecordFrameBusiness() {}
public:
    /**
     * @brief 析构函数（公开）
     */
    ~CRecordFrameBusiness() {}
    /**
     * @brief 友元声明，允许CSingleton访问私有构造函数
     */
    friend class CSingleton<CRecordFrameBusiness>;

public:
    /**
     * @brief 处理启动录像帧流请求
     * @param req_data 请求数据（JSON格式），包含流启动条件（通道、时间范围、媒体类型等）
     * @param url_param URL参数（未使用）
     * @return 响应数据（JSON格式），包含流信息（流ID、端口、媒体类型等）
     */
    std::string StartRecordFrameStream(const std::string& req_data, const std::string& url_param);

    /**
     * @brief 处理停止录像帧流请求
     * @param req_data 请求数据（JSON格式），包含流ID
     * @param url_param URL参数（未使用）
     * @return 响应数据（JSON格式），包含操作结果
     */
    std::string StopRecordFrameStream(const std::string& req_data, const std::string& url_param);
};
