

#include<stdio.h>
#include <stdint.h> 
#include <memory>

#include "NetTVSDKServerInterface.h"
#include "NetTVSDKServerImpl.h"

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


#ifdef __cplusplus
}
#endif
