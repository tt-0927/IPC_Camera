/*
 * @FilePath     : sdk_new/sdk_server/src/interface/BG6_ZHSJ/BU_SJCL/NetTVVoiceComInterface.cpp
 * @Author       : ITC
 * @Date         : 2026-08-19
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-19
 * @Description  : 语音对讲服务端 C 接口实现（BU_SJCL / NVR 侧）
 *                 从 Common/NetTVSDKServerInterface.cpp 迁出，独立业务不经 PIMPL，
 *                 直接调用 CVoiceComServer 单例。
 *                 对外声明仍在 NetTVSDKServerInterface.h（共用头文件）。
 */

#include "NetTVSDKServerInterface.h"
#include "VoiceComServer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 语音对讲 VoiceCom (服务端) ==================== */

NET_API BOOL STDCALL
NET_serverStartVoiceComServer(IN UINT32 uPort)
{
	return tvsdk::CVoiceComServer::instance()->start(static_cast<int>(uPort)) ? TRUE : FALSE;
}

NET_API BOOL STDCALL
NET_serverStopVoiceComServer(void)
{
	tvsdk::CVoiceComServer::instance()->stop();
	return TRUE;
}

NET_API BOOL STDCALL
NET_serverRegisterVoiceComPlayCb(IN NET_serverVoiceComPlayCallBack cb)
{
	if (!cb) {
		tvsdk::CVoiceComServer::instance()->set_play_callback(nullptr);
		return TRUE;
	}

	tvsdk::CVoiceComServer::instance()->set_play_callback(
		[cb](const char* pstrData, size_t uSize) {
			cb(pstrData, static_cast<unsigned int>(uSize));
		});
	return TRUE;
}

NET_API BOOL STDCALL
NET_serverRegisterVoiceComCaptureCb(IN NET_serverVoiceComCaptureCallBack cb,
										 IN LPVOID lpUserData)
{
	if (!cb) {
		tvsdk::CVoiceComServer::instance()->set_capture_callback(nullptr);
		return TRUE;
	}

	tvsdk::CVoiceComServer::instance()->set_capture_callback(
		[cb, lpUserData](const NET_VoiceComAudioParam_S& stAudioParam,
						 char* pstrBuffer,
						 size_t uBufferSize) -> int {
			return cb(&stAudioParam,
					  pstrBuffer,
					  static_cast<UINT32>(uBufferSize),
					  lpUserData);
		});
	return TRUE;
}

NET_API BOOL STDCALL
NET_serverSendVoiceComData(IN const CHAR* pstrData, IN UINT32 uSize)
{
	if (!pstrData || uSize == 0) return FALSE;

	return tvsdk::CVoiceComServer::instance()->send_to_client(pstrData, uSize) ? TRUE : FALSE;
}

NET_API BOOL STDCALL
NET_serverGetVoiceComAudioParam(OUT pNET_VoiceComAudioParam_S pstAudioParam)
{
	if (!pstAudioParam) return FALSE;

	return tvsdk::CVoiceComServer::instance()->get_audio_param(*pstAudioParam) ? TRUE : FALSE;
}

#ifdef __cplusplus
}
#endif
