/**
 * @file NetTVSDKClientInterface.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-22
 *
 * @brief 客户端SDK接口实现，包含初始化、登录、配置获取/设置、回放控制、录像帧流、语音对讲等核心功能
 */

#include <stdio.h>
#include <stdint.h>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <map>
#include <memory>
#include <mutex>
#include <utility>

#include "NetTVSDKClientInterface.h"
#include "NetTVSDKHttpUrl.h"
#include "CommandExecutor.h"
#include "ErrorManage.h"
#include "DeviceManage.h"
#include "NetSdkLog.h"
#include "BG6_ZHSJ/RecordFrameClient.h"
#include "BG6_ZHSJ/VoiceComClient.h"

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
        auto pMgr = CDeviceManage::instance(); \
        /* 检查单例是否存在 以及 标志位是否为 true */ \
        if (!pMgr || !pMgr->IsInitialized()) { \
            CErrorManage::instance()->SetLastError(NET_E_SDK_NOT_INIT); \
            return val; \
        } \
    } while(0)

/* 检查SDK是否已经初始化 */
#define CHECK_SDK_ALREADY_INIT(val) \
    do { \
        auto pMgr = CDeviceManage::instance(); \
        /* 如果单例存在 且 标志位为 true，说明已经 Init 过了 */ \
        if (pMgr && pMgr->IsInitialized()) { \
            CErrorManage::instance()->SetLastError(NET_E_ALREDY_INIT_ERROR); \
            return val; \
        } \
    } while(0)

/* 全局错误码 */
thread_local int CErrorManage::s_nLastErrorCode = NET_E_SDK_NOT_INIT;

/**
 * @brief SDK初始化接口
 * @return 成功返回TRUE，失败返回FALSE
 * @note 调用其他SDK接口前必须先调用此接口；重复调用会返回失败
 */
NET_API BOOL STDCALL NET_Init(void)
{
	CHECK_SDK_ALREADY_INIT(FALSE);
	try
	{
        auto pDevMgr = CDeviceManage::instance();

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
NET_API BOOL STDCALL NET_Cleanup(void)
{

	CHECK_SDK_INIT(FALSE);

	try
	{
        auto pDevMgr = CDeviceManage::instance();

        if (pDevMgr)
		{
            pDevMgr->Cleanup();
        }
		pDevMgr->SetInitialized(false);
        CDeviceManage::DestroyInstance();

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
NET_API BOOL STDCALL NET_SetLogToFile(IN INT32 dwLogLevel,IN CHAR  *strLogDir,IN INT32 dwLogFileSize,IN INT32 dwLogFileNum)
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

/**
 * @brief 获取SDK版本号
 * @return 返回版本号，格式为NETTVSDK_MAKE_VERSION(major, minor, rev1, rev2)
 */
NET_API INT32 STDCALL NET_GetSDKVersion(void)
{

	return NETTVSDK_VERSION;
}

/**
 * @brief 获取最后一次错误码
 * @return 返回错误码，参见NET_TV_COMMON_ECODE_E枚举
 * @note 每次SDK接口调用失败后，可通过此接口获取具体错误原因
 */
NET_API INT32 STDCALL NET_GetLastError()
{
	return CErrorManage::instance()->GetLastError();
}

/**
 * @brief 设置异常回调函数
 * @param cbExceptionCallBack 异常回调函数指针
 * @param lpUserData 用户自定义数据
 * @return 暂未实现，返回FALSE
 */
NET_API BOOL STDCALL NET_SetExceptionCallBack(IN NET_ExceptionCallBack_PF cbExceptionCallBack,
                                                                 IN LPVOID lpUserData)
{
	return FALSE;
}

/**
 * @brief 设置接收超时时间
 * @param pstRevTimeout 超时时间配置结构体
 * @return 暂未实现，返回FALSE
 * @note 设置SDK网络请求的接收超时时间
 */
NET_API BOOL STDCALL NET_SetRevTimeOut(IN pNET_RevTimeout_S pstRevTimeout)
{
	CHECK_SDK_INIT(FALSE);

    if (!pstRevTimeout)
	{
		CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
		return FALSE;
	}

	auto pDevMgr = CDeviceManage::instance();

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
 * @return 暂未实现，返回FALSE
 * @note 设置SDK网络连接的超时时间和重试次数
 */
NET_API BOOL STDCALL NET_SetConnectTime(IN INT32 dwWaitTime,
                                                           IN INT32 dwTrytimes)
{
	CHECK_SDK_INIT(FALSE);
    auto pDevMgr = CDeviceManage::instance();

	if (!pDevMgr)
	{
		CErrorManage::instance()->SetLastError(NET_E_ALLOC_RESOURCE_ERROR);
		return FALSE;
	}

	pDevMgr->SetGlobalConnectTime(dwWaitTime, dwTrytimes);
	CErrorManage::instance()->SetLastError(NET_E_SUCCEED);
	return FALSE;
}

/**
 * @brief 登录设备接口
 * @param pstDevLoginInfo 登录信息结构体，包含设备IP、端口、用户名、密码
 * @param pstDevInfo 输出参数，登录成功后返回设备基本信息
 * @return 成功返回用户句柄(LPVOID)，失败返回NULL
 * @note 登录成功后会自动获取设备信息并填充到pstDevInfo
 */
NET_API LPVOID STDCALL NET_Login(IN pNET_DeviceLoginInfo_S pstDevLoginInfo,
                                                        OUT pNET_DeviceInfo_S pstDevInfo)
{
    NETSDK_LOG_MESSAGE_INFO("[NetTVSDK] NET_Login called, IP=%s, Port=%d, User=%s",
                  pstDevLoginInfo ? pstDevLoginInfo->szIPAddr : "NULL",
                  pstDevLoginInfo ? pstDevLoginInfo->uPort : 0,
                  pstDevLoginInfo ? pstDevLoginInfo->szUserName : "NULL");

	CHECK_SDK_INIT(NULL);
	auto pDevMgr = CDeviceManage::instance();
	if (!pDevMgr)
	{
		CErrorManage::instance()->SetLastError(NET_E_ALLOC_RESOURCE_ERROR);
        NETSDK_LOG_MESSAGE_ERROR("[NetTVSDK] NET_Login failed, DeviceManager is NULL");
		return NULL;
	}
	LPVOID lpUserID = pDevMgr->Login(pstDevLoginInfo->szIPAddr, pstDevLoginInfo->uPort, pstDevLoginInfo->szUserName, pstDevLoginInfo->szPassword);
    NETSDK_LOG_MESSAGE_INFO("[NetTVSDK] NET_Login returned, userID=%p", lpUserID);

	/* 发送获取设备信息命令 */
	if(lpUserID != NULL)
	{
		if(!CommandExecutor::instance()->ExecuteGet<NET_DeviceInfo_S>(lpUserID,NET_API_PATH_DEVICE_GETINFO,pstDevInfo,NULL))
		{
            NETSDK_LOG_MESSAGE_ERROR("[NetTVSDK] NET_Login failed to get device info");
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
 * @note 注销后释放登录会话资源，用户句柄失效
 */
NET_API BOOL STDCALL NET_Logout(IN LPVOID lpUserID)
{
	CHECK_SDK_INIT(FALSE);

	auto pDevMgr = CDeviceManage::instance();

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

/**
 * @brief 设置报警消息回调函数
 * @param lpUserID 用户句柄
 * @param cbAlarmMessCallBack 报警消息回调函数指针
 * @param lpUserData 用户自定义数据，会在回调时传入
 * @return 成功返回TRUE，失败返回FALSE
 * @note 需先调用NET_TV_StartListen开启监听，报警消息才会通过回调通知
 */
NET_API BOOL STDCALL NET_SetAlarmCallBack(IN LPVOID lpUserID,
                                            IN NET_AlarmCallBack cbAlarmMessCallBack,
                                            IN LPVOID lpUserData)
{
    CHECK_SDK_INIT(FALSE);
    auto pDevMgr = CDeviceManage::instance();
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
 * @brief 设置动态图片 V2 告警回调函数。
 * @param [in] lpUserID 用户登录句柄。
 * @param [in] cbAlarmMessCallBack V2 告警回调函数。
 * @param [in] lpUserData 回调用户数据。
 * @return 设置成功返回 TRUE，失败返回 FALSE。
 */
NET_API BOOL STDCALL NET_SetAlarmCallBackV2(IN LPVOID lpUserID,
                                            IN NET_AlarmCallBackV2 cbAlarmMessCallBack,
                                            IN LPVOID lpUserData)
{
    CHECK_SDK_INIT(FALSE);
    const auto pDeviceManage = CDeviceManage::instance();
    if (!pDeviceManage)
    {
        return FALSE;
    }

    const auto pSession = pDeviceManage->GetSession(static_cast<LPUSER_HANDLE>(lpUserID));
    if (!pSession)
    {
        CErrorManage::instance()->SetLastError(NET_E_NO_USER);
        return FALSE;
    }

    pSession->SetAlarmCallbackV2(cbAlarmMessCallBack, lpUserData);
    CErrorManage::instance()->SetLastError(NET_E_SUCCEED);
    return TRUE;
}

/**
 * @brief 设置通道状态回调函数
 * @param lpUserID 用户句柄
 * @param cbChannelStatusCallBack 通道状态回调函数指针
 * @param lpUserData 用户自定义数据，会在回调时传入
 * @return 成功返回TRUE，失败返回FALSE
 * @note 需先调用NET_TV_StartListen开启监听，通道状态变化才会通过回调通知
 */
NET_API BOOL STDCALL NET_SetChannelStatusCallBack(IN LPVOID lpUserID,
                                                        IN NET_ChannelStatusCallBack cbChannelStatusCallBack,
                                                        IN LPVOID lpUserData)
{
    CHECK_SDK_INIT(FALSE);
    auto pDevMgr = CDeviceManage::instance();
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
 * @param lpUserID 用户句柄
 * @return 成功返回TRUE，失败返回FALSE
 * @note 开启后，报警消息和通道状态变化会通过已注册的回调函数通知
 */
NET_API BOOL STDCALL NET_StartListen(IN LPVOID lpUserID)
{
    CHECK_SDK_INIT(FALSE);
    auto pDevMgr = CDeviceManage::instance();
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
 * @param lpUserID 用户句柄
 * @return 成功返回TRUE，失败返回FALSE
 * @note 停止后，不再接收报警消息和通道状态变化通知
 */
NET_API BOOL STDCALL NET_StopListen(IN LPVOID lpUserID)
{
    CHECK_SDK_INIT(FALSE);
    auto pDevMgr = CDeviceManage::instance();
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

/**
 * @brief 设备控制接口
 * @param lpUserID 用户句柄
 * @param pstCtrlInfo 控制信息结构体，包含通道号、控制类型、命令等
 * @return 成功返回TRUE，失败返回FALSE
 * @note 支持PTZ云台控制等设备控制功能
 */
NET_API BOOL STDCALL NET_DeviceControl(IN LPVOID lpUserID,
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
    if (!CommandExecutor::instance()->ExecuteRaw((LPUSER_HANDLE)lpUserID, "POST", NET_API_PATH_DEVICE_CONTROL, body, respBody))
    {
        return FALSE;
    }

    const int respCode = SDKConvert::get_respCode(respBody);
    CErrorManage::instance()->SetLastError(respCode);
    return respCode == NET_E_SUCCEED ? TRUE : FALSE;
}

#include "BG6_ZHSJ/CapabilityInfoConvert.h"

/**
 * @brief 获取设备能力集接口
 * @param lpUserID 用户句柄
 * @param dwChannelID 通道号
 * @param dwCommand 能力集类型命令码，如NET_CAP_VIDEO_ENCODE、NET_CAP_AUDIO、NET_CAP_OSD等
 * @param lpOutBuffer 输出缓冲区，用于存储能力集信息
 * @param dwOutBufferSize 输出缓冲区大小
 * @param pdwBytesReturned 输出参数，实际返回的数据长度
 * @return 成功返回TRUE，失败返回FALSE
 * @note 根据dwCommand不同，返回不同类型的能力集结构体
 */
NET_API BOOL STDCALL NET_GetDeviceCapability(IN LPVOID lpUserID,
                                                   IN INT32 dwChannelID,
                                                   IN INT32 dwCommand,
                                                   OUT LPVOID lpOutBuffer,
                                                   OUT INT32 dwOutBufferSize,
                                                   OUT INT32 *pdwBytesReturned)
{
    CHECK_SDK_INIT(FALSE);

    if (!lpOutBuffer || dwOutBufferSize <= 0)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    // 使用宏生成统一URL
    std::string url = NET_API_URL_DEVICE_CAPABILITY(dwChannelID, dwCommand);

    switch (dwCommand)
    {
        case NET_CAP_VIDEO_ENCODE:
        {
            if (dwOutBufferSize < (INT32)sizeof(NET_VideoEncodeCap_S))
            {
                CErrorManage::instance()->SetLastError(NET_E_NOENOUGH_BUF);
                return FALSE;
            }

            if (CommandExecutor::instance()->ExecuteGet<NET_VideoEncodeCap_S>(
                    lpUserID, url, lpOutBuffer, pdwBytesReturned))
            {
                return TRUE;
            }
            return FALSE;
        }

        case NET_CAP_AUDIO:
        {
            if (dwOutBufferSize < (INT32)sizeof(NET_AudioCap_S))
            {
                CErrorManage::instance()->SetLastError(NET_E_NOENOUGH_BUF);
                return FALSE;
            }

            if (CommandExecutor::instance()->ExecuteGet<NET_AudioCap_S>(
                    lpUserID, url, lpOutBuffer, pdwBytesReturned))
            {

                return TRUE;
            }
            return FALSE;
        }

        case NET_CAP_OSD:
        {
            if (dwOutBufferSize < (INT32)sizeof(NET_OsdCap_S))
            {
                CErrorManage::instance()->SetLastError(NET_E_NOENOUGH_BUF);
                return FALSE;
            }

            if (CommandExecutor::instance()->ExecuteGet<NET_OsdCap_S>(
                    lpUserID, url, lpOutBuffer, pdwBytesReturned))
            {
                return TRUE;
            }
            return FALSE;
        }

        // 后续扩展其他能力集类型
        // case NET_CAP_SMART:
        // case NET_CAP_IMAGE:
        // case NET_CAP_AUDIO:

        default:
            CErrorManage::instance()->SetLastError(NET_E_CMD_NOT_SUPPORT);
            return FALSE;
    }
}

/**
 * @brief 获取设备配置模板实现函数
 * @tparam T_CFG 配置结构体类型
 * @param lpUserID 用户句柄
 * @param dwChannelID 通道号
 * @param dwCommand 配置命令码
 * @param lpOutBuffer 输出缓冲区，用于存储配置信息
 * @param dwOutBufferSize 输出缓冲区大小
 * @param pdwBytesReturned 输出参数，实际返回的数据长度
 * @return 成功返回TRUE，失败返回FALSE
 * @note NET_GetDevConfig内部调用此模板函数，根据命令码分发到不同配置类型
 */
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
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    if (dwOutBufferSize < (INT32)sizeof(T_CFG))
    {
        CErrorManage::instance()->SetLastError(NET_E_NOENOUGH_BUF);
        return FALSE;
    }

    std::string url = NET_API_URL_DEVICE_GET_DEV_CONFIG(dwChannelID, dwCommand);
    return CommandExecutor::instance()->ExecuteGet<T_CFG>(lpUserID, url, lpOutBuffer, pdwBytesReturned) ? TRUE : FALSE;
}

/**
 * @brief URL编码函数
 * @param value 需要编码的字符串
 * @return 编码后的字符串
 * @note 遵循RFC 3986规范，对非字母数字字符进行%XX编码
 */
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

/**
 * @brief 获取录像文件列表实现函数
 * @param lpUserID 用户句柄
 * @param dwChannelID 通道号
 * @param dwCommand 配置命令码（NET_FIND_RECORD_FILE_INFO）
 * @param lpOutBuffer 输出缓冲区，用于存储录像文件列表信息
 * @param dwOutBufferSize 输出缓冲区大小
 * @param pdwBytesReturned 输出参数，实际返回的数据长度
 * @return 成功返回TRUE，失败返回FALSE
 * @note 根据查询条件（通道号、类型、时间范围、文件名）获取录像文件列表
 */
static BOOL NetTV_GetRecordFileList_Impl(LPVOID lpUserID,
                                         INT32 dwChannelID,
                                         INT32 dwCommand,
                                         LPVOID lpOutBuffer,
                                         INT32 dwOutBufferSize,
                                         INT32 *pdwBytesReturned)
{
    if (!lpOutBuffer || dwOutBufferSize <= 0)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    if (dwOutBufferSize < (INT32)sizeof(NET_RecordFileList_S))
    {
        CErrorManage::instance()->SetLastError(NET_E_NOENOUGH_BUF);
        return FALSE;
    }

    NET_RecordFileList_S* pCfg = static_cast<NET_RecordFileList_S*>(lpOutBuffer);
    std::ostringstream url;
    url << NET_API_URL_DEVICE_GET_DEV_CONFIG(dwChannelID, dwCommand)
        << "&ChnId=" << pCfg->stFind.nChnId
        << "&Type=" << pCfg->stFind.nType
        << "&Year=" << NetTV_UrlEncode(pCfg->stFind.szYear)
        << "&Month=" << NetTV_UrlEncode(pCfg->stFind.szMonth)
        << "&Date=" << NetTV_UrlEncode(pCfg->stFind.szDate)
        << "&StartTime=" << NetTV_UrlEncode(pCfg->stFind.szStartTime)
        << "&EndTime=" << NetTV_UrlEncode(pCfg->stFind.szEndTime)
        << "&Filename=" << NetTV_UrlEncode(pCfg->stFind.szFilename);

    return CommandExecutor::instance()->ExecuteGet<NET_RecordFileList_S>(lpUserID, url.str(), lpOutBuffer, pdwBytesReturned) ? TRUE : FALSE;
}

/**
 * @brief 获取日志列表实现函数
 * @param lpUserID 用户句柄
 * @param dwChannelID 通道号
 * @param dwCommand 配置命令码（NET_FIND_LOG、NET_EXPORT_LOG）
 * @param lpOutBuffer 输出缓冲区，用于存储日志列表信息
 * @param dwOutBufferSize 输出缓冲区大小
 * @param pdwBytesReturned 输出参数，实际返回的数据长度
 * @return 成功返回TRUE，失败返回FALSE
 * @note 根据查询条件（类型、操作、时间范围、分页）获取日志列表
 */
static BOOL NetTV_GetLogList_Impl(LPVOID lpUserID,
                                  INT32 dwChannelID,
                                  INT32 dwCommand,
                                  LPVOID lpOutBuffer,
                                  INT32 dwOutBufferSize,
                                  INT32 *pdwBytesReturned)
{
    if (!lpOutBuffer || dwOutBufferSize <= 0)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    if (dwOutBufferSize < (INT32)sizeof(NET_LogList_S))
    {
        CErrorManage::instance()->SetLastError(NET_E_NOENOUGH_BUF);
        return FALSE;
    }

    NET_LogList_S* pCfg = static_cast<NET_LogList_S*>(lpOutBuffer);
    INT32 nCurPage = pCfg->stPage.nCurPage == 0 ? 1 : pCfg->stPage.nCurPage;
    INT32 nPageSize = pCfg->stPage.nPageSize <= 0 ? NET_LOG_QUERY_COND_NUM : pCfg->stPage.nPageSize;

    std::ostringstream url;
    url << NET_API_URL_DEVICE_GET_DEV_CONFIG(dwChannelID, dwCommand)
        << "&Type=" << pCfg->stCond.nType
        << "&Action=" << pCfg->stCond.nAction
        << "&StartTime=" << NetTV_UrlEncode(pCfg->stCond.szStartTime)
        << "&EndTime=" << NetTV_UrlEncode(pCfg->stCond.szEndTime)
        << "&CurPage=" << nCurPage
        << "&PageSize=" << nPageSize;

    return CommandExecutor::instance()->ExecuteGet<NET_LogList_S>(lpUserID, url.str(), lpOutBuffer, pdwBytesReturned) ? TRUE : FALSE;
}

/**
 * @brief 回放控制接口
 * @param lpUserID 用户句柄
 * @param pstInfo 输入输出参数，包含回放控制命令（开始/停止/暂停/倍速等）和返回信息
 * @param pdwBytesReturned 输出参数，实际返回的数据长度
 * @return 成功返回TRUE，失败返回FALSE
 * @note 支持回放开始、停止、暂停、倍速等控制操作
 */
NET_API BOOL STDCALL NET_ControlReplay(IN    LPVOID lpUserID,
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
    if (!CommandExecutor::instance()->ExecuteRaw(lpUserID, "POST", NET_API_URL_REPLAY_CONTROL(), body, respBody))
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
 * @param lpUserID 用户句柄
 * @param pstInfo 输入输出参数，包含查询条件和返回的录像列表
 * @param pdwBytesReturned 输出参数，实际返回的数据长度
 * @return 成功返回TRUE，失败返回FALSE
 * @note 获取指定通道、时间范围内的录像片段列表
 */
NET_API BOOL STDCALL NET_GetReplayRecordList(IN    LPVOID lpUserID,
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
    if (!CommandExecutor::instance()->ExecuteRaw(lpUserID, "POST", NET_API_URL_REPLAY_GET_RECORD_LIST(), body, respBody))
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
 * @param lpUserID 用户句柄
 * @param pstInfo 输入输出参数，包含回放条件和返回的URL信息
 * @param pdwBytesReturned 输出参数，实际返回的数据长度
 * @return 成功返回TRUE，失败返回FALSE
 * @note 获取指定录像片段的播放URL
 */
NET_API BOOL STDCALL NET_GetReplayUrl(IN    LPVOID lpUserID,
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
    if (!CommandExecutor::instance()->ExecuteRaw(lpUserID, "POST", NET_API_URL_REPLAY_GET_URL(), body, respBody))
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

/**
 * @brief 设置设备配置模板实现函数
 * @tparam T_CFG 配置结构体类型
 * @param lpUserID 用户句柄
 * @param dwChannelID 通道号
 * @param dwCommand 配置命令码
 * @param lpInBuffer 输入缓冲区，包含要设置的配置信息
 * @param dwInBufferSize 输入缓冲区大小
 * @param pdwBytesReturned 输出参数，实际返回的数据长度
 * @return 成功返回TRUE，失败返回FALSE
 * @note NET_SetDevConfig内部调用此模板函数，根据命令码分发到不同配置类型
 */
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
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    if (dwInBufferSize < (INT32)sizeof(T_CFG))
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    std::string url = NET_API_URL_DEVICE_SET_DEV_CONFIG(dwChannelID, dwCommand);
    BOOL bRet = CommandExecutor::instance()->ExecuteSet<T_CFG>(lpUserID, "POST", url, lpInBuffer) ? TRUE : FALSE;
    if (bRet && pdwBytesReturned != NULL)
    {
        *pdwBytesReturned = 0;
    }
    return bRet;
}

/**
 * @brief 获取设备配置接口（通用）
 * @param lpUserID 用户句柄
 * @param dwChannelID 通道号，设备级配置填0，通道级配置填实际通道号
 * @param dwCommand 配置命令码，决定获取哪种配置
 * @param lpOutBuffer 输出缓冲区，用于存储配置信息
 * @param dwOutBufferSize 输出缓冲区大小
 * @param pdwBytesReturned 输出参数，实际返回的数据长度
 * @return 成功返回TRUE，失败返回FALSE
 * @note 根据dwCommand不同，返回不同类型的配置结构体；支持设备信息、NTP配置、网络配置、报警配置等多种配置类型
 */
NET_API BOOL STDCALL NET_GetDevConfig(IN  LPVOID  lpUserID,
                                            IN    INT32   dwChannelID,
                                            IN    INT32   dwCommand,
                                            INOUT LPVOID  lpOutBuffer,
                                            OUT   INT32   dwOutBufferSize,
                                            OUT   INT32   *pdwBytesReturned)
{
    CHECK_SDK_INIT(FALSE);

    switch (dwCommand)
    {
        case NET_GET_DEVICECFG:
            return NetTV_GetDevConfig_Impl<NET_DeviceBasicInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_NTPCFG:
            return NetTV_GetDevConfig_Impl<NET_SystemNtpInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_AUDIOCFG:
            return NetTV_GetDevConfig_Impl<NET_AudioCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_STREAMCFG:
            return NetTV_GetDevConfig_Impl<NET_VideoEncodeOption_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_OSDCAPCFG:
            return NetTV_GetDevConfig_Impl<NET_VideoOsdCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_IMAGECFG:
            return NetTV_GetDevConfig_Impl<NET_ImageSetting_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_RTSPURLCFG:
            return NetTV_GetDevConfig_Impl<NET_RtspUrlInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_NETWORKCFG:
            return NetTV_GetDevConfig_Impl<NET_NetworkCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_4G_INFO:
            return NetTV_GetDevConfig_Impl<NET_4GInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_HOTSPOT_CONN:
            return NetTV_GetDevConfig_Impl<NET_HotspotConnInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_SECURITY_SERVICES_INFO:
            return NetTV_GetDevConfig_Impl<NET_SecurityServicesInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_SSH_COUNTDOWN:
            return NetTV_GetDevConfig_Impl<NET_SshCountdownInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_FIND_LOG:
        case NET_EXPORT_LOG:
            return NetTV_GetLogList_Impl(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_LOG_SERVER:
            return NetTV_GetDevConfig_Impl<NET_LogServerInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_RECORD_STATUS:
            return NetTV_GetDevConfig_Impl<NET_RecordStatusInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_SD_CARD_STATUS:
            return NetTV_GetDevConfig_Impl<NET_SdCardStatus_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_AUDIBLE_ALARM_INFO:
            return NetTV_GetDevConfig_Impl<NET_AudibleAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_ALARM_INPUT_INFO:
            return NetTV_GetDevConfig_Impl<NET_AlarmInputInfoList_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_ALARM_OUTPUT_INFO:
            return NetTV_GetDevConfig_Impl<NET_AlarmOutputInfoList_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_FLASHING_LIGHT_ALARM_INFO:
            return NetTV_GetDevConfig_Impl<NET_FlashingLightAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_PIR_ALARM_INFO:
            return NetTV_GetDevConfig_Impl<NET_PirAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_RECORD_SCHEDULE:
            return NetTV_GetDevConfig_Impl<NET_RecordSchedule_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_RECORD_ADVANCED_PARAM:
            return NetTV_GetDevConfig_Impl<NET_RecordAdvancedParam_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_FIND_RECORD_FILE_INFO:
            return NetTV_GetRecordFileList_Impl(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_PRIVACYMASKCFG:
            printf("[ClientSDK] GET_PRIVACYMASKCFG cmd=%d, buf=%d, privacy_size=%zu\n",
                   dwCommand, dwOutBufferSize, sizeof(NET_PrivacyMaskCfg_S));
            return NetTV_GetDevConfig_Impl<NET_PrivacyMaskCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_TAMPERALARM:
            return NetTV_GetDevConfig_Impl<NET_TamperAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_MOTIONALARM:
            return NetTV_GetDevConfig_Impl<NET_MotionAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_CROSSLINEALARM:
            return NetTV_GetDevConfig_Impl<NET_CrossLineAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_INTRUSIONALARM:
            return NetTV_GetDevConfig_Impl<NET_IntrusionAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_ENTERREGIONALARM:
            return NetTV_GetDevConfig_Impl<NET_EnterRegionAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_LEAVEREGIONALARM:
            return NetTV_GetDevConfig_Impl<NET_LeaveRegionAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_LOITERINGALARM:
            return NetTV_GetDevConfig_Impl<NET_LoiteringAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_SCENECHANGEALARM:
            return NetTV_GetDevConfig_Impl<NET_SceneChangeAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_CROWDGATHERINGALARM:
            return NetTV_GetDevConfig_Impl<NET_CrowdGatheringAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_GARBAGE_EXPOSURE_CFG:
            return NetTV_GetDevConfig_Impl<NET_GarbageExposureCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_GARBAGE_OVERFLOW_CFG:
            return NetTV_GetDevConfig_Impl<NET_GarbageOverflowCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_PEOPLE_FLOW_STATISTICS_CFG:
            return NetTV_GetDevConfig_Impl<NET_PeopleFlowStatisticsCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_PEOPLE_DENSITY_DETECTION_CFG:
            return NetTV_GetDevConfig_Impl<NET_PeopleDensityDetectionCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_MANHOLE_COVER_ABNORMAL_CFG:
            return NetTV_GetDevConfig_Impl<NET_ManholeCoverAbnormalCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_SLEEP_ON_DUTY_CFG:
            return NetTV_GetDevConfig_Impl<NET_SleepOnDutyCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG:
            return NetTV_GetDevConfig_Impl<NET_ElectricVehicleInElevatorCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_PERSON_FALL_DOWN_CFG:
            return NetTV_GetDevConfig_Impl<NET_PersonFallDownCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_CONSTRUCTION_OCCUPY_ROAD_CFG:
            return NetTV_GetDevConfig_Impl<NET_ConstructionOccupyRoadCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_CONGESTION_CFG:
            return NetTV_GetDevConfig_Impl<NET_CongestionCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_LICENSE_PLATE_RECOGNITION_CFG:
            return NetTV_GetDevConfig_Impl<NET_LicensePlateRecognitionCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_HIGH_ALTITUDE_SEATBELT_CFG:
            return NetTV_GetDevConfig_Impl<NET_HighAltitudeSeatbeltCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_SAFETY_HELMET_CFG:
            return NetTV_GetDevConfig_Impl<NET_SafetyHelmetCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_PERSON_FALL_CFG:
            return NetTV_GetDevConfig_Impl<NET_PersonFallCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_PHONE_USAGE_CFG:
            return NetTV_GetDevConfig_Impl<NET_PhoneUsageCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_SMOKING_CFG:
            return NetTV_GetDevConfig_Impl<NET_SmokingCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_OPEN_FLAME_CFG:
            return NetTV_GetDevConfig_Impl<NET_OpenFlameCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_BARE_SOIL_CFG:
            return NetTV_GetDevConfig_Impl<NET_BareSoilCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_HOLE_PROTECTION_BAR_CFG:
            return NetTV_GetDevConfig_Impl<NET_HoleProtectionBarCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_REFLECTIVE_CLOTHING_CFG:
            return NetTV_GetDevConfig_Impl<NET_ReflectiveClothingCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_PET_RECOGNITION_INFO:
            return NetTV_GetDevConfig_Impl<NET_PetRecognitionInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_CLIMB_FENCE_INFO:
            return NetTV_GetDevConfig_Impl<NET_ClimbFenceInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_DIMISSION_INFO:
            return NetTV_GetDevConfig_Impl<NET_DimissionInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_ILLEGAL_LANE_INFO:
            return NetTV_GetDevConfig_Impl<NET_IllegalLaneInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_RETROGRADE_INFO:
            return NetTV_GetDevConfig_Impl<NET_RetrogradeInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_NONMOTOR_VEHICLE_INTRUSION_INFO:
            return NetTV_GetDevConfig_Impl<NET_NonmotorVehicleIntrusionInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_OCCUPATION_EMERGENCY_INFO:
            return NetTV_GetDevConfig_Impl<NET_OccupationEmergencyInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_PEDESTRIAN_INTRUSION_INFO:
            return NetTV_GetDevConfig_Impl<NET_PedestrianIntrusionInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_SMOKE_FIRE_CFG:
            return NetTV_GetDevConfig_Impl<NET_SmokeFireCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_ROAD_PONDING_CFG:
            return NetTV_GetDevConfig_Impl<NET_RoadPondingCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_PARKINGALARM:
            return NetTV_GetDevConfig_Impl<NET_ParkingAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_UNATTENDEDOBJECTALARM:
            return NetTV_GetDevConfig_Impl<NET_UnattendedObjectAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_OBJECTREMOVALALARM:
            return NetTV_GetDevConfig_Impl<NET_ObjectRemovalAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_AUDIOANOMALYALARM:
            return NetTV_GetDevConfig_Impl<NET_AudioAnomalyAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_PREVIEW_INFO:
            return NetTV_GetDevConfig_Impl<NET_PreviewInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_CHANNEL_INFO:
            return NetTV_GetDevConfig_Impl<NET_ChannelInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_CHANNEL_LIST:
            return NetTV_GetDevConfig_Impl<NET_ChannelList_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_UPGRADESTATUS:
            return NetTV_GetDevConfig_Impl<NET_UpgradeStatus_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_UPGRADEVERSION:
            return NetTV_GetDevConfig_Impl<NET_UpgradeVersion_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_CAPTURE_PLAN_INFO:
            return NetTV_GetDevConfig_Impl<NET_CapturePlanInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_CAPTURE_PARAM_INFO:
            return NetTV_GetDevConfig_Impl<NET_CaptureParamInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_EXPOSURE_INFO:
            return NetTV_GetDevConfig_Impl<NET_ExposureInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_DAYNIGHT_INFO:
            return NetTV_GetDevConfig_Impl<NET_DayNightInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_BACKLIGHT_INFO:
            return NetTV_GetDevConfig_Impl<NET_BackLightInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_DENOISE_INFO:
            return NetTV_GetDevConfig_Impl<NET_DenoiseInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_WHITEBALANCE_INFO:
            return NetTV_GetDevConfig_Impl<NET_WhiteBalanceInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_FROM_STREAM_TALKBACK:
            return NetTV_GetDevConfig_Impl<NET_TalkbackStreamInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_FACECAPTUREINFO:
            return NetTV_GetDevConfig_Impl<NET_FaceCaptureInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_FACECAPTUREOVERLAYINFO:
            return NetTV_GetDevConfig_Impl<NET_FaceCaptureOverlayInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_TARGET_LIB:
            return NetTV_GetDevConfig_Impl<NET_FaceLibList_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_GET_FACE_INFO:
            return NetTV_GetDevConfig_Impl<NET_FaceInfoList_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        default:
            CErrorManage::instance()->SetLastError(NET_E_CMD_NOT_SUPPORT);
            return FALSE;
    }
}

/**
 * @brief 设置设备配置接口（通用）
 * @param lpUserID 用户句柄
 * @param dwChannelID 通道号，设备级配置填0，通道级配置填实际通道号
 * @param dwCommand 配置命令码，决定设置哪种配置
 * @param lpOutBuffer 输入缓冲区，包含要设置的配置信息
 * @param dwOutBufferSize 输入缓冲区大小
 * @param pdwBytesReturned 输出参数，实际返回的数据长度
 * @return 成功返回TRUE，失败返回FALSE
 * @note 根据dwCommand不同，设置不同类型的配置结构体；支持NTP配置、网络配置、报警配置等多种配置类型
 */
NET_API BOOL STDCALL NET_SetDevConfig(IN  LPVOID  lpUserID,
                                            IN    INT32   dwChannelID,
                                            IN    INT32   dwCommand,
                                            INOUT LPVOID  lpOutBuffer,
                                            OUT   INT32   dwOutBufferSize,
                                            OUT   INT32   *pdwBytesReturned)
{
    CHECK_SDK_INIT(FALSE);

    switch (dwCommand)
    {
        case NET_SET_DEVICECFG:
            return NetTV_SetDevConfig_Impl<NET_DeviceBasicInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_NTPCFG:
            return NetTV_SetDevConfig_Impl<NET_SystemNtpInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_AUDIOCFG:
            return NetTV_SetDevConfig_Impl<NET_AudioCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_STREAMCFG:
            return NetTV_SetDevConfig_Impl<NET_VideoEncodeOption_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_OSDCAPCFG:
            return NetTV_SetDevConfig_Impl<NET_VideoOsdCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_IMAGECFG:
            return NetTV_SetDevConfig_Impl<NET_ImageSetting_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_NETWORKCFG:
            return NetTV_SetDevConfig_Impl<NET_NetworkCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_CONFIG_WIFI_STA:
            return NetTV_SetDevConfig_Impl<NET_WifiStaCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_CONNECT_WIFI_STA:
            return NetTV_SetDevConfig_Impl<NET_WifiStaConnect_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_DISCONNECT_WIFI_STA:
            return NetTV_SetDevConfig_Impl<NET_WifiStaConnect_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_4G_INFO:
            return NetTV_SetDevConfig_Impl<NET_4GInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_HOTSPOT_INFO:
            return NetTV_SetDevConfig_Impl<NET_HotspotInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_SECURITY_SERVICES_INFO:
            return NetTV_SetDevConfig_Impl<NET_SecurityServicesInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_LOG_SERVER:
            return NetTV_SetDevConfig_Impl<NET_LogServerInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TEST_LOG_SERVER:
            return NetTV_SetDevConfig_Impl<NET_LogServerInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_CONTROL_RECORD_INFO:
            return NetTV_SetDevConfig_Impl<NET_RecordInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_RECORD_SCHEDULE:
            return NetTV_SetDevConfig_Impl<NET_RecordSchedule_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_RECORD_ADVANCED_PARAM:
            return NetTV_SetDevConfig_Impl<NET_RecordAdvancedParam_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_DOWNLOAD_RECORD_FILE:
            return NetTV_SetDevConfig_Impl<NET_RecordDownloadList_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_PRIVACYMASKCFG:
            printf("[ClientSDK] SET_PRIVACYMASKCFG cmd=%d, buf=%d, privacy_size=%zu\n",
                   dwCommand, dwOutBufferSize, sizeof(NET_PrivacyMaskCfg_S));
            return NetTV_SetDevConfig_Impl<NET_PrivacyMaskCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_TAMPERALARM:
            return NetTV_SetDevConfig_Impl<NET_TamperAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_MOTIONALARM:
            return NetTV_SetDevConfig_Impl<NET_MotionAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_CROSSLINEALARM:
            return NetTV_SetDevConfig_Impl<NET_CrossLineAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_INTRUSIONALARM:
            return NetTV_SetDevConfig_Impl<NET_IntrusionAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_ENTERREGIONALARM:
            return NetTV_SetDevConfig_Impl<NET_EnterRegionAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_LEAVEREGIONALARM:
            return NetTV_SetDevConfig_Impl<NET_LeaveRegionAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_LOITERINGALARM:
            return NetTV_SetDevConfig_Impl<NET_LoiteringAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_SCENECHANGEALARM:
            return NetTV_SetDevConfig_Impl<NET_SceneChangeAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_CROWDGATHERINGALARM:
            return NetTV_SetDevConfig_Impl<NET_CrowdGatheringAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_GARBAGE_EXPOSURE_CFG:
            return NetTV_SetDevConfig_Impl<NET_GarbageExposureCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_GARBAGE_OVERFLOW_CFG:
            return NetTV_SetDevConfig_Impl<NET_GarbageOverflowCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_PEOPLE_FLOW_STATISTICS_CFG:
            return NetTV_SetDevConfig_Impl<NET_PeopleFlowStatisticsCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_RESET_PEOPLE_FLOW_STATISTICS:
            return NetTV_SetDevConfig_Impl<NET_PeopleFlowStatisticsCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_PEOPLE_DENSITY_DETECTION_CFG:
            return NetTV_SetDevConfig_Impl<NET_PeopleDensityDetectionCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_MANHOLE_COVER_ABNORMAL_CFG:
            return NetTV_SetDevConfig_Impl<NET_ManholeCoverAbnormalCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_SLEEP_ON_DUTY_CFG:
            return NetTV_SetDevConfig_Impl<NET_SleepOnDutyCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG:
            return NetTV_SetDevConfig_Impl<NET_ElectricVehicleInElevatorCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_PERSON_FALL_DOWN_CFG:
            return NetTV_SetDevConfig_Impl<NET_PersonFallDownCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_CONSTRUCTION_OCCUPY_ROAD_CFG:
            return NetTV_SetDevConfig_Impl<NET_ConstructionOccupyRoadCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_CONGESTION_CFG:
            return NetTV_SetDevConfig_Impl<NET_CongestionCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_LICENSE_PLATE_RECOGNITION_CFG:
            return NetTV_SetDevConfig_Impl<NET_LicensePlateRecognitionCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_HIGH_ALTITUDE_SEATBELT_CFG:
            return NetTV_SetDevConfig_Impl<NET_HighAltitudeSeatbeltCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_SAFETY_HELMET_CFG:
            return NetTV_SetDevConfig_Impl<NET_SafetyHelmetCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_PERSON_FALL_CFG:
            return NetTV_SetDevConfig_Impl<NET_PersonFallCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_PHONE_USAGE_CFG:
            return NetTV_SetDevConfig_Impl<NET_PhoneUsageCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_SMOKING_CFG:
            return NetTV_SetDevConfig_Impl<NET_SmokingCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_OPEN_FLAME_CFG:
            return NetTV_SetDevConfig_Impl<NET_OpenFlameCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_BARE_SOIL_CFG:
            return NetTV_SetDevConfig_Impl<NET_BareSoilCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_HOLE_PROTECTION_BAR_CFG:
            return NetTV_SetDevConfig_Impl<NET_HoleProtectionBarCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_REFLECTIVE_CLOTHING_CFG:
            return NetTV_SetDevConfig_Impl<NET_ReflectiveClothingCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_PET_RECOGNITION_INFO:
            return NetTV_SetDevConfig_Impl<NET_PetRecognitionInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_CLIMB_FENCE_INFO:
            return NetTV_SetDevConfig_Impl<NET_ClimbFenceInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_DIMISSION_INFO:
            return NetTV_SetDevConfig_Impl<NET_DimissionInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_ILLEGAL_LANE_INFO:
            return NetTV_SetDevConfig_Impl<NET_IllegalLaneInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_RETROGRADE_INFO:
            return NetTV_SetDevConfig_Impl<NET_RetrogradeInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_NONMOTOR_VEHICLE_INTRUSION_INFO:
            return NetTV_SetDevConfig_Impl<NET_NonmotorVehicleIntrusionInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_OCCUPATION_EMERGENCY_INFO:
            return NetTV_SetDevConfig_Impl<NET_OccupationEmergencyInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_PEDESTRIAN_INTRUSION_INFO:
            return NetTV_SetDevConfig_Impl<NET_PedestrianIntrusionInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_SMOKE_FIRE_CFG:
            return NetTV_SetDevConfig_Impl<NET_SmokeFireCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_ROAD_PONDING_CFG:
            return NetTV_SetDevConfig_Impl<NET_RoadPondingCfg_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_PARKINGALARM:
            return NetTV_SetDevConfig_Impl<NET_ParkingAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_UNATTENDEDOBJECTALARM:
            return NetTV_SetDevConfig_Impl<NET_UnattendedObjectAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_OBJECTREMOVALALARM:
            return NetTV_SetDevConfig_Impl<NET_ObjectRemovalAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_AUDIBLE_ALARM_INFO:
            return NetTV_SetDevConfig_Impl<NET_AudibleAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_ALARM_INPUT_INFO:
            return NetTV_SetDevConfig_Impl<NET_AlarmInputInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_ALARM_OUTPUT_INFO:
            return NetTV_SetDevConfig_Impl<NET_AlarmOutputInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_FLASHING_LIGHT_ALARM_INFO:
            return NetTV_SetDevConfig_Impl<NET_FlashingLightAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_PIR_ALARM_INFO:
            return NetTV_SetDevConfig_Impl<NET_PirAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TRIGGER_SOUND_LIGHT_ALARM:
            return NetTV_SetDevConfig_Impl<NET_SoundLightAlarmTrigger_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_AUDIOANOMALYALARM:
            return NetTV_SetDevConfig_Impl<NET_AudioAnomalyAlarmInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_PREVIEW_INFO:
            return NetTV_SetDevConfig_Impl<NET_PreviewInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_UPGRADE:
            return NetTV_SetDevConfig_Impl<NET_UpgradeInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_CAPTURE_PLAN_INFO:
            return NetTV_SetDevConfig_Impl<NET_CapturePlanInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_CAPTURE_PARAM_INFO:
            return NetTV_SetDevConfig_Impl<NET_CaptureParamInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_EXPOSURE_INFO:
            return NetTV_SetDevConfig_Impl<NET_ExposureInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_DAYNIGHT_INFO:
            return NetTV_SetDevConfig_Impl<NET_DayNightInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_BACKLIGHT_INFO:
            return NetTV_SetDevConfig_Impl<NET_BackLightInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_DENOISE_INFO:
            return NetTV_SetDevConfig_Impl<NET_DenoiseInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_WHITEBALANCE_INFO:
            return NetTV_SetDevConfig_Impl<NET_WhiteBalanceInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_STATE_TALKBACK:
            return NetTV_SetDevConfig_Impl<NET_TalkbackStateInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_TO_STREAM_TALKBACK:
            return NetTV_SetDevConfig_Impl<NET_TalkbackStreamInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_REPLAY_TALKBACK:
            return NetTV_SetDevConfig_Impl<NET_ReplayTalkbackInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_FACECAPTUREINFO:
            return NetTV_SetDevConfig_Impl<NET_FaceCaptureInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_FACECAPTUREOVERLAYINFO:
            return NetTV_SetDevConfig_Impl<NET_FaceCaptureOverlayInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_SET_FACE_COMPARE_INFO:
            return NetTV_SetDevConfig_Impl<NET_FaceCompareInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_ADD_TARGET_LIB:
        case NET_DEL_TARGET_LIB:
        case NET_SET_TARGET_LIB:
            return NetTV_SetDevConfig_Impl<NET_FaceLibInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_ADD_FACE_INFO:
        case NET_SET_FACE_INFO:
            return NetTV_SetDevConfig_Impl<NET_FaceInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        case NET_DEL_FACE_INFO:
            return NetTV_SetDevConfig_Impl<NET_FaceIdInfo_S>(lpUserID, dwChannelID, dwCommand, lpOutBuffer, dwOutBufferSize, pdwBytesReturned);
        default:
            CErrorManage::instance()->SetLastError(NET_E_CMD_NOT_SUPPORT);
            return FALSE;
    }
}

/* ==================== 文件上传 ==================== */

/**
 * @brief 文件上传接口
 * @param lpUserID 用户句柄
 * @param szFilePath 本地文件路径
 * @param szRemoteName 远程文件名（上传到设备后的文件名）
 * @return 成功返回TRUE，失败返回FALSE
 * @note 用于上传升级包等文件到设备
 */
BOOL STDCALL
NET_UploadFile(IN LPVOID   lpUserID,
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
    return CommandExecutor::instance()->ExecuteUpload(
        lpUserID, "PUT", url, std::string(szFilePath), ignoreResp);
}

/* ==================== 录像帧流 RecordFrame ==================== */

static std::map<std::string, std::shared_ptr<tvsdk::CRecordFrameClient>> g_recordFrameMap;
static std::mutex g_recordFrameMutex;

/**
 * @brief 启动录像帧流接口
 * @param lpUserID 用户句柄
 * @param pstCond 帧流条件结构体，包含通道号、时间范围等查询条件
 * @param pstStreamInfo 输出参数，返回帧流信息（TCP端口、流ID等）
 * @param cbRecordFrame 帧数据回调函数，用于接收录像帧数据
 * @param lpUserData 用户自定义数据，会在回调时传入
 * @return 成功返回TRUE，失败返回FALSE
 * @note 启动后通过TCP连接接收录像帧数据，需调用NET_TV_StopRecordFrameStream停止
 */
NET_API BOOL STDCALL
NET_StartRecordFrameStream(IN LPVOID lpUserID,
                              IN pNET_RecordFrameStreamCond_S pstCond,
                              OUT pNET_RecordFrameStreamInfo_S pstStreamInfo,
                              IN NET_RecordFrameCallBack cbRecordFrame,
                              IN LPVOID lpUserData)
{
    CHECK_SDK_INIT(FALSE);

    if (!lpUserID || !pstCond || !pstStreamInfo || pstCond->uChannel <= 0 ||
        pstCond->szStartTime[0] == '\0' || pstCond->szEndTime[0] == '\0')
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    if (pstCond->uSize == 0)
    {
        pstCond->uSize = sizeof(NET_RecordFrameStreamCond_S);
    }

    std::string body = SDKConvert::to_string(*pstCond);
    std::string respBody;
    if (!CommandExecutor::instance()->ExecuteRaw((LPUSER_HANDLE)lpUserID,
                                                 "POST",
                                                 NET_API_PATH_RECORD_FRAME_STREAM_START,
                                                 body,
                                                 respBody))
    {
        return FALSE;
    }

    const int respCode = SDKConvert::get_respCode(respBody);
    CErrorManage::instance()->SetLastError(respCode);
    if (respCode != NET_E_SUCCEED)
    {
        return FALSE;
    }

    std::memset(pstStreamInfo, 0, sizeof(*pstStreamInfo));
    SDKConvert::to_respStruct(respBody, *pstStreamInfo);
    if (pstStreamInfo->uSize == 0)
    {
        pstStreamInfo->uSize = sizeof(NET_RecordFrameStreamInfo_S);
    }

    if (pstStreamInfo->uTcpPort == 0 || pstStreamInfo->szStreamId[0] == '\0')
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    auto session = CDeviceManage::instance()->GetSession(lpUserID);
    if (!session)
    {
        CErrorManage::instance()->SetLastError(NET_E_NO_USER);
        return FALSE;
    }

    const std::string host = session->GetHost();
    if (host.empty())
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    auto client = std::make_shared<tvsdk::CRecordFrameClient>();
    tvsdk::RecordFrameCallback cb = [cbRecordFrame, lpUserData](const NET_RecordFrameInfo_S& frameInfo,
                                                               const char* data,
                                                               size_t size) {
        if (cbRecordFrame) {
            cbRecordFrame(&frameInfo, data, static_cast<UINT32>(size), lpUserData);
        }
    };

    if (!client->start(host,
                       static_cast<int>(pstStreamInfo->uTcpPort),
                       pstStreamInfo->szStreamId,
                       std::move(cb)))
    {
        NET_RecordFrameStopInfo_S stStopInfo;
        std::memset(&stStopInfo, 0, sizeof(stStopInfo));
        stStopInfo.uSize = sizeof(stStopInfo);
#ifdef _WIN32
        strncpy_s(stStopInfo.szStreamId, pstStreamInfo->szStreamId, sizeof(stStopInfo.szStreamId) - 1);
#else
        std::strncpy(stStopInfo.szStreamId, pstStreamInfo->szStreamId, sizeof(stStopInfo.szStreamId) - 1);
        stStopInfo.szStreamId[sizeof(stStopInfo.szStreamId) - 1] = '\0';
#endif
        std::string stopResp;
        CommandExecutor::instance()->ExecuteRaw((LPUSER_HANDLE)lpUserID,
                                                "POST",
                                                NET_API_PATH_RECORD_FRAME_STREAM_STOP,
                                                SDKConvert::to_string(stStopInfo),
                                                stopResp);
        CErrorManage::instance()->SetLastError(NET_E_SYSCALL_FALIED);
        return FALSE;
    }

    {
        std::lock_guard<std::mutex> lock(g_recordFrameMutex);
        auto old = g_recordFrameMap.find(pstStreamInfo->szStreamId);
        if (old != g_recordFrameMap.end())
        {
            old->second->stop();
            g_recordFrameMap.erase(old);
        }
        g_recordFrameMap[pstStreamInfo->szStreamId] = client;
    }

    CErrorManage::instance()->SetLastError(NET_E_SUCCEED);
    return TRUE;
}

/**
 * @brief 停止录像帧流接口
 * @param lpUserID 用户句柄
 * @param szStreamId 流ID，由NET_TV_StartRecordFrameStream返回
 * @return 成功返回TRUE，失败返回FALSE
 * @note 停止录像帧流接收，释放相关资源
 */
NET_API BOOL STDCALL
NET_StopRecordFrameStream(IN LPVOID lpUserID,
                             IN const CHAR* szStreamId)
{
    CHECK_SDK_INIT(FALSE);

    if (!lpUserID || !szStreamId || szStreamId[0] == '\0')
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    NET_RecordFrameStopInfo_S stStopInfo;
    std::memset(&stStopInfo, 0, sizeof(stStopInfo));
    stStopInfo.uSize = sizeof(stStopInfo);
#ifdef _WIN32
    strncpy_s(stStopInfo.szStreamId, szStreamId, sizeof(stStopInfo.szStreamId) - 1);
#else
    std::strncpy(stStopInfo.szStreamId, szStreamId, sizeof(stStopInfo.szStreamId) - 1);
    stStopInfo.szStreamId[sizeof(stStopInfo.szStreamId) - 1] = '\0';
#endif

    std::string respBody;
    if (!CommandExecutor::instance()->ExecuteRaw((LPUSER_HANDLE)lpUserID,
                                                 "POST",
                                                 NET_API_PATH_RECORD_FRAME_STREAM_STOP,
                                                 SDKConvert::to_string(stStopInfo),
                                                 respBody))
    {
        return FALSE;
    }

    const int respCode = SDKConvert::get_respCode(respBody);
    CErrorManage::instance()->SetLastError(respCode);

    {
        std::lock_guard<std::mutex> lock(g_recordFrameMutex);
        auto it = g_recordFrameMap.find(szStreamId);
        if (it != g_recordFrameMap.end())
        {
            it->second->stop();
            g_recordFrameMap.erase(it);
        }
    }

    return respCode == NET_E_SUCCEED ? TRUE : FALSE;
}

/* ==================== 语音对讲 VoiceCom ==================== */

static std::map<LPVOID, std::shared_ptr<tvsdk::CVoiceComClient>> g_voiceComMap;
static std::mutex g_voiceComMutex;

/**
 * @brief 语音对讲音频参数校验与规范化
 * @param audioParam 音频参数结构体，会被校验和规范化
 * @return 校验通过返回true，校验失败返回false
 * @note 支持PCM、AAC、G711A、G711U四种音频格式；校验通道数、采样率、位深度、帧间隔、帧大小等参数
 */
static bool normalize_voicecom_audio_param(NET_VoiceComAudioParam_S& audioParam)
{
    if (audioParam.uChannels != 1) {
        return false;
    }

    int bytesPerSample = 0;
    switch (audioParam.enFormat) {
        case NET_AUDIO_FORMAT_PCM:
        {
            if (audioParam.uBitDepth <= 0) {
                audioParam.uBitDepth = 16;
            }
            if (audioParam.uBitDepth != 16) {
                return false;
            }
            switch (audioParam.uSampleRate) {
                case NET_AUDIO_SAMPRATE_8000:
                case NET_AUDIO_SAMPRATE_16000:
                    break;
                default:
                    return false;
            }
            bytesPerSample = audioParam.uBitDepth / 8;
            break;
        }
        case NET_AUDIO_FORMAT_AAC:
        {
            if (audioParam.uSampleRate <= 0) {
                audioParam.uSampleRate = NET_AUDIO_SAMPRATE_16000;
            }
            if (audioParam.uBitDepth <= 0) {
                audioParam.uBitDepth = 16;
            }
            if (audioParam.uFrameIntervalMs <= 0) {
                audioParam.uFrameIntervalMs = 64;
            }
            if (audioParam.uFrameBytes <= 0) {
                audioParam.uFrameBytes = NET_LEN_4096;
            }
            if (audioParam.uFrameBytes > NET_LEN_4096) {
                return false;
            }
            if (audioParam.uBitRate <= 0) {
                audioParam.uBitRate = 48000;
            }
            audioParam.bLittleEndian = TRUE;
            return true;
        }
        case NET_AUDIO_FORMAT_G711A:
        case NET_AUDIO_FORMAT_G711U:
        {
            if (audioParam.uSampleRate != NET_AUDIO_SAMPRATE_8000) {
                return false;
            }
            if (audioParam.uBitDepth <= 0) {
                audioParam.uBitDepth = 8;
            }
            if (audioParam.uBitDepth != 8) {
                return false;
            }
            bytesPerSample = 1;
            break;
        }
        default:
            return false;
    }

    if (audioParam.uFrameIntervalMs <= 0) {
        audioParam.uFrameIntervalMs = 20;
    }
    if (audioParam.uFrameIntervalMs < 10 || audioParam.uFrameIntervalMs > 1000) {
        return false;
    }

    const int frameBytes = audioParam.uSampleRate * audioParam.uChannels *
                           bytesPerSample * audioParam.uFrameIntervalMs / 1000;
    if (frameBytes <= 0 || frameBytes > NET_LEN_4096) {
        return false;
    }

    if (audioParam.uFrameBytes <= 0) {
        audioParam.uFrameBytes = frameBytes;
    }
    if (audioParam.uFrameBytes != frameBytes) {
        return false;
    }

    audioParam.uBitRate = audioParam.uSampleRate * audioParam.uChannels * audioParam.uBitDepth;
    audioParam.bLittleEndian = TRUE;
    return true;
}

/**
 * @brief 启动语音对讲接口
 * @param lpUserID 用户句柄
 * @param pstStartInfo 语音对讲启动信息结构体，包含音频端口、音频参数等
 * @param cbVoiceCom 语音数据回调函数，用于接收设备端发送的语音数据
 * @param lpUserData 用户自定义数据，会在回调时传入
 * @return 成功返回TRUE，失败返回FALSE
 * @note 启动后通过UDP连接传输音频数据；需调用NET_TV_StopVoiceCom停止；音频参数需与设备端保持一致
 */
BOOL STDCALL
NET_StartVoiceCom(IN LPVOID              lpUserID,
                     IN pNET_VoiceComStartInfo_S pstStartInfo,
                     IN NET_VoiceComCallBack cbVoiceCom,
                     IN LPVOID              lpUserData)
{
    if (!lpUserID || !pstStartInfo || pstStartInfo->uAudioPort == 0) {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    NET_VoiceComAudioParam_S audioParam = pstStartInfo->stAudioParam;
    if (!normalize_voicecom_audio_param(audioParam)) {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    auto session = CDeviceManage::instance()->GetSession(lpUserID);
    if (!session) {
        CErrorManage::instance()->SetLastError(NET_E_NO_USER);
        return FALSE;
    }

    const std::string host = session->GetHost();
    if (host.empty()) {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    auto client = std::make_shared<tvsdk::CVoiceComClient>();

    // 绑定C回调到C++ callback
    tvsdk::VoiceComCallback cb = [cbVoiceCom, lpUserData](const char* data, size_t size) {
        if (cbVoiceCom) {
            cbVoiceCom(data, static_cast<unsigned int>(size), lpUserData);
        }
    };

    if (!client->start(host, static_cast<int>(pstStartInfo->uAudioPort), audioParam, std::move(cb))) {
        CErrorManage::instance()->SetLastError(NET_E_SYSCALL_FALIED);
        return FALSE;
    }

    {
        std::lock_guard<std::mutex> lock(g_voiceComMutex);
        auto old = g_voiceComMap.find(lpUserID);
        if (old != g_voiceComMap.end()) {
            old->second->stop();
            g_voiceComMap.erase(old);
        }
        g_voiceComMap[lpUserID] = client;
    }

    return TRUE;
}

/**
 * @brief 语音对讲发送数据接口
 * @param lpUserID 用户句柄
 * @param pData 要发送的语音数据缓冲区
 * @param dwSize 语音数据大小（字节）
 * @return 成功返回TRUE，失败返回FALSE
 * @note 必须在NET_TV_StartVoiceCom成功后调用；音频格式需与启动时配置一致
 */
BOOL STDCALL
NET_VoiceComSendData(IN LPVOID       lpUserID,
                        IN const CHAR*  pData,
                        IN UINT32       dwSize)
{
    if (!lpUserID || !pData || dwSize == 0) {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    std::lock_guard<std::mutex> lock(g_voiceComMutex);
    auto it = g_voiceComMap.find(lpUserID);
    if (it == g_voiceComMap.end() || !it->second->is_running()) {
        CErrorManage::instance()->SetLastError(NET_E_AUDIO_NO_EXISTED);
        return FALSE;
    }

    if (!it->second->send(pData, static_cast<size_t>(dwSize))) {
        CErrorManage::instance()->SetLastError(NET_E_AUDIO_FAILED);
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief 停止语音对讲接口
 * @param lpUserID 用户句柄
 * @return 成功返回TRUE，失败返回FALSE
 * @note 停止语音对讲，释放UDP连接和相关资源；需在NET_TV_StartVoiceCom成功后调用
 */
BOOL STDCALL
NET_StopVoiceCom(IN LPVOID lpUserID)
{
    if (!lpUserID) {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    std::lock_guard<std::mutex> lock(g_voiceComMutex);
    auto it = g_voiceComMap.find(lpUserID);
    if (it == g_voiceComMap.end()) {
        CErrorManage::instance()->SetLastError(NET_E_AUDIO_NO_EXISTED);
        return FALSE;
    }

    it->second->stop();
    g_voiceComMap.erase(it);
    return TRUE;
}

/* ==================== 设备发现 ==================== */

#include "BG6_ZHSJ/DiscoverySearcher.h"

/**
 * @brief 设备发现接口（局域网搜索）
 * @param szInterfaceIP 本地网络接口IP地址，填NULL则在所有接口上搜索
 * @param dwTimeoutMs 搜索超时时间（毫秒）
 * @param pDeviceList 输出参数，存储搜索到的设备信息列表
 * @param nMaxCount 最大搜索设备数量
 * @param pnOutCount 输出参数，实际搜索到的设备数量
 * @return 成功返回TRUE，失败返回FALSE
 * @note 通过UDP广播方式搜索局域网内的设备；无需登录即可调用
 */
BOOL STDCALL
NET_Discovery_Search(IN  const CHAR*                      szInterfaceIP,
                        IN  UINT32                           dwTimeoutMs,
                        OUT NET_DiscoveryDeviceInfo_S*       pDeviceList,
                        IN  int                              nMaxCount,
                        OUT int*                             pnOutCount)
{
    if (!pDeviceList || nMaxCount <= 0 || !pnOutCount) {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    CDiscoverySearcher searcher;
    int ret = searcher.search(szInterfaceIP, dwTimeoutMs, pDeviceList, nMaxCount, pnOutCount);
    if (ret < 0) {
        CErrorManage::instance()->SetLastError(NET_E_SYSCALL_FALIED);
        return FALSE;
    }
    return TRUE;
}
