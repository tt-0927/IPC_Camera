/**
 * @file NetTVCapabilityCbExecute.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-01-30
 * 
 * @brief 能力集回调执行头文件
 */
#ifndef _NETSDKCAPABILITYCBEXECUTE_H
#define _NETSDKCAPABILITYCBEXECUTE_H

#include "NetTVSDKServerInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 执行视频编码能力集回调 (NET_TV_CAP_VIDEO_ENCODE)
 * @param [IN]  dwChannelID  通道号
 * @param [OUT] pCap         视频编码能力集结构体指针(多码流)
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
int NetSDK_ExecuteCb_GetVideoEncodeCap(INT32 dwChannelID, LPNET_TV_VIDEO_ENCODE_CAP_S pCap);

/**
 * @brief 执行音频编码能力集回调 (NET_TV_CAP_AUDIO)
 * @param [IN]  dwChannelID  通道号
 * @param [OUT] pCap         视频编码能力集结构体指针(多码流)
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
int NetSDK_ExecuteCb_GetAudioCap(INT32 dwChannelID, LPNET_TV_AUDIO_CAP_S pCap); 

/**
 * @brief 执行OSD能力集回调 (NET_TV_CAP_OSD)
 * @param [IN]  dwChannelID  通道号
 * @param [OUT] pCap         OSD能力集结构体指针
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
int NetSDK_ExecuteCb_GetOsdCap(INT32 dwChannelID, LPNET_TV_OSD_CAP_S pCap);

// ==================== 后续扩展能力集执行接口 ====================
// int NetSDK_ExecuteCb_GetOsdCap(...);
// int NetSDK_ExecuteCb_GetSmartCap(...);
// int NetSDK_ExecuteCb_GetImageCap(...);
// int NetSDK_ExecuteCb_GetAudioCap(...);

#ifdef __cplusplus
}
#endif

#endif
