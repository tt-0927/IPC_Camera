/*
 * @FilePath     : sdk/af_sdk/sdk_server/src/cb/BG6_ZHSJ/BU_SJLB/NetTVRecordConfigCbExecute.h
 * @Author       : ITC
 * @Date         : 2026-08-20
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-20
 * @Description  : 录播部门（BU_SJLB）专用配置回调执行声明
 *                 本文件目前仅作为注册函数的前向声明占位，执行函数（NetSDK_ExecuteCb_GetDevConfig
 *                 /NetSDK_ExecuteCb_SetDevConfig）统一定义于 Common/config/NetTVConfigCbExecute.h，
 *                 通过命令码动态分发表统一调度，无需为本部门单独提供执行入口。
 *                 保留此文件以便未来扩展部门独有的录播分发逻辑。
 */

#ifndef NETSDK_RECORD_CONFIG_CALLBACK_EXECUTE_H
#define NETSDK_RECORD_CONFIG_CALLBACK_EXECUTE_H

#include "NetTVSDKServerInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 当前无部门独立执行函数，所有执行逻辑统一走 Common/config/NetTVConfigCbExecute.h 的命令码分发 */

#ifdef __cplusplus
}
#endif

#endif
