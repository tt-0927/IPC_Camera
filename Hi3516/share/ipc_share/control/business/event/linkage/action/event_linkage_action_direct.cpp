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

#include "capture_ctrl.h"
#include "event_database_manage.h"
#include "event_linkage_dict.h"
#include "log_handler.h"
#include "record_ctrl.h"
#include "SipModule.h"

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
