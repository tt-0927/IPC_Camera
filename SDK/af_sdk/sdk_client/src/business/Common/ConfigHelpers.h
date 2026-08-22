/**
 * @file ConfigHelpers.h
 * @brief 配置查询辅助函数 - URL 编码、录像文件列表查询、日志列表查询
 *        从 ConfigQuery 中提取的通用辅助逻辑
 */
#pragma once

#include "NetTVSDKClientInterface.h"
#include "NetTVSDKHttpUrl.h"
#include "CommandExecutor.h"
#include "ErrorManage.h"

#include <cstring>
#include <cctype>
#include <iomanip>
#include <sstream>

/**
 * @brief URL 编码（RFC 3986）
 * @param value 待编码字符串
 * @return 编码后的字符串，非字母数字字符以 %XX 形式输出
 */
std::string UrlEncode(const char* value)
{
    if (!value) return "";

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
 * @brief 获取录像文件列表
 * @note 根据查询条件（通道号、类型、时间范围、文件名）构建特殊 URL 并发起 HTTP 查询
 */
BOOL GetRecordFileList(LPVOID lpUserID, INT32 dwChannelID, INT32 dwCommand,
                       LPVOID lpOutBuffer, INT32 dwOutBufferSize,
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
        << "&Year=" << UrlEncode(pCfg->stFind.szYear)
        << "&Month=" << UrlEncode(pCfg->stFind.szMonth)
        << "&Date=" << UrlEncode(pCfg->stFind.szDate)
        << "&StartTime=" << UrlEncode(pCfg->stFind.szStartTime)
        << "&EndTime=" << UrlEncode(pCfg->stFind.szEndTime)
        << "&Filename=" << UrlEncode(pCfg->stFind.szFilename);

    return CCommandExecutor::instance()->ExecuteGet<NET_RecordFileList_S>(lpUserID, url.str(), lpOutBuffer, pdwBytesReturned) ? TRUE : FALSE;
}

/**
 * @brief 获取日志列表
 * @note 根据查询条件（类型、操作、时间范围、分页）构建特殊 URL 并发起 HTTP 查询
 */
BOOL GetLogList(LPVOID lpUserID, INT32 dwChannelID, INT32 dwCommand,
                LPVOID lpOutBuffer, INT32 dwOutBufferSize,
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
        << "&StartTime=" << UrlEncode(pCfg->stCond.szStartTime)
        << "&EndTime=" << UrlEncode(pCfg->stCond.szEndTime)
        << "&CurPage=" << nCurPage
        << "&PageSize=" << nPageSize;

    return CCommandExecutor::instance()->ExecuteGet<NET_LogList_S>(lpUserID, url.str(), lpOutBuffer, pdwBytesReturned) ? TRUE : FALSE;
}
