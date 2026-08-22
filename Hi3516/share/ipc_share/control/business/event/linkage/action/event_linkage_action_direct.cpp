/**
 * @FilePath     : event_linkage_action_direct.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-15 16:29:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-16 10:26:17
 * @Description  : 事件联动同步动作执行器实现
 */

#include "event_linkage_action_direct.h"

#include <algorithm>
#include <chrono>
#include <ctime>

#if CAP_GARBAGE_STATION_PLATFORM
#include <cstdlib>
#include <sstream>
#endif

#include "capture_ctrl.h"
#include "event_database_manage.h"
#include "event_linkage_dict.h"
#include "log_handler.h"
#include "record_ctrl.h"
#include "SipModule.h"

#if CAP_GARBAGE_STATION_PLATFORM
#include "cJSON.h"
#include "platform_sdk_adapter.h"
#endif

#if CAP_GARBAGE_STATION_PLATFORM
namespace
{
constexpr const char *MQTT_EVENT_ALARM_COMMAND = "NET_TV_EVENT_ALARM";

/* 事件链路中可能同时有上下文时间和事件信息时间，优先使用触发上下文的毫秒时间戳 */
long long get_event_timestamp_ms(const ResolvedLinkagePlan_S &stPlan)
{
    if (stPlan.stContext.llTimestamp > 0)
    {
        return stPlan.stContext.llTimestamp;
    }

    if (stPlan.stEventInfo.lTimestamp > 0)
    {
        return stPlan.stEventInfo.lTimestamp;
    }

    return 0;
}

/* MQTT Data只放有值的可选字段，避免平台收到大量空字符串字段 */
void add_string_if_not_empty(cJSON *pRoot, const char *pKey, const std::string &strValue)
{
    if (!strValue.empty())
    {
        cJSON_AddStringToObject(pRoot, pKey, strValue.c_str());
    }
}

bool is_internal_event_attr(const std::string &strKey)
{
    return strKey == "CaptureImagePath";
}

bool should_expose_event_attr_to_root(Event::Type_E enEventType, const std::string &strKey)
{
    if (enEventType != Event::Type_E::FACE_COMPARE_SUCCESS &&
        enEventType != Event::Type_E::FACE_COMPARE_FAIL)
    {
        return false;
    }

    return strKey == "CompareResult" ||
           strKey == "CompareResultText" ||
           strKey == "Similarity" ||
           strKey == "SimilarityFloat" ||
           strKey == "Threshold" ||
           strKey == "ThresholdFloat" ||
           strKey == "FaceId" ||
           strKey == "Id" ||
           strKey == "Name" ||
           strKey == "PhoneNum" ||
           strKey == "PhoneNumber" ||
           strKey == "FaceLibName" ||
           strKey == "LibFacePath";
}

void add_public_event_attr_to_root(cJSON *pRoot,
                                   Event::Type_E enEventType,
                                   const std::string &strKey,
                                   const std::string &strValue)
{
    if (!should_expose_event_attr_to_root(enEventType, strKey))
    {
        return;
    }

    if (cJSON_GetObjectItemCaseSensitive(pRoot, strKey.c_str()) == nullptr)
    {
        cJSON_AddStringToObject(pRoot, strKey.c_str(), strValue.c_str());
    }

    if (strKey == "PhoneNum" && cJSON_GetObjectItemCaseSensitive(pRoot, "PhoneNumber") == nullptr)
    {
        cJSON_AddStringToObject(pRoot, "PhoneNumber", strValue.c_str());
    }
}

/* 同一次报警和后续图片上传结果共用同一个RequestId前缀，平台可据此做关联 */
std::string build_mqtt_event_request_id(const ResolvedLinkagePlan_S &stPlan)
{
    std::ostringstream oss;
    oss << "event-" << static_cast<int>(stPlan.stContext.enEventType) << "-" << stPlan.stContext.nChnId << "-"
        << get_event_timestamp_ms(stPlan);
    return oss.str();
}

/* 构造报警事件MQTT消息体：只描述事件本身，不等待抓拍和图片上传 */
std::string build_mqtt_event_data(const ResolvedLinkagePlan_S &stPlan)
{
    cJSON *pRoot = cJSON_CreateObject();
    if (!pRoot)
    {
        return "{}";
    }

    const Event::Info_S &stEventInfo = stPlan.stEventInfo;
    const EventTriggerContext_S &stContext = stPlan.stContext;
    const std::string strTimestamp = std::to_string(get_event_timestamp_ms(stPlan));

    cJSON_AddNumberToObject(pRoot, "EventType", static_cast<int>(stContext.enEventType));
    cJSON_AddStringToObject(pRoot, "EventName", EventLinkageDict::get_event_name(stContext.enEventType).c_str());
    cJSON_AddNumberToObject(pRoot, "EventStatus", stContext.bEventEnded ? 0 : 1);
    cJSON_AddNumberToObject(pRoot, "Channel", stContext.nChnId);
    cJSON_AddStringToObject(pRoot, "Timestamp", strTimestamp.c_str());

    add_string_if_not_empty(pRoot, "Date", stEventInfo.strDate);
    cJSON_AddStringToObject(pRoot, "Time", strTimestamp.c_str());
    add_string_if_not_empty(pRoot, "StartTime", stEventInfo.strStartTime);
    add_string_if_not_empty(pRoot, "EndTime", stEventInfo.strEndTime);
    add_string_if_not_empty(pRoot, "Label", stEventInfo.strLabel);
    add_string_if_not_empty(pRoot, "VideoPath", stEventInfo.strVideoPath);

    if (stEventInfo.nVideoSize > 0)
    {
        cJSON_AddNumberToObject(pRoot, "VideoSize", stEventInfo.nVideoSize);
    }

    if (stContext.enEventType == Event::Type_E::FACE_COMPARE_SUCCESS ||
        stContext.enEventType == Event::Type_E::FACE_COMPARE_FAIL)
    {
        for (const auto &item : stContext.mapAttrs)
        {
            if (is_internal_event_attr(item.first))
            {
                continue;
            }
            add_public_event_attr_to_root(pRoot, stContext.enEventType, item.first, item.second);
        }
    }
    else if (!stContext.mapAttrs.empty())
    {
        cJSON *pAttrs = cJSON_CreateObject();
        if (pAttrs)
        {
            bool bHasPublicAttr = false;
            for (const auto &item : stContext.mapAttrs)
            {
                if (is_internal_event_attr(item.first))
                {
                    continue;
                }
                cJSON_AddStringToObject(pAttrs, item.first.c_str(), item.second.c_str());
                bHasPublicAttr = true;
            }
            if (bHasPublicAttr)
            {
                cJSON_AddItemToObject(pRoot, "Attrs", pAttrs);
            }
            else
            {
                cJSON_Delete(pAttrs);
            }
        }
    }

    char *pJson = cJSON_PrintUnformatted(pRoot);
    std::string strData = pJson ? pJson : "{}";
    if (pJson)
    {
        free(pJson);
    }
    cJSON_Delete(pRoot);
    return strData;
}

} // namespace
#endif

int EventLinkageDirectAction::deal_record(const ResolvedLinkagePlan_S &stPlan, const Event::EventState_S &stEventState)
{
    for (const auto &nRecordChn : stPlan.stLinkageList.recordChn)
    {
        /* 当前实现只处理本通道录像，跨通道联动仍沿用原有配置行为 */
        if (nRecordChn == stPlan.stContext.nChnId)
        {
            dlog_info("报警录制联动");
            return create_event_video(stEventState);
        }
    }

    return OK;
}

int EventLinkageDirectAction::deal_capture_image(const ResolvedLinkagePlan_S &stPlan)
{
    dlog_info("抓图联动");
    CCaptureCtrl::instance()->set_event_capture(stPlan.stContext.bEventEnded, stPlan.stEventInfo);
    return OK;
}

int EventLinkageDirectAction::deal_upload(const ResolvedLinkagePlan_S &stPlan)
{
    if (!stPlan.bUploadToCenter)
    {
        return OK;
    }

    /* GB28181上传所需的报警方法、类型和附加参数 */
    int nGbMethod = 0;
    int nGbType = 0;
    int nGbTypeParam = 0;
    bool bIfAlarm = true;

    switch (stPlan.stContext.enEventType)
    {
    case Event::Type_E::ENTER_REGION:
        nGbMethod = GB28181::METHOD_VIDEO;
        nGbType = static_cast<int>(GB28181::AlarmVideoType_E::INTRUSION);
        nGbTypeParam = 1;
        break;
    case Event::Type_E::LEAVE_REGION:
        nGbMethod = GB28181::METHOD_VIDEO;
        nGbType = static_cast<int>(GB28181::AlarmVideoType_E::INTRUSION);
        nGbTypeParam = 2;
        break;
    case Event::Type_E::MOTION_DETECT:
        nGbMethod = GB28181::METHOD_VIDEO;
        nGbType = static_cast<int>(GB28181::AlarmVideoType_E::DETECT_MOVE);
        break;
    case Event::Type_E::OCCLUSION_DETECT:
        nGbMethod = GB28181::METHOD_VIDEO;
        nGbType = static_cast<int>(GB28181::AlarmVideoType_E::HIDE);
        break;
    case Event::Type_E::LOITERING_DETECT:
        nGbMethod = GB28181::METHOD_VIDEO;
        nGbType = static_cast<int>(GB28181::AlarmVideoType_E::LOITERING);
        break;
    case Event::Type_E::UNATTENDED_OBJECT:
        nGbMethod = GB28181::METHOD_VIDEO;
        nGbType = static_cast<int>(GB28181::AlarmVideoType_E::UNATTENDED_BAGGAGE);
        break;
    case Event::Type_E::OBJECT_REMOVAL:
        nGbMethod = GB28181::METHOD_VIDEO;
        nGbType = static_cast<int>(GB28181::AlarmVideoType_E::ATTENDED_BAGGAGE);
        break;
    default:
        bIfAlarm = false;
        break;
    }

    dlog_debug("是否报警:%u, 客户端状态:%u", bIfAlarm, SIP::SipModule::instance()->get_client_status());
    if (bIfAlarm && SIP::SipModule::instance()->get_client_status())
    {
        /* 仅在事件支持且SIP已连接时，才向中心发送报警信息 */
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

        GB28181::AlarmInfo_S stGBAlarmInfo;
        stGBAlarmInfo.nIndex = 1;
        stGBAlarmInfo.enPriority = GB28181::PRIORITY_1;
        stGBAlarmInfo.enMethod = static_cast<GB28181::AlarmMethod_E>(nGbMethod);
        stGBAlarmInfo.nTime = currentTime;
        stGBAlarmInfo.enType = nGbType;
        stGBAlarmInfo.enTypeParam = nGbTypeParam;
        SIP::SipModule::instance()->SendAlarmInfo(stGBAlarmInfo);
    }

#if CAP_GARBAGE_STATION_PLATFORM
    const std::string strMqttData = build_mqtt_event_data(stPlan);
    const std::string strRequestId = build_mqtt_event_request_id(stPlan);
    const std::string strEventName =
        EventLinkageDict::get_event_name(stPlan.stContext.enEventType);
    std::string strImagePath;
    const auto stImagePathIterator =
        stPlan.stContext.mapAttrs.find("CaptureImagePath");
    if (stImagePathIterator != stPlan.stContext.mapAttrs.end())
    {
        strImagePath = stImagePathIterator->second;
    }

    const bool bUploadImage =
        !stPlan.stContext.bEventEnded && stPlan.bUploadSdCard;
    dlog_info("MQTT上传中心事件数据: requestId[%s], data[%s]", strRequestId.c_str(), strMqttData.c_str());
    const int nMqttRet = CPlatformSdkAdapter::instance()->report_event(
        MQTT_EVENT_ALARM_COMMAND,
        strRequestId,
        strMqttData,
        static_cast<int>(stPlan.stContext.enEventType),
        strEventName,
        stPlan.stContext.nChnId,
        get_event_timestamp_ms(stPlan),
        strImagePath,
        bUploadImage,
        bUploadImage);
    if (nMqttRet == OK)
    {
        dlog_info("MQTT上传中心事件发布成功: command[%s], requestId[%s], eventType[%d]",
                  MQTT_EVENT_ALARM_COMMAND,
                  strRequestId.c_str(),
                  static_cast<int>(stPlan.stContext.enEventType));
    }
    else
    {
        dlog_warn("MQTT上传中心事件发布失败: ret[%d], command[%s], requestId[%s], eventType[%d]",
                  nMqttRet,
                  MQTT_EVENT_ALARM_COMMAND,
                  strRequestId.c_str(),
                  static_cast<int>(stPlan.stContext.enEventType));
    }

    /* SDK publishes the alarm first, then its bounded transfer worker uploads the image. */
#endif

    dlog_info("上传中心联动");
    return OK;
}

int EventLinkageDirectAction::write_log(const Event::Info_S &stEventInfo, bool bEventEnded)
{
    Log::Info_S stLogInfo;
    /* 开始事件记录开始时间，结束事件记录结束时间，便于日志侧正确展示 */
    stLogInfo.startTime = bEventEnded ? stEventInfo.strEndTime : stEventInfo.strStartTime;
    stLogInfo.nType = Log::Type::ALARM;

    if (stEventInfo.enType >= Event::Type::DISK_FULL && stEventInfo.enType <= Event::Type::VIDEO_SIGNAL_LOSS)
    {
        /* 异常类事件在日志系统中归类到异常日志，而不是普通告警日志 */
        stLogInfo.nType = Log::Type::EXCEPTION;
    }

    stLogInfo.nAction = Log::to_action(stEventInfo.enType, !bEventEnded);
    stLogInfo.chnName = std::string("D") + std::to_string(stEventInfo.nChnId);
    stLogInfo.context = EventLinkageDict::get_event_name(stEventInfo.enType);
    LogHandler::instance()->write(stLogInfo);
    return OK;
}

int EventLinkageDirectAction::create_event_video(const Event::EventState_S &stEventState)
{
    Event::Info_S stEventInfo = stEventState.stEventInfo;
    if (stEventState.nEventStatus == 0)
    {
        /*若录制因存储满已停止，且事件开始时间在录制停止之前，则截断endTime为录制停止时间，避免事件记录时间超出实际录像范围*/
        if (CRecordCtrl::instance()->isStoppedDueToStorage())
        {
           const std::string& strStopTime = CRecordCtrl::instance()->getLastRecordStopTime();
            if (!strStopTime.empty() && stEventInfo.strStartTime < strStopTime)
            {
                dlog_info("录制已停止，截断事件endTime: %s -> %s",stEventInfo.strEndTime.c_str(), strStopTime.c_str());
                stEventInfo.strEndTime = strStopTime;
            }
        }

        /* 结束阶段补齐录像文件路径，并根据录像模式更新数据库或停止事件录像 */
        dlog_debug("事件结束，更新到事件数据库");
        std::string strDate = stEventInfo.strDate;
        strDate.erase(std::remove(strDate.begin(), strDate.end(), '-'), strDate.end());
        stEventInfo.strVideoPath = std::string(RECORD_PATH) + "/" + strDate + "/normal_" + strDate + ".m3u8";

        if (CRecordCtrl::instance()->get_RecordScheduleType() == 1)
        {
            EventDatabaseManage::instance()->update(stEventInfo);
        }
        else if (CRecordCtrl::instance()->get_RecordScheduleType() == 2)
        {
            EventDatabaseManage::instance()->update(stEventInfo);
            CRecordCtrl::instance()->set_event_record(false, stEventInfo);
        }
    }
    else if (stEventState.nEventStatus == 1)
    {
        /* 开始阶段先写事件数据库，事件录像模式下还要启动录像 */
        if (CRecordCtrl::instance()->get_RecordScheduleType() == 1)
        {
            if (CRecordCtrl::instance()->get_record_status() == Record_NS::RECORD_OPERATION)
            {
                EventDatabaseManage::instance()->add(stEventInfo);
            }
        }
        else if (CRecordCtrl::instance()->get_RecordScheduleType() == 2)
        {
            EventDatabaseManage::instance()->add(stEventInfo);
            CRecordCtrl::instance()->set_event_record(true, stEventInfo);
        }
    }

    return OK;
}
