/**
 * @FilePath     : snap_sqlite.hpp
 * @Author       : zhouzirui
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-06-06 16:54:50
 * @Description  : 人脸抓拍数据库
 */

#pragma once

#include <mutex>

#include "database.hpp"
#include "face_manage_ext.hpp"

#include "dlog.h"
#include "sqlite3.h"


namespace FaceDataDB_NS
{
    class CSnapSqlite : public CDatabase
    {
    public:

        CSnapSqlite();
        ~CSnapSqlite();

        /**
         * @brief 插入数据
         * @param [SnapFaceInfo_S&] stInfo: 数据
         * @return [*] 成功 >= IpcRet_E::OK   其他失败
         * @note
         */
        IpcRet_E insertData(SnapFaceInfo_S& stInfo);

        /**
         * @brief 更新数据
         * @param [int] nId: 需要更新的ID
         * @param [SnapFaceInfo_S&] stInfo: 数据
         * @return [*] 成功 >= IpcRet_E::OK   其他失败
         * @note
         */
        IpcRet_E updateData(int nId, SnapFaceInfo_S& stInfo);

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
         * @param [SnapFaceInfo_S&] stOutInfo: 数据
         * @return [*] 成功 >= IpcRet_E::OK   其他失败
         * @note
         */
        IpcRet_E searchData(int nId, SnapFaceInfo_S& stOutInfo);

        /**
         * @brief 获取全部数据
         * @param [list<SnapFaceInfo_S>] listOutInfo: 获取到的数据
         * @return [*] 成功 >= IpcRet_E::OK   其他失败
         * @note
         */
        IpcRet_E getAllData(std::list<SnapFaceInfo_S>& listOutInfo);

        /**
         * @brief: 获取数据总量
         * @param [int&] nOutTotal: 获取到的总量
         * @return [*] IpcRet_E::OK 成功  其他失败
         * @note:
         */
        IpcRet_E getDataTotal(int& nOutTotal);

    private:

        /**
         * @description: 初始化数据库
         * @return [*] IpcRet_E::OK 成功  其他失败
         * @others:
         */
        IpcRet_E init_sql();

    private:

        /* 互斥锁 */
        std::mutex m_mutex;
    };

}    // namespace FaceDataDB_NS