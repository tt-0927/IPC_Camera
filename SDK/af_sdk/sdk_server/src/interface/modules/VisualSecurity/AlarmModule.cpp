/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : AlarmModule.cpp
 * @Description  : 告警推送管理模块实现，负责告警信息的推送和统计
 */

#include "AlarmModule.h"
#include "SessionModule.h"
#include "SessionManager.h"
#include "AlarmInfoConvert.h"
#include "DeviceInfoConvert.h"
#include "SDKConvert.h"
#include "Json.h"
#include "NetSdkLog.h"

/**
 * 构造函数
 * @param pSessionModule 会话模块指针（用于获取会话信息）
 */
AlarmModule::AlarmModule(SessionModule* pSessionModule)
    : m_pSessionModule(pSessionModule)
    , m_pushCount(0)
{
    NSDK_LOG_DEBUG("AlarmModule created");
}

/**
 * 析构函数
 */
AlarmModule::~AlarmModule()
{
    NSDK_LOG_DEBUG("AlarmModule destroyed, total pushes: %llu", m_pushCount);
}

/**
 * 推送告警信息到所有客户端
 * @details 根据命令码解析告警信息类型（人脸比对、基础告警、规则告警、AI告警、交通告警、异常告警、统计告警等），
 *          将告警信息转换为JSON格式后推送到所有活跃会话客户端
 * @param pAlarmer 告警设备信息
 * @param lCommand 命令码（报警类型）
 * @param pAlarmInfo 具体告警结构体指针
 * @param dwBufLen pAlarmInfo 长度
 * @return TRUE表示成功推送到至少一个客户端，FALSE表示失败
 */
BOOL AlarmModule::PushAlarmInfo(NET_Alarmer_S* pAlarmer,
                               INT32 lCommand,
                               LPVOID pAlarmInfo,
                               INT32 dwBufLen)
{
    if (!pAlarmer || !pAlarmInfo || dwBufLen <= 0)
    {
        NSDK_LOG_ERROR("PushAlarmInfo: Invalid parameters");
        return FALSE;
    }

    NSDK_LOG_DEBUG("Pushing alarm info: cmd=0x%x, device=%s", lCommand, pAlarmer->strDeviceIP);
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
        NET_Alarmer_S tmp = *pAlarmer;
        SDKConvert::deal(pAlarmerJson, tmp, false);
        Json::add(pRoot, "Alarmer", pAlarmerJson);
    }

    // 添加AlarmInfo（按命令码选择结构体）
    {
        Json::Object* pInfoJson = Json::init();
        INT32 alarmBase = lCommand & 0xF000;

        if (lCommand == NET_ALARM_FACE_COMPARE)
        {
            if (dwBufLen < (INT32)sizeof(NET_AlarmFaceCompareInfo_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                /* 含大图片数组，禁止栈上拷贝，直接引用原始数据 */
                NET_AlarmFaceCompareInfo_S& info = *(NET_AlarmFaceCompareInfo_S*)pAlarmInfo;
                SDKConvert::deal(pInfoJson, info, false);
            }
        }
        else if (alarmBase == NET_ALARM_BASE_BASIC)
        {
            if (dwBufLen < (INT32)sizeof(NET_AlarmBasicInfo_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                NET_AlarmBasicInfo_S& info = *(NET_AlarmBasicInfo_S*)pAlarmInfo;
                NSDK_LOG_INFO("[DIAG-ALARM] Basic input: cmd=0x%x, alarmType=0x%x, timestamp=%lld, panoramaLen=%u, bufLen=%d, structSize=%zu",
                              lCommand,
                              info.uAlarmType,
                              (long long)info.llTimestampMs,
                              info.uPanoramaImgLen,
                              dwBufLen,
                              sizeof(NET_AlarmBasicInfo_S));
                SDKConvert::deal(pInfoJson, info, false);
            }
        }
        else if (alarmBase == NET_ALARM_BASE_RULE)
        {
            if (dwBufLen < (INT32)sizeof(NET_AlarmRuleInfo_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                /* 含大图片数组，禁止栈上拷贝，直接引用原始数据 */
                NET_AlarmRuleInfo_S& info = *(NET_AlarmRuleInfo_S*)pAlarmInfo;
                NSDK_LOG_INFO("[DIAG-ALARM] Rule input: cmd=0x%x, alarmType=0x%x, channel=%u, rule=%u, target=%u, "
                              "objType=%u, timestamp=%lld, rect=[%d,%d,%d,%d], panoramaLen=%u, targetLen=%u, bufLen=%d, structSize=%zu",
                              lCommand,
                              info.uAlarmType,
                              info.uChannel,
                              info.uRuleID,
                              info.uTargetID,
                              info.uObjectType,
                              (long long)info.llTimestampMs,
                              info.nLeft,
                              info.nTop,
                              info.nRight,
                              info.nBottom,
                              info.uPanoramaImgLen,
                              info.uTargetImgLen,
                              dwBufLen,
                              sizeof(NET_AlarmRuleInfo_S));
                SDKConvert::deal(pInfoJson, info, false);
            }
        }
        else if (alarmBase == NET_ALARM_BASE_AI)
        {
            if (dwBufLen < (INT32)sizeof(NET_AlarmAiObjectInfo_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                /* 含大图片数组，禁止栈上拷贝，直接引用原始数据 */
                NET_AlarmAiObjectInfo_S& info = *(NET_AlarmAiObjectInfo_S*)pAlarmInfo;
                NSDK_LOG_INFO("[DIAG-ALARM] AI object input: cmd=0x%x, alarmType=0x%x, channel=%u, object=%s, "
                              "objType=%u, timestamp=%lld, rect=[%d,%d,%d,%d], panoramaLen=%u, imgLen=%u, bufLen=%d, structSize=%zu",
                              lCommand,
                              info.uAlarmType,
                              info.uChannel,
                              info.strObjectID,
                              info.uObjectType,
                              (long long)info.llTimestampMs,
                              info.nLeft,
                              info.nTop,
                              info.nRight,
                              info.nBottom,
                              info.uPanoramaImgLen,
                              info.uImgLen,
                              dwBufLen,
                              sizeof(NET_AlarmAiObjectInfo_S));
                SDKConvert::deal(pInfoJson, info, false);
            }
        }
        else if (alarmBase == NET_ALARM_BASE_TRAFFIC)
        {
            if (dwBufLen < (INT32)sizeof(NET_AlarmPlateInfo_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                SDKConvert::deal(pInfoJson, *(NET_AlarmPlateInfo_S*)pAlarmInfo, false);
            }
        }
        else if (alarmBase == NET_ALARM_BASE_EXCEPTION)
        {
            if (dwBufLen < (INT32)sizeof(NET_AlarmExceptionInfo_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                SDKConvert::deal(pInfoJson, *(NET_AlarmExceptionInfo_S*)pAlarmInfo, false);
            }
        }
        else if (alarmBase == NET_ALARM_BASE_STATISTICS)
        {
            if (dwBufLen < (INT32)sizeof(NET_AlarmStatisticsInfo_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                /* 统计类结构体含多个 1MB 图片数组，总大小约 3MB，禁止栈上拷贝，直接使用指针引用原始数据 */
                NET_AlarmStatisticsInfo_S& info = *(NET_AlarmStatisticsInfo_S*)pAlarmInfo;
                SDKConvert::deal(pInfoJson, info, false);
            }
        }
        else if (lCommand == NET_NOTICE_DOWNLOAD_RECORD_PROGRESS)
        {
            if (dwBufLen < (INT32)sizeof(NET_RecordDownloadProgress_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)lCommand);
            }
            else
            {
                SDKConvert::deal(pInfoJson, *(NET_RecordDownloadProgress_S*)pAlarmInfo, false);
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
    if ((lCommand & 0xF000) == NET_ALARM_BASE_RULE ||
        (lCommand & 0xF000) == NET_ALARM_BASE_AI)
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
                      lCommand, pAlarmer->strDeviceIP, totalSessions, dwBufLen, diagInfo.c_str());
    }
    else
    {
        NSDK_LOG_INFO("AlarmModule::PushAlarmInfo: SUCCESS - Forwarded to %zu client(s), cmd=0x%x, device=%s, totalSessions=%zu, bufLen=%d",
                      pushCount, lCommand, pAlarmer->strDeviceIP, totalSessions, dwBufLen);
    }

    m_pushCount++;
    return (pushCount > 0) ? TRUE : FALSE;
}

/**
 * 推送通道上下线状态到所有客户端
 * @details 将通道状态信息转换为JSON格式后推送到所有活跃会话客户端
 * @param pChannelInfo 通道状态信息
 * @return TRUE表示成功，FALSE表示失败
 */
BOOL AlarmModule::PushChannelStatusInfo(NET_ChannelInfo_S* pChannelInfo)
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
    NSDK_LOG_WARN("[AlarmModule]   Channel:      %u", pChannelInfo->uChannel);
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
    Json::add(pRoot, "Command", (long long)NET_NOTIFY_CHANNEL_STATUS);
    NSDK_LOG_WARN("[AlarmModule] Command: NET_NOTIFY_CHANNEL_STATUS (0x%X)", NET_NOTIFY_CHANNEL_STATUS);

    Json::Object* pChannelJson = Json::init();
    NET_ChannelInfo_S info = *pChannelInfo;
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
                      pChannelInfo->uChannel, pChannelInfo->byOnline);
    }
    else
    {
        NSDK_LOG_WARN("[AlarmModule] Channel status pushed to %zu client(s) SUCCESS", pushCount);
        NSDK_LOG_WARN("[AlarmModule]   Channel: %u, Online: %s",
                      pChannelInfo->uChannel,
                      pChannelInfo->byOnline ? "ONLINE" : "OFFLINE");
    }

    m_pushCount++;
    NSDK_LOG_WARN("[AlarmModule] Total push count: %lld", m_pushCount);
    NSDK_LOG_WARN("[AlarmModule] ===== PushChannelStatusInfo End ===== ");
    return TRUE;
}

/**
 * 获取告警推送总次数
 * @return 推送次数
 */
INT64 AlarmModule::GetPushCount() const
{
    return m_pushCount;
}

/**
 * 重置推送计数
 */
void AlarmModule::ResetPushCount()
{
    NSDK_LOG_DEBUG("Resetting push count from %llu to 0", m_pushCount);
    m_pushCount = 0;
}
