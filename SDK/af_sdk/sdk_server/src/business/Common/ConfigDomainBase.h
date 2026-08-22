/*
 * @FilePath     : sdk_new/sdk_server/src/business/Common/ConfigDomainBase.h
 * @Author       : chenchl
 * @Date         : 2026-08-20
 * @LastEditors  : chenchl
 * @LastEditTime : 2026-08-20
 * @Description  : 配置域接口与模板工具函数
 *                 定义统一的配置域接口 IConfigDomain，以及提供各域复用的模板工具函数。
 *                 各业务域（Common/SJGZ/SJCL）继承 CConfigDomainBase 实现自己的命令查表分发，
 *                 DeviceConfigBusiness 只做一级路由：依次询问各域。
 *                 - IConfigDomain    : 域接口（TryHandleGet/TryHandleSet）
 *                 - CConfigDomainBase: 基类（m_getTable/m_setTable + TemplatedGet/Set 模板）
 */
#pragma once

#include <string>
#include <cstring>
#include <unordered_map>
#include <functional>

#include "NetTVSDKServerInterface.h"
#include "NetTVConfigCbExecute.h"
#include "NetTVDeviceCbExecute.h"
#include "DeviceInfoConvert.h"
#include "BG6_ZHSJ/BU_SJGZ/CapabilityInfoConvert.h"
#include "BG6_ZHSJ/BU_SJGZ/IpcInfoConvert.h"
#include "BG6_ZHSJ/BU_SJCL/AlarmInfoConvert.h"
#include "BG6_ZHSJ/BU_SJCL/RecordInfoConvert.h"
#include "BG6_ZHSJ/BU_SJCL/NvrInfoConvert.h"
#include "BG6_ZHSJ/BU_SJLB/RecordInfoConvert.h"
#include "SDKConvert.h"
#include "NetSdkLog.h"

/**
 * 配置域统一接口
 * @details 各业务域实现此接口，DeviceConfigBusiness 依次询问各域处理命令。
 *          命中返回 true 并填充 outResp，未命中返回 false。
 */
class IConfigDomain
{
public:
    virtual ~IConfigDomain() {}

    /* 尝试处理 Get 请求，命中返回 true */
    virtual bool TryHandleGet(INT32 nChannelId, INT32 nCommand,
                             const std::string& req_data,
                             const std::string& url_param,
                             std::string& outResp) = 0;

    /* 尝试处理 Set 请求，命中返回 true */
    virtual bool TryHandleSet(INT32 nChannelId, INT32 nCommand,
                             const std::string& req_data,
                             const std::string& url_param,
                             std::string& outResp) = 0;
};

/**
 * 配置域基类：提供模板工具函数和查表机制
 * @details 各域继承此类，在构造函数中注册命令码→处理函数映射，
 *          TryHandleGet/TryHandleSet 内部查表分发。
 *          命令码不需要连续，用 unordered_map O(1) 查找。
 */
class CConfigDomainBase : public IConfigDomain
{
public:
    /* ===== 处理函数类型 ===== */
    using GetHandler = std::function<std::string(INT32, INT32, const std::string&, const std::string&)>;
    using SetHandler = std::function<std::string(INT32, INT32, const std::string&, const std::string&)>;

    /* 查表分发：命中返回 true */
    bool TryHandleGet(INT32 nChannelId, INT32 nCommand,
                      const std::string& req_data,
                      const std::string& url_param,
                      std::string& outResp) override
    {
        auto it = m_getTable.find(nCommand);
        if (it == m_getTable.end())
        {
            return false; /* 不归我管 */
        }
        outResp = it->second(nChannelId, nCommand, req_data, url_param);
        return true;
    }

    bool TryHandleSet(INT32 nChannelId, INT32 nCommand,
                      const std::string& req_data,
                      const std::string& url_param,
                      std::string& outResp) override
    {
        auto it = m_setTable.find(nCommand);
        if (it == m_setTable.end())
        {
            return false; /* 不归我管 */
        }
        outResp = it->second(nChannelId, nCommand, req_data, url_param);
        return true;
    }

protected:
    std::unordered_map<INT32, GetHandler> m_getTable; /* Get 命令查表 */
    std::unordered_map<INT32, SetHandler> m_setTable; /* Set 命令查表 */

    /* ===== 模板工具函数：通用 Get/Set（复用原 HandleGetConfig/HandleSetConfig 逻辑） ===== */

    /**
     * 通用模板 Get：调配置回调获取结构体，转 JSON 返回
     * @tparam T_CFG 配置结构体类型
     */
    template<typename T_CFG>
    static std::string TemplatedGet(INT32 nChannelId, INT32 nCommand,
                                    const std::string& req_data,
                                    const std::string& url_param)
    {
        (void)req_data;
        (void)url_param;

        T_CFG stCfg;
        memset(&stCfg, 0, sizeof(T_CFG));

        NETSDK_LOG_MESSAGE_INFO("GetDevConfig callback START");
        int nRespCode = executeGetDevConfigCb(nChannelId, nCommand, &stCfg);
        if (nRespCode != NET_E_SUCCEED)
        {
            NETSDK_LOG_MESSAGE_WARN("GetDevConfig callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
        }
        NETSDK_LOG_MESSAGE_INFO("GetDevConfig callback cmd=%d, ret=%d", nCommand, nRespCode);
        return SDKConvert::to_respString(nRespCode, nCommand, nChannelId, stCfg);
    }

    /**
     * 通用模板 Set：解析 JSON，调配置回调设置
     * @tparam T_CFG 配置结构体类型
     */
    template<typename T_CFG>
    static std::string TemplatedSet(INT32 nChannelId, INT32 nCommand,
                                    const std::string& req_data,
                                    const std::string& url_param)
    {
        (void)url_param;

        if (req_data.empty())
        {
            return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
        }

        T_CFG stCfg;
        memset(&stCfg, 0, sizeof(T_CFG));

        Json::Object* pRoot = Json::init(req_data);
        if (!pRoot)
        {
            return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
        }

        SDKConvert::deal(pRoot, stCfg, true);
        Json::deinit(pRoot);

        int nRespCode = executeSetDevConfigCb(nChannelId, nCommand, &stCfg);
        if (nRespCode != NET_E_SUCCEED)
        {
            NETSDK_LOG_MESSAGE_WARN("SetDevConfig callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
        }

        return SDKConvert::to_respString((NET_COMMON_ECODE_E)nRespCode, nCommand);
    }
};
