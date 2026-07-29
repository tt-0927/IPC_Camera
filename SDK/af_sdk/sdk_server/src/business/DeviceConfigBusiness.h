/**
 * @file DeviceConfigBusiness.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief DeviceConfigBusiness 模块接口与类型定义
 * 功能说明：
 * 1. 声明 DeviceConfigBusiness 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : DeviceConfigBusiness.h
 * @Description  : 设备配置业务处理模块，负责处理设备配置的获取和设置请求
 */

#pragma once

#include <string>
#include <cstring>

#include "Singleton.h"
#include "NetTVSDKServerInterface.h"
#include "NetTVConfigCbExecute.h"
#include "DeviceInfoConvert.h"
#include "VisualSecurity/CapabilityInfoConvert.h"
#include "VisualSecurity/AlarmInfoConvert.h"
#include "SDKConvert.h"
#include "NetSdkLog.h"
#include <sstream>

/**
 * 设备配置业务处理类
 * @details 单例模式，负责处理设备配置的获取和设置请求，根据命令码路由到对应的处理函数
 */
class CDeviceConfigBusiness : public CSingleton<CDeviceConfigBusiness>
{
    CDeviceConfigBusiness() {} /* 私有构造函数，禁止外部实例化 */
public:
    ~CDeviceConfigBusiness() {} /* 析构函数 */
    friend class CSingleton<CDeviceConfigBusiness>; /* 允许单例模板访问私有成员 */

public:
    /**
     * 获取设备配置
     * @details 解析URL参数中的通道号和命令码，根据命令码调用对应的获取配置处理函数
     * @param req_data 请求数据（未使用）
     * @param url_param URL参数（包含channel和command）
     * @return JSON格式的响应数据
     */
    std::string GetDevConfig(const std::string& req_data, const std::string& url_param);

    /**
     * 设置设备配置
     * @details 解析URL参数中的通道号和命令码，解析请求数据中的配置信息，根据命令码调用对应的设置配置处理函数
     * @param req_data 请求数据（JSON格式，包含配置信息）
     * @param url_param URL参数（包含channel和command）
     * @return JSON格式的响应数据
     */
    std::string SetDevConfig(const std::string& req_data, const std::string& url_param);

private:
    /**
     * 获取配置通用处理函数（模板）
     * @details 根据命令码调用SDK获取设备配置，将结果转换为JSON格式返回
     * @tparam T_CFG 配置结构体类型
     * @param channelId 通道号
     * @param command 命令码
     * @return JSON格式的响应数据
     */
    template<typename T_CFG>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 HandleGetConfig 定义的内联处理。
 * @param [in] channelId 函数处理参数。
 * @param [in] command 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
    std::string HandleGetConfig(INT32 channelId, INT32 command)
    {
        T_CFG stCfg;
        memset(&stCfg, 0, sizeof(T_CFG));
        NETSDK_LOG_MESSAGE_INFO("GetDevConfig callback START");
        NETSDK_LOG_MESSAGE_INFO("[SDK] stCfg address=%p, sizeof(T_CFG)=%zu\n", (void*)&stCfg, sizeof(T_CFG));

        int nRespCode = NetSDK_ExecuteCb_GetDevConfig(channelId, command, &stCfg);
        NETSDK_LOG_MESSAGE_INFO("[SDK] after callback: stCfg address=%p\n", (void*)&stCfg);
        if (nRespCode != NET_E_SUCCEED)
        {
            NETSDK_LOG_MESSAGE_WARN("GetDevConfig callback failed, cmd=%d, ret=%d", command, nRespCode);
        }
        NETSDK_LOG_MESSAGE_INFO("GetDevConfig callback cmd=%d, ret=%d", command, nRespCode);
        NETSDK_LOG_MESSAGE_INFO("GetDevConfig callback END");
        return SDKConvert::to_respString(nRespCode, stCfg);
    }

    /**
     * 设置配置通用处理函数（模板）
     * @details 解析JSON请求数据，根据命令码调用SDK设置设备配置
     * @tparam T_CFG 配置结构体类型
     * @param channelId 通道号
     * @param command 命令码
     * @param req_data 请求数据（JSON格式）
     * @return JSON格式的响应数据
     */
    template<typename T_CFG>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 HandleSetConfig 定义的内联处理。
 * @param [in] channelId 函数处理参数。
 * @param [in] command 函数处理参数。
 * @param [in] req_data 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
    std::string HandleSetConfig(INT32 channelId, INT32 command, const std::string& req_data)
    {
        if (req_data.empty())
        {
            return SDKConvert::to_respString(NET_E_INVALID_PARAM);
        }

        T_CFG stCfg;
        memset(&stCfg, 0, sizeof(T_CFG));

        Json::Object* pRoot = Json::init(req_data);
        if (!pRoot)
        {
            return SDKConvert::to_respString(NET_E_INVALID_PARAM);
        }

        SDKConvert::deal(pRoot, stCfg, true);
        Json::deinit(pRoot);

        int nRespCode = NetSDK_ExecuteCb_SetDevConfig(channelId, command, &stCfg);
        if (nRespCode != NET_E_SUCCEED)
        {
            NETSDK_LOG_MESSAGE_WARN("SetDevConfig callback failed, cmd=%d, ret=%d", command, nRespCode);
        }

        return SDKConvert::to_respString((NET_COMMON_ECODE_E)nRespCode);
    }

    /**
     * 解析URL参数中的整数值
     * @param url_param URL参数字符串
     * @param key 参数名
     * @param defaultVal 默认值（默认为0）
     * @return 解析到的整数值，未找到或解析失败则返回默认值
     */
    int ParseIntParam(const std::string& url_param, const std::string& key, int defaultVal = 0);

    /**
     * 解析URL参数中的字符串值
     * @param url_param URL参数字符串
     * @param key 参数名
     * @param defaultVal 默认值（默认为空字符串）
     * @return 解析到的字符串值，未找到则返回默认值
     */
    std::string ParseStringParam(const std::string& url_param, const std::string& key, const std::string& defaultVal = "");

    /**
     * URL解码
     * @param value 待解码的URL编码字符串
     * @return 解码后的字符串
     */
    std::string UrlDecode(const std::string& value);

    /**
     * 获取日志列表
     * @details 从URL参数中解析查询条件（类型、动作、时间范围、页码等），调用SDK获取日志列表
     * @param channelId 通道号
     * @param command 命令码
     * @param url_param URL参数
     * @return JSON格式的响应数据
     */
    std::string HandleGetLogList(INT32 channelId, INT32 command, const std::string& url_param);

    /**
     * 获取录像文件列表
     * @details 从URL参数中解析查询条件（通道号、类型、日期、时间范围、文件名等），调用SDK获取录像文件列表
     * @param channelId 通道号
     * @param command 命令码
     * @param url_param URL参数
     * @return JSON格式的响应数据
     */
    std::string HandleGetRecordFileList(INT32 channelId, INT32 command, const std::string& url_param);
};
