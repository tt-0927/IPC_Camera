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

    Json::add(pRoot, "Command", (long long)lCommand);

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
                NET_TV_ALARM_FACE_COMPARE_INFO_S info = *(NET_TV_ALARM_FACE_COMPARE_INFO_S*)pAlarmInfo;
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
                NET_TV_ALARM_BASIC_INFO_S info = *(NET_TV_ALARM_BASIC_INFO_S*)pAlarmInfo;
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
                NET_TV_ALARM_RULE_INFO_S info = *(NET_TV_ALARM_RULE_INFO_S*)pAlarmInfo;
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
                NET_TV_ALARM_AI_OBJECT_INFO_S info = *(NET_TV_ALARM_AI_OBJECT_INFO_S*)pAlarmInfo;
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
                NET_TV_ALARM_PLATE_INFO_S info = *(NET_TV_ALARM_PLATE_INFO_S*)pAlarmInfo;
                SDKConvert::deal(pInfoJson, info, false);
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
                NET_TV_ALARM_EXCEPTION_INFO_S info = *(NET_TV_ALARM_EXCEPTION_INFO_S*)pAlarmInfo;
                SDKConvert::deal(pInfoJson, info, false);
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
                NET_TV_ALARM_STATISTICS_INFO_S info = *(NET_TV_ALARM_STATISTICS_INFO_S*)pAlarmInfo;
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
                NET_TV_RECORD_DOWNLOAD_PROGRESS_S info = *(NET_TV_RECORD_DOWNLOAD_PROGRESS_S*)pAlarmInfo;
                SDKConvert::deal(pInfoJson, info, false);
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

    // 推送到所有会话
    size_t pushCount = CSessionManager::instance()->PushToAll(jsonStr);
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
    return TRUE;
}

BOOL AlarmModule::PushChannelStatusInfo(NET_TV_CHANNEL_INFO_S* pChannelInfo)
{
    if (!pChannelInfo)
    {
        NSDK_LOG_ERROR("PushChannelStatusInfo: Invalid parameters");
        return FALSE;
    }

    Json::Object* pRoot = Json::init();
    if (!pRoot)
    {
        NSDK_LOG_ERROR("PushChannelStatusInfo: Failed to init JSON");
        return FALSE;
    }

    Json::add(pRoot, "Event", "ChannelStatus");
    Json::add(pRoot, "Command", (long long)NET_TV_NOTIFY_CHANNEL_STATUS);

    Json::Object* pChannelJson = Json::init();
    NET_TV_CHANNEL_INFO_S info = *pChannelInfo;
    SDKConvert::deal(pChannelJson, info, false);
    Json::add(pRoot, "ChannelInfo", pChannelJson);

    std::string jsonStr = Json::to_string(pRoot);
    Json::deinit(pRoot);

    size_t pushCount = CSessionManager::instance()->PushToAll(jsonStr);
    if (pushCount == 0)
    {
        NSDK_LOG_WARN("No active clients for channel status, channel=%u, online=%u",
            pChannelInfo->dwChannel, pChannelInfo->byOnline);
    }
    else
    {
        NSDK_LOG_DEBUG("Channel status pushed to %zu clients, channel=%u, online=%u",
            pushCount, pChannelInfo->dwChannel, pChannelInfo->byOnline);
    }

    m_pushCount++;
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
