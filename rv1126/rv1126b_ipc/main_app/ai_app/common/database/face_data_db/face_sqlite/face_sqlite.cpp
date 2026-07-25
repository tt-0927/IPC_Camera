/**
 * @FilePath     : face_sqlite.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-06-06 16:52:40
 * @Description  : 本地人脸名单库数据库
 */

#include "face_sqlite.hpp"

#include <cstring>
#include <fstream>

using namespace Event;
using namespace FaceDataDB_NS;

/* 数据库文件路径 */
#define DB_FACE_DATA_PATH        ("/userdata/cam/db/FaceSqlite.db")
#define DB_FACE_DATA_BUCKUP_PATH ("/userdata/cam/db/.backup.FaceSqlite.db")

/* 创建数据表的SQL语句 */
#define CREATE_FACE_DATA_SQL ("CREATE TABLE IF NOT EXISTS \"%s\" (\
                                        ID INTEGER PRIMARY KEY, \
                                        Name        TEXT, \
                                        PhoneNum    TEXT, \
                                        PicPath     TEXT, \
                                        PicType     TEXT, \
                                        PicSize     INTEGER, \
                                        PicDate     TEXT, \
                                        ModelState   INTEGER, \
                                        RatingLevel  INTEGER, \
                                        Data        BLOB \
                                        );")

/* 插入数据SQL语句 */
#define SQL_INSERT_DATA ("INSERT INTO \"%s\" ( \
                            ID, \
                            Name, \
                            PhoneNum, \
                            PicPath, \
                            PicType, \
                            PicSize, \
                            PicDate, \
                            ModelState, \
                            RatingLevel, \
                            Data) \
                            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);")

/* 更新数据 SQL 语句 */
#define SQL_UPDATE_DATA ("UPDATE \"%s\" SET \
                            Name=?, \
                            PhoneNum=? \
                            WHERE ID=?;")

/**
 * @brief: 删除数据
 * @param [string] : 表名
 */
#define SQL_DELETE_DATA ("DELETE FROM \"%s\" WHERE ID=?;")

/**
 * @brief: 查找数据
 * @param [string] : 表名
 */
#define SQL_SELECT_DATA ("SELECT * FROM \"%s\" WHERE ID=?;")

/**
 * @brief: 查找数据
 * @param [string] : 表名
 */
#define SQL_SELECT_ALL_DATA ("SELECT * FROM \"%s\";")


CFaceSqlite::CFaceSqlite()
{
    init_sql();
}

CFaceSqlite::~CFaceSqlite()
{
    if (m_pDb != nullptr)
    {
        sqlite3_close(m_pDb);
        m_pDb = nullptr;
    }
}


/* 插入数据 */
IpcRet_E CFaceSqlite::insertData(FaceLibsInfo_S& stInfo)
{
    /* 创建新名单组表 */
    // check_creat_table(stInfo.strFaceLibName);
    
    if (NULL == m_pDb)
    {
        dlog_error("未初始化");
        return ERR_UNINIT;
    }

    sqlite3_stmt* pstCountstmt = NULL;
    char*         pchInsertSQL = NULL;

    IpcRet_E enRetCode = OK;

    /* 获取最大ID, 确保不同表内ID 唯一 */
    stInfo.nId = get_tables_maxId() + 1;

    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (stInfo.nId == -1)
    {
        dlog_error("获取所有表最大 ID 的和失败");
        return ERR;
    }

    if (beginTransaction())
    {
        try
        {
            /* 拼接数据 */
            pchInsertSQL = sqlite3_mprintf(SQL_INSERT_DATA, stInfo.strFaceLibName.c_str());
            if (pchInsertSQL == NULL)
            {
                enRetCode = ERR;
                throw std::runtime_error("创建命令失败");
            }

            if (sqlite3_prepare_v2(m_pDb, pchInsertSQL, -1, &pstCountstmt, nullptr) == SQLITE_OK)
            {
                /* 绑定参数 */
                sqlite3_bind_int(pstCountstmt, 1, stInfo.nId);
                sqlite3_bind_text(pstCountstmt, 2, stInfo.strName.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(pstCountstmt, 3, stInfo.strPhoneNum.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(pstCountstmt, 4, stInfo.strPicPath.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(pstCountstmt, 5, stInfo.strPicType.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int(pstCountstmt, 6, stInfo.nPicSize);
                sqlite3_bind_text(pstCountstmt, 7, stInfo.strPicDate.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int(pstCountstmt, 8, stInfo.nModelState);
                sqlite3_bind_int(pstCountstmt, 9, stInfo.nRatingLevel);
                sqlite3_bind_blob(pstCountstmt, 10, stInfo.vfData.data(), static_cast<int>(stInfo.vfData.size() * sizeof(float)), SQLITE_STATIC);

                /* 执行 SQL 语句 */
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
                dlog_error("SQL语句 [%s]", pchInsertSQL);
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

    if (pchInsertSQL)
    {
        sqlite3_free(pchInsertSQL);
        pchInsertSQL = NULL;
    }

    return enRetCode;
}


/* 更新数据 */
IpcRet_E CFaceSqlite::updateData(int nId, FaceLibsInfo_S& stInfo)
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
            pchUpdateSQL = sqlite3_mprintf(SQL_UPDATE_DATA, stInfo.strFaceLibName.c_str());
            if (pchUpdateSQL == NULL)
            {
                enRetCode = ERR;
                throw std::runtime_error("创建命令失败");
            }
            
            /* 编译 SQL 语句 */
            if (sqlite3_prepare_v2(m_pDb, pchUpdateSQL, -1, &pstCountstmt, nullptr) == SQLITE_OK)
            {
                /* 绑定参数 */
                sqlite3_bind_text(pstCountstmt, 1, stInfo.strName.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(pstCountstmt, 2, stInfo.strPhoneNum.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int(pstCountstmt,  3, nId); /* 绑定要更新的 ID */


                /* 执行 SQL 语句 */
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
IpcRet_E CFaceSqlite::deleteData(int nId)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);
    
    if (NULL == m_pDb)
    {
        dlog_error("未初始化");
        return ERR_UNINIT;
    }

    char* pchDeleteSQL = nullptr;
    sqlite3_stmt* pstCountstmt = nullptr;
    const char* pchSelectTablesSQL = "SELECT name FROM sqlite_master WHERE type='table';";
    
    IpcRet_E enRetCode = OK;

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
                        dlog_error("删除数据失败: %s", sqlite3_errmsg(m_pDb));
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

                sqlite3_free(pchDeleteSQL);
            }

            /* 提交事务 */
            commitTransaction();
            enRetCode = OK;
        }
        
        catch (const std::exception& e)
        {
            dlog_error("删除数据失败 [%s]", e.what());
            rollbackTransaction();
            enRetCode = ERR;
        }
    }

    /* 清理表名查询语句 */
    if (pstCountstmt)
    {
        sqlite3_finalize(pstCountstmt);
    }

    return enRetCode;
}

/* 根据 ID 查找数据 */
IpcRet_E CFaceSqlite::searchDataById(int nId, FaceLibsInfo_S& stOutInfo)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog_error("未初始化");
        return ERR_UNINIT;
    }

    char* pchSelectSQL = nullptr;
    sqlite3_stmt* pstCountstmt = nullptr;
    const char* pchSelectTablesSQL = "SELECT name FROM sqlite_master WHERE type='table';";;
    
    IpcRet_E enRetCode = OK;

    /* 清空输出结构体 */
    stOutInfo.clear();

    /* 获取所有表名 */
    if (sqlite3_prepare_v2(m_pDb, pchSelectTablesSQL, -1, &pstCountstmt, nullptr) != SQLITE_OK)
    {
        dlog_error("获取表名失败: %s", sqlite3_errmsg(m_pDb));
        return ERR;
    }

    while (sqlite3_step(pstCountstmt) == SQLITE_ROW)
    {
        const char* strTabName = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 0));
        
        /* 拼接查询语句用于查找指定 ID */
        pchSelectSQL = sqlite3_mprintf("SELECT * FROM \"%s\" WHERE ID = ?;", strTabName);
        if (pchSelectSQL == nullptr)
        {
            dlog_error("创建查询命令失败");
            continue;
        }

        sqlite3_stmt* pSelectStmt = nullptr;

        /* 准备查询 SQL 语句 */
        if (sqlite3_prepare_v2(m_pDb, pchSelectSQL, -1, &pSelectStmt, nullptr) == SQLITE_OK)
        {
            /* 绑定要查找的 ID */
            sqlite3_bind_int(pSelectStmt, 1, nId);

            if (sqlite3_step(pSelectStmt) == SQLITE_ROW)
            {
                /* 获取查询结果并赋值 */
                stOutInfo.strFaceLibName = strTabName;
                stOutInfo.nId = sqlite3_column_int(pSelectStmt, 0);
                stOutInfo.strName = reinterpret_cast<const char*>(sqlite3_column_text(pSelectStmt, 1));
                stOutInfo.strPhoneNum = reinterpret_cast<const char*>(sqlite3_column_text(pSelectStmt, 2));
                stOutInfo.strPicPath = reinterpret_cast<const char*>(sqlite3_column_text(pSelectStmt, 3));
                stOutInfo.strPicType = reinterpret_cast<const char*>(sqlite3_column_text(pSelectStmt, 4));
                stOutInfo.nPicSize = sqlite3_column_int(pSelectStmt, 5);
                stOutInfo.strPicDate = reinterpret_cast<const char*>(sqlite3_column_text(pSelectStmt, 6));
                stOutInfo.nModelState = sqlite3_column_int(pSelectStmt, 7);
                stOutInfo.nRatingLevel = sqlite3_column_int(pSelectStmt, 8);

                enRetCode = OK;
                break;
            }
        }
        else
        {
            dlog_error("编译查询语句失败: %s", sqlite3_errmsg(m_pDb));
        }

        /* 清理和释放 */
        if (pSelectStmt)
        {
            sqlite3_finalize(pSelectStmt);
        }
        sqlite3_free(pchSelectSQL);
    }

    sqlite3_finalize(pstCountstmt);

    if (enRetCode == ERR_NOT_EXIST)
    {
        dlog_error("找不到该ID-查找失败");
    }

    return enRetCode;
}


/* 根据表名查找数据 */
IpcRet_E CFaceSqlite::searchDataByTable(std::string strTabName, std::list<FaceLibsInfo_S>& listOutInfo)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_pDb == nullptr)
    {
        dlog_error("未初始化");
        return ERR_UNINIT;
    }

    std::string query = "SELECT * FROM \"" + strTabName + "\";";
    sqlite3_stmt* pstSelectStmt = nullptr;

    IpcRet_E enRetCode = OK;
    listOutInfo.clear();

    if (sqlite3_prepare_v2(m_pDb, query.c_str(), -1, &pstSelectStmt, nullptr) == SQLITE_OK)
    {
        while (sqlite3_step(pstSelectStmt) == SQLITE_ROW)
        {
            FaceLibsInfo_S stItemInfo;
            stItemInfo.strFaceLibName = strTabName;
            stItemInfo.nId = sqlite3_column_int(pstSelectStmt, 0);
            stItemInfo.strName = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 1));
            stItemInfo.strPhoneNum = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 2));
            stItemInfo.strPicPath = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 3));
            stItemInfo.strPicType = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 4));
            stItemInfo.nPicSize = sqlite3_column_int(pstSelectStmt, 5);
            stItemInfo.strPicDate = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 6));
            stItemInfo.nModelState = sqlite3_column_int(pstSelectStmt, 7);
            stItemInfo.nRatingLevel = sqlite3_column_int(pstSelectStmt, 8);

            listOutInfo.push_back(stItemInfo);
        }
    }
    else
    {
        dlog_error("准备查询语句失败: %s", sqlite3_errmsg(m_pDb));
        enRetCode = ERR;
    }

    if (pstSelectStmt)
    {
        sqlite3_finalize(pstSelectStmt);
    }

    return enRetCode;
}


/* 获取所有数据 */
IpcRet_E CFaceSqlite::getAllData(std::list<FaceLibsInfo_S>& listOutInfo)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog_error("未初始化");
        return ERR_UNINIT;
    }

    sqlite3_stmt* pstCountstmt = NULL;
    const char* pchSelectTablesSQL = "SELECT name FROM sqlite_master WHERE type='table';";

    IpcRet_E enRetCode = OK;

    listOutInfo.clear();

    /* 获取所有表名 */
    if (sqlite3_prepare_v2(m_pDb, pchSelectTablesSQL, -1, &pstCountstmt, NULL) == SQLITE_OK)
    {
        while (sqlite3_step(pstCountstmt) == SQLITE_ROW)
        {
            const char* strTabName = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 0));
            
            /* 对每个表名执行 SELECT ALL 查询 */
            char* pchSelectSQL = sqlite3_mprintf(SQL_SELECT_ALL_DATA, strTabName);
            sqlite3_stmt* pstSelectStmt = NULL;
            
            if (sqlite3_prepare_v2(m_pDb, pchSelectSQL, -1, &pstSelectStmt, NULL) == SQLITE_OK)
            {
                /* 依次获取查询结果 */
                while (sqlite3_step(pstSelectStmt) == SQLITE_ROW)
                {
                    FaceLibsInfo_S stItemInfo;
                    stItemInfo.strFaceLibName = strTabName;
                    stItemInfo.nId = sqlite3_column_int(pstSelectStmt, 0);
                    stItemInfo.strName = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 1));
                    stItemInfo.strPhoneNum = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 2));
                    stItemInfo.strPicPath = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 3));
                    stItemInfo.strPicType = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 4));
                    stItemInfo.nPicSize = sqlite3_column_int(pstSelectStmt, 5);
                    stItemInfo.strPicDate = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 6));
                    stItemInfo.nModelState = sqlite3_column_int(pstSelectStmt, 7);
                    stItemInfo.nRatingLevel = sqlite3_column_int(pstSelectStmt, 8);

                    const void* pBlobData = sqlite3_column_blob(pstSelectStmt, 9);
                    int nDataSize = sqlite3_column_bytes(pstSelectStmt, 9);
                    stItemInfo.vfData.clear();
                    if (pBlobData != nullptr && nDataSize > 0)
                    {
                        const float* pfDataPtr = reinterpret_cast<const float*>(pBlobData);
                        stItemInfo.vfData.assign(pfDataPtr, pfDataPtr + nDataSize / sizeof(float));
                    }

                    listOutInfo.push_back(stItemInfo);
                }
            }
            else
            {
                dlog_error("准备 SELECT 查询语句失败 %s", sqlite3_errmsg(m_pDb));
                enRetCode = ERR;
            }

            if (pstSelectStmt)
            {
                sqlite3_finalize(pstSelectStmt);
            }
            
            if (pchSelectSQL)
            {
                sqlite3_free(pchSelectSQL);
            }
        }
    }
    else
    {
        dlog_error("准备获取表名失败 %s", sqlite3_errmsg(m_pDb));
        enRetCode = ERR;
    }

    if (pstCountstmt)
    {
        sqlite3_finalize(pstCountstmt);
        pstCountstmt = NULL;
    }

    return enRetCode;
}


/* 获取数据总量 */
IpcRet_E CFaceSqlite::getDataTotal(int& nOutTotal)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog_error("未初始化");
        return ERR_UNINIT;
    }

    sqlite3_stmt* pstCountstmt = NULL;
    
    const char* pchSelectTablesSQL = "SELECT name FROM sqlite_master WHERE type='table';";
    IpcRet_E enRetCode = OK;

    nOutTotal = 0; // 初始化总数

    /* 获取所有表名 */
    if (sqlite3_prepare_v2(m_pDb, pchSelectTablesSQL, -1, &pstCountstmt, NULL) == SQLITE_OK)
    {
        while (sqlite3_step(pstCountstmt) == SQLITE_ROW)
        {
            const char* strTabName = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 0));

            /* 对每个表名执行 COUNT(*) 查询 */
            char* pchCountSQL = sqlite3_mprintf("SELECT COUNT(*) FROM \"%s\";", strTabName);
            sqlite3_stmt* pstSelectStmt = NULL;

            if (sqlite3_prepare_v2(m_pDb, pchCountSQL, -1, &pstSelectStmt, NULL) == SQLITE_OK)
            {
                if (sqlite3_step(pstSelectStmt) == SQLITE_ROW)
                {
                    nOutTotal += sqlite3_column_int(pstSelectStmt, 0); // 累加行数
                }
                else
                {
                    dlog_error("执行 COUNT 查询失败 %s", sqlite3_errmsg(m_pDb));
                    enRetCode = ERR;
                }
            }
            else
            {
                dlog_error("准备 COUNT 查询语句失败 %s", sqlite3_errmsg(m_pDb));
                enRetCode = ERR;
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
        dlog_error("准备获取表名失败 %s", sqlite3_errmsg(m_pDb));
        enRetCode = ERR;
    }

    if (pstCountstmt)
    {
        sqlite3_finalize(pstCountstmt);
    }

    return enRetCode;
}


/* 初始化数据库 */
IpcRet_E CFaceSqlite::init_sql()
{
    sqlite3*  pDbhandle        = NULL;
    int       nRet             = 0;
    bool      bDbExit          = false;

    /* 判断数据库是否存在 */
    std::ifstream fileStream(DB_FACE_DATA_PATH);

    if (fileStream.good())
    {
        bDbExit = true;
        dlog_trace("记录数据库存在[%s]", DB_FACE_DATA_PATH);
    }
    else
    {
        dlog_error("记录数据库不存在[%s]", DB_FACE_DATA_PATH);
    }

    /* 打开数据库 */
    nRet = sqlite3_open(DB_FACE_DATA_PATH, &pDbhandle);
    if (nRet < 0)
    {
        dlog_error("打开数据库失败-[%s]", sqlite3_errmsg(pDbhandle));
        return ERR;
    }

    /* 数据库备份 */
    backup_database(pDbhandle, DB_FACE_DATA_BUCKUP_PATH, bDbExit);
    m_pDb = pDbhandle;

    return OK;
}


/* 创建新名单组数据库表 */
IpcRet_E CFaceSqlite::check_creat_table(std::string strTabName)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);
    
    if (NULL == m_pDb)
    {
        dlog_error("未初始化");
        return ERR_UNINIT;
    }

    IpcRet_E enRetCode = OK;
    char* pchCreateSQL = nullptr;
    sqlite3_stmt* pCreateStmt = nullptr;

    /* 创建表的 SQL 语句 */
    pchCreateSQL = sqlite3_mprintf(CREATE_FACE_DATA_SQL, strTabName.c_str());

    if (sqlite3_prepare_v2(m_pDb, pchCreateSQL, -1, &pCreateStmt, nullptr) == SQLITE_OK)
    {
        /* 执行 SQL 语句 */
        if (sqlite3_step(pCreateStmt) != SQLITE_DONE)
        {
            dlog_error("创建表失败: %s", sqlite3_errmsg(m_pDb));
            enRetCode = ERR;
        }
        else
        {
            dlog_trace("成功创建表: %s", strTabName.c_str());
            enRetCode = OK;
        }
    }
    else
    {
        dlog_error("编译创建表语句失败: %s", sqlite3_errmsg(m_pDb));
        enRetCode = ERR;
    }

    /* 清理和释放 */
    if (pCreateStmt)
    {
        sqlite3_finalize(pCreateStmt);
    }

    if (pchCreateSQL)
    {
        sqlite3_free(pchCreateSQL);
    }

    return enRetCode;
}


/* 修改名单组数据库表名 */
IpcRet_E CFaceSqlite::renameTable(std::string oldTabName, std::string newTabName)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_pDb == nullptr)
    {
        dlog_error("未初始化");
        return ERR_UNINIT;
    }

    std::string query = "ALTER TABLE \"" + oldTabName + "\" RENAME TO \"" + newTabName + "\";";
    
    char* errMsg = nullptr;
    if (sqlite3_exec(m_pDb, query.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        dlog_error("修改表名失败: %s", errMsg);
        sqlite3_free(errMsg);
        return ERR;
    }
    
    return OK;
}


/* 删除名单组数据库表 */
IpcRet_E CFaceSqlite::deleteTable(std::string strTabName)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);
    
    if (NULL == m_pDb)
    {
        dlog_error("未初始化");
        return ERR_UNINIT;
    }

    IpcRet_E enRetCode = OK;
    char* pchDeleteSQL = nullptr;
    sqlite3_stmt* pDeleteStmt = nullptr;

    /* 准备删除表的 SQL 语句 */
    pchDeleteSQL = sqlite3_mprintf("DROP TABLE IF EXISTS \"%s\";", strTabName.c_str());

    if (sqlite3_prepare_v2(m_pDb, pchDeleteSQL, -1, &pDeleteStmt, nullptr) == SQLITE_OK)
    {
        /* 执行 SQL 语句 */
        if (sqlite3_step(pDeleteStmt) != SQLITE_DONE)
        {
            dlog_error("删除表失败: %s", sqlite3_errmsg(m_pDb));
            enRetCode = ERR;
        }
        else
        {
            dlog_trace("成功删除表: %s", strTabName.c_str());
            enRetCode = OK;
        }
    }
    else
    {
        dlog_error("编译删除表语句失败: %s", sqlite3_errmsg(m_pDb));
        enRetCode = ERR;
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


/* 获取每张表的数据统计信息 */
IpcRet_E CFaceSqlite::get_table_report(std::vector<Event::FaceLibInfo_S>& listTableReport)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_pDb == nullptr)
    {
        dlog_error("未初始化");
        return ERR_UNINIT;
    }

    const char* pchSelectTablesSQL = "SELECT name FROM sqlite_master WHERE type='table';";
    sqlite3_stmt* pstSelectStmt = nullptr;
    
    listTableReport.clear();

    /* 获取所有表名 */
    if (sqlite3_prepare_v2(m_pDb, pchSelectTablesSQL, -1, &pstSelectStmt, nullptr) == SQLITE_OK)
    {
        while (sqlite3_step(pstSelectStmt) == SQLITE_ROW)
        {
            const char* strTabName = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 0));

            /* 统计每个表的数据个数 */
            std::string countQuery = "SELECT COUNT(*) FROM \"" + std::string(strTabName) + "\";";
            sqlite3_stmt* pstCountStmt = nullptr;
            
            int totalRecords = 0;
            int normalRecords = 0;
            int abnormalRecords = 0;

            if (sqlite3_prepare_v2(m_pDb, countQuery.c_str(), -1, &pstCountStmt, nullptr) == SQLITE_OK)
            {
                if (sqlite3_step(pstCountStmt) == SQLITE_ROW)
                {
                    totalRecords = sqlite3_column_int(pstCountStmt, 0);
                }
            }
            sqlite3_finalize(pstCountStmt);

            /* 统计正常和异常记录 */
            std::string modelStateQuery = "SELECT COUNT(*) FROM \"" + std::string(strTabName) + "\" WHERE ModelState=1;";
            sqlite3_stmt* pstModelStateStmt = nullptr;

            if (sqlite3_prepare_v2(m_pDb, modelStateQuery.c_str(), -1, &pstModelStateStmt, nullptr) == SQLITE_OK)
            {
                if (sqlite3_step(pstModelStateStmt) == SQLITE_ROW)
                {
                    normalRecords = sqlite3_column_int(pstModelStateStmt, 0);
                }
            }
            sqlite3_finalize(pstModelStateStmt);

            /* 异常记录为总记录减去正常记录 */
            abnormalRecords = totalRecords - normalRecords;

            /* 将结果添加到输出列表 */
            Event::FaceLibInfo_S report;
            report.strFaceLibName = strTabName;
            report.nTotalFace = totalRecords;
            report.nNormalNum = normalRecords;
            report.nAbnormalNum = abnormalRecords;
            listTableReport.push_back(report);
        }
    }
    else
    {
        dlog_error("获取表名失败: %s", sqlite3_errmsg(m_pDb));
        return ERR;
    }

    sqlite3_finalize(pstSelectStmt);
    return OK;
}


/* 根据组合条件查询数据 */
IpcRet_E CFaceSqlite::search_combined_data(Event::FaceFind_S stFaceFind, std::list<FaceLibsInfo_S>& listOutInfo)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_pDb == nullptr)
    {
        dlog_error("未初始化");
        return ERR_UNINIT;
    }

    std::string query = "SELECT * FROM \"" + stFaceFind.strFaceLibName + "\" WHERE 1=1";

    /* 根据条件拼接查询 */

    if (!stFaceFind.strName.empty())
    {
        query += " AND Name LIKE '%" + stFaceFind.strName + "%'";
    }
    
    if (!stFaceFind.strPhoneNum.empty())
    {
        query += " AND PhoneNum LIKE '%" + stFaceFind.strPhoneNum + "%'";
    }
    
    if (stFaceFind.nModelState != -1)
    {
        query += " AND ModelState = " + std::to_string(stFaceFind.nModelState);
    }
    
    if (stFaceFind.nRatingLevel != -1) 
    {
        if (stFaceFind.nRatingLevel == 0) 
        {
            /* 未知：查询不带分数的记录 */
            query += " AND RatingLevel IS NULL";
        }
        else if (stFaceFind.nRatingLevel == 8) 
        {
            /* 高：查询 >=8 分的记录 */
            query += " AND RatingLevel >= 8";
        }
        else if (stFaceFind.nRatingLevel == 1) 
        {
            /* 低：查询 1-8 分的记录 */
            query += " AND RatingLevel BETWEEN 1 AND 7";
        }
    }

    sqlite3_stmt* pstSelectStmt = nullptr;
    listOutInfo.clear();

    /* 执行查询 */
    if (sqlite3_prepare_v2(m_pDb, query.c_str(), -1, &pstSelectStmt, nullptr) == SQLITE_OK)
    {
        while (sqlite3_step(pstSelectStmt) == SQLITE_ROW)
        {
            FaceLibsInfo_S stItemInfo;
            stItemInfo.strFaceLibName = stFaceFind.strFaceLibName;
            stItemInfo.nId = sqlite3_column_int(pstSelectStmt, 0);
            stItemInfo.strName = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 1));
            stItemInfo.strPhoneNum = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 2));
            stItemInfo.strPicPath = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 3));
            stItemInfo.strPicType = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 4));
            stItemInfo.nPicSize = sqlite3_column_int(pstSelectStmt, 5);
            stItemInfo.strPicDate = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 6));
            stItemInfo.nModelState = sqlite3_column_int(pstSelectStmt, 7);
            stItemInfo.nRatingLevel = sqlite3_column_int(pstSelectStmt, 8);

            const void* pBlobData = sqlite3_column_blob(pstSelectStmt, 9);
            int nDataSize = sqlite3_column_bytes(pstSelectStmt, 9);
            stItemInfo.vfData.clear();
            
            if (pBlobData != nullptr && nDataSize > 0)
            {
                const float* pfDataPtr = reinterpret_cast<const float*>(pBlobData);
                stItemInfo.vfData.assign(pfDataPtr, pfDataPtr + nDataSize / sizeof(float));
            }

            listOutInfo.push_back(stItemInfo);
        }
    }
    else
    {
        dlog_error("准备查询语句失败: %s", sqlite3_errmsg(m_pDb));
        return ERR;
    }

    if (pstSelectStmt)
    {
        sqlite3_finalize(pstSelectStmt);
    }

    return OK;
}


/* 获取数据库内最大的 ID */
int CFaceSqlite::get_tables_maxId()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog_error("未初始化");
        return -1;
    }

    sqlite3_stmt* pstCountstmt = nullptr;
    const char* pchSelectTablesSQL = "SELECT name FROM sqlite_master WHERE type='table';";

    int totalMaxSum = 0;

    /* 获取所有表名 */
    if (sqlite3_prepare_v2(m_pDb, pchSelectTablesSQL, -1, &pstCountstmt, NULL) == SQLITE_OK)
    {
        while (sqlite3_step(pstCountstmt) == SQLITE_ROW)
        {
            const char* strTabName = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 0));
            char* pchMaxIdSQL = sqlite3_mprintf("SELECT MAX(ID) FROM \"%s\";", strTabName);

            sqlite3_stmt* pMaxIdStmt = nullptr;

            /* 查询当前表的最大 ID */
            if (sqlite3_prepare_v2(m_pDb, pchMaxIdSQL, -1, &pMaxIdStmt, NULL) == SQLITE_OK)
            {
                if (sqlite3_step(pMaxIdStmt) == SQLITE_ROW)
                {
                    int currentMaxId = sqlite3_column_int(pMaxIdStmt, 0);
                    if (currentMaxId > 0)
                    {
                        if (totalMaxSum < currentMaxId)
                        {
                            totalMaxSum = currentMaxId;
                        }
                    }
                }
            }
            else
            {
                dlog_error("准备最大 ID 查询语句失败: %s", sqlite3_errmsg(m_pDb));
            }

            if (pMaxIdStmt)
            {
                sqlite3_finalize(pMaxIdStmt);
            }
            
            sqlite3_free(pchMaxIdSQL);
        }
    }
    else
    {
        dlog_error("准备获取表名失败: %s", sqlite3_errmsg(m_pDb));
    }

    if (pstCountstmt)
    {
        sqlite3_finalize(pstCountstmt);
    }

    dlog_debug("ai_app: \033[34m %s:%d totalMaxSum = %d \033[m\n",__func__,__LINE__, totalMaxSum);
    return totalMaxSum;
}
