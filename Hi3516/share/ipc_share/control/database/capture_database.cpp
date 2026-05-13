/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-10-16 17:22:43
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-03-20 13:43:15
 * @FilePath: /hisi/share/ipc_share/control/database/capture_database.cpp
 * @Description: 抓图信息数据库
 */
#include "capture_database.h"

using namespace Db;

CCaptureDatabase::CCaptureDatabase()
    : m_database(CAPTURE_DATABASE_PATH, CAPTURE_TABLE_NAME)
    , m_captureDirDatabase(CAPTURE_DATABASE_PATH, CAPTURE_DIR_INFO_TABLE_NAME)
{
    create(CAPTURE_TABLE_NAME);
    create(CAPTURE_DIR_INFO_TABLE_NAME);

}

CCaptureDatabase::~CCaptureDatabase()
{
}

int CCaptureDatabase::create(std::string tableName, bool bAddTableKey)
{
    if (tableName == CAPTURE_TABLE_NAME && bAddTableKey)
    {
        m_database.add_tableKey(TableKey(INFO_CHNL_ID, CDbBase::type_int()));
        m_database.add_tableKey(TableKey(INFO_CAPTURE_EVENT_TYPE, CDbBase::type_int()));
        m_database.add_tableKey(TableKey(INFO_CAPTURE_STATRTIME, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_database.add_tableKey(TableKey(INFO_CAPTURE_ENDTIME, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        // m_database.add_tableKey(TableKey(INFO_TIMESTAMP, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_database.add_tableKey(TableKey(INFO_CAPTURE_PATH, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_database.add_tableKey(TableKey(INFO_CAPTURE_SIZE, CDbBase::type_int()));

        m_database.init();
    }
    else if(tableName == CAPTURE_DIR_INFO_TABLE_NAME && bAddTableKey)
    {
        m_captureDirDatabase.add_tableKey(TableKey(INFO_CHNL_ID, CDbBase::type_int()));
        m_captureDirDatabase.add_tableKey(TableKey(INFO_CAPTURE_COUNT, CDbBase::type_int()));
        m_captureDirDatabase.add_tableKey(TableKey(INFO_CAPTURE_TOTAL_SIZE, CDbBase::type_string(MAX_DB_STRING_SIZE)));

        m_captureDirDatabase.init(false);
    }
    else if (tableName == CAPTURE_TABLE_NAME && !bAddTableKey)
    {
        m_database.init(false);
    }
    else if (tableName == CAPTURE_DIR_INFO_TABLE_NAME && !bAddTableKey)
    {
        m_captureDirDatabase.init(false);
    }
    
    return 0;
}

int CCaptureDatabase::init()
{
    create(CAPTURE_TABLE_NAME, false);
    create(CAPTURE_DIR_INFO_TABLE_NAME, false);

    return 0;
}

int CCaptureDatabase::deinit()
{
    m_database.deinit();
    m_captureDirDatabase.deinit();

    return 0;
}  

int CCaptureDatabase::add(const Capture_NS::CaptureInfo_S &stInfo)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    Item item;

    item.push_back(Element(INFO_CHNL_ID, stInfo.nChnId));
    item.push_back(Element(INFO_CAPTURE_EVENT_TYPE, (int)stInfo.enType));
    item.push_back(Element(INFO_CAPTURE_STATRTIME, stInfo.strStartTime));
    item.push_back(Element(INFO_CAPTURE_ENDTIME, stInfo.strEndTime)); 
    // item.push_back(Element(INFO_TIMESTAMP, std::to_string(stInfo.lTimestamp)));
    item.push_back(Element(INFO_CAPTURE_PATH, stInfo.strImagePath));
    item.push_back(Element(INFO_CAPTURE_SIZE, stInfo.nImageSize));

    item.push_back(Element(DB_COMMON_FIELD_RESERVE1, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE2, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE3, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE4, std::string()));

    return m_database.add(item);
}

int CCaptureDatabase::add(const Capture_NS::CaptureDirInfo_S &stInfo)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    Item item;

    item.push_back(Element(INFO_CHNL_ID, stInfo.nChnId));
    item.push_back(Element(INFO_CAPTURE_COUNT, stInfo.nCount));
    item.push_back(Element(INFO_CAPTURE_TOTAL_SIZE, std::to_string(stInfo.nTotalSize)));

    return m_captureDirDatabase.add(item);
}

int CCaptureDatabase::find(const Element &elem, std::vector<Capture_NS::CaptureInfo_S> &infos)
{
    MatchMethods methods;
    methods.push_back(MatchMethod(elem, FIND_CRITERION_EQ));
    find(methods, infos);

    return 0;
}

int CCaptureDatabase::find(const MatchMethods &methods, std::vector<Capture_NS::CaptureInfo_S> &infos)
{
    std::vector<Item> items;
    std::unique_lock<std::mutex> lock(m_mutex);
    m_database.find(methods, items);

    for (Item &item : items)
    {
        Capture_NS::CaptureInfo_S stInfo;
        for (Element &pair : item)
        {
            std::string &key = pair.first;
            FieldValue &value = pair.second;
            switch (str2tag(key.c_str()))
            {
                case str2tag(INFO_CHNL_ID):
                    stInfo.nChnId = mpark::get<int>(value);
                    break;
                case str2tag(INFO_CAPTURE_EVENT_TYPE):
                    stInfo.enType = (Event::Type_E)mpark::get<int>(value);
                    break;
                case str2tag(INFO_CAPTURE_STATRTIME):
                    stInfo.strStartTime = mpark::get<std::string>(value);
                    break;
                case str2tag(INFO_CAPTURE_ENDTIME):
                    stInfo.strEndTime = mpark::get<std::string>(value);
                    break;
                // case str2tag(INFO_TIMESTAMP):
                //     stInfo.lTimestamp = std::stoll(mpark::get<std::string>(value));
                //     break;
                case str2tag(INFO_CAPTURE_PATH):
                    stInfo.strImagePath = mpark::get<std::string>(value);
                    break;
                case str2tag(INFO_CAPTURE_SIZE):
                    stInfo.nImageSize = mpark::get<int>(value);
                    break;
                default:
                    break;
            }
        }
        infos.push_back(stInfo);

    }
    return 0;
}

int CCaptureDatabase::del(const Item &item)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_database.del(item);
}

int CCaptureDatabase::del(const MatchMethods &methods, std::string strTargetTableName)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_database.del(methods, strTargetTableName);
}

int CCaptureDatabase::find(const Element &elem, std::vector<Capture_NS::CaptureDirInfo_S> &infos)
{
    MatchMethods methods;
    methods.push_back(MatchMethod(elem, FIND_CRITERION_EQ));
    find(methods, infos);

    return 0;
}

int CCaptureDatabase::find(const MatchMethods &methods, std::vector<Capture_NS::CaptureDirInfo_S> &infos)
{
    std::vector<Item> items;
    std::unique_lock<std::mutex> lock(m_mutex);
    m_captureDirDatabase.find(methods, items);

    for (Item &item : items)
    {
        Capture_NS::CaptureDirInfo_S stInfo;
        for (Element &pair : item)
        {
            std::string &key = pair.first;
            FieldValue &value = pair.second;
            switch (str2tag(key.c_str()))
            {
                case str2tag(INFO_CHNL_ID):
                    stInfo.nChnId = mpark::get<int>(value);
                    break;
                case str2tag(INFO_CAPTURE_COUNT):
                    stInfo.nCount = mpark::get<int>(value);
                    break;
                case str2tag(INFO_CAPTURE_TOTAL_SIZE):
                    stInfo.nTotalSize = std::stoll(mpark::get<std::string>(value));
                    break;
                default:
                    break;
            }
        }
        infos.push_back(stInfo);

    }
    return 0;
}

int CCaptureDatabase::update(Capture_NS::CaptureDirInfo_S &stInfo)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    Item item;
    item.push_back(Element(INFO_CHNL_ID, stInfo.nChnId));
    item.push_back(Element(INFO_CAPTURE_COUNT, (int)stInfo.nCount));
    item.push_back(Element(INFO_CAPTURE_TOTAL_SIZE, std::to_string(stInfo.nTotalSize)));

    MatchMethods methods;
    methods.push_back(MatchMethod(Element(INFO_CHNL_ID, stInfo.nChnId), FIND_CRITERION_EQ));

    return m_captureDirDatabase.update(item, methods);
}

int CCaptureDatabase::get_count(const MatchMethods &methods, int &nCount, const std::string field)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_database.get_count(methods, nCount, field);
}

int CCaptureDatabase::get_itemInfo(Capture_NS::CaptureDirInfo_S& stInfo)
{
    // std::unique_lock<std::mutex> lock(m_mutex);
    std::vector<Capture_NS::CaptureDirInfo_S> stInfos;
    find(Element(INFO_CHNL_ID, stInfo.nChnId), stInfos);
    if (stInfos.size() == 0)
    {
        dlog_info("抓图目录信息数据库信息为空");
        return -1;
    }
    stInfo = stInfos[0];
    return 0;
}

int CCaptureDatabase::clear_table(const std::string &tableName, bool resetAutoInc)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_database.clear_table(tableName);
}