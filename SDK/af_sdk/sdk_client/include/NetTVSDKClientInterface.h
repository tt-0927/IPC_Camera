
#ifndef _NETTVSDKCLIENTINTERFACE_H
#define _NETTVSDKCLIENTINTERFACE_H

#include "NetTVSDKCommon.h"
 
#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************/
/*                          函数                                  */
/************************************************************************/
/**
 * @brief SDK初始化
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 * @note
 */
NET_TV_API BOOL STDCALL NET_TV_Init(void);

/**
* SDK 清理  SDK cleaning
* @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
* @note
*/
NET_TV_API BOOL STDCALL NET_TV_Cleanup(void);

/**
 * @brief 设置日志
 * @param [IN] dwLogLevel   日志的等级（默认为0）：0-表示关闭日志，1-表示只输出ERROR错误日志，2-输出ERROR错误信息和DEBUG调试信息，3-输出ERROR错误信息、DEBUG调试信息和INFO普通信息等所有信息 
 * @param [IN] strLogDir    日志路径
 * @param [IN] dwLogFileSize 日志文件大小(单位：字节)
 * @param [IN] dwLogFileNum 日志文件个数
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_TV_API BOOL STDCALL NET_TV_SetLogToFile(IN INT32 dwLogLevel,IN CHAR  *strLogDir,IN INT32 dwLogFileSize,IN INT32 dwLogFileNum);

/**
* 获取SDK的版本信息 Get SDK version information 
* @return SDK版本信息 SDK version information
* @note
* - 在两个高字节中高8位表示主版本,低八位表示次版本.两个低字节表示附加版本号如0x01080000：表示版本为1.8.0.0.
* - The two high bytes,The high-8-bit indicate the major version, and the low-8-bytes indicate the minor version.Two low bytes for additional version numbers For example, 0x01080000 means version 1.8.0.0
*/
NET_TV_API INT32 STDCALL NET_TV_GetSDKVersion(void);

/**
* 获取错误码  Get error codes
* @return 错误码 Error codes
*/
NET_TV_API INT32 STDCALL NET_TV_GetLastError();

/**
* 接收异常.重连等消息的回调函数  Callback function to receive exception and reconnection messages
* @param [IN] lpUserID     用户登录句柄 User login ID
* @param [IN] dwType       异常或重连等消息的类型:NET_TV_EXCEPTION_TYPE_E Type of exception or reconnection message: NET_TV_EXCEPTION_TYPE_E
* @param [IN] lpExpHandle  出现异常的相应类型的句柄 Exception type handle
* @param [IN] lpUserData   用户数据 User data
* @note
*/
typedef void(STDCALL *NET_TV_ExceptionCallBack_PF)(IN LPVOID lpUserID,
                                                   IN INT32 dwType,
                                                   IN LPVOID lpExpHandle,
                                                   IN LPVOID lpUserData
                                                   );

/**
* 注册sdk接收异常.重连等消息的回调函数  Callback function to register SDK, receive exception and reconnection messages, etc.
* @param [IN] cbExceptionCallBack       接收异常消息的回调函数,回调当前异常的相关信息 Callback function to receive exception messages, used to call back information about current exceptions
* @param [IN] lpUserData                用户数据 User data
* @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
* @note
*/
NET_TV_API BOOL STDCALL NET_TV_SetExceptionCallBack(IN NET_TV_ExceptionCallBack_PF cbExceptionCallBack,
                                                                 IN LPVOID lpUserData);

/**
* 设置超时时间 Set timeout
* @param [IN]  pstRevTimeout         超时时间指针 Pointer to timeout
* @return TRUE表示成功,其他表示失败    TRUE means success, and any other value means failure.
* @note
*/
NET_TV_API BOOL STDCALL NET_TV_SetRevTimeOut(IN LPNET_TV_REV_TIMEOUT_S pstRevTimeout);

/**
* 设置保活参数 Set keep-alive parameters
* @param [IN]  dwWaitTime            间隔等待时间  Waiting time
* @param [IN]  dwTrytimes            尝试连接次数  Connecting attempts
* @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
* @note
*/
NET_TV_API BOOL STDCALL NET_TV_SetConnectTime(IN INT32 dwWaitTime,
                                                           IN INT32 dwTrytimes);
/**
* 设备登录
* @param [IN]  pstDevLoginInfo  设备登录信息
* @param [OUT] pstDevInfo     	设备信息结构体指针 Pointer to device information structure
* @return 返回值为用户ID。
* @note 
* -
*/
NET_TV_API LPVOID STDCALL NET_TV_Login(IN LPNET_TV_DEVICE_LOGIN_INFO_S pstDevLoginInfo, 
                                                        OUT LPNET_TV_DEVICE_INFO_S pstDevInfo);		

/**
* 用户注销  User logout
* @param [IN] lpUserID    用户登录句柄 User login ID
* @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
* @note
*/
NET_TV_API BOOL STDCALL NET_TV_Logout(IN LPVOID lpUserID);	

/**
* @brief Alarm Callback Function
*/
typedef void(STDCALL *NET_TV_AlarmCallBack)(OUT INT64 lCommand,
                                                   OUT NET_TV_ALARMER_S *pAlarmer,
                                                   OUT CHAR*   pAlarmInfo,
                                                   OUT INT32*   dwBufLen,
                                                   OUT LPVOID lpUserData);

/**
* @brief 设置报警回调函数
* @param [IN] lpUserID              用户登录ID
* @param [IN] cbAlarmMessCallBack   报警消息回调函数
* @param [IN] lpUserData            用户数据
* @return TRUE表示成功,其他表示失败
*/
NET_TV_API BOOL STDCALL NET_TV_SetAlarmCallBack(IN LPVOID lpUserID,
                                            IN NET_TV_AlarmCallBack cbAlarmMessCallBack,
                                            IN LPVOID lpUserData);

/**
* @brief 通道上下线状态回调函数
*/
typedef void(STDCALL *NET_TV_ChannelStatusCallBack)(OUT NET_TV_CHANNEL_INFO_S *pChannelInfo,
                                                    OUT LPVOID lpUserData);

/**
* @brief 设置通道上下线状态回调函数
* @param [IN] lpUserID                    用户登录ID
* @param [IN] cbChannelStatusCallBack     通道状态回调函数
* @param [IN] lpUserData                  用户数据
* @return TRUE表示成功,其他表示失败
*/
NET_TV_API BOOL STDCALL NET_TV_SetChannelStatusCallBack(IN LPVOID lpUserID,
                                                        IN NET_TV_ChannelStatusCallBack cbChannelStatusCallBack,
                                                        IN LPVOID lpUserData);

/**
* @brief 开始监听报警消息
* @param [IN] lpUserID              用户登录ID
* @return TRUE表示成功,其他表示失败
*/
NET_TV_API BOOL STDCALL NET_TV_StartListen(IN LPVOID lpUserID);

/**
* @brief 停止监听报警消息
* @param [IN] lpUserID              用户登录ID
* @return TRUE表示成功,其他表示失败
*/
NET_TV_API BOOL STDCALL NET_TV_StopListen(IN LPVOID lpUserID);

/**
* @brief 获取回放播放地址
* @param [IN]     lpUserID          用户登录句柄
* @param [INOUT]  pstInfo           回放查询条件和返回信息，调用前填通道/时间，返回后读取播放URL
* @param [OUT]    pdwBytesReturned  实际返回的数据长度指针，可为NULL
* @return TRUE表示成功,其他表示失败
*/
NET_TV_API BOOL STDCALL NET_TV_GetReplayUrl(IN    LPVOID lpUserID,
                                            INOUT LPNET_TV_REPLAY_URL_INFO_S pstInfo,
                                            OUT   INT32 *pdwBytesReturned);

/**
* @brief 控制回放开始/停止/倍速
* @param [IN]     lpUserID          用户登录句柄
* @param [INOUT]  pstInfo           回放控制信息，开始播放时返回会话ID和播放URL
* @param [OUT]    pdwBytesReturned  实际返回的数据长度指针，可为NULL
* @return TRUE表示成功,其他表示失败
*/
NET_TV_API BOOL STDCALL NET_TV_ControlReplay(IN    LPVOID lpUserID,
                                             INOUT LPNET_TV_REPLAY_CTRL_INFO_S pstInfo,
                                             OUT   INT32 *pdwBytesReturned);

/**
* @brief 获取NVR回放录像时间段
* @param [IN]     lpUserID          用户登录句柄
* @param [INOUT]  pstInfo           查询条件和返回结果，调用前填通道/日期
* @param [OUT]    pdwBytesReturned  实际返回的数据长度指针，可为NULL
* @return TRUE表示成功,其他表示失败
*/
NET_TV_API BOOL STDCALL NET_TV_GetReplayRecordList(IN    LPVOID lpUserID,
                                                   INOUT LPNET_TV_REPLAY_RECORD_LIST_S pstInfo,
                                                   OUT   INT32 *pdwBytesReturned);

/**
* 获取设备能力集 Obtain device capability
* @param [IN]   lpUserID                用户登录句柄 User login ID
* @param [IN]   dwChannelID             通道号 Channel ID
* @param [IN]   dwCommand               设备能力类型指令 NET_TV_CAPABILITY_COMMOND_E
* @param [OUT]  lpOutBuffer             接收数据的缓冲指针 Pointer to buffer that receives data
* @param [OUT]  dwOutBufferSize         接收数据的缓冲长度(以字节为单位)，不能为0 Length (in byte) of buffer that receives data, cannot be 0.
* @param [OUT]  pdwBytesReturned        实际收到的数据长度指针，不能为NULL  Pointer to length of received data, cannot be NULL.
* @return TRUE表示成功，其他表示失败      TRUE means success, and any other value means failure.
* @note
*/
NET_TV_API BOOL STDCALL NET_TV_GetDeviceCapability(IN LPVOID lpUserID,
                                                                IN INT32 dwChannelID,
                                                                IN INT32 dwCommand,
                                                                OUT LPVOID lpOutBuffer,
                                                                OUT INT32  dwOutBufferSize,
                                                                OUT INT32  *pdwBytesReturned);
/**
* 获取设备的配置信息  Get configuration information of device
* @param [IN]     lpUserID                用户登录句柄 User login ID
* @param [IN]     dwChannelID             通道号 Channel ID
* @param [IN]     dwCommand               设备配置命令,参见# NET_TV_CONFIG_COMMAND_E  Device configuration commands, see #NET_TV_CONFIG_COMMAND_E
* @param [INOUT]  lpOutBuffer             接收数据的缓冲指针 Pointer to buffer that receives data
* @param [OUT]    dwOutBufferSize         接收数据的缓冲长度(以字节为单位),不能为0 Length (in byte) of buffer that receives data, cannot be 0.
* @param [OUT]    pdwBytesReturned        实际收到的数据长度指针,不能为NULL  Pointer to length of received data, cannot be NULL.
* @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
* @note
*/										
NET_TV_API BOOL STDCALL NET_TV_GetDevConfig(IN  LPVOID  lpUserID,
                                                                IN    INT32   dwChannelID,
                                                                IN    INT32   dwCommand,
                                                                INOUT LPVOID  lpOutBuffer,
                                                                OUT   INT32   dwOutBufferSize,
                                                                OUT   INT32   *pdwBytesReturned);
/**
* 设置设备的配置信息  Modify device configuration information
* @param [IN]   lpUserID            用户登录句柄 User login ID
* @param [IN]   dwChannelID         通道号 Channel ID
* @param [IN]   dwCommand           设备配置命令,参见# NET_TV_CONFIG_COMMAND_E  Device configuration commands, see #NET_TV_CONFIG_COMMAND_E
* @param [IN]   lpInBuffer          输入数据的缓冲指针 Pointer to buffer of input data
* @param [IN]   dwInBufferSize      输入数据的缓冲长度(以字节为单位) Length of input data buffer (byte)
* @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
* @note
*/
NET_TV_API BOOL STDCALL NET_TV_SetDevConfig(IN  LPVOID  lpUserID,
                                                                IN    INT32   dwChannelID,
                                                                IN    INT32   dwCommand,
                                                                INOUT LPVOID  lpOutBuffer,
                                                                OUT   INT32   dwOutBufferSize,
                                                                OUT   INT32   *pdwBytesReturned);
#ifdef __cplusplus
}
#endif

#endif
