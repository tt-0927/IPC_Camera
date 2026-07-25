/**
 * @file SQLite3.cpp
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2025-01-16
 *
 * @brief SQLite3封装接口
 */

#include "SQLite3.hpp"
#include "dlog.h"

/**
 * @brief 构造函数，初始化提交间隔为5秒，退出标志为false，并启动一个线程用于周期性提交
 *  注意: 使用间隔为5秒的提交方式，数据量过大,提交缓慢, 会导致提交后,再添加显示数据库锁定
 */
SQLite3::SQLite3()
    : m_nCommitInterval(0), m_bExit(false)
{
    /* 在构造函数体中启动线程，确保 m_mutex 和 m_cv 已完成构造。 */
    m_tid = std::thread(&SQLite3::run_commit, this);
}
/**
 * @brief 析构函数
 */
SQLite3::~SQLite3()
{
    // m_bExit = true;
    deinit();
}
/**
 * @brief 初始化数据库连接
 * @param path 数据库文件路径
 * @return 成功返回0，失败返回-1
 */
int SQLite3::init(std::string path)
{
    if(m_handle)
	{
		return 0;
	}
    m_path = std::move(path);
    int nRet = sqlite3_open(m_path.c_str(), &m_handle);
    if (nRet != SQLITE_OK)
    {
        return -1;
    }
    return 0;
}
/**
 * @brief 关闭数据库连接
 */
void SQLite3::deinit()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        /* 在锁保护下设置退出标志，避免数据竞争 */
        m_bExit = true;

        if (m_bBeginTransaction)
        {
            // rollback_transaction();  // 终止事务
            commit_transaction();
        }
    }

    /* 修改共享状态后再唤醒提交线程 */
    m_cv.notify_one();
    if (m_tid.joinable())
    {
        m_tid.join();
    }

    if (m_handle)
    {
        sqlite3_close(m_handle);
        m_handle = nullptr;
    }
}
/**
 * @brief 创建表
 * @param tables 表的SQL语句
 * @return 成功返回0，失败返回-1
 */
int SQLite3::create_table(std::string tables)
{
    return quick_deal(std::move(tables));
}
/**
 * @brief 设置提交间隔
 * @param interval 提交间隔秒数
 */
void SQLite3::set_commitInterval(int interval)
{
    m_nCommitInterval = interval;
    if (m_bBeginTransaction)
    {
        commit_transaction();
    }
}
/**
 * @brief 处理SQL语句
 * @param sql SQL语句
 * @return 成功返回0，失败返回-1
 */
int SQLite3::deal_sql(std::string sql)
{
    dlog_debug("DB sql=%s\n", sql.c_str());
    if (m_nCommitInterval <= 0)
    {
        return quick_deal(std::move(sql));
    }
    else
    {
        return delay_deal(std::move(sql));
    }
}
/**
 * @brief 获取最后插入的ID
 * @return 最后插入的ID
 */
int SQLite3::get_lastInsertId()
{
    sqlite3_int64 lastId = sqlite3_last_insert_rowid(m_handle);
    return lastId;
}
/**
 * @brief 执行SQL并获取数据
 * @param sql 查询SQL语句
 * @param ppOutData 输出结果指针
 * @param nTotal 总行数
 * @param nLine 总列数
 * @return 成功返回0，失败返回-1
 */
int SQLite3::get_data(std::string sql, char ***ppOutData, int &nTotal, int &nLine)
{
    int nRet = 0;
    int &nRow = nTotal;
    int &nColumn = nLine;
    char *pErrMsg = NULL;
    nRet = sqlite3_get_table(m_handle, sql.c_str(), ppOutData, &nRow, &nColumn, &pErrMsg);
    if (nRet != SQLITE_OK)
    {
        if (pErrMsg)
        {
            dlog_error("DB sql=%s  <---> Error=%s\n", sql.c_str(), pErrMsg);
            sqlite3_free(pErrMsg);
            return -1;
        }
    }
    return 0;
}
/**
 * @brief 释放查询结果占用的内存
 * @param ppData 查询结果指针
 */
void SQLite3::release_data(char **ppData)
{
    if (ppData)
    {
        sqlite3_free_table(ppData);
    }
}
/**
 * @brief 延迟处理SQL语句
 * @param sql SQL语句
 * @return 成功返回0，失败返回-1
 */
int SQLite3::delay_deal(std::string sql)
{
    if (m_nCommitInterval <= 0)
    {
        dlog_error("delay deal not enable\n", sql.c_str());
        return -1;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_bBeginTransaction)
    {
        int nRet = begin_transaction();
        if (nRet < 0)
        {
            return nRet;
        }
    }
    int nRet = add_transaction(std::move(sql));
    if (nRet < 0)
    {
        rollback_transaction();
        return -1;
    }
    return nRet;
}
/**
 * @brief 快速处理SQL语句，立即提交
 * @param sql SQL语句
 * @return 成功返回0，失败返回-1
 */
int SQLite3::quick_deal(std::string sql)
{
    int nRet = begin_transaction();
    if (nRet < 0)
    {
        return nRet;
    }
    nRet = add_transaction(std::move(sql));
    if (nRet < 0)
    {
        rollback_transaction();
        return -1;
    }
    return commit_transaction();
}
/**
 * @brief 添加保存点并处理SQL语句
 * @param sql SQL语句
 * @return 成功返回0，失败返回-1
 */
int SQLite3::add_sp(std::string sql)
{
    std::string sp;
    int nRet = add_point(sp);
    if (nRet < 0)
    {
        return -1;
    }
    /* 执行sql */
    nRet = add_transaction(sql);
    /* 回滚点 */
    if (nRet < 0)
    {
        rollback_point(sp);
        return -1;
    }
    /* 提交点 */
    nRet = release_point(sp);
    return nRet;
}
/**
 * @brief 添加保存点
 * @param name 保存点名称
 * @return 成功返回0，失败返回-1
 */
int SQLite3::add_point(std::string &name)
{
    /* 保存点 */
    char *pErrMsg = nullptr;
    name = "sp" + std::to_string(m_points.size());
    std::string cmd = "SAVEPOINT " + name;
    int nRet = sqlite3_exec(m_handle, cmd.c_str(), 0, 0, &pErrMsg);
    if (nRet != SQLITE_OK)
    {
        dlog_error("add point sp %s failed, code %d error: %s", name.c_str(), nRet, pErrMsg);
        sqlite3_free(pErrMsg);
        return -1;
    }
    return 0;
}
/**
 * @brief 回滚到指定保存点
 * @param name 保存点名称
 * @return 成功返回0，失败返回-1
 */
int SQLite3::rollback_point(std::string &name)
{
    /* 回滚点 */
    char *pErrMsg = nullptr;
    std::string cmd = "ROLLBACK TO " + name;
    int nRet = sqlite3_exec(m_handle, cmd.c_str(), 0, 0, &pErrMsg);
    if (nRet != SQLITE_OK)
    {
        dlog_error("rollback sp %s failed, code %d error: %s", name.c_str(), nRet, pErrMsg);
        sqlite3_free(pErrMsg);
        return -1;
    }
    return 0;
}
/**
 * @brief 释放保存点
 * @param name 保存点名称
 * @return 成功返回0，失败返回-1
 */
int SQLite3::release_point(std::string &name)
{
    /* 提交点 */
    char *pErrMsg = nullptr;
    std::string cmd = "RELEASE " + name;
    int nRet = sqlite3_exec(m_handle, cmd.c_str(), 0, 0, &pErrMsg);
    if (nRet != SQLITE_OK)
    {
        dlog_error("release sp %s failed, code %d error: %s", name.c_str(), nRet, pErrMsg);
        sqlite3_free(pErrMsg);
        rollback_point(name);
        return -1;
    }
    m_points.push_back(std::move(name));
    return 0;
}
/**
 * @brief 开始事务
 * @return 成功返回0，失败返回-1
 */
int SQLite3::begin_transaction()
{
    char *pErrMsg = nullptr;
    int nRet = sqlite3_exec(m_handle, "begin transaction", 0, 0, &pErrMsg);
    if (nRet != SQLITE_OK)
    {
        dlog_error("begin transaction failed, code %d error: %s", nRet, pErrMsg);
        return -1;
    }
    m_bBeginTransaction = true;
    return 0;
}
/**
 * @brief 添加事务
 * @param sql SQL语句
 * @return 成功返回0，失败返回-1
 */
int SQLite3::add_transaction(std::string sql)
{
    char *pErrMsg = nullptr;
    int nRet = sqlite3_exec(m_handle, sql.c_str(), 0, 0, &pErrMsg);
    if (nRet != SQLITE_OK)
    {
        dlog_error("add transaction sql %s failed, code %d error: %s", sql.c_str(), nRet, pErrMsg);
        sqlite3_free(pErrMsg);
        return -1;
    }
    return 0;
}
/**
 * @brief 提交事务
 * @return 成功返回0，失败返回-1
 */
int SQLite3::commit_transaction()
{
    char *pErrMsg = nullptr;
    int nRet = sqlite3_exec(m_handle, "commit transaction", 0, 0, &pErrMsg);
    if (nRet != SQLITE_OK)
    {
        dlog_error("commit transaction failed, code %d error: %s", nRet, pErrMsg);
        sqlite3_free(pErrMsg);
        rollback_transaction();
        return -1;
    }
    m_bBeginTransaction = false;
    m_points.clear();
    return 0;
}
/**
 * @brief 回滚事务
 * @return 成功返回0，失败返回-1
 */
int SQLite3::rollback_transaction()
{
    char *pErrMsg = nullptr;
    int nRet = sqlite3_exec(m_handle, "rollback transaction", 0, 0, &pErrMsg);
    if (nRet != SQLITE_OK)
    {
        dlog_error("rollback transaction failed, code %d error: %s", nRet, pErrMsg);
        sqlite3_free(pErrMsg);
        return -1;
    }
    m_bBeginTransaction = false;
    return 0;
}

std::vector<std::string> SQLite3::get_all_tables(const char *sql)
{
    sqlite3_stmt *stmt = nullptr;
    std::vector<std::string> tables;
    if(sql == NULL)
    {
        return tables;
    }
    
	if (sqlite3_prepare_v2(m_handle, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
		    std::string table = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            if (table != "sqlite_sequence")
            {
                tables.push_back(table);
            }
        }
    }
    if(stmt)
    {
        sqlite3_finalize(stmt);
    }
    return tables;
}

// 获取指定表的特定列数据
std::vector<std::string> SQLite3::get_column_data(const std::string &sql)
{
    std::vector<std::string> result;

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_handle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
        dlog_error("SQL错误: ", sqlite3_errmsg(m_handle));
        return result;
    }

    // int rowCount = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const unsigned char *textPtr = sqlite3_column_text(stmt, 0);
        std::string value = (textPtr) ? reinterpret_cast<const char *>(textPtr) : "";
        result.push_back(value);
        // rowCount++;
    }

    // dlog_debug("获取了%d行数据", rowCount);

    sqlite3_finalize(stmt);
    return result;
}

bool SQLite3::delete_record_by_field(const std::string &sql, const std::string &targetFile)
{
    sqlite3_stmt *stmt = nullptr;

    // 准备语句
    if (sqlite3_prepare_v2(m_handle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
        dlog_error("准备 DELETE 语句失败: ", sqlite3_errmsg(m_handle));
        return false;
    }

    // 绑定参数（索引从1开始）
    int bindResult = sqlite3_bind_text(stmt, 1, targetFile.c_str(), -1, SQLITE_STATIC);
    if (bindResult != SQLITE_OK)
    {
        dlog_error("绑定参数失败: %s", sqlite3_errmsg(m_handle));
        sqlite3_finalize(stmt);
        return false;
    }

    // 执行语句
    int stepResult = sqlite3_step(stmt);
    bool success = (stepResult == SQLITE_DONE);

    // 检查执行结果
    if (!success)
    {
        dlog_error("执行 DELETE 失败: ", sqlite3_errmsg(m_handle));
    }

    // 获取受影响的行数
    int affected = sqlite3_changes(m_handle);

    // 清理语句
    sqlite3_finalize(stmt);

    // 处理结果
    if (affected == 0)
    {
        dlog_info("未找到要删除的记录");
        return false;
    }

    return success;
}

// 返回指定表的记录条数，出错返回 -1
int SQLite3::get_table_data_count(const std::string &sql)
{
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(m_handle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
        return -1;
    }

    int64_t count = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        count = sqlite3_column_int64(stmt, 0); // 第 0 列就是 COUNT(*)
    }

    sqlite3_finalize(stmt);
    return count;
}

// 删除指定表；成功返回 true，失败打印错误并返回 false
bool SQLite3::del_table(const std::string &sql)
{
    char *errMsg = nullptr;

    if (sqlite3_exec(m_handle, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        dlog_error("删除表失败:%s", errMsg);
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

/**
 * @brief 周期性提交事务的线程函数
 */
void SQLite3::run_commit()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    while (!m_bExit)
    {
        if (m_nCommitInterval <= 0)
        {
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            lock.lock();
            continue;
        }

        // 等待超时或唤醒
        std::cv_status status = m_cv.wait_for(
            lock, 
            std::chrono::seconds(m_nCommitInterval)
        );

        if (m_bExit)
        {
            break;  // 要退出
        }
        // 超时了，执行commit
        if (status == std::cv_status::timeout && m_bBeginTransaction)
        {
            lock.unlock();
            commit_transaction();
            lock.lock();
        }
    }
}