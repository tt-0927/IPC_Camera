/**
 * @file ConfigQuery.h
 * @brief 设备配置查询 - 根据命令码路由 GetDevConfig / SetDevConfig / GetDeviceCapability 请求
 */
#pragma once

#include "Singleton.h"
#include "NetTVSDKClientInterface.h"
#include "NetTVSDKHttpUrl.h"
#include "SessionManager.h"
#include "ErrorManage.h"
#include "CommandExecutor.h"

#include <string>

/* 检查SDK是否初始化 */
#define CHECK_SDK_INIT(val) \
    do { \
        auto pMgr = CSessionManager::instance(); \
        if (!pMgr || !pMgr->IsInitialized()) { \
            CErrorManage::instance()->SetLastError(NET_E_SDK_NOT_INIT); \
            return val; \
        } \
    } while(0)

/* 检查SDK是否已经初始化 */
#define CHECK_SDK_ALREADY_INIT(val) \
    do { \
        auto pMgr = CSessionManager::instance(); \
        if (pMgr && pMgr->IsInitialized()) { \
            CErrorManage::instance()->SetLastError(NET_E_ALREDY_INIT_ERROR); \
            return val; \
        } \
    } while(0)

/**
 * @brief 设备配置查询单例
 * @details 根据 dwCommand 命令码将 GetDevConfig / SetDevConfig / GetDeviceCapability
 *          请求路由到对应的结构体类型，通过 CCommandExecutor 执行 HTTP 请求
 */
class CConfigQuery : public CSingleton<CConfigQuery>
{
    CConfigQuery() {}
public:
    ~CConfigQuery() {}
    friend class CSingleton<CConfigQuery>;

    /**
     * @brief 获取设备能力集
     */
    BOOL GetDeviceCapability(LPVOID lpUserID, INT32 dwChannelID, INT32 dwCommand,
                             LPVOID lpOutBuffer, INT32 dwOutBufferSize, INT32 *pdwBytesReturned);

    /**
     * @brief 获取设备配置（通用分发）
     */
    BOOL GetDevConfig(LPVOID lpUserID, INT32 dwChannelID, INT32 dwCommand,
                      LPVOID lpOutBuffer, INT32 dwOutBufferSize, INT32 *pdwBytesReturned);

    /**
     * @brief 设置设备配置（通用分发）
     */
    BOOL SetDevConfig(LPVOID lpUserID, INT32 dwChannelID, INT32 dwCommand,
                      LPVOID lpInBuffer, INT32 dwInBufferSize, INT32 *pdwBytesReturned);

private:
    /* 通用模板：获取配置 */
    template <typename T_CFG>
    static BOOL GetDevConfig_Impl(LPVOID lpUserID,
                                  INT32 dwChannelID, INT32 dwCommand,
                                  LPVOID lpOutBuffer, INT32 dwOutBufferSize,
                                  INT32 *pdwBytesReturned);

    /* 通用模板：设置配置 */
    template <typename T_CFG>
    static BOOL SetDevConfig_Impl(LPVOID lpUserID,
                                  INT32 dwChannelID, INT32 dwCommand,
                                  LPVOID lpInBuffer, INT32 dwInBufferSize,
                                  INT32 *pdwBytesReturned);
};

/* ========================================================================== */
/*  模板实现（必须在头文件中）                                                  */
/* ========================================================================== */

template <typename T_CFG>
BOOL CConfigQuery::GetDevConfig_Impl(LPVOID lpUserID,
                                           INT32 dwChannelID, INT32 dwCommand,
                                           LPVOID lpOutBuffer, INT32 dwOutBufferSize,
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
    return CCommandExecutor::instance()->ExecuteGet<T_CFG>(lpUserID, url, lpOutBuffer, pdwBytesReturned) ? TRUE : FALSE;
}

template <typename T_CFG>
BOOL CConfigQuery::SetDevConfig_Impl(LPVOID lpUserID,
                                           INT32 dwChannelID, INT32 dwCommand,
                                           LPVOID lpInBuffer, INT32 dwInBufferSize,
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
    BOOL bRet = CCommandExecutor::instance()->ExecuteSet<T_CFG>(lpUserID, "POST", url, lpInBuffer) ? TRUE : FALSE;
    if (bRet && pdwBytesReturned != NULL)
    {
        *pdwBytesReturned = 0;
    }
    return bRet;
}
