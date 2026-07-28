/**
 * @file NetTVConfigCbExecute.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief NetTVConfigCbExecute 模块接口与类型定义
 * 功能说明：
 * 1. 声明 NetTVConfigCbExecute 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */

#ifndef NETSDK_CONFIG_CALLBACK_EXECUTE_H
#define NETSDK_CONFIG_CALLBACK_EXECUTE_H

#include "NetTVSDKServerInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

int NetSDK_ExecuteCb_GetDevConfig(INT32 dwChannelID, INT32 dwCommand, LPVOID lpOutBuffer);
int NetSDK_ExecuteCb_SetDevConfig(INT32 dwChannelID, INT32 dwCommand, LPVOID lpInBuffer);
int NetSDK_ExecuteCb_GetReplayUrl(pNET_ReplayUrlInfo_S pInfo);
int NetSDK_ExecuteCb_ControlReplay(pNET_ReplayCtrlInfo_S pInfo);
int NetSDK_ExecuteCb_GetReplayRecordList(pNET_ReplayRecordList_S pInfo);

#ifdef __cplusplus
}
#endif

#endif

