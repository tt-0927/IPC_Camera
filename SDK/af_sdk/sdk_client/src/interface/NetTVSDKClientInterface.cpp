/**
 * @file NetTVSDKClientInterface.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-22
 *
 * @brief 客户端SDK外部接口实现（纯薄壳层）
 *        所有 NET_* 函数均为对外接口，内部逻辑委托给 core 层各管理器
 */

#include <stdio.h>
#include <stdint.h>
#include <cstring>
#include <memory>

#include "NetTVSDKClientInterface.h"
#include "NetTVSDKHttpUrl.h"
#include "CommandExecutor.h"
#include "ErrorManage.h"
#include "SessionManager.h"
#include "NetSdkLog.h"
#include "ConfigQuery.h"
#include "RecordFrameHub.h"
#include "VoiceComHub.h"
#include "BG6_ZHSJ/DiscoveryProber.h"

#define NETTVSDK_MAKE_VERSION(major, minor, rev1, rev2) \
    ((uint32_t)( \
        ((major & 0xFF) << 24) |  /* 主版本左移24位（占高8位） */ \
        ((minor & 0xFF) << 16) |  /* 次版本左移16位（占次高8位） */ \
        ((rev1 & 0xFF) << 8) |    /* 附加版本1左移8位（占第三8位） */ \
        (rev2 & 0xFF)             /* 附加版本2占最低8位 */ \
    ))

#define NETTVSDK_VERSION        NETTVSDK_MAKE_VERSION(1, 0, 0, 0)			/* 当前版本 */

/* ==================== 初始化 / 清理 ==================== */

/**
 * @brief SDK初始化接口
 * @return 成功返回TRUE，失败返回FALSE
 * @note 调用其他SDK接口前必须先调用此接口；重复调用会返回失败
 */
NET_API BOOL STDCALL NET_clientInit(void)
{
	CHECK_SDK_ALREADY_INIT(FALSE);
	try
	{
        auto pDevMgr = CSessionManager::instance();

        if (!pDevMgr)
		{
            return FALSE;
        }
		pDevMgr->SetInitialized(true);
        CErrorManage::instance()->SetLastError(NET_E_SUCCEED);
        return TRUE;
    }
    catch (...)
	{
        return FALSE;
    }

}

/**
 * @brief SDK清理接口
 * @return 成功返回TRUE，失败返回FALSE
 * @note 退出程序前调用，释放SDK内部资源；SDK未初始化时调用会返回失败
 */
NET_API BOOL STDCALL NET_clientCleanup(void)
{

	CHECK_SDK_INIT(FALSE);

	try
	{
        /* 先停止所有流连接（在会话释放前） */
        CRecordFrameHub::instance()->Cleanup();
        CVoiceComHub::instance()->Cleanup();

        auto pDevMgr = CSessionManager::instance();

        if (pDevMgr)
		{
            pDevMgr->Cleanup();
        }
		pDevMgr->SetInitialized(false);
        CSessionManager::DestroyInstance();

        CErrorManage::instance()->SetLastError(NET_E_SDK_NOT_INIT);

        return TRUE;
    }
    catch (...)
	{
        return FALSE;
    }


	return FALSE;
}

/**
 * @brief 设置日志输出到文件
 * @param dwLogLevel 日志等级
 * @param strLogDir 日志文件存放目录
 * @param dwLogFileSize 单个日志文件最大大小（字节），默认5MB
 * @param dwLogFileNum 日志文件最大数量，默认10个
 * @return 成功返回TRUE，失败返回FALSE
 * @note 必须在NET_TV_Init之前调用
 */
NET_API BOOL STDCALL NET_clientSetLogToFile(IN INT32 dwLogLevel,IN CHAR  *strLogDir,IN INT32 dwLogFileSize,IN INT32 dwLogFileNum)
{
    if (strLogDir == NULL)
    {
        return FALSE;
    }

    /* 构造完整日志路径 */
    char szLogPath[512] = {0};
#ifdef _WIN32
    snprintf(szLogPath, sizeof(szLogPath), "%s\\NetTVSDKClient.log", strLogDir);
#else
    snprintf(szLogPath, sizeof(szLogPath), "%s/NetTVSDKClient.log", strLogDir);
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

/**
 * @brief 获取SDK版本号
 * @return 返回版本号
 */
NET_API INT32 STDCALL NET_clientGetSdkVersion(void)
{
	return NETTVSDK_VERSION;
}

/**
 * @brief 获取最后一次错误码
 * @return 返回错误码，参见NET_TV_COMMON_ECODE_E枚举
 */
NET_API INT32 STDCALL NET_clientGetLastError()
{
	return CErrorManage::instance()->GetLastError();
}

/* ==================== 异常 / 超时 / 连接 ==================== */

/**
 * @brief 设置异常回调函数
 * @param cbExceptionCallBack 异常回调函数指针
 * @param lpUserData 用户自定义数据
 * @return 暂未实现，返回FALSE
 */
NET_API BOOL STDCALL NET_clientSetExceptionCallBack(IN NET_ExceptionCallBack_PF cbExceptionCallBack,
                                                                 IN LPVOID lpUserData)
{
	return FALSE;
}

/**
 * @brief 设置接收超时时间
 * @param pstRevTimeout 超时时间配置结构体
 * @return 成功返回TRUE，失败返回FALSE
 */
NET_API BOOL STDCALL NET_clientSetRevTimeOut(IN pNET_RevTimeout_S pstRevTimeout)
{
	CHECK_SDK_INIT(FALSE);

    if (!pstRevTimeout)
	{
		CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
		return FALSE;
	}

	auto pDevMgr = CSessionManager::instance();

	if (!pDevMgr)
	{
		CErrorManage::instance()->SetLastError(NET_E_ALLOC_RESOURCE_ERROR);
		return FALSE;
	}

    pDevMgr->SetGlobalRevTimeout(pstRevTimeout->uRevTimeOut);
	CErrorManage::instance()->SetLastError(NET_E_SUCCEED);
	return FALSE;
}

/**
 * @brief 设置连接超时时间
 * @param dwWaitTime 单次连接等待时间
 * @param dwTrytimes 连接重试次数
 * @return 成功返回TRUE，失败返回FALSE
 */
NET_API BOOL STDCALL NET_clientSetConnectTime(IN INT32 dwWaitTime,
                                                           IN INT32 dwTrytimes)
{
	CHECK_SDK_INIT(FALSE);
    auto pDevMgr = CSessionManager::instance();

	if (!pDevMgr)
	{
		CErrorManage::instance()->SetLastError(NET_E_ALLOC_RESOURCE_ERROR);
		return FALSE;
	}

	pDevMgr->SetGlobalConnectTime(dwWaitTime, dwTrytimes);
	CErrorManage::instance()->SetLastError(NET_E_SUCCEED);
	return FALSE;
}

/* ==================== 登录 / 注销 ==================== */

/**
 * @brief 登录设备接口
 * @param pstDevLoginInfo 登录信息结构体，包含设备IP、端口、用户名、密码
 * @param pstDevInfo 输出参数，登录成功后返回设备基本信息
 * @return 成功返回用户句柄(LPVOID)，失败返回NULL
 */
NET_API LPVOID STDCALL NET_clientLogin(IN pNET_DeviceLoginInfo_S pstDevLoginInfo,
                                                        OUT pNET_DeviceInfo_S pstDevInfo)
{
    NETSDK_LOG_MESSAGE_INFO("[NetTVSDK] NET_clientLogin called, IP=%s, Port=%d, User=%s",
                  pstDevLoginInfo ? pstDevLoginInfo->szIPAddr : "NULL",
                  pstDevLoginInfo ? pstDevLoginInfo->uPort : 0,
                  pstDevLoginInfo ? pstDevLoginInfo->szUserName : "NULL");

	CHECK_SDK_INIT(NULL);
	auto pDevMgr = CSessionManager::instance();
	if (!pDevMgr)
	{
		CErrorManage::instance()->SetLastError(NET_E_ALLOC_RESOURCE_ERROR);
        NETSDK_LOG_MESSAGE_ERROR("[NetTVSDK] NET_clientLogin failed, DeviceManager is NULL");
		return NULL;
	}
	LPVOID lpUserID = pDevMgr->Login(pstDevLoginInfo->szIPAddr, pstDevLoginInfo->uPort, pstDevLoginInfo->szUserName, pstDevLoginInfo->szPassword);
    NETSDK_LOG_MESSAGE_INFO("[NetTVSDK] NET_clientLogin returned, userID=%p", lpUserID);

	/* 发送获取设备信息命令 */
	if(lpUserID != NULL)
	{
		if(!CCommandExecutor::instance()->ExecuteGet<NET_DeviceInfo_S>(lpUserID,NET_API_PATH_DEVICE_GETINFO,pstDevInfo,NULL))
		{
            NETSDK_LOG_MESSAGE_ERROR("[NetTVSDK] NET_clientLogin failed to get device info");
            pDevMgr->Logout(lpUserID);
			return NULL;
		}
	}

	return lpUserID;
}

/**
 * @brief 注销登录接口
 * @param lpUserID 用户句柄，由NET_TV_Login返回
 * @return 成功返回TRUE，失败返回FALSE
 */
NET_API BOOL STDCALL NET_clientLogout(IN LPVOID lpUserID)
{
	CHECK_SDK_INIT(FALSE);

	auto pDevMgr = CSessionManager::instance();

	if (!pDevMgr)
	{
		CErrorManage::instance()->SetLastError(NET_E_ALLOC_RESOURCE_ERROR);
		return FALSE;
	}

	if(!pDevMgr->Logout(lpUserID))
	{
		CErrorManage::instance()->SetLastError(NET_E_NO_USER);
		return FALSE;
	}

	CErrorManage::instance()->SetLastError(NET_E_SUCCEED);
	return TRUE;
}

/* ==================== 报警 / 监听 ==================== */

/**
 * @brief 设置报警消息回调函数
 */
NET_API BOOL STDCALL NET_clientSetAlarmCallBack(IN LPVOID lpUserID,
                                            IN NET_AlarmCallBack cbAlarmMessCallBack,
                                            IN LPVOID lpUserData)
{
    CHECK_SDK_INIT(FALSE);
    auto pDevMgr = CSessionManager::instance();
    if (!pDevMgr) return FALSE;

    auto session = pDevMgr->GetSession((LPUSER_HANDLE)lpUserID);
    if (!session) {
        CErrorManage::instance()->SetLastError(NET_E_NO_USER);
        return FALSE;
    }

    session->SetAlarmCallback(cbAlarmMessCallBack, lpUserData);
    CErrorManage::instance()->SetLastError(NET_E_SUCCEED);
    return TRUE;
}

/**
 * @brief 设置通道状态回调函数
 */
NET_API BOOL STDCALL NET_clientSetChannelStatusCallBack(IN LPVOID lpUserID,
                                                        IN NET_ChannelStatusCallBack cbChannelStatusCallBack,
                                                        IN LPVOID lpUserData)
{
    CHECK_SDK_INIT(FALSE);
    auto pDevMgr = CSessionManager::instance();
    if (!pDevMgr) return FALSE;

    auto session = pDevMgr->GetSession((LPUSER_HANDLE)lpUserID);
    if (!session)
    {
        CErrorManage::instance()->SetLastError(NET_E_NO_USER);
        return FALSE;
    }

    session->SetChannelStatusCallback(cbChannelStatusCallBack, lpUserData);
    CErrorManage::instance()->SetLastError(NET_E_SUCCEED);
    return TRUE;
}

/**
 * @brief 开启报警监听接口
 */
NET_API BOOL STDCALL NET_clientStartListen(IN LPVOID lpUserID)
{
    CHECK_SDK_INIT(FALSE);
    auto pDevMgr = CSessionManager::instance();
    if (!pDevMgr) return FALSE;

    auto session = pDevMgr->GetSession((LPUSER_HANDLE)lpUserID);
    if (!session)
    {
        CErrorManage::instance()->SetLastError(NET_E_NO_USER);
        return FALSE;
    }

    if (!session->StartAlarmListen())
    {
        CErrorManage::instance()->SetLastError(NET_E_FAILED);
        return FALSE;
    }

    CErrorManage::instance()->SetLastError(NET_E_SUCCEED);
    return TRUE;
}

/**
 * @brief 停止报警监听接口
 */
NET_API BOOL STDCALL NET_clientStopListen(IN LPVOID lpUserID)
{
    CHECK_SDK_INIT(FALSE);
    auto pDevMgr = CSessionManager::instance();
    if (!pDevMgr) return FALSE;

    auto session = pDevMgr->GetSession((LPUSER_HANDLE)lpUserID);
    if (!session)
    {
        CErrorManage::instance()->SetLastError(NET_E_NO_USER);
        return FALSE;
    }

    if (!session->StopAlarmListen())
    {
        CErrorManage::instance()->SetLastError(NET_E_FAILED);
        return FALSE;
    }

    CErrorManage::instance()->SetLastError(NET_E_SUCCEED);
    return TRUE;
}

/* ==================== 设备控制 ==================== */

/**
 * @brief 设备控制接口
 * @note 支持PTZ云台控制等设备控制功能
 */
NET_API BOOL STDCALL NET_clientDeviceControl(IN LPVOID lpUserID,
                                             IN pNET_DeviceControlInfo_S pstCtrlInfo)
{
    CHECK_SDK_INIT(FALSE);

    if (!lpUserID || !pstCtrlInfo || pstCtrlInfo->uChannelID <= 0 ||
        pstCtrlInfo->uControlType <= 0 || pstCtrlInfo->uCommand <= 0 ||
        pstCtrlInfo->uDurationMs < 0)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    if (pstCtrlInfo->uControlType == NET_DEVICE_CTRL_TYPE_PTZ &&
        (pstCtrlInfo->uSpeed < NET_MIN_PTZ_SPEED_LEVEL || pstCtrlInfo->uSpeed > NET_MAX_PTZ_SPEED_LEVEL))
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    if (pstCtrlInfo->uSize == 0)
    {
        pstCtrlInfo->uSize = sizeof(NET_DeviceControlInfo_S);
    }

    std::string body = SDKConvert::to_string(*pstCtrlInfo);
    std::string respBody;
    if (!CCommandExecutor::instance()->ExecuteRaw((LPUSER_HANDLE)lpUserID, "POST", NET_API_PATH_DEVICE_CONTROL, body, respBody))
    {
        return FALSE;
    }

    const int respCode = SDKConvert::get_respCode(respBody);
    CErrorManage::instance()->SetLastError(respCode);
    return respCode == NET_E_SUCCEED ? TRUE : FALSE;
}

/* ==================== 配置 / 能力集（委托 ConfigQuery） ==================== */

/**
 * @brief 获取设备能力集接口
 */
NET_API BOOL STDCALL NET_clientGetDeviceCapability(IN LPVOID lpUserID,
                                                   IN INT32 dwChannelID,
                                                   IN INT32 dwCommand,
                                                   OUT LPVOID lpOutBuffer,
                                                   OUT INT32 dwOutBufferSize,
                                                   OUT INT32 *pdwBytesReturned)
{
    CHECK_SDK_INIT(FALSE);
    return CConfigQuery::instance()->GetDeviceCapability(lpUserID, dwChannelID, dwCommand,
                                                              lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
}

/**
 * @brief 获取设备配置接口（通用）
 */
NET_API BOOL STDCALL NET_clientGetDevConfig(IN  LPVOID  lpUserID,
                                            IN    INT32   dwChannelID,
                                            IN    INT32   dwCommand,
                                            INOUT LPVOID  lpOutBuffer,
                                            OUT   INT32   dwOutBufferSize,
                                            OUT   INT32   *pdwBytesReturned)
{
    CHECK_SDK_INIT(FALSE);
    return CConfigQuery::instance()->GetDevConfig(lpUserID, dwChannelID, dwCommand,
                                                        lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
}

/**
 * @brief 设置设备配置接口（通用）
 */
NET_API BOOL STDCALL NET_clientSetDevConfig(IN  LPVOID  lpUserID,
                                            IN    INT32   dwChannelID,
                                            IN    INT32   dwCommand,
                                            INOUT LPVOID  lpOutBuffer,
                                            OUT   INT32   dwOutBufferSize,
                                            OUT   INT32   *pdwBytesReturned)
{
    CHECK_SDK_INIT(FALSE);
    return CConfigQuery::instance()->SetDevConfig(lpUserID, dwChannelID, dwCommand,
                                                        lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
}

/* ==================== 回放 ==================== */

/**
 * @brief 回放控制接口
 */
NET_API BOOL STDCALL NET_clientControlReplay(IN    LPVOID lpUserID,
                                             INOUT pNET_ReplayCtrlInfo_S pstInfo,
                                             OUT   INT32 *pdwBytesReturned)
{
    CHECK_SDK_INIT(FALSE);

    if (!pstInfo)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    if (pstInfo->uChannel <= 0)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    std::string body = SDKConvert::to_string(*pstInfo);
    std::string respBody;
    if (!CCommandExecutor::instance()->ExecuteRaw(lpUserID, "POST", NET_API_URL_REPLAY_CONTROL(), body, respBody))
    {
        return FALSE;
    }

    SDKConvert::to_respStruct(respBody, *pstInfo);
    if (pdwBytesReturned)
    {
        *pdwBytesReturned = sizeof(NET_ReplayCtrlInfo_S);
    }

    return TRUE;
}

/**
 * @brief 获取回放录像列表接口
 */
NET_API BOOL STDCALL NET_clientGetReplayRecordList(IN    LPVOID lpUserID,
                                                   INOUT pNET_ReplayRecordList_S pstInfo,
                                                   OUT   INT32 *pdwBytesReturned)
{
    CHECK_SDK_INIT(FALSE);

    if (!pstInfo)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    if (pstInfo->uChannel <= 0)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    std::string body = SDKConvert::to_string(*pstInfo);
    std::string respBody;
    if (!CCommandExecutor::instance()->ExecuteRaw(lpUserID, "POST", NET_API_URL_REPLAY_GET_RECORD_LIST(), body, respBody))
    {
        return FALSE;
    }

    SDKConvert::to_respStruct(respBody, *pstInfo);
    if (pdwBytesReturned)
    {
        *pdwBytesReturned = sizeof(NET_ReplayRecordList_S);
    }

    return TRUE;
}

/**
 * @brief 获取回放URL接口
 */
NET_API BOOL STDCALL NET_clientGetReplayUrl(IN    LPVOID lpUserID,
                                            INOUT pNET_ReplayUrlInfo_S pstInfo,
                                            OUT   INT32 *pdwBytesReturned)
{
    CHECK_SDK_INIT(FALSE);

    if (!pstInfo)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    if (pstInfo->uChannel <= 0)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    std::string body = SDKConvert::to_string(*pstInfo);
    std::string respBody;
    if (!CCommandExecutor::instance()->ExecuteRaw(lpUserID, "POST", NET_API_URL_REPLAY_GET_URL(), body, respBody))
    {
        return FALSE;
    }

    SDKConvert::to_respStruct(respBody, *pstInfo);
    if (pdwBytesReturned)
    {
        *pdwBytesReturned = sizeof(NET_ReplayUrlInfo_S);
    }

    return TRUE;
}

/* ==================== 文件上传 ==================== */

/**
 * @brief 文件上传接口
 */
BOOL STDCALL
NET_clientUploadFile(IN LPVOID   lpUserID,
                  IN const CHAR* szFilePath,
                  IN const CHAR* szRemoteName)
{
    if (!lpUserID || !szFilePath || !szRemoteName) {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    std::string remoteName(szRemoteName);
    std::string url = std::string(NET_API_PATH_UPGRADE_UPLOAD)
                      + "?" NET_API_PARAM_FILENAME "=" + remoteName;

    std::string ignoreResp;
    return CCommandExecutor::instance()->ExecuteUpload(
        lpUserID, "PUT", url, std::string(szFilePath), ignoreResp);
}

/* ==================== 录像帧流（委托 RecordFrameHub） ==================== */

/**
 * @brief 启动录像帧流接口
 */
NET_API BOOL STDCALL
NET_clientStartRecordFrameStream(IN LPVOID lpUserID,
                              IN pNET_RecordFrameStreamCond_S pstCond,
                              OUT pNET_RecordFrameStreamInfo_S pstStreamInfo,
                              IN NET_RecordFrameCallBack cbRecordFrame,
                              IN LPVOID lpUserData)
{
    CHECK_SDK_INIT(FALSE);
    return CRecordFrameHub::instance()->StartStream(lpUserID, pstCond, pstStreamInfo, cbRecordFrame, lpUserData);
}

/**
 * @brief 停止录像帧流接口
 */
NET_API BOOL STDCALL
NET_clientStopRecordFrameStream(IN LPVOID lpUserID,
                             IN const CHAR* szStreamId)
{
    CHECK_SDK_INIT(FALSE);
    return CRecordFrameHub::instance()->StopStream(lpUserID, szStreamId);
}

/* ==================== 语音对讲（委托 VoiceComHub） ==================== */

/**
 * @brief 启动语音对讲接口
 */
BOOL STDCALL
NET_clientStartVoiceCom(IN LPVOID              lpUserID,
                     IN pNET_VoiceComStartInfo_S pstStartInfo,
                     IN NET_VoiceComCallBack cbVoiceCom,
                     IN LPVOID              lpUserData)
{
    return CVoiceComHub::instance()->Start(lpUserID, pstStartInfo, cbVoiceCom, lpUserData);
}

/**
 * @brief 语音对讲发送数据接口
 */
BOOL STDCALL
NET_clientVoiceComSendData(IN LPVOID       lpUserID,
                        IN const CHAR*  pData,
                        IN UINT32       dwSize)
{
    return CVoiceComHub::instance()->SendData(lpUserID, pData, dwSize);
}

/**
 * @brief 停止语音对讲接口
 */
BOOL STDCALL
NET_clientStopVoiceCom(IN LPVOID lpUserID)
{
    return CVoiceComHub::instance()->Stop(lpUserID);
}

/* ==================== 设备发现 ==================== */

/**
 * @brief 设备发现接口（局域网搜索）
 */
BOOL STDCALL
NET_clientSearchDiscovery(IN  const CHAR*                      szInterfaceIP,
                        IN  UINT32                           dwTimeoutMs,
                        OUT NET_DiscoveryDeviceInfo_S*       pDeviceList,
                        IN  int                              nMaxCount,
                        OUT int*                             pnOutCount)
{
    if (!pDeviceList || nMaxCount <= 0 || !pnOutCount) {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    CDiscoveryProber searcher;
    int ret = searcher.search(szInterfaceIP, dwTimeoutMs, pDeviceList, nMaxCount, pnOutCount);
    if (ret < 0) {
        CErrorManage::instance()->SetLastError(NET_E_SYSCALL_FALIED);
        return FALSE;
    }
    return TRUE;
}
