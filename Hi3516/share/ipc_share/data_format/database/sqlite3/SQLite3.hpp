/**
 * @FilePath     : SQLite3.hpp
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2025-01-16 00:00:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-13 15:03:25
 * @Description  : SQLite3封装接口
 */

#pragma once

#include "sqlite3.h"
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <chrono>
#include <functional>
#include "dlog.h"
#include <condition_variable>  // C++11
class SQLite3
{
public:
    /**
     * @brief 构造函数，初始化提交间隔为5秒，退出标志为false，并启动一个线程用于周期性提交
     */
    SQLite3();
    /**
     * @brief 析构函数
     */
    ~SQLite3();
    /**
     * @brief 初始化数据库连接
     * @param path 数据库文件路径
     * @return 成功返回0，失败返回-1
     */
    int init(std::string path);
    /**
     * @brief 关闭数据库连接
     */
    void deinit();
    /**
     * @brief 创建表
     * @param tables 表的SQL语句
     * @return 成功返回0，失败返回-1
     */
    int create_table(std::string tables);
    /**
     * @brief 设置提交间隔
     * @param interval 提交间隔秒数
     */
    void set_commitInterval(int interval);
    /**
     * @brief 处理SQL语句
     * @param sql SQL语句
     * @return 成功返回0，失败返回-1
     */
    int deal_sql(std::string sql);

    /**
     * @brief 设置SQLite同步模式（PRAGMA synchronous）
     * @param nMode 同步模式：0=OFF，1=NORMAL，2=FULL（SQLite默认）
     * @return 成功返回0，失败返回-1
     * @note NORMAL 模式仅在检查点时同步数据库文件，提交延迟显著低于 FULL，
     *       适用于可容忍断电丢失最近一次提交的元数据场景（如抓图索引）
     */
    int set_synchronous_mode(int nMode);
    /**
     * @brief 获取最后插入的ID
     * @return 最后插入的ID
     */
    int get_lastInsertId();
    /**
     * @brief 执行SQL并获取数据
     * @param sql 查询SQL语句
     * @param ppOutData 输出结果指针
     * @param nTotal 总行数
     * @param nLine 总列数
     * @return 成功返回0，失败返回-1
     */
    int get_data(std::string sql, char ***ppOutData, int &nTotal, int &nLine);
    /**
     * @brief 释放查询结果占用的内存
     * @param ppData 查询结果指针
     */
    void release_data(char **ppData);

    /**
     * @brief 获取所有表名
     * @param sql SQL语句
     * @return 成功返回0，失败返回-1
     */
    std::vector<std::string> get_all_tables(const char *sql);

    /**
     * @brief 获取指定的表中，对应的字段数据
     * @param sql SQL语句
     * @return 成功返回对应数据
     */
    std::vector<std::string> get_column_data(const std::string &sql);

    /**
     * @brief 删除对应的表中指定的字段的信息
     * @param sql SQL语句
     * @param targetFile 删除的文件名字
     * @return 成功返回true，失败返回false
     */
    bool delete_record_by_field(const std::string &sql, const std::string &targetFile);

    /**
     * @brief 获取当前表格中有多少条数据
     * @param sql SQL语句
     * @return 返回表格中数据数量
     */
    int get_table_data_count(const std::string &sql);

    /**
     * @brief 删除指定表格
     * @param sql SQL语句
     * @return 成功返回true，失败返回false
     */
    bool del_table(const std::string &sql);

private:
    /**
     * @brief 延迟处理SQL语句
     * @param sql SQL语句
     * @return 成功返回0，失败返回-1
     */
    int delay_deal(std::string sql);
    /**
     * @brief 快速处理SQL语句，立即提交
     * @param sql SQL语句
     * @return 成功返回0，失败返回-1
     */
    int quick_deal(std::string sql);
    /**
     * @brief 添加保存点并处理SQL语句
     * @param sql SQL语句
     * @return 成功返回0，失败返回-1
     */
    int add_sp(std::string sql);
    /**
     * @brief 添加保存点
     * @param name 保存点名称
     * @return 成功返回0，失败返回-1
     */
    int add_point(std::string &name);
    /**
     * @brief 回滚到指定保存点
     * @param name 保存点名称
     * @return 成功返回0，失败返回-1
     */
    int rollback_point(std::string &name);
    /**
     * @brief 释放保存点
     * @param name 保存点名称
     * @return 成功返回0，失败返回-1
     */
    int release_point(std::string &name);
    /**
     * @brief 开始事务
     * @return 成功返回0，失败返回-1
     */
    int begin_transaction();
    /**
     * @brief 添加事务
     * @param sql SQL语句
     * @return 成功返回0，失败返回-1
     */
    int add_transaction(std::string sql);
    /**
     * @brief 提交事务
     * @return 成功返回0，失败返回-1
     */
    int commit_transaction();
    /**
     * @brief 回滚事务
     * @return 成功返回0，失败返回-1
     */
    int rollback_transaction();

    /**
     * @brief 周期性提交事务的线程函数
     */
    void run_commit();

private:
    bool m_bBeginTransaction = false;  // 事务开始标志
    std::vector<std::string> m_points; // 保存点列表
    std::string m_path;                // 数据库文件路径
    sqlite3 *m_handle = nullptr;       // SQLite数据库句柄
    int m_nCommitInterval = 5;         // 提交间隔秒数
    bool m_bExit = false;              // 退出标志
    std::thread m_tid;                 // 提交线程
    std::mutex m_mutex;                // 互斥锁
    std::condition_variable m_cv;
};