

#include<stdio.h>
#include <stdint.h>
#include <memory>

#include "NetTVSDKServerInterface.h"
#include "NetTVSDKServerImpl.h"
#include "VoiceComServer.h"
#include "RecordFrameServer.h"

// 全局Impl单例（使用智能指针）
static std::unique_ptr<CNetTVSDKServerImpl> g_pServerImpl;

#ifdef __cplusplus
extern "C" {
#endif

NET_API BOOL STDCALL NET_SERVER_Init(IN UINT32 udwPort,IN CHAR szUserName[NET_LEN_132],IN CHAR szPassword[NET_LEN_132])
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoInit(udwPort, szUserName, szPassword);
}

NET_API BOOL STDCALL NET_SERVER_Cleanup(void)
{
	if (g_pServerImpl)
	{
		BOOL ret = g_pServerImpl->DoCleanup();
		g_pServerImpl.reset();
		return ret;
	}
	return TRUE;
}

NET_API BOOL STDCALL NET_SERVER_SetLogToFile(IN INT32 dwLogLevel,IN CHAR  *strLogDir,IN INT32 dwLogFileSize,IN INT32 dwLogFileNum)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoSetLogToFile(dwLogLevel, strLogDir, dwLogFileSize, dwLogFileNum);
}

NET_API INT32 STDCALL NET_SERVER_GetSDKVersion(void)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoGetSDKVersion();
}

NET_API INT32 STDCALL NET_SERVER_GetClientCount(void)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoGetClientCount();
}

NET_API BOOL STDCALL NET_SERVER_SetUserPasswd(IN CHAR szUserName[NET_LEN_132],IN CHAR szPassword[NET_LEN_132])
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoSetUserPasswd(szUserName, szPassword);
}

NET_API BOOL STDCALL NET_SERVER_PushAlarmInfo(IN NET_Alarmer_S *pAlarmer,
                                                    IN INT32 lCommand,
                                                    IN LPVOID pAlarmInfo,
                                                    IN INT32 dwBufLen)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoPushAlarmInfo(pAlarmer, lCommand, pAlarmInfo, dwBufLen);
}

NET_API BOOL STDCALL NET_SERVER_PushChannelStatusInfo(IN NET_ChannelInfo_S *pChannelInfo)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoPushChannelStatusInfo(pChannelInfo);
}

NET_API BOOL STDCALL
NET_SERVER_RegisterCb_GetDiscoveryDeviceInfo(
    IN NET_CB_GetDiscoveryDeviceInfo cbFunc)
{
	if (!g_pServerImpl) {
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoRegisterCb_GetDiscoveryDeviceInfo(cbFunc);
}

NET_API BOOL STDCALL
NET_SERVER_Discovery_Start(IN const CHAR* szInterfaceName)
{
	if (!g_pServerImpl) {
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoDiscoveryStart(szInterfaceName);
}

NET_API BOOL STDCALL
NET_SERVER_Discovery_Stop(void)
{
	if (!g_pServerImpl) {
		return FALSE;
	}
	return g_pServerImpl->DoDiscoveryStop();
}

/* ==================== 语音对讲 VoiceCom (服务端) ==================== */

NET_API BOOL STDCALL
NET_SERVER_StartVoiceComServer(IN UINT32 dwPort)
{
	return tvsdk::VoiceComServer::instance()->start(static_cast<int>(dwPort)) ? TRUE : FALSE;
}

NET_API BOOL STDCALL
NET_SERVER_StopVoiceComServer(void)
{
	tvsdk::VoiceComServer::instance()->stop();
	return TRUE;
}

NET_API BOOL STDCALL
NET_SERVER_RegisterCb_VoiceComPlay(IN NET_SERVER_VoiceComPlayCallBack cb)
{
	if (!cb) {
		tvsdk::VoiceComServer::instance()->set_play_callback(nullptr);
		return TRUE;
	}

	tvsdk::VoiceComServer::instance()->set_play_callback(
		[cb](const char* data, size_t size) {
			cb(data, static_cast<unsigned int>(size));
		});
	return TRUE;
}

NET_API BOOL STDCALL
NET_SERVER_RegisterCb_VoiceComCapture(IN NET_SERVER_VoiceComCaptureCallBack cb,
										 IN LPVOID lpUserData)
{
	if (!cb) {
		tvsdk::VoiceComServer::instance()->set_capture_callback(nullptr);
		return TRUE;
	}

	tvsdk::VoiceComServer::instance()->set_capture_callback(
		[cb, lpUserData](const NET_VoiceComAudioParam_S& audioParam,
						 char* buffer,
						 size_t bufferSize) -> int {
			return cb(&audioParam,
					  buffer,
					  static_cast<UINT32>(bufferSize),
					  lpUserData);
		});
	return TRUE;
}

NET_API BOOL STDCALL
NET_SERVER_SendVoiceComData(IN const CHAR* pData, IN UINT32 dwSize)
{
	if (!pData || dwSize == 0) return FALSE;

	return tvsdk::VoiceComServer::instance()->send_to_client(pData, dwSize) ? TRUE : FALSE;
}

NET_API BOOL STDCALL
NET_SERVER_GetVoiceComAudioParam(OUT pNET_VoiceComAudioParam_S pstAudioParam)
{
	if (!pstAudioParam) return FALSE;

	return tvsdk::VoiceComServer::instance()->get_audio_param(*pstAudioParam) ? TRUE : FALSE;
}

/* ==================== 录像帧流 RecordFrame (服务端) ==================== */

/**
 * @brief 启动录像帧流 TCP 服务
 * @details 创建监听socket，开始接收客户端连接
 * @param [IN] dwPort TCP监听端口，建议使用9005
 * @return TRUE 成功，FALSE 失败
 * @note 服务端必须先启动此服务，客户端才能建立TCP连接接收帧数据
 */
NET_API BOOL STDCALL
NET_SERVER_StartRecordFrameServer(IN UINT32 dwPort)
{
	return tvsdk::RecordFrameServer::instance()->start(static_cast<int>(dwPort)) ? TRUE : FALSE;
}

/**
 * @brief 停止录像帧流 TCP 服务
 * @details 关闭监听socket，停止所有客户端连接，释放资源
 * @return TRUE 成功
 */
NET_API BOOL STDCALL
NET_SERVER_StopRecordFrameServer(void)
{
	tvsdk::RecordFrameServer::instance()->stop();
	return TRUE;
}

/**
 * @brief 注册录像帧流启动回调
 * @details 将宿主程序实现的C风格回调转换为C++ Lambda，传递给RecordFrameServer
 *          Lambda捕获cb和lpUserData，在回调执行时恢复C风格函数调用
 * @param [IN] cb 宿主实现的启动回调函数指针，NULL表示取消注册
 * @param [IN] lpUserData 用户自定义数据，SDK会原样传回给宿主
 * @return TRUE 成功
 * @note 为什么要这样写？
 *       1. 宿主程序使用C语言，回调是C风格函数指针，带有lpUserData参数
 *       2. SDK内部使用C++，RecordFrameServer期望的是C++风格的std::function
 *       3. 需要用Lambda做适配器，将C风格回调转换为C++风格回调
 *       4. cond参数做了拷贝（condCopy），因为C回调期望指针，防止引用失效
 */
NET_API BOOL STDCALL
NET_SERVER_RegisterCb_RecordFrameStart(IN NET_SERVER_RecordFrameStartCallBack cb,
										 IN LPVOID lpUserData)
{
	if (!cb) {
		// 取消注册，设置为空回调
		tvsdk::RecordFrameServer::instance()->set_start_callback(nullptr);
		return TRUE;
	}

	// 使用Lambda做适配器：C风格回调 → C++风格回调
	tvsdk::RecordFrameServer::instance()->set_start_callback(
		[cb, lpUserData](const NET_RecordFrameStreamCond_S& cond,
						 NET_RecordFrameStreamInfo_S& info) -> NET_COMMON_ECODE_E {
			// cond参数做拷贝，因为C回调期望指针，防止引用失效
			NET_RecordFrameStreamCond_S condCopy = cond;
			// 调用宿主注册的C风格回调，传入lpUserData
			return cb(&condCopy, &info, lpUserData);
		});
	return TRUE;
}

/**
 * @brief 注册录像帧读取回调
 * @details 将宿主程序实现的C风格回调转换为C++ Lambda，传递给RecordFrameServer
 *          Lambda捕获cb和lpUserData，在回调执行时恢复C风格函数调用
 * @param [IN] cb 宿主实现的读取回调函数指针，NULL表示取消注册
 * @param [IN] lpUserData 用户自定义数据，SDK会原样传回给宿主
 * @return TRUE 成功
 * @note 为什么要这样写？
 *       1. C回调参数：const CHAR* szStreamId, UINT32 dwBufferSize
 *       2. C++回调参数：const std::string& streamId, size_t bufferSize
 *       3. 需要做类型转换：std::string → const char*, size_t → UINT32
 *       4. Read回调会被持续循环调用，Lambda捕获保证cb和lpUserData不会失效
 */
NET_API BOOL STDCALL
NET_SERVER_RegisterCb_RecordFrameRead(IN NET_SERVER_RecordFrameReadCallBack cb,
										IN LPVOID lpUserData)
{
	if (!cb) {
		tvsdk::RecordFrameServer::instance()->set_read_callback(nullptr);
		return TRUE;
	}

	tvsdk::RecordFrameServer::instance()->set_read_callback(
		[cb, lpUserData](const std::string& streamId,
						 NET_RecordFrameInfo_S& frameInfo,
						 char* buffer,
						 size_t bufferSize) -> int {
			// 类型转换：std::string → const char*, size_t → UINT32
			return cb(streamId.c_str(), &frameInfo, buffer, static_cast<UINT32>(bufferSize), lpUserData);
		});
	return TRUE;
}

/**
 * @brief 注册录像帧流停止回调
 * @details 将宿主程序实现的C风格回调转换为C++ Lambda，传递给RecordFrameServer
 *          Lambda捕获cb和lpUserData，在回调执行时恢复C风格函数调用
 * @param [IN] cb 宿主实现的停止回调函数指针，NULL表示取消注册
 * @param [IN] lpUserData 用户自定义数据，SDK会原样传回给宿主
 * @return TRUE 成功
 * @note 为什么要这样写？
 *       1. C回调参数：const CHAR* szStreamId
 *       2. C++回调参数：const std::string& streamId
 *       3. 需要做类型转换：std::string → const char*
 */
NET_API BOOL STDCALL
NET_SERVER_RegisterCb_RecordFrameStop(IN NET_SERVER_RecordFrameStopCallBack cb,
										IN LPVOID lpUserData)
{
	if (!cb) {
		tvsdk::RecordFrameServer::instance()->set_stop_callback(nullptr);
		return TRUE;
	}

	tvsdk::RecordFrameServer::instance()->set_stop_callback(
		[cb, lpUserData](const std::string& streamId) -> NET_COMMON_ECODE_E {
			return cb(streamId.c_str(), lpUserData);
		});
	return TRUE;
}


#ifdef __cplusplus
}
#endif
