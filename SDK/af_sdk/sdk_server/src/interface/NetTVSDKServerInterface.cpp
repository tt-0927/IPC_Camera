

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

NET_TV_API BOOL STDCALL NET_TV_SERVER_Init(IN UINT32 udwPort,IN CHAR szUserName[NET_TV_LEN_132],IN CHAR szPassword[NET_TV_LEN_132])
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoInit(udwPort, szUserName, szPassword);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_Cleanup(void)
{
	if (g_pServerImpl)
	{
		BOOL ret = g_pServerImpl->DoCleanup();
		g_pServerImpl.reset();
		return ret;
	}
	return TRUE;
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_SetLogToFile(IN INT32 dwLogLevel,IN CHAR  *strLogDir,IN INT32 dwLogFileSize,IN INT32 dwLogFileNum)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoSetLogToFile(dwLogLevel, strLogDir, dwLogFileSize, dwLogFileNum);
}

NET_TV_API INT32 STDCALL NET_TV_SERVER_GetSDKVersion(void)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoGetSDKVersion();
}

NET_TV_API INT32 STDCALL NET_TV_SERVER_GetClientCount(void)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoGetClientCount();
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_SetUserPasswd(IN CHAR szUserName[NET_TV_LEN_132],IN CHAR szPassword[NET_TV_LEN_132])
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoSetUserPasswd(szUserName, szPassword);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_PushAlarmInfo(IN NET_TV_ALARMER_S *pAlarmer,
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

NET_TV_API BOOL STDCALL NET_TV_SERVER_PushChannelStatusInfo(IN NET_TV_CHANNEL_INFO_S *pChannelInfo)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoPushChannelStatusInfo(pChannelInfo);
}

NET_TV_API BOOL STDCALL
NET_TV_SERVER_RegisterCb_GetDiscoveryDeviceInfo(
    IN NET_TV_CB_GetDiscoveryDeviceInfo cbFunc)
{
	if (!g_pServerImpl) {
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoRegisterCb_GetDiscoveryDeviceInfo(cbFunc);
}

NET_TV_API BOOL STDCALL
NET_TV_SERVER_Discovery_Start(IN const CHAR* szInterfaceName)
{
	if (!g_pServerImpl) {
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoDiscoveryStart(szInterfaceName);
}

NET_TV_API BOOL STDCALL
NET_TV_SERVER_Discovery_Stop(void)
{
	if (!g_pServerImpl) {
		return FALSE;
	}
	return g_pServerImpl->DoDiscoveryStop();
}

/* ==================== 语音对讲 VoiceCom (服务端) ==================== */

NET_TV_API BOOL STDCALL
NET_TV_SERVER_StartVoiceComServer(IN UINT32 dwPort)
{
	return tvsdk::VoiceComServer::instance()->start(static_cast<int>(dwPort)) ? TRUE : FALSE;
}

NET_TV_API BOOL STDCALL
NET_TV_SERVER_StopVoiceComServer(void)
{
	tvsdk::VoiceComServer::instance()->stop();
	return TRUE;
}

NET_TV_API BOOL STDCALL
NET_TV_SERVER_RegisterCb_VoiceComPlay(IN NET_TV_SERVER_VoiceComPlayCallBack cb)
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

NET_TV_API BOOL STDCALL
NET_TV_SERVER_RegisterCb_VoiceComCapture(IN NET_TV_SERVER_VoiceComCaptureCallBack cb,
										 IN LPVOID lpUserData)
{
	if (!cb) {
		tvsdk::VoiceComServer::instance()->set_capture_callback(nullptr);
		return TRUE;
	}

	tvsdk::VoiceComServer::instance()->set_capture_callback(
		[cb, lpUserData](const NET_TV_VOICECOM_AUDIO_PARAM_S& audioParam,
						 char* buffer,
						 size_t bufferSize) -> int {
			return cb(&audioParam,
					  buffer,
					  static_cast<UINT32>(bufferSize),
					  lpUserData);
		});
	return TRUE;
}

NET_TV_API BOOL STDCALL
NET_TV_SERVER_SendVoiceComData(IN const CHAR* pData, IN UINT32 dwSize)
{
	if (!pData || dwSize == 0) return FALSE;

	return tvsdk::VoiceComServer::instance()->send_to_client(pData, dwSize) ? TRUE : FALSE;
}

NET_TV_API BOOL STDCALL
NET_TV_SERVER_GetVoiceComAudioParam(OUT LPNET_TV_VOICECOM_AUDIO_PARAM_S pstAudioParam)
{
	if (!pstAudioParam) return FALSE;

	return tvsdk::VoiceComServer::instance()->get_audio_param(*pstAudioParam) ? TRUE : FALSE;
}

/* ==================== 录像帧流 RecordFrame (服务端) ==================== */

NET_TV_API BOOL STDCALL
NET_TV_SERVER_StartRecordFrameServer(IN UINT32 dwPort)
{
	return tvsdk::RecordFrameServer::instance()->start(static_cast<int>(dwPort)) ? TRUE : FALSE;
}

NET_TV_API BOOL STDCALL
NET_TV_SERVER_StopRecordFrameServer(void)
{
	tvsdk::RecordFrameServer::instance()->stop();
	return TRUE;
}

NET_TV_API BOOL STDCALL
NET_TV_SERVER_RegisterCb_RecordFrameStart(IN NET_TV_SERVER_RecordFrameStartCallBack cb,
										 IN LPVOID lpUserData)
{
	if (!cb) {
		tvsdk::RecordFrameServer::instance()->set_start_callback(nullptr);
		return TRUE;
	}

	tvsdk::RecordFrameServer::instance()->set_start_callback(
		[cb, lpUserData](const NET_TV_RECORD_FRAME_STREAM_COND_S& cond,
						 NET_TV_RECORD_FRAME_STREAM_INFO_S& info) -> NET_TV_COMMON_ECODE_E {
			NET_TV_RECORD_FRAME_STREAM_COND_S condCopy = cond;
			return cb(&condCopy, &info, lpUserData);
		});
	return TRUE;
}

NET_TV_API BOOL STDCALL
NET_TV_SERVER_RegisterCb_RecordFrameRead(IN NET_TV_SERVER_RecordFrameReadCallBack cb,
										IN LPVOID lpUserData)
{
	if (!cb) {
		tvsdk::RecordFrameServer::instance()->set_read_callback(nullptr);
		return TRUE;
	}

	tvsdk::RecordFrameServer::instance()->set_read_callback(
		[cb, lpUserData](const std::string& streamId,
						 NET_TV_RECORD_FRAME_INFO_S& frameInfo,
						 char* buffer,
						 size_t bufferSize) -> int {
			return cb(streamId.c_str(), &frameInfo, buffer, static_cast<UINT32>(bufferSize), lpUserData);
		});
	return TRUE;
}

NET_TV_API BOOL STDCALL
NET_TV_SERVER_RegisterCb_RecordFrameStop(IN NET_TV_SERVER_RecordFrameStopCallBack cb,
										IN LPVOID lpUserData)
{
	if (!cb) {
		tvsdk::RecordFrameServer::instance()->set_stop_callback(nullptr);
		return TRUE;
	}

	tvsdk::RecordFrameServer::instance()->set_stop_callback(
		[cb, lpUserData](const std::string& streamId) -> NET_TV_COMMON_ECODE_E {
			return cb(streamId.c_str(), lpUserData);
		});
	return TRUE;
}

#ifdef __cplusplus
}
#endif
