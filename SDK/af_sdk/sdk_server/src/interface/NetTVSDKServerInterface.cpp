/**
 * @file NetTVSDKServerInterface.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief NetTVSDKServerInterface 模块实现
 * 功能说明：
 * 1. 实现 NetTVSDKServerInterface 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#include<stdio.h>
#include <stdint.h>
#include <memory>

#include "NetTVSDKServerInterface.h"
#include "NetTVSDKServerImpl.h"
#include "VoiceComServer.h"
#include "RecordFrameServer.h"

/* 全局Impl单例（使用智能指针） */
static std::unique_ptr<CNetTVSDKServerImpl> g_pServerImpl;

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_Init 定义的内部处理。
 * @param [in] udwPort 函数处理参数。
 * @param [in] szUserName 函数处理参数。
 * @param [in] szPassword 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_Init(NET_IN UINT32 udwPort,NET_IN CHAR szUserName[NET_LEN_132],NET_IN CHAR szPassword[NET_LEN_132])
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoInit(udwPort, szUserName, szPassword);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_Cleanup 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_Cleanup(void)
{
	if (g_pServerImpl)
	{
		BOOL ret = g_pServerImpl->DoCleanup();
		g_pServerImpl.reset();
		return ret;
	}
	return NET_TRUE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_SetLogToFile 定义的内部处理。
 * @param [in] dwLogLevel 函数处理参数。
 * @param [in,out] strLogDir 函数处理参数。
 * @param [in] dwLogFileSize 函数处理参数。
 * @param [in] dwLogFileNum 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_SetLogToFile(NET_IN INT32 dwLogLevel,NET_IN CHAR  *strLogDir,NET_IN INT32 dwLogFileSize,NET_IN INT32 dwLogFileNum)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoSetLogToFile(dwLogLevel, strLogDir, dwLogFileSize, dwLogFileNum);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_GetSDKVersion 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

NET_API INT32 NET_STDCALL NET_SERVER_GetSDKVersion(void)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoGetSDKVersion();
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_GetClientCount 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

NET_API INT32 NET_STDCALL NET_SERVER_GetClientCount(void)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoGetClientCount();
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_SetUserPasswd 定义的内部处理。
 * @param [in] szUserName 函数处理参数。
 * @param [in] szPassword 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_SetUserPasswd(NET_IN CHAR szUserName[NET_LEN_132],NET_IN CHAR szPassword[NET_LEN_132])
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoSetUserPasswd(szUserName, szPassword);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_PushAlarmInfo 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_PushAlarmInfo(NET_IN NET_Alarmer_S *pAlarmer,
                                                    NET_IN INT32 lCommand,
                                                    NET_IN LPVOID pAlarmInfo,
                                                    NET_IN INT32 dwBufLen)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoPushAlarmInfo(pAlarmer, lCommand, pAlarmInfo, dwBufLen);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_PushChannelStatusInfo 定义的内部处理。
 * @param [in,out] pChannelInfo 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_PushChannelStatusInfo(NET_IN NET_ChannelInfo_S *pChannelInfo)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoPushChannelStatusInfo(pChannelInfo);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetDiscoveryDeviceInfo 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL
NET_SERVER_RegisterCb_GetDiscoveryDeviceInfo(
    NET_IN NET_CB_GetDiscoveryDeviceInfo cbFunc)
{
	if (!g_pServerImpl) {
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoRegisterCb_GetDiscoveryDeviceInfo(cbFunc);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_Discovery_Start 定义的内部处理。
 * @param [in] szInterfaceName 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL
NET_SERVER_Discovery_Start(NET_IN const CHAR* szInterfaceName)
{
	if (!g_pServerImpl) {
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoDiscoveryStart(szInterfaceName);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_Discovery_Stop 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL
NET_SERVER_Discovery_Stop(void)
{
	if (!g_pServerImpl) {
		return NET_FALSE;
	}
	return g_pServerImpl->DoDiscoveryStop();
}

/* ==================== 语音对讲 VoiceCom (服务端) ==================== */

NET_API BOOL NET_STDCALL
NET_SERVER_StartVoiceComServer(NET_IN UINT32 dwPort)
{
	return tvsdk::CVoiceComServer::instance()->start(static_cast<int>(dwPort)) ? NET_TRUE : NET_FALSE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_StopVoiceComServer 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL
NET_SERVER_StopVoiceComServer(void)
{
	tvsdk::CVoiceComServer::instance()->stop();
	return NET_TRUE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_VoiceComPlay 定义的内部处理。
 * @param [in] cb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL
NET_SERVER_RegisterCb_VoiceComPlay(NET_IN NET_SERVER_VoiceComPlayCallBack cb)
{
	if (!cb) {
		tvsdk::CVoiceComServer::instance()->set_play_callback(nullptr);
		return NET_TRUE;
	}

	tvsdk::CVoiceComServer::instance()->set_play_callback(
		[cb](const char* data, size_t size) {
			cb(data, static_cast<unsigned int>(size));
		});
	return NET_TRUE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_VoiceComCapture 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL
NET_SERVER_RegisterCb_VoiceComCapture(NET_IN NET_SERVER_VoiceComCaptureCallBack cb,
										 NET_IN LPVOID lpUserData)
{
	if (!cb) {
		tvsdk::CVoiceComServer::instance()->set_capture_callback(nullptr);
		return NET_TRUE;
	}

	tvsdk::CVoiceComServer::instance()->set_capture_callback(
		[cb, lpUserData](const NET_VoiceComAudioParam_S& audioParam,
						 char* buffer,
						 size_t bufferSize) -> int {
			return cb(&audioParam,
					  buffer,
					  static_cast<UINT32>(bufferSize),
					  lpUserData);
		});
	return NET_TRUE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_SendVoiceComData 定义的内部处理。
 * @param [in] pData 函数处理参数。
 * @param [in] dwSize 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL
NET_SERVER_SendVoiceComData(NET_IN const CHAR* pData, NET_IN UINT32 dwSize)
{
	if (!pData || dwSize == 0) return NET_FALSE;

	return tvsdk::CVoiceComServer::instance()->send_to_client(pData, dwSize) ? NET_TRUE : NET_FALSE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_GetVoiceComAudioParam 定义的内部处理。
 * @param [out] pstAudioParam 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL
NET_SERVER_GetVoiceComAudioParam(NET_OUT pNET_VoiceComAudioParam_S pstAudioParam)
{
	if (!pstAudioParam) return NET_FALSE;

	return tvsdk::CVoiceComServer::instance()->get_audio_param(*pstAudioParam) ? NET_TRUE : NET_FALSE;
}

/* ==================== 录像帧流 RecordFrame (服务端) ==================== */

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 启动录像帧流 TCP 服务
 * @details 创建监听socket，开始接收客户端连接
 * @param [in] dwPort TCP监听端口，建议使用9005
 * @return NET_TRUE 成功，NET_FALSE 失败
 * @note 服务端必须先启动此服务，客户端才能建立TCP连接接收帧数据
 */
NET_API BOOL NET_STDCALL
NET_SERVER_StartRecordFrameServer(NET_IN UINT32 dwPort)
{
	return tvsdk::CRecordFrameServer::instance()->start(static_cast<int>(dwPort)) ? NET_TRUE : NET_FALSE;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 停止录像帧流 TCP 服务
 * @details 关闭监听socket，停止所有客户端连接，释放资源
 * @return NET_TRUE 成功
 */
NET_API BOOL NET_STDCALL
NET_SERVER_StopRecordFrameServer(void)
{
	tvsdk::CRecordFrameServer::instance()->stop();
	return NET_TRUE;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册录像帧流启动回调
 * @details 将宿主程序实现的C风格回调转换为C++ Lambda，传递给RecordFrameServer
 *          Lambda捕获cb和lpUserData，在回调执行时恢复C风格函数调用
 * @param [in] cb 宿主实现的启动回调函数指针，NULL表示取消注册
 * @param [in] lpUserData 用户自定义数据，SDK会原样传回给宿主
 * @return NET_TRUE 成功
 * @note 为什么要这样写？
 *       1. 宿主程序使用C语言，回调是C风格函数指针，带有lpUserData参数
 *       2. SDK内部使用C++，RecordFrameServer期望的是C++风格的std::function
 *       3. 需要用Lambda做适配器，将C风格回调转换为C++风格回调
 *       4. cond参数做了拷贝（condCopy），因为C回调期望指针，防止引用失效
 */
NET_API BOOL NET_STDCALL
NET_SERVER_RegisterCb_RecordFrameStart(NET_IN NET_SERVER_RecordFrameStartCallBack cb,
										 NET_IN LPVOID lpUserData)
{
	if (!cb) {
		/* 取消注册，设置为空回调 */
		tvsdk::CRecordFrameServer::instance()->set_start_callback(nullptr);
		return NET_TRUE;
	}

	/* 使用Lambda做适配器：C风格回调 → C++风格回调 */
	tvsdk::CRecordFrameServer::instance()->set_start_callback(
		[cb, lpUserData](const NET_RecordFrameStreamCond_S& cond,
						 NET_RecordFrameStreamInfo_S& info) -> NET_COMMON_ECODE_E {
			/* cond参数做拷贝，因为C回调期望指针，防止引用失效 */
			NET_RecordFrameStreamCond_S condCopy = cond;
			/* 调用宿主注册的C风格回调，传入lpUserData */
			return cb(&condCopy, &info, lpUserData);
		});
	return NET_TRUE;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册录像帧读取回调
 * @details 将宿主程序实现的C风格回调转换为C++ Lambda，传递给RecordFrameServer
 *          Lambda捕获cb和lpUserData，在回调执行时恢复C风格函数调用
 * @param [in] cb 宿主实现的读取回调函数指针，NULL表示取消注册
 * @param [in] lpUserData 用户自定义数据，SDK会原样传回给宿主
 * @return NET_TRUE 成功
 * @note 为什么要这样写？
 *       1. C回调参数：const CHAR* szStreamId, UINT32 dwBufferSize
 *       2. C++回调参数：const std::string& streamId, size_t bufferSize
 *       3. 需要做类型转换：std::string → const char*, size_t → UINT32
 *       4. Read回调会被持续循环调用，Lambda捕获保证cb和lpUserData不会失效
 */
NET_API BOOL NET_STDCALL
NET_SERVER_RegisterCb_RecordFrameRead(NET_IN NET_SERVER_RecordFrameReadCallBack cb,
										NET_IN LPVOID lpUserData)
{
	if (!cb) {
		tvsdk::CRecordFrameServer::instance()->set_read_callback(nullptr);
		return NET_TRUE;
	}

	tvsdk::CRecordFrameServer::instance()->set_read_callback(
		[cb, lpUserData](const std::string& streamId,
						 NET_RecordFrameInfo_S& frameInfo,
						 char* buffer,
						 size_t bufferSize) -> int {
			/* 类型转换：std::string → const char*, size_t → UINT32 */
			return cb(streamId.c_str(), &frameInfo, buffer, static_cast<UINT32>(bufferSize), lpUserData);
		});
	return NET_TRUE;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册录像帧流停止回调
 * @details 将宿主程序实现的C风格回调转换为C++ Lambda，传递给RecordFrameServer
 *          Lambda捕获cb和lpUserData，在回调执行时恢复C风格函数调用
 * @param [in] cb 宿主实现的停止回调函数指针，NULL表示取消注册
 * @param [in] lpUserData 用户自定义数据，SDK会原样传回给宿主
 * @return NET_TRUE 成功
 * @note 为什么要这样写？
 *       1. C回调参数：const CHAR* szStreamId
 *       2. C++回调参数：const std::string& streamId
 *       3. 需要做类型转换：std::string → const char*
 */
NET_API BOOL NET_STDCALL
NET_SERVER_RegisterCb_RecordFrameStop(NET_IN NET_SERVER_RecordFrameStopCallBack cb,
										NET_IN LPVOID lpUserData)
{
	if (!cb) {
		tvsdk::CRecordFrameServer::instance()->set_stop_callback(nullptr);
		return NET_TRUE;
	}

	tvsdk::CRecordFrameServer::instance()->set_stop_callback(
		[cb, lpUserData](const std::string& streamId) -> NET_COMMON_ECODE_E {
			return cb(streamId.c_str(), lpUserData);
		});
	return NET_TRUE;
}


#ifdef __cplusplus
}
#endif
