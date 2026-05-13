/**
 * @FilePath     : record_file_database.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-10-29
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-08 17:00:20
 * @Description  : 录制文件数据库
 */

#pragma once

#include <map>
#include "DbBase.h"
#include "record_define.h"
#include "path_define.h"
#include "Singleton.h"

using namespace Db;

namespace Db
{
    constexpr const char *RECORD_FILE_TABLE_NAME = "record_file_manage";
    constexpr const char *RECORD_FILE_DATABASE_PATH = RECORD_DATABASE_PATH;
    constexpr const char *RECORD_FILE_FIELD_CHN_ID = "chn_id";
    constexpr const char *RECORD_FILE_FIELD_PATH = "path";
    constexpr const char *RECORD_FILE_FIELD_FILENAME = "filename";
    constexpr const char *RECORD_FILE_FIELD_SIZE = "size";
    constexpr const char *RECORD_FILE_FIELD_TYPE = "type";
    constexpr const char *RECORD_FILE_FIELD_IS_LOCK = "is_lock";
    constexpr const char *RECORD_FILE_FIELD_STATUS = "status";
    constexpr const char *RECORD_FILE_FIELD_CREATE_TIME = "create_time";
    constexpr const char *RECORD_FILE_FIELD_MODIFY_TIME = "modify_time";
    constexpr const char *RECORD_FILE_FIELD_DURATION = "duration";
    /* ts用 */
    constexpr const char *RECORD_FILE_FIELD_FILE_INDEX = "file_index";

    constexpr const char *RECORD_DIR_INFO_TABLE_NAME = "record_dir_info_manage";
    constexpr const char *INFO_TS_COUNT = "ts_count";
    constexpr const char *INFO_TS_TOTAL_SIZE = "ts_total_size";

    class RecordFileDatabase : public CSingleton<RecordFileDatabase>
    {
        RecordFileDatabase();
    public:

        ~RecordFileDatabase();
        friend class CSingleton<RecordFileDatabase>;

        /*
         * @description: 初始化指定日期表格
         * @param[out]: stDate 日期
         * @return:  <0 失败
         */
        int init_sub(std::string strDate);

        /*
         * @description: 添加数据
         * @param[out]: stData 数据
         * @return:  <0 失败
         */
        int add(const Record_NS::FileInfo_S &stData);
        int add(const Record_NS::TsFileInfo_S &stData);
        int add(const Record_NS::RecordDirInfo_S &stInfo);


        /*
         * @description: 查找信息数据
         * @param[int]: elem 需要查找的内容
         * @param[out]: infos 输出信息数据
         * @return:  <0 失败
         */
        int find(const Element &elem, std::vector<Record_NS::FileInfo_S> &infos);
        int find(const MatchMethods &methods, std::vector<Record_NS::FileInfo_S> &infos);
        int find(const Element &elem, std::vector<Record_NS::TsFileInfo_S> &infos);
        int find(const MatchMethods &methods, std::vector<Record_NS::TsFileInfo_S> &infos, std::string strTargetTableName = std::string());
        int find(const Element &elem, std::vector<Record_NS::RecordDirInfo_S> &infos);
        int find(const MatchMethods &methods, std::vector<Record_NS::RecordDirInfo_S> &infos);

        int find(std::string cmd, std::vector<Record_NS::TsFileInfo_S> &infos);
        int get_count(const MatchMethods &methods, int &nCount, const std::string field);
        int get_subDataCount(const MatchMethods &methods, int &nCount, const std::string field, std::string strTargetTableName = std::string());
        /*
         * @description: 更新信息
         * @param[int]: item 需要更新的信息
         * @param[int]: methods 匹配方式
         * @param[std::string]: strTargetTableName 目标表名
         * @return:  <0 失败
         */
        int update(const Item &item, const MatchMethods &methods, std::string strTargetTableName = std::string());
        int update(Record_NS::RecordDirInfo_S &stInfo);
        /*
         * @description: 删除数据
         * @param[int]: item 需要删除条目的相关信息
         * @return:  <0 失败
         */
        int del(const Item &item);

        /**
         * @brief: 删除数据
         * @param[in]: methods 匹配方法
         * @param[in]: strTargetTableName 指定的表
         * @return: <0 失败
         */
        int del(const MatchMethods &methods, std::string strTargetTableName = std::string());

        /**
        * @brief 根据通道id获取录制目录相关信息
        * @param stnfo 录制目录相关信息 
        * @return int <0:失败
        */
        int get_itemInfo(Record_NS::RecordDirInfo_S& stInfo);

        /*
         * @description: 获取所有表名
         * @return:  <0 失败
         */
        std::vector<std::string> get_all_tables();

        /*
         * @description: 获取指定的表中，对应的字段数据
         * @param[string]: tableName 表名
         * @param[string]: columnName 目标字段
         * @param[int]: nMaxRows 获取列字段数据的数量
         * @return:  <0 失败
         */
        std::vector<std::string> get_column_data(const std::string &tableName, const std::string &targetField, int nMaxRows);

        /*
         * @description: 删除对应的表中指定的字段的信息
         * @param[string]: tableName 表名
         * @param[string]: targetFile 要删除的文件信息
         * @param[string]: targetField 目标字段
         * @return:  <0 失败
         */
        bool delete_record_by_field(const std::string &tableName, const std::string &targetFile, const std::string &targetField);

        /*
         * @description: 获取当前表格中有多少条数据
         * @param[string]: tableName 表名
         * @return:  <0 失败
         */
        int get_table_data_count(const std::string &tableName);

        /*
         * @description: 删除指定表格
         * @param[string]: tableName 表名
         * @return:  <0 失败
         */
        int del_table(const std::string &tableName);

        /**
        * @brief 清空指定表格数据
        * @param stnfo 抓图目录相关信息
        * @return int <0:失败
        */
        int clear_table(const std::string &tableName, bool resetAutoInc = true);

        /**
        * @brief 初始化
        * @return 0::成功 int <0:失败
        */
        int init();

        /**
        * @brief 去初始化
        * @return 0::成功 int <0:失败
        */
        int deinit();

    private:
        /**
        * @brief 创建数据表
        * @param tableName 表名
        * @param bAddTableKey 是否添加字段
        * @return int <0:失败
        */
        int create(std::string tableName, bool bAddTableKey = true);
        int create_sub(std::string tableName, bool bAddTableKey = true);

        /**
        * @brief 内部获取或创建子表句柄
        * @param tableName 表名
        * @return CDbBase*
        */
        CDbBase* get_sub_handle(std::string tableName);

    private:
        CDbBase m_database;
        CDbBase m_recordDirDatabase;
        std::map<std::string, CDbBase*> m_subDbMap;
        std::mutex m_mutex;
    };

} /* namespace Db */