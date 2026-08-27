

#include<stdio.h>
#include <stdint.h>
#include <memory>

#include "NetTVSDKServerInterface.h"
#include "NetTVSDKServerImpl.h"

/* VoiceCom/RecordFrame 的 C 接口实现已迁出至 BG6_ZHSJ/BU_SJCL/ 目录：
 *   NetTVVoiceComInterface.cpp      ← 语音对讲（直接调 CVoiceComServer 单例）
 *   NetTVRecordFrameInterface.cpp   ← 录像帧流（直接调 CRecordFrameServer 单例）
 * 它们是 BU_SJCL 独立业务，不经 PIMPL，对外声明仍在本头文件中。
 */

// 全局Impl单例（使用智能指针）
static std::unique_ptr<CNetTVSDKServerImpl> g_pServerImpl;

#ifdef __cplusplus
extern "C" {
#endif

NET_API BOOL STDCALL NET_serverInit(IN UINT32 udwPort,IN CHAR szUserName[NET_LEN_132],IN CHAR szPassword[NET_LEN_132],IN CHAR szDeviceName[NET_LEN_132])
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoInit(udwPort, szUserName, szPassword, szDeviceName);
}

NET_API BOOL STDCALL NET_serverCleanup(void)
{
	if (g_pServerImpl)
	{
		BOOL ret = g_pServerImpl->DoCleanup();
		g_pServerImpl.reset();
		return ret;
	}
	return TRUE;
}

NET_API BOOL STDCALL NET_serverSetLogToFile(IN INT32 dwLogLevel,IN CHAR  *strLogDir,IN INT32 nLogFileSize,IN INT32 dwLogFileNum)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoSetLogToFile(dwLogLevel, strLogDir, nLogFileSize, dwLogFileNum);
}

NET_API INT32 STDCALL NET_serverGetSdkVersion(void)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoGetSDKVersion();
}

NET_API INT32 STDCALL NET_serverGetClientCount(void)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoGetClientCount();
}

NET_API BOOL STDCALL NET_serverSetUserPassword(IN CHAR szUserName[NET_LEN_132],IN CHAR szPassword[NET_LEN_132])
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoSetUserPasswd(szUserName, szPassword);
}

NET_API BOOL STDCALL NET_serverPushAlarmInfo(IN NET_Alarmer_S *pAlarmer,
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

NET_API BOOL STDCALL NET_serverPushChannelStatusInfo(IN NET_ChannelInfo_S *pChannelInfo)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoPushChannelStatusInfo(pChannelInfo);
}

NET_API BOOL STDCALL
NET_serverRegisterGetDiscoveryDeviceInfoCb(
    IN NET_CB_GetDiscoveryDeviceInfo cbFunc)
{
	if (!g_pServerImpl) {
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoRegisterCb_GetDiscoveryDeviceInfo(cbFunc);
}

NET_API BOOL STDCALL
NET_serverStartDiscovery(IN const CHAR* szInterfaceName)
{
	if (!g_pServerImpl) {
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoDiscoveryStart(szInterfaceName);
}

NET_API BOOL STDCALL
NET_serverStopDiscovery(void)
{
	if (!g_pServerImpl) {
		return FALSE;
	}
	return g_pServerImpl->DoDiscoveryStop();
}


#ifdef __cplusplus
}
#endif
