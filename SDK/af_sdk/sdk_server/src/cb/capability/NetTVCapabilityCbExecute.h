/**
 * @file NetTVCapabilityCbExecute.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief NetTVCapabilityCbExecute 模块接口与类型定义
 * 功能说明：
 * 1. 声明 NetTVCapabilityCbExecute 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#ifndef NETSDK_CAPABILITY_CALLBACK_EXECUTE_H
#define NETSDK_CAPABILITY_CALLBACK_EXECUTE_H

#include "NetTVSDKServerInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行视频编码能力集回调 (NET_TV_CAP_VIDEO_ENCODE)
 * @param [in]  dwChannelID  通道号
 * @param [out] pCap         视频编码能力集结构体指针(多码流)
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
int NetSDK_ExecuteCb_GetVideoEncodeCap(INT32 dwChannelID, pNET_VideoEncodeCap_S pCap);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行音频编码能力集回调 (NET_TV_CAP_AUDIO)
 * @param [in]  dwChannelID  通道号
 * @param [out] pCap         视频编码能力集结构体指针(多码流)
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
int NetSDK_ExecuteCb_GetAudioCap(INT32 dwChannelID, pNET_AudioCap_S pCap);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行OSD能力集回调 (NET_TV_CAP_OSD)
 * @param [in]  dwChannelID  通道号
 * @param [out] pCap         OSD能力集结构体指针
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
int NetSDK_ExecuteCb_GetOsdCap(INT32 dwChannelID, pNET_OsdCap_S pCap);

/* ==================== 后续扩展能力集执行接口 ==================== */
/* int NetSDK_ExecuteCb_GetOsdCap(...); */
/* int NetSDK_ExecuteCb_GetSmartCap(...); */
/* int NetSDK_ExecuteCb_GetImageCap(...); */
/* int NetSDK_ExecuteCb_GetAudioCap(...); */

#ifdef __cplusplus
}
#endif

#endif
