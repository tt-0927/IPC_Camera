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
    
    if(m_subDatabase)
    {
        m_subDatabase->deinit();
        delete m_subDatabase;
        m_subDatabase = nullptr;
    }
    return 0;
}  

/* 创建一个表格 */
int RecordFileDatabase::create_sub(std::string tableName, bool bAddTableKey)
{
    if (m_subDatabase)
    {
        /* 表名一样 */
        if (m_subDatabase->get_tableName() == tableName)
        {
            return 0;
        }
        delete m_subDatabase;
        m_subDatabase = nullptr;
    }
    bool bAddDefault = false;
    m_subDatabase = new CDbBase(RECORD_FILE_DATABASE_PATH, tableName);

    if(bAddTableKey)
    {
        m_subDatabase->add_tableKey(TableKey(RECORD_FILE_FIELD_CHN_ID, CDbBase::type_int()));
        m_subDatabase->add_tableKey(TableKey(RECORD_FILE_FIELD_TYPE, CDbBase::type_int()));
        m_subDatabase->add_tableKey(TableKey(RECORD_FILE_FIELD_PATH, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_subDatabase->add_tableKey(TableKey(RECORD_FILE_FIELD_FILENAME, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_subDatabase->add_tableKey(TableKey(RECORD_FILE_FIELD_SIZE, CDbBase::type_int()));
        m_subDatabase->add_tableKey(TableKey(RECORD_FILE_FIELD_CREATE_TIME, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        m_subDatabase->add_tableKey(TableKey(RECORD_FILE_FIELD_FILE_INDEX, CDbBase::type_int()));
        m_subDatabase->add_tableKey(TableKey(RECORD_FILE_FIELD_DURATION, CDbBase::type_int()));
    }

    m_subDatabase->init(bAddDefault);
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
    /* 新建表格 */
    std::string date(stInfo.createTime);
    date.resize(strlen("YYYY-MM-DD"));
    std::string tableName = "\"" + date + "\"";
    create_sub(tableName);

    return m_database.add(item);
}
int RecordFileDatabase::add(const Record_NS::TsFileInfo_S &stInfo)
{
    if(m_subDatabase == nullptr)
    {
        dlog_error("subDatabase is nullptr");
        return -1;
    }

    /* 找到空格的位置 */
    size_t pos = stInfo.createTime.find(' ');
    /* 如果找到空格，裁剪出后面的部分 */
    std::string strCreateTime = (pos != std::string::npos) ? stInfo.createTime.substr(pos + 1) : stInfo.createTime;

    Item item;
    item.push_back(Element(RECORD_FILE_FIELD_CHN_ID, stInfo.nChnId));
    item.push_back(Element(RECORD_FILE_FIELD_TYPE, stInfo.nType));
    item.push_back(Element(RECORD_FILE_FIELD_PATH, stInfo.path));
    item.push_back(Element(RECORD_FILE_FIELD_FILENAME, stInfo.filename));
    item.push_back(Element(RECORD_FILE_FIELD_SIZE, stInfo.nSize));
    item.push_back(Element(RECORD_FILE_FIELD_CREATE_TIME, strCreateTime));
    item.push_back(Element(RECORD_FILE_FIELD_FILE_INDEX, stInfo.nIndex));
    item.push_back(Element(RECORD_FILE_FIELD_DURATION, stInfo.nDuration));

    std::string date(stInfo.createTime);
    date.resize(strlen("YYYY-MM-DD"));
    std::string strTargetTableName = "\"" + date + "\"";

    std::unique_lock<std::mutex> lock(m_mutex);
    return m_subDatabase->add(item, strTargetTableName);
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
    if(m_subDatabase == nullptr)
    {
        dlog_error("subDatabase is nullptr");
        return -1;
    }
    std::vector<Item> items;
    std::unique_lock<std::mutex> lock(m_mutex);
    if(strTargetTableName.empty())
    {
        m_subDatabase->find(methods, items);
    }
    else 
    {
        m_subDatabase->find(methods, items, strTargetTableName);
    }

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
    if(m_subDatabase == nullptr)
    {
        dlog_error("subDatabase is nullptr");
        return -1;
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    std::vector<Item> items;
    m_subDatabase->find(cmd, items);

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
    if(m_subDatabase == nullptr)
    {
        dlog_error("subDatabase is nullptr");
        return -1;
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_subDatabase->get_count(methods, nCount, field, strTargetTableName);
}

int RecordFileDatabase::update(const Item &item, const MatchMethods &methods, std::string strTargetTableName)
{
    if(m_subDatabase == nullptr)
    {
        dlog_error("subDatabase is nullptr");
        return -1;
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    if(strTargetTableName.empty())
    {
        return m_database.update(item, methods);
    }
    else 
    {
        return m_subDatabase->update(item, methods, strTargetTableName);
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
