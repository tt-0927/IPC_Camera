/**
 * @FilePath     : database.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-06-06 17:00:12
 * @Description  : 
 */

#include "database.hpp"

#include <fstream>
#include <cstring>

CDatabase::CDatabase()
{
}

CDatabase::~CDatabase()
{
}

/* 错误打印函数 */
bool CDatabase::db_print_errMsg(const char* achCmd, char* pchErrMsg)
{
    if (NULL == achCmd ||
        NULL == pchErrMsg)
    {
        dlog_error("传入参数为空");
        return false;
    }
    dlog_error("错误打印=%s  <---> Error=%s", achCmd, pchErrMsg);

    sqlite3_free(pchErrMsg);
    pchErrMsg = NULL;

    return true;
}

/* 执行事务 */
bool CDatabase::exec_transaction(const std::string& strSelectSQL)
{
    if (NULL == m_pDb)
    {
        dlog_error("传入参数为空");
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
        bRet = false;
        goto EXIT;
    }
    nRet = sqlite3_exec(m_pDb, strSelectSQL.c_str(), NULL, NULL, &pchErrMsg);
    if (nRet != SQLITE_OK)
    {
        /* 错误打印 */
        db_print_errMsg(strSelectSQL.c_str(), pchErrMsg);

        /* 异常，回滚，撤销失败的动作 */
        nRet = sqlite3_exec(m_pDb, "rollback transaction", NULL, NULL, &pchErrMsg);
        if (nRet != SQLITE_OK)
        {
            /* 错误打印 */
            db_print_errMsg(strSelectSQL.c_str(), pchErrMsg);
            bRet = false;
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
        bRet = false;
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
#define DEBUG_PRINT() printf("test debug fun : %s, line : %d\n", __FUNCTION__, __LINE__);
/* 数据库备份 */
bool CDatabase::backup_database(sqlite3* pSq, const char* pchDbBackupName, bool bIsSave)
{
    if (NULL == pSq || NULL == pchDbBackupName)
    {
        dlog_error("传入参数为空");
        return false;
    }
    DEBUG_PRINT()
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
    DEBUG_PRINT()
    if (fileStream.good())
    {
        nHaveBackup = 1;
        dlog_trace("备份-记录数据库存在[%s]", pchDbBackupName);
    }
    else
    {
        dlog_error("备份-记录数据库不存在[%s]", pchDbBackupName);
    }
    DEBUG_PRINT()
    if (!nHaveBackup && 0 == bIsSave)
    {
        dlog_error("没有备份数据库，无法恢复数据库");
        return true;
    }
    DEBUG_PRINT()
    nRet = sqlite3_open(pchDbBackupName, &pFile);
    if (nRet == SQLITE_OK)
    {
    DEBUG_PRINT()
        pFrom   = (bIsSave ? pSq : pFile);
        pTo     = (bIsSave ? pFile : pSq);
        /* 备份process创建备份对象 */
        DEBUG_PRINT()
        pBackup = sqlite3_backup_init(pTo, "main", pFrom, "main");
        DEBUG_PRINT()
        if (pBackup)
        {
            DEBUG_PRINT()
            /* 拷贝数据 */
            (void)sqlite3_backup_step(pBackup, -1);
            DEBUG_PRINT()
            /* 释放资源 */
            (void)sqlite3_backup_finish(pBackup);
            DEBUG_PRINT()
        }
        /* 若拷贝的过程中出现任何错误,该函数可以获取具体的错误码 */
        nRet = sqlite3_errcode(pTo);
    }
    DEBUG_PRINT()
    if (bIsSave)
    {
        dlog_trace("备份数据成功");
    }
    else
    {
        dlog_trace("还原数据成功");
    }
    DEBUG_PRINT()

    (void)sqlite3_close(pFile);
    DEBUG_PRINT()
    return true;
}

/* 检查数据表是否存在 */
IpcRet_E CDatabase::check_tableExist(const char* pchTableName)
{
    if (NULL == pchTableName)
    {
        dlog_error("传入参数为空");
        return ERR_PARAM;
    }

    if (NULL == m_pDb)
    {
        dlog_error("未初始化");
        return ERR_UNINIT;
    }

    int           nTableExists    = 0;
    char*         pchCheckItemSql = NULL;
    sqlite3_stmt* pstCountstmt    = NULL;
    int           nRet            = 0;
    IpcRet_E     enRetCode       = OK;

    /* 检查数据项是否存在 */
    pchCheckItemSql = sqlite3_mprintf(SQL_CHECK_TABLE_EXIT, pchTableName);
    if (pchCheckItemSql == NULL)
    {
        dlog_error("创建命令失败");
        enRetCode = ERR;
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
        dlog_error("编译SQL语句-失败");
        enRetCode = ERR;
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
bool CDatabase::create_dataTable(const char* pchTableName, const char* pchSqlStr)
{
    if (NULL == pchTableName || NULL == pchSqlStr)
    {
        dlog_error("传入参数为空");
        return false;
    }

    if (NULL == m_pDb)
    {
        dlog_error("未初始化");
        return false;
    }

    char* pchSqlCmd = NULL;

    pchSqlCmd = sqlite3_mprintf(pchSqlStr, pchTableName);
    if (pchSqlCmd == NULL)
    {
        dlog_error("创建命令失败");
        return false;
    }

    if (!exec_transaction(pchSqlCmd))
    {
        dlog_error("创建数据表-失败");
    }

    if (pchSqlCmd)
    {
        sqlite3_free(pchSqlCmd);
        pchSqlCmd = NULL;
    }

    return true;
}

/* 检查字段是否存在，不存在追加 */
IpcRet_E CDatabase::check_fieldExist(
    const char* pchTableName,
    const char* pchFieldName,
    const char* pchFieldType)
{
    if (NULL == pchFieldName || NULL == pchTableName || NULL == pchFieldType)
    {
        dlog_error("传入参数为空");
        return ERR_PARAM;
    }

    if (NULL == m_pDb)
    {
        dlog_error("未初始化");
        return ERR_UNINIT;
    }

    int           nCount          = 0;
    char*         pchCheckItemSql = NULL;
    char*         pchAddColumnSql = NULL;
    sqlite3_stmt* pstCountstmt    = NULL;
    IpcRet_E     enRetCode       = OK;

    /* 检查数据项是否存在 */
    pchCheckItemSql = sqlite3_mprintf(SQL_PRAGMA_TABLE_INFO, pchTableName);
    if (pchCheckItemSql == NULL)
    {
        dlog_error("创建命令失败");
        enRetCode = ERR;
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
        dlog_error("编译SQL语句-失败");
        enRetCode = ERR;
        goto EXIT;
    }

    /* 存在 */
    if (enRetCode == OK_EXIST)
    {
        dlog_trace("字段[%s]存在", pchFieldName);
        goto EXIT;
    }
    /* 不存在 */
    else if (enRetCode == OK_NOT_EXIST)
    {
        dlog_trace("字段[%s]不存在，追加字段", pchFieldName);

        /* 字段不存在，执行追加字段操作 */
        pchAddColumnSql = sqlite3_mprintf(SQL_ALTER_TABLE_ADD_COLUMN,
                                          pchTableName,
                                          pchFieldName,
                                          pchFieldType);

        if (!exec_transaction(pchAddColumnSql))
        {
            dlog_error("执行事务-失败");
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
bool CDatabase::beginTransaction()
{
    if (NULL == m_pDb)
    {
        dlog_error("传入参数为空");
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
        bRet = false;
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
bool CDatabase::commitTransaction()
{
    if (NULL == m_pDb)
    {
        dlog_error("传入参数为空");
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
        bRet = false;
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
bool CDatabase::rollbackTransaction()
{
    if (NULL == m_pDb)
    {
        dlog_error("传入参数为空");
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
        bRet = false;
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
