/*
 * @FilePath     : sdk/af_sdk/sdk_server/src/cb/BG6_ZHSJ/BU_SJLB/NetTVRecordConfigCb.c
 * @Author       : ITC
 * @Date         : 2026-08-20
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-20
 * @Description  : 录播部门（BU_SJLB）专用配置回调注册实现
 *                 包含：
 *                 1. 注册信息获取回调（NET_GET_REGISTERINFO / 520）
 *                 2. 注册信息设置回调（NET_SET_REGISTERINFO / 521）
 *                 依赖：所有注册函数最终调用 registerGetCmdCb/registerSetCmdCb
 *                       （定义于Common/config/NetTVConfigCb.c，声明于NetTVConfigCbExecute.h）
 */

#include <stdio.h>
#include <stddef.h>
#include "NetTVRecordConfigCbExecute.h"
#include "NetTVConfigCbExecute.h"
#include "NetTVSDKServerInterface.h"


/* ===================== 注册信息回调 ===================== */

/**
 * @brief 注册获取注册信息的回调函数
 * @param [in] pCb 用于填充 NET_RegisterInfo_S 输出缓冲区的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note 520：获取机器码、注册码、注册时间、可用时长、激活类型
 */
NET_API BOOL STDCALL NET_serverRegisterGetRegisterInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_REGISTERINFO, pCb);
}

/**
 * @brief 注册设置注册信息的回调函数
 * @param [in] pCb 接收 NET_RegisterInfo_S 输入缓冲区并执行注册的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note 521：传入注册码执行设备注册
 */
NET_API BOOL STDCALL NET_serverRegisterSetRegisterInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_REGISTERINFO, pCb);
}

/**
 * @brief 注册录制控制回调函数
 * @param [in] pCb 用于执行录制启停控制的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterControlRecordInfoExCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_CONTROL_RECORD, pCb);
}

/**
 * @brief 注册获取录制状态回调函数
 * @param [in] pCb 用于填充 NET_RecordControlInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetRecordStatusExCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_RECORD_INFO, pCb);
}

/**
 * @brief 注册直播控制回调函数
 * @param [in] pCb 用于执行直播启停控制的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterControlLiveInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_CONTROL_LIVE, pCb);
}

/**
 * @brief 注册获取直播状态回调函数
 * @param [in] pCb 用于填充 NET_LiveStatusInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetLiveStatusCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_LIVE_STATUS, pCb);
}

/**
 * @brief 注册获取录制文件列表回调函数
 * @param [in] pCb 用于填充 NET_RecordFileInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note 526：获取录制文件列表，支持分页查询
 */
NET_API BOOL STDCALL NET_serverRegisterGetRecordFileListCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_RECORD_FILE_LIST, pCb);
}

/**
 * @brief 注册设置导播模式回调函数
 * @param [in] pCb 接收 NET_DirectorModeInfo_S 输入缓冲区并设置导播模式的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note 527：设置导播模式（半自动/自动/手动）
 */
NET_API BOOL STDCALL NET_serverRegisterSetDirectorModeCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_DIRECTOR_MODE, pCb);
}

/**
 * @brief 注册云台控制回调函数
 * @param [in] pCb 接收 NET_CameraControlInfo_S 输入缓冲区的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note 528：云台控制（上下左右/变焦/预置位）
 */
NET_API BOOL STDCALL NET_serverRegisterControlCameraCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_CONTROL_CAMERA, pCb);
}

/**
 * @brief 注册获取预置位列表回调函数
 * @param [in] pCb 用于填充 NET_PresetBitInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note 529：获取预置位列表
 */
NET_API BOOL STDCALL NET_serverRegisterGetPresetBitCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_CONTROL_PRESET_BIT, pCb);
}

/**
 * @brief 注册预置位控制回调函数
 * @param [in] pCb 接收 NET_PresetBitCtrl_S 输入缓冲区的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note 529：设置/调用/删除预置位
 */
NET_API BOOL STDCALL NET_serverRegisterControlPresetBitCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_CONTROL_PRESET_BIT, pCb);
}

/**
 * @brief 注册调用中控回调函数
 * @param [in] pCb 接收 NET_ExternalControlInfo_S 输入缓冲区的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note 530：调用中控
 */
NET_API BOOL STDCALL NET_serverRegisterControlExternalCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_CONTROL_EXTERNAL, pCb);
}

/**
 * @brief 注册获取布局信息回调函数
 * @param [in] pCb 用于填充 NET_LayoutSelfInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note 531：获取布局信息
 */
NET_API BOOL STDCALL NET_serverRegisterGetLayoutCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_CONTROL_LAYOUT, pCb);
}

/**
 * @brief 注册布局控制回调函数
 * @param [in] pCb 接收 NET_LayoutSelfInfo_S 输入缓冲区的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note 531：设置/使用布局
 */
NET_API BOOL STDCALL NET_serverRegisterControlLayoutCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_CONTROL_LAYOUT, pCb);
}

/**
 * @brief 注册PVW输出到PGM模式回调函数
 * @param [in] pCb 接收 NET_PVW2PGMInfo_S 输入缓冲区的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note 532：设置PVW输出到PGM模式
 */
NET_API BOOL STDCALL NET_serverRegisterSetPVW2PGMCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_PVW2PGM, pCb);
}

/**
 * @brief 注册获取预约录制列表回调函数
 * @param [in] pCb 用于填充 NET_AppointmentInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note 533：获取预约录制列表
 */
NET_API BOOL STDCALL NET_serverRegisterGetAppointmentInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_APPOINTMENT_INFO, pCb);
}

/**
 * @brief 注册添加预约录制回调函数
 * @param [in] pCb 接收 NET_AppointmentItem_S 输入缓冲区的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note 534：添加预约录制
 */
NET_API BOOL STDCALL NET_serverRegisterAddAppointmentCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_ADD_APPOINTMENT, pCb);
}

/**
 * @brief 注册设备重启控制回调函数
 * @param [in] pCb 接收 NET_RebootInfo_S 输入缓冲区的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note 535：控制设备重启
 */
NET_API BOOL STDCALL NET_serverRegisterControlRebootCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_CONTROL_REBOOT, pCb);
}

/**
 * @brief 注册获取输出音量回调 (command=536)
 * @param [in] pCb 回调函数指针
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetOutVolumeCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_OUT_VOLUME, pCb);
}

/**
 * @brief 注册设置输出音量回调 (command=537)
 * @param [in] pCb 回调函数指针
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetOutVolumeCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_OUT_VOLUME, pCb);
}

/**
 * @brief 注册获取SSH安全信息回调 (command=538)
 * @param [in] pCb 回调函数指针
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetSshSafeInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_SSH_SAFE_INFO, pCb);
}

/**
 * @brief 注册设置SSH安全信息回调 (command=539)
 * @param [in] pCb 回调函数指针
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetSshSafeInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_SSH_SAFE_INFO, pCb);
}