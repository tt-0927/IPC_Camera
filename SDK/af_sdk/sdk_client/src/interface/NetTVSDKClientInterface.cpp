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
#include <cctype>
#include <cerrno>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <utility>
#include <condition_variable>
#include <unordered_map>

#ifdef _WIN32
#include <process.h>    /* _getpid() */
#define getpid() _getpid()
#else
#include <unistd.h>     /* getpid() */
#endif

#include "NetTVSDKClientInterface.h"
#include "NetTVSDKHttpUrl.h"
#include "CommandExecutor.h"
#include "ErrorManage.h"
#include "SessionManager.h"
#include "UserSession.h"
#include "NetSdkLog.h"
#include "ConfigQuery.h"
#include "RecordFrameHub.h"
#include "VoiceComHub.h"
#include "tvsdkhttplib.h"
#include "BG6_ZHSJ/DiscoveryProber.h"

#define NETTVSDK_MAKE_VERSION(major, minor, rev1, rev2) \
    ((uint32_t)( \
        ((major & 0xFF) << 24) |  /* 主版本左移24位（占高8位） */ \
        ((minor & 0xFF) << 16) |  /* 次版本左移16位（占次高8位） */ \
        ((rev1 & 0xFF) << 8) |    /* 附加版本1左移8位（占第三8位） */ \
        (rev2 & 0xFF)             /* 附加版本2占最低8位 */ \
    ))

#define NETTVSDK_VERSION        NETTVSDK_MAKE_VERSION(1, 0, 0, 0)			/* 当前版本 */

/* ==================== 录像下载任务管理 ====================
 * 异步下载的核心是"任务句柄"：调用方拿到句柄后可查询进度、等待结束或停止。
 * 句柄本体就是 DownloadTask*，真实生命周期由 g_downloadTaskRegistry 中的
 * shared_ptr 保管；句柄一旦从注册表摘除（Release/Stop），裸指针即失效。
 * 这个设计让"查询进度"与"停止任务"可以来自不同线程，而任务对象不会被提前析构。
 */
namespace
{
struct DownloadTask
{
    std::string taskId;      /* 设备端任务标识，作为独立FIFO文件名和停止索引 */
    std::atomic<bool>  cancelRequested{false};
    std::atomic<INT32> status{NET_DOWNLOAD_TASK_CREATED};
    std::atomic<UINT64> downloadedBytes{0};
    std::atomic<UINT64> totalBytes{0};
    std::atomic<INT32> resultError{NET_E_SUCCEED};
    LPVOID userId{nullptr};
    INT32  channelId{-1};
    std::mutex clientMutex;
    httplib::Client* client{nullptr};   /* 仅在下载线程存活期间有效，访问受 clientMutex 保护 */
    std::mutex completeMutex;
    std::condition_variable completeCv;
    bool completed{false};
    std::thread worker;
};

/* 借助 RAII 保证所有异常路径下 task->client 都能被清空，
 * 避免取消线程访问到已经析构的 httplib::Client。 */
struct DownloadClientGuard
{
    DownloadTask* task;

    DownloadClientGuard(DownloadTask* downloadTask, httplib::Client* downloadClient)
        : task(downloadTask)
    {
        if (task)
        {
            std::lock_guard<std::mutex> lock(task->clientMutex);
            task->client = downloadClient;
        }
    }

    ~DownloadClientGuard()
    {
        if (task)
        {
            std::lock_guard<std::mutex> lock(task->clientMutex);
            task->client = nullptr;
        }
    }

    DownloadClientGuard(const DownloadClientGuard&) = delete;
    DownloadClientGuard& operator=(const DownloadClientGuard&) = delete;
};

/* 异步接口工作线程在进入同步下载实现前绑定自己的任务；同步接口调用时保持NULL，
 * 同步实现据此判断是走 /download/replay/ 还是 /download/sdk-replay/。 */
thread_local DownloadTask* g_activeDownloadTask = nullptr;

using DownloadTaskPtr = std::shared_ptr<DownloadTask>;
std::mutex g_downloadTaskRegistryMutex;
std::unordered_map<DownloadTask*, DownloadTaskPtr> g_downloadTaskRegistry;

DownloadTaskPtr FindDownloadTask(LPVOID hTask)
{
    if (!hTask)
    {
        return DownloadTaskPtr();
    }
    std::lock_guard<std::mutex> lock(g_downloadTaskRegistryMutex);
    const auto it = g_downloadTaskRegistry.find(static_cast<DownloadTask*>(hTask));
    return it == g_downloadTaskRegistry.end() ? DownloadTaskPtr() : it->second;
}

/* 原子地把任务从注册表摘除；摘除后其它线程再也拿不到该任务。
 * 返回的 shared_ptr 保证已经进入其它接口的线程可安全完成本次访问。 */
DownloadTaskPtr TakeDownloadTask(LPVOID hTask)
{
    if (!hTask)
    {
        return DownloadTaskPtr();
    }
    std::lock_guard<std::mutex> lock(g_downloadTaskRegistryMutex);
    const auto it = g_downloadTaskRegistry.find(static_cast<DownloadTask*>(hTask));
    if (it == g_downloadTaskRegistry.end())
    {
        return DownloadTaskPtr();
    }
    DownloadTaskPtr task = it->second;
    g_downloadTaskRegistry.erase(it);
    return task;
}
}

/**
 * @brief 生成设备端下载任务标识
 * @return 仅包含 [A-Za-z0-9_-] 的全局唯一字符串
 * @note  taskId 会作为设备端 FIFO 文件名（/tmp/sdk_download_<taskId>.pipe）和取消任务的索引key，
 *        因此字符集必须落在设备端白名单 [A-Za-z0-9_-] 内。
 *        不能只使用"静态变量地址 + 进程内序号"：两个 SDK 客户端进程通常会得到
 *        相同的地址和序号，从而共用同一个 pipe，取消一个任务会误伤另一个任务。
 *        这里混入 pid 与高精度时钟，保证跨进程、跨重启都不重复。
 */
static std::string NetTV_MakeDownloadTaskId()
{
    static std::atomic<UINT64> sequence{0};
    static const UINT64 processSalt =
        static_cast<UINT64>(::getpid()) ^
        static_cast<UINT64>(std::chrono::high_resolution_clock::now()
                                .time_since_epoch().count());
    const UINT64 value = ++sequence;
    const UINT64 now = static_cast<UINT64>(std::chrono::high_resolution_clock::now()
                                                .time_since_epoch().count());

    std::ostringstream stream;
    stream << std::hex << processSalt << "_" << now << "_" << value;
    return stream.str();
}

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
 * @param dwLogLevel 日志输出等级（spdlog 阈值语义，数值越大输出越少）：0-TRACE 全量(最详细)，1-DEBUG，2-INFO，3-WARN(仅 warn/error)，4-ERROR(仅 error)；无“关闭”档，0 为全量输出
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
 * @return 成功返回TRUE，失败返回FALSE
 */
NET_API BOOL STDCALL NET_clientSetExceptionCallBack(IN NET_ExceptionCallBack_PF cbExceptionCallBack,
                                                                 IN LPVOID lpUserData)
{
	CHECK_SDK_INIT(FALSE);

	auto pDevMgr = CSessionManager::instance();
	if (!pDevMgr)
	{
		return FALSE;
	}

	pDevMgr->SetExceptionCallBack(cbExceptionCallBack, lpUserData);
	return TRUE;
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
	return TRUE;
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
	return TRUE;
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
    if (!lpUserID)
    {
        CErrorManage::instance()->SetLastError(NET_E_NO_USER);
        NETSDK_LOG_MESSAGE_ERROR("[NetTVSDK] NET_clientLogin failed, check username/password");
        return NULL;
    }
    NETSDK_LOG_MESSAGE_INFO("[NetTVSDK] NET_clientLogin returned, userID=%p", lpUserID);

	/* 发送获取设备信息命令 */
	{
		if(!CCommandExecutor::instance()->ExecuteGet<NET_DeviceInfo_S>(lpUserID,NET_API_PATH_DEVICE_GETINFO,pstDevInfo,NULL))
		{
            NETSDK_LOG_MESSAGE_ERROR("[NetTVSDK] NET_clientLogin failed to get device info");
            pDevMgr->Logout(lpUserID);
			return NULL;
		}

		/* 将设备类型写入 Session，供后续 ConfigQuery 做命令支持性校验与专属结构体分发 */
		std::shared_ptr<CUserSession> pSession = pDevMgr->GetSession(static_cast<LPUSER_HANDLE>(lpUserID));
		if (pSession && pstDevInfo)
		{
			pSession->SetDeviceType(pstDevInfo->uDevType);
			NETSDK_LOG_MESSAGE_INFO("[NetTVSDK] NET_clientLogin device type=%d, model=%s",
							pstDevInfo->uDevType, pstDevInfo->strDevModel);
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

    if (!lpUserID || !pstCtrlInfo ||
        pstCtrlInfo->uControlType <= 0 || pstCtrlInfo->uCommand <= 0 ||
        pstCtrlInfo->uDurationMs < 0)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }
    
    if (pstCtrlInfo->uControlType < NET_DEVICE_CTRL_TYPE_REBOOT &&
        pstCtrlInfo->uChannelID < 0)
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

/**
 * @brief 修改用户密码（需旧密码校验）
 */
NET_API BOOL STDCALL NET_clientSetUserPassword(IN LPVOID lpUserID,
                                               IN pNET_UserPasswordInfo_S pstInfo)
{
    CHECK_SDK_INIT(FALSE);

    if (!pstInfo || !strlen(pstInfo->strUserName) ||
        !strlen(pstInfo->strOldPassword) || !strlen(pstInfo->strNewPassword))
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    return CConfigQuery::instance()->SetDevConfig(lpUserID, NET_API_PARAM_NVRCHN, NET_SET_USERPASSWORD,
                                                        pstInfo, sizeof(NET_UserPasswordInfo_S), NULL);
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

    /* 通道 0 合法：与 sdk_old/6860 一致，只有 < 0 才判非法参数。
     * （此前写成 <=0 会把 0 号通道直接挡在本地，设备端根本收不到请求。） */
    if (pstInfo->uChannel < 0)
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

    /* 通道 0 合法：与 sdk_old/6860 一致，只有 < 0 才判非法参数。
     * （此前写成 <=0 会把 0 号通道直接挡在本地，设备端根本收不到请求。） */
    if (pstInfo->uChannel < 0)
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
 * @brief 分页查询真实录像文件
 * @note  检索条件三组互斥（StartTime/EndTime、Date、Year(+Month)），设备侧只接受一组，
 *        同时下发会被判为非法参数（102 Invalid parameter，3588 侧日志为
 *        "invalid record datetime range"）。为兼容"应用层把已知字段一次填满"的调用习惯，
 *        本接口在发送前按优先级裁剪：时间段 > 单日 > 整月 > 整年。
 */
NET_API BOOL STDCALL NET_clientQueryRecordFiles(IN    LPVOID lpUserID,
                                                INOUT pNET_RecordFileQuery_S pstQuery,
                                                OUT   INT32 *pdwBytesReturned)
{
    CHECK_SDK_INIT(FALSE);

    if (!pstQuery || !pstQuery->pResults || pstQuery->nResultCapacity <= 0 ||
        pstQuery->stFind.nChnId < 0 || pstQuery->stPage.nCurPage <= 0 ||
        pstQuery->stPage.nPageSize <= 0 ||
        pstQuery->stPage.nPageSize > pstQuery->nResultCapacity ||
        pstQuery->stPage.nPageSize > NET_RECORD_QUERY_PAGE_MAX_NUM)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    pstQuery->nResultCount = 0;
    pstQuery->stPage.nDataTotal = 0;
    pstQuery->stPage.nPageTotal = 0;
    pstQuery->stPage.bHasMore = FALSE;
    std::memset(pstQuery->pResults, 0,
                static_cast<size_t>(pstQuery->nResultCapacity) * sizeof(NET_RecordFindResult_S));

    /* 条件互斥裁剪：只保留优先级最高的一组，避免设备侧报 102。
     * 下面这个裁剪对 3588 与 6860 都成立 —— 两端的校验规则完全相同。 */
    if (pstQuery->stFind.szStartTime[0] != '\0' && pstQuery->stFind.szEndTime[0] != '\0')
    {
        pstQuery->stFind.szDate[0] = '\0';
        pstQuery->stFind.szYear[0] = '\0';
        pstQuery->stFind.szMonth[0] = '\0';
    }
    else if (pstQuery->stFind.szDate[0] != '\0')
    {
        pstQuery->stFind.szStartTime[0] = '\0';
        pstQuery->stFind.szEndTime[0] = '\0';
        pstQuery->stFind.szYear[0] = '\0';
        pstQuery->stFind.szMonth[0] = '\0';
    }
    else if (pstQuery->stFind.szYear[0] != '\0')
    {
        pstQuery->stFind.szStartTime[0] = '\0';
        pstQuery->stFind.szEndTime[0] = '\0';
        pstQuery->stFind.szDate[0] = '\0';
    }

    std::string body = SDKConvert::to_string(*pstQuery);
    std::string respBody;
    if (!CCommandExecutor::instance()->ExecuteRaw(lpUserID, "POST", NET_API_URL_RECORD_QUERY_FILES(), body, respBody))
    {
        return FALSE;
    }

    const int nRespCode = SDKConvert::get_respCode(respBody);
    if (nRespCode != NET_E_SUCCEED)
    {
        CErrorManage::instance()->SetLastError(nRespCode);
        return FALSE;
    }

    SDKConvert::to_respStruct(respBody, *pstQuery);
    if (pdwBytesReturned)
    {
        *pdwBytesReturned = sizeof(NET_RecordFileQuery_S);
    }

    return TRUE;
}

/* ==================== 录像下载 ====================
 * 数据流：
 *   客户端 GET /download/replay/            （同步，全局共享管道）
 *   客户端 GET /download/sdk-replay/?taskId （异步，每任务独立管道）
 *     → control 设置响应头(video/mkv) 并以 chunked 流式输出
 *     → 从命名管道读取 file_system 的转码数据，管道EOF后结束响应
 *   客户端：httplib ContentReceiver 逐段接收，写入本地文件 + 触发进度回调
 */

/**
 * @brief 下载录像文件到本地（同步接口）
 * @note  异步任务工作线程会通过 g_activeDownloadTask 绑定自己的任务，
 *        从而改走 /download/sdk-replay/ 独立管道；同步调用时该指针为NULL，
 *        继续走 /download/replay/ 共享管道以保持历史行为不变。
 */
NET_API BOOL STDCALL NET_clientDownloadRecordFile(IN LPVOID lpUserID,
                                                  IN INT32 nChannelID,
                                                  IN const CHAR* szDate,
                                                  IN INT32 nStartTime,
                                                  IN INT32 nEndTime,
                                                  IN const CHAR* szSavePath,
                                                  IN NET_DownloadProgressCb cbProgress,
                                                  IN LPVOID lpUserData)
{
    CHECK_SDK_INIT(FALSE);

    /* 参数校验 */
    if (!lpUserID || nChannelID < 0 ||
        !szDate || szDate[0] == '\0' ||
        nStartTime < 0 || nEndTime <= nStartTime ||
        !szSavePath || szSavePath[0] == '\0')
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    /* 获取已登录设备的IP地址 */
    auto session = CSessionManager::instance()->GetSession((LPUSER_HANDLE)lpUserID);
    if (!session)
    {
        CErrorManage::instance()->SetLastError(NET_E_NO_USER);
        return FALSE;
    }
    const std::string strHost = session->GetHost();
    if (strHost.empty())
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    NETSDK_LOG_MESSAGE_INFO("[NetTVSDK] NET_clientDownloadRecordFile start, host=%s:%d, chn=%d, date=%s, [%d-%d], save=%s",
                            strHost.c_str(), NET_DOWNLOAD_HTTP_PORT, nChannelID, szDate,
                            nStartTime, nEndTime, szSavePath);

    /*
     * 日期 "2026-08-24" → "20260824"，秒数 56286 → "153806"(HHMMSS)，
     * 拼成设备端下载服务期望的 "20260824_153806"。
     */
    auto format_datetime = [](const char* szDateIn, INT32 nSeconds) -> std::string {
        std::string strDate;
        for (const char* p = szDateIn; p && *p; ++p)
        {
            if (*p != '-')
            {
                strDate += *p;
            }
        }
        int nHour = nSeconds / 3600;
        int nMin = (nSeconds % 3600) / 60;
        int nSec = nSeconds % 60;
        char szTime[16] = {0};
        std::snprintf(szTime, sizeof(szTime), "%02d%02d%02d", nHour, nMin, nSec);
        return strDate + "_" + szTime;
    };

    const std::string strStartTime = format_datetime(szDate, nStartTime);
    const std::string strEndTime = format_datetime(szDate, nEndTime);

    /*
     * decode 参数由本地保存路径后缀推断（mkv/mp4/mov/ts，其他按 mkv），
     * 设备侧 file_system 的 ffmpeg 据此选择输出封装格式。
     */
    std::string strDecode;
    {
        std::string savePath(szSavePath);
        size_t dotPos = savePath.find_last_of('.');
        if (dotPos != std::string::npos) {
            std::string ext = savePath.substr(dotPos + 1);
            for (auto& c : ext) c = static_cast<char>(tolower(c));
            if (ext == "mkv" || ext == "mp4" || ext == "mov" || ext == "ts") {
                strDecode = ext;
            }
        }
    }

    /* 工作线程已预先绑定任务；同步接口则为NULL。 */
    DownloadTask* task = g_activeDownloadTask;

    /* 异步任务走独立路由并携带 taskId；同步接口继续使用旧路由。 */
    std::string strPath;
    if (task)
    {
        strPath = NET_API_URL_SDK_RECORD_DOWNLOAD(nChannelID, strStartTime, strEndTime, task->taskId);
    }
    else
    {
        strPath = NET_API_URL_RECORD_DOWNLOAD(nChannelID, strStartTime, strEndTime);
    }
    if (!strDecode.empty()) {
        strPath += "&decode=" + strDecode;
    }

    NETSDK_LOG_MESSAGE_INFO("[NetTVSDK] NET_clientDownloadRecordFile GET %s:%d%s",
                            strHost.c_str(), NET_DOWNLOAD_HTTP_PORT, strPath.c_str());

    /* 直连设备 control 进程的HTTP下载端口 */
    httplib::Client client(strHost, NET_DOWNLOAD_HTTP_PORT);
    /* 录像可能较长：连接30秒，接收最长2小时，发送30秒 */
    client.set_connection_timeout(30);
    client.set_read_timeout(7200);
    client.set_write_timeout(30);

    /* 打开本地保存文件 */
    std::ofstream outFile(szSavePath, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open())
    {
        NETSDK_LOG_MESSAGE_ERROR("[NetTVSDK] NET_clientDownloadRecordFile open save file failed: %s", szSavePath);
        CErrorManage::instance()->SetLastError(NET_E_TRANSFILE_FAIL);
        return FALSE;
    }

    /* HTTP响应状态记录：默认失败，ResponseHandler中置为成功 */
    std::atomic<int>  httpStatus{0};
    /* 累计已下载字节数：用于进度回调与状态查询 */
    std::atomic<UINT64> downloaded{0};
    DownloadClientGuard clientGuard(task, &client);

    /*
     * 总字节数估算：设备以 chunked 方式输出，通常拿不到真实总长，
     * 因此先按时长 × 3Mbps（安防主码流H264 1080P实测典型值）估算，
     * 后续若响应头给出合法 Content-Length 则以响应头为准。
     * 估算值仅用于进度展示，不能作为下载完成条件。
     */
    UINT64 totalBytes = 0;
    {
        INT32 durationSec = nEndTime - nStartTime;
        if (durationSec <= 0) durationSec = 1;
        const UINT64 kBitrateBps = 3ULL * 1024ULL * 1024ULL;  /* 3 Mbps */
        UINT64 est = static_cast<UINT64>(durationSec) * kBitrateBps / 8ULL;
        if (est < 1024ULL * 1024ULL) est = 1024ULL * 1024ULL;
        totalBytes = est;
    }

    /*
     * 响应头处理：取HTTP状态码，并尝试用 Content-Length / X-Total-Estimate-Bytes
     * 覆盖估算总量（非法值一律忽略，保留估算值）。
     */
    auto respHandler = [&httpStatus, &totalBytes](const httplib::Response& resp) -> bool {
        httpStatus = resp.status;
        const UINT64 MAX_REASONABLE = 16384ULL * 1024ULL * 1024ULL * 1024ULL; /* 16TB */

        auto try_apply_header = [&](const char* headerName) -> bool {
            auto it = resp.headers.find(headerName);
            if (it == resp.headers.end()) return false;
            const std::string& s = it->second;
            try {
                UINT64 v = static_cast<UINT64>(std::stoull(s));
                if (v > 0 && v <= MAX_REASONABLE) {
                    totalBytes = v;
                    return true;
                }
                NETSDK_LOG_MESSAGE_WARN("[NetTVSDK] NET_clientDownloadRecordFile ignore invalid %s=%s",
                                        headerName, s.c_str());
                return false;
            } catch (...) {
                NETSDK_LOG_MESSAGE_WARN("[NetTVSDK] NET_clientDownloadRecordFile %s parse failed: %s",
                                        headerName, s.c_str());
                return false;
            }
        };

        if (!try_apply_header("Content-Length")) {
            try_apply_header("X-Total-Estimate-Bytes");
        }

        NETSDK_LOG_MESSAGE_INFO("[NetTVSDK] NET_clientDownloadRecordFile resp status=%d, content-type=%s, total(estimate/header)=%llu",
                                resp.status,
                                resp.get_header_value("Content-Type").c_str(),
                                (unsigned long long)totalBytes);
        return true;
    };

    /* 跨lambda共享"本地写文件是否真的失败" */
    std::atomic<bool> bWriteFileFailed{false};
    std::atomic<int>  bWriteFileErrno{0};

    /*
     * 数据接收回调：逐段写文件 + 进度回调。
     * 取消请求置位后立即返回false，让httplib尽快中止本次传输。
     */
    auto contentReceiver = [&outFile, &downloaded, &totalBytes, &bWriteFileFailed, &bWriteFileErrno,
                           cbProgress, lpUserData, task](
                                const char* data, size_t size) -> bool {
        if (task && task->cancelRequested.load())
        {
            return false;
        }
        if (size == 0) {
            return true; /* 0字节chunk（如终止符），不处理，继续 */
        }
        if (!outFile.is_open()) {
            return false;
        }
        outFile.write(data, static_cast<std::streamsize>(size));
        if (outFile.good())
        {
            downloaded += static_cast<UINT64>(size);

            /* 总量在下载结束前不动态扩展，避免进度从100%回退。 */
            if (task)
            {
                task->downloadedBytes = downloaded.load();
                task->totalBytes = totalBytes;
            }
            if (cbProgress)
            {
                /* 回调始终报告真实下载量；百分比上限由调用方处理。 */
                cbProgress(downloaded.load(), totalBytes, lpUserData);
            }
            return true;
        }
        else
        {
            int e = errno;
            bWriteFileFailed = true;
            bWriteFileErrno  = e;
            NETSDK_LOG_MESSAGE_ERROR("[NetTVSDK] NET_clientDownloadRecordFile write file FAILED at %llu bytes, size_request=%zu, errno=%d",
                                     (unsigned long long)downloaded.load(), size, e);
            return false; /* 通知httplib取消传输 */
        }
    };

    if (task)
    {
        task->status = NET_DOWNLOAD_TASK_RUNNING;
    }
    auto res = client.Get(strPath.c_str(), respHandler, contentReceiver);

    /* 关闭本地文件（强制flush）；clientGuard负责所有异常路径的client清理。 */
    outFile.flush();
    outFile.close();

    UINT64 dl = downloaded.load();

    /* ==================== 结束判断逻辑 ====================
     * 1. 本地写文件失败：铁定失败
     * 2. HTTP状态码非200：铁定失败（409专指通道忙）
     * 3. httplib错误：全部失败。SDK专用下载接口使用合法的HTTP chunked结束标记，
     *    正常完成只能得到 Success(0)。不能因已收到部分数据就把 Read/ConnectionClosed
     *    当作成功，否则 RFS 读取失败、服务端异常关闭或网络中断都会生成被误判为
     *    成功的残缺录像。
     * 4. HTTP 200 + httplib Success + 0字节：警告但仍返回成功（空时间段或未生成帧）
     */

    /* #1 本地写文件失败 */
    if (bWriteFileFailed)
    {
        NETSDK_LOG_MESSAGE_ERROR("[NetTVSDK] NET_clientDownloadRecordFile aborted: local file write failed, errno=%d, downloaded=%llu bytes, save=%s",
                                 (int)bWriteFileErrno, (unsigned long long)dl, szSavePath);
        if (task) task->resultError = NET_E_TRANSFILE_FAIL;
        CErrorManage::instance()->SetLastError(NET_E_TRANSFILE_FAIL);
        return FALSE;
    }

    /* #2 HTTP状态码非200 */
    if (httpStatus != 0 && httpStatus != NET_HTTP_RESP_CODE_SUCCESS)
    {
        /* 409 = 该通道已有下载任务在跑。设备端对同一通道做了互斥（rfs 库的读/搜索
         * 状态按通道共享，同通道并行会让后者输出残缺），这里单独报"忙"，
         * 让调用方在前一个任务结束后重试，而不是笼统的传输失败。 */
        if (httpStatus.load() == NET_HTTP_RESP_CODE_CONFLICT)
        {
            NETSDK_LOG_MESSAGE_WARN("[NetTVSDK] NET_clientDownloadRecordFile aborted: channel busy (http 409), downloaded=%llu bytes",
                                    (unsigned long long)dl);
            if (task) task->resultError = NET_E_BUSY;
            CErrorManage::instance()->SetLastError(NET_E_BUSY);
            return FALSE;
        }
        NETSDK_LOG_MESSAGE_ERROR("[NetTVSDK] NET_clientDownloadRecordFile aborted: http status=%d, downloaded=%llu bytes",
                                 httpStatus.load(), (unsigned long long)dl);
        if (task) task->resultError = NET_E_TRANSFILE_FAIL;
        CErrorManage::instance()->SetLastError(NET_E_TRANSFILE_FAIL);
        return FALSE;
    }

    /* #3 HTTP流读取错误。正常 chunked EOF 会返回有效响应，不会进入此分支。 */
    if (!res)
    {
        if (task && task->cancelRequested.load())
        {
            task->status = NET_DOWNLOAD_TASK_CANCELED;
            task->resultError = NET_E_TRANSFILE_FAIL;
            CErrorManage::instance()->SetLastError(NET_E_TRANSFILE_FAIL);
            return FALSE;
        }
        const int errCode = static_cast<int>(res.error());
        NETSDK_LOG_MESSAGE_ERROR("[NetTVSDK] NET_clientDownloadRecordFile httplib error=%d, downloaded=%llu bytes, httpStatus=%d; partial file is not successful",
                                 errCode, (unsigned long long)dl, httpStatus.load());
        if (task) task->resultError = NET_E_TRANSFILE_FAIL;
        CErrorManage::instance()->SetLastError(NET_E_TRANSFILE_FAIL);
        return FALSE;
    }

    /* #4 成功（可能是0字节） */
    if (dl == 0)
    {
        NETSDK_LOG_MESSAGE_WARN("[NetTVSDK] NET_clientDownloadRecordFile success but 0 bytes downloaded (empty segment?), status=%d",
                                httpStatus.load());
    }

    /* 下载真正结束后才发送一次"完成"回调，避免估算总量造成提前100%。 */
    if (task && task->cancelRequested.load())
    {
        task->status = NET_DOWNLOAD_TASK_CANCELED;
        task->resultError = NET_E_TRANSFILE_FAIL;
        CErrorManage::instance()->SetLastError(NET_E_TRANSFILE_FAIL);
        return FALSE;
    }
    if (dl > 0 && cbProgress)
    {
        cbProgress(dl, dl, lpUserData);
    }
    if (task)
    {
        task->downloadedBytes = dl;
        task->totalBytes = dl;
        task->status = NET_DOWNLOAD_TASK_SUCCEEDED;
        task->resultError = NET_E_SUCCEED;
    }

    NETSDK_LOG_MESSAGE_INFO("[NetTVSDK] NET_clientDownloadRecordFile success, downloaded=%llu bytes, save=%s",
                            (unsigned long long)dl, szSavePath);

    CErrorManage::instance()->SetLastError(NET_E_SUCCEED);
    return TRUE;
}

/**
 * @brief 异步启动录像下载
 */
NET_API BOOL STDCALL NET_clientStartDownloadRecordFile(IN  LPVOID lpUserID,
                                                       IN  INT32 nChannelID,
                                                       IN  const CHAR* szDate,
                                                       IN  INT32 nStartTime,
                                                       IN  INT32 nEndTime,
                                                       IN  const CHAR* szSavePath,
                                                       IN  NET_DownloadProgressCb cbProgress,
                                                       IN  LPVOID lpUserData,
                                                       OUT LPVOID* phTask)
{
    if (!phTask)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }
    *phTask = NULL;

    /* 异步接口在创建线程前完成参数和SDK状态校验，避免"启动成功"后才发现参数错误。 */
    CHECK_SDK_INIT(FALSE);
    if (!lpUserID || nChannelID < 0 ||
        !szDate || szDate[0] == '\0' ||
        nStartTime < 0 || nEndTime <= nStartTime ||
        !szSavePath || szSavePath[0] == '\0')
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    DownloadTaskPtr task = std::make_shared<DownloadTask>();
    DownloadTask* rawTask = task.get();
    rawTask->userId = lpUserID;
    rawTask->channelId = nChannelID;
    rawTask->taskId = NetTV_MakeDownloadTaskId();
    try
    {
        rawTask->worker = std::thread([rawTask, lpUserID, nChannelID, date = std::string(szDate),
                                       nStartTime, nEndTime, path = std::string(szSavePath),
                                       cbProgress, lpUserData]() {
            g_activeDownloadTask = rawTask;
            BOOL result = FALSE;
            INT32 workerError = NET_E_SUCCEED;
            try
            {
                rawTask->status = NET_DOWNLOAD_TASK_RUNNING;
                /* 若用户在请求真正发起前就取消了，则不再发起下载，避免出现
                 * "stop() 早于 Get() 生效" 导致连接被拉起的窗口。 */
                if (rawTask->cancelRequested.load())
                {
                    workerError = NET_E_TRANSFILE_FAIL;
                    CErrorManage::instance()->SetLastError(workerError);
                    result = FALSE;
                }
                else
                {
                    result = NET_clientDownloadRecordFile(lpUserID, nChannelID, date.c_str(),
                                                          nStartTime, nEndTime, path.c_str(),
                                                          cbProgress, lpUserData);
                    workerError = CErrorManage::instance()->GetLastError();
                }
            }
            catch (...)
            {
                workerError = NET_E_TRANSFILE_FAIL;
                CErrorManage::instance()->SetLastError(workerError);
                result = FALSE;
            }
            g_activeDownloadTask = nullptr;
            if (rawTask->cancelRequested.load())
            {
                rawTask->status = NET_DOWNLOAD_TASK_CANCELED;
                rawTask->resultError = (workerError == NET_E_SUCCEED)
                                           ? NET_E_TRANSFILE_FAIL
                                           : workerError;
            }
            else
            {
                rawTask->status = result ? NET_DOWNLOAD_TASK_SUCCEEDED
                                         : NET_DOWNLOAD_TASK_FAILED;
                rawTask->resultError = result
                                           ? NET_E_SUCCEED
                                           : ((workerError == NET_E_SUCCEED)
                                                  ? NET_E_TRANSFILE_FAIL
                                                  : workerError);
            }
            {
                std::lock_guard<std::mutex> lock(rawTask->completeMutex);
                rawTask->completed = true;
            }
            rawTask->completeCv.notify_all();
        });
    }
    catch (...)
    {
        CErrorManage::instance()->SetLastError(NET_E_CREATE_THREAD_FAIL);
        return FALSE;
    }

    {
        std::lock_guard<std::mutex> lock(g_downloadTaskRegistryMutex);
        g_downloadTaskRegistry.emplace(rawTask, task);
    }
    *phTask = rawTask;
    CErrorManage::instance()->SetLastError(NET_E_SUCCEED);
    return TRUE;
}

/**
 * @brief 请求取消指定的客户端下载任务
 */
NET_API BOOL STDCALL NET_clientCancelDownloadRecordFile(IN LPVOID hTask)
{
    DownloadTaskPtr task = FindDownloadTask(hTask);
    if (!task)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }
    INT32 status = task->status.load();
    if (status == NET_DOWNLOAD_TASK_SUCCEEDED ||
        status == NET_DOWNLOAD_TASK_FAILED ||
        status == NET_DOWNLOAD_TASK_CANCELED)
    {
        return TRUE;
    }

    /* 先置位，避免设备停止请求返回前继续写入本地文件。 */
    task->cancelRequested = true;

    /* 通过 SDK 专用 DeviceControl/CUSTOM 入口把 taskId 放进 szExt 转发给设备，
     * 由 control 再发送给 file_system，只停止当前SDK任务，不影响其它任务。 */
    NET_DeviceControlInfo_S stopInfo;
    std::memset(&stopInfo, 0, sizeof(stopInfo));
    stopInfo.uSize = sizeof(stopInfo);
    stopInfo.uChannelID = task->channelId;
    stopInfo.uControlType = NET_DEVICE_CTRL_TYPE_CUSTOM;
    stopInfo.uCommand = NET_CUSTOM_CTRL_STOP_SDK_DOWNLOAD_RECORD_FILE;
    stopInfo.uParam1 = -1;
    stopInfo.uParam2 = 0;
    if (task->taskId.size() >= sizeof(stopInfo.szExt))
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }
    std::memcpy(stopInfo.szExt, task->taskId.c_str(), task->taskId.size() + 1);

    BOOL remoteStopOk = NET_clientDeviceControl(task->userId, &stopInfo);
    if (!remoteStopOk)
    {
        const INT32 remoteError = CErrorManage::instance()->GetLastError();
        NETSDK_LOG_MESSAGE_WARN("[NetTVSDK] NET_clientCancelDownloadRecordFile: device stop failed, channel=%d, error=%d",
                                task->channelId, remoteError);
        CErrorManage::instance()->SetLastError(remoteError);
    }

    /* 无论设备端请求是否成功，都必须停止本地HTTP接收，避免句柄/线程泄漏。 */
    {
        std::lock_guard<std::mutex> lock(task->clientMutex);
        if (task->client)
        {
            task->client->stop();
        }
    }

    /* 返回值表示取消请求已被本地接受；设备端失败通过日志和 LastError 暴露。 */
    return TRUE;
}

/**
 * @brief 获取指定下载任务的状态与进度快照
 */
NET_API BOOL STDCALL NET_clientGetDownloadRecordFileStatus(IN  LPVOID hTask,
                                                           OUT INT32* pnStatus,
                                                           OUT UINT64* pdwDownloadedBytes,
                                                           OUT UINT64* pdwTotalBytes)
{
    DownloadTaskPtr task = FindDownloadTask(hTask);
    if (!task)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }
    if (pnStatus) *pnStatus = task->status.load();
    if (pdwDownloadedBytes) *pdwDownloadedBytes = task->downloadedBytes.load();
    if (pdwTotalBytes) *pdwTotalBytes = task->totalBytes.load();
    return TRUE;
}

/**
 * @brief 等待下载任务结束
 */
NET_API BOOL STDCALL NET_clientWaitDownloadRecordFile(IN LPVOID hTask,
                                                      IN UINT32 dwTimeoutMs)
{
    DownloadTaskPtr task = FindDownloadTask(hTask);
    if (!task)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }
    std::unique_lock<std::mutex> lock(task->completeMutex);
    if (dwTimeoutMs == 0xFFFFFFFFU)
    {
        task->completeCv.wait(lock, [task] { return task->completed; });
        return TRUE;
    }
    return task->completeCv.wait_for(lock, std::chrono::milliseconds(dwTimeoutMs),
                                     [task] { return task->completed; }) ? TRUE : FALSE;
}

/**
 * @brief 获取指定下载任务结束时记录的SDK错误码
 */
NET_API BOOL STDCALL NET_clientGetDownloadRecordFileError(IN  LPVOID hTask,
                                                          OUT INT32* pnErrorCode)
{
    DownloadTaskPtr task = FindDownloadTask(hTask);
    if (!task || !pnErrorCode)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    *pnErrorCode = task->resultError.load();
    return TRUE;
}

/**
 * @brief 释放下载任务句柄
 */
NET_API BOOL STDCALL NET_clientReleaseDownloadRecordFile(IN LPVOID hTask)
{
    DownloadTaskPtr task = TakeDownloadTask(hTask);
    if (!task)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    /* 从注册表原子摘除后，其它调用者将无法再获取同一任务；局部shared_ptr
     * 则保证已经进入其它接口的线程可安全完成本次访问。 */
    if (task->worker.joinable())
    {
        if (std::this_thread::get_id() == task->worker.get_id())
        {
            CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
            return FALSE;
        }
        task->worker.join();
    }
    return TRUE;
}

/**
 * @brief 停止指定录像下载并自动回收SDK资源
 */
NET_API BOOL STDCALL NET_clientStopDownloadRecordFile(NET_INOUT LPVOID* phTask)
{
    return NET_clientCancelAndReleaseDownloadRecordFile(phTask);
}

/**
 * @brief 取消并释放录像下载任务
 */
NET_API BOOL STDCALL NET_clientCancelAndReleaseDownloadRecordFile(NET_INOUT LPVOID* phTask)
{
    if (!phTask || !*phTask)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    LPVOID hTask = *phTask;
    if (!NET_clientCancelDownloadRecordFile(hTask))
    {
        return FALSE;
    }

    if (!NET_clientReleaseDownloadRecordFile(hTask))
    {
        return FALSE;
    }

    *phTask = NULL;
    CErrorManage::instance()->SetLastError(NET_E_SUCCEED);
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

    /* 通道 0 合法：与 sdk_old/6860 一致，只有 < 0 才判非法参数。
     * （此前写成 <=0 会把 0 号通道直接挡在本地，设备端根本收不到请求。） */
    if (pstInfo->uChannel < 0)
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

/**
 * @brief 未登录通过 SDK 搜索 JSON 组播协议按 MAC 设置摄像机网络参数
 */
NET_API BOOL STDCALL
NET_clientSetPoeNetwork(IN const NET_PoeNetworkConfig_S* pstConfig)
{
    CHECK_SDK_INIT(FALSE);
    if (!pstConfig)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    CDiscoveryProber prober;
    const int ret = prober.set_network(*pstConfig);
    if (ret != 0)
    {
        CErrorManage::instance()->SetLastError(
            ret == -2 ? NET_E_INVALID_PARAM : NET_E_SYSCALL_FALIED);
        return FALSE;
    }

    CErrorManage::instance()->SetLastError(NET_E_SUCCEED);
    return TRUE;
}

/* ==================== 错误码描述 / 重连开关 ==================== */

/**
 * @brief 获取最近一次错误码的描述信息
 * @return 错误描述字符串（UTF-8），无需调用方释放内存
 * @note  与海康 NET_DVR_GetErrorMsg、大华 CLIENT_GetLastError 对齐
 */
NET_API const char* STDCALL NET_clientGetErrorMsg(void)
{
    return CErrorManage::instance()->GetErrorMsg();
}

/**
 * @brief 设置自动重连开关
 * @param [in] lpUserID  用户登录句柄，不能为空
 * @param [in] bEnable   TRUE 启用自动重连，FALSE 禁用
 * @return 成功返回 TRUE，失败返回 FALSE
 * @note
 * - 启用后，心跳失败达上限时 SDK 自动启动 ReconnectLoop（指数退避重连）
 * - 禁用后，心跳失败达上限时仅通过会话断开通知上层，SDK 不发起重连
 * - 与海康 NET_DVR_SetReconnectCallBack、大华 CLIENT_SetAutoReconnect 对齐
 */
NET_API BOOL STDCALL NET_clientSetAutoReconnect(IN LPVOID lpUserID,
                                                 IN BOOL   bEnable)
{
    CHECK_SDK_INIT(FALSE);

    if (!lpUserID) {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_HANDLE);
        return FALSE;
    }

    auto pDevMgr = CSessionManager::instance();
    if (!pDevMgr) {
        CErrorManage::instance()->SetLastError(NET_E_ALLOC_RESOURCE_ERROR);
        return FALSE;
    }

    auto pSession = pDevMgr->GetSession(lpUserID);
    if (!pSession) {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_HANDLE);
        return FALSE;
    }

    pSession->SetAutoReconnect(bEnable);
    CErrorManage::instance()->SetLastError(NET_E_SUCCEED);
    return TRUE;
}
