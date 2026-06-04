#include <stdio.h>
#include <stdint.h> 
#include <cctype>
#include <iomanip>
#include <sstream>

#include "NetTVSDKClientInterface.h"
#include "NetTVSDKHttpUrl.h"
#include "CommandExecutor.h"
#include "ErrorManage.h"
#include "DeviceManage.h"
#include "NetSdkLog.h"

#define NETTVSDK_MAKE_VERSION(major, minor, rev1, rev2) \
    ((uint32_t)( \
        ((major & 0xFF) << 24) |  /* 主版本左移24位（占高8位） */ \
        ((minor & 0xFF) << 16) |  /* 次版本左移16位（占次高8位） */ \
        ((rev1 & 0xFF) << 8) |    /* 附加版本1左移8位（占第三8位） */ \
        (rev2 & 0xFF)             /* 附加版本2占最低8位 */ \
    ))

#define NETTVSDK_GET_MAJOR(version)    ((uint8_t)((version >> 24) & 0xFF))  // 提取主版本
#define NETTVSDK_GET_MINOR(version)    ((uint8_t)((version >> 16) & 0xFF))  // 提取次版本
#define NETTVSDK_GET_REV1(version)     ((uint8_t)((version >> 8) & 0xFF))   // 提取附加版本1
#define NETTVSDK_GET_REV2(version)     ((uint8_t)(version & 0xFF))          // 提取附加版本2

#define NETTVSDK_VERSION        NETTVSDK_MAKE_VERSION(1, 0, 0, 0)			/* 当前版本 */

/* 检查SDK是否初始化 */
#define CHECK_SDK_INIT(val) \
    do { \
        auto* pMgr = CDeviceManage::instance(); \
        /* 检查单例是否存在 以及 标志位是否为 true */ \
        if (!pMgr || !pMgr->IsInitialized()) { \
            CErrorManage::instance()->SetLastError(NET_TV_E_SDK_NOT_INIT); \
            return val; \
        } \
    } while(0)

/* 检查SDK是否已经初始化 */
#define CHECK_SDK_ALREADY_INIT(val) \
    do { \
        auto* pMgr = CDeviceManage::instance(); \
        /* 如果单例存在 且 标志位为 true，说明已经 Init 过了 */ \
        if (pMgr && pMgr->IsInitialized()) { \
            CErrorManage::instance()->SetLastError(NET_TV_E_ALREDY_INIT_ERROR); \
            return val; \
        } \
    } while(0)

/* 全局错误码 */
thread_local int CErrorManage::lastErrorCode_ = NET_TV_E_SDK_NOT_INIT;

NET_TV_API BOOL STDCALL NET_TV_Init(void)
{
	CHECK_SDK_ALREADY_INIT(FALSE);
	try 
	{
        auto* pDevMgr = CDeviceManage::instance();
        
        if (!pDevMgr) 
		{
            return FALSE;
        }
		pDevMgr->SetInitialized(true);
        CErrorManage::instance()->SetLastError(NET_TV_E_SUCCEED);
        return TRUE;
    } 
    catch (...) 
	{
        return FALSE;
    }

}

NET_TV_API BOOL STDCALL NET_TV_Cleanup(void)
{
	
	CHECK_SDK_INIT(FALSE);

	try 
	{
        auto* pDevMgr = CDeviceManage::instance();

        if (pDevMgr) 
		{
            pDevMgr->Cleanup(); 
        }
		pDevMgr->SetInitialized(false);
        CDeviceManage::DestroyInstance();

        CErrorManage::instance()->SetLastError(NET_TV_E_SDK_NOT_INIT);
        
        return TRUE;
    } 
    catch (...) 
	{
        return FALSE;
    }


	return FALSE;
}

NET_TV_API BOOL STDCALL NET_TV_SetLogToFile(IN INT32 dwLogLevel,IN CHAR  *strLogDir,IN INT32 dwLogFileSize,IN INT32 dwLogFileNum)
{
    if (strLogDir == NULL)
    {
        return FALSE;
    }

    /* 构造完整日志路径 */
    char szLogPath[512] = {0};
#ifdef _WIN32
    sprintf(szLogPath, "%s\\NetTVSDKClient.log", strLogDir);
#else
    sprintf(szLogPath, "%s/NetTVSDKClient.log", strLogDir);
#endif

    if (dwLogFileSize <= 0) 
    {
        dwLogFileSize = 5 * 1024 * 1024; // Default 5MB
    }
    
    if (dwLogFileNum <= 0)
    {
        dwLogFileNum = 10; // Default 10 files
    }

	/* 初始化日志 */
    // 使用 "NetTVSDKClient" 作为 logger 名称
    if (initSdkLogBySize("NetTVSDKClient", szLogPath, dwLogFileSize, dwLogFileNum) != 0)
    {
        return FALSE;
    }
    
    /* 设置日志输出同步输出控制台 */
	syncPrintf(true);

    /* 设置日志等级 */
	setLogLevel(dwLogLevel);

	return TRUE;
}

NET_TV_API INT32 STDCALL NET_TV_GetSDKVersion(void)
{

	return NETTVSDK_VERSION;
}

NET_TV_API INT32 STDCALL NET_TV_GetLastError()
{
	return CErrorManage::instance()->GetLastError();
}

NET_TV_API BOOL STDCALL NET_TV_SetExceptionCallBack(IN NET_TV_ExceptionCallBack_PF cbExceptionCallBack,
                                                                 IN LPVOID lpUserData)
{
	return FALSE;
}																 

NET_TV_API BOOL STDCALL NET_TV_SetRevTimeOut(IN LPNET_TV_REV_TIMEOUT_S pstRevTimeout)
{
	CHECK_SDK_INIT(FALSE);

    if (!pstRevTimeout)
	{
		CErrorManage::instance()->SetLastError(NET_TV_E_INVALID_PARAM);
		return FALSE;
	} 

	auto* pDevMgr = CDeviceManage::instance();

	if (!pDevMgr) 
	{
		CErrorManage::instance()->SetLastError(NET_TV_E_ALLOC_RESOURCE_ERROR);
		return FALSE;
	}

    pDevMgr->SetGlobalRevTimeout(pstRevTimeout->dwRevTimeOut); 
	CErrorManage::instance()->SetLastError(NET_TV_E_SUCCEED);
	return FALSE;
}

NET_TV_API BOOL STDCALL NET_TV_SetConnectTime(IN INT32 dwWaitTime,
                                                           IN INT32 dwTrytimes)
{
	CHECK_SDK_INIT(FALSE);
    auto* pDevMgr = CDeviceManage::instance();

	if (!pDevMgr) 
	{
		CErrorManage::instance()->SetLastError(NET_TV_E_ALLOC_RESOURCE_ERROR);
		return FALSE;
	}

	pDevMgr->SetGlobalConnectTime(dwWaitTime, dwTrytimes);
	CErrorManage::instance()->SetLastError(NET_TV_E_SUCCEED);
	return FALSE;
}

NET_TV_API LPVOID STDCALL NET_TV_Login(IN LPNET_TV_DEVICE_LOGIN_INFO_S pstDevLoginInfo, 
                                                        OUT LPNET_TV_DEVICE_INFO_S pstDevInfo)
{
	CHECK_SDK_INIT(NULL);
	auto* pDevMgr = CDeviceManage::instance();
	if (!pDevMgr) 
	{
		CErrorManage::instance()->SetLastError(NET_TV_E_ALLOC_RESOURCE_ERROR);
		return NULL;
	}
	LPVOID lpUserID = pDevMgr->Login(pstDevLoginInfo->szIPAddr, pstDevLoginInfo->dwPort, pstDevLoginInfo->szUserName, pstDevLoginInfo->szPassword);

	/* 发送获取设备信息命令 */
	if(lpUserID != NULL)
	{
		if(!CommandExecutor::instance()->ExecuteGet<NET_TV_DEVICE_INFO_S>(lpUserID,TVAPI_PATH_DEVICE_GETINFO,pstDevInfo,NULL))
		{
			return NULL;
		}
	}

	return lpUserID;
}

NET_TV_API BOOL STDCALL NET_TV_Logout(IN LPVOID lpUserID)
{
	CHECK_SDK_INIT(FALSE);

	auto* pDevMgr = CDeviceManage::instance();

	if (!pDevMgr) 
	{
		CErrorManage::instance()->SetLastError(NET_TV_E_ALLOC_RESOURCE_ERROR);
		return FALSE;
	}

	if(!pDevMgr->Logout(lpUserID))
	{
		CErrorManage::instance()->SetLastError(NET_TV_E_NO_USER);
		return FALSE;
	}

	CErrorManage::instance()->SetLastError(NET_TV_E_SUCCEED);
	return TRUE;
}

NET_TV_API BOOL STDCALL NET_TV_SetAlarmCallBack(IN LPVOID lpUserID,
                                            IN NET_TV_AlarmCallBack cbAlarmMessCallBack,
                                            IN LPVOID lpUserData)
{
    CHECK_SDK_INIT(FALSE);
    auto* pDevMgr = CDeviceManage::instance();
    if (!pDevMgr) return FALSE;
    
    auto session = pDevMgr->GetSession((LPUSER_HANDLE)lpUserID);
    if (!session) {
        CErrorManage::instance()->SetLastError(NET_TV_E_NO_USER);
        return FALSE;
    }
    
    session->SetAlarmCallback(cbAlarmMessCallBack, lpUserData);
    CErrorManage::instance()->SetLastError(NET_TV_E_SUCCEED);
    return TRUE;
}

NET_TV_API BOOL STDCALL NET_TV_SetChannelStatusCallBack(IN LPVOID lpUserID,
                                                        IN NET_TV_ChannelStatusCallBack cbChannelStatusCallBack,
                                                        IN LPVOID lpUserData)
{
    CHECK_SDK_INIT(FALSE);
    auto* pDevMgr = CDeviceManage::instance();
    if (!pDevMgr) return FALSE;

    auto session = pDevMgr->GetSession((LPUSER_HANDLE)lpUserID);
    if (!session)
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_NO_USER);
        return FALSE;
    }

    session->SetChannelStatusCallback(cbChannelStatusCallBack, lpUserData);
    CErrorManage::instance()->SetLastError(NET_TV_E_SUCCEED);
    return TRUE;
}

NET_TV_API BOOL STDCALL NET_TV_StartListen(IN LPVOID lpUserID)
{
    CHECK_SDK_INIT(FALSE);
    auto* pDevMgr = CDeviceManage::instance();
    if (!pDevMgr) return FALSE;
    
    auto session = pDevMgr->GetSession((LPUSER_HANDLE)lpUserID);
    if (!session) 
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_NO_USER);
        return FALSE;
    }
    
    if (!session->StartAlarmListen()) 
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_FAILED);
        return FALSE;
    }
    
    CErrorManage::instance()->SetLastError(NET_TV_E_SUCCEED);
    return TRUE;
}

NET_TV_API BOOL STDCALL NET_TV_StopListen(IN LPVOID lpUserID)
{
    CHECK_SDK_INIT(FALSE);
    auto* pDevMgr = CDeviceManage::instance();
    if (!pDevMgr) return FALSE;
    
    auto session = pDevMgr->GetSession((LPUSER_HANDLE)lpUserID);
    if (!session) 
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_NO_USER);
        return FALSE;
    }
    
    if (!session->StopAlarmListen()) 
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_FAILED);
        return FALSE;
    }
    
    CErrorManage::instance()->SetLastError(NET_TV_E_SUCCEED);
    return TRUE;
}

#include "CapabilityInfoConvert.h"

NET_TV_API BOOL STDCALL NET_TV_GetDeviceCapability(IN LPVOID lpUserID,
                                                   IN INT32 dwChannelID,
                                                   IN INT32 dwCommand,
                                                   OUT LPVOID lpOutBuffer,
                                                   OUT INT32 dwOutBufferSize,
                                                   OUT INT32 *pdwBytesReturned)
{
    CHECK_SDK_INIT(FALSE);
    
    if (!lpOutBuffer || dwOutBufferSize <= 0)
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_INVALID_PARAM);
        return FALSE;
    }
    
    // 使用宏生成统一URL
    std::string url = TVAPI_URL_DEVICE_CAPABILITY(dwChannelID, dwCommand);
    
    switch (dwCommand)
    {
        case NET_TV_CAP_VIDEO_ENCODE:
        {
            if (dwOutBufferSize < (INT32)sizeof(NET_TV_VIDEO_ENCODE_CAP_S))
            {
                CErrorManage::instance()->SetLastError(NET_TV_E_NOENOUGH_BUF);
                return FALSE;
            }
            
            if (CommandExecutor::instance()->ExecuteGet<NET_TV_VIDEO_ENCODE_CAP_S>(
                    lpUserID, url, lpOutBuffer, pdwBytesReturned))
            {
                return TRUE;
            }
            return FALSE;
        }

        case NET_TV_CAP_AUDIO:
        {
            if (dwOutBufferSize < (INT32)sizeof(NET_TV_AUDIO_CAP_S))
            {
                CErrorManage::instance()->SetLastError(NET_TV_E_NOENOUGH_BUF);
                return FALSE;
            }
            
            if (CommandExecutor::instance()->ExecuteGet<NET_TV_AUDIO_CAP_S>(
                    lpUserID, url, lpOutBuffer, pdwBytesReturned))
            {
                
                return TRUE;
            }
            return FALSE;
        }
        
        case NET_TV_CAP_OSD:
        {
            if (dwOutBufferSize < (INT32)sizeof(NET_TV_OSD_CAP_S))
            {
                CErrorManage::instance()->SetLastError(NET_TV_E_NOENOUGH_BUF);
                return FALSE;
            }
            
            if (CommandExecutor::instance()->ExecuteGet<NET_TV_OSD_CAP_S>(
                    lpUserID, url, lpOutBuffer, pdwBytesReturned))
            {
                return TRUE;
            }
            return FALSE;
        }

        // 后续扩展其他能力集类型
        // case NET_TV_CAP_SMART:
        // case NET_TV_CAP_IMAGE:
        // case NET_TV_CAP_AUDIO:
        
        default:
            CErrorManage::instance()->SetLastError(NET_TV_E_CMD_NOT_SUPPORT);
            return FALSE;
    }
}

template <typename T_CFG>
static BOOL NetTV_GetDevConfig_Impl(LPVOID lpUserID,
                                    INT32 dwChannelID,
                                    INT32 dwCommand,
                                    LPVOID lpOutBuffer,
                                    INT32 dwOutBufferSize,
                                    INT32 *pdwBytesReturned)
{
    if (!lpOutBuffer || dwOutBufferSize <= 0)
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_INVALID_PARAM);
        return FALSE;
    }

    if (dwOutBufferSize < (INT32)sizeof(T_CFG))
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_NOENOUGH_BUF);
        return FALSE;
    }

    std::string url = TVAPI_URL_DEVICE_GET_DEV_CONFIG(dwChannelID, dwCommand);
    return CommandExecutor::instance()->ExecuteGet<T_CFG>(lpUserID, url, lpOutBuffer, pdwBytesReturned) ? TRUE : FALSE;
}

static std::string NetTV_UrlEncode(const char* value)
{
    if (!value)
    {
        return "";
    }

    std::ostringstream oss;
    oss << std::uppercase << std::hex;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(value);
    while (*p)
    {
        unsigned char ch = *p++;
        if (std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~')
        {
            oss << (char)ch;
        }
        else
        {
            oss << '%' << std::setw(2) << std::setfill('0') << (int)ch;
        }
    }
    return oss.str();
}

static BOOL NetTV_GetRecordFileList_Impl(LPVOID lpUserID,
                                         INT32 dwChannelID,
                                         INT32 dwCommand,
                                         LPVOID lpOutBuffer,
                                         INT32 dwOutBufferSize,
                                         INT32 *pdwBytesReturned)
{
    if (!lpOutBuffer || dwOutBufferSize <= 0)
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_INVALID_PARAM);
        return FALSE;
    }

    if (dwOutBufferSize < (INT32)sizeof(NET_TV_RECORD_FILE_LIST_S))
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_NOENOUGH_BUF);
        return FALSE;
    }

    NET_TV_RECORD_FILE_LIST_S* pCfg = static_cast<NET_TV_RECORD_FILE_LIST_S*>(lpOutBuffer);
    std::ostringstream url;
    url << TVAPI_URL_DEVICE_GET_DEV_CONFIG(dwChannelID, dwCommand)
        << "&ChnId=" << pCfg->stFind.nChnId
        << "&Type=" << pCfg->stFind.nType
        << "&Year=" << NetTV_UrlEncode(pCfg->stFind.szYear)
        << "&Month=" << NetTV_UrlEncode(pCfg->stFind.szMonth)
        << "&Date=" << NetTV_UrlEncode(pCfg->stFind.szDate)
        << "&StartTime=" << NetTV_UrlEncode(pCfg->stFind.szStartTime)
        << "&EndTime=" << NetTV_UrlEncode(pCfg->stFind.szEndTime)
        << "&Filename=" << NetTV_UrlEncode(pCfg->stFind.szFilename);

    return CommandExecutor::instance()->ExecuteGet<NET_TV_RECORD_FILE_LIST_S>(lpUserID, url.str(), lpOutBuffer, pdwBytesReturned) ? TRUE : FALSE;
}

static BOOL NetTV_GetLogList_Impl(LPVOID lpUserID,
                                  INT32 dwChannelID,
                                  INT32 dwCommand,
                                  LPVOID lpOutBuffer,
                                  INT32 dwOutBufferSize,
                                  INT32 *pdwBytesReturned)
{
    if (!lpOutBuffer || dwOutBufferSize <= 0)
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_INVALID_PARAM);
        return FALSE;
    }

    if (dwOutBufferSize < (INT32)sizeof(NET_TV_LOG_LIST_S))
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_NOENOUGH_BUF);
        return FALSE;
    }

    NET_TV_LOG_LIST_S* pCfg = static_cast<NET_TV_LOG_LIST_S*>(lpOutBuffer);
    INT32 nCurPage = pCfg->stPage.nCurPage == 0 ? 1 : pCfg->stPage.nCurPage;
    INT32 nPageSize = pCfg->stPage.nPageSize <= 0 ? NET_TV_LOG_QUERY_COND_NUM : pCfg->stPage.nPageSize;

    std::ostringstream url;
    url << TVAPI_URL_DEVICE_GET_DEV_CONFIG(dwChannelID, dwCommand)
        << "&Type=" << pCfg->stCond.nType
        << "&Action=" << pCfg->stCond.nAction
        << "&StartTime=" << NetTV_UrlEncode(pCfg->stCond.szStartTime)
        << "&EndTime=" << NetTV_UrlEncode(pCfg->stCond.szEndTime)
        << "&CurPage=" << nCurPage
        << "&PageSize=" << nPageSize;

    return CommandExecutor::instance()->ExecuteGet<NET_TV_LOG_LIST_S>(lpUserID, url.str(), lpOutBuffer, pdwBytesReturned) ? TRUE : FALSE;
}

NET_TV_API BOOL STDCALL NET_TV_ControlReplay(IN    LPVOID lpUserID,
                                             INOUT LPNET_TV_REPLAY_CTRL_INFO_S pstInfo,
                                             OUT   INT32 *pdwBytesReturned)
{
    CHECK_SDK_INIT(FALSE);

    if (!pstInfo)
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_INVALID_PARAM);
        return FALSE;
    }

    if (pstInfo->dwChannel <= 0)
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_INVALID_PARAM);
        return FALSE;
    }

    std::string body = SDKConvert::to_string(*pstInfo);
    std::string respBody;
    if (!CommandExecutor::instance()->ExecuteRaw(lpUserID, "POST", TVAPI_URL_REPLAY_CONTROL(), body, respBody))
    {
        return FALSE;
    }

    SDKConvert::to_respStruct(respBody, *pstInfo);
    if (pdwBytesReturned)
    {
        *pdwBytesReturned = sizeof(NET_TV_REPLAY_CTRL_INFO_S);
    }

    return TRUE;
}

NET_TV_API BOOL STDCALL NET_TV_GetReplayRecordList(IN    LPVOID lpUserID,
                                                   INOUT LPNET_TV_REPLAY_RECORD_LIST_S pstInfo,
                                                   OUT   INT32 *pdwBytesReturned)
{
    CHECK_SDK_INIT(FALSE);

    if (!pstInfo)
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_INVALID_PARAM);
        return FALSE;
    }

    if (pstInfo->dwChannel <= 0)
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_INVALID_PARAM);
        return FALSE;
    }

    std::string body = SDKConvert::to_string(*pstInfo);
    std::string respBody;
    if (!CommandExecutor::instance()->ExecuteRaw(lpUserID, "POST", TVAPI_URL_REPLAY_GET_RECORD_LIST(), body, respBody))
    {
        return FALSE;
    }

    SDKConvert::to_respStruct(respBody, *pstInfo);
    if (pdwBytesReturned)
    {
        *pdwBytesReturned = sizeof(NET_TV_REPLAY_RECORD_LIST_S);
    }

    return TRUE;
}

NET_TV_API BOOL STDCALL NET_TV_GetReplayUrl(IN    LPVOID lpUserID,
                                            INOUT LPNET_TV_REPLAY_URL_INFO_S pstInfo,
                                            OUT   INT32 *pdwBytesReturned)
{
    CHECK_SDK_INIT(FALSE);

    if (!pstInfo)
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_INVALID_PARAM);
        return FALSE;
    }

    if (pstInfo->dwChannel <= 0)
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_INVALID_PARAM);
        return FALSE;
    }

    std::string body = SDKConvert::to_string(*pstInfo);
    std::string respBody;
    if (!CommandExecutor::instance()->ExecuteRaw(lpUserID, "POST", TVAPI_URL_REPLAY_GET_URL(), body, respBody))
    {
        return FALSE;
    }

    SDKConvert::to_respStruct(respBody, *pstInfo);
    if (pdwBytesReturned)
    {
        *pdwBytesReturned = sizeof(NET_TV_REPLAY_URL_INFO_S);
    }

    return TRUE;
}

template <typename T_CFG>
static BOOL NetTV_SetDevConfig_Impl(LPVOID lpUserID,
                                    INT32 dwChannelID,
                                    INT32 dwCommand,
                                    LPVOID lpInBuffer,
                                    INT32 dwInBufferSize,
                                    INT32 *pdwBytesReturned)
{
    if (!lpInBuffer || dwInBufferSize <= 0)
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_INVALID_PARAM);
        return FALSE;
    }

    if (dwInBufferSize < (INT32)sizeof(T_CFG))
    {
        CErrorManage::instance()->SetLastError(NET_TV_E_INVALID_PARAM);
        return FALSE;
    }

    std::string url = TVAPI_URL_DEVICE_SET_DEV_CONFIG(dwChannelID, dwCommand);
    BOOL bRet = CommandExecutor::instance()->ExecuteSet<T_CFG>(lpUserID, "POST", url, lpInBuffer) ? TRUE : FALSE;
    if (bRet && pdwBytesReturned != NULL)
    {
        *pdwBytesReturned = 0;
    }
    return bRet;
}

NET_TV_API BOOL STDCALL NET_TV_GetDevConfig(IN  LPVOID  lpUserID,
                                            IN    INT32   dwChannelID,
                                            IN    INT32   dwCommand,
                                            INOUT LPVOID  lpOutBuffer,
                                            OUT   INT32   dwOutBufferSize,
                                            OUT   INT32   *pdwBytesReturned)
{
    CHECK_SDK_INIT(FALSE);

    switch (dwCommand)
    {
        case NET_TV_GET_DEVICECFG:
            return NetTV_GetDevConfig_Impl<NET_TV_DEVICE_BASICINFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_NTPCFG:
            return NetTV_GetDevConfig_Impl<NET_TV_ALARM_EXCEPTION_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_AUDIOCFG:
            return NetTV_GetDevConfig_Impl<NET_TV_AUDIO_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_STREAMCFG:
            return NetTV_GetDevConfig_Impl<NET_TV_VIDEO_ENCODE_OPTION_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_OSDCAPCFG:
            return NetTV_GetDevConfig_Impl<NET_TV_VIDEO_OSD_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_IMAGECFG:
            return NetTV_GetDevConfig_Impl<NET_TV_IMAGE_SETTING_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_RTSPURLCFG:
            return NetTV_GetDevConfig_Impl<NET_TV_RTSP_URL_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_NETWORKCFG:
            return NetTV_GetDevConfig_Impl<NET_TV_NETWORKCFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_4G_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_4G_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_HOTSPOT_CONN:
            return NetTV_GetDevConfig_Impl<NET_TV_HOTSPOT_CONN_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_SECURITY_SERVICES_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_SECURITY_SERVICES_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_SSH_COUNTDOWN:
            return NetTV_GetDevConfig_Impl<NET_TV_SSH_COUNTDOWN_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_FIND_LOG:
        case NET_TV_EXPORT_LOG:
            return NetTV_GetLogList_Impl(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_LOG_SERVER:
            return NetTV_GetDevConfig_Impl<NET_TV_LOG_SERVER_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_RECORD_STATUS:
            return NetTV_GetDevConfig_Impl<NET_TV_RECORD_STATUS_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_RECORD_SCHEDULE:
            return NetTV_GetDevConfig_Impl<NET_TV_RECORD_SCHEDULE_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_RECORD_ADVANCED_PARAM:
            return NetTV_GetDevConfig_Impl<NET_TV_RECORD_ADVANCED_PARAM_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_FIND_RECORD_FILE_INFO:
            return NetTV_GetRecordFileList_Impl(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_PRIVACYMASKCFG:
            printf("[ClientSDK] GET_PRIVACYMASKCFG cmd=%d, buf=%d, privacy_size=%zu\n",
                   dwCommand, dwOutBufferSize, sizeof(NET_TV_PRIVACY_MASK_CFG_S));
            return NetTV_GetDevConfig_Impl<NET_TV_PRIVACY_MASK_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_TAMPERALARM:
            return NetTV_GetDevConfig_Impl<NET_TV_TAMPER_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_MOTIONALARM:
            return NetTV_GetDevConfig_Impl<NET_TV_MOTION_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_CROSSLINEALARM:
            return NetTV_GetDevConfig_Impl<NET_TV_CROSS_LINE_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_INTRUSIONALARM:
            return NetTV_GetDevConfig_Impl<NET_TV_INTRUSION_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_ENTERREGIONALARM:
            return NetTV_GetDevConfig_Impl<NET_TV_ENTER_REGION_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_LEAVEREGIONALARM:
            return NetTV_GetDevConfig_Impl<NET_TV_LEAVE_REGION_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_LOITERINGALARM:
            return NetTV_GetDevConfig_Impl<NET_TV_LOITERING_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_SCENECHANGEALARM:
            return NetTV_GetDevConfig_Impl<NET_TV_SCENE_CHANGE_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_CROWDGATHERINGALARM:
            return NetTV_GetDevConfig_Impl<NET_TV_CROWD_GATHERING_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_GARBAGE_EXPOSURE_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_GARBAGE_EXPOSURE_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_GARBAGE_OVERFLOW_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_GARBAGE_OVERFLOW_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_PEOPLE_FLOW_STATISTICS_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_PEOPLE_DENSITY_DETECTION_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_MANHOLE_COVER_ABNORMAL_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_SLEEP_ON_DUTY_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_SLEEP_ON_DUTY_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_PERSON_FALL_DOWN_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_PERSON_FALL_DOWN_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_CONSTRUCTION_OCCUPY_ROAD_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_CONGESTION_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_CONGESTION_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_LICENSE_PLATE_RECOGNITION_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_HIGH_ALTITUDE_SEATBELT_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_SAFETY_HELMET_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_SAFETY_HELMET_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_PERSON_FALL_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_PERSON_FALL_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_PHONE_USAGE_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_PHONE_USAGE_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_SMOKING_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_SMOKING_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_OPEN_FLAME_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_OPEN_FLAME_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_BARE_SOIL_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_BARE_SOIL_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_HOLE_PROTECTION_BAR_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_HOLE_PROTECTION_BAR_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_REFLECTIVE_CLOTHING_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_REFLECTIVE_CLOTHING_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_PET_RECOGNITION_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_PET_RECOGNITION_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_CLIMB_FENCE_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_CLIMB_FENCE_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_DIMISSION_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_DIMISSION_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_ILLEGAL_LANE_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_ILLEGAL_LANE_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_RETROGRADE_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_RETROGRADE_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_NONMOTOR_VEHICLE_INTRUSION_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_OCCUPATION_EMERGENCY_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_OCCUPATION_EMERGENCY_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_PEDESTRIAN_INTRUSION_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_PEDESTRIAN_INTRUSION_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_SMOKE_FIRE_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_SMOKE_FIRE_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_ROAD_PONDING_CFG:
            return NetTV_GetDevConfig_Impl<NET_TV_ROAD_PONDING_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_PARKINGALARM:
            return NetTV_GetDevConfig_Impl<NET_TV_PARKING_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_UNATTENDEDOBJECTALARM:
            return NetTV_GetDevConfig_Impl<NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_OBJECTREMOVALALARM:
            return NetTV_GetDevConfig_Impl<NET_TV_OBJECT_REMOVAL_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_AUDIOANOMALYALARM:
            return NetTV_GetDevConfig_Impl<NET_TV_AUDIO_ANOMALY_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_PREVIEW_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_PREVIEW_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_CHANNEL_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_CHANNEL_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_CHANNEL_LIST:
            return NetTV_GetDevConfig_Impl<NET_TV_CHANNEL_LIST_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_UPGRADESTATUS:
            return NetTV_GetDevConfig_Impl<NET_TV_UPGRADE_STATUS_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_UPGRADEVERSION:
            return NetTV_GetDevConfig_Impl<NET_TV_UPGRADE_VERSION_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_CAPTURE_PLAN_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_CAPTURE_PLAN_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_CAPTURE_PARAM_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_CAPTURE_PARAM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_EXPOSURE_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_EXPOSURE_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_DAYNIGHT_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_DAYNIGHT_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_BACKLIGHT_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_BACKLIGHT_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_DENOISE_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_DENOISE_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_WHITEBALANCE_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_WHITEBALANCE_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_FROM_STREAM_TALKBACK:
            return NetTV_GetDevConfig_Impl<NET_TV_TALKBACK_STREAM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_FACECAPTUREINFO:
            return NetTV_GetDevConfig_Impl<NET_TV_FACE_CAPTURE_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_TARGET_LIB:
            return NetTV_GetDevConfig_Impl<NET_TV_FACE_LIB_LIST_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_GET_FACE_INFO:
            return NetTV_GetDevConfig_Impl<NET_TV_FACE_INFO_LIST_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        default:
            CErrorManage::instance()->SetLastError(NET_TV_E_CMD_NOT_SUPPORT);
            return FALSE;
    }
}

NET_TV_API BOOL STDCALL NET_TV_SetDevConfig(IN  LPVOID  lpUserID,
                                            IN    INT32   dwChannelID,
                                            IN    INT32   dwCommand,
                                            INOUT LPVOID  lpOutBuffer,
                                            OUT   INT32   dwOutBufferSize,
                                            OUT   INT32   *pdwBytesReturned)
{
    CHECK_SDK_INIT(FALSE);

    switch (dwCommand)
    {
        case NET_TV_SET_DEVICECFG:
            return NetTV_SetDevConfig_Impl<NET_TV_DEVICE_BASICINFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_NTPCFG:
            return NetTV_SetDevConfig_Impl<NET_TV_ALARM_EXCEPTION_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_AUDIOCFG:
            return NetTV_SetDevConfig_Impl<NET_TV_AUDIO_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_STREAMCFG:
            return NetTV_SetDevConfig_Impl<NET_TV_VIDEO_ENCODE_OPTION_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_OSDCAPCFG:
            return NetTV_SetDevConfig_Impl<NET_TV_VIDEO_OSD_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_IMAGECFG:
            return NetTV_SetDevConfig_Impl<NET_TV_IMAGE_SETTING_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_NETWORKCFG:
            return NetTV_SetDevConfig_Impl<NET_TV_NETWORKCFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_CONFIG_WIFI_STA:
            return NetTV_SetDevConfig_Impl<NET_TV_WIFI_STA_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_CONNECT_WIFI_STA:
            return NetTV_SetDevConfig_Impl<NET_TV_WIFI_STA_CONNECT_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_DISCONNECT_WIFI_STA:
            return NetTV_SetDevConfig_Impl<NET_TV_WIFI_STA_CONNECT_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_4G_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_4G_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_HOTSPOT_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_HOTSPOT_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_SECURITY_SERVICES_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_SECURITY_SERVICES_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_LOG_SERVER:
            return NetTV_SetDevConfig_Impl<NET_TV_LOG_SERVER_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_TEST_LOG_SERVER:
            return NetTV_SetDevConfig_Impl<NET_TV_LOG_SERVER_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_CONTROL_RECORD_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_RECORD_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_RECORD_SCHEDULE:
            return NetTV_SetDevConfig_Impl<NET_TV_RECORD_SCHEDULE_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_RECORD_ADVANCED_PARAM:
            return NetTV_SetDevConfig_Impl<NET_TV_RECORD_ADVANCED_PARAM_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_DOWNLOAD_RECORD_FILE:
            return NetTV_SetDevConfig_Impl<NET_TV_RECORD_DOWNLOAD_LIST_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_PRIVACYMASKCFG:
            printf("[ClientSDK] SET_PRIVACYMASKCFG cmd=%d, buf=%d, privacy_size=%zu\n",
                   dwCommand, dwOutBufferSize, sizeof(NET_TV_PRIVACY_MASK_CFG_S));
            return NetTV_SetDevConfig_Impl<NET_TV_PRIVACY_MASK_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_TAMPERALARM:
            return NetTV_SetDevConfig_Impl<NET_TV_TAMPER_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_MOTIONALARM:
            return NetTV_SetDevConfig_Impl<NET_TV_MOTION_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_CROSSLINEALARM:
            return NetTV_SetDevConfig_Impl<NET_TV_CROSS_LINE_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_INTRUSIONALARM:
            return NetTV_SetDevConfig_Impl<NET_TV_INTRUSION_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_ENTERREGIONALARM:
            return NetTV_SetDevConfig_Impl<NET_TV_ENTER_REGION_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_LEAVEREGIONALARM:
            return NetTV_SetDevConfig_Impl<NET_TV_LEAVE_REGION_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_LOITERINGALARM:
            return NetTV_SetDevConfig_Impl<NET_TV_LOITERING_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);   
        case NET_TV_SET_SCENECHANGEALARM:
            return NetTV_SetDevConfig_Impl<NET_TV_SCENE_CHANGE_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_CROWDGATHERINGALARM:
            return NetTV_SetDevConfig_Impl<NET_TV_CROWD_GATHERING_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);  
        case NET_TV_SET_GARBAGE_EXPOSURE_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_GARBAGE_EXPOSURE_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_GARBAGE_OVERFLOW_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_GARBAGE_OVERFLOW_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);      
        case NET_TV_SET_PEOPLE_FLOW_STATISTICS_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_RESET_PEOPLE_FLOW_STATISTICS:
            return NetTV_SetDevConfig_Impl<NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_PEOPLE_DENSITY_DETECTION_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_MANHOLE_COVER_ABNORMAL_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_SLEEP_ON_DUTY_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_SLEEP_ON_DUTY_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_PERSON_FALL_DOWN_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_PERSON_FALL_DOWN_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_CONSTRUCTION_OCCUPY_ROAD_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_CONGESTION_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_CONGESTION_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_LICENSE_PLATE_RECOGNITION_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_HIGH_ALTITUDE_SEATBELT_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_SAFETY_HELMET_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_SAFETY_HELMET_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_PERSON_FALL_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_PERSON_FALL_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_PHONE_USAGE_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_PHONE_USAGE_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_SMOKING_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_SMOKING_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_OPEN_FLAME_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_OPEN_FLAME_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_BARE_SOIL_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_BARE_SOIL_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_HOLE_PROTECTION_BAR_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_HOLE_PROTECTION_BAR_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_REFLECTIVE_CLOTHING_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_REFLECTIVE_CLOTHING_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_PET_RECOGNITION_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_PET_RECOGNITION_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_CLIMB_FENCE_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_CLIMB_FENCE_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_DIMISSION_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_DIMISSION_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_ILLEGAL_LANE_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_ILLEGAL_LANE_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_RETROGRADE_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_RETROGRADE_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_NONMOTOR_VEHICLE_INTRUSION_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_OCCUPATION_EMERGENCY_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_OCCUPATION_EMERGENCY_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_PEDESTRIAN_INTRUSION_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_PEDESTRIAN_INTRUSION_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_SMOKE_FIRE_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_SMOKE_FIRE_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_ROAD_PONDING_CFG:
            return NetTV_SetDevConfig_Impl<NET_TV_ROAD_PONDING_CFG_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_PARKINGALARM:
            return NetTV_SetDevConfig_Impl<NET_TV_PARKING_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_UNATTENDEDOBJECTALARM:
            return NetTV_SetDevConfig_Impl<NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_OBJECTREMOVALALARM:
            return NetTV_SetDevConfig_Impl<NET_TV_OBJECT_REMOVAL_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_AUDIOANOMALYALARM:
            return NetTV_SetDevConfig_Impl<NET_TV_AUDIO_ANOMALY_ALARM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_PREVIEW_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_PREVIEW_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_UPGRADE:
            return NetTV_SetDevConfig_Impl<NET_TV_UPGRADE_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_CAPTURE_PLAN_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_CAPTURE_PLAN_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_CAPTURE_PARAM_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_CAPTURE_PARAM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_EXPOSURE_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_EXPOSURE_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_DAYNIGHT_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_DAYNIGHT_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_BACKLIGHT_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_BACKLIGHT_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_DENOISE_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_DENOISE_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_WHITEBALANCE_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_WHITEBALANCE_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_STATE_TALKBACK:
            return NetTV_SetDevConfig_Impl<NET_TV_TALKBACK_STATE_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_TO_STREAM_TALKBACK:
            return NetTV_SetDevConfig_Impl<NET_TV_TALKBACK_STREAM_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_REPLAY_TALKBACK:
            return NetTV_SetDevConfig_Impl<NET_TV_REPLAY_TALKBACK_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_FACECAPTUREINFO:
            return NetTV_SetDevConfig_Impl<NET_TV_FACE_CAPTURE_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_SET_FACE_COMPARE_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_FACE_COMPARE_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_ADD_TARGET_LIB:
        case NET_TV_DEL_TARGET_LIB:
        case NET_TV_SET_TARGET_LIB:
            return NetTV_SetDevConfig_Impl<NET_TV_FACE_LIB_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_ADD_FACE_INFO:
        case NET_TV_SET_FACE_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_FACE_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TV_DEL_FACE_INFO:
            return NetTV_SetDevConfig_Impl<NET_TV_FACE_ID_INFO_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        default:
            CErrorManage::instance()->SetLastError(NET_TV_E_CMD_NOT_SUPPORT);
            return FALSE;
    }
}
