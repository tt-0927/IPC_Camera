/*
 * @FilePath     : FaceDataDB.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-06 14:11:39
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-08-07 08:53:49
 * @Description  : 人脸特征数据库
 */
#include "FaceDataDB.hpp"

#include <cstring>
#include <fstream>

using namespace FR_NS;

/* 观看记录本地推荐数据库的路径 */
#define DB_FACE_DATA_PATH        ("/opt/bl/db/FaceDataDB.db")
#define DB_FACE_DATA_BUCKUP_PATH ("/opt/bl/db/.backup.FaceDataDB.db")

/* play_record观看记录表 */
#define DB_FACE_DATA_TABLE_NAME ("featureInfo")

/* 创建数据表 */
#define CREATE_FACE_DATA_SQL ("CREATE TABLE IF NOT EXISTS %s (\
                                        ID      INTEGER PRIMARY KEY AUTOINCREMENT, \
                                        Name    TEXT, \
                                        PicName TEXT, \
                                        PicPath TEXT, \
                                        Data    BLOB, \
                                        CardId  INTEGER \
                                        );")


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

/**
 * @brief: 添加数据
 * @param [string] : 表名
 */
#define SQL_INSERT_DATA ("INSERT INTO %s ( \
                            Name, \
                            PicName, \
                            PicPath, \
                            Data, \
                            CardId) \
                            VALUES (?, ?, ?, ?, ?);")

/**
 * @brief: 更新数据
 * @param [string] : 表名
 */
#define SQL_UPDATE_DATA ("UPDATE %s SET \
                            Name=?, \
                            PicName=?, \
                            PicPath=?, \
                            Data=? \
                            WHERE \
                            ID=?;")

/**
 * @brief: 删除云平台ID数据（0本地上传数据的默认CardId）
 * @param [string] : 表名
 */
#define SQL_DELETE_CARDID_DATA ("DELETE FROM %s WHERE CardId!=0;")

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

/**
 * @brief: 查找数据
 * @param [string] : 表名
 */
#define SQL_SELECT_ALL_DATA_TERM1 ("SELECT * FROM %s DESC \
                                    LIMIT ? OFFSET ?;")
/**
 * @brief: 查找数据
 * @param [string] : 表名
 */
#define SQL_SELECT_ALL_DATA_TERM2 ("SELECT * FROM %s \
                                    WHERE \
                                        Name LIKE '%%' || ? || '%%' ESCAPE '\\' COLLATE NOCASE OR \
                                        PicName LIKE '%%' || ? || '%%' ESCAPE '\\' COLLATE NOCASE OR \
                                        PicPath LIKE '%%' || ? || '%%' ESCAPE '\\' COLLATE NOCASE \
                                    LIMIT ? OFFSET ?;")

CFaceDataDB::CFaceDataDB()
{
    init_sql();
}

CFaceDataDB::~CFaceDataDB()
{
    if (m_pDb != nullptr)
    {
        sqlite3_close(m_pDb);
        m_pDb = nullptr;
    }
}

/* 插入数据 */
BlError_E CFaceDataDB::insertData(FaceDataInfo_S& stInfo)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    sqlite3_stmt* pstCountstmt = NULL;
    char*         pchSelectSQL = NULL;

    int       nRet      = 0;
    BlError_E enRetCode = OK;


    if (beginTransaction())
    {
        try
        {
            /* 拼接数据 */
            pchSelectSQL = sqlite3_mprintf(SQL_INSERT_DATA, DB_FACE_DATA_TABLE_NAME);
            if (pchSelectSQL == NULL)
            {
                enRetCode = ERR_CREATE;
                throw std::runtime_error("创建命令失败");
            }

            /* 编译 SQL 语句 */
            if (sqlite3_prepare_v2(m_pDb, pchSelectSQL, -1, &pstCountstmt, nullptr) == SQLITE_OK)
            {
                sqlite3_bind_text(pstCountstmt, 1, (stInfo.strName).c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(pstCountstmt, 2, (stInfo.strPicName).c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(pstCountstmt, 3, (stInfo.strPicPath).c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_blob(pstCountstmt, 4, stInfo.vfData.data(), static_cast<int>(stInfo.vfData.size() * sizeof(float)), SQLITE_STATIC);
                sqlite3_bind_int(pstCountstmt, 5, stInfo.nCardId);

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
                dlog(LOG_ERROR, "SQL语句 [%s]", pchSelectSQL);
                throw std::runtime_error("编译SQL语句-失败");
            }
        }
        catch (const std::exception& e)
        {
            dlog(LOG_ERROR, "插入数据失败 [%s]", e.what());
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
BlError_E CFaceDataDB::updateData(int nId, FaceDataInfo_S& stInfo)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    sqlite3_stmt* pstCountstmt = NULL;
    char*         pchUpdateSQL = NULL;

    int       nRet      = 0;
    BlError_E enRetCode = OK;


    if (beginTransaction())
    {
        try
        {
            /* 拼接数据 */
            pchUpdateSQL = sqlite3_mprintf(SQL_UPDATE_DATA, DB_FACE_DATA_TABLE_NAME);
            if (pchUpdateSQL == NULL)
            {
                enRetCode = ERR_CREATE;
                throw std::runtime_error("创建命令失败");
            }

            /* 编译 SQL 语句 */
            if (sqlite3_prepare_v2(m_pDb, pchUpdateSQL, -1, &pstCountstmt, 0) == SQLITE_OK)
            {
                sqlite3_bind_text(pstCountstmt, 1, (stInfo.strName).c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(pstCountstmt, 2, (stInfo.strPicName).c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(pstCountstmt, 3, (stInfo.strPicPath).c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_blob(pstCountstmt, 4, stInfo.vfData.data(), static_cast<int>(stInfo.vfData.size() * sizeof(float)), SQLITE_STATIC);
                sqlite3_bind_int(pstCountstmt, 5, nId);

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
                throw std::runtime_error("编译SQL语句-失败");
            }
        }
        catch (const std::exception& e)
        {
            dlog(LOG_ERROR, "插入数据失败 [%s]", e.what());
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
BlError_E CFaceDataDB::deleteData()
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

    if (beginTransaction())
    {
        try
        {
            /* 拼接数据 */
            pchDeleteSQL = sqlite3_mprintf(SQL_DELETE_CARDID_DATA, DB_FACE_DATA_TABLE_NAME);
            if (pchDeleteSQL == NULL)
            {
                enRetCode = ERR_CREATE;
                throw std::runtime_error("创建命令失败");
            }

            /* 编译 SQL 语句 */
            if (sqlite3_prepare_v2(m_pDb, pchDeleteSQL, -1, &pstCountstmt, 0) == SQLITE_OK)
            {

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
                        enRetCode = NOK;
                        throw std::runtime_error("删除数据-提交事务失败");
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
                throw std::runtime_error("编译SQL语句-失败");
            }
        }
        catch (const std::exception& e)
        {
            dlog(LOG_ERROR, "删除数据失败 [%s]", e.what());
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

/* 删除数据 */
BlError_E CFaceDataDB::deleteData(int nId)
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

    if (beginTransaction())
    {
        try
        {
            /* 拼接数据 */
            pchDeleteSQL = sqlite3_mprintf(SQL_DELETE_DATA, DB_FACE_DATA_TABLE_NAME);
            if (pchDeleteSQL == NULL)
            {
                enRetCode = ERR_CREATE;
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
                throw std::runtime_error("编译SQL语句-失败");
            }
        }
        catch (const std::exception& e)
        {
            dlog(LOG_ERROR, "插入数据失败 [%s]", e.what());
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
BlError_E CFaceDataDB::searchData(int nId, FaceDataInfo_S& stOutInfo)
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

    const char*  pchName    = nullptr;
    const void*  pBlobData  = nullptr;
    int          nDataSize  = 0;
    const float* pfDataPtr  = nullptr;
    int          nNumFloats = 0;

    stOutInfo.clear();

    /* 拼接数据 */
    pchDeleteSQL = sqlite3_mprintf(SQL_SELECT_DATA, DB_FACE_DATA_TABLE_NAME);
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
            stOutInfo.nId = sqlite3_column_int(pstCountstmt, 0);

            /* 名字 */
            pchName = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 1));
            if (pchName != nullptr)
            {
                stOutInfo.strName = pchName;
            }
            /* 图片名字 */
            pchName = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 2));
            if (pchName != nullptr)
            {
                stOutInfo.strPicName = pchName;
            }
            /* 图片路径 */
            pchName = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 3));
            if (pchName != nullptr)
            {
                stOutInfo.strPicPath = pchName;
            }
            pBlobData = sqlite3_column_blob(pstCountstmt, 4);
            nDataSize = sqlite3_column_bytes(pstCountstmt, 4);

            /* 数据 */
            stOutInfo.vfData.clear();
            if (pBlobData != nullptr && nDataSize > 0)
            {
                pfDataPtr  = reinterpret_cast<const float*>(pBlobData);
                nNumFloats = nDataSize / sizeof(float);

                stOutInfo.vfData.assign(pfDataPtr, pfDataPtr + nNumFloats);
            }

            stOutInfo.nCardId = sqlite3_column_int(pstCountstmt, 5);
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

/* 查找数据 */
BlError_E CFaceDataDB::searchData(int nCurPageNum, int nPageSize, std::list<FaceDataInfo_S>& listOutInfo)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    if (nCurPageNum < 1 && nPageSize < 0)
    {
        dlog(LOG_ERROR, "参数异常");
        return ERR_PARAM;
    }

    sqlite3_stmt* pstCountstmt = NULL;
    char*         pchDeleteSQL = NULL;

    int       nRet      = 0;
    BlError_E enRetCode = OK;

    FaceDataInfo_S stItemInfo;
    const void*    pBlobData = nullptr;
    int            nDataSize = 0;
    const float*   pfDataPtr = 0;

    /* 计算偏移量 */
    int nOffset = nPageSize * (nCurPageNum - 1);


    listOutInfo.clear();

    /* 拼接数据 */
    pchDeleteSQL = sqlite3_mprintf(SQL_SELECT_ALL_DATA_TERM1, DB_FACE_DATA_TABLE_NAME);
    if (pchDeleteSQL == NULL)
    {
        enRetCode = ERR_CREATE;
        goto EXIT;
    }

    /* 编译 SQL 语句 */
    if (sqlite3_prepare_v2(m_pDb, pchDeleteSQL, -1, &pstCountstmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int(pstCountstmt, 1, nPageSize);
        sqlite3_bind_int(pstCountstmt, 2, nOffset);

        while (sqlite3_step(pstCountstmt) == SQLITE_ROW)
        {
            stItemInfo.nId        = sqlite3_column_int(pstCountstmt, 0);
            stItemInfo.strName    = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 1));
            stItemInfo.strPicName = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 2));
            stItemInfo.strPicPath = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 3));

            pBlobData = sqlite3_column_blob(pstCountstmt, 4);
            nDataSize = sqlite3_column_bytes(pstCountstmt, 4);
            stItemInfo.vfData.clear();
            if (pBlobData != nullptr && nDataSize > 0)
            {
                pfDataPtr = reinterpret_cast<const float*>(pBlobData);
                for (int i = 0; i < nDataSize / sizeof(float); ++i)
                {
                    stItemInfo.vfData.push_back(pfDataPtr[i]);
                }
            }

            stItemInfo.nCardId = sqlite3_column_int(pstCountstmt, 5);

            listOutInfo.push_back(stItemInfo);
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

/* 查找数据 */
BlError_E FR_NS::CFaceDataDB::searchData(
    std::string                strSearchKey,
    int                        nCurPageNum,
    int                        nPageSize,
    std::list<FaceDataInfo_S>& listOutInfo)
{
    /* 自动锁定互斥锁 */
    std::unique_lock<std::mutex> lock(m_mutex);

    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    if (nCurPageNum < 1 && nPageSize < 0)
    {
        dlog(LOG_ERROR, "参数异常");
        return ERR_PARAM;
    }

    sqlite3_stmt* pstCountstmt = NULL;
    char*         pchDeleteSQL = NULL;

    int       nRet      = 0;
    BlError_E enRetCode = OK;

    FaceDataInfo_S stItemInfo;
    const void*    pBlobData = nullptr;
    int            nDataSize = 0;
    const float*   pfDataPtr = 0;

    /* 计算偏移量 */
    int nOffset = nPageSize * (nCurPageNum - 1);


    listOutInfo.clear();

    /* 拼接数据 */
    pchDeleteSQL = sqlite3_mprintf(SQL_SELECT_ALL_DATA_TERM2, DB_FACE_DATA_TABLE_NAME);
    if (pchDeleteSQL == NULL)
    {
        enRetCode = ERR_CREATE;
        goto EXIT;
    }

    /* 编译 SQL 语句 */
    if (sqlite3_prepare_v2(m_pDb, pchDeleteSQL, -1, &pstCountstmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_text(pstCountstmt, 1, (strSearchKey).c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(pstCountstmt, 2, (strSearchKey).c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(pstCountstmt, 3, (strSearchKey).c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(pstCountstmt, 4, nPageSize);
        sqlite3_bind_int(pstCountstmt, 5, nOffset);

        while (sqlite3_step(pstCountstmt) == SQLITE_ROW)
        {
            stItemInfo.nId        = sqlite3_column_int(pstCountstmt, 0);
            stItemInfo.strName    = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 1));
            stItemInfo.strPicName = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 2));
            stItemInfo.strPicPath = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 3));

            pBlobData = sqlite3_column_blob(pstCountstmt, 4);
            nDataSize = sqlite3_column_bytes(pstCountstmt, 4);
            stItemInfo.vfData.clear();
            if (pBlobData != nullptr && nDataSize > 0)
            {
                pfDataPtr = reinterpret_cast<const float*>(pBlobData);
                for (int i = 0; i < nDataSize / sizeof(float); ++i)
                {
                    stItemInfo.vfData.push_back(pfDataPtr[i]);
                }
            }
            stItemInfo.nCardId = sqlite3_column_int(pstCountstmt, 5);

            listOutInfo.push_back(stItemInfo);
        }
    }
    else
    {
        enRetCode = NOK;
        dlog(LOG_ERROR, "编译SQL语句-失败[%s]", pchDeleteSQL);
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
BlError_E CFaceDataDB::getAllData(std::list<FaceDataInfo_S>& listOutInfo)
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

    FaceDataInfo_S stItemInfo;
    const void*    pBlobData = nullptr;
    int            nDataSize = 0;
    const float*   pfDataPtr = 0;


    listOutInfo.clear();

    /* 拼接数据 */
    pchDeleteSQL = sqlite3_mprintf(SQL_SELECT_ALL_DATA, DB_FACE_DATA_TABLE_NAME);
    if (pchDeleteSQL == NULL)
    {
        enRetCode = ERR_CREATE;
        goto EXIT;
    }

    /* 编译 SQL 语句 */
    if (sqlite3_prepare_v2(m_pDb, pchDeleteSQL, -1, &pstCountstmt, 0) == SQLITE_OK)
    {
        while (sqlite3_step(pstCountstmt) == SQLITE_ROW)
        {

            stItemInfo.nId        = sqlite3_column_int(pstCountstmt, 0);
            stItemInfo.strName    = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 1));
            stItemInfo.strPicName = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 2));
            stItemInfo.strPicPath = reinterpret_cast<const char*>(sqlite3_column_text(pstCountstmt, 3));

            pBlobData = sqlite3_column_blob(pstCountstmt, 4);
            nDataSize = sqlite3_column_bytes(pstCountstmt, 4);
            stItemInfo.vfData.clear();
            if (pBlobData != nullptr && nDataSize > 0)
            {
                pfDataPtr = reinterpret_cast<const float*>(pBlobData);
                for (int i = 0; i < nDataSize / sizeof(float); ++i)
                {
                    stItemInfo.vfData.push_back(pfDataPtr[i]);
                }
            }

            stItemInfo.nCardId = sqlite3_column_int(pstCountstmt, 5);

            listOutInfo.push_back(stItemInfo);
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

/* 获取数据总量 */
BlError_E CFaceDataDB::getDataTotal(int& nOutTotal)
{
    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    sqlite3_stmt* pstCountstmt = NULL;
    char*         pchSelectSQL = NULL;

    BlError_E enRetCode = OK;

    pchSelectSQL = sqlite3_mprintf(SQL_GET_TOTAL, DB_FACE_DATA_TABLE_NAME);
    if (pchSelectSQL == NULL)
    {
        dlog(LOG_ERROR, "创建命令失败");
        enRetCode = ERR_CREATE;
        goto EXIT;
    }

    /* 编译SQL语句 */
    if (sqlite3_prepare_v2(m_pDb, pchSelectSQL, -1, &pstCountstmt, NULL) == SQLITE_OK)
    {
        enRetCode = NOK;
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

    if (pchSelectSQL)
    {
        sqlite3_free(pchSelectSQL);
        pchSelectSQL = NULL;
    }

    return enRetCode;
}

/* 添加转义字符 */
std::string CFaceDataDB::add_ESC(const std::string& strInput)
{
    std::string strOutput;
    for (char c : strInput)
    {
        if (c == '_')
        {
            strOutput += '\\';
        }
        strOutput += c;
    }
    return strOutput;
}

/* 错误打印函数 */
BlError_E CFaceDataDB::db_print_errMsg(const char* achCmd, char* pchErrMsg)
{
    if (NULL == achCmd ||
        NULL == pchErrMsg)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }
    dlog(LOG_ERROR, "错误打印=%s  <---> Error=%s", achCmd, pchErrMsg);

    sqlite3_free(pchErrMsg);
    pchErrMsg = NULL;

    return OK;
}

/* 执行事务 */
BlError_E CFaceDataDB::exec_transaction(const std::string& strSelectSQL)
{
    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_UNINIT;
    }

    int       nRet      = 0;
    BlError_E enRetCode = OK;
    char*     pchErrMsg = NULL;

    /* 开始一个事务的SQL语句 */
    nRet = sqlite3_exec(m_pDb, "begin transaction", NULL, NULL, &pchErrMsg);
    if (nRet != SQLITE_OK)
    {
        /* 错误打印 */
        db_print_errMsg(strSelectSQL.c_str(), pchErrMsg);
        enRetCode = NOK;
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
            enRetCode = NOK;
            goto EXIT;
        }
        enRetCode = NOK;
        goto EXIT;
    }

    /* 提交事务 */
    nRet = sqlite3_exec(m_pDb, "commit transaction", NULL, NULL, &pchErrMsg);
    if (nRet != SQLITE_OK)
    {
        /* 错误打印 */
        db_print_errMsg(strSelectSQL.c_str(), pchErrMsg);
        enRetCode = NOK;
        goto EXIT;
    }

EXIT:
    if (pchErrMsg)
    {
        sqlite3_free(pchErrMsg);
        pchErrMsg = NULL;
    }

    return enRetCode;
}

/* 数据库备份 */
BlError_E CFaceDataDB::backup_database(sqlite3* pSq, bool bIsSave)
{
    if (NULL == pSq)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
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

    std::ifstream fileStream(DB_FACE_DATA_BUCKUP_PATH);

    if (fileStream.good())
    {
        nHaveBackup = 1;
        dlog(LOG_TRACE, "备份-记录数据库存在[%s]", DB_FACE_DATA_BUCKUP_PATH);
    }
    else
    {
        dlog(LOG_ERROR, "备份-记录数据库不存在[%s]", DB_FACE_DATA_BUCKUP_PATH);
    }

    if (!nHaveBackup && 0 == bIsSave)
    {
        dlog(LOG_ERROR, "没有备份数据库，无法恢复数据库");
        return OK;
    }

    nRet = sqlite3_open(DB_FACE_DATA_BUCKUP_PATH, &pFile);
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
        dlog(LOG_TRACE, "备份数据成功");
    }
    else
    {
        dlog(LOG_TRACE, "还原数据成功");
    }


    (void)sqlite3_close(pFile);

    return OK;
}

/* 检查数据表是否存在 */
BlError_E CFaceDataDB::check_tableExist(const char* pchTableName)
{
    if (NULL == pchTableName)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

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
        enRetCode = ERR_CREATE;
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
BlError_E CFaceDataDB::create_dataTable(const char* pchTableName)
{
    if (NULL == pchTableName)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    if (NULL == m_pDb)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    BlError_E enRetCode = OK;
    char*     pchSqlCmd = NULL;

    pchSqlCmd = sqlite3_mprintf(CREATE_FACE_DATA_SQL, pchTableName);
    if (pchSqlCmd == NULL)
    {
        dlog(LOG_ERROR, "创建命令失败");
        return ERR_CREATE;
    }

    enRetCode = exec_transaction(pchSqlCmd);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "创建数据表-失败");
    }

    if (pchSqlCmd)
    {
        sqlite3_free(pchSqlCmd);
        pchSqlCmd = NULL;
    }

    return enRetCode;
}

/* 检查字段是否存在，不存在追加 */
BlError_E CFaceDataDB::check_fieldExist(
    const char* pchTableName,
    const char* pchFieldName,
    const char* pchFieldType)
{
    if (NULL == pchFieldName || NULL == pchTableName || NULL == pchFieldType)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
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
        enRetCode = ERR_CREATE;
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
        dlog(LOG_TRACE, "字段[%s]存在", pchFieldName);
        goto EXIT;
    }
    /* 不存在 */
    else if (enRetCode == OK_NOT_EXIST)
    {
        dlog(LOG_TRACE, "字段[%s]不存在，追加字段", pchFieldName);

        /* 字段不存在，执行追加字段操作 */
        pchAddColumnSql = sqlite3_mprintf(SQL_ALTER_TABLE_ADD_COLUMN,
                                          pchTableName,
                                          pchFieldName,
                                          pchFieldType);

        enRetCode = exec_transaction(pchAddColumnSql);
        if (enRetCode < OK)
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

/* 初始化数据库 */
BlError_E CFaceDataDB::init_sql()
{
    BlError_E enRetCode        = OK;
    sqlite3*  pDbhandle        = NULL;
    int       nIndex           = 0;
    int       nRet             = 0;
    bool      bDbExit          = false;
    char      achTableName[16] = { 0 };

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
    backup_database(pDbhandle, bDbExit);
    m_pDb = pDbhandle;

    /* 检验数据库表是否存在 */
    enRetCode = check_tableExist(DB_FACE_DATA_TABLE_NAME);
    if (enRetCode >= OK)
    {
        /* 不存在 */
        if (enRetCode == OK_NOT_EXIST)
        {
            dlog(LOG_ERROR, "数据表不存在");
            /* 创建表 */
            enRetCode = create_dataTable(DB_FACE_DATA_TABLE_NAME);
            if (enRetCode < OK)
            {
                return enRetCode;
            }
        }
        else
        {
            dlog(LOG_TRACE, "数据表存在");
        }

        /* 校验表格字段 */
        check_fieldExist(DB_FACE_DATA_TABLE_NAME, "ID", "INTEGER PRIMARY KEY AUTOINCREMENT");
        check_fieldExist(DB_FACE_DATA_TABLE_NAME, "Name", "TEXT");
        check_fieldExist(DB_FACE_DATA_TABLE_NAME, "PicName", "TEXT");
        check_fieldExist(DB_FACE_DATA_TABLE_NAME, "PicPath", "TEXT");
        check_fieldExist(DB_FACE_DATA_TABLE_NAME, "Data", "BLOB");
        check_fieldExist(DB_FACE_DATA_TABLE_NAME, "CardId", "INTEGER");
    }
    else
    {
        dlog(LOG_ERROR, "检验数据库表是否存在-失败");
    }

    return OK;
}

bool CFaceDataDB::beginTransaction()
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

bool CFaceDataDB::commitTransaction()
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

bool CFaceDataDB::rollbackTransaction()
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
