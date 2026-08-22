/*
 * @FilePath     : sdk_new/sdk_server/src/cb/BG6_ZHSJ/BU_SJCL/NetTVNvrConfigCbExecute.h
 * @Author       : ITC
 * @Date         : 2026-08-19
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-19
 * @Description  : NVR独有配置回调执行声明（BG6_ZHSJ/BU_SJCL部门专用）
 *                 本文件声明NVR特有功能的回调执行函数：
 *                 1. RTSP流地址获取（executeGetRtspUrlCb）
 *                 2. 回放URL获取（executeGetReplayUrlCb）
 *                 3. 回放控制（executeControlReplayCb）
 *                 4. 回放录像列表获取（executeGetReplayRecordListCb）
 *                 这些功能为NVR特有，不属于通用设备配置，故从Common迁移至此。
 */

#ifndef NETSDK_NVR_CONFIG_CALLBACK_EXECUTE_H
#define NETSDK_NVR_CONFIG_CALLBACK_EXECUTE_H

#include "NetTVSDKServerInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

/* NVR独有：执行获取RTSP流地址回调（NET_GET_RTSPURLCFG；供NvrBusiness等业务层调用） */
int executeGetRtspUrlCb(INT32 dwChannelID, pNET_RtspUrlInfo_S pInfo);

/* NVR独有：执行获取回放播放URL回调（按起止时间+通道查询录像；供PlaybackBusiness等调用） */
int executeGetReplayUrlCb(pNET_ReplayUrlInfo_S pInfo);

/* NVR独有：执行回放控制回调（开始/停止/倍速/定位等；供PlaybackBusiness等调用） */
int executeControlReplayCb(pNET_ReplayCtrlInfo_S pInfo);

/* NVR独有：执行获取回放录像时间段列表回调；供PlaybackBusiness等调用 */
int executeGetReplayRecordListCb(pNET_ReplayRecordList_S pInfo);

#ifdef __cplusplus
}
#endif

#endif
