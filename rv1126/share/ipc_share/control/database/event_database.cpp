/*
 * @Author: xiejh xiejh@kfb.cn
 * @Date: 2024-10-14
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2025-12-31 17:24:54
 * @FilePath: /hisi/hisi/share/ipc_share/control/database/event_database.cpp
 * @Description: 事件数据库
 */

#include "event_database.h"

using namespace Event;
using namespace Db;

EventDatabase::EventDatabase()
    : m_eventDb(EVENT_DATABASE_PATH, EVENT_TABLE_NAME)
    , m_faceCompareDb(EVENT_DATABASE_PATH, FACE_COMPARE_TABLE_NAME)
{
    create(EVENT_TABLE_NAME);
    create(FACE_COMPARE_TABLE_NAME);
}

EventDatabase::~EventDatabase()
{
}

int EventDatabase::create(std::string tableName, bool bAddTableKey)
{
    if (tableName == EVENT_TABLE_NAME && bAddTableKey)
    {
        m_eventDb.add_tableKey(TableKey(INFO_CHANNEL_ID, CDbBase::type_int()));
        m_eventDb.add_tableKey(TableKey(INFO_EVENT_TYPE, CDbBase::type_int()));
        m_eventDb.add_tableKey(TableKey(INFO_EVENT_DATE, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_eventDb.add_tableKey(TableKey(INFO_EVENT_TIME, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_eventDb.add_tableKey(TableKey(INFO_RECORD_STATRTIME, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_eventDb.add_tableKey(TableKey(INFO_RECORD_ENDTIME, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_eventDb.add_tableKey(TableKey(INFO_TIMESTAMP, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_eventDb.add_tableKey(TableKey(INFO_RECORD_LABEL, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_eventDb.add_tableKey(TableKey(INFO_VIDEO_PATH, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_eventDb.add_tableKey(TableKey(INFO_VIDEO_SIZE, CDbBase::type_int()));
        m_eventDb.add_tableKey(TableKey(INFO_VIDEO_BIND_ID, CDbBase::type_int()));

        m_eventDb.init();
    }
    else if (tableName == FACE_COMPARE_TABLE_NAME && bAddTableKey)
    {
        m_faceCompareDb.add_tableKey(TableKey(INFO_EVENT_ID, CDbBase::type_int()));
        m_faceCompareDb.add_tableKey(TableKey(INFO_COMP_RESULT, CDbBase::type_int()));
        m_faceCompareDb.add_tableKey(TableKey(INFO_SIMILARITY, CDbBase::type_int()));
        m_faceCompareDb.add_tableKey(TableKey(INFO_FACE_ID, CDbBase::type_int()));
        m_faceCompareDb.add_tableKey(TableKey(INFO_LIB_NAME, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_faceCompareDb.add_tableKey(TableKey(INFO_FACE_NAME, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_faceCompareDb.add_tableKey(TableKey(INFO_LIB_FACE_PATH, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_faceCompareDb.add_tableKey(TableKey(INFO_CAPTURE_FACE_PATH, CDbBase::type_string(MAX_DB_STRING_SIZE)));

        m_faceCompareDb.init();
    }
    else if (tableName == EVENT_TABLE_NAME && !bAddTableKey)
    {
        m_eventDb.init(false);
    }
    else if (tableName == FACE_COMPARE_TABLE_NAME && !bAddTableKey)
    {
        m_faceCompareDb.init(false);
    }

    return 0;
}

int EventDatabase::init()
{
    create(EVENT_TABLE_NAME, false);
    create(FACE_COMPARE_TABLE_NAME, false);
    return 0;
}

int EventDatabase::deinit()
{
    m_eventDb.deinit();
    m_faceCompareDb.deinit();
    return 0;
}  

int EventDatabase::add(const Event::Info_S &stEventInfo)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    Item item;
    item.push_back(Element(INFO_CHANNEL_ID, stEventInfo.nChnId));
    item.push_back(Element(INFO_EVENT_TYPE, (int)stEventInfo.enType));
    item.push_back(Element(INFO_EVENT_DATE, stEventInfo.strDate));
    item.push_back(Element(INFO_EVENT_TIME, stEventInfo.strTime));
    item.push_back(Element(INFO_RECORD_STATRTIME, stEventInfo.strStartTime));
    item.push_back(Element(INFO_RECORD_ENDTIME, stEventInfo.strEndTime));
    item.push_back(Element(INFO_TIMESTAMP, std::to_string(stEventInfo.lTimestamp)));
    item.push_back(Element(INFO_RECORD_LABEL, stEventInfo.strLabel));
    item.push_back(Element(INFO_VIDEO_PATH, stEventInfo.strVideoPath));
    item.push_back(Element(INFO_VIDEO_SIZE, stEventInfo.nVideoSize));
    item.push_back(Element(INFO_VIDEO_BIND_ID, stEventInfo.nVideoBindId));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE1, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE2, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE3, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE4, std::string()));
    return m_eventDb.add(item);
}

int Db::EventDatabase::add(const Event::FaceCompareInfo_S &stFaceCompareInfo)
{
    Item item;
    item.push_back(Element(INFO_EVENT_ID, stFaceCompareInfo.nEventId));
    item.push_back(Element(INFO_COMP_RESULT, stFaceCompareInfo.nCompResult));
    item.push_back(Element(INFO_SIMILARITY, stFaceCompareInfo.nSimilarity));
    item.push_back(Element(INFO_FACE_ID, stFaceCompareInfo.nFaceId));
    item.push_back(Element(INFO_LIB_NAME, stFaceCompareInfo.strFaceLibName));
    item.push_back(Element(INFO_FACE_NAME, stFaceCompareInfo.strFaceName));
    item.push_back(Element(INFO_LIB_FACE_PATH, stFaceCompareInfo.strLibFacePath));
    item.push_back(Element(INFO_CAPTURE_FACE_PATH, stFaceCompareInfo.strCapFacePath));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE1, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE2, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE3, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE4, std::string()));
    return m_faceCompareDb.add(item);
}

int EventDatabase::find(const Element &elem, std::vector<Event::Info_S> &EventInfos)
{
    MatchMethods methods;
    methods.push_back(MatchMethod(elem, FIND_CRITERION_EQ));
    find(methods, EventInfos);

    return 0;
}

int EventDatabase::find(const MatchMethods &methods, std::vector<Event::Info_S> &EventInfos)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    std::vector<Item> items;
    m_eventDb.find(methods, items);

    for (Item &item : items)
    {
        Event::Info_S stEventInfo;
        for (Element &pair : item)
        {
            std::string &key = pair.first;
            FieldValue &value = pair.second;
            switch (str2tag(key.c_str()))
            {
            case str2tag(DB_COMMON_FIELD_ID):
                stEventInfo.nId = mpark::get<int>(value);
                break;
            case str2tag(INFO_CHANNEL_ID):
                stEventInfo.nChnId = mpark::get<int>(value);
                break;
            case str2tag(INFO_EVENT_TYPE):
                stEventInfo.enType = (Type_E)mpark::get<int>(value);
                break;
            case str2tag(INFO_EVENT_DATE):
                stEventInfo.strDate = mpark::get<std::string>(value);
                break;
            case str2tag(INFO_EVENT_TIME):
                stEventInfo.strTime = mpark::get<std::string>(value);
                break;
            case str2tag(INFO_RECORD_STATRTIME):
                stEventInfo.strStartTime = mpark::get<std::string>(value);
                break;
            case str2tag(INFO_RECORD_ENDTIME):
                stEventInfo.strEndTime = mpark::get<std::string>(value);
                break;
            case str2tag(INFO_TIMESTAMP):
                stEventInfo.lTimestamp = std::stoll(mpark::get<std::string>(value));
                break;
            case str2tag(INFO_RECORD_LABEL):
                stEventInfo.strLabel = mpark::get<std::string>(value);
                break;
            case str2tag(INFO_VIDEO_PATH):
                stEventInfo.strVideoPath = mpark::get<std::string>(value);
                break;
            case str2tag(INFO_VIDEO_SIZE):
                stEventInfo.nVideoSize = mpark::get<int>(value);
                break;
            case str2tag(INFO_VIDEO_BIND_ID):
                stEventInfo.nVideoBindId = mpark::get<int>(value);
                break;
            default:
                break;
            }

        }
        EventInfos.push_back(stEventInfo);
    }

    return 0;
}

int EventDatabase::get_count(const MatchMethods &methods, int &nCount, const std::string field)
{
    return m_eventDb.get_count(methods, nCount, field);
}

int EventDatabase::get_faceCompareCount(const MatchMethods &methods, int &nCount, const std::string field)
{
    return m_faceCompareDb.get_count(methods, nCount, field);
}
int EventDatabase::find(const MatchMethods &methods, std::vector<Event::FaceCompareInfo_S> &faceCompareInfos)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    std::vector<Item> items;
    m_faceCompareDb.find(methods, items);

    for (Item &item : items)
    {
        Event::FaceCompareInfo_S stFaceCompareInfo;
        for (Element &pair : item)
        {
            std::string &key = pair.first;
            FieldValue &value = pair.second;
            switch (str2tag(key.c_str()))
            {
            case str2tag(INFO_EVENT_ID):
                stFaceCompareInfo.nEventId = mpark::get<int>(value);
                break;
            case str2tag(INFO_COMP_RESULT):
                stFaceCompareInfo.nCompResult = mpark::get<int>(value);
                break;
            case str2tag(INFO_SIMILARITY):
                stFaceCompareInfo.nSimilarity = mpark::get<int>(value);
                break;
            case str2tag(INFO_FACE_ID):
                stFaceCompareInfo.nFaceId = mpark::get<int>(value);
                break;
            case str2tag(INFO_LIB_NAME):
                stFaceCompareInfo.strFaceLibName = mpark::get<std::string>(value);
                break;
            case str2tag(INFO_FACE_NAME):
                stFaceCompareInfo.strFaceName = mpark::get<std::string>(value);
                break;
            case str2tag(INFO_LIB_FACE_PATH):
                stFaceCompareInfo.strLibFacePath = mpark::get<std::string>(value);
                break;
            case str2tag(INFO_CAPTURE_FACE_PATH):
                stFaceCompareInfo.strCapFacePath = mpark::get<std::string>(value);
                break;
            default:
                break;
            }
        }
        faceCompareInfos.push_back(stFaceCompareInfo);
    }

    return 0;
}


int EventDatabase::update(const Item &item, const MatchMethods &methods)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_eventDb.update(item, methods);
}


int EventDatabase::del(const Item &item)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_eventDb.del(item);
}

int EventDatabase::del(const MatchMethods &methods, std::string strTargetTableName)
{
    return m_eventDb.del(methods, strTargetTableName);
}

int EventDatabase::clear_table(const std::string &tableName, bool resetAutoInc)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_eventDb.clear_table(tableName);
}