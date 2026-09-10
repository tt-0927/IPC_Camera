/**
 * @file LogDatabase.h
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2025-02-11
 *
 * @brief 日志数据库
 */
#pragma once

#include "DbBase.h"
#include "log_define.h"
#include "Singleton.h"

namespace Db
{
    /*日志系统数据库表名定义 */
    constexpr const char *LOG_TABLE_NAME = "log_manage";
    /*日志数据库绝对路径*/
    constexpr const char *LOG_DATABASE_PATH = "/opt/cam/db/log_manage.db";
    /*日志记录字段 - 开始时间*/
    constexpr const char *LOG_FIELD_START_TIME = "start_time";
    /*日志记录字段 - 日志主类型*/
    constexpr const char *LOG_FIELD_TYPE = "type";
    /*日志记录字段 - 日志次类型*/
    constexpr const char *LOG_FIELD_ACTION = "action";
    /*日志记录字段 - 通道名称*/
    // constexpr const char *LOG_FIELD_CHN_NAME = "chn_name";
    /*日志记录字段 - 操作用户*/
    constexpr const char *LOG_FIELD_USER = "user";
    /*日志记录字段 - 主机标识*/
    constexpr const char *LOG_FIELD_HOST = "host";
    /*日志记录字段 - 日志内容*/
    constexpr const char *LOG_FIELD_CONTEXT = "context";

    class LogDatabase : public CSingleton<LogDatabase>
    {
        LogDatabase();

    public:
        ~LogDatabase();
        friend class CSingleton<LogDatabase>;

        /**
         * @brief   : 添加数据
         * @param    {Info_S} &stData [out] 数据
         * @return   {int} <0 失败
         */
        int add(const Log::Info_S &stData);

        /**
         * @brief   : 查找信息数据
         * @param    {Element} &elem [int]: elem 需要查找的内容
         * @param    {vector<Log::Info_S>} &infos [out] 输出信息数据
         * @return   {int} <0 失败
         */
        int find(const Element &elem, std::vector<Log::Info_S> &infos);

        /**
         * @brief   : 查找信息数据
         * @param    {MatchMethods} &methods：匹配方法
         * @param    {vector<Log::Info_S>} &infos [out] 输出信息数据 
         * @return   {int} <0 失败
         */ 
        int find(const MatchMethods &methods, std::vector<Log::Info_S> &infos);

        /**
         * @brief   : 统计表中符合特定条件的记录数量
         * @param    {MatchMethods} &methods：匹配方法
         * @param    {int} &nCount：记录数量
         * @param    {string} field：需要记录数量的字段
         * @return   {int} <0 失败
         */
        int get_count(const MatchMethods &methods, int &nCount, const std::string field);

        /**
         * @brief   : 更新信息
         * @param    {Item} &item [int]: item 需要更新的信息
         * @param    {MatchMethods} &methods [in]匹配方式
         * @return   {int} <0 失败
         */
        int update(const Item &item, const MatchMethods &methods);

        /**
         * @brief   : 删除数据
         * @param    {Item} &item：需要删除条目的相关信息
         * @return   {int} <0 失败
         */
        int del(const Item &item);

    private:
        /**
         * @brief   : 创建表
         * @param    {string} tableName：表名
         * @return   {int} <0 失败
         */
        int create(std::string tableName);

    private:
        /*数据库表句柄指针*/
        CDbBase *m_database = nullptr;
    };

} /* namespace Db */