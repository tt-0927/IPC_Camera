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