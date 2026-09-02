/**
 * @file NetTVSDKClientInterface.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief NetTVSDKClientInterface 模块接口与类型定义
 * 功能说明：
 * 1. 声明 NetTVSDKClientInterface 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#ifndef NETSDK_CLIENT_INTERFACE_H
#define NETSDK_CLIENT_INTERFACE_H



#include "NetTVSDKCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************/
/*                          函数                                  */
/************************************************************************/
/**
 * @author tianl (tianl@kfb.cn)
 * @brief SDK初始化
 * @return TRUE表示成功,其他表示失败 NET_TRUE means success, and any other value means failure.
 * @note
 */
NET_API BOOL NET_STDCALL NET_clientInit(void);

/**
* SDK 清理  SDK cleaning
* @return TRUE表示成功,其他表示失败 NET_TRUE means success, and any other value means failure.
* @note
*/
NET_API BOOL NET_STDCALL NET_clientCleanup(void);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 设置日志
 * @param [in] dwLogLevel   日志的等级（默认为0）：0-表示关闭日志，1-表示只输出ERROR错误日志，2-输出ERROR错误信息和DEBUG调试信息，3-输出ERROR错误信息、DEBUG调试信息和INFO普通信息等所有信息
 * @param [in] strLogDir    日志路径
 * @param [in] dwLogFileSize 日志文件大小(单位：字节)
 * @param [in] dwLogFileNum 日志文件个数
 * @return TRUE表示成功,其他表示失败 NET_TRUE means success, and any other value means failure.
 */
NET_API BOOL NET_STDCALL NET_clientSetLogToFile(NET_IN INT32 dwLogLevel,NET_IN CHAR  *strLogDir,NET_IN INT32 dwLogFileSize,NET_IN INT32 dwLogFileNum);

/**
* 获取SDK的版本信息 Get SDK version information
* @return SDK版本信息 SDK version information
* @note
* - 在两个高字节中高8位表示主版本,低八位表示次版本.两个低字节表示附加版本号如0x01080000：表示版本为1.8.0.0.
* - The two high bytes,The high-8-bit indicate the major version, and the low-8-bytes indicate the minor version.Two low bytes for additional version numbers For example, 0x01080000 means version 1.8.0.0
*/
NET_API INT32 NET_STDCALL NET_clientGetSdkVersion(void);

/**
* 获取错误码  Get error codes
* @return 错误码 Error codes
*/
NET_API INT32 NET_STDCALL NET_clientGetLastError();

/**
* 获取最近一次错误码的描述信息  Get description of last error code
* @return 错误描述字符串（UTF-8），无需调用方释放内存  Error description string (UTF-8), caller does not need to free memory
* @note 与海康 NET_DVR_GetErrorMsg、大华 CLIENT_GetLastError 对齐
*/
NET_API const char* NET_STDCALL NET_clientGetErrorMsg(void);

/**
* 接收异常.重连等消息的回调函数  Callback function to receive exception and reconnection messages
* @param [in] lpUserID     用户登录句柄 User login ID
* @param [in] dwType       异常或重连等消息的类型:NET_EXCEPTION_TYPE_E Type of exception or reconnection message: NET_EXCEPTION_TYPE_E
* @param [in] lpExpHandle  出现异常的相应类型的句柄 Exception type handle
* @param [in] lpUserData   用户数据 User data
* @note
*/
typedef void(NET_STDCALL *NET_ExceptionCallBack_PF)(NET_IN LPVOID lpUserID,
                                                   NET_IN INT32 dwType,
                                                   NET_IN LPVOID lpExpHandle,
                                                   NET_IN LPVOID lpUserData
                                                   );

/**
* 注册sdk接收异常.重连等消息的回调函数  Callback function to register SDK, receive exception and reconnection messages, etc.
* @param [in] cbExceptionCallBack       接收异常消息的回调函数,回调当前异常的相关信息 Callback function to receive exception messages, used to call back information about current exceptions
* @param [in] lpUserData                用户数据 User data
* @return TRUE表示成功,其他表示失败 NET_TRUE means success, and any other value means failure.
* @note
*/
NET_API BOOL NET_STDCALL NET_clientSetExceptionCallBack(NET_IN NET_ExceptionCallBack_PF cbExceptionCallBack,
                                                                 NET_IN LPVOID lpUserData);

/**
* 设置超时时间 Set timeout
* @param [in]  pstRevTimeout         超时时间指针 Pointer to timeout
* @return TRUE表示成功,其他表示失败    NET_TRUE means success, and any other value means failure.
* @note
*/
NET_API BOOL NET_STDCALL NET_clientSetRevTimeOut(NET_IN pNET_RevTimeout_S pstRevTimeout);

/**
* 设置保活参数 Set keep-alive parameters
* @param [in]  dwWaitTime            间隔等待时间  Waiting time
* @param [in]  dwTrytimes            尝试连接次数  Connecting attempts
* @return TRUE表示成功,其他表示失败 NET_TRUE means success, and any other value means failure.
* @note
*/
NET_API BOOL NET_STDCALL NET_clientSetConnectTime(NET_IN INT32 dwWaitTime,
                                                           NET_IN INT32 dwTrytimes);

/**
* 设置自动重连开关  Set auto-reconnect switch
* @param [in] lpUserID  用户登录句柄，不能为空  User login ID, cannot be NULL
* @param [in] bEnable   TRUE启用自动重连，FALSE禁用  TRUE to enable, FALSE to disable
* @return TRUE表示成功,其他表示失败  NET_TRUE means success, and any other value means failure.
* @note
* - 启用后，心跳失败达上限时 SDK 自动启动 ReconnectLoop（指数退避重连）
* - 禁用后，心跳失败达上限时仅通过会话断开通知上层，SDK 不发起重连
* - 与海康 NET_DVR_SetReconnectCallBack、大华 CLIENT_SetAutoReconnect 对齐
*/
NET_API BOOL NET_STDCALL NET_clientSetAutoReconnect(NET_IN LPVOID lpUserID,
                                                    NET_IN BOOL   bEnable);

/**
* 设备登录
* @param [in]  pstDevLoginInfo  设备登录信息
* @param [out] pstDevInfo     	设备信息结构体指针 Pointer to device information structure
* @return 返回值为用户ID。
* @note
* -
*/
NET_API LPVOID NET_STDCALL NET_clientLogin(NET_IN pNET_DeviceLoginInfo_S pstDevLoginInfo,
                                                        NET_OUT pNET_DeviceInfo_S pstDevInfo);

/**
* 用户注销  User logout
* @param [in] lpUserID    用户登录句柄 User login ID
* @return TRUE表示成功,其他表示失败 NET_TRUE means success, and any other value means failure.
* @note
*/
NET_API BOOL NET_STDCALL NET_clientLogout(NET_IN LPVOID lpUserID);

/**
 * @author tianl (tianl@kfb.cn)
* @brief Alarm Callback Function
*/
typedef void(NET_STDCALL *NET_AlarmCallBack)(NET_OUT INT64 lCommand,
                                                   NET_OUT NET_Alarmer_S *pAlarmer,
                                                   NET_OUT CHAR*   pAlarmInfo,
                                                   NET_OUT INT32*   dwBufLen,
                                                   NET_OUT LPVOID lpUserData);

/**
 * @author tianl (tianl@kfb.cn)
* @brief 设置报警回调函数
* @param [in] lpUserID              用户登录ID
* @param [in] cbAlarmMessCallBack   报警消息回调函数
* @param [in] lpUserData            用户数据
* @return TRUE表示成功,其他表示失败
*/
NET_API BOOL NET_STDCALL NET_clientSetAlarmCallBack(NET_IN LPVOID lpUserID,
                                            NET_IN NET_AlarmCallBack cbAlarmMessCallBack,
                                            NET_IN LPVOID lpUserData);

/**
 * @author tianl (tianl@kfb.cn)
* @brief 通道上下线状态回调函数
*/
typedef void(NET_STDCALL *NET_ChannelStatusCallBack)(NET_OUT NET_ChannelInfo_S *pChannelInfo,
                                                    NET_OUT LPVOID lpUserData);

/**
 * @author tianl (tianl@kfb.cn)
* @brief 设置通道上下线状态回调函数
* @param [in] lpUserID                    用户登录ID
* @param [in] cbChannelStatusCallBack     通道状态回调函数
* @param [in] lpUserData                  用户数据
* @return TRUE表示成功,其他表示失败
*/
NET_API BOOL NET_STDCALL NET_clientSetChannelStatusCallBack(NET_IN LPVOID lpUserID,
                                                        NET_IN NET_ChannelStatusCallBack cbChannelStatusCallBack,
                                                        NET_IN LPVOID lpUserData);

/**
 * @author tianl (tianl@kfb.cn)
* @brief 开始监听报警消息
* @param [in] lpUserID              用户登录ID
* @return TRUE表示成功,其他表示失败
*/
NET_API BOOL NET_STDCALL NET_clientStartListen(NET_IN LPVOID lpUserID);

/**
 * @author tianl (tianl@kfb.cn)
* @brief 停止监听报警消息
* @param [in] lpUserID              用户登录ID
* @return TRUE表示成功,其他表示失败
*/
NET_API BOOL NET_STDCALL NET_clientStopListen(NET_IN LPVOID lpUserID);

/**
 * @author tianl (tianl@kfb.cn)
* @brief 设备硬件控制统一入口
* @param [in] lpUserID       用户登录句柄
* @param [in] pstCtrlInfo    设备控制参数，参见 NET_DeviceControlInfo_S
* @return TRUE表示成功,其他表示失败
*/
NET_API BOOL NET_STDCALL NET_clientDeviceControl(NET_IN LPVOID lpUserID,
                                             NET_IN pNET_DeviceControlInfo_S pstCtrlInfo);

/**
 * @author tianl (tianl@kfb.cn)
* @brief 获取回放播放地址
* @param [in]     lpUserID          用户登录句柄
* @param [in,out]  pstInfo           回放查询条件和返回信息，调用前填通道/时间，返回后读取播放URL
* @param [out]    pdwBytesReturned  实际返回的数据长度指针，可为NULL
* @return TRUE表示成功,其他表示失败
*/
NET_API BOOL NET_STDCALL NET_clientGetReplayUrl(NET_IN    LPVOID lpUserID,
                                            NET_INOUT pNET_ReplayUrlInfo_S pstInfo,
                                            NET_OUT   INT32 *pdwBytesReturned);

/**
 * @author tianl (tianl@kfb.cn)
* @brief 控制回放开始/停止/倍速
* @param [in]     lpUserID          用户登录句柄
* @param [in,out]  pstInfo           回放控制信息，开始播放时返回会话ID和播放URL
* @param [out]    pdwBytesReturned  实际返回的数据长度指针，可为NULL
* @return TRUE表示成功,其他表示失败
*/
NET_API BOOL NET_STDCALL NET_clientControlReplay(NET_IN    LPVOID lpUserID,
                                             NET_INOUT pNET_ReplayCtrlInfo_S pstInfo,
                                             NET_OUT   INT32 *pdwBytesReturned);

/**
 * @author tianl (tianl@kfb.cn)
* @brief 获取NVR回放录像时间段
* @param [in]     lpUserID          用户登录句柄
* @param [in,out]  pstInfo           查询条件和返回结果，调用前填通道/日期
* @param [out]    pdwBytesReturned  实际返回的数据长度指针，可为NULL
* @return TRUE表示成功,其他表示失败
*/
NET_API BOOL NET_STDCALL NET_clientGetReplayRecordList(NET_IN    LPVOID lpUserID,
                                                   NET_INOUT pNET_ReplayRecordList_S pstInfo,
                                                   NET_OUT   INT32 *pdwBytesReturned);

/**
* 获取设备能力集 Obtain device capability
* @param [in]   lpUserID                用户登录句柄 User login ID
* @param [in]   dwChannelID             通道号 Channel ID
* @param [in]   dwCommand               设备能力类型指令 NET_CAPABILITY_COMMOND_E
* @param [out]  lpOutBuffer             接收数据的缓冲指针 Pointer to buffer that receives data
* @param [out]  dwOutBufferSize         接收数据的缓冲长度(以字节为单位)，不能为0 Length (in byte) of buffer that receives data, cannot be 0.
* @param [out]  pdwBytesReturned        实际收到的数据长度指针，不能为NULL  Pointer to length of received data, cannot be NULL.
* @return TRUE表示成功，其他表示失败      NET_TRUE means success, and any other value means failure.
* @note
*/
NET_API BOOL NET_STDCALL NET_clientGetDeviceCapability(NET_IN LPVOID lpUserID,
                                                                NET_IN INT32 dwChannelID,
                                                                NET_IN INT32 dwCommand,
                                                                NET_OUT LPVOID lpOutBuffer,
                                                                NET_OUT INT32  dwOutBufferSize,
                                                                NET_OUT INT32  *pdwBytesReturned);
/**
* 获取设备的配置信息  Get configuration information of device
* @param [in]     lpUserID                用户登录句柄 User login ID
* @param [in]     dwChannelID             通道号 Channel ID
* @param [in]     dwCommand               设备配置命令,参见# NET_CONFIG_COMMAND_E  Device configuration commands, see #NET_CONFIG_COMMAND_E
* @param [in,out]  lpOutBuffer             接收数据的缓冲指针 Pointer to buffer that receives data
* @param [out]    dwOutBufferSize         接收数据的缓冲长度(以字节为单位),不能为0 Length (in byte) of buffer that receives data, cannot be 0.
* @param [out]    pdwBytesReturned        实际收到的数据长度指针,不能为NULL  Pointer to length of received data, cannot be NULL.
* @return TRUE表示成功,其他表示失败 NET_TRUE means success, and any other value means failure.
* @note
*/
NET_API BOOL NET_STDCALL NET_clientGetDevConfig(NET_IN  LPVOID  lpUserID,
                                                                NET_IN    INT32   dwChannelID,
                                                                NET_IN    INT32   dwCommand,
                                                                NET_INOUT LPVOID  lpOutBuffer,
                                                                NET_OUT   INT32   dwOutBufferSize,
                                                                NET_OUT   INT32   *pdwBytesReturned);
/**
* 设置设备的配置信息  Modify device configuration information
* @param [in]   lpUserID            用户登录句柄 User login ID
* @param [in]   dwChannelID         通道号 Channel ID
* @param [in]   dwCommand           设备配置命令,参见# NET_CONFIG_COMMAND_E  Device configuration commands, see #NET_CONFIG_COMMAND_E
* @param [in]   lpInBuffer          输入数据的缓冲指针 Pointer to buffer of input data
* @param [in]   dwInBufferSize      输入数据的缓冲长度(以字节为单位) Length of input data buffer (byte)
* @return TRUE表示成功,其他表示失败 NET_TRUE means success, and any other value means failure.
* @note
*/
NET_API BOOL NET_STDCALL NET_clientSetDevConfig(NET_IN  LPVOID  lpUserID,
                                                                NET_IN    INT32   dwChannelID,
                                                                NET_IN    INT32   dwCommand,
                                                                NET_INOUT LPVOID  lpOutBuffer,
                                                                NET_OUT   INT32   dwOutBufferSize,
                                                                NET_OUT   INT32   *pdwBytesReturned);
/************************************************************************/
/*                    设备发现 Device Discovery                           */
/************************************************************************/
/**
/************************************************************************/
/*                       升级 Upgrade                                    */
/************************************************************************/
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 上传固件文件到设备
 * @param [in]  lpUserID     用户登录句柄
 * @param [in]  szFilePath   本地固件文件路径
 * @param [in]  szRemoteName 上传到设备的文件名
 * @return NET_TRUE 成功，NET_FALSE 失败（调用 NET_clientGetLastError() 获取错误码）
 */
NET_API BOOL NET_STDCALL
NET_clientUploadFile(NET_IN LPVOID   lpUserID,
                  NET_IN const CHAR* szFilePath,
                  NET_IN const CHAR* szRemoteName);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 搜索局域网内设备（UDP 组播）
 * @param [in]  szInterfaceIP 网卡 IP 地址，传 NULL 使用默认网卡
 * @param [in]  dwTimeoutMs   等待响应超时 (ms)，建议 2000~5000
 * @param [out] pDeviceList   输出设备列表缓冲区
 * @param [in]  nMaxCount     设备列表最大容量
 * @param [out] pnOutCount    实际发现的设备数量
 * @return NET_TRUE 成功，NET_FALSE 失败（调用 NET_clientGetLastError() 获取错误码）
 * @note 重复调用 NET_clientSearchDiscovery 前无需调用 NET_clientInit()
 */
NET_API BOOL NET_STDCALL
NET_clientSearchDiscovery(NET_IN  const CHAR*                      szInterfaceIP,
                        NET_IN  UINT32                           dwTimeoutMs,
                        NET_OUT NET_DiscoveryDeviceInfo_S*       pDeviceList,
                        NET_IN  int                              nMaxCount,
                        NET_OUT int*                             pnOutCount);

/**
 * @brief 未登录通过设备发现组播协议按 MAC 设置摄像机网络参数。
 * @param [in] pstConfig 发送网卡、目标 MAC 及目标网络参数。
 * @return TRUE 表示组播报文发送成功；FALSE 表示参数或发送失败。
 * @note 必须先调用 NET_clientInit，但不需要调用 NET_clientLogin。发送成功不代表设备已完成修改，调用方应重新搜索确认。
 */
NET_API BOOL NET_STDCALL
NET_clientSetPoeNetwork(NET_IN const NET_PoeNetworkConfig_S* pstConfig);

/************************************************************************/
/*                       语音对讲 VoiceCom                                */
/************************************************************************/
/** @brief 语音对讲回调: 设备端采集的PCM音频数据 */
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 录像帧回调: SDK收到一包录像帧后调用
 * @param [in] pstFrameInfo 帧元数据，包含媒体类型、序号、时间戳、负载长度和标志位
 * @param [in] pData        帧负载数据；流结束包可为空
 * @param [in] dwSize       帧负载长度
 * @param [in] lpUserData   用户数据
 */
typedef void (NET_STDCALL *NET_RecordFrameCallBack)(NET_IN const NET_RecordFrameInfo_S* pstFrameInfo,
                                                   NET_IN const CHAR* pData,
                                                   NET_IN UINT32 dwSize,
                                                   NET_IN LPVOID lpUserData);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 启动录像帧TCP流
 * @param [in]  lpUserID      用户登录句柄
 * @param [in]  pstCond       录像帧流请求条件，包含通道、起止时间、媒体类型、TCP端口等
 * @param [out] pstStreamInfo 服务端返回的流信息，包含流ID、TCP端口、编码信息等
 * @param [in]  cbRecordFrame 录像帧回调
 * @param [in]  lpUserData    回调用户数据
 * @return NET_TRUE 成功，NET_FALSE 失败；失败原因通过 NET_clientGetLastError 获取
 */
NET_API BOOL NET_STDCALL
NET_clientStartRecordFrameStream(NET_IN LPVOID lpUserID,
                              NET_IN pNET_RecordFrameStreamCond_S pstCond,
                              NET_OUT pNET_RecordFrameStreamInfo_S pstStreamInfo,
                              NET_IN NET_RecordFrameCallBack cbRecordFrame,
                              NET_IN LPVOID lpUserData);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 停止录像帧TCP流
 * @param [in] lpUserID  用户登录句柄
 * @param [in] szStreamId 需要停止的流ID
 * @return NET_TRUE 成功，NET_FALSE 失败
 */
NET_API BOOL NET_STDCALL
NET_clientStopRecordFrameStream(NET_IN LPVOID lpUserID,
                             NET_IN const CHAR* szStreamId);

typedef void (NET_STDCALL *NET_VoiceComCallBack)(const char* data, unsigned int size, LPVOID lpUserData);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 开始语音对讲
 * @param [in]  lpUserID      用户登录句柄
 * @param [in]  pstStartInfo  对讲启动参数, 包含设备端音频TCP端口和音频参数
 * @param [in]  cbVoiceCom    音频数据回调
 * @param [in]  lpUserData    回调用户数据
 * @return NET_TRUE 成功，NET_FALSE 失败
 */
NET_API BOOL NET_STDCALL
NET_clientStartVoiceCom(NET_IN LPVOID              lpUserID,
                     NET_IN pNET_VoiceComStartInfo_S pstStartInfo,
                     NET_IN NET_VoiceComCallBack cbVoiceCom,
                     NET_IN LPVOID              lpUserData);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 发送音频数据到设备
 * @param [in]  lpUserID  用户登录句柄
 * @param [in]  pData     音频帧数据，格式需与 NET_clientStartVoiceCom 协商参数一致
 * @param [in]  dwSize    数据长度(字节)
 * @return NET_TRUE 成功，NET_FALSE 失败
 */
NET_API BOOL NET_STDCALL
NET_clientVoiceComSendData(NET_IN LPVOID       lpUserID,
                        NET_IN const CHAR*  pData,
                        NET_IN UINT32       dwSize);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 停止语音对讲
 * @param [in]  lpUserID  用户登录句柄
 * @return NET_TRUE 成功，NET_FALSE 失败
 */
NET_API BOOL NET_STDCALL
NET_clientStopVoiceCom(NET_IN LPVOID lpUserID);

#ifdef __cplusplus
}
#endif

#endif
