/**
 * @FilePath     : DbBase.cpp
 * @Author       : zjc
 * @Date         : 2022-01-02 00:00:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-13 15:03:25
 * @Description  : 数据库
 */

#include "DbBase.h"
#include <iostream>
#include <ostream>
#include <filesystem>
#include "SQLite3.hpp"
using namespace Db;

CDbBase::CDbBase(std::string path, std::string tableName)
    : m_path(path),
      m_tableName(tableName)
{
    /*目录判断*/
    std::filesystem::path filePath(path);
    std::filesystem::path dirPath = filePath.parent_path();
    /*检查目录是否存在*/
    if (std::filesystem::exists(dirPath))
    {
        if (!std::filesystem::is_directory(dirPath))
        {
            dlog_error("错误: %s 不是一个目录", dirPath.c_str());
        }
    }
    else
    {
        /*目录不存在，创建*/
        if (!std::filesystem::create_directories(dirPath))
        {
            dlog_error("错误: 无法创建目录 %s", dirPath.c_str());
        }
    }

    /* 默认添加首列：id  */
    TableKey key(DB_COMMON_FIELD_ID, CDbBase::type_int());
    m_tableKey.push_back(key);
}
CDbBase::~CDbBase()
{
    deinit();
}

void CDbBase::add_tableKey(const TableKey &key)
{
    m_tableKey.push_back(key);
}

int CDbBase::init(bool bAddDefault)
{
    if (m_tableKey.size() == 0)
    {
        return -1;
    }
    if (bAddDefault)
    {
        /* 默认添加扩展字段 */
        add_tableKey(TableKey(DB_COMMON_FIELD_RESERVE1, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        add_tableKey(TableKey(DB_COMMON_FIELD_RESERVE2, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        add_tableKey(TableKey(DB_COMMON_FIELD_RESERVE3, CDbBase::type_string(MAX_DB_STRING_SIZE)));
        add_tableKey(TableKey(DB_COMMON_FIELD_RESERVE4, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    }
    int nRet = m_sqlite3.init(m_path);
    if (nRet < 0)
    {
        dlog_error("创建数据库失败，%s", const_cast<char *>(m_path.c_str()));
        return -1;
    }

    std::string tables("CREATE TABLE IF NOT EXISTS " + m_tableName + "(");
    tables += "id INTEGER PRIMARY KEY AUTOINCREMENT,";
    for (auto &items : m_tableKey)
    {
        if (items.first == "id")
        {
            continue;
        }
        tables += items.first + " " + items.second + ",";
    }
    /* 删除最后一个逗号 */
    tables = tables.substr(0, tables.length() - 1);
    tables += ");";
    nRet = m_sqlite3.create_table(tables.c_str());
    if (nRet < 0)
    {
        dlog_error("创建数据表失败，%s", const_cast<char *>(m_path.c_str()));
        return -1;
    }

    return 0;
}
void CDbBase::deinit()
{
    m_sqlite3.deinit();
}

int CDbBase::set_sync_mode(int nMode)
{
    return m_sqlite3.set_synchronous_mode(nMode);
}

int CDbBase::add(const Item &item, std::string strTargetTableName)
{
    if (item.size() != m_tableKey.size() - 1)
    {
        dlog_error("参数错误items.size() %ld m_tableKey.size() - 1 %ld", item.size(), m_tableKey.size() - 1);
        return -1;
    }
    std::string key;
    std::string value;
    cut(item, key, value);

    std::string cmd;

    if(strTargetTableName.empty())
    {
        cmd = "INSERT INTO " + m_tableName + "(" + key + ")" + "VALUES(" + value + ");";
    }
    else 
    {
        cmd = "INSERT INTO " + strTargetTableName + "(" + key + ")" + "VALUES(" + value + ");";
    }

    int nRet = m_sqlite3.deal_sql(std::move(cmd.c_str()));
    if (nRet < 0)
    {
        return nRet;
    }
    
    return m_sqlite3.get_lastInsertId();
}

int CDbBase::del(const Item &item)
{
    std::string group;
    combiner(item, group);
    group += ";";
    std::string cmd = "DELETE FROM " + m_tableName + " WHERE " + group;
    return m_sqlite3.deal_sql(std::move(cmd.c_str()));
}
int CDbBase::del(const MatchMethods &methods, std::string strTargetTableName)
{
    std::string cmd;
    if(strTargetTableName.empty())
    {
        cmd = "DELETE FROM " + m_tableName + " WHERE " + group_methods(methods) + ";";
    }
    else 
    {
        cmd = "DELETE FROM \"" + strTargetTableName + "\" WHERE " + group_methods(methods) + ";";
    }
    
    return m_sqlite3.deal_sql(std::move(cmd.c_str()));
}


int CDbBase::update(const Item &item, const MatchMethods &methods, std::string strTargetTableName)
{
    std::string itemGroup;
    combiner(item, itemGroup);
    std::string criteriaGroup = group_methods(methods);
    criteriaGroup += ";";
    std::string cmd;
    if(strTargetTableName.empty())
    {
        cmd = "UPDATE " + m_tableName + " SET " + itemGroup + " WHERE " + criteriaGroup;
    }
    else 
    {
        cmd = "UPDATE \"" + strTargetTableName + "\" SET " + itemGroup + " WHERE " + criteriaGroup;
    }
    
    return m_sqlite3.deal_sql(std::move(cmd.c_str()));
}

int CDbBase::find(const MatchMethods &methods, std::vector<Item> &items, std::string strTargetTableName)
{
    std::string cmd;

    if(strTargetTableName.empty())
    {
        if (methods.size() == 0)
        {
            cmd += "SELECT * FROM " + m_tableName + ";";
        }
        else
        {
            cmd += "SELECT * FROM " + m_tableName + " WHERE " + group_methods(methods) + ";";
        }
    }
    else 
    {
        if (methods.size() == 0)
        {
            cmd += "SELECT * FROM " + strTargetTableName + ";";
        }
        else
        {
            cmd += "SELECT * FROM \"" + strTargetTableName + "\" WHERE " + group_methods(methods) + ";";
        }
    }


    char **pBuff = NULL;
    int nRow = 0;
    int nColumn = 0;
    m_sqlite3.get_data(std::move(cmd.c_str()), &pBuff, nRow, nColumn);
    // dlog_info("cmd[%s] nrow[%d] nColumn[%d]", cmd.c_str(), nRow, nColumn);
    int nPos = 0;
    int nNamePos = 0;
    for (int i = 0; i < nRow; i++)
    {
        Item oItem;
        nPos = (i + 1) * nColumn; /* 每行的第一个下标 */
        nNamePos = 0;             /* 每一行的表头 */
        for (int j = 0; j < nColumn; j++)
        {
            oItem.push_back(Element(pBuff[nNamePos], to_value(pBuff[nNamePos], pBuff[nPos])));
            nNamePos++;
            nPos++;
        }
        items.push_back(oItem);
    }

    if (pBuff)
    {
        m_sqlite3.release_data(pBuff);
        pBuff = nullptr;
    }

    return  0;
}

int CDbBase::find(const std::string cmd, std::vector<Item> &items)
{
    char **pBuff = NULL;
    int nRow = 0;
    int nColumn = 0;
    m_sqlite3.get_data(std::move(cmd.c_str()), &pBuff, nRow, nColumn);
    dlog_info("cmd[%s] nrow[%d] nColumn[%d]", cmd.c_str(), nRow, nColumn);
    int nPos = 0;
    int nNamePos = 0;
    for (int i = 0; i < nRow; i++)
    {
        Item oItem;
        nPos = (i + 1) * nColumn; /* 每行的第一个下标 */
        nNamePos = 0;             /* 每一行的表头 */
        for (int j = 0; j < nColumn; j++)
        {
            oItem.push_back(Element(pBuff[nNamePos], to_value(pBuff[nNamePos], pBuff[nPos])));
            nNamePos++;
            nPos++;
        }
        items.push_back(oItem);
    }

    if (pBuff)
    {
        m_sqlite3.release_data(pBuff);
        pBuff = nullptr;
    }

    return  0;
}

int CDbBase::get_count(const MatchMethods &methods, int &nCount, const std::string field, std::string strTargetTableName)
{
    std::string cmd;
    if(strTargetTableName.empty())
    {
        if (methods.size() == 0)
        {
            cmd += "SELECT COUNT("+ field + ") FROM " + m_tableName + ";";
        }
        else
        {
            cmd += "SELECT COUNT("+ field + ") FROM " + m_tableName + " WHERE " + group_methods(methods) + ";";
        }
    }
    else 
    {
        if (methods.size() == 0)
        {
            cmd += "SELECT COUNT("+ field + ") FROM " + strTargetTableName + ";";
        }
        else
        {
            cmd += "SELECT COUNT("+ field + ") FROM \"" + strTargetTableName + "\" WHERE " + group_methods(methods) + ";";
        }
    }

    char **pBuff = NULL;
    int nRow = 0;
    int nColumn = 0;
    m_sqlite3.get_data(std::move(cmd.c_str()), &pBuff, nRow, nColumn);
    if (!pBuff)
    {
        return -1;
    }
    if (nRow > 0 && nColumn > 0)
    {
        nCount = atoi(pBuff[1]);
    }
    m_sqlite3.release_data(pBuff);
    pBuff = nullptr;
    return  0;
}


int CDbBase::get_avg(const std::string field, const MatchMethods &methods, int &nAvg)
{
    std::string cmd;
    if (methods.size() == 0)
    {
        cmd += "SELECT AVG("+ field + ") FROM " + m_tableName + ";";
    }
    else
    {
        cmd += "SELECT AVG("+ field + ") FROM " + m_tableName + " WHERE " + group_methods(methods) + ";";
    }
    char **pBuff = NULL;
    int nRow = 0;
    int nColumn = 0;
    m_sqlite3.get_data(std::move(cmd.c_str()), &pBuff, nRow, nColumn);
    if (!pBuff)
    {
        return -1;
    }
    nAvg = atoi(pBuff[1]);
    m_sqlite3.release_data(pBuff);
    return  0;
}
int CDbBase::get_sum(const std::string field, const MatchMethods &methods, int &nSum)
{
    std::string cmd;
    if (methods.size() == 0)
    {
        cmd += "SELECT SUM("+ field + ")FROM " + m_tableName + ";";
    }
    else
    {
        cmd += "SELECT SUM("+ field + ") FROM " + m_tableName + " WHERE " + group_methods(methods) + ";";
    }
    char **pBuff = NULL;
    int nRow = 0;
    int nColumn = 0;
    m_sqlite3.get_data(std::move(cmd.c_str()), &pBuff, nRow, nColumn);
    if (!pBuff)
    {
        return -1;
    }
    nSum = atoi(pBuff[1]);
    m_sqlite3.release_data(pBuff);
    return  0;
}
void CDbBase::print(const std::vector<Item> &items)
{
    dlog_info("打印数据");
    for (auto &item : items)
    {
        for (auto &elem : item)
        {
            auto &key = elem.first;
            auto &value = elem.second;

            std::cout << key << " " << to_string(value) << " ";
        }
        std::cout << std::endl;
    }
}
std::string Db::CDbBase::get_tableName() const
{
    return m_tableName;
}
std::string CDbBase::group_methods(const MatchMethods &methods)
{
    if (methods.size() == 0)
    {
        return std::string();
    }
    std::string group;
    Criterion_E enLastAndOr = FIND_CRITERION_NONE;
    for (auto &i : methods)
    {
        /* value值起始结束符 */
        std::string start = "'";
        std::string end = "'";
        if (i.enCriterion == FIND_CRITERION_LIKE)
        {
            start = "'%";
            end = "%'";
        }
        else if (i.enCriterion == FIND_CRITERION_NONE)
        {
            start = "";
            end = "";
        }
        /* 括号起始结束 */
        std::string startP("");
        std::string endP("");
        if (i.enAndOr == FIND_CRITERION_OR_P && enLastAndOr != FIND_CRITERION_OR_P)
        {
            startP = "(";

        }
        else if (i.enAndOr != FIND_CRITERION_OR_P && enLastAndOr == FIND_CRITERION_OR_P)
        {
            endP = ")";
        }
        /* 拼接，左括号+ 字段+条件+值起始符+值+值结束符 +右括号+与下一个条件关系 */
        group += startP + i.elem.first + to_string(i.enCriterion) + start + to_string(i.elem.second) + end + endP + to_string(i.enAndOr);
        enLastAndOr = i.enAndOr;
    }
    return group;
}

std::string CDbBase::to_string(const FieldValue value)
{
    std::string outValue;
    if (mpark::holds_alternative<int>(value))
    {
        int nValue = mpark::get<int>(value);
        outValue = std::to_string(nValue);

    }
    else if (mpark::holds_alternative<std::string>(value))
    {
        outValue = mpark::get<std::string>(value);
    }
    return outValue;
}

FieldValue CDbBase::to_value(const std::string ikey, const std::string value)
{
    FieldValue outValue;
    for (auto &pair : m_tableKey)
    {
        std::string &key = pair.first;
        std::string &valueType = pair.second;
        if (ikey == key)
        {
            if (to_type(valueType) == VALUE_TYPE_INT)
            {
                outValue = atoi(value.c_str());
            }
            else if (to_type(valueType) == VALUE_TYPE_STR)
            {
                outValue = value.c_str();
            }
            break;
        }
    }
    return outValue;
}

std::string CDbBase::to_string(const Criterion_E enCriterion)
{
    std::string criterion;
    switch (enCriterion)
    {
    case FIND_CRITERION_NONE:
        criterion = " ";
        break;
    case FIND_CRITERION_EQ:
        criterion = " = ";
        break;
    case FIND_CRITERION_NE:
        criterion = " != ";
        break;
    case FIND_CRITERION_GT:
        criterion = " > ";
        break;
    case FIND_CRITERION_GE:
        criterion = " >= ";
        break;
    case FIND_CRITERION_IT:
        criterion = " < ";
        break;
    case FIND_CRITERION_IE:
        criterion = " <= ";
        break;
    case FIND_CRITERION_AND:
        criterion = " AND ";
        break;
    case FIND_CRITERION_OR:
    case FIND_CRITERION_OR_P:
        criterion = " OR ";
        break;
    case FIND_CRITERION_LIKE:
        criterion = " LIKE ";
        break;
    default:
        criterion = " ";
        break;
    }
    return criterion;
}
void CDbBase::cut(const Item &item, std::string &key, std::string &value)
{
    for (auto &i : item)
    {
        key += i.first + ",";
        value += "\"" + to_string(i.second) + "\",";
    }
    /* 删除最后一个逗号 */
    key = key.substr(0, key.length() - 1);
    value = value.substr(0, value.length() - 1);
}

void CDbBase::combiner(const Item &item, std::string &group)
{
    for (auto &i : item)
    {
        group += i.first + " = '" + to_string(i.second) + "',";
    }
    /* 删除最后一个逗号 */
    group = group.substr(0, group.length() - 1);
}
std::string CDbBase::type_int()
{
    return "int";
}

std::string CDbBase::type_string(size_t nSize)
{
    return "varchar(" + std::to_string(nSize) + ")";
}

FieldVelueType_E CDbBase::to_type(const std::string valueType)
{
    if (valueType.find("int") != std::string::npos)
    {
        return VALUE_TYPE_INT;
    }
    else if (valueType.find("varchar") != std::string::npos)
    {
        return VALUE_TYPE_STR;
    }
    return VALUE_TYPE_STR;
}

std::vector<std::string> CDbBase::get_all_tables()
{
    const char *sql = "SELECT name FROM sqlite_master WHERE type='table'";
    return m_sqlite3.get_all_tables(sql);
}

std::vector<std::string> CDbBase::get_column_data(const std::string &tableName, const std::string &targetField, int nMaxRows)
{
    // 构造SQL语句
    std::string sql = "SELECT \"" + targetField + "\" FROM \"" + tableName + "\"";

    // LIMIT子句
    if (nMaxRows > 0)
    {
        sql += " LIMIT " + std::to_string(nMaxRows);
    }
    sql += ";";

    return m_sqlite3.get_column_data(sql);
}

bool CDbBase::delete_record_by_field(const std::string &tableName, const std::string &targetFile, const std::string &targetField)
{
    std::string sql = "DELETE FROM \"" + tableName + "\" WHERE \"" + targetField + "\" = ?;";
    return m_sqlite3.delete_record_by_field(sql, targetFile);
}

int CDbBase::get_table_data_count(const std::string &tableName)
{
    std::string sql = "SELECT COUNT(*) FROM \"" + tableName + "\";";
    return m_sqlite3.get_table_data_count(sql);
}

int CDbBase::del_table(const std::string &tableName)
{
    //校验表名不能为空且不含分号
    if (tableName.empty() || tableName.find(';') != std::string::npos)
    {
        dlog_error("非法表名\n");
        return false;
    }

    std::string sql = "DROP TABLE IF EXISTS \"" + tableName + "\";";

    return m_sqlite3.deal_sql(sql);
    // return m_sqlite3.del_table(sql);
}

int CDbBase::clear_table(const std::string &tableName, bool resetAutoInc)
{
    if (tableName.empty())
    {
        dlog_error("传入表名为空");
        return -1;
    }

    int nRet = 0;

    /* 1. 清空表数据 */ 
    std::string sql = "DELETE FROM \"" + tableName + "\";";
    nRet = m_sqlite3.deal_sql(sql);

    /*  2. 是否重置 AUTOINCREMENT */
    if (resetAutoInc)
    {
        std::string resetSql = "DELETE FROM sqlite_sequence WHERE name='" + tableName + "';";
        /* 失败并不致命（可能表没 AUTOINCREMENT） */ 
        m_sqlite3.deal_sql(resetSql);
    }
        
    return nRet;
}
