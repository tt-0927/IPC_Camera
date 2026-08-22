/**
 * @file RecordFrameHub.cpp
 * @brief 录像帧流汇聚管理实现 - 启动/停止录像帧 TCP 流
 */

#include "RecordFrameHub.h"
#include "NetSdkLog.h"

#include <cstring>

/* ========================================================================== */
/*  StartStream                                                                */
/* ========================================================================== */

BOOL CRecordFrameHub::StartStream(LPVOID lpUserID,
                                       pNET_RecordFrameStreamCond_S pstCond,
                                       pNET_RecordFrameStreamInfo_S pstStreamInfo,
                                       NET_RecordFrameCallBack cbRecordFrame,
                                       LPVOID lpUserData)
{
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

    /* 1. 请求服务端启动帧流 */
    std::string body = SDKConvert::to_string(*pstCond);
    std::string respBody;
    if (!CCommandExecutor::instance()->ExecuteRaw((LPUSER_HANDLE)lpUserID,
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

    /* 2. 解析流信息 */
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

    /* 3. 获取设备地址 */
    auto session = CSessionManager::instance()->GetSession(lpUserID);
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

    /* 4. 建立 TCP 连接 */
    auto client = std::make_shared<tvsdk::CRecordFrameStream>();
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
        /* TCP 连接失败 → 通知服务端停止帧流（回滚） */
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
        CCommandExecutor::instance()->ExecuteRaw((LPUSER_HANDLE)lpUserID,
                                                "POST",
                                                NET_API_PATH_RECORD_FRAME_STREAM_STOP,
                                                SDKConvert::to_string(stStopInfo),
                                                stopResp);
        CErrorManage::instance()->SetLastError(NET_E_SYSCALL_FALIED);
        return FALSE;
    }

    /* 5. 记录到映射表 */
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto old = m_streamMap.find(pstStreamInfo->szStreamId);
        if (old != m_streamMap.end())
        {
            old->second->stop();
            m_streamMap.erase(old);
        }
        m_streamMap[pstStreamInfo->szStreamId] = client;
    }

    CErrorManage::instance()->SetLastError(NET_E_SUCCEED);
    return TRUE;
}

/* ========================================================================== */
/*  StopStream                                                                 */
/* ========================================================================== */

BOOL CRecordFrameHub::StopStream(LPVOID lpUserID, const CHAR* szStreamId)
{
    if (!lpUserID || !szStreamId || szStreamId[0] == '\0')
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    /* 1. 通知服务端停止帧流 */
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
    if (!CCommandExecutor::instance()->ExecuteRaw((LPUSER_HANDLE)lpUserID,
                                                 "POST",
                                                 NET_API_PATH_RECORD_FRAME_STREAM_STOP,
                                                 SDKConvert::to_string(stStopInfo),
                                                 respBody))
    {
        return FALSE;
    }

    const int respCode = SDKConvert::get_respCode(respBody);
    CErrorManage::instance()->SetLastError(respCode);

    /* 2. 停止本地 TCP 连接 */
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_streamMap.find(szStreamId);
        if (it != m_streamMap.end())
        {
            it->second->stop();
            m_streamMap.erase(it);
        }
    }

    return respCode == NET_E_SUCCEED ? TRUE : FALSE;
}

/* ========================================================================== */
/*  Cleanup                                                                    */
/* ========================================================================== */

void CRecordFrameHub::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_streamMap)
    {
        pair.second->stop();
    }
    m_streamMap.clear();
}
