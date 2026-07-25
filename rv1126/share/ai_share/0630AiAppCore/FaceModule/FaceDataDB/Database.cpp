/*
 * @FilePath     : Database.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-09-25 15:04:39
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-26 09:17:43
 * @Description  :
 */
#include "Database.hpp"

#include <cstring>
#include <fstream>
#include <vector>

using namespace Ai0630_NS;

Database::Database()
{
}

Database::~Database()
{
}

BlError_E Database::clearAllTables()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (NULL == m_pDb)
    {
        return ERR_UNINIT;
    }

    BlError_E                enRetCode    = OK;
    sqlite3_stmt*            pstCountstmt = NULL;
    /* 用于缓存表名 */
    std::vector<std::string> tableNames;

    /* 1. 获取所有表名 */
    const char* pchSqlCmd = "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%';";
    if (sqlite3_prepare_v2(m_pDb, pchSqlCmd, -1, &pstCountstmt, nullptr) != SQLITE_OK)
    {
        return NOK;
    }

    while (sqlite3_step(pstCountstmt) == SQLITE_ROW)
    {
        const char* pTableName = (const char*)sqlite3_column_text(pstCountstmt, 0);
        if (pTableName)
        {
            dlog(LOG_INFO, "【待清空的数据表】 %s", pTableName);
            tableNames.push_back(pTableName);
        }
    }
    /* 关键点：立即释放查询句柄，解除对 sqlite_master 的锁定 */
    sqlite3_finalize(pstCountstmt);
    pstCountstmt = NULL;

    /* 2. 执行删除操作 */
    sqlite3_exec(m_pDb, "PRAGMA foreign_keys = OFF;", nullptr, nullptr, nullptr);

    if (beginTransaction())
    {
        try
        {
            for (const auto& name : tableNames)
            {
                std::string strDropSql = "DROP TABLE IF EXISTS \"" + name + "\";";
                char*       pchErrMsg  = nullptr;
                if (sqlite3_exec(m_pDb, strDropSql.c_str(), nullptr, nullptr, &pchErrMsg) != SQLITE_OK)
                {
                    if (pchErrMsg)
                    {
                        dlog(LOG_ERROR, "DROP Error: %s", pchErrMsg);
                        sqlite3_free(pchErrMsg);
                    }
                    continue;
                }
            }
            commitTransaction();
            enRetCode = OK;
        }
        catch (...)
        {
            rollbackTransaction();
            enRetCode = NOK;
        }
    }

    return enRetCode;
}

// /* 删除全部表 */
// BlError_E Database::clearAllTables()
// {
//     /* 自动锁定互斥锁 */
//     std::unique_lock<std::mutex> lock(m_mutex);

//     if (NULL == m_pDb)
//     {
//         dlog(LOG_ERROR, "未初始化");
//         return ERR_UNINIT;
//     }

//     BlError_E     enRetCode    = OK;
//     char*         pchErrMsg    = nullptr;
//     sqlite3_stmt* pstCountstmt = NULL;

//     /* 关闭外键，否则无法 DROP */
//     sqlite3_exec(m_pDb, "PRAGMA foreign_keys = OFF;", nullptr, nullptr, nullptr);

//     if (beginTransaction())
//     {
//         try
//         {
//             const char* pchSqlCmd = "SELECT name FROM sqlite_master WHERE type='table';";

//             /* 编译SQL语句 */
//             if (sqlite3_prepare_v2(m_pDb, pchSqlCmd, -1, &pstCountstmt, nullptr) != SQLITE_OK)
//             {
//                 throw std::runtime_error("编译SQL语句-失败");
//             }

//             while (sqlite3_step(pstCountstmt) == SQLITE_ROW)
//             {
//                 const char* pTableName = (const char*)sqlite3_column_text(pstCountstmt, 0);

//                 if (!pTableName)
//                 {
//                     continue;
//                 }

//                 std::string strDropSql  = "DROP TABLE IF EXISTS \"";
//                 strDropSql             += pTableName;
//                 strDropSql             += "\";";

//                 if (sqlite3_exec(m_pDb, strDropSql.c_str(), nullptr, nullptr, &pchErrMsg) != SQLITE_OK)
//                 {
//                     db_print_errMsg(pchSqlCmd, pchErrMsg);
//                     pchErrMsg = NULL;
//                     continue;
//                 }
//             }

//             /* 提交事务 */
//             commitTransaction();
//             enRetCode = OK;
//         }
//         catch (const std::exception& e)
//         {
//             dlog(LOG_ERROR, "删除全部表 [%s]\n%s", e.what(), sqlite3_errmsg(m_pDb));
//             rollbackTransaction();
//             enRetCode = NOK;
//         }
//     }

//     if (pstCountstmt)
//     {
//         sqlite3_finalize(pstCountstmt);
//         pstCountstmt = NULL;
//     }

//     if (pchErrMsg)
//     {
//         sqlite3_free(pchErrMsg);
//         pchErrMsg = NULL;
//     }
//     return enRetCode;
// }

/* 删除单个表 */
BlError_E Ai0630_NS::Database::deleteTable(std::string strTabName)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    BlError_E     enRetCode    = OK;
    char*         pchDeleteSQL = nullptr;
    sqlite3_stmt* pDeleteStmt  = nullptr;

    if (beginTransaction())
    {
        try
        {
            /* 准备删除表的 SQL 语句 */
            pchDeleteSQL = sqlite3_mprintf("DROP TABLE IF EXISTS \"%s\";", strTabName.c_str());

            if (sqlite3_prepare_v2(m_pDb, pchDeleteSQL, -1, &pDeleteStmt, nullptr) == SQLITE_OK)
            {
                /* 执行 SQL 语句 */
                if (sqlite3_step(pDeleteStmt) != SQLITE_DONE)
                {
                    throw std::runtime_error("删除表失败");
                }
            }
            else
            {
                throw std::runtime_error("编译删除表语句失败");
            }

            /* 提交事务 */
            commitTransaction();
            enRetCode = OK;
        }
        catch (const std::exception& e)
        {
            dlog(LOG_ERROR, "删除单个表 [%s]\n%s", e.what(), sqlite3_errmsg(m_pDb));
            rollbackTransaction();
            enRetCode = NOK;
        }
    }

    /* 清理和释放 */
    if (pDeleteStmt)
    {
        sqlite3_finalize(pDeleteStmt);
    }

    if (pchDeleteSQL)
    {
        sqlite3_free(pchDeleteSQL);
    }

    return enRetCode;
}

/* 删除数据 */
BlError_E Database::deleteData(int nId)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    char*         pchDeleteSQL       = nullptr;
    sqlite3_stmt* pstCountstmt       = nullptr;
    const char*   pchSelectTablesSQL = "SELECT name FROM sqlite_master WHERE type='table';";

    BlError_E enRetCode = OK;

    if (beginTransaction())
    {
        try
        {
            /* 获取所有表名 */
            if (sqlite3_prepare_v2(m_pDb, pchSelectTablesSQL, -1, &pstCountstmt, nullptr) != SQLITE_OK)
            {
                throw std::runtime_error("获取表名失败");
            }

            while (sqlite3_step(pstCountstmt) == SQLITE_ROW)
            {
                const char* strTabName = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 0));

                /* 拼接删除语句 */
                pchDeleteSQL = sqlite3_mprintf("DELETE FROM \"%s\" WHERE ID = ?;", strTabName);
                if (pchDeleteSQL == nullptr)
                {
                    throw std::runtime_error("创建删除命令失败");
                }

                sqlite3_stmt* pDeleteStmt = nullptr;

                /* 编译删除 SQL 语句 */
                if (sqlite3_prepare_v2(m_pDb, pchDeleteSQL, -1, &pDeleteStmt, nullptr) == SQLITE_OK)
                {
                    /* 绑定要删除的 ID */
                    sqlite3_bind_int(pDeleteStmt, 1, nId);

                    if (sqlite3_step(pDeleteStmt) != SQLITE_DONE)
                    {
                        dlog(LOG_ERROR, "删除数据失败: %s", sqlite3_errmsg(m_pDb));
                    }
                }
                else
                {
                    throw std::runtime_error("编译删除语句失败");
                }

                /* 清理和释放 */
                if (pDeleteStmt)
                {
                    sqlite3_finalize(pDeleteStmt);
                }
            }

            /* 提交事务 */
            commitTransaction();
            enRetCode = OK;
        }
        catch (const std::exception& e)
        {
            dlog(LOG_ERROR, "删除数据失败 [%s]", e.what());
            rollbackTransaction();
            enRetCode = NOK;
        }
    }

    if (pchDeleteSQL)
    {
        sqlite3_free(pchDeleteSQL);
    }

    /* 清理表名查询语句 */
    if (pstCountstmt)
    {
        sqlite3_finalize(pstCountstmt);
    }


    return enRetCode;
}

/* 获取数据总量 */
BlError_E Database::getDataTotal(int& nOutTotal)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    sqlite3_stmt* pstCountstmt = NULL;

    const char* pchSelectTablesSQL = "SELECT name FROM sqlite_master WHERE type='table';";
    BlError_E   enRetCode          = OK;

    nOutTotal = 0;    // 初始化总数

    /* 获取所有表名 */
    if (sqlite3_prepare_v2(m_pDb, pchSelectTablesSQL, -1, &pstCountstmt, NULL) == SQLITE_OK)
    {
        while (sqlite3_step(pstCountstmt) == SQLITE_ROW)
        {
            const char* strTabName = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 0));

            /* 对每个表名执行 COUNT(*) 查询 */
            char*         pchCountSQL   = sqlite3_mprintf("SELECT COUNT(*) FROM \"%s\";", strTabName);
            sqlite3_stmt* pstSelectStmt = NULL;

            if (sqlite3_prepare_v2(m_pDb, pchCountSQL, -1, &pstSelectStmt, NULL) == SQLITE_OK)
            {
                if (sqlite3_step(pstSelectStmt) == SQLITE_ROW)
                {
                    nOutTotal += sqlite3_column_int(pstSelectStmt, 0);    // 累加行数
                }
                else
                {
                    dlog(LOG_ERROR, "执行 COUNT 查询失败 %s", sqlite3_errmsg(m_pDb));
                    enRetCode = NOK;
                }
            }
            else
            {
                dlog(LOG_ERROR, "准备 COUNT 查询语句失败 %s", sqlite3_errmsg(m_pDb));
                enRetCode = NOK;
            }

            /* 清理和释放 */
            if (pstSelectStmt)
            {
                sqlite3_finalize(pstSelectStmt);
            }

            if (pchCountSQL)
            {
                sqlite3_free(pchCountSQL);
            }
        }
    }
    else
    {
        dlog(LOG_ERROR, "准备获取表名失败 %s", sqlite3_errmsg(m_pDb));
        enRetCode = NOK;
    }

    if (pstCountstmt)
    {
        sqlite3_finalize(pstCountstmt);
    }

    return enRetCode;
}

/* 错误打印函数 */
bool Database::db_print_errMsg(const char* achCmd, char* pchErrMsg)
{
    if (NULL == achCmd ||
        NULL == pchErrMsg)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return false;
    }
    dlog(LOG_ERROR, "错误打印=%s  <---> Error=%s", achCmd, pchErrMsg);

    sqlite3_free(pchErrMsg);
    pchErrMsg = NULL;

    return true;
}

/* 执行事务 */
bool Database::exec_transaction(const std::string& strSelectSQL)
{
    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return false;
    }

    int   nRet      = 0;
    bool  bRet      = true;
    char* pchErrMsg = NULL;

    /* 开始一个事务的SQL语句 */
    nRet = sqlite3_exec(m_pDb, "begin transaction", NULL, NULL, &pchErrMsg);
    if (nRet != SQLITE_OK)
    {
        /* 错误打印 */
        db_print_errMsg(strSelectSQL.c_str(), pchErrMsg);
        pchErrMsg = NULL;
        bRet      = false;
        goto EXIT;
    }
    nRet = sqlite3_exec(m_pDb, strSelectSQL.c_str(), NULL, NULL, &pchErrMsg);
    if (nRet != SQLITE_OK)
    {
        /* 错误打印 */
        db_print_errMsg(strSelectSQL.c_str(), pchErrMsg);
        pchErrMsg = NULL;

        /* 异常，回滚，撤销失败的动作 */
        nRet = sqlite3_exec(m_pDb, "rollback transaction", NULL, NULL, &pchErrMsg);
        if (nRet != SQLITE_OK)
        {
            /* 错误打印 */
            db_print_errMsg(strSelectSQL.c_str(), pchErrMsg);
            pchErrMsg = NULL;
            bRet      = false;
            goto EXIT;
        }
        bRet = false;
        goto EXIT;
    }

    /* 提交事务 */
    nRet = sqlite3_exec(m_pDb, "commit transaction", NULL, NULL, &pchErrMsg);
    if (nRet != SQLITE_OK)
    {
        /* 错误打印 */
        db_print_errMsg(strSelectSQL.c_str(), pchErrMsg);
        pchErrMsg = NULL;
        bRet      = false;
        goto EXIT;
    }

EXIT:
    if (pchErrMsg)
    {
        sqlite3_free(pchErrMsg);
        pchErrMsg = NULL;
    }

    return bRet;
}

/* 数据库备份 */
bool Database::backup_database(sqlite3* pSq, const char* pchDbBackupName, bool bIsSave)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == pSq || NULL == pchDbBackupName)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return false;
    }

    /* 函数返回值的存储 */
    int             nRet        = 0;
    /* 存储备份数据库文件指针 */
    sqlite3*        pFile       = NULL;
    /* 备份操作指针 */
    sqlite3_backup* pBackup     = NULL;
    /* 指向目标数据库句柄指针 */
    sqlite3*        pTo         = NULL;
    /* 指向源数据库句柄指针 */
    sqlite3*        pFrom       = NULL;
    /* 用于存储备份数据库存在值 */
    int             nHaveBackup = 0;

    std::ifstream fileStream(pchDbBackupName);

    if (fileStream.good())
    {
        nHaveBackup = 1;
        dlog(LOG_INFO, "备份-记录数据库存在[%s]", pchDbBackupName);
    }
    else
    {
        dlog(LOG_ERROR, "备份-记录数据库不存在[%s]", pchDbBackupName);
    }

    if (!nHaveBackup && 0 == bIsSave)
    {
        dlog(LOG_ERROR, "没有备份数据库，无法恢复数据库");
        return true;
    }

    nRet = sqlite3_open(pchDbBackupName, &pFile);
    if (nRet == SQLITE_OK)
    {
        pFrom   = (bIsSave ? pSq : pFile);
        pTo     = (bIsSave ? pFile : pSq);
        /* 备份process创建备份对象 */
        pBackup = sqlite3_backup_init(pTo, "main", pFrom, "main");
        if (pBackup)
        {
            /* 拷贝数据 */
            (void)sqlite3_backup_step(pBackup, -1);
            /* 释放资源 */
            (void)sqlite3_backup_finish(pBackup);
        }
        /* 若拷贝的过程中出现任何错误,该函数可以获取具体的错误码 */
        nRet = sqlite3_errcode(pTo);
    }

    if (bIsSave)
    {
        dlog(LOG_INFO, "备份数据成功");
    }
    else
    {
        dlog(LOG_INFO, "还原数据成功");
    }


    (void)sqlite3_close(pFile);

    return true;
}

/* 检查数据表是否存在 */
BlError_E Database::check_tableExist(const char* pchTableName)
{
    if (NULL == pchTableName)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_PARAM;
    }

    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    int           nTableExists    = 0;
    char*         pchCheckItemSql = NULL;
    sqlite3_stmt* pstCountstmt    = NULL;
    int           nRet            = 0;
    BlError_E     enRetCode       = OK;

    /* 检查数据项是否存在 */
    pchCheckItemSql = sqlite3_mprintf(SQL_CHECK_TABLE_EXIT, pchTableName);
    if (pchCheckItemSql == NULL)
    {
        dlog(LOG_ERROR, "创建命令失败");
        enRetCode = NOK;
        goto EXIT;
    }

    /* 编译SQL语句 */
    if (sqlite3_prepare_v2(m_pDb, pchCheckItemSql, -1, &pstCountstmt, NULL) == SQLITE_OK)
    {
        /* 默认不存在 */
        enRetCode = OK_NOT_EXIST;
        /* 执行循环以获取结果行 */
        while (sqlite3_step(pstCountstmt) == SQLITE_ROW)
        {
            /* 存在 */
            enRetCode = OK_EXIST;
            break;
        }
    }
    else
    {
        dlog(LOG_ERROR, "编译SQL语句-失败");
        enRetCode = NOK;
        goto EXIT;
    }



EXIT:

    if (pstCountstmt)
    {
        sqlite3_finalize(pstCountstmt);
        pstCountstmt = NULL;
    }

    if (pchCheckItemSql)
    {
        sqlite3_free(pchCheckItemSql);
        pchCheckItemSql = NULL;
    }


    return enRetCode;
}

/* 创建数据表 */
bool Database::create_dataTable(const char* pchTableName, const char* pchSqlStr)
{
    if (NULL == pchTableName || NULL == pchSqlStr)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return false;
    }

    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "未初始化");
        return false;
    }

    char* pchSqlCmd = NULL;

    pchSqlCmd = sqlite3_mprintf(pchSqlStr, pchTableName);
    if (pchSqlCmd == NULL)
    {
        dlog(LOG_ERROR, "创建命令失败");
        return false;
    }

    if (!exec_transaction(pchSqlCmd))
    {
        dlog(LOG_ERROR, "创建数据表-失败");
    }

    if (pchSqlCmd)
    {
        sqlite3_free(pchSqlCmd);
        pchSqlCmd = NULL;
    }

    return true;
}

/* 检查字段是否存在，不存在追加 */
BlError_E Database::check_fieldExist(
    const char* pchTableName,
    const char* pchFieldName,
    const char* pchFieldType)
{
    if (NULL == pchFieldName || NULL == pchTableName || NULL == pchFieldType)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_PARAM;
    }

    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    int           nCount          = 0;
    char*         pchCheckItemSql = NULL;
    char*         pchAddColumnSql = NULL;
    sqlite3_stmt* pstCountstmt    = NULL;
    BlError_E     enRetCode       = OK;

    /* 检查数据项是否存在 */
    pchCheckItemSql = sqlite3_mprintf(SQL_PRAGMA_TABLE_INFO, pchTableName);
    if (pchCheckItemSql == NULL)
    {
        dlog(LOG_ERROR, "创建命令失败");
        enRetCode = NOK;
        goto EXIT;
    }

    /* 编译SQL语句 */
    if (sqlite3_prepare_v2(m_pDb, pchCheckItemSql, -1, &pstCountstmt, NULL) == SQLITE_OK)
    {
        /* 默认不存在 */
        enRetCode = OK_NOT_EXIST;
        /* 执行循环以获取结果行 */
        while (sqlite3_step(pstCountstmt) == SQLITE_ROW)
        {
            const unsigned char* pchName = sqlite3_column_text(pstCountstmt, 1);
            if (pchName != NULL)
            {
                /* 判断字段名是否存在 */
                if (strcmp(pchFieldName, (const char*)pchName) == 0)
                {
                    enRetCode = OK_EXIST;
                    break;
                }
            }
        }
    }
    else
    {
        dlog(LOG_ERROR, "编译SQL语句-失败");
        enRetCode = NOK;
        goto EXIT;
    }

    /* 存在 */
    if (enRetCode == OK_EXIST)
    {
        dlog(LOG_INFO, "字段[%s]存在", pchFieldName);
        goto EXIT;
    }
    /* 不存在 */
    else if (enRetCode == OK_NOT_EXIST)
    {
        dlog(LOG_INFO, "字段[%s]不存在，追加字段", pchFieldName);

        /* 字段不存在，执行追加字段操作 */
        pchAddColumnSql = sqlite3_mprintf(SQL_ALTER_TABLE_ADD_COLUMN,
                                          pchTableName,
                                          pchFieldName,
                                          pchFieldType);

        if (!exec_transaction(pchAddColumnSql))
        {
            dlog(LOG_ERROR, "执行事务-失败");
        }
    }

EXIT:
    if (pchCheckItemSql)
    {
        sqlite3_free(pchCheckItemSql);
        pchCheckItemSql = NULL;
    }
    if (pchAddColumnSql)
    {
        sqlite3_free(pchAddColumnSql);
        pchAddColumnSql = NULL;
    }

    if (pstCountstmt)
    {
        sqlite3_finalize(pstCountstmt);
        pstCountstmt = NULL;
    }

    return enRetCode;
}

/* 开始事务 */
bool Database::beginTransaction()
{
    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return false;
    }

    bool  bRet      = true;
    int   nRet      = 0;
    char* pchErrMsg = NULL;

    /* 开始一个事务的SQL语句 */
    nRet = sqlite3_exec(m_pDb, "BEGIN TRANSACTION;", NULL, NULL, &pchErrMsg);
    if (nRet != SQLITE_OK)
    {
        /* 错误打印 */
        db_print_errMsg("开始事务-失败", pchErrMsg);
        pchErrMsg = NULL;
        bRet      = false;
        goto EXIT;
    }

EXIT:

    if (pchErrMsg)
    {
        sqlite3_free(pchErrMsg);
        pchErrMsg = NULL;
    }

    return bRet;
}

/* 执行事务 */
bool Database::commitTransaction()
{
    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return false;
    }

    bool  bRet      = true;
    int   nRet      = 0;
    char* pchErrMsg = NULL;

    /* 提交事务 */
    nRet = sqlite3_exec(m_pDb, "COMMIT TRANSACTION;", NULL, NULL, &pchErrMsg);
    if (nRet != SQLITE_OK)
    {
        /* 错误打印 */
        db_print_errMsg("提交事务-失败", pchErrMsg);
        pchErrMsg = NULL;
        bRet      = false;
        goto EXIT;
    }

EXIT:

    if (pchErrMsg)
    {
        sqlite3_free(pchErrMsg);
        pchErrMsg = NULL;
    }

    return bRet;
}

/* 回滚事务 */
bool Database::rollbackTransaction()
{
    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return false;
    }

    bool  bRet      = true;
    int   nRet      = 0;
    char* pchErrMsg = NULL;

    /* 异常，回滚，撤销 */
    nRet = sqlite3_exec(m_pDb, "ROLLBACK TRANSACTION;", NULL, NULL, &pchErrMsg);
    if (nRet != SQLITE_OK)
    {
        /* 错误打印 */
        db_print_errMsg("回滚事务-失败", pchErrMsg);
        pchErrMsg = NULL;
        bRet      = false;
        goto EXIT;
    }

EXIT:

    if (pchErrMsg)
    {
        sqlite3_free(pchErrMsg);
        pchErrMsg = NULL;
    }

    return bRet;
}
