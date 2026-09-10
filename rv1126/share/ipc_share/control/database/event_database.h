/*
 * @Author: xiejh xiejh@kfb.cn
 * @Date: 2024-10-14
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-01-04 02:45:36
 * @FilePath: /hisi/hisi/share/ipc_share/control/database/event_database.h
 * @Description: 事件数据库
 */

#pragma once

#include "DbBase.h"
#include "event_define.h"
#include "event_convert.h"
#include "Singleton.h"
#include "path_define.h"

namespace Db
{
    constexpr const char *EVENT_TABLE_NAME = "event_manage";
    constexpr const char *EVENT_DATABASE_PATH = RECORD_EVENT_DATABASE_PATH;

    /* 抓拍人脸对比数据表 */
    constexpr const char *FACE_COMPARE_TABLE_NAME = "face_compare_manage";

    constexpr const char *INFO_EVENT_ID = "event_id";
    constexpr const char *INFO_COMP_RESULT = "comp_result";
    constexpr const char *INFO_SIMILARITY = "similarity";
    constexpr const char *INFO_FACE_ID = "face_id";
    constexpr const char *INFO_LIB_NAME   = "lib_name  ";
    constexpr const char *INFO_FACE_NAME = "face_name";
    constexpr const char *INFO_LIB_FACE_PATH    = "lib_face_path";
    constexpr const char *INFO_CAPTURE_FACE_PATH = "capture_face_path";


    class EventDatabase : public CSingleton<EventDatabase>
    {
        EventDatabase();
    public:

        ~EventDatabase();
        friend class CSingleton<EventDatabase>;
        

        /**
         * @brief 新增常规事件
         * @param stEventInfo 
         * @return int 
         */
        int add(const Event::Info_S &stEventInfo);
        int add(const Event::FaceCompareInfo_S &stFaceCompareInfo);


        /**
         * @brief 查找信息数据
         * @param elem 
         * @param EventInfos 
         * @return int 
         */
        int find(const Element &elem, std::vector<Event::Info_S> &EventInfos);
        int find(const MatchMethods &methods, std::vector<Event::Info_S> &EventInfos);
        int get_count(const MatchMethods &methods, int &nCount, const std::string field);
        int get_faceCompareCount(const MatchMethods &methods, int &nCount, const std::string field);

        int find(const MatchMethods &methods, std::vector<Event::FaceCompareInfo_S> &faceCompareInfos);

        /**
         * @brief 更新数据
         * @param item 
         * @param methods 
         * @return int 
         */
        int update(const Item &item, const MatchMethods &methods);


        /**
         * @brief 删除数据
         * @param item 
         * @return int 
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

    private:
        CDbBase m_eventDb;
        CDbBase m_faceCompareDb;
        std::mutex m_mutex;
    };

} /* namespace Db */