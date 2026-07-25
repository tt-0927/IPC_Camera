#include "AlarmModule.h"
#include "SessionModule.h"
#include "SessionManager.h"
#include "AlarmInfoConvert.h"
#include "DeviceInfoConvert.h"
#include "SDKConvert.h"
#include "Json.h"
#include "NetSdkLog.h"

AlarmModule::AlarmModule(SessionModule* pSessionModule)
    : m_pSessionModule(pSessionModule)
    , m_pushCount(0)
{
    NSDK_LOG_DEBUG("AlarmModule created");
}

AlarmModule::~AlarmModule()
{
    NSDK_LOG_DEBUG("AlarmModule destroyed, total pushes: %llu", m_pushCount);
}

BOOL AlarmModule::PushAlarmInfo(NET_TV_ALARMER_S* pAlarmer,
                               INT32 lCommand,
                               LPVOID pAlarmInfo,
                               INT32 dwBufLen)
{
    if (!pAlarmer || !pAlarmInfo || dwBufLen <= 0)
    {
        NSDK_LOG_ERROR("PushAlarmInfo: Invalid parameters");
        return FALSE;
    }

    NSDK_LOG_DEBUG("Pushing alarm info: cmd=0x%x, device=%s", lCommand, pAlarmer->szDeviceIP);
    const size_t totalSessions = CSessionManager::instance()->GetSessionCount();

    // 构建告警JSON
    Json::Object* pRoot = Json::init();
    if (!pRoot)
    {
        NSDK_LOG_ERROR("PushAlarmInfo: Failed to init JSON");
        return FALSE;
    }

    // [诊断] 注入入队时间戳，方便下游测量队列延迟
    {
        auto tp_now = std::chrono::steady_clock::now();
        long long ts_now = std::chrono::duration_cast<std::chrono::milliseconds>(
            tp_now.time_since_epoch()).count();
        Json::add(pRoot, "enqueue_ts", ts_now);
    }

    Json::add(pRoot, "Command", (long long)lCommand);

    // [诊断] 注入入队时间戳，方便下游测量队列延迟
    {
        auto tp_now = std::chrono::steady_clock::now();
        long long ts_now = std::chrono::duration_cast<std::chrono::milliseconds>(
            tp_now.time_since_epoch()).count();
        Json::add(pRoot, "enqueue_ts", ts_now);
    }

    // 添加Alarmer信息
    {
        Json::Object* pAlarmerJson = Json::init();
        NET_TV_ALARMER_S tmp = *pAlarmer;
        SDKConvert::deal(pAlarmerJson, tmp, false);
        Json::add(pRoot, "Alarmer", pAlarmerJson);
    }

    // 添加AlarmInfo（按命令码选择结构体）
    {
        Json::Object* pInfoJson = Json::init();
        INT32 alarmBase = lCommand & 0xF000;

        if (lCommand == NET_TV_ALARM_FACE_COMPARE)
        {
            if (dwBufLen < (INT32)sizeof(NET_TV_ALARM_FACE_COMPARE_INFO_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                /* 含大图片数组，禁止栈上拷贝，直接引用原始数据 */
                NET_TV_ALARM_FACE_COMPARE_INFO_S& info = *(NET_TV_ALARM_FACE_COMPARE_INFO_S*)pAlarmInfo;
                SDKConvert::deal(pInfoJson, info, false);
            }
        }
        else if (alarmBase == NET_TV_ALARM_BASE_BASIC)
        {
            if (dwBufLen < (INT32)sizeof(NET_TV_ALARM_BASIC_INFO_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                NET_TV_ALARM_BASIC_INFO_S& info = *(NET_TV_ALARM_BASIC_INFO_S*)pAlarmInfo;
                NSDK_LOG_INFO("[DIAG-ALARM] Basic input: cmd=0x%x, alarmType=0x%x, timestamp=%lld, panoramaLen=%u, bufLen=%d, structSize=%zu",
                              lCommand,
                              info.dwAlarmType,
                              (long long)info.llTimestampMs,
                              info.dwPanoramaImgLen,
                              dwBufLen,
                              sizeof(NET_TV_ALARM_BASIC_INFO_S));
                SDKConvert::deal(pInfoJson, info, false);
            }
        }
        else if (alarmBase == NET_TV_ALARM_BASE_RULE)
        {
            if (dwBufLen < (INT32)sizeof(NET_TV_ALARM_RULE_INFO_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                /* 含大图片数组，禁止栈上拷贝，直接引用原始数据 */
                NET_TV_ALARM_RULE_INFO_S& info = *(NET_TV_ALARM_RULE_INFO_S*)pAlarmInfo;
                NSDK_LOG_INFO("[DIAG-ALARM] Rule input: cmd=0x%x, alarmType=0x%x, channel=%u, rule=%u, target=%u, "
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
                              sizeof(NET_TV_ALARM_RULE_INFO_S));
                SDKConvert::deal(pInfoJson, info, false);
            }
        }
        else if (alarmBase == NET_TV_ALARM_BASE_AI)
        {
            if (dwBufLen < (INT32)sizeof(NET_TV_ALARM_AI_OBJECT_INFO_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                /* 含大图片数组，禁止栈上拷贝，直接引用原始数据 */
                NET_TV_ALARM_AI_OBJECT_INFO_S& info = *(NET_TV_ALARM_AI_OBJECT_INFO_S*)pAlarmInfo;
                NSDK_LOG_INFO("[DIAG-ALARM] AI object input: cmd=0x%x, alarmType=0x%x, channel=%u, object=%s, "
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
                              sizeof(NET_TV_ALARM_AI_OBJECT_INFO_S));
                SDKConvert::deal(pInfoJson, info, false);
            }
        }
        else if (alarmBase == NET_TV_ALARM_BASE_TRAFFIC)
        {
            if (dwBufLen < (INT32)sizeof(NET_TV_ALARM_PLATE_INFO_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                SDKConvert::deal(pInfoJson, *(NET_TV_ALARM_PLATE_INFO_S*)pAlarmInfo, false);
            }
        }
        else if (alarmBase == NET_TV_ALARM_BASE_EXCEPTION)
        {
            if (dwBufLen < (INT32)sizeof(NET_TV_ALARM_EXCEPTION_INFO_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                SDKConvert::deal(pInfoJson, *(NET_TV_ALARM_EXCEPTION_INFO_S*)pAlarmInfo, false);
            }
        }
        else if (alarmBase == NET_TV_ALARM_BASE_STATISTICS)
        {
            if (dwBufLen < (INT32)sizeof(NET_TV_ALARM_STATISTICS_INFO_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                /* 统计类结构体含多个 1MB 图片数组，总大小约 3MB，禁止栈上拷贝，直接使用指针引用原始数据 */
                NET_TV_ALARM_STATISTICS_INFO_S& info = *(NET_TV_ALARM_STATISTICS_INFO_S*)pAlarmInfo;
                SDKConvert::deal(pInfoJson, info, false);
            }
        }
        else if (lCommand == NET_TV_NOTICE_DOWNLOAD_RECORD_PROGRESS)
        {
            if (dwBufLen < (INT32)sizeof(NET_TV_RECORD_DOWNLOAD_PROGRESS_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                SDKConvert::deal(pInfoJson, *(NET_TV_RECORD_DOWNLOAD_PROGRESS_S*)pAlarmInfo, false);
            }
        }
        else
        {
            Json::add(pInfoJson, "AlarmType", (long long)lCommand);
        }

        Json::add(pRoot, "AlarmInfo", pInfoJson);
    }

    // 转换为JSON字符串
    std::string jsonStr = Json::to_string(pRoot);
    Json::deinit(pRoot);
    if ((lCommand & 0xF000) == NET_TV_ALARM_BASE_RULE ||
        (lCommand & 0xF000) == NET_TV_ALARM_BASE_AI)
    {
        NSDK_LOG_INFO("[DIAG-ALARM] JSON output: cmd=0x%x, jsonLen=%zu, hasPanoramaB64=%d, hasTargetB64=%d, hasImgDataB64=%d",
                      lCommand,
                      jsonStr.size(),
                      jsonStr.find("PanoramaImgBase64") != std::string::npos ? 1 : 0,
                      jsonStr.find("TargetImgBase64") != std::string::npos ? 1 : 0,
                      jsonStr.find("ImgDataBase64") != std::string::npos ? 1 : 0);
    }

    // [诊断] 记录 PushToAll 前的时间戳，用于定位报警转发延迟
    auto tp_before_push = std::chrono::steady_clock::now();
    long long ts_before_push = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp_before_push.time_since_epoch()).count();

    // 推送到所有会话
    size_t pushCount = CSessionManager::instance()->PushToAll(jsonStr);

    auto tp_after_push = std::chrono::steady_clock::now();
    long long ts_after_push = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp_after_push.time_since_epoch()).count();
    NSDK_LOG_INFO("[DIAG] PushToAll done: cmd=0x%x, enqueue_ts=%lld, cost_ms=%lld",
                  lCommand, ts_before_push, ts_after_push - ts_before_push);
    if (pushCount == 0)
    {
        std::string diagInfo = CSessionManager::instance()->GetSessionDiagnosticInfo();
        NSDK_LOG_WARN("AlarmModule::PushAlarmInfo: FAILED - No eligible clients, cmd=0x%x, device=%s, totalSessions=%zu, bufLen=%d. "
                      "Client status: %s",
                      lCommand, pAlarmer->szDeviceIP, totalSessions, dwBufLen, diagInfo.c_str());
    }
    else
    {
        NSDK_LOG_INFO("AlarmModule::PushAlarmInfo: SUCCESS - Forwarded to %zu client(s), cmd=0x%x, device=%s, totalSessions=%zu, bufLen=%d",
                      pushCount, lCommand, pAlarmer->szDeviceIP, totalSessions, dwBufLen);
    }

    m_pushCount++;
    return (pushCount > 0) ? TRUE : FALSE;
}

BOOL AlarmModule::PushChannelStatusInfo(NET_TV_CHANNEL_INFO_S* pChannelInfo)
{
    NSDK_LOG_WARN("[AlarmModule] ===== PushChannelStatusInfo Start ===== ");

    if (!pChannelInfo)
    {
        NSDK_LOG_ERROR("[AlarmModule] PushChannelStatusInfo: Invalid parameters (NULL)");
        NSDK_LOG_WARN("[AlarmModule] ===== PushChannelStatusInfo End (Failed) ===== ");
        return FALSE;
    }

    // 打印输入参数详情
    NSDK_LOG_WARN("[AlarmModule] Input Channel Info:");
    NSDK_LOG_WARN("[AlarmModule]   Channel:      %u", pChannelInfo->dwChannel);
    NSDK_LOG_WARN("[AlarmModule]   ChannelName:  %s", pChannelInfo->szChannelName);
    NSDK_LOG_WARN("[AlarmModule]   DeviceName:   %s", pChannelInfo->szDevName);
    NSDK_LOG_WARN("[AlarmModule]   DeviceIP:     %s", pChannelInfo->szDeviceIP);
    NSDK_LOG_WARN("[AlarmModule]   SerialNum:    %s", pChannelInfo->szSerialNum);
    NSDK_LOG_WARN("[AlarmModule]   Enable:       %d (%s)", pChannelInfo->byEnable,
                  pChannelInfo->byEnable ? "ENABLED" : "DISABLED");
    NSDK_LOG_WARN("[AlarmModule]   Online:       %d (%s)", pChannelInfo->byOnline,
                  pChannelInfo->byOnline ? "ONLINE" : "OFFLINE");
    NSDK_LOG_WARN("[AlarmModule]   RecordStatus: %d", pChannelInfo->nRecordStatus);
    NSDK_LOG_WARN("[AlarmModule]   DevState:     %d", pChannelInfo->nDevState);

    Json::Object* pRoot = Json::init();
    if (!pRoot)
    {
        NSDK_LOG_ERROR("[AlarmModule] PushChannelStatusInfo: Failed to init JSON");
        NSDK_LOG_WARN("[AlarmModule] ===== PushChannelStatusInfo End (JSON init failed) ===== ");
        return FALSE;
    }

    Json::add(pRoot, "Event", "ChannelStatus");
    Json::add(pRoot, "Command", (long long)NET_TV_NOTIFY_CHANNEL_STATUS);
    NSDK_LOG_WARN("[AlarmModule] Command: NET_TV_NOTIFY_CHANNEL_STATUS (0x%X)", NET_TV_NOTIFY_CHANNEL_STATUS);

    Json::Object* pChannelJson = Json::init();
    NET_TV_CHANNEL_INFO_S info = *pChannelInfo;
    SDKConvert::deal(pChannelJson, info, false);
    Json::add(pRoot, "ChannelInfo", pChannelJson);

    std::string jsonStr = Json::to_string(pRoot);
    Json::deinit(pRoot);

    // 打印将要推送的JSON数据
    NSDK_LOG_DEBUG("[AlarmModule] Channel Status JSON: %s", jsonStr.c_str());

    // 获取在线客户端数量
    size_t clientCount = CSessionManager::instance()->GetSessionCount();
    NSDK_LOG_WARN("[AlarmModule] Current online clients: %zu", clientCount);

    // 执行推送
    NSDK_LOG_WARN("[AlarmModule] Calling PushToAll...");
    size_t pushCount = CSessionManager::instance()->PushToAll(jsonStr);

    if (pushCount == 0)
    {
        NSDK_LOG_WARN("[AlarmModule] No active clients for channel status!");
        NSDK_LOG_WARN("[AlarmModule]   Channel: %u, Online: %u", 
                      pChannelInfo->dwChannel, pChannelInfo->byOnline);
    }
    else
    {
        NSDK_LOG_WARN("[AlarmModule] Channel status pushed to %zu client(s) SUCCESS", pushCount);
        NSDK_LOG_WARN("[AlarmModule]   Channel: %u, Online: %s",
                      pChannelInfo->dwChannel,
                      pChannelInfo->byOnline ? "ONLINE" : "OFFLINE");
    }

    m_pushCount++;
    NSDK_LOG_WARN("[AlarmModule] Total push count: %lld", m_pushCount);
    NSDK_LOG_WARN("[AlarmModule] ===== PushChannelStatusInfo End ===== ");
    return TRUE;
}

INT64 AlarmModule::GetPushCount() const
{
    return m_pushCount;
}

void AlarmModule::ResetPushCount()
{
    NSDK_LOG_DEBUG("Resetting push count from %llu to 0", m_pushCount);
    m_pushCount = 0;
}
