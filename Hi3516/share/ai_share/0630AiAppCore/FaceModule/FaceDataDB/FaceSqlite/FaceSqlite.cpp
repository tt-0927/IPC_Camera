/**
 * @file FaceSqlite.cpp
 * @author xiejh (xiejh@kfb.cn)
 * @date 2024-11-26
 *
 * @brief 本地人脸名单库数据库
 */

#include "FaceSqlite.hpp"

#include <cstring>
#include <fstream>

using namespace Ai0630_NS;

/* 数据库文件路径 */
#define DB_FACE_DATA_PATH        ("/opt/bl/db/FaceSqlite.db")
#define DB_FACE_DATA_BUCKUP_PATH ("/opt/bl/db/.backup.FaceSqlite.db")

/* 创建数据表的SQL语句 */
#define CREATE_FACE_DATA_SQL ("CREATE TABLE IF NOT EXISTS \"%s\" (\
                                        ID INTEGER PRIMARY KEY, \
                                        ClassId       INTEGER, \
                                        MemberId      INTEGER, \
                                        Name          TEXT, \
                                        PicName       TEXT, \
                                        LocalPicPath  TEXT, \
                                        RemotePicPath TEXT, \
                                        PicMd5        TEXT, \
                                        PicDate       TEXT, \
                                        Identity    INTEGER, \
                                        Confidence    REAL, \
                                        Data1         BLOB, \
                                        Data2         BLOB \
                                        );")

/* 插入数据SQL语句 */
#define SQL_INSERT_DATA ("INSERT INTO \"%s\" ( \
                            ClassId, \
                            MemberId, \
                            Name, \
                            PicName, \
                            LocalPicPath, \
                            RemotePicPath, \
                            PicMd5, \
                            PicDate, \
                            Identity, \
                            Confidence, \
                            Data1, \
                            Data2) \
                            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);")

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

FaceSqlite::FaceSqlite()
{
    init_sql();
}

FaceSqlite::~FaceSqlite()
{
    if (m_pDb != nullptr)
    {
        sqlite3_close(m_pDb);
        m_pDb = nullptr;
    }
}

/* 插入数据 */
BlError_E FaceSqlite::insertData(FaceLibsInfo_S& stInfo)
{
    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    if (check_tableExist(std::to_string(stInfo.nClassId).c_str()) == BlError_E::OK_NOT_EXIST)
    {
        /* 创建新名单组表 */
        create_dataTable(std::to_string(stInfo.nClassId).c_str(), CREATE_FACE_DATA_SQL);
    }

    sqlite3_stmt* pstCountstmt = NULL;
    char*         pchInsertSQL = NULL;

    BlError_E enRetCode = OK;

    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (beginTransaction())
    {
        try
        {
            /* 拼接数据 */
            pchInsertSQL = sqlite3_mprintf(SQL_INSERT_DATA, std::to_string(stInfo.nClassId).c_str());
            if (pchInsertSQL == NULL)
            {
                enRetCode = NOK;
                throw std::runtime_error("创建命令失败");
            }

            if (sqlite3_prepare_v2(m_pDb, pchInsertSQL, -1, &pstCountstmt, nullptr) == SQLITE_OK)
            {
                /* 绑定参数 */
                sqlite3_bind_int(pstCountstmt, 1, stInfo.nClassId);
                sqlite3_bind_int(pstCountstmt, 2, stInfo.nMemberId);
                sqlite3_bind_text(pstCountstmt, 3, stInfo.strName.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(pstCountstmt, 4, stInfo.strPicName.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(pstCountstmt, 5, stInfo.strLocalPicPath.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(pstCountstmt, 6, stInfo.strRemotePicPath.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(pstCountstmt, 7, stInfo.strPicMd5.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(pstCountstmt, 8, stInfo.strPicDate.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int(pstCountstmt, 9, stInfo.nIdentity);
                sqlite3_bind_double(pstCountstmt, 10, stInfo.fConfidence);
                sqlite3_bind_blob(pstCountstmt, 11, stInfo.vfData1.data(), static_cast<int>(stInfo.vfData1.size() * sizeof(float)), SQLITE_STATIC);
                sqlite3_bind_blob(pstCountstmt, 12, stInfo.vfData2.data(), static_cast<int>(stInfo.vfData2.size() * sizeof(float)), SQLITE_STATIC);

                /* 执行 SQL 语句 */
                if (sqlite3_step(pstCountstmt) == SQLITE_DONE)
                {
                    stInfo.nId = static_cast<int>(sqlite3_last_insert_rowid(m_pDb));

                    /* 提交事务 */
                    if (commitTransaction())
                    {
                        /* 执行成功 */
                        enRetCode = OK;
                    }
                    else
                    {
                        enRetCode = NOK;
                        throw std::runtime_error("插入数据-提交事务失败");
                    }
                }
                else
                {
                    enRetCode = NOK;
                    throw std::runtime_error("执行SQL语句出错");
                }
            }
            else
            {
                enRetCode = NOK;
                dlog(LOG_ERROR, "SQL语句 [%s]", pchInsertSQL);
                throw std::runtime_error("编译SQL语句-失败");
            }
        }

        catch (const std::exception& e)
        {
            dlog(LOG_ERROR, "插入数据失败 [%s]", e.what());
            rollbackTransaction();
        }
    }

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

/* 根据 ID 查找数据 */
BlError_E FaceSqlite::searchDataById(int nId, FaceLibsInfo_S& stOutInfo)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    char*         pchSelectSQL       = nullptr;
    sqlite3_stmt* pstCountstmt       = nullptr;
    const char*   pchSelectTablesSQL = "SELECT name FROM sqlite_master WHERE type='table';";
    ;

    BlError_E enRetCode = OK;

    /* 清空输出结构体 */
    stOutInfo.clear();

    /* 获取所有表名 */
    if (sqlite3_prepare_v2(m_pDb, pchSelectTablesSQL, -1, &pstCountstmt, nullptr) != SQLITE_OK)
    {
        dlog(LOG_ERROR, "获取表名失败: %s", sqlite3_errmsg(m_pDb));
        return NOK;
    }

    while (sqlite3_step(pstCountstmt) == SQLITE_ROW)
    {
        const char* strTabName = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 0));

        /* 拼接查询语句用于查找指定 ID */
        pchSelectSQL = sqlite3_mprintf("SELECT * FROM \"%s\" WHERE ID = ?;", strTabName);
        if (pchSelectSQL == nullptr)
        {
            dlog(LOG_ERROR, "创建查询命令失败");
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
                stOutInfo.nId              = sqlite3_column_int(pSelectStmt, 0);
                stOutInfo.nClassId         = sqlite3_column_int(pSelectStmt, 1);
                stOutInfo.nMemberId        = sqlite3_column_int(pSelectStmt, 2);
                stOutInfo.strName          = reinterpret_cast<const char*>(sqlite3_column_text(pSelectStmt, 3));
                stOutInfo.strPicName       = reinterpret_cast<const char*>(sqlite3_column_text(pSelectStmt, 4));
                stOutInfo.strLocalPicPath  = reinterpret_cast<const char*>(sqlite3_column_text(pSelectStmt, 5));
                stOutInfo.strRemotePicPath = reinterpret_cast<const char*>(sqlite3_column_text(pSelectStmt, 6));
                stOutInfo.strPicMd5        = reinterpret_cast<const char*>(sqlite3_column_text(pSelectStmt, 7));
                stOutInfo.strPicDate       = reinterpret_cast<const char*>(sqlite3_column_text(pSelectStmt, 8));
                stOutInfo.nIdentity        = sqlite3_column_int(pSelectStmt, 9);
                stOutInfo.fConfidence      = sqlite3_column_double(pSelectStmt, 10);

                const void* pBlobData1 = sqlite3_column_blob(pstCountstmt, 11);
                int         nDataSize1 = sqlite3_column_bytes(pstCountstmt, 11);
                stOutInfo.vfData1.clear();
                if (pBlobData1 != nullptr && nDataSize1 > 0)
                {
                    const float* pfDataPtr = reinterpret_cast<const float*>(pBlobData1);
                    stOutInfo.vfData1.assign(pfDataPtr, pfDataPtr + nDataSize1 / sizeof(float));
                }

                const void* pBlobData2 = sqlite3_column_blob(pstCountstmt, 12);
                int         nDataSize2 = sqlite3_column_bytes(pstCountstmt, 12);
                stOutInfo.vfData2.clear();
                if (pBlobData2 != nullptr && nDataSize2 > 0)
                {
                    const float* pfDataPtr = reinterpret_cast<const float*>(pBlobData2);
                    stOutInfo.vfData2.assign(pfDataPtr, pfDataPtr + nDataSize2 / sizeof(float));
                }


                /* 清理和释放 */
                if (pSelectStmt)
                {
                    sqlite3_finalize(pSelectStmt);
                }
                sqlite3_free(pchSelectSQL);

                enRetCode = OK;
                break;
            }
        }
        else
        {
            dlog(LOG_ERROR, "编译查询语句失败: %s", sqlite3_errmsg(m_pDb));
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
        dlog(LOG_ERROR, "找不到该ID-查找失败");
    }

    return enRetCode;
}

/* 查找数据 */
BlError_E FaceSqlite::searchDataById(
    int             nId,
    std::string     strTabName,
    FaceLibsInfo_S& stOutInfo)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    sqlite3_stmt* pstCountstmt = NULL;
    char*         pchDeleteSQL = NULL;

    int       nRet      = 0;
    BlError_E enRetCode = OK;

    const char* pchTmp = nullptr;

    stOutInfo.clear();

    /* 拼接数据 */
    pchDeleteSQL = sqlite3_mprintf(SQL_SELECT_DATA, strTabName.c_str());
    if (pchDeleteSQL == NULL)
    {
        enRetCode = ERR_CREATE;
        goto EXIT;
    }

    /* 编译 SQL 语句 */
    if (sqlite3_prepare_v2(m_pDb, pchDeleteSQL, -1, &pstCountstmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int(pstCountstmt, 1, nId);

        if (sqlite3_step(pstCountstmt) == SQLITE_ROW)
        {
            stOutInfo.nId              = sqlite3_column_int(pstCountstmt, 0);
            stOutInfo.nClassId         = sqlite3_column_int(pstCountstmt, 1);
            stOutInfo.nMemberId        = sqlite3_column_int(pstCountstmt, 2);
            stOutInfo.strName          = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 3));
            stOutInfo.strPicName       = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 4));
            stOutInfo.strLocalPicPath  = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 5));
            stOutInfo.strRemotePicPath = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 6));
            stOutInfo.strPicMd5        = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 7));
            stOutInfo.strPicDate       = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 8));
            stOutInfo.nIdentity        = sqlite3_column_int(pstCountstmt, 9);
            stOutInfo.fConfidence      = sqlite3_column_double(pstCountstmt, 10);

            const void* pBlobData1 = sqlite3_column_blob(pstCountstmt, 11);
            int         nDataSize1 = sqlite3_column_bytes(pstCountstmt, 11);
            stOutInfo.vfData1.clear();
            if (pBlobData1 != nullptr && nDataSize1 > 0)
            {
                const float* pfDataPtr = reinterpret_cast<const float*>(pBlobData1);
                stOutInfo.vfData1.assign(pfDataPtr, pfDataPtr + nDataSize1 / sizeof(float));
            }

            const void* pBlobData2 = sqlite3_column_blob(pstCountstmt, 12);
            int         nDataSize2 = sqlite3_column_bytes(pstCountstmt, 12);
            stOutInfo.vfData2.clear();
            if (pBlobData2 != nullptr && nDataSize2 > 0)
            {
                const float* pfDataPtr = reinterpret_cast<const float*>(pBlobData2);
                stOutInfo.vfData2.assign(pfDataPtr, pfDataPtr + nDataSize2 / sizeof(float));
            }
        }
        else
        {
            dlog(LOG_ERROR, "找不到该ID-查找失败");
            enRetCode = ERR_NOT_EXIST;
            goto EXIT;
        }
    }
    else
    {
        enRetCode = NOK;
        dlog(LOG_ERROR, "编译SQL语句-失败");
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

/* 根据表名查找数据 */
BlError_E FaceSqlite::searchDataByTable(
    std::string                strTabName,
    std::list<FaceLibsInfo_S>& listOutInfo)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_pDb == nullptr)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    std::string   query         = "SELECT * FROM \"" + strTabName + "\";";
    sqlite3_stmt* pstSelectStmt = nullptr;

    BlError_E enRetCode = OK;
    listOutInfo.clear();

    if (sqlite3_prepare_v2(m_pDb, query.c_str(), -1, &pstSelectStmt, nullptr) == SQLITE_OK)
    {
        while (sqlite3_step(pstSelectStmt) == SQLITE_ROW)
        {
            FaceLibsInfo_S stItemInfo;
            stItemInfo.nId              = sqlite3_column_int(pstSelectStmt, 0);
            stItemInfo.nClassId         = sqlite3_column_int(pstSelectStmt, 1);
            stItemInfo.nMemberId        = sqlite3_column_int(pstSelectStmt, 2);
            stItemInfo.strName          = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 3));
            stItemInfo.strPicName       = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 4));
            stItemInfo.strLocalPicPath  = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 5));
            stItemInfo.strRemotePicPath = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 6));
            stItemInfo.strPicMd5        = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 7));
            stItemInfo.strPicDate       = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 8));
            stItemInfo.nIdentity        = sqlite3_column_int(pstSelectStmt, 9);
            stItemInfo.fConfidence      = sqlite3_column_double(pstSelectStmt, 10);

            const void* pBlobData1 = sqlite3_column_blob(pstSelectStmt, 11);
            int         nDataSize1 = sqlite3_column_bytes(pstSelectStmt, 11);
            stItemInfo.vfData1.clear();
            if (pBlobData1 != nullptr && nDataSize1 > 0)
            {
                const float* pfDataPtr = reinterpret_cast<const float*>(pBlobData1);
                stItemInfo.vfData1.assign(pfDataPtr, pfDataPtr + nDataSize1 / sizeof(float));
            }

            const void* pBlobData2 = sqlite3_column_blob(pstSelectStmt, 12);
            int         nDataSize2 = sqlite3_column_bytes(pstSelectStmt, 12);
            stItemInfo.vfData2.clear();
            if (pBlobData2 != nullptr && nDataSize2 > 0)
            {
                const float* pfDataPtr = reinterpret_cast<const float*>(pBlobData2);
                stItemInfo.vfData2.assign(pfDataPtr, pfDataPtr + nDataSize2 / sizeof(float));
            }

            listOutInfo.push_back(stItemInfo);
        }
    }
    else
    {
        dlog(LOG_ERROR, "准备查询语句失败: %s", sqlite3_errmsg(m_pDb));
        enRetCode = NOK;
    }

    if (pstSelectStmt)
    {
        sqlite3_finalize(pstSelectStmt);
    }

    return enRetCode;
}

/* 获取所有数据 */
BlError_E FaceSqlite::getAllData(std::list<FaceLibsInfo_S>& listOutInfo,
                                 std::string                strTabName)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    sqlite3_stmt* pstCountstmt       = NULL;
    const char*   pchSelectTablesSQL = "SELECT name FROM sqlite_master WHERE type='table';";

    BlError_E enRetCode = OK;

    listOutInfo.clear();

    if (strTabName.empty())
    {
        /* 获取所有表名 */
        if (sqlite3_prepare_v2(m_pDb, pchSelectTablesSQL, -1, &pstCountstmt, NULL) == SQLITE_OK)
        {
            while (sqlite3_step(pstCountstmt) == SQLITE_ROW)
            {
                const char* strTabName = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 0));

                /* 对每个表名执行 SELECT ALL 查询 */
                char*         pchSelectSQL  = sqlite3_mprintf(SQL_SELECT_ALL_DATA, strTabName);
                sqlite3_stmt* pstSelectStmt = NULL;

                if (sqlite3_prepare_v2(m_pDb, pchSelectSQL, -1, &pstSelectStmt, NULL) == SQLITE_OK)
                {
                    /* 依次获取查询结果 */
                    while (sqlite3_step(pstSelectStmt) == SQLITE_ROW)
                    {
                        FaceLibsInfo_S stItemInfo;
                        stItemInfo.nId              = sqlite3_column_int(pstSelectStmt, 0);
                        stItemInfo.nClassId         = sqlite3_column_int(pstSelectStmt, 1);
                        stItemInfo.nMemberId        = sqlite3_column_int(pstSelectStmt, 2);
                        stItemInfo.strName          = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 3));
                        stItemInfo.strPicName       = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 4));
                        stItemInfo.strLocalPicPath  = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 5));
                        stItemInfo.strRemotePicPath = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 6));
                        stItemInfo.strPicMd5        = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 7));
                        stItemInfo.strPicDate       = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 8));
                        stItemInfo.nIdentity        = sqlite3_column_int(pstSelectStmt, 9);
                        stItemInfo.fConfidence      = sqlite3_column_double(pstSelectStmt, 10);

                        const void* pBlobData1 = sqlite3_column_blob(pstSelectStmt, 11);
                        int         nDataSize1 = sqlite3_column_bytes(pstSelectStmt, 11);
                        stItemInfo.vfData1.clear();
                        if (pBlobData1 != nullptr && nDataSize1 > 0)
                        {
                            const float* pfDataPtr = reinterpret_cast<const float*>(pBlobData1);
                            stItemInfo.vfData1.assign(pfDataPtr, pfDataPtr + nDataSize1 / sizeof(float));
                        }

                        const void* pBlobData2 = sqlite3_column_blob(pstSelectStmt, 12);
                        int         nDataSize2 = sqlite3_column_bytes(pstSelectStmt, 12);
                        stItemInfo.vfData2.clear();
                        if (pBlobData2 != nullptr && nDataSize2 > 0)
                        {
                            const float* pfDataPtr = reinterpret_cast<const float*>(pBlobData2);
                            stItemInfo.vfData2.assign(pfDataPtr, pfDataPtr + nDataSize2 / sizeof(float));
                        }

                        listOutInfo.push_back(stItemInfo);
                    }
                }
                else
                {
                    dlog(LOG_ERROR, "准备 SELECT 查询语句失败 %s", sqlite3_errmsg(m_pDb));
                    enRetCode = NOK;
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
            dlog(LOG_ERROR, "准备获取表名失败 %s", sqlite3_errmsg(m_pDb));
            enRetCode = NOK;
        }
    }
    else
    {
        /* 对表名执行 SELECT ALL 查询 */
        char*         pchSelectSQL  = sqlite3_mprintf(SQL_SELECT_ALL_DATA, strTabName);
        sqlite3_stmt* pstSelectStmt = NULL;

        if (sqlite3_prepare_v2(m_pDb, pchSelectSQL, -1, &pstSelectStmt, NULL) == SQLITE_OK)
        {
            /* 依次获取查询结果 */
            while (sqlite3_step(pstSelectStmt) == SQLITE_ROW)
            {
                FaceLibsInfo_S stItemInfo;
                stItemInfo.nId              = sqlite3_column_int(pstSelectStmt, 0);
                stItemInfo.nClassId         = sqlite3_column_int(pstSelectStmt, 1);
                stItemInfo.nMemberId        = sqlite3_column_int(pstSelectStmt, 2);
                stItemInfo.strName          = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 3));
                stItemInfo.strPicName       = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 4));
                stItemInfo.strLocalPicPath  = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 5));
                stItemInfo.strRemotePicPath = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 6));
                stItemInfo.strPicMd5        = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 7));
                stItemInfo.strPicDate       = reinterpret_cast<const char*>(sqlite3_column_text(pstSelectStmt, 8));
                stItemInfo.nIdentity        = sqlite3_column_int(pstSelectStmt, 9);
                stItemInfo.fConfidence      = sqlite3_column_double(pstSelectStmt, 10);

                const void* pBlobData1 = sqlite3_column_blob(pstSelectStmt, 11);
                int         nDataSize1 = sqlite3_column_bytes(pstSelectStmt, 11);
                stItemInfo.vfData1.clear();
                if (pBlobData1 != nullptr && nDataSize1 > 0)
                {
                    const float* pfDataPtr = reinterpret_cast<const float*>(pBlobData1);
                    stItemInfo.vfData1.assign(pfDataPtr, pfDataPtr + nDataSize1 / sizeof(float));
                }

                const void* pBlobData2 = sqlite3_column_blob(pstSelectStmt, 12);
                int         nDataSize2 = sqlite3_column_bytes(pstSelectStmt, 12);
                stItemInfo.vfData2.clear();
                if (pBlobData2 != nullptr && nDataSize2 > 0)
                {
                    const float* pfDataPtr = reinterpret_cast<const float*>(pBlobData2);
                    stItemInfo.vfData2.assign(pfDataPtr, pfDataPtr + nDataSize2 / sizeof(float));
                }

                listOutInfo.push_back(stItemInfo);
            }
        }
        else
        {
            dlog(LOG_ERROR, "准备 SELECT 查询语句失败 %s", sqlite3_errmsg(m_pDb));
            enRetCode = NOK;
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

    if (pstCountstmt)
    {
        sqlite3_finalize(pstCountstmt);
        pstCountstmt = NULL;
    }

    return enRetCode;
}

/* 初始化数据库 */
BlError_E FaceSqlite::init_sql()
{
    sqlite3* pDbhandle = NULL;
    int      nRet      = 0;
    bool     bDbExit   = false;

    /* 判断数据库是否存在 */
    std::ifstream fileStream(DB_FACE_DATA_PATH);

    if (fileStream.good())
    {
        bDbExit = true;
        dlog(LOG_TRACE, "记录数据库存在[%s]", DB_FACE_DATA_PATH);
    }
    else
    {
        dlog(LOG_ERROR, "记录数据库不存在[%s]", DB_FACE_DATA_PATH);
    }

    /* 打开数据库 */
    nRet = sqlite3_open(DB_FACE_DATA_PATH, &pDbhandle);
    if (nRet < 0)
    {
        dlog(LOG_ERROR, "打开数据库失败-[%s]", sqlite3_errmsg(pDbhandle));
        return NOK;
    }

    /* 数据库备份 */
    backup_database(pDbhandle, DB_FACE_DATA_BUCKUP_PATH, bDbExit);
    m_pDb = pDbhandle;

    return OK;
}
