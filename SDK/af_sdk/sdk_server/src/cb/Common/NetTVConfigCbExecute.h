/*
 * @FilePath     : sdk_new/sdk_server/src/cb/Common/config/NetTVConfigCbExecute.h
 * @Author        : tianl (tianl@kfb.cn)
 * @Date          : 2026-07-28
 * @LastEditors   : ITC
 * @LastEditTime  : 2026-08-18
 * @Description   : 通用设备配置回调执行声明
 *                  本文件声明：
 *                  1. 命令码回调注册函数（Net_RegisterGetCmdCb/Net_RegisterSetCmdCb）
 *                     供部门子模块（BG6_ZHSJ/BU_SJCL/NetTVEventConfigCb.c 等）调用
 *                  2. 核心命令码分发执行函数（NetSDK_ExecuteCb_GetDevConfig 等）
 */

#ifndef NETSDK_CONFIG_CALLBACK_EXECUTE_H
#define NETSDK_CONFIG_CALLBACK_EXECUTE_H

#include "NetTVSDKServerInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 命令码回调注册函数（供部门子模块如BG6_ZHSJ/BU_SJCL的NetTVEventConfigCb.c调用） */
BOOL Net_RegisterGetCmdCb(INT32 nCommand, NET_CB_GetDevConfigByCommand pCb);
BOOL Net_RegisterSetCmdCb(INT32 nCommand, NET_CB_SetDevConfigByCommand pCb);

int NetSDK_ExecuteCb_GetDevConfig(INT32 dwChannelID, INT32 dwCommand, LPVOID lpOutBuffer);
int NetSDK_ExecuteCb_SetDevConfig(INT32 dwChannelID, INT32 dwCommand, LPVOID lpInBuffer);
int NetSDK_ExecuteCb_GetReplayUrl(pNET_ReplayUrlInfo_S pInfo);
int NetSDK_ExecuteCb_ControlReplay(pNET_ReplayCtrlInfo_S pInfo);
int NetSDK_ExecuteCb_GetReplayRecordList(pNET_ReplayRecordList_S pInfo);

#ifdef __cplusplus
}
#endif

#endif

