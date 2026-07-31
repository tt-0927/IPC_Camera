

/**
 * @file NetTVSDKServerInterface.h
 * @brief SDK服务端接口头文件，定义服务端初始化、配置回调注册、设备发现、语音对讲、录像帧流等核心接口
 * @note 服务端接口采用C风格API，供宿主程序（如NVR、IPC）调用，用于注册回调和推送消息
 */
#ifndef _NET_SDKSERVER_INTERFACE_H
#define _NET_SDKSERVER_INTERFACE_H



#include "NetTVSDKCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************/
/*                          SDK服务端核心接口                           */
/************************************************************************/
/**
 * @brief SDK服务端初始化
 * @param [IN] dwPort 服务器端口号
 * @param [IN] szUserName 用户名
 * @param [IN] szPassword 密码
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 * @note
 */
NET_API BOOL STDCALL NET_SERVER_Init(IN UINT32 udwPort,IN CHAR szUserName[NET_LEN_132],IN CHAR szPassword[NET_LEN_132]);

/**
* SDK 清理  SDK cleaning
* @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
* @note
*/
NET_API BOOL STDCALL NET_SERVER_Cleanup(void);

/**
 * @brief 设置日志
 * @param [IN] dwLogLevel   日志的等级（默认为0）：0-表示关闭日志，1-表示只输出ERROR错误日志，2-输出ERROR错误信息和DEBUG调试信息，3-输出ERROR错误信息、DEBUG调试信息和INFO普通信息等所有信息
 * @param [IN] strLogDir    日志路径
 * @param [IN] dwLogFileSize 日志文件大小(单位：字节)
 * @param [IN] dwLogFileNum 日志文件个数
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_API BOOL STDCALL NET_SERVER_SetLogToFile(IN INT32 dwLogLevel,IN CHAR  *strLogDir,IN INT32 dwLogFileSize,IN INT32 dwLogFileNum);

/**
* 获取SDK的版本信息 Get SDK version information
* @return SDK版本信息 SDK version information
* @note
* - 在两个高字节中高8位表示主版本,低八位表示次版本.两个低字节表示附加版本号如0x01080000：表示版本为1.8.0.0.
* - The two high bytes,The high-8-bit indicate the major version, and the low-8-bytes indicate the minor version.Two low bytes for additional version numbers For example, 0x01080000 means version 1.8.0.0
*/
NET_API INT32 STDCALL NET_SERVER_GetSDKVersion(void);

/**
 * @brief 获取当前在线客户端数量（活跃会话数）
 * @return 客户端数量
 */
NET_API INT32 STDCALL NET_SERVER_GetClientCount(void);

/**
 * @brief 设置用户名密码
 * @param [IN] szUserName 用户名
 * @param [IN] szPassword 密码
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_API BOOL STDCALL NET_SERVER_SetUserPasswd(IN CHAR szUserName[NET_LEN_132],IN CHAR szPassword[NET_LEN_132]);

/**
 * @brief 推送告警信息
 * @param [IN] pAlarmer    告警设备信息
 * @param [IN] lCommand    命令码(报警类型)，用于客户端按命令码反序列化结构体
 * @param [IN] pAlarmInfo  具体告警结构体指针（类型由 lCommand 决定）
 * @param [IN] dwBufLen    pAlarmInfo 长度（一般为 sizeof(对应结构体)）
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_API BOOL STDCALL NET_SERVER_PushAlarmInfo(IN NET_Alarmer_S *pAlarmer,
                                                    IN INT32 lCommand,
                                                    IN LPVOID pAlarmInfo,
                                                    IN INT32 dwBufLen);

/**
 * @brief 推送通道上下线状态
 * @param [IN] pChannelInfo 通道信息，byOnline/nDevState 表示当前状态
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_API BOOL STDCALL NET_SERVER_PushChannelStatusInfo(IN NET_ChannelInfo_S *pChannelInfo);

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetDeviceInfo(NET_COMMON_ECODE_E (*CB)(pNET_DeviceInfo_S pInfo));

/**
 * @brief 设备控制回调类型
 * @param [IN] pstCtrlInfo 设备硬件控制参数，参见 NET_DeviceControlInfo_S
 * @return NET_E_SUCCEED 成功，其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_DeviceControl)(pNET_DeviceControlInfo_S pstCtrlInfo);

/**
 * @brief 注册设备控制回调
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_DeviceControl(NET_CB_DeviceControl pCb);

/**
 * @brief 修改用户密码回调类型
 * @param [IN] pPasswordInfo 修改密码参数，包含用户名、旧密码、新密码
 * @return NET_E_SUCCEED 成功，其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_SetUserPassword)(pNET_UserPasswordInfo_S pPasswordInfo);

/**
 * @brief 注册修改用户密码回调
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetUserPassword(NET_CB_SetUserPassword pCb);

/**
 * @brief 视频编码能力集回调类型 (NET_CAP_VIDEO_ENCODE)
 * @param [IN]  dwChannelID  通道号
 * @param [OUT] pCap         视频编码能力集结构体指针(多码流)
 * @return NET_E_SUCCEED 成功, 其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetVideoEncodeCap)(INT32 dwChannelID,
                                                             pNET_VideoEncodeCap_S pCap);

/**
 * @brief 注册视频编码能力集回调
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetVideoEncodeCap(NET_CB_GetVideoEncodeCap pCb);

/**
 * @brief 音频编码能力集回调类型 (NET_CAP_AUDIO)
 * @param [IN]  dwChannelID  通道号
 * @param [OUT] pCap         音频编码能力集结构体指针
 * @return NET_E_SUCCEED 成功, 其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetAudioEncodeCap)(INT32 dwChannelID,
                                                             pNET_AudioCap_S pCap);

/**
 * @brief 注册音频编码能力集回调
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetAudioEncodeCap(NET_CB_GetAudioEncodeCap pCb);

/**
 * @brief OSD能力集回调类型 (NET_CAP_OSD)
 * @param [IN]  dwChannelID  通道号
 * @param [OUT] pCap         OSD能力集结构体指针
 * @return NET_E_SUCCEED 成功, 其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetOsdCap)(INT32 dwChannelID, pNET_OsdCap_S pCap);

/**
 * @brief 注册OSD能力集回调
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetOsdCap(NET_CB_GetOsdCap pCb);

/**
 * @brief 通用配置回调类型（按命令码分发）
 * @param [IN] dwChannelID 通道号
 * @param [IN] dwCommand 命令码（标识配置类型）
 * @param [OUT] lpOutBuffer 输出缓冲区，存放配置数据
 * @return NET_E_SUCCEED 成功，其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetDevConfig)(INT32 dwChannelID,
                                                         INT32 dwCommand,
                                                         LPVOID lpOutBuffer);

/**
 * @brief 通用配置设置回调类型（按命令码分发）
 * @param [IN] dwChannelID 通道号
 * @param [IN] dwCommand 命令码（标识配置类型）
 * @param [IN] lpInBuffer 输入缓冲区，包含要设置的配置数据
 * @return NET_E_SUCCEED 成功，其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_SetDevConfig)(INT32 dwChannelID,
                                                         INT32 dwCommand,
                                                         LPVOID lpInBuffer);

/**
 * @brief 按命令码注册的配置回调类型（专用回调）
 * @note 回调参数由命令码对应结构体决定，比通用回调更具体
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetDevConfigByCommand)(INT32 dwChannelID, LPVOID lpOutBuffer);
typedef NET_COMMON_ECODE_E (*NET_CB_SetDevConfigByCommand)(INT32 dwChannelID, LPVOID lpInBuffer);

/**
 * @brief 获取RTSP流地址回调类型 (NET_GET_RTSPURLCFG)
 * @param [IN]  dwChannelID  通道号
 * @param [OUT] pInfo        RTSP URL 信息结构体指针
 * @return NET_E_SUCCEED 成功, 其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetRtspUrl)(INT32 dwChannelID, pNET_RtspUrlInfo_S pInfo);

/**
 * @brief 获取回放播放地址回调类型
 * @param [INOUT] pInfo 回放查询条件和播放URL返回信息
 * @return NET_E_SUCCEED 成功, 其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetReplayUrl)(pNET_ReplayUrlInfo_S pInfo);

/**
 * @brief 回放控制回调类型
 * @param [INOUT] pInfo 回放控制输入输出参数
 * @return NET_E_SUCCEED 成功, 其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_ControlReplay)(pNET_ReplayCtrlInfo_S pInfo);

/**
 * @brief 获取回放录像时间段回调类型
 * @param [INOUT] pInfo 查询条件及结果
 * @return NET_E_SUCCEED 成功, 其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetReplayRecordList)(pNET_ReplayRecordList_S pInfo);

/**
 * @brief 注册通用配置获取回调（所有命令码统一处理）
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 * @note 当没有按命令码注册的专用回调时，会调用此通用回调
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetDevConfig(NET_CB_GetDevConfig pCb);

/**
 * @brief 注册通用配置设置回调（所有命令码统一处理）
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 * @note 当没有按命令码注册的专用回调时，会调用此通用回调
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetDevConfig(NET_CB_SetDevConfig pCb);

/************************************************************************/
/*                          按命令码注册的配置回调接口                     */
/************************************************************************/
/**
 * @brief 注册设备基本信息获取回调 (NET_GET_DEVICECFG)
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetDeviceCfg(NET_CB_GetDevConfigByCommand pCb);

/**
 * @brief 注册设备基本信息设置回调 (NET_SET_DEVICECFG)
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetDeviceCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetNtpCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetNtpCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetStreamCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetStreamCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetRtspUrl(NET_CB_GetRtspUrl pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetReplayUrl(NET_CB_GetReplayUrl pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_ControlReplay(NET_CB_ControlReplay pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetReplayRecordList(NET_CB_GetReplayRecordList pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetOsdCapCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetOsdCapCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetImageCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetImageCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetNetworkCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetNetworkCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetConfigWifiSta(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_ConnectWifiSta(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_DisconnectWifiSta(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_Get4GInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_Set4GInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetHotspotInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetHotspotConn(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetSecurityServicesInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetSecurityServicesInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetSshCountdown(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_FindLog(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_ExportLog(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetLogServer(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetLogServer(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_TestLogServer(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_ControlRecordInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetRecordStatus(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetRecordSchedule(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetRecordSchedule(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetRecordAdvancedParam(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetRecordAdvancedParam(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_FindRecordFileInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_DownloadRecordFile(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPrivacyMaskCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPrivacyMaskCfg(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetTamperAlarm(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetTamperAlarm(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetMotionAlarm(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetMotionAlarm(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetCrossLineAlarm(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetCrossLineAlarm(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetIntrusionAlarm(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetIntrusionAlarm(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetLoiteringAlarm(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetLoiteringAlarm(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetAudioAnomalyAlarm(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetAudioAnomalyAlarm(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPreviewInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPreviewInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetChannelInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetChannelList(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetCrowGatheringAlarm(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetCrowGatheringAlarm(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetParkingAlarm(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetParkingAlarm(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetUnattendedObjectAlarm(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetUnattendedObjectAlarm(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetObjectRemovalAlarm(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetObjectRemovalAlarm(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetSceneChangeAlarm(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetSceneChangeAlarm(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetGarbageExposureCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetGarbageExposureCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetGarbageOverflowCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetGarbageOverflowCfg(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetTalkbackState(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetTalkbackToStream(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetTalkbackFromStream(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetReplayTalkback(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetUpgradeStatus(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetUpgrade(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetUpgradeVersion(NET_CB_GetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetCapturePlanInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetCapturePlanInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetCaptureParamInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetCaptureParamInfo(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetExposureInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetExposureInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetDayNightInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetDayNightInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetBackLightInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetBackLightInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetDenoiseInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetDenoiseInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetWhiteBalanceInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetWhiteBalanceInfo(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetAudioCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetAudioCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetEnterRegionAlarm(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetEnterRegionAlarm(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetLeaveRegionAlarm(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetLeaveRegionAlarm(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetFaceCaptureInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetFaceCaptureInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetFaceCompareInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_AddTargetLib(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_DelTargetLib(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetTargetLib(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetTargetLib(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_AddFaceInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_DelFaceInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetFaceInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetFaceInfo(NET_CB_GetDevConfigByCommand pCb);


NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPeopleFlowStatisticsCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPeopleFlowStatisticsCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_ResetPeopleFlowStatistics(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPeopleDensityDetectionCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPeopleDensityDetectionCfg(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetManholeCoverAbnormalCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetManholeCoverAbnormalCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetSleepOnDutyCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetSleepOnDutyCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetElectricVehicleInElevatorCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetElectricVehicleInElevatorCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPersonFallDownCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPersonFallDownCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetConstructionOccupyRoadCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetConstructionOccupyRoadCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetCongestionCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetCongestionCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetLicensePlateRecognitionCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetLicensePlateRecognitionCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetHighAltitudeSeatbeltCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetHighAltitudeSeatbeltCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetSafetyHelmetCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetSafetyHelmetCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPersonFallCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPersonFallCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPhoneUsageCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPhoneUsageCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetSmokingCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetSmokingCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetOpenFlameCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetOpenFlameCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetBareSoilCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetBareSoilCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetHoleProtectionBarCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetHoleProtectionBarCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetReflectiveClothingCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetReflectiveClothingCfg(NET_CB_SetDevConfigByCommand pCb);

/* 智能事件配置回调注册接口 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPetRecognitionInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPetRecognitionInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetClimbFenceInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetClimbFenceInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetDimissionInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetDimissionInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetIllegalLaneInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetIllegalLaneInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetRetrogradeInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetRetrogradeInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetNonmotorVehicleIntrusionInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetNonmotorVehicleIntrusionInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetOccupationEmergencyInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetOccupationEmergencyInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPedestrianIntrusionInfo(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPedestrianIntrusionInfo(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetSmokeFireCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetSmokeFireCfg(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetRoadPondingCfg(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetRoadPondingCfg(NET_CB_SetDevConfigByCommand pCb);

/************************************************************************/
/*                    设备发现 Device Discovery                           */
/************************************************************************/
/**
 * @brief 获取设备发现信息的回调
 * @param [OUT] pDeviceInfo 由宿主应用填充设备信息
 */
typedef void(STDCALL *NET_CB_GetDiscoveryDeviceInfo)(
    OUT NET_DiscoveryDeviceInfo_S* pDeviceInfo);

/**
 * @brief 注册设备发现信息回调（启动前必须调用）
 * @param [IN] cbFunc 回调函数指针
 * @return TRUE 成功，FALSE 失败
 */
NET_API BOOL STDCALL
NET_SERVER_RegisterCb_GetDiscoveryDeviceInfo(
    IN NET_CB_GetDiscoveryDeviceInfo cbFunc);

/**
 * @brief 启动设备发现响应服务（阻塞线程中运行 AF_PACKET 接收循环）
 * @param [IN] szInterfaceName 网卡名称 (如 "eth0")
 * @return TRUE 成功，FALSE 失败
 * @note 需先调用 NET_SERVER_RegisterCb_GetDiscoveryDeviceInfo 注册回调
 */
NET_API BOOL STDCALL
NET_SERVER_Discovery_Start(IN const CHAR* szInterfaceName);

/**
 * @brief 停止设备发现响应服务
 * @return TRUE 成功，FALSE 失败
 */
NET_API BOOL STDCALL
NET_SERVER_Discovery_Stop(void);

/************************************************************************/
/*                       语音对讲 VoiceCom (服务端)                       */
/************************************************************************/
/** @brief 语音对讲播放回调: 收到NVR端音频时调用, 推送到扬声器 */
typedef void (STDCALL *NET_SERVER_VoiceComPlayCallBack)(const char* data, unsigned int size);
/**
 * @brief 语音对讲采集回调: SDK按协商参数主动拉取设备侧采集帧并发送到NVR
 * @param [IN]  pstAudioParam 当前 VoiceCom 会话协商的音频参数
 * @param [OUT] pBuffer       输出音频帧缓存
 * @param [IN]  dwBufferSize  输出缓存长度
 * @param [IN]  lpUserData    用户数据
 * @return 实际写入的音频字节数，返回 <=0 表示当前无可用音频帧
 * @note 回调内应写入与 pstAudioParam 匹配的裸音频帧；建议每次返回 dwFrameBytes 字节。
 */
typedef INT32 (STDCALL *NET_SERVER_VoiceComCaptureCallBack)(
    IN const NET_VoiceComAudioParam_S* pstAudioParam,
    OUT CHAR* pBuffer,
    IN UINT32 dwBufferSize,
    IN LPVOID lpUserData);

/**
 * @brief 启动语音对讲TCP监听
 * @param [IN]  dwPort  监听端口, 默认9006
 * @return TRUE 成功，FALSE 失败
 */
NET_API BOOL STDCALL
NET_SERVER_StartVoiceComServer(IN UINT32 dwPort);

/**
 * @brief 停止语音对讲TCP监听
 * @return TRUE 成功，FALSE 失败
 */
NET_API BOOL STDCALL
NET_SERVER_StopVoiceComServer(void);

/**
 * @brief 注册播放回调 (收到NVR音频 → 扬声器)
 * @param [IN]  cb  播放回调
 * @return TRUE 成功，FALSE 失败
 */
NET_API BOOL STDCALL
NET_SERVER_RegisterCb_VoiceComPlay(IN NET_SERVER_VoiceComPlayCallBack cb);

/**
 * @brief 注册采集回调 (麦克风/LineIn -> NVR)
 * @param [IN]  cb          采集回调，传 NULL 表示注销
 * @param [IN]  lpUserData  用户数据，回调时原样透传
 * @return TRUE 成功，FALSE 失败
 * @note SDK负责按当前 VoiceCom 会话参数定时拉帧并发送，业务侧只需要提供采集帧。
 */
NET_API BOOL STDCALL
NET_SERVER_RegisterCb_VoiceComCapture(IN NET_SERVER_VoiceComCaptureCallBack cb,
                                         IN LPVOID lpUserData);

/**
 * @brief 发送麦克风采集的音频到NVR
 * @param [IN]  pData  音频帧数据，格式需与当前 VoiceCom 会话协商参数一致
 * @param [IN]  dwSize 数据长度(字节)
 * @return TRUE 成功，FALSE 失败
 */
NET_API BOOL STDCALL
NET_SERVER_SendVoiceComData(IN const CHAR* pData, IN UINT32 dwSize);

/**
 * @brief 获取当前 VoiceCom 会话协商的音频参数
 * @param [OUT] pstAudioParam  音频参数
 * @return TRUE 成功，FALSE 表示尚未建立会话或参数未协商
 */
NET_API BOOL STDCALL
NET_SERVER_GetVoiceComAudioParam(OUT pNET_VoiceComAudioParam_S pstAudioParam);

/************************************************************************/
/*                       录像帧流 RecordFrame (服务端)                    */
/************************************************************************/
/**
 * @brief 录像帧流开始回调: 收到客户端起止时间查询后调用, 由宿主打开录像源并填充流信息
 */
typedef NET_COMMON_ECODE_E (STDCALL *NET_SERVER_RecordFrameStartCallBack)(
    IN pNET_RecordFrameStreamCond_S pstCond,
    INOUT pNET_RecordFrameStreamInfo_S pstInfo,
    IN LPVOID lpUserData);

/**
 * @brief 录像帧读取回调: SDK在TCP连接建立后循环拉取帧并发送给客户端
 * @return 实际写入 pBuffer 的负载字节数；0 表示暂时无帧；<0 表示结束/失败
 */
typedef INT32 (STDCALL *NET_SERVER_RecordFrameReadCallBack)(
    IN const CHAR* szStreamId,
    OUT pNET_RecordFrameInfo_S pstFrameInfo,
    OUT CHAR* pBuffer,
    IN UINT32 dwBufferSize,
    IN LPVOID lpUserData);

/** @brief 录像帧流停止回调: 客户端停止或流结束时调用 */
typedef NET_COMMON_ECODE_E (STDCALL *NET_SERVER_RecordFrameStopCallBack)(
    IN const CHAR* szStreamId,
    IN LPVOID lpUserData);

NET_API BOOL STDCALL
NET_SERVER_StartRecordFrameServer(IN UINT32 dwPort);

NET_API BOOL STDCALL
NET_SERVER_StopRecordFrameServer(void);

NET_API BOOL STDCALL
NET_SERVER_RegisterCb_RecordFrameStart(IN NET_SERVER_RecordFrameStartCallBack cb,
                                         IN LPVOID lpUserData);

NET_API BOOL STDCALL
NET_SERVER_RegisterCb_RecordFrameRead(IN NET_SERVER_RecordFrameReadCallBack cb,
                                        IN LPVOID lpUserData);

NET_API BOOL STDCALL
NET_SERVER_RegisterCb_RecordFrameStop(IN NET_SERVER_RecordFrameStopCallBack cb,
                                        IN LPVOID lpUserData);

#ifdef __cplusplus
}
#endif

#endif
