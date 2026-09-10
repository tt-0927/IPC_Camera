/**
 * @FilePath     : log_database.cpp
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2025-02-11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-23 19:17:30
 * @Description  : 日志数据库
 */

#include "log_database.h"

#include <iostream>

using namespace Db;
LogDatabase::LogDatabase()
{
    create(LOG_TABLE_NAME);
}

LogDatabase::~LogDatabase()
{
    if (m_database)
    {
        delete m_database;
        m_database = nullptr;
    }
}

int LogDatabase::create(std::string tableName)
{
    if (m_database)
    {
        /* 表名一样 */
        if (m_database->get_tableName() == tableName)
        {
            return 0;
        }
        delete m_database;
        m_database = nullptr;
    }
    m_database = new CDbBase(LOG_DATABASE_PATH, tableName);

    bool bAddDefault = false;
    m_database->add_tableKey(TableKey(LOG_FIELD_START_TIME, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    m_database->add_tableKey(TableKey(LOG_FIELD_TYPE, CDbBase::type_int()));
    m_database->add_tableKey(TableKey(LOG_FIELD_ACTION, CDbBase::type_int()));
    // m_database->add_tableKey(TableKey(LOG_FIELD_CHN_NAME, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    m_database->add_tableKey(TableKey(LOG_FIELD_USER, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    m_database->add_tableKey(TableKey(LOG_FIELD_HOST, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    m_database->add_tableKey(TableKey(LOG_FIELD_CONTEXT, CDbBase::type_string(MAX_DB_STRING_SIZE)));

    m_database->init(bAddDefault);
    return 0;
}

int LogDatabase::add(const Log::Info_S &stInfo)
{
    if (m_database == nullptr)
    {
        return -1;
    }
    Item item;
    item.emplace_back(LOG_FIELD_START_TIME, stInfo.startTime);
    item.emplace_back(LOG_FIELD_TYPE, stInfo.nType);
    item.emplace_back(LOG_FIELD_ACTION, stInfo.nAction);
    // item.emplace_back(LOG_FIELD_CHN_NAME, stInfo.chnName);
    item.emplace_back(LOG_FIELD_USER, stInfo.user);
    item.emplace_back(LOG_FIELD_HOST, stInfo.host);
    item.emplace_back(LOG_FIELD_CONTEXT, stInfo.context);

    // /* 新建表格 */
    // std::string date(stInfo.startTime);
    // date.resize(strlen("YYYY-MM-DD"));
    // std::string tableName = "\"" + date + "\"";
    // create(tableName);
    return m_database->add(item);
}


int LogDatabase::find(const Element &elem, std::vector<Log::Info_S> &infos)
{
    if (m_database == nullptr)
    {
        return 0;
    }
    MatchMethods methods;
    methods.push_back(MatchMethod(elem, FIND_CRITERION_EQ));
    find(methods, infos);
    return 0;
}


int LogDatabase::find(const MatchMethods &methods, std::vector<Log::Info_S> &infos)
{
    if (m_database == nullptr)
    {
        return 0;
    }
    std::vector<Item> items;
    m_database->find(methods, items);

    for (Item &item : items)
    {
        Log::Info_S stInfo;
        for (Element &pair : item)
        {
            std::string &key = pair.first;
            FieldValue &value = pair.second;
            switch (str2tag(key.c_str()))
            {
            case str2tag(LOG_FIELD_START_TIME):
                stInfo.startTime = mpark::get<std::string>(value);
                break;
            case str2tag(LOG_FIELD_TYPE):
                stInfo.nType = mpark::get<int>(value);
                break;
            case str2tag(LOG_FIELD_ACTION):
                stInfo.nAction = mpark::get<int>(value);
                break;
            // case str2tag(LOG_FIELD_CHN_NAME):
            //     stInfo.chnName = mpark::get<std::string>(value);
            //     break;
            case str2tag(LOG_FIELD_USER):
                stInfo.user = mpark::get<std::string>(value);
                break;
            case str2tag(LOG_FIELD_HOST):
                stInfo.host = mpark::get<std::string>(value);
                break;
            case str2tag(LOG_FIELD_CONTEXT):
                stInfo.context = mpark::get<std::string>(value);
                break;
            default:
                break;
            }
        }
        infos.push_back(stInfo);
    }
    return 0;
}

int LogDatabase::get_count(const MatchMethods &methods, int &nCount, const std::string field)
{
    if (m_database == nullptr)
    {
        return -1;
    }
    return m_database->get_count(methods, nCount, field);
}
int LogDatabase::update(const Item &item, const MatchMethods &methods)
{
    if (m_database == nullptr)
    {
        return -1;
    }
    return m_database->update(item, methods);
}

int LogDatabase::del(const Item &item)
{
    if (m_database == nullptr)
    {
        return -1;
    }
    return m_database->del(item);
}

