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
#include <unistd.h>

#if CAP_GARBAGE_STATION_PLATFORM
#include <cstdlib>
#include <sstream>
#include <thread>
#endif

#include "capture_ctrl.h"
#include "capture_database.h"
#include "event_database_manage.h"
#include "event_linkage_dict.h"
#include "log_handler.h"
#include "record_ctrl.h"
#include "SipModule.h"

#if CAP_GARBAGE_STATION_PLATFORM
#include "cJSON.h"
#include "platform_manager.h"
#endif

#if CAP_GARBAGE_STATION_PLATFORM
namespace
{
constexpr const char *MQTT_EVENT_ALARM_COMMAND = "NET_TV_EVENT_ALARM";
constexpr const char *MQTT_EVENT_IMAGE_UPLOAD_COMMAND = "NET_TV_EVENT_IMAGE_UPLOAD";
constexpr int EVENT_IMAGE_WAIT_INTERVAL_MS = 500;
constexpr int EVENT_IMAGE_WAIT_TIMEOUT_MS = 3000;

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

    if (!stContext.mapAttrs.empty())
    {
        cJSON *pAttrs = cJSON_CreateObject();
        if (pAttrs)
        {
            bool bHasPublicAttr = false;
            for (const auto &item : stContext.mapAttrs)
            {
                if (item.first == "CaptureImagePath")
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

std::string build_event_image_upload_data(const ResolvedLinkagePlan_S &stPlan,
                                          const std::string &strAlarmRequestId,
                                          const CPlatformManager::EventImageUploadResponse &stResponse,
                                          bool bUploadOk,
                                          const std::string &strError)
{
    /* 图片上传结果单独发一条MQTT，避免图片上传耗时影响报警事件先到平台 */
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
    cJSON_AddStringToObject(pRoot, "AlarmRequestId", strAlarmRequestId.c_str());
    cJSON_AddNumberToObject(pRoot, "UploadStatus", bUploadOk ? 1 : 0);

    add_string_if_not_empty(pRoot, "Date", stEventInfo.strDate);
    cJSON_AddStringToObject(pRoot, "Time", strTimestamp.c_str());
    add_string_if_not_empty(pRoot, "StartTime", stEventInfo.strStartTime);
    add_string_if_not_empty(pRoot, "EndTime", stEventInfo.strEndTime);
    add_string_if_not_empty(pRoot, "ImagePath", stResponse.image_path);
    add_string_if_not_empty(pRoot, "FileName", stResponse.file_name);
    add_string_if_not_empty(pRoot, "ImageUrl", stResponse.image_url);

    if (stResponse.status_code != 0)
    {
        cJSON_AddNumberToObject(pRoot, "StatusCode", stResponse.status_code);
    }
    add_string_if_not_empty(pRoot, "Message", stResponse.message);
    add_string_if_not_empty(pRoot, "Error", strError);

    char *pJson = cJSON_PrintUnformatted(pRoot);
    std::string strData = pJson ? pJson : "{}";
    if (pJson)
    {
        free(pJson);
    }
    cJSON_Delete(pRoot);
    return strData;
}

void publish_event_image_upload_result(const ResolvedLinkagePlan_S &stPlan,
                                       const std::string &strAlarmRequestId,
                                       const CPlatformManager::EventImageUploadResponse &stResponse,
                                       bool bUploadOk,
                                       const std::string &strError)
{
    /* RequestId追加-image，既能区分报警消息，又能通过AlarmRequestId回查原报警 */
    std::ostringstream oss;
    oss << strAlarmRequestId << "-image";
    const std::string strData = build_event_image_upload_data(stPlan, strAlarmRequestId, stResponse, bUploadOk, strError);
    const int nRet = CPlatformManager::instance()->publish_event(MQTT_EVENT_IMAGE_UPLOAD_COMMAND, strData, oss.str());
    if (nRet != OK)
    {
        dlog_warn("MQTT事件图片上传结果发布失败: ret[%d], eventType[%d]",
                  nRet,
                  static_cast<int>(stPlan.stContext.enEventType));
    }
}

/**
 * @brief   : 从抓图数据库查询指定事件类型的最新图片路径
 * @param    {Event::Type_E} enEventType：事件类型
 * @param    {std::string &} strImagePath：输出图片路径
 * @return   {bool} true：找到图片 false：未找到
 * @note    : 用于人脸抓拍等由 AI 算法层自行保存图片、不走通用抓图系统的事件
 */
static bool query_latest_capture_image(Event::Type_E enEventType, std::string &strImagePath)
{
    /* Element 是 std::pair<string, variant<int, string>>，用构造函数初始化 */
    Db::Element stElem(Db::INFO_CAPTURE_EVENT_TYPE, static_cast<int>(enEventType));

    std::vector<Capture_NS::CaptureInfo_S> vecInfos;
    if (Db::CCaptureDatabase::instance()->find(stElem, vecInfos) != 0 || vecInfos.empty())
    {
        return false;
    }

    /* 取最新一条（数据库按插入时间排序，最后一条即最新） */
    strImagePath = vecInfos.back().strImagePath;
    return !strImagePath.empty();
}

static bool get_context_capture_image_path(const EventTriggerContext_S &stContext, std::string &strImagePath)
{
    auto it = stContext.mapAttrs.find("CaptureImagePath");
    if (it == stContext.mapAttrs.end() || it->second.empty())
    {
        return false;
    }

    if (access(it->second.c_str(), F_OK) != 0)
    {
        dlog_warn("事件上下文抓拍图片不存在: path[%s]", it->second.c_str());
        return false;
    }

    strImagePath = it->second;
    return true;
}

static bool is_face_compare_event(Event::Type_E enEventType)
{
    return enEventType == Event::Type_E::FACE_COMPARE_SUCCESS ||
           enEventType == Event::Type_E::FACE_COMPARE_FAIL;
}

void upload_event_image_async(ResolvedLinkagePlan_S stPlan, std::string strAlarmRequestId)
{
    /* 结束事件不上传图片；只有配置了抓图/存储联动时，才等待抓拍文件落盘 */
    if (stPlan.stContext.bEventEnded || !stPlan.bUploadSdCard)
    {
        return;
    }

    std::string strImagePath;

    if (get_context_capture_image_path(stPlan.stContext, strImagePath))
    {
        dlog_info("事件图片从事件上下文获取: eventType[%d], path[%s]",
                  static_cast<int>(stPlan.stContext.enEventType),
                  strImagePath.c_str());
    }
    /*
     * 人脸抓拍/人脸比对目标图由 AI 算法层直接保存，并通过 CaptureImagePath 传入。
     * 人脸抓拍保留数据库兜底；人脸比对要求上传当前比对目标图，没有上下文路径时直接失败，
     * 避免误传通用全景抓图。
     */
    else if (stPlan.stContext.enEventType == Event::Type_E::FACE_CAPTURE)
    {
        const auto start = std::chrono::steady_clock::now();
        while (true)
        {
            if (query_latest_capture_image(Event::Type_E::FACE_CAPTURE, strImagePath))
            {
                dlog_info("人脸抓拍图片从数据库获取: path[%s]", strImagePath.c_str());
                break;
            }

            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::steady_clock::now() - start)
                                     .count();
            if (elapsed >= EVENT_IMAGE_WAIT_TIMEOUT_MS)
            {
                CPlatformManager::EventImageUploadResponse stResponse;
                publish_event_image_upload_result(stPlan, strAlarmRequestId, stResponse, false, "face capture image not found in database");
                dlog_warn("人脸抓拍图片数据库查询超时: eventType[%d]", static_cast<int>(stPlan.stContext.enEventType));
                return;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(EVENT_IMAGE_WAIT_INTERVAL_MS));
        }
    }
    else if (is_face_compare_event(stPlan.stContext.enEventType))
    {
        CPlatformManager::EventImageUploadResponse stResponse;
        publish_event_image_upload_result(stPlan, strAlarmRequestId, stResponse, false, "face compare capture image not found in context");
        dlog_warn("人脸比对抓拍图片未随事件上下文传入: eventType[%d]", static_cast<int>(stPlan.stContext.enEventType));
        return;
    }
    else
    {
        /* 通用事件：轮询等待通用抓图系统完成首张图片 */
        const auto start = std::chrono::steady_clock::now();
        while (true)
        {
            if (CCaptureCtrl::instance()->get_event_first_capture_status(stPlan.stContext.enEventType, strImagePath) &&
                !strImagePath.empty())
            {
                break;
            }

            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::steady_clock::now() - start)
                                     .count();
            if (elapsed >= EVENT_IMAGE_WAIT_TIMEOUT_MS)
            {
                CPlatformManager::EventImageUploadResponse stResponse;
                publish_event_image_upload_result(stPlan, strAlarmRequestId, stResponse, false, "wait capture image timeout");
                dlog_warn("等待事件首张抓拍图超时: eventType[%d]", static_cast<int>(stPlan.stContext.enEventType));
                return;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(EVENT_IMAGE_WAIT_INTERVAL_MS));
        }
    }

    /*
     * 组装图片上传请求，字段结构与垃圾暴露事件完全一致：
     * - event_type:    事件类型码（int）
     * - event_name:    事件名称（string）
     * - channel:       通道号（int）
     * - timestamp:     毫秒级 Unix 时间戳（long long），格式与 get_event_timestamp_ms() 一致
     * - request_id:    关联告警事件的 RequestId（string）
     * - image_path:    设备端图片路径（string）
     *
     * HTTP 上传时由 CPlatformManager 统一补充 device_sn、生成文件名、读取图片二进制
     */
    CPlatformManager::EventImageUploadRequest stRequest;
    stRequest.event_type = static_cast<int>(stPlan.stContext.enEventType);
    stRequest.event_name = EventLinkageDict::get_event_name(stPlan.stContext.enEventType);
    stRequest.channel = stPlan.stContext.nChnId;
    stRequest.timestamp = get_event_timestamp_ms(stPlan);
    stRequest.request_id = strAlarmRequestId;
    stRequest.image_path = strImagePath;

    CPlatformManager::EventImageUploadResponse stResponse;
    const bool bUploadOk = CPlatformManager::instance()->upload_event_image(stRequest, stResponse);
    publish_event_image_upload_result(stPlan, strAlarmRequestId, stResponse, bUploadOk, bUploadOk ? "" : "upload event image failed");
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
    int nMqttRet = CPlatformManager::instance()->publish_event(MQTT_EVENT_ALARM_COMMAND, strMqttData, strRequestId);
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

    /* 报警消息先发；图片上传放到后台线程，成功或失败都会再发NET_TV_EVENT_IMAGE_UPLOAD结果 */
    if (!stPlan.stContext.bEventEnded && stPlan.bUploadSdCard)
    {
        std::thread(upload_event_image_async, stPlan, strRequestId).detach();
    }
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
