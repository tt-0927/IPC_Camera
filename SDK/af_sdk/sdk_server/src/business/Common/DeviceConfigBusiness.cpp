/**
 * @file DeviceConfigBusiness.cpp
 * @brief 设备配置业务处理实现（一级路由）
 * @details 重构后不再包含巨大的 switch-case，改为依次询问各配置域处理。
 *          各域内部自己管理命令码→处理函数映射，新增命令只需在对应域注册。
 */
#include "DeviceConfigBusiness.h"
#include "CommonDomain.h"

#ifndef BU_SJGZ_EXCLUDE
#include "BG6_ZHSJ/BU_SJGZ/SjgzDomain.h"
#endif

#ifndef BU_SJCL_EXCLUDE
#include "BG6_ZHSJ/BU_SJCL/SjclDomain.h"
#endif

#ifndef BU_SJLB_EXCLUDE
#include "BG6_ZHSJ/BU_SJLB/SjlbDomain.h"
#endif

#include "UrlParamUtils.h"

#include <cctype>
#include <cstdlib>
#include <memory>

/**
 * 判断命令码是否为设备级配置（不需要通道号）
 * @param nCommand 命令码
 * @return true 表示设备级命令，false 表示通道级命令
 */
static bool IsDeviceLevelCommand(INT32 nCommand)
{
    switch (nCommand)
    {
        /* ===== 设备基础/系统配置 ===== */
        case NET_GET_DEVICECFG:
        case NET_SET_DEVICECFG:
        case NET_GET_REGISTERINFO:
        case NET_SET_REGISTERINFO:
        case NET_CONTROL_REBOOT:
        case NET_GET_OUT_VOLUME:
        case NET_SET_OUT_VOLUME:
        case NET_GET_STORAGE_INFO:
        case NET_GET_NTPCFG:
        case NET_SET_NTPCFG:
        case NET_SET_SYSTEM_TIME:
        case NET_GET_UPGRADESTATUS:
        case NET_GET_UPGRADEVERSION:
        case NET_SET_UPGRADE:

        /* ===== 网络配置 ===== */
        case NET_GET_NETWORKCFG:
        case NET_SET_NETWORKCFG:
        case NET_GET_SECURITY_SERVICES_INFO:
        case NET_SET_SECURITY_SERVICES_INFO:
        case NET_GET_SSH_COUNTDOWN:

        /* ===== 日志 ===== */
        case NET_FIND_LOG:
        case NET_EXPORT_LOG:
        case NET_GET_LOG_SERVER:
        case NET_SET_LOG_SERVER:
        case NET_TEST_LOG_SERVER:

        /* ===== 通道信息 ===== */
        case NET_GET_CHANNEL_INFO:

        /* ===== 录像高级参数/下载 ===== */
        case NET_GET_RECORD_ADVANCED_PARAM:
        case NET_SET_RECORD_ADVANCED_PARAM:
        case NET_DOWNLOAD_RECORD_FILE:

        /* ===== 人脸库（设备级） ===== */
        case NET_GET_TARGET_LIB:
        case NET_ADD_TARGET_LIB:
        case NET_DEL_TARGET_LIB:
        case NET_SET_TARGET_LIB:
            return true;

        default:
            return false;
    }
}

/**
 * 判断命令码是否为各业务域独有的设备级命令
 * @details 依次询问各业务域，命令码→设备级判断由域自行维护，
 *          公用命令不在此处，见 IsDeviceLevelCommand。
 * @param nCommand 命令码
 * @return true 表示设备级命令，false 表示通道级命令
 */
static bool IsAnyDomainDeviceLevelCommand(INT32 nCommand)
{
#ifndef BU_SJLB_EXCLUDE
    /* BU_SJLB 域（录播独有命令：录制/直播/文件列表/导播/云台/预置位/中控/布局/PVW2PGM/预约录制） */
    if (CBujlbDomain::instance()->IsDeviceLevelCommand(nCommand))
    {
        return true;
    }
#endif

    return false;
}

/**
 * 获取设备配置（一级路由）
 * @details 解析URL参数中的通道号和命令码，依次询问各域处理，
 *          第一个命中的域返回结果。所有域都不认识则返回 NET_E_CMD_NOT_SUPPORT。
 *          询问顺序：Common（通用） → SJGZ（视频/报警/AI） → SJCL（NVR/录播） → SJLB（设备基础）
 * @param req_data 请求数据（未使用）
 * @param url_param URL参数（包含channel和nCommand）
 * @return JSON格式的响应数据
 */
std::string CDeviceConfigBusiness::GetDevConfig(const std::string& req_data, const std::string& url_param)
{
    INT32 nChannelId = ParseIntParam(url_param, NET_API_PARAM_CHANNEL, NET_API_PARAM_NVRCHN);
    INT32 nCommand = ParseIntParam(url_param, NET_API_PARAM_COMMAND, NET_CFG_INVALID);
    NETSDK_LOG_MESSAGE_INFO("GetDevConfig request: url[%s], channel=%d, nCommand=%d",
                  url_param.c_str(), nChannelId, nCommand);

    /* 通道级命令必须传入有效通道号 */
    if (nChannelId < 0 && !IsDeviceLevelCommand(nCommand) && !IsAnyDomainDeviceLevelCommand(nCommand))
    {
        NETSDK_LOG_MESSAGE_WARN("GetDevConfig: channel required but nChannelId=%d, nCommand=%d", nChannelId, nCommand);
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    /* ====== 依次询问各域处理 ====== */
    std::string strResp;

    /* 1. Common 域（通用，始终编入） */
    if (CCommonDomain::instance()->TryHandleGet(nChannelId, nCommand, req_data, url_param, strResp))
    {
        return strResp;
    }

    /* 2. SJGZ 域（视频/报警/AI，条件编译） */
#ifndef BU_SJGZ_EXCLUDE
    if (CSjgzDomain::instance()->TryHandleGet(nChannelId, nCommand, req_data, url_param, strResp))
    {
        return strResp;
    }
#endif

    /* 3. SJCL 域（NVR/录播，条件编译） */
#ifndef BU_SJCL_EXCLUDE
    if (CSjclDomain::instance()->TryHandleGet(nChannelId, nCommand, req_data, url_param, strResp))
    {
        return strResp;
    }
#endif

    /* 4. SJLB 域（设备基础，条件编译） */
#ifndef BU_SJLB_EXCLUDE
    if (CBujlbDomain::instance()->TryHandleGet(nChannelId, nCommand, req_data, url_param, strResp))
    {
        return strResp;
    }
#endif

    /* 所有域都不认识 → 命令不支持 */
    NETSDK_LOG_MESSAGE_WARN("Unsupported GetDevConfig nCommand: %d", nCommand);
    return SDKConvert::to_respString(NET_E_CMD_NOT_SUPPORT, nCommand);
}

/**
 * 设置设备配置（一级路由）
 * @details 解析URL参数中的通道号和命令码，依次询问各域处理，
 *          第一个命中的域返回结果。所有域都不认识则返回 NET_E_CMD_NOT_SUPPORT。
 * @param req_data 请求数据（JSON格式，包含配置信息）
 * @param url_param URL参数（包含channel和nCommand）
 * @return JSON格式的响应数据
 */
std::string CDeviceConfigBusiness::SetDevConfig(const std::string& req_data, const std::string& url_param)
{
    INT32 nChannelId = ParseIntParam(url_param, NET_API_PARAM_CHANNEL, NET_API_PARAM_NVRCHN);
    INT32 nCommand = ParseIntParam(url_param, NET_API_PARAM_COMMAND, NET_CFG_INVALID);
    NETSDK_LOG_MESSAGE_INFO("SetDevConfig request: url[%s], channel=%d, nCommand=%d",
                  url_param.c_str(), nChannelId, nCommand);

    /* 通道级命令必须传入有效通道号 */
    if (nChannelId < 0 && !IsDeviceLevelCommand(nCommand) && !IsAnyDomainDeviceLevelCommand(nCommand))
    {
        NETSDK_LOG_MESSAGE_WARN("SetDevConfig: channel required but nChannelId=%d, nCommand=%d", nChannelId, nCommand);
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    /* ====== 依次询问各域处理 ====== */
    std::string strResp;

    /* 1. Common 域（通用，始终编入） */
    if (CCommonDomain::instance()->TryHandleSet(nChannelId, nCommand, req_data, url_param, strResp))
    {
        return strResp;
    }

        /* 2. SJCL 域（NVR/录播，条件编译） */
#ifndef BU_SJCL_EXCLUDE
    if (CSjclDomain::instance()->TryHandleSet(nChannelId, nCommand, req_data, url_param, strResp))
    {
        return strResp;
    }
#endif

    /* 3. SJGZ 域（视频/报警/AI，条件编译） */
#ifndef BU_SJGZ_EXCLUDE
    if (CSjgzDomain::instance()->TryHandleSet(nChannelId, nCommand, req_data, url_param, strResp))
    {
        return strResp;
    }
#endif

     /* 4. SJLB 域（设备基础，条件编译） */
#ifndef BU_SJLB_EXCLUDE
    if (CBujlbDomain::instance()->TryHandleSet(nChannelId, nCommand, req_data, url_param, strResp))
    {
        return strResp;
    }
#endif


    /* 所有域都不认识 → 命令不支持 */
    NETSDK_LOG_MESSAGE_WARN("Unsupported SetDevConfig nCommand: %d", nCommand);
    return SDKConvert::to_respString(NET_E_CMD_NOT_SUPPORT, nCommand);
}
