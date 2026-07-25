/**
 * @FilePath     : face_sqlite.hpp
 * @Author       : zhouzirui
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-06-06 16:52:54
 * @Description  : 本地人脸名单库数据库c
 */

#pragma once

#include <mutex>

#include "database.hpp"
#include "event_define.h"
#include "face_manage_ext.hpp"

#include "dlog.h"
#include "sqlite3.h"


namespace FaceDataDB_NS
{
    class CFaceSqlite : public CDatabase
    {
    public:
        
        CFaceSqlite();
        ~CFaceSqlite();

        /**
         * @brief 插入数据
         * @param [FaceLibsInfo_S&] stInfo: 数据
         * @return [*] 成功 >= IpcRet_E::OK   其他失败
         * @note
         */
        IpcRet_E insertData(FaceLibsInfo_S& stInfo);
        
        /**
         * @brief 更新数据
         * @param [int] nId: 需要更新的ID
         * @param [FaceLibsInfo_S&] stInfo: 数据
         * @return [*] 成功 >= IpcRet_E::OK   其他失败
         * @note
         */
        IpcRet_E updateData(int nId, FaceLibsInfo_S& stInfo);
        
        /**
         * @brief 删除数据
         * @param [int] nId: 需要删除的ID
         * @return [*] 成功 >= IpcRet_E::OK   其他失败
         * @note
         */
        IpcRet_E deleteData(int nId);
        
        /**
         * @brief 查找数据
         * @param [int] nId: 需要删除的ID
         * @param [FaceLibsInfo_S&] stOutInfo: 数据
         * @return [*] 成功 >= IpcRet_E::OK   其他失败
         * @note
         */
        IpcRet_E searchDataById(int nId, FaceLibsInfo_S& stOutInfo);

        /**
         * @brief 获取全部数据
         * @param [list<FaceLibsInfo_S>] listOutInfo: 获取到的数据
         * @return [*] 成功 >= IpcRet_E::OK   其他失败
         * @note
         */
        IpcRet_E getAllData(std::list<FaceLibsInfo_S>& listOutInfo);
        
        /**
         * @brief: 获取数据总量
         * @param [int&] nOutTotal: 获取到的总量
         * @return [*] IpcRet_E::OK 成功  其他失败
         * @note:
         */
        IpcRet_E getDataTotal(int& nOutTotal);

        /**
         * @brief 创建新名单组数据库表
         * @param strFaceLibName 
         * @return IpcRet_E 
         */
        IpcRet_E check_creat_table(std::string strFaceLibName);

        /* 修改名单组数据库表名 */
        IpcRet_E renameTable(std::string oldTabName, std::string newTabName);

        /* 删除名单组数据库表 */
        IpcRet_E deleteTable(std::string strTabName);

        /* 根据表名查找数据 */
        IpcRet_E searchDataByTable(std::string strTabName, std::list<FaceLibsInfo_S>& listOutInfo);

        /* 获取每张表的数据统计信息 */
        IpcRet_E get_table_report(std::vector<Event::FaceLibInfo_S>& listTableReport);

        /* 根据组合条件查询数据 */
        IpcRet_E search_combined_data(Event::FaceFind_S stFaceFind, std::list<FaceLibsInfo_S>& listOutInfo);

    private:
        
        /**
         * @description: 初始化数据库
         * @return [*] IpcRet_E::OK 成功  其他失败
         * @others:
         */
        IpcRet_E init_sql();

        /**
         * @brief 获取数据库内最大的 ID
         * @return int 
         */
        int get_tables_maxId();

    private:
        
        /* 互斥锁 */
        std::mutex m_mutex;

        std::string m_strFaceTableName ;
    };
}
