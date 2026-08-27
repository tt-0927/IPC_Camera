/*
 * @FilePath     : sdk_new/sdk_server/src/cb/Common/config/NetTVConfigCbExecute.h
 * @Author        : chenchl (chenchl@kfb.cn)
 * @Date          : 2026-07-28
 * @LastEditors   : ITC
 * @LastEditTime  : 2026-08-18
 * @Description   : 通用设备配置回调执行声明
 *                  本文件声明：
 *                  1. 命令码回调注册函数（registerGetCmdCb/registerSetCmdCb）
 *                     供部门子模块（BG6_ZHSJ/BU_SJCL/NetTVEventConfigCb.c 等）调用
 *                  2. 核心命令码分发执行函数（executeGetDevConfigCb 等）
 */

#ifndef NETSDK_CONFIG_CALLBACK_EXECUTE_H
#define NETSDK_CONFIG_CALLBACK_EXECUTE_H

#include "NetTVSDKServerInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 命令码回调注册函数（按Get命令码注册获取配置回调；供部门子模块如BG6_ZHSJ/BU_SJCL/NetTVEventConfigCb.c等调用） */
BOOL registerGetCmdCb(INT32 nCommand, NET_CB_GetDevConfigByCommand pCb);

/* 命令码回调注册函数（按Set命令码注册设置配置回调；供部门子模块如BG6_ZHSJ/BU_SJGZ/NetTVIpcConfigCb.c等调用） */
BOOL registerSetCmdCb(INT32 nCommand, NET_CB_SetDevConfigByCommand pCb);

/* 通用配置获取回调分发执行函数（优先按命令码查找专用回调，未命中降级到通用回调；供CommonDomain/DeviceConfigBusiness等调用） */
int executeGetDevConfigCb(INT32 nChannelId, INT32 dwCommand, LPVOID lpOutBuffer);

/* 通用配置设置回调分发执行函数（优先按命令码查找专用回调，未命中降级到通用回调；供CommonDomain/DeviceConfigBusiness等调用） */
int executeSetDevConfigCb(INT32 nChannelId, INT32 dwCommand, LPVOID lpInBuffer);

#ifdef __cplusplus
}
#endif

#endif

