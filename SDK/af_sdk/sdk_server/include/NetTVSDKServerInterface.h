/**
 * @file NetTVSDKServerInterface.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief NetTVSDKServerInterface 模块接口与类型定义
 * 功能说明：
 * 1. 声明 NetTVSDKServerInterface 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */

#ifndef NETSDK_SERVER_INTERFACE_H
#define NETSDK_SERVER_INTERFACE_H

#ifdef NET_TV_SDK_SERVER_API
    #include "NetTVSDKServer.h"
#else

#include "NetTVSDKCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************/
/*                          SDK服务端核心接口                           */
/************************************************************************/
/**
 * @author tianl (tianl@kfb.cn)
 * @brief SDK服务端初始化
 * @param [in] dwPort 服务器端口号
 * @param [in] szUserName 用户名
 * @param [in] szPassword 密码
 * @return TRUE表示成功,其他表示失败 NET_TV_TRUE means success, and any other value means failure.
 * @note
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_Init(NET_TV_IN UINT32 udwPort,NET_TV_IN CHAR szUserName[NET_TV_LEN_132],NET_TV_IN CHAR szPassword[NET_TV_LEN_132]);

/**
* SDK 清理  SDK cleaning
* @return TRUE表示成功,其他表示失败 NET_TV_TRUE means success, and any other value means failure.
* @note
*/
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_Cleanup(void);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 设置日志
 * @param [in] dwLogLevel   日志的等级（默认为0）：0-表示关闭日志，1-表示只输出ERROR错误日志，2-输出ERROR错误信息和DEBUG调试信息，3-输出ERROR错误信息、DEBUG调试信息和INFO普通信息等所有信息
 * @param [in] strLogDir    日志路径
 * @param [in] dwLogFileSize 日志文件大小(单位：字节)
 * @param [in] dwLogFileNum 日志文件个数
 * @return TRUE表示成功,其他表示失败 NET_TV_TRUE means success, and any other value means failure.
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_SetLogToFile(NET_TV_IN INT32 dwLogLevel,NET_TV_IN CHAR  *strLogDir,NET_TV_IN INT32 dwLogFileSize,NET_TV_IN INT32 dwLogFileNum);

/**
* 获取SDK的版本信息 Get SDK version information
* @return SDK版本信息 SDK version information
* @note
* - 在两个高字节中高8位表示主版本,低八位表示次版本.两个低字节表示附加版本号如0x01080000：表示版本为1.8.0.0.
* - The two high bytes,The high-8-bit indicate the major version, and the low-8-bytes indicate the minor version.Two low bytes for additional version numbers For example, 0x01080000 means version 1.8.0.0
*/
NET_TV_API INT32 NET_TV_STDCALL NET_TV_SERVER_GetSDKVersion(void);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 获取当前在线客户端数量（活跃会话数）
 * @return 客户端数量
 */
NET_TV_API INT32 NET_TV_STDCALL NET_TV_SERVER_GetClientCount(void);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 设置用户名密码
 * @param [in] szUserName 用户名
 * @param [in] szPassword 密码
 * @return TRUE表示成功,其他表示失败 NET_TV_TRUE means success, and any other value means failure.
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_SetUserPasswd(NET_TV_IN CHAR szUserName[NET_TV_LEN_132],NET_TV_IN CHAR szPassword[NET_TV_LEN_132]);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 推送告警信息
 * @param [in] pAlarmer    告警设备信息
 * @param [in] lCommand    命令码(报警类型)，用于客户端按命令码反序列化结构体
 * @param [in] pAlarmInfo  具体告警结构体指针（类型由 lCommand 决定）
 * @param [in] dwBufLen    pAlarmInfo 长度（一般为 sizeof(对应结构体)）
 * @return TRUE表示成功,其他表示失败 NET_TV_TRUE means success, and any other value means failure.
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_PushAlarmInfo(NET_TV_IN NET_Alarmer_S *pAlarmer,
                                                    NET_TV_IN INT32 lCommand,
                                                    NET_TV_IN LPVOID pAlarmInfo,
                                                    NET_TV_IN INT32 dwBufLen);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 推送通道上下线状态
 * @param [in] pChannelInfo 通道信息，byOnline/nDevState 表示当前状态
 * @return TRUE表示成功,其他表示失败 NET_TV_TRUE means success, and any other value means failure.
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_PushChannelStatusInfo(NET_TV_IN NET_ChannelInfo_S *pChannelInfo);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetDeviceInfo 对应的服务端回调。
 * @param [in] CB 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetDeviceInfo(NET_TV_COMMON_ECODE_E (*CB)(pNET_DeviceInfo_S pInfo));

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 设备控制回调类型
 * @param [in] pstCtrlInfo 设备硬件控制参数，参见 NET_DeviceControlInfo_S
 * @return NET_TV_E_SUCCEED 成功，其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_DeviceControl)(pNET_DeviceControlInfo_S pstCtrlInfo);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册设备控制回调
 * @param [in] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_DeviceControl(NET_TV_CB_DeviceControl pCb);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 修改用户密码回调类型
 * @param [in] pPasswordInfo 修改密码参数，包含用户名、旧密码、新密码
 * @return NET_TV_E_SUCCEED 成功，其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_SetUserPassword)(pNET_UserPasswordInfo_S pPasswordInfo);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册修改用户密码回调
 * @param [in] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetUserPassword(NET_TV_CB_SetUserPassword pCb);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 视频编码能力集回调类型 (NET_TV_CAP_VIDEO_ENCODE)
 * @param [in]  dwChannelID  通道号
 * @param [out] pCap         视频编码能力集结构体指针(多码流)
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetVideoEncodeCap)(INT32 dwChannelID,
                                                             pNET_VideoEncodeCap_S pCap);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册视频编码能力集回调
 * @param [in] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetVideoEncodeCap(NET_TV_CB_GetVideoEncodeCap pCb);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 音频编码能力集回调类型 (NET_TV_CAP_AUDIO)
 * @param [in]  dwChannelID  通道号
 * @param [out] pCap         音频编码能力集结构体指针
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetAudioEncodeCap)(INT32 dwChannelID,
                                                             pNET_AudioCap_S pCap);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册音频编码能力集回调
 * @param [in] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetAudioEncodeCap(NET_TV_CB_GetAudioEncodeCap pCb);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief OSD能力集回调类型 (NET_TV_CAP_OSD)
 * @param [in]  dwChannelID  通道号
 * @param [out] pCap         OSD能力集结构体指针
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetOsdCap)(INT32 dwChannelID, pNET_OsdCap_S pCap);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册OSD能力集回调
 * @param [in] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetOsdCap(NET_TV_CB_GetOsdCap pCb);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 通用配置回调类型（按命令码分发）
 * @param [in] dwChannelID 通道号
 * @param [in] dwCommand 命令码（标识配置类型）
 * @param [out] lpOutBuffer 输出缓冲区，存放配置数据
 * @return NET_TV_E_SUCCEED 成功，其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetDevConfig)(INT32 dwChannelID,
                                                         INT32 dwCommand,
                                                         LPVOID lpOutBuffer);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 通用配置设置回调类型（按命令码分发）
 * @param [in] dwChannelID 通道号
 * @param [in] dwCommand 命令码（标识配置类型）
 * @param [in] lpInBuffer 输入缓冲区，包含要设置的配置数据
 * @return NET_TV_E_SUCCEED 成功，其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_SetDevConfig)(INT32 dwChannelID,
                                                         INT32 dwCommand,
                                                         LPVOID lpInBuffer);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 按命令码注册的配置回调类型（专用回调）
 * @note 回调参数由命令码对应结构体决定，比通用回调更具体
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetDevConfigByCommand)(INT32 dwChannelID, LPVOID lpOutBuffer);
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_SetDevConfigByCommand)(INT32 dwChannelID, LPVOID lpInBuffer);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 获取RTSP流地址回调类型 (NET_TV_GET_RTSPURLCFG)
 * @param [in]  dwChannelID  通道号
 * @param [out] pInfo        RTSP URL 信息结构体指针
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetRtspUrl)(INT32 dwChannelID, pNET_RtspUrlInfo_S pInfo);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 获取回放播放地址回调类型
 * @param [in,out] pInfo 回放查询条件和播放URL返回信息
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetReplayUrl)(pNET_ReplayUrlInfo_S pInfo);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 回放控制回调类型
 * @param [in,out] pInfo 回放控制输入输出参数
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_ControlReplay)(pNET_ReplayCtrlInfo_S pInfo);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 获取回放录像时间段回调类型
 * @param [in,out] pInfo 查询条件及结果
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetReplayRecordList)(pNET_ReplayRecordList_S pInfo);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册通用配置获取回调（所有命令码统一处理）
 * @param [in] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 * @note 当没有按命令码注册的专用回调时，会调用此通用回调
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetDevConfig(NET_TV_CB_GetDevConfig pCb);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册通用配置设置回调（所有命令码统一处理）
 * @param [in] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 * @note 当没有按命令码注册的专用回调时，会调用此通用回调
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetDevConfig(NET_TV_CB_SetDevConfig pCb);

/************************************************************************/
/*                          按命令码注册的配置回调接口                     */
/************************************************************************/
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册设备基本信息获取回调 (NET_TV_GET_DEVICECFG)
 * @param [in] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetDeviceCfg(NET_TV_CB_GetDevConfigByCommand pCb);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册设备基本信息设置回调 (NET_TV_SET_DEVICECFG)
 * @param [in] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetDeviceCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetNtpCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetNtpCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetNtpCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetNtpCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetStreamCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetStreamCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetStreamCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetStreamCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetRtspUrl 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetRtspUrl(NET_TV_CB_GetRtspUrl pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetReplayUrl 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetReplayUrl(NET_TV_CB_GetReplayUrl pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_ControlReplay 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_ControlReplay(NET_TV_CB_ControlReplay pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetReplayRecordList 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetReplayRecordList(NET_TV_CB_GetReplayRecordList pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetOsdCapCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetOsdCapCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetOsdCapCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetOsdCapCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetImageCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetImageCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetImageCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetImageCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetNetworkCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetNetworkCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetNetworkCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetNetworkCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetConfigWifiSta 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetConfigWifiSta(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_ConnectWifiSta 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_ConnectWifiSta(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_DisconnectWifiSta 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_DisconnectWifiSta(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_Get4GInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_Get4GInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_Set4GInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_Set4GInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetHotspotInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetHotspotInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetHotspotConn 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetHotspotConn(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetSecurityServicesInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetSecurityServicesInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetSecurityServicesInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetSecurityServicesInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetSshCountdown 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetSshCountdown(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_FindLog 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_FindLog(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_ExportLog 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_ExportLog(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetLogServer 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetLogServer(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetLogServer 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetLogServer(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_TestLogServer 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_TestLogServer(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_ControlRecordInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_ControlRecordInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetRecordStatus 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetRecordStatus(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetRecordSchedule 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetRecordSchedule(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetRecordSchedule 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetRecordSchedule(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetRecordAdvancedParam 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetRecordAdvancedParam(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetRecordAdvancedParam 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetRecordAdvancedParam(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_FindRecordFileInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_FindRecordFileInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_DownloadRecordFile 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_DownloadRecordFile(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetPrivacyMaskCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetPrivacyMaskCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetPrivacyMaskCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetPrivacyMaskCfg(NET_TV_CB_SetDevConfigByCommand pCb);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetTamperAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetTamperAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetTamperAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetTamperAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetMotionAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetMotionAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetMotionAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetMotionAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetCrossLineAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetCrossLineAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetCrossLineAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetCrossLineAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetIntrusionAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetIntrusionAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetIntrusionAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetIntrusionAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetLoiteringAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetLoiteringAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetLoiteringAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetLoiteringAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetAudioAnomalyAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetAudioAnomalyAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetAudioAnomalyAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetAudioAnomalyAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetPreviewInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetPreviewInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetPreviewInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetPreviewInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetChannelInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetChannelInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetChannelList 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetChannelList(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetCrowGatheringAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetCrowGatheringAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetCrowGatheringAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetCrowGatheringAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetParkingAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetParkingAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetParkingAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetParkingAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetUnattendedObjectAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetUnattendedObjectAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetUnattendedObjectAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetUnattendedObjectAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetObjectRemovalAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetObjectRemovalAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetObjectRemovalAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetObjectRemovalAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetSceneChangeAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetSceneChangeAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetSceneChangeAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetSceneChangeAlarm(NET_TV_CB_SetDevConfigByCommand pCb);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetGarbageExposureCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetGarbageExposureCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetGarbageExposureCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetGarbageExposureCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetGarbageOverflowCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetGarbageOverflowCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetGarbageOverflowCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetGarbageOverflowCfg(NET_TV_CB_SetDevConfigByCommand pCb);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetTalkbackState 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetTalkbackState(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetTalkbackToStream 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetTalkbackToStream(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetTalkbackFromStream 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetTalkbackFromStream(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetReplayTalkback 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetReplayTalkback(NET_TV_CB_SetDevConfigByCommand pCb);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetUpgradeStatus 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetUpgradeStatus(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetUpgrade 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetUpgrade(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetUpgradeVersion 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetUpgradeVersion(NET_TV_CB_GetDevConfigByCommand pCb);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetCapturePlanInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetCapturePlanInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetCapturePlanInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetCapturePlanInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetCaptureParamInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetCaptureParamInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetCaptureParamInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetCaptureParamInfo(NET_TV_CB_SetDevConfigByCommand pCb);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetExposureInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetExposureInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetExposureInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetExposureInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetDayNightInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetDayNightInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetDayNightInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetDayNightInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetBackLightInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetBackLightInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetBackLightInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetBackLightInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetDenoiseInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetDenoiseInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetDenoiseInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetDenoiseInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetWhiteBalanceInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetWhiteBalanceInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetWhiteBalanceInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetWhiteBalanceInfo(NET_TV_CB_SetDevConfigByCommand pCb);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetAudioCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetAudioCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetAudioCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetAudioCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetEnterRegionAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetEnterRegionAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetEnterRegionAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetEnterRegionAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetLeaveRegionAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetLeaveRegionAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetLeaveRegionAlarm 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetLeaveRegionAlarm(NET_TV_CB_SetDevConfigByCommand pCb);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetFaceCaptureInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetFaceCaptureInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetFaceCaptureInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetFaceCaptureInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetFaceCompareInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetFaceCompareInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_AddTargetLib 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_AddTargetLib(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_DelTargetLib 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_DelTargetLib(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetTargetLib 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetTargetLib(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetTargetLib 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetTargetLib(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_AddFaceInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_AddFaceInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_DelFaceInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_DelFaceInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetFaceInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetFaceInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetFaceInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetFaceInfo(NET_TV_CB_GetDevConfigByCommand pCb);


/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetPeopleFlowStatisticsCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetPeopleFlowStatisticsCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetPeopleFlowStatisticsCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetPeopleFlowStatisticsCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_ResetPeopleFlowStatistics 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_ResetPeopleFlowStatistics(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetPeopleDensityDetectionCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetPeopleDensityDetectionCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetPeopleDensityDetectionCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetPeopleDensityDetectionCfg(NET_TV_CB_SetDevConfigByCommand pCb);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetManholeCoverAbnormalCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetManholeCoverAbnormalCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetManholeCoverAbnormalCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetManholeCoverAbnormalCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetSleepOnDutyCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetSleepOnDutyCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetSleepOnDutyCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetSleepOnDutyCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetElectricVehicleInElevatorCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetElectricVehicleInElevatorCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetElectricVehicleInElevatorCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetElectricVehicleInElevatorCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetPersonFallDownCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetPersonFallDownCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetPersonFallDownCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetPersonFallDownCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetConstructionOccupyRoadCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetConstructionOccupyRoadCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetConstructionOccupyRoadCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetConstructionOccupyRoadCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetCongestionCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetCongestionCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetCongestionCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetCongestionCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetLicensePlateRecognitionCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetLicensePlateRecognitionCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetLicensePlateRecognitionCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetLicensePlateRecognitionCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetHighAltitudeSeatbeltCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetHighAltitudeSeatbeltCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetHighAltitudeSeatbeltCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetHighAltitudeSeatbeltCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetSafetyHelmetCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetSafetyHelmetCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetSafetyHelmetCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetSafetyHelmetCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetPersonFallCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetPersonFallCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetPersonFallCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetPersonFallCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetPhoneUsageCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetPhoneUsageCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetPhoneUsageCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetPhoneUsageCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetSmokingCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetSmokingCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetSmokingCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetSmokingCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetOpenFlameCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetOpenFlameCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetOpenFlameCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetOpenFlameCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetBareSoilCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetBareSoilCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetBareSoilCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetBareSoilCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetHoleProtectionBarCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetHoleProtectionBarCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetHoleProtectionBarCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetHoleProtectionBarCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetReflectiveClothingCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetReflectiveClothingCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetReflectiveClothingCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetReflectiveClothingCfg(NET_TV_CB_SetDevConfigByCommand pCb);

/* 智能事件配置回调注册接口 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetPetRecognitionInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetPetRecognitionInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetPetRecognitionInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetClimbFenceInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetClimbFenceInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetClimbFenceInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetClimbFenceInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetDimissionInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetDimissionInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetDimissionInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetDimissionInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetIllegalLaneInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetIllegalLaneInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetIllegalLaneInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetIllegalLaneInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetRetrogradeInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetRetrogradeInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetRetrogradeInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetRetrogradeInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetNonmotorVehicleIntrusionInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetNonmotorVehicleIntrusionInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetNonmotorVehicleIntrusionInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetNonmotorVehicleIntrusionInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetOccupationEmergencyInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetOccupationEmergencyInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetOccupationEmergencyInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetOccupationEmergencyInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetPedestrianIntrusionInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetPedestrianIntrusionInfo(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetPedestrianIntrusionInfo 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetPedestrianIntrusionInfo(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetSmokeFireCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetSmokeFireCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetSmokeFireCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetSmokeFireCfg(NET_TV_CB_SetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_GetRoadPondingCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetRoadPondingCfg(NET_TV_CB_GetDevConfigByCommand pCb);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_SetRoadPondingCfg 对应的服务端回调。
 * @param [in] pCb 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetRoadPondingCfg(NET_TV_CB_SetDevConfigByCommand pCb);

/************************************************************************/
/*                    设备发现 Device Discovery                           */
/************************************************************************/
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 获取设备发现信息的回调
 * @param [out] pDeviceInfo 由宿主应用填充设备信息
 */
typedef void(NET_TV_STDCALL *NET_TV_CB_GetDiscoveryDeviceInfo)(
    NET_TV_OUT NET_DiscoveryDeviceInfo_S* pDeviceInfo);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册设备发现信息回调（启动前必须调用）
 * @param [in] cbFunc 回调函数指针
 * @return NET_TV_TRUE 成功，NET_TV_FALSE 失败
 */
NET_TV_API BOOL NET_TV_STDCALL
NET_TV_SERVER_RegisterCb_GetDiscoveryDeviceInfo(
    NET_TV_IN NET_TV_CB_GetDiscoveryDeviceInfo cbFunc);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 启动设备发现响应服务（阻塞线程中运行 AF_PACKET 接收循环）
 * @param [in] szInterfaceName 网卡名称 (如 "eth0")
 * @return NET_TV_TRUE 成功，NET_TV_FALSE 失败
 * @note 需先调用 NET_TV_SERVER_RegisterCb_GetDiscoveryDeviceInfo 注册回调
 */
NET_TV_API BOOL NET_TV_STDCALL
NET_TV_SERVER_Discovery_Start(NET_TV_IN const CHAR* szInterfaceName);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 停止设备发现响应服务
 * @return NET_TV_TRUE 成功，NET_TV_FALSE 失败
 */
NET_TV_API BOOL NET_TV_STDCALL
NET_TV_SERVER_Discovery_Stop(void);

/************************************************************************/
/*                       语音对讲 VoiceCom (服务端)                       */
/************************************************************************/
/** @brief 语音对讲播放回调: 收到NVR端音频时调用, 推送到扬声器 */
typedef void (NET_TV_STDCALL *NET_TV_SERVER_VoiceComPlayCallBack)(const char* data, unsigned int size);
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 语音对讲采集回调: SDK按协商参数主动拉取设备侧采集帧并发送到NVR
 * @param [in]  pstAudioParam 当前 VoiceCom 会话协商的音频参数
 * @param [out] pBuffer       输出音频帧缓存
 * @param [in]  dwBufferSize  输出缓存长度
 * @param [in]  lpUserData    用户数据
 * @return 实际写入的音频字节数，返回 <=0 表示当前无可用音频帧
 * @note 回调内应写入与 pstAudioParam 匹配的裸音频帧；建议每次返回 dwFrameBytes 字节。
 */
typedef INT32 (NET_TV_STDCALL *NET_TV_SERVER_VoiceComCaptureCallBack)(
    NET_TV_IN const NET_VoiceComAudioParam_S* pstAudioParam,
    NET_TV_OUT CHAR* pBuffer,
    NET_TV_IN UINT32 dwBufferSize,
    NET_TV_IN LPVOID lpUserData);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 启动语音对讲TCP监听
 * @param [in]  dwPort  监听端口, 默认9006
 * @return NET_TV_TRUE 成功，NET_TV_FALSE 失败
 */
NET_TV_API BOOL NET_TV_STDCALL
NET_TV_SERVER_StartVoiceComServer(NET_TV_IN UINT32 dwPort);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 停止语音对讲TCP监听
 * @return NET_TV_TRUE 成功，NET_TV_FALSE 失败
 */
NET_TV_API BOOL NET_TV_STDCALL
NET_TV_SERVER_StopVoiceComServer(void);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册播放回调 (收到NVR音频 → 扬声器)
 * @param [in]  cb  播放回调
 * @return NET_TV_TRUE 成功，NET_TV_FALSE 失败
 */
NET_TV_API BOOL NET_TV_STDCALL
NET_TV_SERVER_RegisterCb_VoiceComPlay(NET_TV_IN NET_TV_SERVER_VoiceComPlayCallBack cb);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册采集回调 (麦克风/LineIn -> NVR)
 * @param [in]  cb          采集回调，传 NULL 表示注销
 * @param [in]  lpUserData  用户数据，回调时原样透传
 * @return NET_TV_TRUE 成功，NET_TV_FALSE 失败
 * @note SDK负责按当前 VoiceCom 会话参数定时拉帧并发送，业务侧只需要提供采集帧。
 */
NET_TV_API BOOL NET_TV_STDCALL
NET_TV_SERVER_RegisterCb_VoiceComCapture(NET_TV_IN NET_TV_SERVER_VoiceComCaptureCallBack cb,
                                         NET_TV_IN LPVOID lpUserData);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 发送麦克风采集的音频到NVR
 * @param [in]  pData  音频帧数据，格式需与当前 VoiceCom 会话协商参数一致
 * @param [in]  dwSize 数据长度(字节)
 * @return NET_TV_TRUE 成功，NET_TV_FALSE 失败
 */
NET_TV_API BOOL NET_TV_STDCALL
NET_TV_SERVER_SendVoiceComData(NET_TV_IN const CHAR* pData, NET_TV_IN UINT32 dwSize);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 获取当前 VoiceCom 会话协商的音频参数
 * @param [out] pstAudioParam  音频参数
 * @return NET_TV_TRUE 成功，NET_TV_FALSE 表示尚未建立会话或参数未协商
 */
NET_TV_API BOOL NET_TV_STDCALL
NET_TV_SERVER_GetVoiceComAudioParam(NET_TV_OUT pNET_VoiceComAudioParam_S pstAudioParam);

/************************************************************************/
/*                       录像帧流 RecordFrame (服务端)                    */
/************************************************************************/
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 录像帧流开始回调: 收到客户端起止时间查询后调用, 由宿主打开录像源并填充流信息
 */
typedef NET_TV_COMMON_ECODE_E (NET_TV_STDCALL *NET_TV_SERVER_RecordFrameStartCallBack)(
    NET_TV_IN pNET_RecordFrameStreamCond_S pstCond,
    NET_TV_INOUT pNET_RecordFrameStreamInfo_S pstInfo,
    NET_TV_IN LPVOID lpUserData);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 录像帧读取回调: SDK在TCP连接建立后循环拉取帧并发送给客户端
 * @return 实际写入 pBuffer 的负载字节数；0 表示暂时无帧；<0 表示结束/失败
 */
typedef INT32 (NET_TV_STDCALL *NET_TV_SERVER_RecordFrameReadCallBack)(
    NET_TV_IN const CHAR* szStreamId,
    NET_TV_OUT pNET_RecordFrameInfo_S pstFrameInfo,
    NET_TV_OUT CHAR* pBuffer,
    NET_TV_IN UINT32 dwBufferSize,
    NET_TV_IN LPVOID lpUserData);

/** @brief 录像帧流停止回调: 客户端停止或流结束时调用 */
typedef NET_TV_COMMON_ECODE_E (NET_TV_STDCALL *NET_TV_SERVER_RecordFrameStopCallBack)(
    NET_TV_IN const CHAR* szStreamId,
    NET_TV_IN LPVOID lpUserData);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 启动 NET_TV_SERVER_StartRecordFrameServer 对应的服务能力。
 * @param [in] dwPort 服务端操作参数。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL
NET_TV_SERVER_StartRecordFrameServer(NET_TV_IN UINT32 dwPort);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 停止 NET_TV_SERVER_StopRecordFrameServer 对应的服务能力。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL
NET_TV_SERVER_StopRecordFrameServer(void);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_RecordFrameStart 对应的服务端回调。
 * @param [in] cb 服务端回调函数指针。
 * @param [in] lpUserData 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL
NET_TV_SERVER_RegisterCb_RecordFrameStart(NET_TV_IN NET_TV_SERVER_RecordFrameStartCallBack cb,
                                         NET_TV_IN LPVOID lpUserData);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_RecordFrameRead 对应的服务端回调。
 * @param [in] cb 服务端回调函数指针。
 * @param [in] lpUserData 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL
NET_TV_SERVER_RegisterCb_RecordFrameRead(NET_TV_IN NET_TV_SERVER_RecordFrameReadCallBack cb,
                                        NET_TV_IN LPVOID lpUserData);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册 NET_TV_SERVER_RegisterCb_RecordFrameStop 对应的服务端回调。
 * @param [in] cb 服务端回调函数指针。
 * @param [in] lpUserData 服务端回调函数指针。
 * @return NET_TV_TRUE 表示成功；NET_TV_FALSE 表示失败。
 */
NET_TV_API BOOL NET_TV_STDCALL
NET_TV_SERVER_RegisterCb_RecordFrameStop(NET_TV_IN NET_TV_SERVER_RecordFrameStopCallBack cb,
                                        NET_TV_IN LPVOID lpUserData);

#ifdef __cplusplus
}
#endif

#endif

#endif
