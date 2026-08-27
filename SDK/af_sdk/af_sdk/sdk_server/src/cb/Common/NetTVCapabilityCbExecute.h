/**
 * @file NetTVCapabilityCbExecute.h
 * @author chenchl (chenchl@kfb.cn)
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
 * @brief 执行视频编码能力集回调 (NET_CAP_VIDEO_ENCODE)
 * @param [IN]  nChannelId  通道号
 * @param [OUT] pCap         视频编码能力集结构体指针(多码流)
 * @return NET_E_SUCCEED 成功, 其他值失败
 */
int executeGetVideoEncodeCapCb(INT32 nChannelId, pNET_VideoEncodeCap_S pCap);

/**
 * @brief 执行音频编码能力集回调 (NET_CAP_AUDIO)
 * @param [IN]  nChannelId  通道号
 * @param [OUT] pCap         视频编码能力集结构体指针(多码流)
 * @return NET_E_SUCCEED 成功, 其他值失败
 */
int executeGetAudioCapCb(INT32 nChannelId, pNET_AudioCap_S pCap);

/**
 * @brief 执行OSD能力集回调 (NET_CAP_OSD)
 * @param [IN]  nChannelId  通道号
 * @param [OUT] pCap         OSD能力集结构体指针
 * @return NET_E_SUCCEED 成功, 其他值失败
 */
int executeGetOsdCapCb(INT32 nChannelId, pNET_OsdCap_S pCap);

// ==================== 后续扩展能力集执行接口 ====================
// int executeGetOsdCapCb(...);
// int executeGetSmartCapCb(...);
// int executeGetImageCapCb(...);
// int executeGetAudioCapCb(...);

#ifdef __cplusplus
}
#endif

#endif
