/*
 * @FilePath     : FaceDataDB.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-06 14:11:17
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2024-01-20 14:29:08
 * @Description  : 人脸特征数据库
 */
#pragma once

#include <iostream>
#include <mutex>

#include "BlError.h"
#include "dlog.h"
#include "FaceRecExtern.hpp"
#include "sqlite3.h"

namespace FR_NS
{
    class CFaceDataDB
    {
    public:

        CFaceDataDB();
        ~CFaceDataDB();

        /**
         * @brief 插入数据
         * @param [FaceDataInfo_S&] stInfo: 数据
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E insertData(FaceDataInfo_S& stInfo);

        /**
         * @brief 更新数据
         * @param [int] nId: 需要更新的ID
         * @param [FaceDataInfo_S&] stInfo: 数据
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E updateData(int nId, FaceDataInfo_S& stInfo);

        /**
         * @brief 删除CardId不为-1的数据
         * @param [*]
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E deleteData();

        /**
         * @brief 删除数据
         * @param [int] nId: 需要删除的ID
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E deleteData(int nId);

        /**
         * @brief 查找数据
         * @param [int] nId: 需要删除的ID
         * @param [FaceDataInfo_S&] stOutInfo: 数据
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E searchData(int nId, FaceDataInfo_S& stOutInfo);

        /**
         * @brief 查找数据
         * @param [int] nCurPageNum: 第几页
         * @param [int] nPageSize: 一页多少数据
         * @param [list<FaceDataInfo_S>] listOutInfo: 获取到的数据
         * @return [*]
         * @note
         */
        BlError_E searchData(int nCurPageNum, int nPageSize, std::list<FaceDataInfo_S>& listOutInfo);

        /**
         * @brief 查找数据
         * @param [string] strSearchKey: 搜索关键字
         * @param [int] nCurPageNum: 第几页
         * @param [int] nPageSize: 一页多少数据
         * @param [list<FaceDataInfo_S>] listOutInfo: 获取到的数据
         * @return [*]
         * @note
         */
        BlError_E searchData(std::string strSearchKey, int nCurPageNum, int nPageSize, std::list<FaceDataInfo_S>& listOutInfo);

        /**
         * @brief 获取全部数据
         * @param [list<FaceDataInfo_S>] listOutInfo: 获取到的数据
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E getAllData(std::list<FaceDataInfo_S>& listOutInfo);

        /**
         * @brief: 获取数据总量
         * @param [int&] nOutTotal: 获取到的总量
         * @return [*] BlError_E::OK 成功  其他失败
         * @note:
         */
        BlError_E getDataTotal(int& nOutTotal);


    private:

        /**
         * @brief 添加转义字符
         * @param [string&] strInput: 待添加支付串
         * @return [*] 添加后的字符串
         * @note
         */
        std::string add_ESC(const std::string& strInput);

        /**
         * @brief: 错误打印函数
         * @param [char*] achCmd: 打印数据
         * @param [char**] pchErrMsg: 错误数据
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note:
         */
        BlError_E db_print_errMsg(const char* achCmd, char* pchErrMsg);

        /**
         * @brief: 执行事务
         * @param [char*] pchSelectSQL: 命令
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note:
         */
        BlError_E exec_transaction(const std::string& strSelectSQL);


        /**
         * @brief: 数据库备份
         * @param [sqlite3*] pSq: 需要备份的数据库句柄
         * @param [bool] bIsSave: true-备份数据 false-还原数据
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note:
         */
        BlError_E backup_database(sqlite3* pSq, bool bIsSave);

        /**
         * @brief: 检查数据表是否存在
         * @param [char*] pchTableName: 表名
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note:
         */
        BlError_E check_tableExist(const char* pchTableName);

        /**
         * @brief: 创建数据表
         * @param [char*] pchTableName: 表名
         * @return [*] BlError_E::OK 成功  其他失败
         * @note:
         */
        BlError_E create_dataTable(const char* pchTableName);

        /**
         * @brief  检查字段是否存在，不存在追加
         * @param  [char*] pchTableName 表格名称
         * @param  [char*] pchFieldName 字段名
         * @param  [char*] pchFieldType
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E check_fieldExist(
            const char* pchTableName,
            const char* pchFieldName,
            const char* pchFieldType);

        /**
         * @description: 初始化数据库
         * @return [*] BlError_E::OK 成功  其他失败
         * @others:
         */
        BlError_E init_sql();

        /**
         * @brief 开始事务
         * @return [*]
         * @note
         */
        bool beginTransaction();

        /**
         * @brief 执行事务
         * @return [*]
         * @note
         */
        bool commitTransaction();

        /**
         * @brief 回滚事务
         * @return [*]
         * @note
         */
        bool rollbackTransaction();

    private:

        /* 互斥锁 */
        std::mutex m_mutex;

        sqlite3*    m_pDb = nullptr;
        std::string m_strDatabaseName;
    };

}    // namespace FR_NS