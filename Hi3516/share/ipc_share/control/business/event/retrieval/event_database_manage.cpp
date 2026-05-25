/**
 * @file event_database_manage.cpp
 * @author xiejh (xiejh@kfb.cn)
 * @date 2024-10-15
 * 
 * @brief 检索数据库管理：提供增删改相关接口
 */

#include "event_database_manage.h"
#include "convert_interface.h"
#include "event_linkage_dict.h"
#include <iomanip>
#include <memory>
#include "log_handler.h"
#include "path_define.h"

using namespace Event;
using namespace Db;

int EventDatabaseManage::init()
{
    return 0;
}

void EventDatabaseManage::deinit()
{
    return;
}


int EventDatabaseManage::add(Info_S stEventInfo)
{
    // Log::Info_S stLogInfo;
    /* 记录事件开始 */
    auto it = m_motionDetectMap.find(stEventInfo.nChnId);
    if (it != m_motionDetectMap.end())
    {
        auto &stDbdEventInfo = it->second;
        auto to_time_t = [](const std::string& str) -> std::time_t {
            std::tm tm{};
            std::stringstream ss(str);
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            if (ss.fail()) 
            {
                dlog_error("时间转换失败：%s", str.c_str());
                
                return static_cast<std::time_t>(-1);
            }
            return std::mktime(&tm);
        };
        /* 如果时间差小于10s */
        if ((to_time_t(stEventInfo.strStartTime) - to_time_t(stDbdEventInfo.strEndTime)) < 10)
        {
            /* 更新事件信息 */
            stDbdEventInfo.strEndTime = stEventInfo.strEndTime;
            stDbdEventInfo.lTimestamp = stEventInfo.lTimestamp;
            update(stDbdEventInfo);
            return stDbdEventInfo.nId;
        }
    }
    // stLogInfo.startTime = stEventInfo.strStartTime;
    // stLogInfo.nType = Log::Type::ALARM;
    // stLogInfo.nAction = Log::to_action(stEventInfo.enType, true /* isStart */);
    // stLogInfo.chnName = std::string("D") + std::to_string(stEventInfo.nChnId + 1);
    // stLogInfo.context = "人员";
    // LogHandler::instance()->write(stLogInfo);  

    stEventInfo.nId = EventDatabase::instance()->add(stEventInfo);
    if (stEventInfo.enType == Event::Type::MOTION_DETECT)
    {
        m_motionDetectMap[stEventInfo.nChnId] = stEventInfo;
    }
    return stEventInfo.nId;
}

int EventDatabaseManage::add(Event::FaceCompareInfo_S stFaceCompareInfo)
{
    int nRet = EventDatabase::instance()->add(stFaceCompareInfo);
    // 入库成功后构造 EventTriggerContext_S，并把人脸比对结果放进 EventTvSdkPayload_S
    if (nRet >= 0)
    {
        EventTriggerContext_S stContext;
        stContext.enEventType = Event::Type_E::FACE_COMPARE;
        stContext.bEventEnded = false;
        stContext.nChnId = stFaceCompareInfo.stInfo.nChnId;
        stContext.llTimestamp = stFaceCompareInfo.stInfo.lTimestamp;

        EventTvSdkPayload_S stPayload;
        stPayload.enType = EventTvSdkPayloadType_E::FACE_COMPARE;
        stPayload.stFaceCompare.stFaceCompareInfo = stFaceCompareInfo;
        stContext.pTvSdkPayload = std::make_shared<EventTvSdkPayload_S>(stPayload);

        // 这里没有走 CEventLinkage::handleEvent()，是为了避开事件时间窗去重，避免连续的人脸比对结果被吞掉
        EventLinkageDict::push_tvsdk_event_alarm(stContext);
    }

    return nRet;
}


// int EventDatabaseManage::add(VehicleInfo_S stVehicleInfo)
// {
//     return VehicleDatabase::instance()->add(stVehicleInfo);
// }




int EventDatabaseManage::del(Info_S stEventInfo)
{
    Item item;
    item.push_back(Element(DB_COMMON_FIELD_ID, stEventInfo.nId));
    return EventDatabase::instance()->del(item);
}


// int EventDatabaseManage::del(VehicleInfo_S stVehicleInfo)
// {
//     Item item;
//     item.push_back(Element(DB_COMMON_FIELD_ID, stVehicleInfo.stInfo.nId));
//     return VehicleDatabase::instance()->del(item);
// }


int EventDatabaseManage::update(Info_S stEventInfo)
{
    Item item;
    item.push_back(Element(INFO_CHANNEL_ID, stEventInfo.nChnId));
    item.push_back(Element(INFO_EVENT_TYPE, (int)stEventInfo.enType));
    item.push_back(Element(INFO_EVENT_DATE, stEventInfo.strDate));
    item.push_back(Element(INFO_EVENT_TIME, stEventInfo.strTime));
    item.push_back(Element(INFO_RECORD_STATRTIME, stEventInfo.strStartTime));
    item.push_back(Element(INFO_RECORD_ENDTIME, stEventInfo.strEndTime));
    item.push_back(Element(INFO_TIMESTAMP, std::to_string(stEventInfo.lTimestamp)));
    item.push_back(Element(INFO_RECORD_LABEL, stEventInfo.strLabel));
    
    if (!stEventInfo.strVideoPath.empty())
    {
        item.push_back(Element(INFO_VIDEO_PATH, stEventInfo.strVideoPath));
        item.push_back(Element(INFO_VIDEO_SIZE, stEventInfo.nVideoSize));
        item.push_back(Element(INFO_VIDEO_BIND_ID, stEventInfo.nVideoBindId));
    }

    MatchMethods methods;
    methods.push_back(MatchMethod(Element(INFO_CHANNEL_ID, stEventInfo.nChnId), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    methods.push_back(MatchMethod(Element(INFO_EVENT_TYPE, (int)stEventInfo.enType), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    methods.push_back(MatchMethod(Element(INFO_EVENT_DATE, stEventInfo.strDate), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    methods.push_back(MatchMethod(Element(INFO_EVENT_TIME, stEventInfo.strTime), FIND_CRITERION_EQ));
    return EventDatabase::instance()->update(item, methods);
}


// int EventDatabaseManage::update(VehicleInfo_S stVehicleInfo)
// {
//     Item item;
//     item.push_back(Element(INFO_CHANNEL_ID, stVehicleInfo.stInfo.nChnId));
//     item.push_back(Element(INFO_EVENT_TYPE, (int)stVehicleInfo.stInfo.enType));
//     item.push_back(Element(INFO_EVENT_DATE, stVehicleInfo.stInfo.strDate));
//     item.push_back(Element(INFO_EVENT_TIME, stVehicleInfo.stInfo.strTime));
//     item.push_back(Element(INFO_RECORD_STATRTIME, stVehicleInfo.stInfo.strStartTime));
//     item.push_back(Element(INFO_RECORD_ENDTIME, stVehicleInfo.stInfo.strEndTime));
//     item.push_back(Element(INFO_TIMESTAMP, std::to_string(stVehicleInfo.stInfo.lTimestamp)));
//     item.push_back(Element(INFO_RECORD_LABEL, stVehicleInfo.stInfo.strLabel));
//     item.push_back(Element(INFO_IMAGE_PATH, stVehicleInfo.stInfo.strImagePath));
//     item.push_back(Element(INFO_VIDEO_PATH, stVehicleInfo.stInfo.strVideoPath));
//     item.push_back(Element(VEHICLE_FIELD_PLATEREGION, (int)stVehicleInfo.enPlateRegion));
//     item.push_back(Element(VEHICLE_FIELD_PLATESERIAL, stVehicleInfo.strPlateSerial));
    
//     MatchMethods methods;
//     methods.push_back(MatchMethod(Element(DB_COMMON_FIELD_ID, stVehicleInfo.stInfo.nId), FIND_CRITERION_EQ));
//     return VehicleDatabase::instance()->update(item, methods);
// }

int EventDatabaseManage::get_itemInfo(Event::Info_S &stInfo)
{
    MatchMethods methods;
    /* id查询 */
    if (stInfo.nChnId != -1)
    {
        methods.push_back(MatchMethod(Element(INFO_CHANNEL_ID, stInfo.nChnId), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    }
    if (stInfo.enType != Event::Type::UNKNOWN)
    {
        methods.push_back(MatchMethod(Element(INFO_EVENT_TYPE, (int)stInfo.enType), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    }
    if (!stInfo.strDate.empty())
    {
        methods.push_back(MatchMethod(Element(INFO_EVENT_DATE, stInfo.strDate), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    }
    if (!stInfo.strTime.empty())
    {
        methods.push_back(MatchMethod(Element(INFO_EVENT_TIME, stInfo.strTime), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    }
    if (methods.size() > 0)
    {
        MatchMethod &lastMethod = methods.back();
        lastMethod.enAndOr = FIND_CRITERION_NONE;
    }
    std::vector<Event::Info_S> infos;
    EventDatabase::instance()->find(methods, infos);
    if (infos.size() != 0)
    {
        stInfo = infos[0];
        return 0;
    }
    return -1;
}
int EventDatabaseManage::find(Event::RetrievalCond_S stCond, std::vector<Event::FaceCompareInfo_S> &faceCompareInfos, bool bAsc)
{
    Common::PageInfo_S stPageInfo;
    return find(stCond, faceCompareInfos, stPageInfo, bAsc);
}

int EventDatabaseManage::find(Event::RetrievalCond_S stCond, std::vector<Event::FaceCompareInfo_S> &faceCompareInfos, Common::PageInfo_S &stPageInfo, bool bAsc)
{
    MatchMethods methods;
    /* 多id查询 */
    if (stCond.nChnIds.size() > 0)
    {
        for (auto it = stCond.nChnIds.begin(); it != stCond.nChnIds.end(); ++it)
        {
            methods.push_back(MatchMethod(Element(INFO_EVENT_ID, std::to_string(*it)), FIND_CRITERION_EQ, FIND_CRITERION_OR_P));
        }
        methods.back().enAndOr = FIND_CRITERION_AND;
    }
    if (!stCond.strCapFacePath.empty())
    {
        methods.push_back(MatchMethod(Element(INFO_CAPTURE_FACE_PATH, stCond.strCapFacePath), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    }
    /* 只有-2情况下才检索0未比对数据 */
    if (stCond.nCompResult == -1)
    {
        /* 默认不搜索未必比对数据 */
        methods.push_back(MatchMethod(Element(INFO_COMP_RESULT, 0), FIND_CRITERION_NE, FIND_CRITERION_AND));
    }
    /* 比对结果：0未比对 1成功 2失败 */
    else  if (stCond.nCompResult != -1 && stCond.nCompResult != -2)
    {
        methods.push_back(MatchMethod(Element(INFO_COMP_RESULT, stCond.nCompResult), FIND_CRITERION_EQ, FIND_CRITERION_AND));
        /* 比对结果：成功 */
        if (stCond.nCompResult == 1)
        {
            methods.push_back(MatchMethod(Element(INFO_LIB_FACE_PATH, std::string()), FIND_CRITERION_NE, FIND_CRITERION_AND));
        }
    }
    
    /* 默认升序 */
    if (!bAsc)
    {
        /* 根据id降序 */
        std::string key = "order by " + std::string(DB_COMMON_FIELD_ID);
        methods.push_back(MatchMethod(Element(key, "desc"), FIND_CRITERION_NONE, FIND_CRITERION_NONE));
    }
    /* 带页数 */
    if (stPageInfo.nCurPage != -1)
    {
        if (methods.size() > 0)
        {
            MatchMethod &lastMethod = methods.back();
            lastMethod.enAndOr = FIND_CRITERION_NONE;
        }

        /* 总个数, 要放在前面 */
        int nCount = -1;
        EventDatabase::instance()->get_faceCompareCount(methods, nCount, DB_COMMON_FIELD_ID);
        stPageInfo.nDataTotal = nCount;
        /* 每页数据个数,默认20 */
        stPageInfo.nPageSize = stPageInfo.nPageSize == 0 ? 20 : stPageInfo.nPageSize;
        stPageInfo.nPageTotal = (stPageInfo.nDataTotal + stPageInfo.nPageSize - 1) / stPageInfo.nPageSize;
        /* 一页数据个数 */
        std::string key = "limit";
        methods.push_back(MatchMethod(Element(key, stPageInfo.nPageSize), FIND_CRITERION_NONE, FIND_CRITERION_NONE));
        /* 第几页 */
        key = "OFFSET" ;
        methods.push_back(MatchMethod(Element(key, std::to_string(stPageInfo.nPageSize * (stPageInfo.nCurPage - 1))), FIND_CRITERION_NONE, FIND_CRITERION_NONE));

    }
    else
    {
        if (methods.size() > 0)
        {
            MatchMethod &lastMethod = methods.back();
            lastMethod.enAndOr = FIND_CRITERION_NONE;
        }
        stPageInfo.nCurPage = 1;
        stPageInfo.nPageTotal = 1;
        int nCount = -1;
        EventDatabase::instance()->get_faceCompareCount(methods, nCount, DB_COMMON_FIELD_ID);
        stPageInfo.nDataTotal = nCount;
    }
    if (methods.size() > 0)
    {
        MatchMethod &lastMethod = methods.back();
        lastMethod.enAndOr = FIND_CRITERION_NONE;
    }
    return EventDatabase::instance()->find(methods, faceCompareInfos);
}

int EventDatabaseManage::set_rule(Event::RuleInfo_S stRuleInfo)
{
    /* 读取记录的规则 */  
    std::vector<::Event::RuleInfo_S> dbRuleInfos;
    Convert::read_file(EVENT_RULE_INFOS_CONFIG_FILE, dbRuleInfos);
    
    /* 查找 dbRuleInfos 中是否有相同 id 的元素 */
    auto it = std::find_if(dbRuleInfos.begin(), dbRuleInfos.end(),
                            [&stRuleInfo](const ::Event::RuleInfo_S& oldInfo) {
                                /* 匹配通道id、类型 */
                                return oldInfo.nChnId == stRuleInfo.nChnId && oldInfo.enType == stRuleInfo.enType; 
                            });

    if (it != dbRuleInfos.end()) {
        /* 如果找到了相同 id 的元素，替换它 */
        *it = stRuleInfo;
    } else {
        /* 如果没有找到相同 id 的元素，添加新元素 */
        dbRuleInfos.push_back(stRuleInfo);
    }
    Convert::write_file(EVENT_RULE_INFOS_CONFIG_FILE, dbRuleInfos);
    return 0;
}

std::vector<Event::RuleInfo_S> EventDatabaseManage::get_rule()
{
    /* 读取记录的规则 */  
    std::vector<::Event::RuleInfo_S> dbRuleInfos;
    Convert::read_file(EVENT_RULE_INFOS_CONFIG_FILE, dbRuleInfos);
    dbRuleInfos.erase(std::remove_if(dbRuleInfos.begin(), dbRuleInfos.end(),
        [](const ::Event::RuleInfo_S& ruleInfo) {
            return ruleInfo.bEnable == false; // 如果使能字段为 0，则返回 true
        }),
        dbRuleInfos.end()
    );
    return dbRuleInfos;
}

int EventDatabaseManage::del(Event::RetrievalCond_S &stCond, std::string strTargetTableName)
{
    MatchMethods methods;

    /* 多通道查询 */
    if (stCond.nChnIds.size() > 0)
    {
        for (auto it = stCond.nChnIds.begin(); it != stCond.nChnIds.end(); ++it)
        {
            methods.push_back(MatchMethod(Element(INFO_CHANNEL_ID, std::to_string(*it)), FIND_CRITERION_EQ, FIND_CRITERION_OR_P));
        }
        methods.back().enAndOr = FIND_CRITERION_AND;
    }

    if (!stCond.strEndTime.empty()) 
    {
        methods.push_back(MatchMethod(Element(INFO_RECORD_ENDTIME, stCond.strEndTime), FIND_CRITERION_GE, FIND_CRITERION_AND));
    }

    if (methods.size() != 0)
    {
        MatchMethod &lastMethod = methods.back();
        lastMethod.enAndOr = FIND_CRITERION_NONE;
    }
 
    return EventDatabase::instance()->del(methods, strTargetTableName);
}