

#ifndef _NET_TV_SDKSERVER_INTERFACE_H
#define _NET_TV_SDKSERVER_INTERFACE_H

#ifdef NET_TV_SDK_SERVER_API
    #include "NetTVSDKServer.h"
#else

#include "NetTVSDKCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************/
/*                          函数                                  */
/************************************************************************/
/**
 * @brief SDK服务端初始化
 * @param [IN] dwPort 服务器端口号
 * @param [IN] szUserName 用户名
 * @param [IN] szPassword 密码
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 * @note
 */
NET_TV_API BOOL STDCALL NET_TV_SERVER_Init(IN UINT32 udwPort,IN CHAR szUserName[NET_TV_LEN_132],IN CHAR szPassword[NET_TV_LEN_132]);

/**
* SDK 清理  SDK cleaning
* @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
* @note
*/
NET_TV_API BOOL STDCALL NET_TV_SERVER_Cleanup(void);

/**
 * @brief 设置日志
 * @param [IN] dwLogLevel   日志的等级（默认为0）：0-表示关闭日志，1-表示只输出ERROR错误日志，2-输出ERROR错误信息和DEBUG调试信息，3-输出ERROR错误信息、DEBUG调试信息和INFO普通信息等所有信息 
 * @param [IN] strLogDir    日志路径
 * @param [IN] dwLogFileSize 日志文件大小(单位：字节)
 * @param [IN] dwLogFileNum 日志文件个数
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_TV_API BOOL STDCALL NET_TV_SERVER_SetLogToFile(IN INT32 dwLogLevel,IN CHAR  *strLogDir,IN INT32 dwLogFileSize,IN INT32 dwLogFileNum);

/**
* 获取SDK的版本信息 Get SDK version information 
* @return SDK版本信息 SDK version information
* @note
* - 在两个高字节中高8位表示主版本,低八位表示次版本.两个低字节表示附加版本号如0x01080000：表示版本为1.8.0.0.
* - The two high bytes,The high-8-bit indicate the major version, and the low-8-bytes indicate the minor version.Two low bytes for additional version numbers For example, 0x01080000 means version 1.8.0.0
*/
NET_TV_API INT32 STDCALL NET_TV_SERVER_GetSDKVersion(void);

/**
 * @brief 获取当前在线客户端数量（活跃会话数）
 * @return 客户端数量
 */
NET_TV_API INT32 STDCALL NET_TV_SERVER_GetClientCount(void);

/**
 * @brief 设置用户名密码
 * @param [IN] szUserName 用户名
 * @param [IN] szPassword 密码
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_TV_API BOOL STDCALL NET_TV_SERVER_SetUserPasswd(IN CHAR szUserName[NET_TV_LEN_132],IN CHAR szPassword[NET_TV_LEN_132]);

/**
 * @brief 推送告警信息
 * @param [IN] pAlarmer    告警设备信息
 * @param [IN] lCommand    命令码(报警类型)，用于客户端按命令码反序列化结构体
 * @param [IN] pAlarmInfo  具体告警结构体指针（类型由 lCommand 决定）
 * @param [IN] dwBufLen    pAlarmInfo 长度（一般为 sizeof(对应结构体)）
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_TV_API BOOL STDCALL NET_TV_SERVER_PushAlarmInfo(IN NET_TV_ALARMER_S *pAlarmer,
                                                    IN INT32 lCommand,
                                                    IN LPVOID pAlarmInfo,
                                                    IN INT32 dwBufLen);

/**
 * @brief 推送通道上下线状态
 * @param [IN] pChannelInfo 通道信息，byOnline/nDevState 表示当前状态
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_TV_API BOOL STDCALL NET_TV_SERVER_PushChannelStatusInfo(IN NET_TV_CHANNEL_INFO_S *pChannelInfo);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetDeviceInfo(NET_TV_COMMON_ECODE_E (*CB)(LPNET_TV_DEVICE_INFO_S pInfo));

/**
 * @brief 视频编码能力集回调类型 (NET_TV_CAP_VIDEO_ENCODE)
 * @param [IN]  dwChannelID  通道号
 * @param [OUT] pCap         视频编码能力集结构体指针(多码流)
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetVideoEncodeCap)(INT32 dwChannelID, 
                                                             LPNET_TV_VIDEO_ENCODE_CAP_S pCap);

/**
 * @brief 注册视频编码能力集回调
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetVideoEncodeCap(NET_TV_CB_GetVideoEncodeCap pCb);

/**
 * @brief 音频编码能力集回调类型 (NET_TV_CAP_AUDIO)
 * @param [IN]  dwChannelID  通道号
 * @param [OUT] pCap         音频编码能力集结构体指针
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetAudioEncodeCap)(INT32 dwChannelID, 
                                                             LPNET_TV_AUDIO_CAP_S pCap);

/**
 * @brief 注册音频编码能力集回调
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetAudioEncodeCap(NET_TV_CB_GetAudioEncodeCap pCb);

/**
 * @brief OSD能力集回调类型 (NET_TV_CAP_OSD)
 * @param [IN]  dwChannelID  通道号
 * @param [OUT] pCap         OSD能力集结构体指针
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetOsdCap)(INT32 dwChannelID, LPNET_TV_OSD_CAP_S pCap);

/**
 * @brief 注册OSD能力集回调
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetOsdCap(NET_TV_CB_GetOsdCap pCb);

typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetDevConfig)(INT32 dwChannelID,
                                                         INT32 dwCommand,
                                                         LPVOID lpOutBuffer);
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_SetDevConfig)(INT32 dwChannelID,
                                                         INT32 dwCommand,
                                                         LPVOID lpInBuffer);

/* 按命令码注册配置回调，回调参数由命令码对应结构体决定 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetDevConfigByCommand)(INT32 dwChannelID, LPVOID lpOutBuffer);
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_SetDevConfigByCommand)(INT32 dwChannelID, LPVOID lpInBuffer);

/**
 * @brief 获取RTSP流地址回调类型 (NET_TV_GET_RTSPURLCFG)
 * @param [IN]  dwChannelID  通道号
 * @param [OUT] pInfo        RTSP URL 信息结构体指针
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetRtspUrl)(INT32 dwChannelID, LPNET_TV_RTSP_URL_INFO_S pInfo);

/**
 * @brief 获取回放播放地址回调类型
 * @param [INOUT] pInfo 回放查询条件和播放URL返回信息
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetReplayUrl)(LPNET_TV_REPLAY_URL_INFO_S pInfo);

/**
 * @brief 回放控制回调类型
 * @param [INOUT] pInfo 回放控制输入输出参数
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_ControlReplay)(LPNET_TV_REPLAY_CTRL_INFO_S pInfo);

/**
 * @brief 获取回放录像时间段回调类型
 * @param [INOUT] pInfo 查询条件及结果
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetReplayRecordList)(LPNET_TV_REPLAY_RECORD_LIST_S pInfo);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetDevConfig(NET_TV_CB_GetDevConfig pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetDevConfig(NET_TV_CB_SetDevConfig pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetDeviceCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetDeviceCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetNtpCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetNtpCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetStreamCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetStreamCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetRtspUrl(NET_TV_CB_GetRtspUrl pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetReplayUrl(NET_TV_CB_GetReplayUrl pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_ControlReplay(NET_TV_CB_ControlReplay pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetReplayRecordList(NET_TV_CB_GetReplayRecordList pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetOsdCapCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetOsdCapCfg(NET_TV_CB_SetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetOsdCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetOsdCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetImageCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetImageCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetNetworkCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetNetworkCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetConfigWifiSta(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_ConnectWifiSta(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_DisconnectWifiSta(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_Get4GInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_Set4GInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetHotspotInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetHotspotConn(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSecurityServicesInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetSecurityServicesInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSshCountdown(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_FindLog(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_ExportLog(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetLogServer(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetLogServer(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_TestLogServer(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_ControlRecordInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetRecordStatus(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetRecordSchedule(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetRecordSchedule(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetRecordAdvancedParam(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetRecordAdvancedParam(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_FindRecordFileInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_DownloadRecordFile(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPrivacyMaskCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPrivacyMaskCfg(NET_TV_CB_SetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetTamperAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetTamperAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetMotionAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetMotionAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetCrossLineAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetCrossLineAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetIntrusionAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetIntrusionAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetLoiteringAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetLoiteringAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetAudioAnomalyAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetAudioAnomalyAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPreviewInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPreviewInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetChannelInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetChannelList(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetCrowGatheringAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetCrowGatheringAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetParkingAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetParkingAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetUnattendedObjectAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetUnattendedObjectAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetObjectRemovalAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetObjectRemovalAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSceneChangeAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetSceneChangeAlarm(NET_TV_CB_SetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetGarbageExposureCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetGarbageExposureCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetGarbageOverflowCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetGarbageOverflowCfg(NET_TV_CB_SetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetTalkbackState(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetTalkbackToStream(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetTalkbackFromStream(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetReplayTalkback(NET_TV_CB_SetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetUpgradeStatus(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetUpgrade(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetUpgradeVersion(NET_TV_CB_GetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetCapturePlanInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetCapturePlanInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetCaptureParamInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetCaptureParamInfo(NET_TV_CB_SetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetExposureInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetExposureInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetDayNightInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetDayNightInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetBackLightInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetBackLightInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetDenoiseInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetDenoiseInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetWhiteBalanceInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetWhiteBalanceInfo(NET_TV_CB_SetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetAudioCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetAudioCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetEnterRegionAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetEnterRegionAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetLeaveRegionAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetLeaveRegionAlarm(NET_TV_CB_SetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetFaceCaptureInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetFaceCaptureInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetFaceCompareInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_AddTargetLib(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_DelTargetLib(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetTargetLib(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetTargetLib(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_AddFaceInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_DelFaceInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetFaceInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetFaceInfo(NET_TV_CB_GetDevConfigByCommand pCb);


NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPeopleFlowStatisticsCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPeopleFlowStatisticsCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_ResetPeopleFlowStatistics(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPeopleDensityDetectionCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPeopleDensityDetectionCfg(NET_TV_CB_SetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetManholeCoverAbnormalCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetManholeCoverAbnormalCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSleepOnDutyCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetSleepOnDutyCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetElectricVehicleInElevatorCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetElectricVehicleInElevatorCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPersonFallDownCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPersonFallDownCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetConstructionOccupyRoadCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetConstructionOccupyRoadCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetCongestionCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetCongestionCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetLicensePlateRecognitionCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetLicensePlateRecognitionCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetHighAltitudeSeatbeltCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetHighAltitudeSeatbeltCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSafetyHelmetCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetSafetyHelmetCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPersonFallCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPersonFallCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPhoneUsageCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPhoneUsageCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSmokingCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetSmokingCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetOpenFlameCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetOpenFlameCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetBareSoilCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetBareSoilCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetHoleProtectionBarCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetHoleProtectionBarCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetReflectiveClothingCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetReflectiveClothingCfg(NET_TV_CB_SetDevConfigByCommand pCb);

/* 智能事件配置回调注册接口 */
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPetRecognitionInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPetRecognitionInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetClimbFenceInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetClimbFenceInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetDimissionInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetDimissionInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetIllegalLaneInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetIllegalLaneInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetRetrogradeInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetRetrogradeInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetNonmotorVehicleIntrusionInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetNonmotorVehicleIntrusionInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetOccupationEmergencyInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetOccupationEmergencyInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPedestrianIntrusionInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPedestrianIntrusionInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSmokeFireCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetSmokeFireCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetRoadPondingCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetRoadPondingCfg(NET_TV_CB_SetDevConfigByCommand pCb);

#ifdef __cplusplus
}
#endif

#endif

#endif
