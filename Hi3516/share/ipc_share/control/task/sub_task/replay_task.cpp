/*
 * @Author: zhangjc zhangjc@kfb.cn
 * @Date: 2024-10-29 19:33:17
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-05-29 11:13:56
 * @FilePath: /hisi/share/ipc_share/control/task/sub_task/replay_task.cpp
 * @Description: 录像回放
 */

#include "replay_task.h"
#include "dlog.h"
#include "convert_interface.h"
#include "record_file_manage.h"
#include "m3u8.h"
#include <iomanip>
#include "event_search.h"
#include "storage_manage.h"

/* 普通事件 */
static const Event::Type_E ORDINARY_EVENT_MAP[] =
{
    ::Event::Type_E::MOTION_DETECT,
    ::Event::Type_E::ALARM_INPUT,
};

/* 周界事件 */
static const Event::Type_E PERIMETER_INCIDENT_EVENT_MAP[] =
{
    ::Event::Type_E::LINE_CROSSING,
    ::Event::Type_E::INTRUSION,
    ::Event::Type_E::ENTER_REGION,
    ::Event::Type_E::LEAVE_REGION,
};

/* 行为分析事件 */
static const Event::Type_E BEHAVIOR_ANALYSIS_EVENT_MAP[] =
{
    ::Event::Type_E::LOITERING_DETECT,
    ::Event::Type_E::CROWD_GATHERING,
    ::Event::Type_E::PARKING_DETECT,
};

/* 场景检测事件 */
static const Event::Type_E SCENE_DETECTION_EVENT_MAP[] =
{
    ::Event::Type_E::AUDIO_ANOMALY,
    ::Event::Type_E::AUDIO_SUDDEN_RISE,
    ::Event::Type_E::AUDIO_SUDDEN_DROP,
    ::Event::Type_E::SCENE_CHANGE,
    ::Event::Type_E::UNATTENDED_OBJECT,
    ::Event::Type_E::OBJECT_REMOVAL,
};

/* 目标检测事件 */
static const Event::Type_E TARGET_DETECTION_EVENT_MAP[] =
{
    ::Event::Type_E::FACE_DETECT,
    ::Event::Type_E::PET_RECOGNITION,
};

/* 人脸抓拍事件 */
static const Event::Type_E FACE_CAPTURE_EVENT_MAP[] =
{
    ::Event::Type_E::FACE_CAPTURE,
};

/* 行为监管事件 */
static const Event::Type_E BEHAVIOR_MONITORING_EVENT_MAP[] =
{
    ::Event::Type_E::SLEEP_ON_DUTY,
    ::Event::Type_E::LEAVE_POST,
    ::Event::Type_E::ELECTRIC_VEHICLE_IN_ELEVATOR,
    ::Event::Type_E::PERSON_FALL_DOWN,
    ::Event::Type_E::FENCE_CLIMBING,
    ::Event::Type_E::SMOKING,
    ::Event::Type_E::PHONE_USAGE,
    ::Event::Type_E::GARBAGE_EXPOSURE,
    ::Event::Type_E::SMOKE_FIRE,
    ::Event::Type_E::OPEN_FLAME,
    ::Event::Type_E::GARBAGE_OVERFLOW,
    ::Event::Type_E::MANHOLE_COVER_ABNORMAL,
    ::Event::Type_E::BARE_SOIL,
    ::Event::Type_E::HOLE_PROTECTION_BAR,
    ::Event::Type_E::PEDESTRIAN_INTRUSION,
    ::Event::Type_E::PERSON_TRIP,
};

/* 穿戴规范事件 */
static const Event::Type_E CLOTHING_COMPLIANCE_EVENT_MAP[] =
{
    ::Event::Type_E::SAFETY_HELMET,
    ::Event::Type_E::REFLECTIVE_CLOTHING,
    ::Event::Type_E::HIGH_ALTITUDE_SEATBELT,
};

/* 交通行为监管事件 */
static const Event::Type_E TRAFFIC_BEHAVIOR_MONITORING_EVENT_MAP[] =
{
    ::Event::Type_E::CONSTRUCTION_OCCUPY_ROAD,
    ::Event::Type_E::EMERGENCY_LANE_OCCUPANCY,
    ::Event::Type_E::REVERSE_DIRECTION,
    ::Event::Type_E::NON_MOTOR_VEHICLE_INTRUSION,
    ::Event::Type_E::ROAD_PONDING,
    ::Event::Type_E::CONGESTION,
    // ::Event::Type_E::ILLEGAL_PARKING,
    ::Event::Type_E::ILLEGAL_LANE_CHANGE,
};

/* 属性识别事件 */
static const Event::Type_E ATTRIBUTE_RECOGNITION_EVENT_MAP[] =
{
    ::Event::Type_E::PLATE_NUMBER,
};

/* 设置回放布局信息 */
void Task::Replay::SetLayoutInfo::handle()
{
    SD_CARD_STATUS_E eSdCardStatus = CStorageManage::instance()->get_SdCardStatus();
    /* sd卡异常以及录制ts文件信息数据库不存在都返回空数据 */
    if (!(std::filesystem::exists(RECORD_DATABASE_PATH)) ||
         (eSdCardStatus != SD_CARD_STATUS_E::NORMAL && eSdCardStatus != SD_CARD_STATUS_E::WRITE_ERROR))
    {
        std::string retrievalResult = "{\"reason\":\"record data is empty.\"}";
        result(retrievalResult, -1);
        return;
    }
    ::Replay::LayoutInfo_S stLayoutInfo;
    Convert::to_struct(m_taskData, stLayoutInfo);

    /* 找到对应日期的录制文件 */
    ::Record_NS::Find_S stFind;
    stFind.date = stLayoutInfo.date;
    /* 普通视频与事件视频是一样的 */
    stFind.nType = 0;
    for (auto &stChnInfo : stLayoutInfo.chnInfos)
    {
        if (stChnInfo.stItem.nChnId == -1)
        {
            continue;
        }
        stFind.chnIds.push_back(stChnInfo.stItem.nChnId);
    }

    std::vector<::Record_NS::FindResult_S> outInfos;
    RecordFileManage::instance()->find(stFind, outInfos);



    /* 如果数据库中查找结果为空 */
    if(!outInfos.size())
    {
        /* 从录制目录中获取存在m3u8的所有目录 */
        std::vector<std::string> vecResult = RecordFileManage::instance()->findM3u8Dates(RECORD_PATH, "normal");
        for(auto &Result : vecResult)
        {
            /* 比对所有目录是否有符合查找条件的 */
            if(Result == stFind.date)
            {
                // /opt/course/record/20260121/normal_20260121.m3u8
                Record_NS::FindResult_S stFindResult;
                stFindResult.nChnId = 0;
                stFindResult.dates.push_back(Result);
                Result.erase(std::remove(Result.begin(), Result.end(), '-'), Result.end());
                stFindResult.filename = RECORD_PATH + std::string("/") + Result + std::string("/normal_") + Result + std::string(".m3u8");

                outInfos.push_back(stFindResult);
            }
        }
    }

    for (auto &outInfo : outInfos)
    {
        for (auto &findInfo :  stLayoutInfo.chnInfos)
        {
            if (findInfo.stItem.nChnId == outInfo.nChnId)
            {
                findInfo.filename = outInfo.filename;
            }
        }
        M3U8 m3u8(outInfo.filename);
        /* 录像结束时间 */
        auto videoTimes = m3u8.get_videoTime();
        stLayoutInfo.recordTime.videoTimes.insert(stLayoutInfo.recordTime.videoTimes.end(), videoTimes.begin(), videoTimes.end());
    }

    /* 获取事件时间 */
    std::vector<::Event::Info_S> eventInfos;
    ::Event::RetrievalCond_S stEventCond = {};
    stEventCond.nChnIds =  stFind.chnIds;
    stEventCond.enType = ::Event::Type::UNKNOWN;

    if(stLayoutInfo.bSmartVideoSummary)
    {
        stEventCond.strStartTime = stLayoutInfo.strStartTime;
        stEventCond.strEndTime = stLayoutInfo.strEndTime;

        switch (stLayoutInfo.enSummaryType)
        {
            case ::Replay::SummaryType_E::ORDINARY_TYPE:
            {
                for(unsigned int i = 0; i < sizeof(ORDINARY_EVENT_MAP)/sizeof(::Event::Type_E); i++)
                {
                    stEventCond.enType = ORDINARY_EVENT_MAP[i];
                    EventSearch::instance()->searchByEventType(stEventCond, eventInfos);
                }
                break;
            }
            case ::Replay::SummaryType_E::PERIMETER_INCIDENT_TYPE:
            {
                for(unsigned int i = 0; i < sizeof(PERIMETER_INCIDENT_EVENT_MAP)/sizeof(::Event::Type_E); i++)
                {
                    stEventCond.enType = PERIMETER_INCIDENT_EVENT_MAP[i];
                    EventSearch::instance()->searchByEventType(stEventCond, eventInfos);
                }
                break;
            }
            case ::Replay::SummaryType_E::BEHAVIOURAL_ANALYSIS_TYPE:
            {
                for(unsigned int i = 0; i < sizeof(BEHAVIOR_ANALYSIS_EVENT_MAP)/sizeof(::Event::Type_E); i++)
                {
                    stEventCond.enType = BEHAVIOR_ANALYSIS_EVENT_MAP[i];
                    EventSearch::instance()->searchByEventType(stEventCond, eventInfos);
                }
                break;
            }
            case ::Replay::SummaryType_E::SCENE_DETECTION_TYPE:
            {
                for(unsigned int i = 0; i < sizeof(SCENE_DETECTION_EVENT_MAP)/sizeof(::Event::Type_E); i++)
                {
                    stEventCond.enType = SCENE_DETECTION_EVENT_MAP[i];
                    EventSearch::instance()->searchByEventType(stEventCond, eventInfos);
                }
                break;
            }
            case ::Replay::SummaryType_E::TARGET_DETECTION_TYPE:
            {
                for(unsigned int i = 0; i < sizeof(TARGET_DETECTION_EVENT_MAP)/sizeof(::Event::Type_E); i++)
                {
                    stEventCond.enType = TARGET_DETECTION_EVENT_MAP[i];
                    EventSearch::instance()->searchByEventType(stEventCond, eventInfos);
                }
                break;
            }
            case ::Replay::SummaryType_E::FACE_CAPTURE_TYPE:
            {
                for(unsigned int i = 0; i < sizeof(FACE_CAPTURE_EVENT_MAP)/sizeof(::Event::Type_E); i++)
                {
                    stEventCond.enType = FACE_CAPTURE_EVENT_MAP[i];
                    EventSearch::instance()->searchByEventType(stEventCond, eventInfos);
                }
                break;
            }
            case ::Replay::SummaryType_E::BEHAVIOR_MONITORING_TYPE:
            {
                for(unsigned int i = 0; i < sizeof(BEHAVIOR_MONITORING_EVENT_MAP)/sizeof(::Event::Type_E); i++)
                {
                    stEventCond.enType = BEHAVIOR_MONITORING_EVENT_MAP[i];
                    EventSearch::instance()->searchByEventType(stEventCond, eventInfos);
                }
                break;
            }
            case ::Replay::SummaryType_E::CLOTHING_COMPLIANCE_TYPE:
            {
                for(unsigned int i = 0; i < sizeof(CLOTHING_COMPLIANCE_EVENT_MAP)/sizeof(::Event::Type_E); i++)
                {
                    stEventCond.enType = CLOTHING_COMPLIANCE_EVENT_MAP[i];
                    EventSearch::instance()->searchByEventType(stEventCond, eventInfos);
                }
                break;
            }
            case ::Replay::SummaryType_E::TRAFFIC_BEHAVIOR_MONITORING_TYPE:
            {
                for(unsigned int i = 0; i < sizeof(TRAFFIC_BEHAVIOR_MONITORING_EVENT_MAP)/sizeof(::Event::Type_E); i++)
                {
                    stEventCond.enType = TRAFFIC_BEHAVIOR_MONITORING_EVENT_MAP[i];
                    EventSearch::instance()->searchByEventType(stEventCond, eventInfos);
                }
                break;
            }
            case ::Replay::SummaryType_E::ATTRIBUTE_RECOGNITION_TYPE:
            {
                for(unsigned int i = 0; i < sizeof(ATTRIBUTE_RECOGNITION_EVENT_MAP)/sizeof(::Event::Type_E); i++)
                {
                    stEventCond.enType = ATTRIBUTE_RECOGNITION_EVENT_MAP[i];
                    EventSearch::instance()->searchByEventType(stEventCond, eventInfos);
                }
                break;
            }
            default:
                dlog_debug("未识别的事件类型");
                break;
        }
    }

    /* 删除无录制视频的事件 */
    eventInfos.erase(
        std::remove_if(eventInfos.begin(), eventInfos.end(), [](::Event::Info_S& info) { return info.strVideoPath.empty(); }), eventInfos.end());
    /* 按开始时间从小到大排序 */
    std::sort(eventInfos.begin(), eventInfos.end(), [](::Event::Info_S& info1, ::Event::Info_S& info2) { return info1.strStartTime < info2.strStartTime; });

    for (auto &eventInfo : eventInfos)
    {
        /* 时间转换 */
        auto timeToSeconds = [](const std::string& timeStr)
        {
            std::tm timeStruct = {};
            std::istringstream timeStream(timeStr);
            // 解析 "HH:MM:SS" 部分
            timeStream >> std::get_time(&timeStruct, "%Y-%m-%d %H:%M:%S");
            if (timeStream.fail())
            {
                return -1;
            }
            // 转换为秒数
            int totalSeconds = timeStruct.tm_hour * 3600 + timeStruct.tm_min * 60 + timeStruct.tm_sec;
            return totalSeconds;
        };
        ::Record_NS::VideoTime_S stVideoTime;
        stVideoTime.nStartTime = timeToSeconds(eventInfo.strStartTime);                /* 录像开始时间 */
        stVideoTime.nEndTime = timeToSeconds(eventInfo.strEndTime);                    /* 录像开始时间 */

        /* 无效时间直接丢掉 */
        if (stVideoTime.nStartTime == -1 || stVideoTime.nEndTime == -1)
        {
            continue;
        }

        auto &events = stLayoutInfo.recordTime.EventTimes;

        /* 第一段，直接加入 */
        if (events.empty())
        {
            events.push_back(stVideoTime);
        }
        else
        {
            auto &last = events.back();

            if (stVideoTime.nStartTime <= last.nEndTime)
            {
                last.nEndTime = std::max(last.nEndTime, stVideoTime.nEndTime);
            }
            else
            {
                events.push_back(stVideoTime);
            }
        }
    }

    // /* 填充布局信息 */
    // CReplayManage::instance()->fill_layoutInfo(stLayoutInfo);
    /* 设置数据 */
    std::string str;
    if (m_nActionCode == 3114 /* AC_GET_VIDEO_TIME */)
    {
        /* 只返回时间 */
        ::Replay::RecordTime_S stRecordTime;
        stRecordTime.videoTimes = stLayoutInfo.recordTime.videoTimes;
        stRecordTime.EventTimes = stLayoutInfo.recordTime.EventTimes;
        stRecordTime.nEventType = (int)stLayoutInfo.enSummaryType;
        str = Convert::to_string(stRecordTime);
        result(str);
        return;
    }
    else
    {
        str = Convert::to_string(stLayoutInfo);
    }

    result(str);
}
