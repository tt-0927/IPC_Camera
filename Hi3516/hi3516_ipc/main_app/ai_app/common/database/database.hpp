/**
 * @FilePath     : database.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-06-06 16:50:02
 * @Description  : 
 */
#pragma once

#include <iostream>

#include "IpcRet.h"
#include "dlog.h"
#include "sqlite3.h"


/**
 * @brief: 检查表是否存在
 * @param [string] : 表名
 */
#define SQL_CHECK_TABLE_EXIT ("SELECT name FROM sqlite_master WHERE type='table' AND name='%s';")

/**
 * @brief: 获取表信息
 * @param [string] : 表名
 */
#define SQL_PRAGMA_TABLE_INFO ("PRAGMA table_info(%s);")

/**
 * @brief: 获取数据表的数据总数
 * @param [string] : 表名
 */
#define SQL_GET_TOTAL ("SELECT COUNT(*) FROM %s;")

/**
 * @brief: 追加字段
 * @param [string] : 表名
 * @param [string] : 字段名
 * @param [string] : 字段类型
 */
#define SQL_ALTER_TABLE_ADD_COLUMN ("ALTER TABLE %s ADD COLUMN '%s' '%s';")

class CDatabase
{

public:

    CDatabase();
    ~CDatabase();

protected:

    /**
     * @brief: 错误打印函数
     * @param [char*] achCmd: 打印数据
     * @param [char**] pchErrMsg: 错误数据
     * @return [*]
     * @note:
     */
    bool db_print_errMsg(const char* achCmd, char* pchErrMsg);

    /**
     * @brief: 执行事务
     * @param [char*] pchSelectSQL: 命令
     * @return [*]
     * @note:
     */
    bool exec_transaction(const std::string& strSelectSQL);


    /**
     * @brief: 数据库备份
     * @param [sqlite3*] pSq: 需要备份的数据库句柄
     * @param [char*] pchDbBackupName: 备份的数据库名
     * @param [bool] bIsSave: true-备份数据 false-还原数据
     * @return [*]
     * @note:
     */
    bool backup_database(sqlite3* pSq, const char* pchDbBackupName, bool bIsSave);

    /**
     * @brief: 检查数据表是否存在
     * @param [char*] pchTableName: 表名
     * @return [*] 成功 >= IpcRet_E::OK   其他失败
     * @note:
     */
    IpcRet_E check_tableExist(const char* pchTableName);

    /**
     * @brief: 创建数据表
     * @param [char*] pchTableName: 表名
     * @param [char*] pchSqlStr: 创建表的SQL命令
     * @return [*]
     * @note:
     */
    bool create_dataTable(const char* pchTableName, const char* pchSqlStr);

    /**
     * @brief  检查字段是否存在，不存在追加
     * @param  [char*] pchTableName 表格名称
     * @param  [char*] pchFieldName 字段名
     * @param  [char*] pchFieldType
     * @return [*] 成功 >= IpcRet_E::OK   其他失败
     * @note
     */
    IpcRet_E check_fieldExist(
        const char* pchTableName,
        const char* pchFieldName,
        const char* pchFieldType);

    /**
     * @brief 开始事务
     * @return [*]
     * @note
     */
    bool beginTransaction();

    /**
     * @brief 执行事务
     * @return [*]
     * @note
     */
    bool commitTransaction();

    /**
     * @brief 回滚事务
     * @return [*]
     * @note
     */
    bool rollbackTransaction();

protected:

    sqlite3* m_pDb = nullptr;
};
