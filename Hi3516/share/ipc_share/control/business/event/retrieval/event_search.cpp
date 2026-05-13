/**
 * @file EventSearch.cpp
 * @author xiejh (xiejh@kfb.cn)
 * @date 2024-10-15
 * 
 * @brief 检索数据库管理：提供检索相关接口
 */

#include "event_search.h"

using namespace Event;
using namespace Db;


int EventSearch::init()
{
    return 0;
}


void EventSearch::deinit()
{
    return;
}


/* 通过通道检索 */
int EventSearch::searchByChnId(const std::vector<int>& ChnIds, std::vector<Event::Info_S> &EventInfos)
{
    if (ChnIds.empty())
    {
        dlog_error("通道ID为空");
        return -1;
    }

    MatchMethods methods;
    for (size_t i = 0; i < ChnIds.size(); i++)
    {
        methods.push_back(MatchMethod(Element(INFO_CHANNEL_ID, ChnIds[i]), FIND_CRITERION_EQ));
        MatchMethod& method = methods.back();

        if (i < ChnIds.size() - 1)
        {
            method.enAndOr = FIND_CRITERION_OR;
        }
    }
    
    return EventDatabase::instance()->find(methods, EventInfos);
}

int EventSearch::searchByEventType(Event::RetrievalCond_S &stCond, std::vector<Event::Info_S> &EventInfos)
{
    Common::PageInfo_S stPageInfo;
    return searchByEventType(stCond, EventInfos, stPageInfo);
}

/* 通过事件类型检索 */
int EventSearch::searchByEventType(Event::RetrievalCond_S &stCond, std::vector<Event::Info_S> &EventInfos, Common::PageInfo_S &stPageInfo, bool bAsc, bool bImage)
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

    if (stCond.videoBingIds.size() > 0)
    {
        for (auto it = stCond.videoBingIds.begin(); it != stCond.videoBingIds.end(); ++it)
        {
            methods.push_back(MatchMethod(Element(INFO_VIDEO_BIND_ID, std::to_string(*it)), FIND_CRITERION_EQ, FIND_CRITERION_OR_P));
        }
        methods.back().enAndOr = FIND_CRITERION_AND;
    }

    /* 根据id查找 */
    if (stCond.nId != -1)
    {
        methods.push_back(MatchMethod(Element(DB_COMMON_FIELD_ID, stCond.nId), FIND_CRITERION_EQ, FIND_CRITERION_AND)); 
    }

    /* 过滤无效事件Id */
    methods.push_back(MatchMethod(Element(DB_COMMON_FIELD_ID, -1), FIND_CRITERION_NE, FIND_CRITERION_AND)); 

    if(!stCond.strStartTime.empty())
    {
        methods.push_back(MatchMethod(Element(INFO_RECORD_STATRTIME, stCond.strStartTime), FIND_CRITERION_GE, FIND_CRITERION_AND));

    }

    if(!stCond.strEndTime.empty())
    {
        methods.push_back(MatchMethod(Element(INFO_RECORD_STATRTIME, stCond.strEndTime), FIND_CRITERION_IE, FIND_CRITERION_AND));
    }


    /* 开始日期 */
    if (!stCond.strStartDate.empty())
    {
        std::string key= "strftime('%Y-%m-%d'," + std::string(INFO_RECORD_STATRTIME) + ")";
        methods.push_back(MatchMethod(Element(key, stCond.strStartDate), FIND_CRITERION_GE, FIND_CRITERION_AND)); 
    }

    /* 结束日期 */
    if (!stCond.strEndDate.empty())
    {
        std::string key= "strftime('%Y-%m-%d'," + std::string(INFO_RECORD_ENDTIME) + ")";
        methods.push_back(MatchMethod(Element(key, stCond.strEndDate), FIND_CRITERION_IE, FIND_CRITERION_AND)); 
    }
    /* 事件类型匹配 */
    if (stCond.enType != Event::Type::UNKNOWN)
    {
        methods.push_back(MatchMethod(Element(INFO_EVENT_TYPE, (int)stCond.enType), FIND_CRITERION_EQ, FIND_CRITERION_AND));
        if (stCond.enType == Event::Type::LABEL)
        {

            if (!stCond.strLabel.empty())
            {
                methods.push_back(MatchMethod(Element(INFO_RECORD_LABEL, stCond.strLabel), FIND_CRITERION_EQ, FIND_CRITERION_AND));
            }
            else
            {
                methods.push_back(MatchMethod(Element(INFO_RECORD_LABEL, std::string()), FIND_CRITERION_NE, FIND_CRITERION_AND));
            }
        }
    }

    /* 带页数 */
    if (stPageInfo.nCurPage != -1)
    {
        MatchMethod &lastMethod = methods.back();
        lastMethod.enAndOr = FIND_CRITERION_NONE;

        /* 总个数, 要放在前面 */
        int nCount = -1;
        EventDatabase::instance()->get_count(methods, nCount, DB_COMMON_FIELD_ID);
        stPageInfo.nDataTotal = nCount;
        /* 每页数据个数,默认20 */
        stPageInfo.nPageSize = stPageInfo.nPageSize == 0 ? 20 : stPageInfo.nPageSize;
        stPageInfo.nPageTotal = (stPageInfo.nDataTotal + stPageInfo.nPageSize - 1) / stPageInfo.nPageSize;
        /* 根据id升序 */
        std::string key = "order by " + std::string(DB_COMMON_FIELD_ID);
        std::string order = bAsc ? "asc" : "desc";
        methods.push_back(MatchMethod(Element(key, order), FIND_CRITERION_NONE, FIND_CRITERION_NONE));
        /* 一页数据个数 */
        key = "limit";
        methods.push_back(MatchMethod(Element(key, stPageInfo.nPageSize), FIND_CRITERION_NONE, FIND_CRITERION_NONE));
        /* 第几页 */
        key = "OFFSET" ;
        methods.push_back(MatchMethod(Element(key, std::to_string(stPageInfo.nPageSize * (stPageInfo.nCurPage - 1))), FIND_CRITERION_NONE, FIND_CRITERION_NONE));

    }
    else
    {
        if (methods.size() != 0)
        {
            MatchMethod &lastMethod = methods.back();
            lastMethod.enAndOr = FIND_CRITERION_NONE;
        }
        stPageInfo.nCurPage = 1;
        stPageInfo.nPageTotal = 1;
        int nCount = -1;
        EventDatabase::instance()->get_count(methods, nCount, DB_COMMON_FIELD_ID);
        stPageInfo.nDataTotal = nCount;
    }
    if (methods.size() != 0)
    {
        MatchMethod &lastMethod = methods.back();
        lastMethod.enAndOr = FIND_CRITERION_NONE;
    }
    return EventDatabase::instance()->find(methods, EventInfos);
}


/* 通过日期检索 */
int EventSearch::searchByDate(std::string startdate, std::string enddate, std::vector<Event::Info_S> &EventInfos)
{
    MatchMethods methods;
    methods.push_back(MatchMethod(Element(INFO_EVENT_DATE, startdate), FIND_CRITERION_GE));
    MatchMethod& method = methods.front();
    method.enAndOr = FIND_CRITERION_AND;
    methods.push_back(MatchMethod(Element(INFO_EVENT_DATE, enddate), FIND_CRITERION_IE));

    return EventDatabase::instance()->find(methods, EventInfos);
}


/* 通过时间检索 */
int EventSearch::searchByTime(std::string starttime, std::string endtime, std::vector<Event::Info_S> &EventInfos)
{
    MatchMethods methods;
    methods.push_back(MatchMethod(Element(INFO_RECORD_STATRTIME, starttime), FIND_CRITERION_GE));
    MatchMethod& method = methods.front();
    method.enAndOr = FIND_CRITERION_AND;
    methods.push_back(MatchMethod(Element(INFO_RECORD_ENDTIME, endtime), FIND_CRITERION_IE));

    return EventDatabase::instance()->find(methods, EventInfos);
}


/* 通过图片检索 */
int EventSearch::searchByImageType(Event::RetrievalCond_S &stCond, std::vector<Capture_NS::CaptureInfo_S> &CaptureInfos, Common::PageInfo_S &stPageInfo)
{
    MatchMethods methods;

    /* 多通道查询 */
    if (stCond.nChnIds.size() > 0)
    {
        for (auto it = stCond.nChnIds.begin(); it != stCond.nChnIds.end(); ++it)
        {
            methods.push_back(MatchMethod(Element(INFO_CHNL_ID, std::to_string(*it)), FIND_CRITERION_EQ, FIND_CRITERION_OR_P));
        }
        methods.back().enAndOr = FIND_CRITERION_AND;
    }

    if (!stCond.strStartDate.empty())
    {
        methods.push_back(MatchMethod(Element(INFO_RECORD_STATRTIME, stCond.strStartDate), FIND_CRITERION_GE, FIND_CRITERION_AND));
    }

    if (!stCond.strEndDate.empty()) 
    {
        methods.push_back(MatchMethod(Element(INFO_RECORD_STATRTIME, stCond.strEndDate), FIND_CRITERION_IE, FIND_CRITERION_AND));
    }
    if (stCond.strImagePath.empty())
    {
        if (stCond.nPicType >= 0)
        {
            methods.push_back(MatchMethod(Element(INFO_EVENT_TYPE, stCond.nPicType), FIND_CRITERION_EQ, FIND_CRITERION_AND)); 
        }

        methods.push_back(MatchMethod(Element(INFO_CAPTURE_EVENT_TYPE, ""), FIND_CRITERION_NE, FIND_CRITERION_AND));
    }
    else
    {
        methods.push_back(MatchMethod(Element(INFO_CAPTURE_EVENT_TYPE, stCond.strImagePath), FIND_CRITERION_EQ));
    }

    /* 带页数 */
    if (stPageInfo.nCurPage != -1)
    {
        if (methods.size() != 0)
        {
            MatchMethod &lastMethod = methods.back();
            lastMethod.enAndOr = FIND_CRITERION_NONE;
        }

        /* 总个数, 要放在前面 */
        int nCount = -1;
        CCaptureDatabase::instance()->get_count(methods, nCount, DB_COMMON_FIELD_ID);
        stPageInfo.nDataTotal = nCount;
        /* 每页数据个数,默认20 */
        stPageInfo.nPageSize = stPageInfo.nPageSize == 0 ? 20 : stPageInfo.nPageSize;
        stPageInfo.nPageTotal = (stPageInfo.nDataTotal + stPageInfo.nPageSize - 1) / stPageInfo.nPageSize;
        /* 根据id升序 */
        std::string key = "order by " + std::string(DB_COMMON_FIELD_ID);
        std::string order = "asc";
        methods.push_back(MatchMethod(Element(key, order), FIND_CRITERION_NONE, FIND_CRITERION_NONE));
        /* 一页数据个数 */
        key = "limit";
        methods.push_back(MatchMethod(Element(key, stPageInfo.nPageSize), FIND_CRITERION_NONE, FIND_CRITERION_NONE));
        /* 第几页 */
        key = "OFFSET" ;
        methods.push_back(MatchMethod(Element(key, std::to_string(stPageInfo.nPageSize * (stPageInfo.nCurPage - 1))), FIND_CRITERION_NONE, FIND_CRITERION_NONE));

    }
    else
    {
        MatchMethod &lastMethod = methods.back();
        lastMethod.enAndOr = FIND_CRITERION_NONE;
        stPageInfo.nCurPage = 1;
        stPageInfo.nPageTotal = 1;
        int nCount = -1;
        CCaptureDatabase::instance()->get_count(methods, nCount, DB_COMMON_FIELD_ID);
        stPageInfo.nDataTotal = nCount;
    }
    if (methods.size() != 0)
    {
        MatchMethod &lastMethod = methods.back();
        lastMethod.enAndOr = FIND_CRITERION_NONE;
    }
    return CCaptureDatabase::instance()->find(methods, CaptureInfos);
}

/* 通过车辆信息检索 */
int EventSearch::searchByVehicle(Event::RetrievalCond_S &stCond, std::vector<Event::VehicleInfo_S> &VehicleInfos)
{
    // todo 待实现
    return 0;
#if 0
    MatchMethods methods;
    
    for (size_t i = 0; i < stCond.nChnIds.size(); i++)
    {
        methods.push_back(MatchMethod(Element(INFO_CHANNEL_ID, stCond.nChnIds[i]), FIND_CRITERION_EQ));

        methods.back().enAndOr = FIND_CRITERION_AND;
        methods.push_back(MatchMethod(Element(INFO_EVENT_DATE, stCond.strStartDate), FIND_CRITERION_GE));

        methods.back().enAndOr = FIND_CRITERION_AND;
        methods.push_back(MatchMethod(Element(INFO_EVENT_DATE, stCond.strEndDate), FIND_CRITERION_IE));

        /* 车辆信息匹配 */
        methods.back().enAndOr = FIND_CRITERION_AND;
        methods.push_back(MatchMethod(Element(VEHICLE_FIELD_PLATEREGION, (int)stCond.enPlateRegion), FIND_CRITERION_EQ));

        methods.back().enAndOr = FIND_CRITERION_AND;
        methods.push_back(MatchMethod(Element(VEHICLE_FIELD_PLATESERIAL, stCond.strPlateSerial), FIND_CRITERION_EQ));

        MatchMethod& method = methods.back();
        if (i < stCond.nChnIds.size() - 1)
        {
            method.enAndOr = FIND_CRITERION_OR;
        }
    }

    return VehicleDatabase::instance()->find(methods, VehicleInfos);
#endif
}

/* 通过视频类型检索 */
int EventSearch::searchByRecordType(Event::RetrievalCond_S &stCond, std::vector<Event::Info_S> &EventInfos)
{
    MatchMethods methods;
    
    for (size_t i = 0; i < stCond.nChnIds.size(); i++)
    {
        methods.push_back(MatchMethod(Element(INFO_CHANNEL_ID, stCond.nChnIds[i]), FIND_CRITERION_EQ));
        
        methods.back().enAndOr = FIND_CRITERION_AND;
        methods.push_back(MatchMethod(Element(INFO_EVENT_DATE, stCond.strStartDate), FIND_CRITERION_GE));

        methods.back().enAndOr = FIND_CRITERION_AND;
        methods.push_back(MatchMethod(Element(INFO_EVENT_DATE, stCond.strEndDate), FIND_CRITERION_IE));

        methods.back().enAndOr = FIND_CRITERION_AND;
        methods.push_back(MatchMethod(Element(INFO_VIDEO_PATH, ""), FIND_CRITERION_NE));

        /* 标签检索 */
        if (!stCond.strLabel.empty())
        {
            methods.back().enAndOr = FIND_CRITERION_AND;
            methods.push_back(MatchMethod(Element(INFO_RECORD_LABEL, stCond.strLabel), FIND_CRITERION_EQ));
        }

        MatchMethod& method = methods.back();
        if (i < stCond.nChnIds.size() - 1)
        {
            method.enAndOr = FIND_CRITERION_OR;
        }
    }

    return EventDatabase::instance()->find(methods, EventInfos);
}
