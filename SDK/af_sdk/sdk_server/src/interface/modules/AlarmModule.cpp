/**
 * @file AlarmModule.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CAlarmModule 模块实现
 * 功能说明：
 * 1. 实现 CAlarmModule 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#include "AlarmModule.h"
#include "SessionModule.h"
#include "SessionManager.h"
#include "AlarmInfoConvert.h"
#include "DeviceInfoConvert.h"
#include "SDKConvert.h"
#include "Json.h"
#include "NetSdkLog.h"

CAlarmModule::CAlarmModule(CSessionModule* pSessionModule)
    : m_pSessionModule(pSessionModule)
    , m_lPushCount(0)
{
    NETSDK_LOG_MESSAGE_DEBUG("CAlarmModule created");
}

CAlarmModule::~CAlarmModule()
{
    NETSDK_LOG_MESSAGE_DEBUG("CAlarmModule destroyed, total pushes: %llu", m_lPushCount);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 PushAlarmInfo 对应的处理。
 * @return 返回该处理的状态或结果。
 */

BOOL CAlarmModule::PushAlarmInfo(NET_ALARMER_S* pAlarmer,
                               INT32 lCommand,
                               LPVOID pAlarmInfo,
                               INT32 dwBufLen)
{
    if (!pAlarmer || !pAlarmInfo || dwBufLen <= 0)
    {
        NETSDK_LOG_MESSAGE_ERROR("PushAlarmInfo: Invalid parameters");
        return NET_FALSE;
    }

    NETSDK_LOG_MESSAGE_DEBUG("Pushing alarm info: cmd=0x%x, device=%s", lCommand, pAlarmer->szDeviceIP);
    const size_t totalSessions = CSessionManager::instance()->GetSessionCount();

    /* 构建告警JSON */
    Json::Object* pRoot = Json::init();
    if (!pRoot)
    {
        NETSDK_LOG_MESSAGE_ERROR("PushAlarmInfo: Failed to init JSON");
        return NET_FALSE;
    }

    /* [诊断] 注入入队时间戳，方便下游测量队列延迟 */
    {
        auto tp_now = std::chrono::steady_clock::now();
        long long ts_now = std::chrono::duration_cast<std::chrono::milliseconds>(
            tp_now.time_since_epoch()).count();
        Json::add(pRoot, "enqueue_ts", ts_now);
    }

    Json::add(pRoot, "Command", (long long)lCommand);

    /* [诊断] 注入入队时间戳，方便下游测量队列延迟 */
    {
        auto tp_now = std::chrono::steady_clock::now();
        long long ts_now = std::chrono::duration_cast<std::chrono::milliseconds>(
            tp_now.time_since_epoch()).count();
        Json::add(pRoot, "enqueue_ts", ts_now);
    }

    /* 添加Alarmer信息 */
    {
        Json::Object* pAlarmerJson = Json::init();
        NET_ALARMER_S tmp = *pAlarmer;
        SDKConvert::deal(pAlarmerJson, tmp, false);
        Json::add(pRoot, "Alarmer", pAlarmerJson);
    }

    /* 添加AlarmInfo（按命令码选择结构体） */
    {
        Json::Object* pInfoJson = Json::init();
        INT32 alarmBase = lCommand & 0xF000;

        if (lCommand == NET_ALARM_FACE_COMPARE)
        {
            if (dwBufLen < (INT32)sizeof(NET_ALARM_FACE_COMPARE_INFO_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                /* 含大图片数组，禁止栈上拷贝，直接引用原始数据 */
                NET_ALARM_FACE_COMPARE_INFO_S& info = *(NET_ALARM_FACE_COMPARE_INFO_S*)pAlarmInfo;
                SDKConvert::deal(pInfoJson, info, false);
            }
        }
        else if (alarmBase == NET_ALARM_BASE_BASIC)
        {
            if (dwBufLen < (INT32)sizeof(NET_ALARM_BASIC_INFO_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                NET_ALARM_BASIC_INFO_S& info = *(NET_ALARM_BASIC_INFO_S*)pAlarmInfo;
                NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] Basic input: cmd=0x%x, alarmType=0x%x, timestamp=%lld, panoramaLen=%u, bufLen=%d, structSize=%zu",
                              lCommand,
                              info.dwAlarmType,
                              (long long)info.llTimestampMs,
                              info.dwPanoramaImgLen,
                              dwBufLen,
                              sizeof(NET_ALARM_BASIC_INFO_S));
                SDKConvert::deal(pInfoJson, info, false);
            }
        }
        else if (alarmBase == NET_ALARM_BASE_RULE)
        {
            if (dwBufLen < (INT32)sizeof(NET_ALARM_RULE_INFO_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                /* 含大图片数组，禁止栈上拷贝，直接引用原始数据 */
                NET_ALARM_RULE_INFO_S& info = *(NET_ALARM_RULE_INFO_S*)pAlarmInfo;
                NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] Rule input: cmd=0x%x, alarmType=0x%x, channel=%u, rule=%u, target=%u, "
                              "objType=%u, timestamp=%lld, rect=[%d,%d,%d,%d], panoramaLen=%u, targetLen=%u, bufLen=%d, structSize=%zu",
                              lCommand,
                              info.dwAlarmType,
                              info.dwChannel,
                              info.dwRuleID,
                              info.dwTargetID,
                              info.dwObjectType,
                              (long long)info.llTimestampMs,
                              info.nLeft,
                              info.nTop,
                              info.nRight,
                              info.nBottom,
                              info.dwPanoramaImgLen,
                              info.dwTargetImgLen,
                              dwBufLen,
                              sizeof(NET_ALARM_RULE_INFO_S));
                SDKConvert::deal(pInfoJson, info, false);
            }
        }
        else if (alarmBase == NET_ALARM_BASE_AI)
        {
            if (dwBufLen < (INT32)sizeof(NET_ALARM_AI_OBJECT_INFO_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                /* 含大图片数组，禁止栈上拷贝，直接引用原始数据 */
                NET_ALARM_AI_OBJECT_INFO_S& info = *(NET_ALARM_AI_OBJECT_INFO_S*)pAlarmInfo;
                NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] AI object input: cmd=0x%x, alarmType=0x%x, channel=%u, object=%s, "
                              "objType=%u, timestamp=%lld, rect=[%d,%d,%d,%d], panoramaLen=%u, imgLen=%u, bufLen=%d, structSize=%zu",
                              lCommand,
                              info.dwAlarmType,
                              info.dwChannel,
                              info.szObjectID,
                              info.dwObjectType,
                              (long long)info.llTimestampMs,
                              info.nLeft,
                              info.nTop,
                              info.nRight,
                              info.nBottom,
                              info.dwPanoramaImgLen,
                              info.dwImgLen,
                              dwBufLen,
                              sizeof(NET_ALARM_AI_OBJECT_INFO_S));
                SDKConvert::deal(pInfoJson, info, false);
            }
        }
        else if (alarmBase == NET_ALARM_BASE_TRAFFIC)
        {
            if (dwBufLen < (INT32)sizeof(NET_ALARM_PLATE_INFO_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                SDKConvert::deal(pInfoJson, *(NET_ALARM_PLATE_INFO_S*)pAlarmInfo, false);
            }
        }
        else if (alarmBase == NET_ALARM_BASE_EXCEPTION)
        {
            if (dwBufLen < (INT32)sizeof(NET_ALARM_EXCEPTION_INFO_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                SDKConvert::deal(pInfoJson, *(NET_ALARM_EXCEPTION_INFO_S*)pAlarmInfo, false);
            }
        }
        else if (alarmBase == NET_ALARM_BASE_STATISTICS)
        {
            if (dwBufLen < (INT32)sizeof(NET_ALARM_STATISTICS_INFO_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                /* 统计类结构体含多个 1MB 图片数组，总大小约 3MB，禁止栈上拷贝，直接使用指针引用原始数据 */
                NET_ALARM_STATISTICS_INFO_S& info = *(NET_ALARM_STATISTICS_INFO_S*)pAlarmInfo;
                SDKConvert::deal(pInfoJson, info, false);
            }
        }
        else if (lCommand == NET_NOTICE_DOWNLOAD_RECORD_PROGRESS)
        {
            if (dwBufLen < (INT32)sizeof(NET_RECORD_DOWNLOAD_PROGRESS_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                SDKConvert::deal(pInfoJson, *(NET_RECORD_DOWNLOAD_PROGRESS_S*)pAlarmInfo, false);
            }
        }
        else
        {
            Json::add(pInfoJson, "AlarmType", (long long)lCommand);
        }

        Json::add(pRoot, "AlarmInfo", pInfoJson);
    }

    /* 转换为JSON字符串 */
    std::string jsonStr = Json::to_string(pRoot);
    Json::deinit(pRoot);
    if ((lCommand & 0xF000) == NET_ALARM_BASE_RULE ||
        (lCommand & 0xF000) == NET_ALARM_BASE_AI)
    {
        NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] JSON output: cmd=0x%x, jsonLen=%zu, hasPanoramaB64=%d, hasTargetB64=%d, hasImgDataB64=%d",
                      lCommand,
                      jsonStr.size(),
                      jsonStr.find("PanoramaImgBase64") != std::string::npos ? 1 : 0,
                      jsonStr.find("TargetImgBase64") != std::string::npos ? 1 : 0,
                      jsonStr.find("ImgDataBase64") != std::string::npos ? 1 : 0);
    }

    /* [诊断] 记录 PushToAll 前的时间戳，用于定位报警转发延迟 */
    auto tp_before_push = std::chrono::steady_clock::now();
    long long ts_before_push = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp_before_push.time_since_epoch()).count();

    /* 推送到所有会话 */
    size_t pushCount = CSessionManager::instance()->PushToAll(jsonStr);

    auto tp_after_push = std::chrono::steady_clock::now();
    long long ts_after_push = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp_after_push.time_since_epoch()).count();
    NETSDK_LOG_MESSAGE_INFO("[DIAG] PushToAll done: cmd=0x%x, enqueue_ts=%lld, cost_ms=%lld",
                  lCommand, ts_before_push, ts_after_push - ts_before_push);
    if (pushCount == 0)
    {
        std::string diagInfo = CSessionManager::instance()->GetSessionDiagnosticInfo();
        NETSDK_LOG_MESSAGE_WARN("CAlarmModule::PushAlarmInfo: FAILED - No eligible clients, cmd=0x%x, device=%s, totalSessions=%zu, bufLen=%d. "
                      "Client status: %s",
                      lCommand, pAlarmer->szDeviceIP, totalSessions, dwBufLen, diagInfo.c_str());
    }
    else
    {
        NETSDK_LOG_MESSAGE_INFO("CAlarmModule::PushAlarmInfo: SUCCESS - Forwarded to %zu client(s), cmd=0x%x, device=%s, totalSessions=%zu, bufLen=%d",
                      pushCount, lCommand, pAlarmer->szDeviceIP, totalSessions, dwBufLen);
    }

    m_lPushCount++;
    return (pushCount > 0) ? NET_TRUE : NET_FALSE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 PushChannelStatusInfo 对应的处理。
 * @param [in,out] pChannelInfo 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

BOOL CAlarmModule::PushChannelStatusInfo(NET_CHANNEL_INFO_S* pChannelInfo)
{
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] ===== PushChannelStatusInfo Start ===== ");

    if (!pChannelInfo)
    {
        NETSDK_LOG_MESSAGE_ERROR("[CAlarmModule] PushChannelStatusInfo: Invalid parameters (NULL)");
        NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] ===== PushChannelStatusInfo End (Failed) ===== ");
        return NET_FALSE;
    }

    /* 打印输入参数详情 */
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] Input Channel Info:");
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule]   Channel:      %u", pChannelInfo->dwChannel);
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule]   ChannelName:  %s", pChannelInfo->szChannelName);
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule]   DeviceName:   %s", pChannelInfo->szDevName);
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule]   DeviceIP:     %s", pChannelInfo->szDeviceIP);
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule]   SerialNum:    %s", pChannelInfo->szSerialNum);
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule]   Enable:       %d (%s)", pChannelInfo->byEnable,
                  pChannelInfo->byEnable ? "ENABLED" : "DISABLED");
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule]   Online:       %d (%s)", pChannelInfo->byOnline,
                  pChannelInfo->byOnline ? "ONLINE" : "OFFLINE");
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule]   RecordStatus: %d", pChannelInfo->nRecordStatus);
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule]   DevState:     %d", pChannelInfo->nDevState);

    Json::Object* pRoot = Json::init();
    if (!pRoot)
    {
        NETSDK_LOG_MESSAGE_ERROR("[CAlarmModule] PushChannelStatusInfo: Failed to init JSON");
        NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] ===== PushChannelStatusInfo End (JSON init failed) ===== ");
        return NET_FALSE;
    }

    Json::add(pRoot, "Event", "ChannelStatus");
    Json::add(pRoot, "Command", (long long)NET_NOTIFY_CHANNEL_STATUS);
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] Command: NET_NOTIFY_CHANNEL_STATUS (0x%X)", NET_NOTIFY_CHANNEL_STATUS);

    Json::Object* pChannelJson = Json::init();
    NET_CHANNEL_INFO_S info = *pChannelInfo;
    SDKConvert::deal(pChannelJson, info, false);
    Json::add(pRoot, "ChannelInfo", pChannelJson);

    std::string jsonStr = Json::to_string(pRoot);
    Json::deinit(pRoot);

    /* 打印将要推送的JSON数据 */
    NETSDK_LOG_MESSAGE_DEBUG("[CAlarmModule] Channel Status JSON: %s", jsonStr.c_str());

    /* 获取在线客户端数量 */
    size_t clientCount = CSessionManager::instance()->GetSessionCount();
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] Current online clients: %zu", clientCount);

    /* 执行推送 */
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] Calling PushToAll...");
    size_t pushCount = CSessionManager::instance()->PushToAll(jsonStr);

    if (pushCount == 0)
    {
        NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] No active clients for channel status!");
        NETSDK_LOG_MESSAGE_WARN("[CAlarmModule]   Channel: %u, Online: %u",
                      pChannelInfo->dwChannel, pChannelInfo->byOnline);
    }
    else
    {
        NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] Channel status pushed to %zu client(s) SUCCESS", pushCount);
        NETSDK_LOG_MESSAGE_WARN("[CAlarmModule]   Channel: %u, Online: %s",
                      pChannelInfo->dwChannel,
                      pChannelInfo->byOnline ? "ONLINE" : "OFFLINE");
    }

    m_lPushCount++;
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] Total push count: %lld", m_lPushCount);
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] ===== PushChannelStatusInfo End ===== ");
    return NET_TRUE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 GetPushCount 对应的数据。
 * @return 返回该处理的状态或结果。
 */

INT64 CAlarmModule::GetPushCount() const
{
    return m_lPushCount;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 ResetPushCount 定义的内部处理。
 * @return 无返回值。
 */

void CAlarmModule::ResetPushCount()
{
    NETSDK_LOG_MESSAGE_DEBUG("Resetting push count from %llu to 0", m_lPushCount);
    m_lPushCount = 0;
}
