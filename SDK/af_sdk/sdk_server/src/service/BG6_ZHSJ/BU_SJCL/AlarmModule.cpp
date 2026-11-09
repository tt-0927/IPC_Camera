/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : CAlarmModule.cpp
 * @Description  : 告警推送管理模块实现，负责告警信息的推送和统计
 */

#include "AlarmModule.h"
#include "SessionModule.h"
#include "SessionManager.h"
#include "BG6_ZHSJ/BU_SJCL/AlarmInfoConvert.h"
#include "BG6_ZHSJ/BU_SJCL/RecordInfoConvert.h"
#include "DeviceInfoConvert.h"
#include "SDKConvert.h"
#include "Json.h"
#include "NetSdkLog.h"
#include <utility>

/**
 * 构造函数
 * @param pSessionModule 会话模块指针（用于获取会话信息）
 */
CAlarmModule::CAlarmModule(CSessionModule* pSessionModule)
    : m_pSessionModule(pSessionModule)
    , m_lPushCount(0)
{
    NETSDK_LOG_MESSAGE_DEBUG("CAlarmModule created");
}

/**
 * 析构函数
 */
CAlarmModule::~CAlarmModule()
{
    NETSDK_LOG_MESSAGE_DEBUG("CAlarmModule destroyed, total pushes: %llu", m_lPushCount);
}

/**
 * 推送告警信息到所有客户端
 * @details 根据命令码解析告警信息类型（人脸比对、基础告警、规则告警、AI告警、交通告警、异常告警、统计告警等），
 *          将告警信息转换为JSON格式后推送到所有活跃会话客户端
 * @param pAlarmer 告警设备信息
 * @param nCommand 命令码（报警类型）
 * @param pAlarmInfo 具体告警结构体指针
 * @param nBufLen pAlarmInfo 长度
 * @return TRUE表示成功推送到至少一个客户端，FALSE表示失败
 */
BOOL CAlarmModule::PushAlarmInfo(NET_Alarmer_S* pAlarmer,
                               INT32 nCommand,
                               LPVOID pAlarmInfo,
                               INT32 nBufLen)
{
    if (!pAlarmer || !pAlarmInfo || nBufLen <= 0)
    {
        NETSDK_LOG_MESSAGE_ERROR("PushAlarmInfo: Invalid parameters");
        return FALSE;
    }

    NETSDK_LOG_MESSAGE_DEBUG("Pushing alarm info: cmd=0x%x, device=%s", nCommand, pAlarmer->strDeviceIP);
    const size_t uTotalSessions = CSessionManager::instance()->GetSessionCount();

    // 构建告警JSON
    Json::Object* pRoot = Json::init();
    if (!pRoot)
    {
        NETSDK_LOG_MESSAGE_ERROR("PushAlarmInfo: Failed to init JSON");
        return FALSE;
    }

    // [诊断] 注入入队时间戳，方便下游测量队列延迟
    {
        auto stTpNow = std::chrono::steady_clock::now();
        long long lTsNow = std::chrono::duration_cast<std::chrono::milliseconds>(
            stTpNow.time_since_epoch()).count();
        Json::add(pRoot, "enqueue_ts", lTsNow);
    }

    Json::add(pRoot, "Command", (long long)nCommand);

    // 添加Alarmer信息
    {
        Json::Object* pAlarmerJson = Json::init();
        NET_Alarmer_S stTmp = *pAlarmer;
        SDKConvert::deal(pAlarmerJson, stTmp, false);
        Json::add(pRoot, "Alarmer", pAlarmerJson);
    }

    // 添加AlarmInfo（按命令码选择结构体）
    {
        Json::Object* pInfoJson = Json::init();
        /* 抓拍基类为 0x6100，必须按高字节分流，不能用 0xF000 与统计类 0x6000 混淆。 */
        INT32 nAlarmBase = nCommand & 0xFF00;

        if (nCommand == NET_ALARM_FACE_COMPARE)
        {
            if (nBufLen < (INT32)sizeof(NET_AlarmFaceCompareInfo_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)nCommand);
            }
            else
            {
                /* 含大图片数组，禁止栈上拷贝，直接引用原始数据 */
                NET_AlarmFaceCompareInfo_S& stInfo = *(NET_AlarmFaceCompareInfo_S*)pAlarmInfo;
                SDKConvert::deal(pInfoJson, stInfo, false);
            }
        }
        else if (nAlarmBase == NET_ALARM_BASE_BASIC)
        {
            if (nBufLen < (INT32)sizeof(NET_AlarmBasicInfo_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)nCommand);
            }
            else
            {
                NET_AlarmBasicInfo_S& stInfo = *(NET_AlarmBasicInfo_S*)pAlarmInfo;
                NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] Basic input: cmd=0x%x, alarmType=0x%x, timestamp=%lld, panoramaLen=%u, bufLen=%d, structSize=%zu",
                              nCommand,
                              stInfo.uAlarmType,
                              (long long)stInfo.llTimestampMs,
                              stInfo.uPanoramaImgLen,
                              nBufLen,
                              sizeof(NET_AlarmBasicInfo_S));
                SDKConvert::deal(pInfoJson, stInfo, false);
            }
        }
        else if (nAlarmBase == NET_ALARM_BASE_RULE)
        {
            if (nBufLen < (INT32)sizeof(NET_AlarmRuleInfo_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)nCommand);
            }
            else
            {
                /* 含大图片数组，禁止栈上拷贝，直接引用原始数据 */
                NET_AlarmRuleInfo_S& stInfo = *(NET_AlarmRuleInfo_S*)pAlarmInfo;
                NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] Rule input: cmd=0x%x, alarmType=0x%x, channel=%u, rule=%u, target=%u, "
                              "objType=%u, timestamp=%lld, rect=[%d,%d,%d,%d], panoramaLen=%u, targetLen=%u, bufLen=%d, structSize=%zu",
                              nCommand,
                              stInfo.uAlarmType,
                              stInfo.uChannel,
                              stInfo.uRuleID,
                              stInfo.uTargetID,
                              stInfo.uObjectType,
                              (long long)stInfo.llTimestampMs,
                              stInfo.nLeft,
                              stInfo.nTop,
                              stInfo.nRight,
                              stInfo.nBottom,
                              stInfo.uPanoramaImgLen,
                              stInfo.uTargetImgLen,
                              nBufLen,
                              sizeof(NET_AlarmRuleInfo_S));
                SDKConvert::deal(pInfoJson, stInfo, false);
            }
        }
        else if (nAlarmBase == NET_ALARM_BASE_AI)
        {
            if (nBufLen < (INT32)sizeof(NET_AlarmAiObjectInfo_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)nCommand);
            }
            else
            {
                /* 含大图片数组，禁止栈上拷贝，直接引用原始数据 */
                NET_AlarmAiObjectInfo_S& stInfo = *(NET_AlarmAiObjectInfo_S*)pAlarmInfo;
                NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] AI object input: cmd=0x%x, alarmType=0x%x, channel=%u, object=%s, "
                              "objType=%u, timestamp=%lld, rect=[%d,%d,%d,%d], panoramaLen=%u, imgLen=%u, bufLen=%d, structSize=%zu",
                              nCommand,
                              stInfo.uAlarmType,
                              stInfo.uChannel,
                              stInfo.strObjectID,
                              stInfo.uObjectType,
                              (long long)stInfo.llTimestampMs,
                              stInfo.nLeft,
                              stInfo.nTop,
                              stInfo.nRight,
                              stInfo.nBottom,
                              stInfo.uPanoramaImgLen,
                              stInfo.uImgLen,
                              nBufLen,
                              sizeof(NET_AlarmAiObjectInfo_S));
                SDKConvert::deal(pInfoJson, stInfo, false);
            }
        }
        else if (nAlarmBase == NET_ALARM_BASE_TRAFFIC)
        {
            if (nBufLen < (INT32)sizeof(NET_AlarmPlateInfo_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)nCommand);
            }
            else
            {
                SDKConvert::deal(pInfoJson, *(NET_AlarmPlateInfo_S*)pAlarmInfo, false);
            }
        }
        else if (nAlarmBase == NET_ALARM_BASE_EXCEPTION)
        {
            if (nBufLen < (INT32)sizeof(NET_AlarmExceptionInfo_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)nCommand);
            }
            else
            {
                SDKConvert::deal(pInfoJson, *(NET_AlarmExceptionInfo_S*)pAlarmInfo, false);
            }
        }
        else if (nAlarmBase == NET_ALARM_CLASSROOM_STATS_BASE)
        {
            /* 课堂统计子段（0x6200 累计计数 / 0x6201 行为段 / 0x6202 预警）：
               载荷为自描述二进制报文（魔数 CST1 + 版本 + 消息类型），不套用
               NET_AlarmStatisticsInfo_S；头非法或长度不足时只回传命令码。 */
            bool bParsed = false;
            if (nBufLen >= (INT32)sizeof(NET_ClassroomStatsHead_S))
            {
                NET_ClassroomStatsHead_S& stHead = *(NET_ClassroomStatsHead_S*)pAlarmInfo;
                /* 先做有符号差值并确认非负，再与无符号 payloadLen 比较，
                   避免 short buffer 下 signed→unsigned 转换绕过长度校验 */
                const INT32 nRemain = nBufLen - (INT32)sizeof(NET_ClassroomStatsHead_S);
                if (nRemain >= 0 &&
                    stHead.u32Magic == NET_CLASSROOM_STATS_MAGIC &&
                    stHead.u32PayloadLen <= static_cast<UINT32>(nRemain))
                {
                    if (nCommand == NET_ALARM_CLASSROOM_STATS_COUNTERS &&
                        nBufLen >= (INT32)sizeof(NET_ClassroomStatsCounters_S))
                    {
                        SDKConvert::deal(pInfoJson, *(NET_ClassroomStatsCounters_S*)pAlarmInfo, false);
                        bParsed = true;
                    }
                    else if (nCommand == NET_ALARM_CLASSROOM_STATS_SEGMENTS &&
                             nBufLen >= (INT32)sizeof(NET_ClassroomStatsSegments_S))
                    {
                        SDKConvert::deal(pInfoJson, *(NET_ClassroomStatsSegments_S*)pAlarmInfo, false);
                        bParsed = true;
                    }
                    else if (nCommand == NET_ALARM_CLASSROOM_STATS_ALERTS &&
                             nBufLen >= (INT32)sizeof(NET_ClassroomStatsAlerts_S))
                    {
                        SDKConvert::deal(pInfoJson, *(NET_ClassroomStatsAlerts_S*)pAlarmInfo, false);
                        bParsed = true;
                    }
                }
            }
            if (!bParsed)
            {
                Json::add(pInfoJson, "AlarmType", (long long)nCommand);
            }
        }
        else if (nAlarmBase == NET_ALARM_BASE_STATISTICS)
        {
            if (nBufLen < (INT32)sizeof(NET_AlarmStatisticsInfo_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)nCommand);
            }
            else
            {
                /* 统计类结构体含多个 1MB 图片数组，总大小约 3MB，禁止栈上拷贝，直接使用指针引用原始数据 */
                NET_AlarmStatisticsInfo_S& stInfo = *(NET_AlarmStatisticsInfo_S*)pAlarmInfo;
                SDKConvert::deal(pInfoJson, stInfo, false);
            }
        }
        else if (nAlarmBase == NET_ALARM_BASE_CAPTURE)
        {
            if (nBufLen < (INT32)sizeof(NET_AlarmCaptureInfo_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)nCommand);
            }
            else
            {
                /* 图片由调用方持有，转换过程在 PushAlarmInfo 返回前完成。 */
                NET_AlarmCaptureInfo_S& stInfo = *(NET_AlarmCaptureInfo_S*)pAlarmInfo;
                NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] Capture input: cmd=0x%x, alarmType=0x%x, channel=%u, "
                              "captureType=%u, timestamp=%lld, panorama=%ux%u/%u, cropCount=%u, "
                              "faceAttrs={male=%d,age=%d,glasses=%d,beard=%d,mask=%d,emotion=%d,faceExpression=%d}, bufLen=%d, structSize=%zu",
                              nCommand,
                              stInfo.uAlarmType,
                              stInfo.uChannel,
                              stInfo.uCaptureType,
                              (long long)stInfo.llTimestampMs,
                              stInfo.uPanoramaWidth,
                              stInfo.uPanoramaHeight,
                              stInfo.stPanoramaImg.uDataLen,
                              stInfo.uCropCount,
                              stInfo.stExtraInfo.bMale,
                              stInfo.stExtraInfo.nAgeLabel,
                              stInfo.stExtraInfo.bGlasses,
                              stInfo.stExtraInfo.bBeard,
                              stInfo.stExtraInfo.bMask,
                              stInfo.stExtraInfo.nEmotionLabel,
                              stInfo.stExtraInfo.nFaceExpression,
                              nBufLen,
                              sizeof(NET_AlarmCaptureInfo_S));
                SDKConvert::deal(pInfoJson, stInfo, false);
            }
        }
        else if (nCommand == NET_NOTICE_DOWNLOAD_RECORD_PROGRESS)
        {
            if (nBufLen < (INT32)sizeof(NET_RecordDownloadProgress_S))
            {
                Json::add(pInfoJson, "AlarmType", (long long)nCommand);
            }
            else
            {
                SDKConvert::deal(pInfoJson, *(NET_RecordDownloadProgress_S*)pAlarmInfo, false);
            }
        }
        else
        {
            Json::add(pInfoJson, "AlarmType", (long long)nCommand);
        }

        Json::add(pRoot, "AlarmInfo", pInfoJson);
    }

    // 转换为JSON字符串
    std::string strJson = Json::to_string(pRoot);
    Json::deinit(pRoot);
    if ((nCommand & 0xFF00) == NET_ALARM_BASE_RULE ||
        (nCommand & 0xFF00) == NET_ALARM_BASE_AI ||
        (nCommand & 0xFF00) == NET_ALARM_BASE_CAPTURE)
    {
        NETSDK_LOG_MESSAGE_INFO("[DIAG-ALARM] JSON output: cmd=0x%x, jsonLen=%zu, hasPanoramaB64=%d, hasTargetB64=%d, hasImgDataB64=%d, hasCaptureImgB64=%d, hasExtraInfo=%d",
                      nCommand,
                      strJson.size(),
                      strJson.find("PanoramaImgBase64") != std::string::npos ? 1 : 0,
                      strJson.find("TargetImgBase64") != std::string::npos ? 1 : 0,
                      strJson.find("ImgDataBase64") != std::string::npos ? 1 : 0,
                      strJson.find("\"ImgBase64\"") != std::string::npos ? 1 : 0,
                      strJson.find("ExtraInfo") != std::string::npos ? 1 : 0);
    }

    // [诊断] 记录 PushToAll 前的时间戳，用于定位报警转发延迟
    auto stTpBeforePush = std::chrono::steady_clock::now();
    long long lTsBeforePush = std::chrono::duration_cast<std::chrono::milliseconds>(
        stTpBeforePush.time_since_epoch()).count();

    // 推送到所有会话
    size_t uPushCount = CSessionManager::instance()->PushToAll(std::move(strJson));

    auto stTpAfterPush = std::chrono::steady_clock::now();
    long long lTsAfterPush = std::chrono::duration_cast<std::chrono::milliseconds>(
        stTpAfterPush.time_since_epoch()).count();
    NETSDK_LOG_MESSAGE_INFO("[DIAG] PushToAll done: cmd=0x%x, enqueue_ts=%lld, cost_ms=%lld",
                  nCommand, lTsBeforePush, lTsAfterPush - lTsBeforePush);
    if (uPushCount == 0)
    {
        std::string strDiagInfo = CSessionManager::instance()->GetSessionDiagnosticInfo();
        NETSDK_LOG_MESSAGE_WARN("CAlarmModule::PushAlarmInfo: FAILED - No eligible clients, cmd=0x%x, device=%s, totalSessions=%zu, bufLen=%d. "
                      "Client status: %s",
                      nCommand, pAlarmer->strDeviceIP, uTotalSessions, nBufLen, strDiagInfo.c_str());
    }
    else
    {
        NETSDK_LOG_MESSAGE_INFO("CAlarmModule::PushAlarmInfo: SUCCESS - Forwarded to %zu client(s), cmd=0x%x, device=%s, totalSessions=%zu, bufLen=%d",
                      uPushCount, nCommand, pAlarmer->strDeviceIP, uTotalSessions, nBufLen);
    }

    m_lPushCount++;
    return (uPushCount > 0) ? TRUE : FALSE;
}

/**
 * 推送通道上下线状态到所有客户端
 * @details 将通道状态信息转换为JSON格式后推送到所有活跃会话客户端
 * @param pChannelInfo 通道状态信息
 * @return TRUE表示成功，FALSE表示失败
 */
BOOL CAlarmModule::PushChannelStatusInfo(NET_ChannelInfo_S* pChannelInfo)
{
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] ===== PushChannelStatusInfo Start ===== ");

    if (!pChannelInfo)
    {
        NETSDK_LOG_MESSAGE_ERROR("[CAlarmModule] PushChannelStatusInfo: Invalid parameters (NULL)");
        NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] ===== PushChannelStatusInfo End (Failed) ===== ");
        return FALSE;
    }

    // 打印输入参数详情
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] Input Channel Info:");
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule]   Channel:      %u", pChannelInfo->uChannel);
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule]   ChannelName:  %s", pChannelInfo->strChannelName);
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule]   DeviceName:   %s", pChannelInfo->strDevName);
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule]   DeviceIP:     %s", pChannelInfo->strDeviceIP);
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule]   SerialNum:    %s", pChannelInfo->strSerialNum);
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
        return FALSE;
    }

    Json::add(pRoot, "Event", "ChannelStatus");
    Json::add(pRoot, "Command", (long long)NET_NOTIFY_CHANNEL_STATUS);
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] Command: NET_NOTIFY_CHANNEL_STATUS (0x%X)", NET_NOTIFY_CHANNEL_STATUS);

    Json::Object* pChannelJson = Json::init();
    NET_ChannelInfo_S stInfo = *pChannelInfo;
    SDKConvert::deal(pChannelJson, stInfo, false);
    Json::add(pRoot, "ChannelInfo", pChannelJson);

    std::string strJson = Json::to_string(pRoot);
    Json::deinit(pRoot);

    // 打印将要推送的JSON数据
    NETSDK_LOG_MESSAGE_DEBUG("[CAlarmModule] Channel Status JSON: %s", strJson.c_str());

    // 获取在线客户端数量
    size_t uClientCount = CSessionManager::instance()->GetSessionCount();
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] Current online clients: %zu", uClientCount);

    // 执行推送
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] Calling PushToAll...");
    size_t uPushCount = CSessionManager::instance()->PushToAll(std::move(strJson));

    if (uPushCount == 0)
    {
        NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] No active clients for channel status!");
        NETSDK_LOG_MESSAGE_WARN("[CAlarmModule]   Channel: %u, Online: %u",
                      pChannelInfo->uChannel, pChannelInfo->byOnline);
    }
    else
    {
        NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] Channel status pushed to %zu client(s) SUCCESS", uPushCount);
        NETSDK_LOG_MESSAGE_WARN("[CAlarmModule]   Channel: %u, Online: %s",
                      pChannelInfo->uChannel,
                      pChannelInfo->byOnline ? "ONLINE" : "OFFLINE");
    }

    m_lPushCount++;
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] Total push count: %lld", m_lPushCount);
    NETSDK_LOG_MESSAGE_WARN("[CAlarmModule] ===== PushChannelStatusInfo End ===== ");
    return TRUE;
}

/**
 * 获取告警推送总次数
 * @return 推送次数
 */
INT64 CAlarmModule::GetPushCount() const
{
    return m_lPushCount;
}

/**
 * 重置推送计数
 */
void CAlarmModule::ResetPushCount()
{
    NETSDK_LOG_MESSAGE_DEBUG("Resetting push count from %llu to 0", m_lPushCount);
    m_lPushCount = 0;
}
