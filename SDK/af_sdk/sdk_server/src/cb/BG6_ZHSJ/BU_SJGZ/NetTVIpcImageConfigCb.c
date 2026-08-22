/*
 * @FilePath     : sdk_new/sdk_server/src/cb/BG6_ZHSJ/BU_SJGZ/NetTVIpcImageConfigCb.c
 * @Author       : ITC
 * @Date         : 2026-08-19
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-19
 * @Description  : IPC（摄像机）图像调节类配置回调注册实现（BG6_ZHSJ/BU_SJGZ部门专用）
 *                 与 NetTVIpcConfigCb.c（外设/连接类：SD/WiFi/4G/热点）分离，
 *                 专门承载IPC摄像机独有的图像调节类配置回调，避免单文件膨胀。
 *                 当前包含：
 *                 1. 曝光参数回调（GetExposureInfo/SetExposureInfo）
 *                 2. 日夜切换回调（GetDayNightInfo/SetDayNightInfo）
 *                 3. 背光补偿回调（GetBackLightInfo/SetBackLightInfo）
 *                 4. 降噪参数回调（GetDenoiseInfo/SetDenoiseInfo）
 *                 5. 白平衡回调（GetWhiteBalanceInfo/SetWhiteBalanceInfo）
 *                 6. 抓拍计划回调（GetCapturePlanInfo/SetCapturePlanInfo）
 *                 7. 抓拍参数回调（GetCaptureParamInfo/SetCaptureParamInfo）
 *                 依赖：所有注册函数最终调用 registerGetCmdCb/registerSetCmdCb
 *                       （定义于Common/config/NetTVConfigCb.c，声明于NetTVConfigCbExecute.h）
 *                 后续IPC独有的宽动态/强光抑制/锐化/除雾/镜像等图像参数
 *                 均统一扩展至本文件。
 */

#include <stdio.h>
#include <stddef.h>
#include "NetTVIpcImageConfigCbExecute.h"
#include "NetTVConfigCbExecute.h"
#include "NetTVSDKServerInterface.h"

/* ===================== 曝光参数回调（IPC独有） ===================== */

/**
 * @brief 注册获取曝光参数的回调函数
 * @param [in] pCb 用于填充 NET_ExposureInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：摄像机曝光时间/增益/模式等参数查询
 */
NET_API BOOL STDCALL NET_serverRegisterGetExposureInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_EXPOSURE_INFO, pCb);
}

/**
 * @brief 注册设置曝光参数的回调函数
 * @param [in] pCb 用于读取 NET_ExposureInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：摄像机曝光时间/增益/模式等参数配置
 */
NET_API BOOL STDCALL NET_serverRegisterSetExposureInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_EXPOSURE_INFO, pCb);
}

/* ===================== 日夜切换回调（IPC独有） ===================== */

/**
 * @brief 注册获取日夜切换参数的回调函数
 * @param [in] pCb 用于填充 NET_DayNightInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：摄像机日/夜模式切换阈值与策略查询
 */
NET_API BOOL STDCALL NET_serverRegisterGetDayNightInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_DAYNIGHT_INFO, pCb);
}

/**
 * @brief 注册设置日夜切换参数的回调函数
 * @param [in] pCb 用于读取 NET_DayNightInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：摄像机日/夜模式切换阈值与策略配置
 */
NET_API BOOL STDCALL NET_serverRegisterSetDayNightInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_DAYNIGHT_INFO, pCb);
}

/* ===================== 背光补偿回调（IPC独有） ===================== */

/**
 * @brief 注册获取背光补偿参数的回调函数
 * @param [in] pCb 用于填充 NET_BackLightInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：摄像机背光补偿区域/强度等参数查询
 */
NET_API BOOL STDCALL NET_serverRegisterGetBackLightInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_BACKLIGHT_INFO, pCb);
}

/**
 * @brief 注册设置背光补偿参数的回调函数
 * @param [in] pCb 用于读取 NET_BackLightInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：摄像机背光补偿区域/强度等参数配置
 */
NET_API BOOL STDCALL NET_serverRegisterSetBackLightInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_BACKLIGHT_INFO, pCb);
}

/* ===================== 降噪参数回调（IPC独有） ===================== */

/**
 * @brief 注册获取降噪参数的回调函数
 * @param [in] pCb 用于填充 NET_DenoiseInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：摄像机2D/3D降噪等级参数查询
 */
NET_API BOOL STDCALL NET_serverRegisterGetDenoiseInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_DENOISE_INFO, pCb);
}

/**
 * @brief 注册设置降噪参数的回调函数
 * @param [in] pCb 用于读取 NET_DenoiseInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：摄像机2D/3D降噪等级参数配置
 */
NET_API BOOL STDCALL NET_serverRegisterSetDenoiseInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_DENOISE_INFO, pCb);
}

/* ===================== 白平衡回调（IPC独有） ===================== */

/**
 * @brief 注册获取白平衡参数的回调函数
 * @param [in] pCb 用于填充 NET_WhiteBalanceInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：摄像机白平衡模式/色温等参数查询
 */
NET_API BOOL STDCALL NET_serverRegisterGetWhiteBalanceInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_WHITEBALANCE_INFO, pCb);
}

/**
 * @brief 注册设置白平衡参数的回调函数
 * @param [in] pCb 用于读取 NET_WhiteBalanceInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：摄像机白平衡模式/色温等参数配置
 */
NET_API BOOL STDCALL NET_serverRegisterSetWhiteBalanceInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_WHITEBALANCE_INFO, pCb);
}

/* ===================== 抓拍计划与参数回调（IPC独有） ===================== */

/**
 * @brief 注册获取抓拍计划配置的回调函数
 * @param [in] pCb 用于填充 NET_CapturePlanInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：摄像机周/日抓拍计划（执行抓拍的硬件是IPC）
 */
NET_API BOOL STDCALL NET_serverRegisterGetCapturePlanInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_CAPTURE_PLAN_INFO, pCb);
}

/**
 * @brief 注册设置抓拍计划配置的回调函数
 * @param [in] pCb 用于读取 NET_CapturePlanInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：摄像机周/日抓拍计划（执行抓拍的硬件是IPC）
 */
NET_API BOOL STDCALL NET_serverRegisterSetCapturePlanInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_CAPTURE_PLAN_INFO, pCb);
}

/**
 * @brief 注册获取抓拍参数的回调函数
 * @param [in] pCb 用于填充 NET_CaptureParamInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：摄像机抓图分辨率/质量/张数等参数
 */
NET_API BOOL STDCALL NET_serverRegisterGetCaptureParamInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_CAPTURE_PARAM_INFO, pCb);
}

/**
 * @brief 注册设置抓拍参数的回调函数
 * @param [in] pCb 用于读取 NET_CaptureParamInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：摄像机抓图分辨率/质量/张数等参数
 */
NET_API BOOL STDCALL NET_serverRegisterSetCaptureParamInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_CAPTURE_PARAM_INFO, pCb);
}

/**
 * @brief 注册获取 OSD 叠加能力集的回调函数
 * @param [in] pCb 用于填充 NET_OsdCapCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetOsdCapConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_OSDCAPCFG, pCb);
}

/**
 * @brief 注册设置 OSD 叠加配置的回调函数
 * @param [in] pCb 用于读取 NET_OsdCapCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetOsdCapConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_OSDCAPCFG, pCb);
}

/**
 * @brief 注册获取图像参数配置的回调函数
 * @param [in] pCb 用于填充 NET_ImageCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetImageConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_IMAGECFG, pCb);
}

/**
 * @brief 注册设置图像参数配置的回调函数
 * @param [in] pCb 用于读取 NET_ImageCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetImageConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_IMAGECFG, pCb);
}

/**
 * @brief 注册获取音频参数配置的回调函数
 * @param [in] pCb 用于填充 NET_AudioCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetAudioConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_AUDIOCFG, pCb);
}

/**
 * @brief 注册设置音频参数配置的回调函数
 * @param [in] pCb 用于读取 NET_AudioCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetAudioConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_AUDIOCFG, pCb);
}
