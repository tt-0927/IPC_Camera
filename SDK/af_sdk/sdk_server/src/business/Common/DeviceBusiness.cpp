/*
 * @FilePath     : sdk_new/sdk_server/src/business/Common/DeviceBusiness.cpp
 * @Author       : ITC
 * @Date         : 2026-08-19
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-19
 * @Description  : 设备级业务处理实现（通用，跨设备共用）
 *                 日志查询/导出、升级状态/版本/设置，从 DeviceConfigBusiness 迁入。
 *                 这些是设备级操作而非配置读写，参考海康/大华均独立于 GetDevConfig。
 */

#include "DeviceBusiness.h"

#include <cstdlib>

/* ===================== 日志查询/导出 ===================== */

/**
 * @brief 查询/导出设备日志
 * @details 从URL参数解析查询条件（类型、动作、时间范围、页码），
 *          调用 executeGetDevConfigCb 获取日志列表。
 *          从 DeviceConfigBusiness 迁入，命令码 NET_FIND_LOG/NET_EXPORT_LOG 不变。
 * @param nChannelId 通道号（设备级操作，仅用于响应）
 * @param nCommand 命令码
 * @param url_param URL参数
 * @return JSON格式的响应数据
 */
std::string CDeviceBusiness::HandleGetLogList(INT32 nChannelId, INT32 nCommand, const std::string& url_param)
{
    NET_LogList_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    stCfg.stCond.nType = ParseIntParam(url_param, "Type", 0);
    stCfg.stCond.nAction = ParseIntParam(url_param, "Action", 0);

    std::string strStartTime = ParseStringParam(url_param, "StartTime");
    std::string strEndTime = ParseStringParam(url_param, "EndTime");
    snprintf(stCfg.stCond.szStartTime, sizeof(stCfg.stCond.szStartTime), "%s", strStartTime.c_str());
    snprintf(stCfg.stCond.szEndTime, sizeof(stCfg.stCond.szEndTime), "%s", strEndTime.c_str());

    stCfg.stPage.nCurPage = ParseIntParam(url_param, "CurPage", 1);
    stCfg.stPage.nPageSize = ParseIntParam(url_param, "PageSize", NET_LOG_QUERY_COND_NUM);
    if (stCfg.stPage.nCurPage == 0)
    {
        stCfg.stPage.nCurPage = 1;
    }
    if (stCfg.stPage.nPageSize <= 0)
    {
        stCfg.stPage.nPageSize = NET_LOG_QUERY_COND_NUM;
    }

    NETSDK_LOG_MESSAGE_INFO("GetLogList callback START");
    int nRespCode = executeGetDevConfigCb(nChannelId, nCommand, &stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("GetLogList callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }
    NETSDK_LOG_MESSAGE_INFO("GetLogList callback cmd=%d, ret=%d", nCommand, nRespCode);
    NETSDK_LOG_MESSAGE_INFO("GetLogList callback END");
    return SDKConvert::to_respString(nRespCode, nCommand, stCfg);
}

/* ===================== 升级状态/版本/设置 ===================== */

/**
 * @brief 获取设备升级状态
 * @details 查询当前固件升级进度/结果，走通用配置回调 executeGetDevConfigCb。
 *          从 DeviceConfigBusiness HandleGetConfig<NET_UpgradeStatus_S> 迁入。
 * @param nChannelId 通道号（设备级操作，仅用于响应）
 * @param nCommand 命令码
 * @return JSON格式的响应数据
 */
std::string CDeviceBusiness::HandleGetUpgradeStatus(INT32 nChannelId, INT32 nCommand)
{
    NET_UpgradeStatus_S stCfg;
    memset(&stCfg, 0, sizeof(NET_UpgradeStatus_S));

    NETSDK_LOG_MESSAGE_INFO("GetUpgradeStatus callback START");
    int nRespCode = executeGetDevConfigCb(nChannelId, nCommand, &stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("GetUpgradeStatus callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }
    NETSDK_LOG_MESSAGE_INFO("GetUpgradeStatus callback cmd=%d, ret=%d", nCommand, nRespCode);
    return SDKConvert::to_respString(nRespCode, nCommand, nChannelId, stCfg);
}

/**
 * @brief 获取设备升级版本信息
 * @details 查询设备固件版本信息，走通用配置回调 executeGetDevConfigCb。
 *          从 DeviceConfigBusiness HandleGetConfig<NET_UpgradeVersion_S> 迁入。
 * @param nChannelId 通道号（设备级操作，仅用于响应）
 * @param nCommand 命令码
 * @return JSON格式的响应数据
 */
std::string CDeviceBusiness::HandleGetUpgradeVersion(INT32 nChannelId, INT32 nCommand)
{
    NET_UpgradeVersion_S stCfg;
    memset(&stCfg, 0, sizeof(NET_UpgradeVersion_S));

    NETSDK_LOG_MESSAGE_INFO("GetUpgradeVersion callback START");
    int nRespCode = executeGetDevConfigCb(nChannelId, nCommand, &stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("GetUpgradeVersion callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }
    NETSDK_LOG_MESSAGE_INFO("GetUpgradeVersion callback cmd=%d, ret=%d", nCommand, nRespCode);
    return SDKConvert::to_respString(nRespCode, nCommand, nChannelId, stCfg);
}

/**
 * @brief 设置设备升级
 * @details 解析JSON请求体中的升级信息，触发设备固件升级，走通用配置回调 executeSetDevConfigCb。
 *          从 DeviceConfigBusiness HandleSetConfig<NET_UpgradeInfo_S> 迁入。
 * @param nChannelId 通道号（设备级操作）
 * @param nCommand 命令码
 * @param req_data 请求数据（JSON格式）
 * @return JSON格式的响应数据
 */
std::string CDeviceBusiness::HandleSetUpgrade(INT32 nChannelId, INT32 nCommand, const std::string& req_data)
{
    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    NET_UpgradeInfo_S stCfg;
    memset(&stCfg, 0, sizeof(NET_UpgradeInfo_S));

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
        NETSDK_LOG_MESSAGE_WARN("SetUpgrade callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }

    return SDKConvert::to_respString((NET_COMMON_ECODE_E)nRespCode, nCommand);
}
