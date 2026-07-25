
/**
 * @file FaceSqlite.hpp
 * @author xiejh (xiejh@kfb.cn)
 * @date 2024-11-26
 *
 * @brief 本地人脸名单库数据库
 */

#pragma once

#include <mutex>

#include "Database.hpp"
#include "dlog.h"
#include "FaceRecExtern.hpp"
#include "sqlite3.h"

namespace Ai0630_NS
{
    class FaceSqlite : public Database
    {
    public:

        FaceSqlite();
        ~FaceSqlite();

        /**
         * @brief 插入数据
         * @param [FaceLibsInfo_S&] stInfo: 数据
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E insertData(FaceLibsInfo_S& stInfo);

        /**
         * @brief 查找数据
         * @param nId 查找的ID
         * @param strTabName 表名
         * @param stOutInfo 结果
         * @return BlError_E
         */
        BlError_E searchDataById(int nId, FaceLibsInfo_S& stOutInfo);
        BlError_E searchDataById(int nId, std::string strTabName, FaceLibsInfo_S& stOutInfo);

        /**
         * @brief 获取全部数据
         * @param [list<FaceLibsInfo_S>] listOutInfo: 获取到的数据
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E getAllData(std::list<FaceLibsInfo_S>& listOutInfo,
                             std::string                strTabName = std::string());


        /* 根据表名查找数据 */
        BlError_E searchDataByTable(std::string strTabName, std::list<FaceLibsInfo_S>& listOutInfo);

    private:

        /**
         * @description: 初始化数据库
         * @return [*] BlError_E::OK 成功  其他失败
         * @others:
         */
        BlError_E init_sql();
    };
}    // namespace Ai0630_NS
