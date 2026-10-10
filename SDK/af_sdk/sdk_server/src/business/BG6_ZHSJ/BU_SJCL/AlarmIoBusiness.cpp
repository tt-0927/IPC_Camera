/*
 * 文件名：AlarmIoBusiness.cpp
 * 作者：ITC
 * 日期：2026-10-10
 * 功能：报警输入输出列表设置适配。
 * 修改记录：2026-10-10，新增列表 JSON 拆分并逐路调用 IPC TVSDK 设置回调。
 */

#include "AlarmIoBusiness.h"

#include <memory>

#include "AlarmInfoConvert.h"
#include "NetSdkLog.h"
#include "NetTVConfigCbExecute.h"
#include "SDKConvert.h"

namespace
{
/**
 * 功能：将单路或列表报警 JSON 转换为单路结构并逐路设置。
 * param [in] nChannelId：设备通道标识。
 * param [in] nCommand：设置命令。
 * param [in] strRequest：单路或列表 JSON。
 * param [in] pListKey：列表字段名。
 * param [out]：无。
 * return：设置结果响应 JSON。
 * 说明：仅识别 JSON 对象和数组，不重复检查报警业务范围；逐路失败即停止，
 *       已成功保存的配置不回滚。每次请求独占 JSON，不持锁执行设备回调。
 */
template<typename TSingle>
static std::string set_alarm_io(INT32 nChannelId, INT32 nCommand,
                                const std::string& strRequest,
                                const char* pListKey)
{
    std::unique_ptr<Json::Object, decltype(&cJSON_Delete)> pRootJson(
        Json::init(strRequest), &cJSON_Delete);
    if (!cJSON_IsObject(pRootJson.get()))
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    Json::Object* pList = cJSON_GetObjectItemCaseSensitive(pRootJson.get(), pListKey);
    if (pList && !cJSON_IsArray(pList))
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }
    if (pList)
    {
        /* 先确认所有列表项均为对象，避免格式错误时写入默认配置。 */
        for (Json::Object* pItem = pList->child; pItem; pItem = pItem->next)
        {
            if (!cJSON_IsObject(pItem))
            {
                return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
            }
        }
    }

    /* 直接逐项转换，不经固定容量列表结构体，避免静默截断和大块栈占用。 */
    Json::Object* pItem = pList ? pList->child : pRootJson.get();
    while (pItem)
    {
        TSingle stConfig{};
        SDKConvert::deal(pItem, stConfig, true);
        const NET_COMMON_ECODE_E enResult = static_cast<NET_COMMON_ECODE_E>(
            executeSetDevConfigCb(nChannelId, nCommand, &stConfig));
        if (enResult != NET_E_SUCCEED)
        {
            NETSDK_LOG_MESSAGE_WARN("报警输入输出设置回调失败: cmd=%d, ret=%d",
                                   nCommand, enResult);
            return SDKConvert::to_respString(enResult, nCommand);
        }
        pItem = pList ? pItem->next : nullptr;
    }
    return SDKConvert::to_respString(NET_E_SUCCEED, nCommand);
}
}
/**
 * 功能：设置报警输入配置，兼容单路对象和列表对象。
 * param [in] nChannelId：设备通道标识。
 * param [in] nCommand：报警输入设置命令。
 * param [in] strRequest：单路或列表 JSON。
 * param [in] strUrlParam：URL 参数。
 * param [out]：无。
 * return：设置结果响应 JSON。
 */
std::string CAlarmIoBusiness::HandleSetAlarmInputInfo(INT32 nChannelId, INT32 nCommand,
                                                      const std::string& strRequest,
                                                      const std::string& strUrlParam)
{
    (void)strUrlParam;
    if (nCommand != NET_SET_ALARM_INPUT_INFO)
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }
    return set_alarm_io<NET_AlarmInputInfo_S>(
        nChannelId, nCommand, strRequest, "AlarmInputs");
}
/**
 * 功能：设置报警输出配置，兼容单路对象和列表对象。
 * param [in] nChannelId：设备通道标识。
 * param [in] nCommand：报警输出设置命令。
 * param [in] strRequest：单路或列表 JSON。
 * param [in] strUrlParam：URL 参数。
 * param [out]：无。
 * return：设置结果响应 JSON。
 */
std::string CAlarmIoBusiness::HandleSetAlarmOutputInfo(INT32 nChannelId, INT32 nCommand,
                                                       const std::string& strRequest,
                                                       const std::string& strUrlParam)
{
    (void)strUrlParam;
    if (nCommand != NET_SET_ALARM_OUTPUT_INFO)
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }
    return set_alarm_io<NET_AlarmOutputInfo_S>(
        nChannelId, nCommand, strRequest, "AlarmOutputs");
}
