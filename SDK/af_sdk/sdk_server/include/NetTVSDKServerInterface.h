

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
 * @param [IN] szDeviceName 设备名称（响应JSON中device_name字段值）
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 * @note
 */
NET_API BOOL STDCALL NET_serverInit(IN UINT32 udwPort,IN CHAR szUserName[NET_LEN_132],IN CHAR szPassword[NET_LEN_132],IN CHAR szDeviceName[NET_LEN_132]);

/**
* SDK 清理  SDK cleaning
* @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
* @note
*/
NET_API BOOL STDCALL NET_serverCleanup(void);

/**
 * @brief 设置日志
 * @param [IN] dwLogLevel   日志的等级（默认为0）：0-表示关闭日志，1-表示只输出ERROR错误日志，2-输出ERROR错误信息和DEBUG调试信息，3-输出ERROR错误信息、DEBUG调试信息和INFO普通信息等所有信息
 * @param [IN] strLogDir    日志路径
 * @param [IN] nLogFileSize 日志文件大小(单位：字节)
 * @param [IN] dwLogFileNum 日志文件个数
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_API BOOL STDCALL NET_serverSetLogToFile(IN INT32 dwLogLevel,IN CHAR  *strLogDir,IN INT32 nLogFileSize,IN INT32 dwLogFileNum);

/**
* 获取SDK的版本信息 Get SDK version information
* @return SDK版本信息 SDK version information
* @note
* - 在两个高字节中高8位表示主版本,低八位表示次版本.两个低字节表示附加版本号如0x01080000：表示版本为1.8.0.0.
* - The two high bytes,The high-8-bit indicate the major version, and the low-8-bytes indicate the minor version.Two low bytes for additional version numbers For example, 0x01080000 means version 1.8.0.0
*/
NET_API INT32 STDCALL NET_serverGetSdkVersion(void);

/**
 * @brief 获取当前在线客户端数量（活跃会话数）
 * @return 客户端数量
 */
NET_API INT32 STDCALL NET_serverGetClientCount(void);

/**
 * @brief 设置用户名密码
 * @param [IN] szUserName 用户名
 * @param [IN] szPassword 密码
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_API BOOL STDCALL NET_serverSetUserPassword(IN CHAR szUserName[NET_LEN_132],IN CHAR szPassword[NET_LEN_132]);

/**
 * @brief 推送告警信息
 * @param [IN] pAlarmer    告警设备信息
 * @param [IN] lCommand    命令码(报警类型)，用于客户端按命令码反序列化结构体
 * @param [IN] pAlarmInfo  具体告警结构体指针（类型由 lCommand 决定）
 * @param [IN] dwBufLen    pAlarmInfo 长度（一般为 sizeof(对应结构体)）
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_API BOOL STDCALL NET_serverPushAlarmInfo(IN NET_Alarmer_S *pAlarmer,
                                                    IN INT32 lCommand,
                                                    IN LPVOID pAlarmInfo,
                                                    IN INT32 dwBufLen);

/*
 * 专用抓拍兼容入口。
 * 服务端内部会转换为 NET_AlarmCaptureInfo_S 和 NET_ALARM_CAPTURE_* 统一协议后推送，
 * 客户端无需再维护四套不同的接收分支。
 */
NET_API BOOL STDCALL NET_serverPushFaceCaptureInfo(IN NET_Alarmer_S* pAlarmer,
                                                    IN NET_FaceCapturePushInfo_S* pCaptureInfo);
NET_API BOOL STDCALL NET_serverPushPersonCaptureInfo(IN NET_Alarmer_S* pAlarmer,
                                                      IN NET_PersonCapturePushInfo_S* pCaptureInfo);
NET_API BOOL STDCALL NET_serverPushMotorvehicleCaptureInfo(IN NET_Alarmer_S* pAlarmer,
                                                            IN NET_MotorvehicleCapturePushInfo_S* pCaptureInfo);
NET_API BOOL STDCALL NET_serverPushNonMotorvehicleCaptureInfo(IN NET_Alarmer_S* pAlarmer,
                                                               IN NET_NonMotorvehicleCapturePushInfo_S* pCaptureInfo);

/**
 * @brief 推送通道上下线状态
 * @param [IN] pChannelInfo 通道信息，byOnline/nDevState 表示当前状态
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_API BOOL STDCALL NET_serverPushChannelStatusInfo(IN NET_ChannelInfo_S *pChannelInfo);

/* 注册设备信息获取回调（NVR规模/能力数量：通道数/报警端口数等，BG6_ZHSJ/BU_SJCL专用） */
NET_API BOOL STDCALL NET_serverRegisterGetDeviceInfoCb(NET_COMMON_ECODE_E (*CB)(pNET_DeviceInfo_S pInfo));

/**
 * @brief 设备基本信息回调类型（通用身份信息：型号/序列号/固件/MAC等）
 * @note  NET_DeviceBasicInfo_S 为通用设备身份属性，收口于通用设备回调
 * @param [OUT] pInfo 设备基本信息结构体指针，由回调填充
 * @return NET_E_SUCCEED 成功，其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetDeviceBasicInfo)(pNET_DeviceBasicInfo_S pInfo);

/**
 * @brief 注册获取设备基本信息回调 (NET_GET_DEVICECFG)
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_serverRegisterGetDeviceBasicInfoCb(NET_CB_GetDeviceBasicInfo pCb);

/**
 * @brief 设置设备基本信息回调类型（仅设备名strDeviceName可写，其余字段只读）
 * @note  身份字段(序列号/固件/MAC/型号/厂商)只读，宿主回调应仅应用strDeviceName
 * @param [IN] pInfo 设备基本信息结构体指针，含待设置字段
 * @return NET_E_SUCCEED 成功，其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_SetDeviceBasicInfo)(pNET_DeviceBasicInfo_S pInfo);

/**
 * @brief 注册设置设备基本信息回调 (NET_SET_DEVICECFG)
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_serverRegisterSetDeviceBasicInfoCb(NET_CB_SetDeviceBasicInfo pCb);

/**
 * @brief 设备存储信息回调类型（NVR/录播等有硬盘的设备专用）
 * @param [OUT] pInfo 设备存储信息结构体指针，由回调函数填充
 * @return NET_E_SUCCEED 成功，NET_E_NOT_SUPPORT 表示设备无存储，其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetDeviceStorageInfo)(pNET_DeviceStorageInfo_S pInfo);

/**
 * @brief 注册获取设备存储信息回调 (NET_GET_STORAGE_INFO)
 * @details 只有具备存储能力的设备才需要注册此回调。
 *          编码器、矩阵等无存储设备不注册即可，SDK 返回 NET_E_NOT_SUPPORT。
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_serverRegisterGetDeviceStorageInfoCb(NET_CB_GetDeviceStorageInfo pCb);

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
NET_API BOOL STDCALL NET_serverRegisterDeviceControlCb(NET_CB_DeviceControl pCb);

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
NET_API BOOL STDCALL NET_serverRegisterSetUserPasswordCb(NET_CB_SetUserPassword pCb);

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
NET_API BOOL STDCALL NET_serverRegisterGetVideoEncodeCapCb(NET_CB_GetVideoEncodeCap pCb);

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
NET_API BOOL STDCALL NET_serverRegisterGetAudioEncodeCapCb(NET_CB_GetAudioEncodeCap pCb);

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
NET_API BOOL STDCALL NET_serverRegisterGetOsdCapCb(NET_CB_GetOsdCap pCb);

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
NET_API BOOL STDCALL NET_serverRegisterGetDevConfigCb(NET_CB_GetDevConfig pCb);

/**
 * @brief 注册通用配置设置回调（所有命令码统一处理）
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 * @note 当没有按命令码注册的专用回调时，会调用此通用回调
 */
NET_API BOOL STDCALL NET_serverRegisterSetDevConfigCb(NET_CB_SetDevConfig pCb);
/************************************************************************/
/*                          录播的配置回调接口                     */
/************************************************************************/

NET_API BOOL STDCALL NET_serverRegisterGetRegisterInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetRegisterInfoCb(NET_CB_SetDevConfigByCommand pCb);

/************************************************************************/
/*                          按命令码注册的配置回调接口                     */
/************************************************************************/
/**
 * @brief 注册设备基本信息获取回调 (NET_GET_DEVICECFG)
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_serverRegisterGetDeviceConfigCb(NET_CB_GetDevConfigByCommand pCb);

/**
 * @brief 注册设备基本信息设置回调 (NET_SET_DEVICECFG)
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_serverRegisterSetDeviceConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetNtpConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetNtpConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetStreamConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetStreamConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetRtspUrlCb(NET_CB_GetRtspUrl pCb);
NET_API BOOL STDCALL NET_serverRegisterGetReplayUrlCb(NET_CB_GetReplayUrl pCb);
NET_API BOOL STDCALL NET_serverRegisterControlReplayCb(NET_CB_ControlReplay pCb);
NET_API BOOL STDCALL NET_serverRegisterGetReplayRecordListCb(NET_CB_GetReplayRecordList pCb);
NET_API BOOL STDCALL NET_serverRegisterGetOsdCapConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetOsdCapConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetImageConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetImageConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetNetworkConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetNetworkConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetConfigWifiStaCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterConnectWifiStaCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterDisconnectWifiStaCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGet4GInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSet4GInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetHotspotInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetHotspotConnCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetSecurityServicesInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetSecurityServicesInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetSshCountdownCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterFindLogCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterExportLogCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetLogServerCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetLogServerCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterTestLogServerCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterControlRecordInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetRecordStatusCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetRecordScheduleCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetRecordScheduleCb(NET_CB_SetDevConfigByCommand pCb);
/**
 * @brief 注册获取 SD 卡物理状态的回调函数。
 * @author ITC
 * @param [in] pCb 用于填充 NET_SdCardStatus_S 输出缓冲区的回调函数。
 * @param [out] 无。SDK 将回调函数保存到配置回调表。
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterGetSdCardStatusCb(NET_CB_GetDevConfigByCommand pCb);
/*
 * 功能描述：注册获取声音报警配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于填写 NET_AudibleAlarmInfo_S 输出数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterGetAudibleAlarmInfoCb(NET_CB_GetDevConfigByCommand pCb);
/*
 * 功能描述：注册设置声音报警配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于处理 NET_AudibleAlarmInfo_S 输入数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterSetAudibleAlarmInfoCb(NET_CB_SetDevConfigByCommand pCb);
/*
 * 功能描述：注册获取报警输入配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于填写 NET_AlarmInputInfoList_S 输出数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterGetAlarmInputInfoCb(NET_CB_GetDevConfigByCommand pCb);
/*
 * 功能描述：注册设置单个报警输入配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于处理 NET_AlarmInputInfo_S 输入数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterSetAlarmInputInfoCb(NET_CB_SetDevConfigByCommand pCb);
/*
 * 功能描述：注册获取报警输出配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于填写 NET_AlarmOutputInfoList_S 输出数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterGetAlarmOutputInfoCb(NET_CB_GetDevConfigByCommand pCb);
/*
 * 功能描述：注册设置单个报警输出配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于处理 NET_AlarmOutputInfo_S 输入数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterSetAlarmOutputInfoCb(NET_CB_SetDevConfigByCommand pCb);
/*
 * 功能描述：注册获取闪光报警配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于填写 NET_FlashingLightAlarmInfo_S 输出数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterGetFlashingLightAlarmInfoCb(NET_CB_GetDevConfigByCommand pCb);
/*
 * 功能描述：注册设置闪光报警配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于处理 NET_FlashingLightAlarmInfo_S 输入数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterSetFlashingLightAlarmInfoCb(NET_CB_SetDevConfigByCommand pCb);
/*
 * 功能描述：注册获取人体红外（PIR）报警配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于填写 NET_PirAlarmInfo_S 输出数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterGetPirAlarmInfoCb(NET_CB_GetDevConfigByCommand pCb);
/*
 * 功能描述：注册设置人体红外（PIR）报警配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于处理 NET_PirAlarmInfo_S 输入数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterSetPirAlarmInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetRecordAdvancedParamCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetRecordAdvancedParamCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterFindRecordFileInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterDownloadRecordFileCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetPrivacyMaskConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetPrivacyMaskConfigCb(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_serverRegisterGetTamperAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetTamperAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetMotionAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetMotionAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetCrossLineAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetCrossLineAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetIntrusionAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetIntrusionAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetLoiteringAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetLoiteringAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetAudioAnomalyAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetAudioAnomalyAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetPreviewInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetPreviewInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetChannelInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetCrowGatheringAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetCrowGatheringAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetParkingAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetParkingAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetUnattendedObjectAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetUnattendedObjectAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetObjectRemovalAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetObjectRemovalAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetSceneChangeAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetSceneChangeAlarmCb(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_serverRegisterGetGarbageExposureConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetGarbageExposureConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetGarbageOverflowConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetGarbageOverflowConfigCb(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_serverRegisterSetTalkbackStateCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetTalkbackToStreamCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetTalkbackFromStreamCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetReplayTalkbackCb(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_serverRegisterGetUpgradeStatusCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetUpgradeCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetUpgradeVersionCb(NET_CB_GetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_serverRegisterGetCapturePlanInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetCapturePlanInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetCaptureParamInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetCaptureParamInfoCb(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_serverRegisterGetExposureInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetExposureInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetDayNightInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetDayNightInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetBackLightInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetBackLightInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetDenoiseInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetDenoiseInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetWhiteBalanceInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetWhiteBalanceInfoCb(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_serverRegisterGetAudioConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetAudioConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetEnterRegionAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetEnterRegionAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetLeaveRegionAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetLeaveRegionAlarmCb(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_serverRegisterGetFaceCaptureInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetFaceCaptureInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetFaceCaptureOverlayInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetFaceCaptureOverlayInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetFaceCompareInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterAddTargetLibCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterDelTargetLibCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetTargetLibCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetTargetLibCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterAddFaceInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterDelFaceInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetFaceInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetFaceInfoCb(NET_CB_GetDevConfigByCommand pCb);


NET_API BOOL STDCALL NET_serverRegisterGetPeopleFlowStatisticsConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetPeopleFlowStatisticsConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterResetPeopleFlowStatisticsCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetPeopleDensityDetectionConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetPeopleDensityDetectionConfigCb(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_serverRegisterGetManholeCoverAbnormalConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetManholeCoverAbnormalConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetSleepOnDutyConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetSleepOnDutyConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetElectricVehicleInElevatorConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetElectricVehicleInElevatorConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetPersonFallDownConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetPersonFallDownConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetConstructionOccupyRoadConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetConstructionOccupyRoadConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetCongestionConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetCongestionConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetLicensePlateRecognitionConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetLicensePlateRecognitionConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetHighAltitudeSeatbeltConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetHighAltitudeSeatbeltConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetSafetyHelmetConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetSafetyHelmetConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetPersonFallConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetPersonFallConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetPhoneUsageConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetPhoneUsageConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetSmokingConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetSmokingConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetOpenFlameConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetOpenFlameConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetBareSoilConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetBareSoilConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetHoleProtectionBarConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetHoleProtectionBarConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetReflectiveClothingConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetReflectiveClothingConfigCb(NET_CB_SetDevConfigByCommand pCb);

/* 智能事件配置回调注册接口 */
NET_API BOOL STDCALL NET_serverRegisterGetPetRecognitionInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetPetRecognitionInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetClimbFenceInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetClimbFenceInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetDimissionInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetDimissionInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetIllegalLaneInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetIllegalLaneInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetRetrogradeInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetRetrogradeInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetNonmotorVehicleIntrusionInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetNonmotorVehicleIntrusionInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetOccupationEmergencyInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetOccupationEmergencyInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetPedestrianIntrusionInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetPedestrianIntrusionInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetSmokeFireConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetSmokeFireConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetRoadPondingConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetRoadPondingConfigCb(NET_CB_SetDevConfigByCommand pCb);

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
NET_serverRegisterGetDiscoveryDeviceInfoCb(
    IN NET_CB_GetDiscoveryDeviceInfo cbFunc);

/**
 * @brief 启动设备发现响应服务（阻塞线程中运行 AF_PACKET 接收循环）
 * @param [IN] szInterfaceName 网卡名称 (如 "eth0")
 * @return TRUE 成功，FALSE 失败
 * @note 需先调用 NET_serverRegisterGetDiscoveryDeviceInfoCb 注册回调
 */
NET_API BOOL STDCALL
NET_serverStartDiscovery(IN const CHAR* szInterfaceName);

/**
 * @brief 停止设备发现响应服务
 * @return TRUE 成功，FALSE 失败
 */
NET_API BOOL STDCALL
NET_serverStopDiscovery(void);

/************************************************************************/
/*                       语音对讲 VoiceCom (服务端)                       */
/************************************************************************/
/** @brief 语音对讲播放回调: 收到NVR端音频时调用, 推送到扬声器 */
typedef void (STDCALL *NET_serverVoiceComPlayCallBack)(const char* data, unsigned int size);
/**
 * @brief 语音对讲采集回调: SDK按协商参数主动拉取设备侧采集帧并发送到NVR
 * @param [IN]  pstAudioParam 当前 VoiceCom 会话协商的音频参数
 * @param [OUT] pBuffer       输出音频帧缓存
 * @param [IN]  dwBufferSize  输出缓存长度
 * @param [IN]  lpUserData    用户数据
 * @return 实际写入的音频字节数，返回 <=0 表示当前无可用音频帧
 * @note 回调内应写入与 pstAudioParam 匹配的裸音频帧；建议每次返回 dwFrameBytes 字节。
 */
typedef INT32 (STDCALL *NET_serverVoiceComCaptureCallBack)(
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
NET_serverStartVoiceComServer(IN UINT32 dwPort);

/**
 * @brief 停止语音对讲TCP监听
 * @return TRUE 成功，FALSE 失败
 */
NET_API BOOL STDCALL
NET_serverStopVoiceComServer(void);

/**
 * @brief 注册播放回调 (收到NVR音频 → 扬声器)
 * @param [IN]  cb  播放回调
 * @return TRUE 成功，FALSE 失败
 */
NET_API BOOL STDCALL
NET_serverRegisterVoiceComPlayCb(IN NET_serverVoiceComPlayCallBack cb);

/**
 * @brief 注册采集回调 (麦克风/LineIn -> NVR)
 * @param [IN]  cb          采集回调，传 NULL 表示注销
 * @param [IN]  lpUserData  用户数据，回调时原样透传
 * @return TRUE 成功，FALSE 失败
 * @note SDK负责按当前 VoiceCom 会话参数定时拉帧并发送，业务侧只需要提供采集帧。
 */
NET_API BOOL STDCALL
NET_serverRegisterVoiceComCaptureCb(IN NET_serverVoiceComCaptureCallBack cb,
                                         IN LPVOID lpUserData);

/**
 * @brief 发送麦克风采集的音频到NVR
 * @param [IN]  pData  音频帧数据，格式需与当前 VoiceCom 会话协商参数一致
 * @param [IN]  dwSize 数据长度(字节)
 * @return TRUE 成功，FALSE 失败
 */
NET_API BOOL STDCALL
NET_serverSendVoiceComData(IN const CHAR* pData, IN UINT32 dwSize);

/**
 * @brief 获取当前 VoiceCom 会话协商的音频参数
 * @param [OUT] pstAudioParam  音频参数
 * @return TRUE 成功，FALSE 表示尚未建立会话或参数未协商
 */
NET_API BOOL STDCALL
NET_serverGetVoiceComAudioParam(OUT pNET_VoiceComAudioParam_S pstAudioParam);

/************************************************************************/
/*                       录像帧流 RecordFrame (服务端)                    */
/************************************************************************/
/**
 * @brief 录像帧流开始回调: 收到客户端起止时间查询后调用, 由宿主打开录像源并填充流信息
 */
typedef NET_COMMON_ECODE_E (STDCALL *NET_serverRecordFrameStartCallBack)(
    IN pNET_RecordFrameStreamCond_S pstCond,
    INOUT pNET_RecordFrameStreamInfo_S pstInfo,
    IN LPVOID lpUserData);

/**
 * @brief 录像帧读取回调: SDK在TCP连接建立后循环拉取帧并发送给客户端
 * @return 实际写入 pBuffer 的负载字节数；0 表示暂时无帧；<0 表示结束/失败
 */
typedef INT32 (STDCALL *NET_serverRecordFrameReadCallBack)(
    IN const CHAR* szStreamId,
    OUT pNET_RecordFrameInfo_S pstFrameInfo,
    OUT CHAR* pBuffer,
    IN UINT32 dwBufferSize,
    IN LPVOID lpUserData);

/** @brief 录像帧流停止回调: 客户端停止或流结束时调用 */
typedef NET_COMMON_ECODE_E (STDCALL *NET_serverRecordFrameStopCallBack)(
    IN const CHAR* szStreamId,
    IN LPVOID lpUserData);

NET_API BOOL STDCALL
NET_serverStartRecordFrameServer(IN UINT32 dwPort);

NET_API BOOL STDCALL
NET_serverStopRecordFrameServer(void);

NET_API BOOL STDCALL
NET_serverRegisterRecordFrameStartCb(IN NET_serverRecordFrameStartCallBack cb,
                                         IN LPVOID lpUserData);

NET_API BOOL STDCALL
NET_serverRegisterRecordFrameReadCb(IN NET_serverRecordFrameReadCallBack cb,
                                        IN LPVOID lpUserData);

NET_API BOOL STDCALL
NET_serverRegisterRecordFrameStopCb(IN NET_serverRecordFrameStopCallBack cb,
                                        IN LPVOID lpUserData);

#ifdef __cplusplus
}
#endif

#endif
