/**
 * @FilePath     : record_file_database.cpp
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-10-29
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-30 20:31:46
 * @Description  : 录制文件数据库
 */

#include "record_file_database.h"

#include <iostream>

using namespace Db;
RecordFileDatabase::RecordFileDatabase()
    : m_database(RECORD_FILE_DATABASE_PATH, RECORD_FILE_TABLE_NAME)
    , m_recordDirDatabase(RECORD_FILE_DATABASE_PATH, RECORD_DIR_INFO_TABLE_NAME)
{
    // 创建主表
    create(RECORD_FILE_TABLE_NAME);
    create(RECORD_DIR_INFO_TABLE_NAME);
}

RecordFileDatabase::~RecordFileDatabase()
{
}

/* 创建表record_file_manage */
int RecordFileDatabase::create(std::string tableName, bool bAddTableKey)
{
    if(tableName == RECORD_FILE_TABLE_NAME && bAddTableKey)
    {
        m_database.add_tableKey(TableKey(RECORD_FILE_FIELD_CHN_ID, CDbBase::type_int()));
        m_database.add_tableKey(TableKey(RECORD_FILE_FIELD_PATH, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_database.add_tableKey(TableKey(RECORD_FILE_FIELD_FILENAME, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_database.add_tableKey(TableKey(RECORD_FILE_FIELD_SIZE, CDbBase::type_int()));
        m_database.add_tableKey(TableKey(RECORD_FILE_FIELD_TYPE, CDbBase::type_int()));
        m_database.add_tableKey(TableKey(RECORD_FILE_FIELD_IS_LOCK, CDbBase::type_int()));
        m_database.add_tableKey(TableKey(RECORD_FILE_FIELD_STATUS, CDbBase::type_int()));
        m_database.add_tableKey(TableKey(RECORD_FILE_FIELD_CREATE_TIME, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_database.add_tableKey(TableKey(RECORD_FILE_FIELD_MODIFY_TIME, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_database.add_tableKey(TableKey(RECORD_FILE_FIELD_DURATION, CDbBase::type_int()));

        m_database.init();
    }
    else if(tableName == RECORD_DIR_INFO_TABLE_NAME && bAddTableKey)
    {
        m_recordDirDatabase.add_tableKey(TableKey(RECORD_FILE_FIELD_CHN_ID, CDbBase::type_int()));
        m_recordDirDatabase.add_tableKey(TableKey(INFO_TS_COUNT, CDbBase::type_int()));
        m_recordDirDatabase.add_tableKey(TableKey(INFO_TS_TOTAL_SIZE, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    
        m_recordDirDatabase.init(false);
    }
    else if(tableName == RECORD_FILE_TABLE_NAME && !bAddTableKey)
    {
        m_database.init(false);
    }
    else if(tableName == RECORD_DIR_INFO_TABLE_NAME && !bAddTableKey)
    {
        m_recordDirDatabase.init(false);
    }

    return 0;
}

int RecordFileDatabase::init()
{
    create(RECORD_FILE_TABLE_NAME, false);
    create(RECORD_DIR_INFO_TABLE_NAME, false);

    // 获取当前时间点
    std::time_t now = std::time(nullptr);
    // 转换为本地时间
    std::tm today;
    localtime_r(&now, &today); // 使用线程安全版本的 localtime
    // 使用 strftime 格式化日期
    char buffer[11] = {0}; // YYYY-MM-DD + 1 字符长度
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &today);
    std::string tableName = "\"" + std::string(buffer) + "\"";
    /* 创建一个记录录制ts信息的表 */
    create_sub(tableName);

    return 0;
}

int RecordFileDatabase::deinit()
{
    m_database.deinit();
    m_recordDirDatabase.deinit();
    
    std::unique_lock<std::mutex> lock(m_mutex);
    for (auto& pair : m_subDbMap) 
    {
        if (pair.second) 
        {
            pair.second->deinit();
            delete pair.second;
        }
    }
    m_subDbMap.clear();

    return 0;
}  

/* 创建一个表格 */
int RecordFileDatabase::create_sub(std::string tableName, bool bAddTableKey) {
    std::unique_lock<std::mutex> lock(m_mutex);
    get_sub_handle(tableName);
    return 0;
}

int RecordFileDatabase::init_sub(std::string strDate)
{
    std::string date(strDate);
    date.resize(strlen("YYYY-MM-DD"));
    std::string tableName = "\"" + date + "\"";
    create_sub(tableName);
    return 0;
}

CDbBase* RecordFileDatabase::get_sub_handle(std::string tableName)
{
    //调用此函数的地方要加 m_mutex 锁
    if (m_subDbMap.count(tableName)) 
    {
        return m_subDbMap[tableName];
    }

    // 限制缓存数量：保留最近 2 天的句柄
    if (m_subDbMap.size() >= 2) 
    {
        auto it = m_subDbMap.begin();
        it->second->deinit();
        delete it->second;
        m_subDbMap.erase(it);
    }

    CDbBase* pNewDb = new CDbBase(RECORD_FILE_DATABASE_PATH, tableName);
    
    pNewDb->add_tableKey(TableKey(RECORD_FILE_FIELD_CHN_ID, CDbBase::type_int()));
    pNewDb->add_tableKey(TableKey(RECORD_FILE_FIELD_TYPE, CDbBase::type_int()));
    pNewDb->add_tableKey(TableKey(RECORD_FILE_FIELD_PATH, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    pNewDb->add_tableKey(TableKey(RECORD_FILE_FIELD_FILENAME, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    pNewDb->add_tableKey(TableKey(RECORD_FILE_FIELD_SIZE, CDbBase::type_int()));
    pNewDb->add_tableKey(TableKey(RECORD_FILE_FIELD_CREATE_TIME, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    pNewDb->add_tableKey(TableKey(RECORD_FILE_FIELD_FILE_INDEX, CDbBase::type_int()));
    pNewDb->add_tableKey(TableKey(RECORD_FILE_FIELD_DURATION, CDbBase::type_int()));

    pNewDb->init(false);
    m_subDbMap[tableName] = pNewDb;
    
    return pNewDb;
}

int RecordFileDatabase::add(const Record_NS::FileInfo_S &stInfo)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    Item item;
    item.push_back(Element(RECORD_FILE_FIELD_CHN_ID, stInfo.nChnId));
    item.push_back(Element(RECORD_FILE_FIELD_PATH, stInfo.path));
    item.push_back(Element(RECORD_FILE_FIELD_FILENAME, stInfo.filename));
    item.push_back(Element(RECORD_FILE_FIELD_SIZE, stInfo.nSize));
    item.push_back(Element(RECORD_FILE_FIELD_TYPE, stInfo.nType));
    item.push_back(Element(RECORD_FILE_FIELD_IS_LOCK, stInfo.bLock));
    item.push_back(Element(RECORD_FILE_FIELD_STATUS, stInfo.nStatus));
    item.push_back(Element(RECORD_FILE_FIELD_CREATE_TIME, stInfo.createTime));
    item.push_back(Element(RECORD_FILE_FIELD_MODIFY_TIME, stInfo.modifyTime));
    item.push_back(Element(RECORD_FILE_FIELD_DURATION, stInfo.nDuration));

    item.push_back(Element(DB_COMMON_FIELD_RESERVE1, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE2, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE3, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE4, std::string()));

    std::string date = stInfo.createTime;
    if (date.length() >= 10) 
    {
        std::string subTableName = "\"" + date.substr(0, 10) + "\"";
        get_sub_handle(subTableName);
    }

    return m_database.add(item);
}

int RecordFileDatabase::add(const Record_NS::TsFileInfo_S &stInfo)
{
    std::unique_lock<std::mutex> lock(m_mutex); 
    // 获取完整的日期和时间
    std::string fullTime = stInfo.createTime; // 格式 "2024-10-29 18:25:33"
    
    if (fullTime.length() < 10) 
    {
        dlog_error("Time format error, too short: %s", fullTime.c_str());
        return -1;
    }

    // 提取日期作为表名路由
    std::string datePart = fullTime.substr(0, 10); // "2024-10-29"
    std::string tableName = "\"" + datePart + "\"";

    // 提取时间部分用于存入子表字段 (HH:MM:SS)
    std::string timeOnly = fullTime;
    size_t pos = fullTime.find(' ');
    if (pos != std::string::npos) 
    {
        timeOnly = fullTime.substr(pos + 1); // "18:25:33"
    }

    // 获取数据库句柄
    CDbBase* db = get_sub_handle(tableName);

    Item item;
    item.push_back(Element(RECORD_FILE_FIELD_CHN_ID, stInfo.nChnId));
    item.push_back(Element(RECORD_FILE_FIELD_TYPE, stInfo.nType));
    item.push_back(Element(RECORD_FILE_FIELD_PATH, stInfo.path));
    item.push_back(Element(RECORD_FILE_FIELD_FILENAME, stInfo.filename));
    item.push_back(Element(RECORD_FILE_FIELD_SIZE, stInfo.nSize));
    item.push_back(Element(RECORD_FILE_FIELD_CREATE_TIME, timeOnly));
    item.push_back(Element(RECORD_FILE_FIELD_FILE_INDEX, stInfo.nIndex));
    item.push_back(Element(RECORD_FILE_FIELD_DURATION, stInfo.nDuration));

    return db->add(item);
}

int RecordFileDatabase::add(const Record_NS::RecordDirInfo_S &stInfo)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    Item item;

    item.push_back(Element(RECORD_FILE_FIELD_CHN_ID, stInfo.nChnId));
    item.push_back(Element(INFO_TS_COUNT, stInfo.nCount));
    item.push_back(Element(INFO_TS_TOTAL_SIZE, std::to_string(stInfo.nTotalSize)));

    return m_recordDirDatabase.add(item);
}

int RecordFileDatabase::find(const Element &elem, std::vector<Record_NS::FileInfo_S> &infos)
{
    MatchMethods methods;
    methods.push_back(MatchMethod(elem, FIND_CRITERION_EQ));
    find(methods, infos);

    return 0;
}

int RecordFileDatabase::find(const Element &elem, std::vector<Record_NS::TsFileInfo_S> &infos)
{
    MatchMethods methods;
    methods.push_back(MatchMethod(elem, FIND_CRITERION_EQ));
    find(methods, infos);
    return 0;
}

int RecordFileDatabase::find(const Element &elem, std::vector<Record_NS::RecordDirInfo_S> &infos)
{
    MatchMethods methods;
    methods.push_back(MatchMethod(elem, FIND_CRITERION_EQ));
    find(methods, infos);

    return 0;
}

int RecordFileDatabase::find(const MatchMethods &methods, std::vector<Record_NS::RecordDirInfo_S> &infos)
{
    std::vector<Item> items;
    std::unique_lock<std::mutex> lock(m_mutex);
    m_recordDirDatabase.find(methods, items);

    for (Item &item : items)
    {
        Record_NS::RecordDirInfo_S stInfo;
        for (Element &pair : item)
        {
            std::string &key = pair.first;
            FieldValue &value = pair.second;
            switch (str2tag(key.c_str()))
            {
                case str2tag(RECORD_FILE_FIELD_CHN_ID):
                    stInfo.nChnId = mpark::get<int>(value);
                    break;
                case str2tag(INFO_TS_COUNT):
                    stInfo.nCount = mpark::get<int>(value);
                    break;
                case str2tag(INFO_TS_TOTAL_SIZE):
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

int RecordFileDatabase::find(const MatchMethods &methods, std::vector<Record_NS::FileInfo_S> &infos)
{
    std::vector<Item> items;
    std::unique_lock<std::mutex> lock(m_mutex);
    m_database.find(methods, items);

    for (Item &item : items)
    {
        Record_NS::FileInfo_S stInfo;
        for (Element &pair : item)
        {
            std::string &key = pair.first;
            FieldValue &value = pair.second;
            switch (str2tag(key.c_str()))
            {
            case str2tag(DB_COMMON_FIELD_ID):
                stInfo.nId = mpark::get<int>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_CHN_ID):
                stInfo.nChnId = mpark::get<int>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_PATH):
                stInfo.path = mpark::get<std::string>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_FILENAME):
                stInfo.filename = mpark::get<std::string>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_SIZE):
                stInfo.nSize = mpark::get<int>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_TYPE):
                stInfo.nType = mpark::get<int>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_IS_LOCK):
                stInfo.bLock = mpark::get<int>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_STATUS):
                stInfo.nStatus = mpark::get<int>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_CREATE_TIME):
                stInfo.createTime = mpark::get<std::string>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_MODIFY_TIME):
                stInfo.modifyTime = mpark::get<std::string>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_DURATION):
                stInfo.nDuration = mpark::get<int>(value);
                break;
            default:
                break;
            }
        }
        infos.push_back(stInfo);
    }
    return 0;
}

int RecordFileDatabase::find(const MatchMethods &methods, std::vector<Record_NS::TsFileInfo_S> &infos, std::string strTargetTableName)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string actualTableName = strTargetTableName;
    if (actualTableName.empty()) 
    {
        std::time_t now = std::time(nullptr);
        char buffer[11] = {0};
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", std::localtime(&now));
        actualTableName = "\"" + std::string(buffer) + "\"";
    }

    CDbBase* db = get_sub_handle(actualTableName);
    std::vector<Item> items;
    db->find(methods, items, actualTableName);

    for (Item &item : items)
    {
        Record_NS::TsFileInfo_S stInfo;
        for (Element &pair : item)
        {
            std::string &key = pair.first;
            FieldValue &value = pair.second;
            switch (str2tag(key.c_str()))
            {
            case str2tag(DB_COMMON_FIELD_ID):
                stInfo.nId = mpark::get<int>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_CHN_ID):
                stInfo.nChnId = mpark::get<int>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_PATH):
                stInfo.path = mpark::get<std::string>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_FILENAME):
                stInfo.filename = mpark::get<std::string>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_SIZE):
                stInfo.nSize = mpark::get<int>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_CREATE_TIME):
                stInfo.createTime = mpark::get<std::string>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_FILE_INDEX):
                stInfo.nIndex = mpark::get<int>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_DURATION):
                stInfo.nDuration = mpark::get<int>(value);
                break;
            default:
                break;
            }
        }
        infos.push_back(stInfo);
    }
    return 0;
}

int Db::RecordFileDatabase::find(std::string cmd, std::vector<Record_NS::TsFileInfo_S> &infos)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_subDbMap.empty()) 
    {
        // 如果缓存是空的，默认打开今天的表
        std::time_t now = std::time(nullptr);
        char buf[11];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&now));
        get_sub_handle("\"" + std::string(buf) + "\"");
    }

    auto it = m_subDbMap.begin();
    std::vector<Item> items;
    it->second->find(cmd, items);

    for (Item &item : items)
    {
        Record_NS::TsFileInfo_S stInfo;
        for (Element &pair : item)
        {
            std::string &key = pair.first;
            FieldValue &value = pair.second;
            switch (str2tag(key.c_str()))
            {
            case str2tag(DB_COMMON_FIELD_ID):
                stInfo.nId = mpark::get<int>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_CHN_ID):
                stInfo.nChnId = mpark::get<int>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_PATH):
                stInfo.path = mpark::get<std::string>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_FILENAME):
                stInfo.filename = mpark::get<std::string>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_SIZE):
                stInfo.nSize = mpark::get<int>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_CREATE_TIME):
                stInfo.createTime = mpark::get<std::string>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_FILE_INDEX):
                stInfo.nIndex = mpark::get<int>(value);
                break;
            case str2tag(RECORD_FILE_FIELD_DURATION):
                stInfo.nDuration = mpark::get<int>(value);
                break;
            default:
                break;
            }
        }
        infos.push_back(stInfo);
    }
    return 0;
}

int RecordFileDatabase::get_count(const MatchMethods &methods, int &nCount, const std::string field)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_database.get_count(methods, nCount, field);
}

int RecordFileDatabase::get_subDataCount(const MatchMethods &methods, int &nCount, const std::string field, std::string strTargetTableName)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    std::string actualTable = strTargetTableName;
    if (actualTable.empty()) 
    {
        std::time_t now = std::time(nullptr);
        char buffer[11] = {0};
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", std::localtime(&now));
        actualTable = "\"" + std::string(buffer) + "\"";
    }

    CDbBase* db = get_sub_handle(actualTable);
    return db->get_count(methods, nCount, field, actualTable);
}

int RecordFileDatabase::update(const Item &item, const MatchMethods &methods, std::string strTargetTableName)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if(strTargetTableName.empty())
    {
        return m_database.update(item, methods);
    }
    else 
    {
        CDbBase* db = get_sub_handle(strTargetTableName);
        return db->update(item, methods, strTargetTableName);
    }
    
}

int RecordFileDatabase::update(Record_NS::RecordDirInfo_S &stInfo)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    Item item;
    item.push_back(Element(RECORD_FILE_FIELD_CHN_ID, stInfo.nChnId));
    item.push_back(Element(INFO_TS_COUNT, (int)stInfo.nCount));
    item.push_back(Element(INFO_TS_TOTAL_SIZE, std::to_string(stInfo.nTotalSize)));

    MatchMethods methods;
    methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CHN_ID, stInfo.nChnId), FIND_CRITERION_EQ));

    return m_recordDirDatabase.update(item, methods);
}

int RecordFileDatabase::get_itemInfo(Record_NS::RecordDirInfo_S& stInfo)
{
    // std::unique_lock<std::mutex> lock(m_mutex);
    std::vector<Record_NS::RecordDirInfo_S> stInfos;
    find(Element(RECORD_FILE_FIELD_CHN_ID, stInfo.nChnId), stInfos);
    if (stInfos.size() == 0)
    {
        // dlog_info("录制文件目录信息数据库信息为空");
        return -1;
    }
    stInfo = stInfos[0];

    return 0;
}

int RecordFileDatabase::del(const Item &item)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_database.del(item);
}

int RecordFileDatabase::del(const MatchMethods &methods, std::string strTargetTableName)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_database.del(methods, strTargetTableName);
}

std::vector<std::string> RecordFileDatabase::get_all_tables()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_database.get_all_tables();
}

std::vector<std::string> RecordFileDatabase::get_column_data(const std::string &tableName, const std::string &targetField, int nMaxRows)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_database.get_column_data(tableName, targetField, nMaxRows);
}

bool RecordFileDatabase::delete_record_by_field(const std::string &tableName, const std::string &targetFile, const std::string &targetField)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_database.delete_record_by_field(tableName, targetFile, targetField);
}

int RecordFileDatabase::get_table_data_count(const std::string &tableName)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_database.get_table_data_count(tableName);
}

int RecordFileDatabase::del_table(const std::string &tableName)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_database.del_table(tableName);
}

int RecordFileDatabase::clear_table(const std::string &tableName, bool resetAutoInc)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_database.clear_table(tableName);
}
