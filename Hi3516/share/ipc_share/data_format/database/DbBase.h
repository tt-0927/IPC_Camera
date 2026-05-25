/*
 *  File Name: DbBase.h
 *  Created on: 2022年11月02日
 *  Author: zjc
 *  description: 数据库
 */

#ifndef DB_BASE_H_
#define DB_BASE_H_
#include "variant.hpp"
#include <string>
#include "dlog.h"

extern "C"
{
#include "db_sqlite_control.h"
}
#include <string>
#include <vector>
#include "SQLite3.hpp"

#define TO_TAG(str) (str##_t)
namespace Db
{
    /* 字符最大长度 */
    constexpr const int MAX_DB_STRING_SIZE = 2048;

    constexpr const char *DB_COMMON_FIELD_ID = "id";
    /* 扩展字段，默认添加 */
    constexpr const char *DB_COMMON_FIELD_RESERVE1 = "reserve1";
    constexpr const char *DB_COMMON_FIELD_RESERVE2 = "reserve2";
    constexpr const char *DB_COMMON_FIELD_RESERVE3 = "reserve3";
    constexpr const char *DB_COMMON_FIELD_RESERVE4 = "reserve4";
    /* 联合体 */
    using FieldValue = mpark::variant<int, std::string>;

    enum FieldVelueType_E
    {
        /* int型 */
        VALUE_TYPE_INT,
        /* string型 */
        VALUE_TYPE_STR
    };


    /* 查找条件枚举 */
    enum Criterion_E
    {
        /* 无条件 */
        FIND_CRITERION_NONE,
        /* 等于 */
        FIND_CRITERION_EQ,
        /* 不等于 */
        FIND_CRITERION_NE,
        /* 大于 */
        FIND_CRITERION_GT,
        /* 大于等于 */
        FIND_CRITERION_GE,
        /* 小于 */
        FIND_CRITERION_IT,
        /* 小等于 */
        FIND_CRITERION_IE,
        /* 而且 */
        FIND_CRITERION_AND,
        /* 或者， value or value or value */
        FIND_CRITERION_OR,
        /* 或者, 带括号, (value or value or value) */
        FIND_CRITERION_OR_P,
        /* 模糊 */
        FIND_CRITERION_LIKE,
    };
    /* 键值（字段，值） */
    using KeyValue = std::pair<std::string, FieldValue>;

    /* 元素，键值别名 */
    using Element = KeyValue;

    /* 匹配方法 */
    struct FindMethod_S
    {
        FindMethod_S(Element iElem, Criterion_E enICriterion, Criterion_E enIAndOr = FIND_CRITERION_NONE)
            : elem(iElem),
              enCriterion(enICriterion),
              enAndOr(enIAndOr)
        {
        }
        /* 键值 */
        Element elem;
        /* 条件 */
        Criterion_E enCriterion;
        /* 是否与下一个条件相关 */
        Criterion_E enAndOr;
    };
    /* 表格元素 */
    using TableKey = std::pair<std::string, std::string>;

    /* 元素集合，一条数据 */
    using Item = std::vector<Element>;
    /* 匹配方法 */
    using MatchMethod = FindMethod_S;
    using MatchMethods = std::vector<MatchMethod>;

    /* 字符串转固定数字 */
    inline constexpr unsigned int str2tag_core(const char *s, size_t l, unsigned int h)
    {
        return (l == 0) ? h
                    : str2tag_core(s + 1, l - 1,
                                    (h * 33) ^ static_cast<unsigned char>(*s));
    }

    inline unsigned int str2tag(const std::string &s)
    {
        return str2tag_core(s.data(), s.size(), 0);
    }

    inline constexpr unsigned int str2tag(const char *pData)
    {
        std::string_view sv(pData);
        return str2tag_core(pData, sv.size() + 1, 0);
    }

    class CDbBase
    {
    public:
        CDbBase() = delete;
        CDbBase(std::string path, std::string tableName);
        ~CDbBase();

        /*
         * @description: 添加表格字段
         * @param[in]: key 字段名
         * @param[in]: type 字段类型
         * @return:
         */
        void add_tableKey(const TableKey &key);

        /*
         * @description: 初始化数据库
         * @return: <0 失败
         */
        int init(bool bAddDefault = true);

        /*
         * @description: 反初始化数据库
         * @return: <0 失败
         */
        void deinit();

        /*
         * @description: 添加数据
         * @param[out]: items 数据集
         * @param[in]: strTargetTableName 指定的表
         * @return:  <0 失败
         */
        int add(const Item &item, std::string strTargetTableName = std::string());

        /*
         * @description: 删除数据
         * @param[in]: items 数据集
         * @return: <0 失败
         */
        int del(const Item &item);

        /*
         * @description: 删除数据
         * @param[in]: methods 匹配方法
         * @param[in]: strTargetTableName 指定的表
         * @return: <0 失败
         */
        int del(const MatchMethods &methods, std::string strTargetTableName = std::string());
        /*
         * @description: 更新数据
         * @param[in]: items 数据集
         * @param[in]: methods 条件
         * @param[in]: strTargetTableName 指定的表
         * @return: <0 失败
         */
        int update(const Item &item, const MatchMethods &methods, std::string strTargetTableName = std::string());

        /*
         * @description: 查找数据
         * @param[in]: methods 查找方法
         * @param[out]: items 数据集
         * @param[in]: strTargetTableName 指定的表
         * @return: <0 失败
         */
        int find(const MatchMethods &methods, std::vector<Item> &items, std::string strTargetTableName = std::string());
        /* 自定义命令查找 */
        int find(const std::string cmd, std::vector<Item> &items);
        int get_count(const MatchMethods &methods, int &nCount, const std::string field, std::string strTargetTableName = std::string());
        int get_avg(const std::string field, const MatchMethods &methods, int &avg);
        int get_sum(const std::string field, const MatchMethods &methods, int &avg);

        /*
         * @description: 打印数据
         * @param[in]: items 需要打印的数据
         * @return:
         */
        void print(const std::vector<Item> &items);
        std::string get_tableName() const;
        /*
         * @description: int类型
         * @return: 返回数据库int类型格式
         */
        static std::string type_int();

        /*
         * @description: char 数组类型
         * @param[in]: nSize 数组大小
         * @return: 返回数据库char 数组类型格式
         */
        static std::string type_string(size_t nSize);

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
        std::vector<std::string> get_column_data(const std::string &tableName, const std::string &targetField, int nMaxRows=1);
        
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

    private:
        /* string格式value转为FieldValue格式， key用于匹配 */
        FieldValue to_value(const std::string key, const std::string value);
        /* 组合匹配方法 */
        std::string group_methods(const MatchMethods &methods);
        /* 条件枚举转string */
        std::string to_string(const Criterion_E enCriterion);
        /* 字段值转string */
        std::string to_string(const FieldValue value);
        /* 字符串转类型 */
        FieldVelueType_E to_type(const std::string valueType);
        /* 分离数据为 kye: xx,xx valeue: 'xx','xx'格式 */
        void cut(const Item &item, std::string &key, std::string &value);
        /* 组合数据为 xx = '%d' xx = '%d'格式 */
        void combiner(const Item &item, std::string &group);

    private:
        /* 数据表键名 */
        std::vector<TableKey> m_tableKey;
        /* 数据库路径 */
        std::string m_path;
        /* 数据表名称 */
        std::string m_tableName;
        /* 数据库句柄 */
        sqlite3 *m_pHandle;
        SQLite3 m_sqlite3;

    };
} /* end namespace */

#endif
