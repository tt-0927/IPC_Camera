/**
 * @FilePath     : snap_sqlite.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-06-06 16:54:35
 * @Description  : 人脸抓拍数据库
 */

#include "snap_sqlite.hpp"

#include <cstring>
#include <fstream>

using namespace FaceDataDB_NS;

/* 数据库文件路径 */
#define DB_SNAP_DATA_PATH        ("/userdata/cam/db/SnapSqlite.db")
#define DB_SNAP_DATA_BUCKUP_PATH ("/userdata/cam/db/.backup.SnapSqlite.db")

/* 数据库表名 */
#define DB_SNAP_DATA_TABLE_NAME ("SnapInfo")

/* 创建数据表 */
#define CREATE_SNAP_DATA_SQL ("CREATE TABLE IF NOT EXISTS %s (\
                                        ID      INTEGER PRIMARY KEY AUTOINCREMENT, \
                                        ChnId   INTEGER, \
                                        PicPath TEXT, \
                                        Data    BLOB \
                                        );")

/**
 * @brief: 添加数据
 * @param [string] : 表名
 */
#define SQL_INSERT_DATA ("INSERT INTO %s ( \
                            ChnId, \
                            PicPath, \
                            Data) \
                            VALUES (?, ?, ?);")

/**
 * @brief: 更新数据
 * @param [string] : 表名
 */
#define SQL_UPDATE_DATA ("UPDATE %s SET \
                            ChnId=?, \
                            PicPath=?, \
                            Data=? \
                            WHERE \
                            ID=?;")

/**
 * @brief: 删除数据
 * @param [string] : 表名
 */
#define SQL_DELETE_DATA ("DELETE FROM %s WHERE ID=?;")

/**
 * @brief: 查找数据
 * @param [string] : 表名
 */
#define SQL_SELECT_DATA ("SELECT * FROM %s WHERE ID=?;")

/**
 * @brief: 查找数据
 * @param [string] : 表名
 */
#define SQL_SELECT_ALL_DATA ("SELECT * FROM %s;")


CSnapSqlite::CSnapSqlite()
{
    init_sql();
}

CSnapSqlite::~CSnapSqlite()
{
    if (m_pDb != nullptr)
    {
        sqlite3_close(m_pDb);
        m_pDb = nullptr;
    }
}

/* 插入数据 */
IpcRet_E CSnapSqlite::insertData(SnapFaceInfo_S& stInfo)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog_error("未初始化");
        return ERR_UNINIT;
    }

    sqlite3_stmt* pstCountstmt = NULL;
    char*         pchSelectSQL = NULL;

    IpcRet_E enRetCode = OK;

    if (beginTransaction())
    {
        try
        {
            /* 拼接数据 */
            pchSelectSQL = sqlite3_mprintf(SQL_INSERT_DATA, DB_SNAP_DATA_TABLE_NAME);
            if (pchSelectSQL == NULL)
            {
                enRetCode = ERR;
                throw std::runtime_error("创建命令失败");
            }

            /* 编译 SQL 语句 */
            if (sqlite3_prepare_v2(m_pDb, pchSelectSQL, -1, &pstCountstmt, nullptr) == SQLITE_OK)
            {
                sqlite3_bind_int(pstCountstmt, 1, stInfo.nChnId);
                sqlite3_bind_text(pstCountstmt, 2, (stInfo.strPicPath).c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_blob(pstCountstmt, 3, stInfo.vfData.data(), static_cast<int>(stInfo.vfData.size() * sizeof(float)), SQLITE_STATIC);

                if (sqlite3_step(pstCountstmt) == SQLITE_DONE)
                {
                    stInfo.nId = static_cast<int>(sqlite3_last_insert_rowid(m_pDb));
                    
                    /* 提交事务 */
                    if (commitTransaction())
                    {
                        /* 执行成功 */
                        enRetCode = OK;
                        goto EXIT;
                    }
                    else
                    {
                        enRetCode = ERR;
                        throw std::runtime_error("插入数据-提交事务失败");
                    }
                }
                else
                {
                    enRetCode = ERR;
                    throw std::runtime_error("执行SQL语句出错");
                }
            }
            else
            {
                enRetCode = ERR;
                dlog_error("SQL语句 [%s]", pchSelectSQL);
                throw std::runtime_error("编译SQL语句-失败");
            }
        }
        catch (const std::exception& e)
        {
            dlog_error("插入数据失败 [%s]", e.what());
            rollbackTransaction();
        }
    }

EXIT:
    if (pstCountstmt)
    {
        sqlite3_finalize(pstCountstmt);
        pstCountstmt = NULL;
    }

    if (pchSelectSQL)
    {
        sqlite3_free(pchSelectSQL);
        pchSelectSQL = NULL;
    }

    return enRetCode;
}

/* 更新数据 */
IpcRet_E CSnapSqlite::updateData(int nId, SnapFaceInfo_S& stInfo)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog_error("未初始化");
        return ERR_UNINIT;
    }

    sqlite3_stmt* pstCountstmt = NULL;
    char*         pchUpdateSQL = NULL;

    IpcRet_E enRetCode = OK;


    if (beginTransaction())
    {
        try
        {
            /* 拼接数据 */
            pchUpdateSQL = sqlite3_mprintf(SQL_UPDATE_DATA, DB_SNAP_DATA_TABLE_NAME);
            if (pchUpdateSQL == NULL)
            {
                enRetCode = ERR;
                throw std::runtime_error("创建命令失败");
            }

            /* 编译 SQL 语句 */
            if (sqlite3_prepare_v2(m_pDb, pchUpdateSQL, -1, &pstCountstmt, 0) == SQLITE_OK)
            {
                sqlite3_bind_int(pstCountstmt, 1, stInfo.nChnId);
                sqlite3_bind_text(pstCountstmt, 2, (stInfo.strPicPath).c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_blob(pstCountstmt, 3, stInfo.vfData.data(), static_cast<int>(stInfo.vfData.size() * sizeof(float)), SQLITE_STATIC);
                
                sqlite3_bind_int(pstCountstmt, 4, nId);

                if (sqlite3_step(pstCountstmt) == SQLITE_DONE)
                {
                    /* 提交事务 */
                    if (commitTransaction())
                    {
                        /* 执行成功 */
                        enRetCode = OK;
                        goto EXIT;
                    }
                    else
                    {
                        enRetCode = ERR;
                        throw std::runtime_error("插入数据-提交事务失败");
                    }
                }
                else
                {
                    enRetCode = ERR;
                    throw std::runtime_error("执行SQL语句出错");
                }
            }
            else
            {
                enRetCode = ERR;
                throw std::runtime_error("编译SQL语句-失败");
            }
        }
        catch (const std::exception& e)
        {
            dlog_error("插入数据失败 [%s]", e.what());
            rollbackTransaction();
        }
    }

EXIT:
    if (pstCountstmt)
    {
        sqlite3_finalize(pstCountstmt);
        pstCountstmt = NULL;
    }

    if (pchUpdateSQL)
    {
        sqlite3_free(pchUpdateSQL);
        pchUpdateSQL = NULL;
    }

    return enRetCode;
}

/* 删除数据 */
IpcRet_E CSnapSqlite::deleteData(int nId)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog_error("未初始化");
        return ERR_UNINIT;
    }

    sqlite3_stmt* pstCountstmt = NULL;
    char*         pchDeleteSQL = NULL;

    IpcRet_E enRetCode = OK;

    if (beginTransaction())
    {
        try
        {
            /* 拼接数据 */
            pchDeleteSQL = sqlite3_mprintf(SQL_DELETE_DATA, DB_SNAP_DATA_TABLE_NAME);
            if (pchDeleteSQL == NULL)
            {
                enRetCode = ERR;
                throw std::runtime_error("创建命令失败");
            }

            /* 编译 SQL 语句 */
            if (sqlite3_prepare_v2(m_pDb, pchDeleteSQL, -1, &pstCountstmt, 0) == SQLITE_OK)
            {
                sqlite3_bind_int(pstCountstmt, 1, nId);

                if (sqlite3_step(pstCountstmt) == SQLITE_DONE)
                {
                    /* 提交事务 */
                    if (commitTransaction())
                    {
                        /* 执行成功 */
                        enRetCode = OK;
                        goto EXIT;
                    }
                    else
                    {
                        enRetCode = ERR;
                        throw std::runtime_error("插入数据-提交事务失败");
                    }
                }
                else
                {
                    enRetCode = ERR;
                    throw std::runtime_error("执行SQL语句出错");
                }
            }
            else
            {
                enRetCode = ERR;
                throw std::runtime_error("编译SQL语句-失败");
            }
        }
        catch (const std::exception& e)
        {
            dlog_error("插入数据失败 [%s]", e.what());
            rollbackTransaction();
        }
    }

EXIT:
    if (pstCountstmt)
    {
        sqlite3_finalize(pstCountstmt);
        pstCountstmt = NULL;
    }

    if (pchDeleteSQL)
    {
        sqlite3_free(pchDeleteSQL);
        pchDeleteSQL = NULL;
    }

    return enRetCode;
}

/* 查找数据 */
IpcRet_E CSnapSqlite::searchData(int nId, SnapFaceInfo_S& stOutInfo)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog_error("未初始化");
        return ERR_UNINIT;
    }

    sqlite3_stmt* pstCountstmt = NULL;
    char*         pchDeleteSQL = NULL;

    IpcRet_E enRetCode = OK;

    const char*  pchName    = nullptr;
    const void*  pBlobData  = nullptr;
    int          nDataSize  = 0;
    const float* pfDataPtr  = nullptr;
    int          nNumFloats = 0;

    stOutInfo.clear();

    /* 拼接数据 */
    pchDeleteSQL = sqlite3_mprintf(SQL_SELECT_DATA, DB_SNAP_DATA_TABLE_NAME);
    if (pchDeleteSQL == NULL)
    {
        enRetCode = ERR;
        goto EXIT;
    }

    /* 编译 SQL 语句 */
    if (sqlite3_prepare_v2(m_pDb, pchDeleteSQL, -1, &pstCountstmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int(pstCountstmt, 1, nId);

        if (sqlite3_step(pstCountstmt) == SQLITE_ROW)
        {
            stOutInfo.nId = sqlite3_column_int(pstCountstmt, 0);
            stOutInfo.nChnId = sqlite3_column_int(pstCountstmt, 1);

            /* 图片名字 */
            pchName = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 2));
            if (pchName != nullptr)
            {
                stOutInfo.strPicPath = pchName;
            }
            pBlobData = sqlite3_column_blob(pstCountstmt, 3);
            nDataSize = sqlite3_column_bytes(pstCountstmt, 3);

            /* 数据 */
            stOutInfo.vfData.clear();
            if (pBlobData != nullptr && nDataSize > 0)
            {
                pfDataPtr  = reinterpret_cast<const float*>(pBlobData);
                nNumFloats = nDataSize / sizeof(float);

                stOutInfo.vfData.assign(pfDataPtr, pfDataPtr + nNumFloats);
            }
        }
        else
        {
            dlog_error("找不到该ID-查找失败");
            enRetCode = ERR_NOT_EXIST;
            goto EXIT;
        }
    }
    else
    {
        enRetCode = ERR;
        dlog_error("编译SQL语句-失败");
        goto EXIT;
    }

EXIT:
    if (pstCountstmt)
    {
        sqlite3_finalize(pstCountstmt);
        pstCountstmt = NULL;
    }

    if (pchDeleteSQL)
    {
        sqlite3_free(pchDeleteSQL);
        pchDeleteSQL = NULL;
    }

    return enRetCode;
}


/* 获取全部数据 */
IpcRet_E CSnapSqlite::getAllData(std::list<SnapFaceInfo_S>& listOutInfo)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog_error("未初始化");
        return ERR_UNINIT;
    }

    sqlite3_stmt* pstCountstmt = NULL;
    char*         pchDeleteSQL = NULL;

    IpcRet_E enRetCode = OK;

    SnapFaceInfo_S stItemInfo;
    const void*    pBlobData = nullptr;
    int            nDataSize = 0;
    const float*   pfDataPtr = 0;


    listOutInfo.clear();

    /* 拼接数据 */
    pchDeleteSQL = sqlite3_mprintf(SQL_SELECT_ALL_DATA, DB_SNAP_DATA_TABLE_NAME);
    if (pchDeleteSQL == NULL)
    {
        enRetCode = ERR;
        goto EXIT;
    }

    /* 编译 SQL 语句 */
    if (sqlite3_prepare_v2(m_pDb, pchDeleteSQL, -1, &pstCountstmt, 0) == SQLITE_OK)
    {
        while (sqlite3_step(pstCountstmt) == SQLITE_ROW)
        {

            stItemInfo.nId        = sqlite3_column_int(pstCountstmt, 0);
            stItemInfo.nChnId     = sqlite3_column_int(pstCountstmt, 1);
            stItemInfo.strPicPath = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 2));

            pBlobData = sqlite3_column_blob(pstCountstmt, 3);
            nDataSize = sqlite3_column_bytes(pstCountstmt, 3);
            stItemInfo.vfData.clear();
            if (pBlobData != nullptr && nDataSize > 0)
            {
                pfDataPtr = reinterpret_cast<const float*>(pBlobData);
                for (int i = 0; i < (int)(nDataSize / sizeof(float)); ++i)
                {
                    stItemInfo.vfData.push_back(pfDataPtr[i]);
                }
            }

            listOutInfo.push_back(stItemInfo);
        }
    }
    else
    {
        enRetCode = ERR;
        dlog_error("编译SQL语句-失败");
        goto EXIT;
    }

EXIT:
    if (pstCountstmt)
    {
        sqlite3_finalize(pstCountstmt);
        pstCountstmt = NULL;
    }

    if (pchDeleteSQL)
    {
        sqlite3_free(pchDeleteSQL);
        pchDeleteSQL = NULL;
    }

    return enRetCode;
}

/* 获取数据总量 */
IpcRet_E CSnapSqlite::getDataTotal(int& nOutTotal)
{
    if (NULL == m_pDb)
    {
        dlog_error("未初始化");
        return ERR_UNINIT;
    }

    sqlite3_stmt* pstCountstmt = NULL;
    char*         pchSelectSQL = NULL;

    IpcRet_E enRetCode = OK;

    pchSelectSQL = sqlite3_mprintf(SQL_GET_TOTAL, DB_SNAP_DATA_TABLE_NAME);
    if (pchSelectSQL == NULL)
    {
        dlog_error("创建命令失败");
        enRetCode = ERR;
        goto EXIT;
    }

    /* 编译SQL语句 */
    if (sqlite3_prepare_v2(m_pDb, pchSelectSQL, -1, &pstCountstmt, NULL) == SQLITE_OK)
    {
        enRetCode = ERR;
        /* 执行循环以获取结果行 */
        while (sqlite3_step(pstCountstmt) == SQLITE_ROW)
        {
            nOutTotal = sqlite3_column_int(pstCountstmt, 0);

            enRetCode = OK;
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

    if (pchSelectSQL)
    {
        sqlite3_free(pchSelectSQL);
        pchSelectSQL = NULL;
    }

    return enRetCode;
}

/* 初始化数据库 */
IpcRet_E CSnapSqlite::init_sql()
{
    IpcRet_E enRetCode        = OK;
    sqlite3*  pDbhandle        = NULL;
    int       nRet             = 0;
    bool      bDbExit          = false;

    /* 判断数据库是否存在 */
    std::ifstream fileStream(DB_SNAP_DATA_PATH);

    if (fileStream.good())
    {
        bDbExit = true;
        dlog_trace("记录数据库存在[%s]", DB_SNAP_DATA_PATH);
    }
    else
    {
        dlog_error("记录数据库不存在[%s]", DB_SNAP_DATA_PATH);
    }

    /* 打开数据库 */
    nRet = sqlite3_open(DB_SNAP_DATA_PATH, &pDbhandle);
    if (nRet < 0)
    {
        dlog_error("打开数据库失败-[%s]", sqlite3_errmsg(pDbhandle));
        return ERR;
    }

    /* 数据库备份 */
    backup_database(pDbhandle, DB_SNAP_DATA_BUCKUP_PATH, bDbExit);
    m_pDb = pDbhandle;

    /* 检验数据库表是否存在 */
    enRetCode = check_tableExist(DB_SNAP_DATA_TABLE_NAME);
    if (enRetCode >= OK)
    {
        /* 不存在 */
        if (enRetCode == OK_NOT_EXIST)
        {
            dlog_error("数据表不存在");
            /* 创建表 */
            if (!create_dataTable(DB_SNAP_DATA_TABLE_NAME, CREATE_SNAP_DATA_SQL))
            {
                return ERR;
            }
        }
        else
        {
            dlog_trace("数据表存在");
        }

        /* 校验表格字段 */
        check_fieldExist(DB_SNAP_DATA_TABLE_NAME, "ID", "INTEGER ·RIMARY KEY AUTOINCREMENT");
        check_fieldExist(DB_SNAP_DATA_TABLE_NAME, "ChnId", "INTEGER");
        check_fieldExist(DB_SNAP_DATA_TABLE_NAME, "PicPath", "TEXT");
        check_fieldExist(DB_SNAP_DATA_TABLE_NAME, "Data", "BLOB");
    }
    else
    {
        dlog_error("检验数据库表是否存在-失败");
    }

    return OK;
}