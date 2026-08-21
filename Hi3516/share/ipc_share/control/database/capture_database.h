/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-10-16 17:22:50
 * @LastEditors: zhouzr@kfb.cn
 * @LastEditTime: 2026-08-13 15:03:25
 * @FilePath: /hisi/share/ipc_share/control/database/capture_database.h
 * @Description: 抓图信息数据库
 */
#pragma once


#include "DbBase.h"
#include "capture_define.h"
#include "path_define.h"
#include "Singleton.h"

using namespace Db;

namespace Db
{

	constexpr const char *CAPTURE_TABLE_NAME = "capture_manage";
    constexpr const char *CAPTURE_DIR_INFO_TABLE_NAME = "capture_dir_info_manage";
    constexpr const char *CAPTURE_FILE_DATABASE_PATH = CAPTURE_DATABASE_PATH;

    constexpr const char *INFO_CHNL_ID = "channel_id";
    constexpr const char *INFO_CAPTURE_EVENT_TYPE = "type";
    constexpr const char *INFO_CAPTURE_STATRTIME = "start_time";
    constexpr const char *INFO_CAPTURE_ENDTIME = "end_time";
    // constexpr const char *INFO_TIMESTAMP = "timestamp";
    constexpr const char *INFO_CAPTURE_PATH = "image_path";
    constexpr const char *INFO_CAPTURE_SIZE = "image_size";
    constexpr const char *INFO_CAPTURE_COUNT = "image_count";
    constexpr const char *INFO_CAPTURE_TOTAL_SIZE = "image_total_size";

	class CCaptureDatabase : public CSingleton<CCaptureDatabase>
    {
        CCaptureDatabase();
    public:

        ~CCaptureDatabase();
        friend class CSingleton<CCaptureDatabase>;
        /*
         * @description: 添加数据
         * @param[out]: stInfo 数据
         * @return:  <0 失败
         */
        int add(const Capture_NS::CaptureInfo_S &stInfo);
        int add(const Capture_NS::CaptureDirInfo_S &stInfo);


        /*
         * @description: 查找信息数据
         * @param[int]: elem 需要查找的内容
         * @param[out]: infos 输出信息数据
         * @return:  <0 失败
         */
        int find(const Element &elem, std::vector<Capture_NS::CaptureInfo_S> &infos);
        int find(const MatchMethods &methods, std::vector<Capture_NS::CaptureInfo_S> &infos);
        int find(const Element &elem, std::vector<Capture_NS::CaptureDirInfo_S> &infos);
        int find(const MatchMethods &methods, std::vector<Capture_NS::CaptureDirInfo_S> &infos);


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
        int del(const MatchMethods &methods, std::string strTargetTableName);
        /**
        * @brief 更新抓图目录相关信息
        * @param stInfo 抓图目录相关信息
        * @return int  <0:失败
        */
        int update(Capture_NS::CaptureDirInfo_S &stInfo);
        
        int get_count(const MatchMethods &methods, int &nCount, const std::string field);
        
        /**
        * @brief 根据通道id获取抓图目录相关信息
        * @param stnfo 抓图目录相关信息
        * @return int <0:失败
        */
        int get_itemInfo(Capture_NS::CaptureDirInfo_S& stInfo);
        
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

        /**
        * @brief 清空指定表格数据
        * @param stnfo 抓图目录相关信息
        * @return int <0:失败
        */
        int clear_table(const std::string &tableName, bool resetAutoInc = true);
    private:

        /**
        * @brief 创建数据表
        * @param tableName 表名
        * @param bAddTableKey 是否添加字段
        * @return int <0:失败
        */
        int create(std::string tableName, bool bAddTableKey = true);

        /**
        * @brief 设置SQLite同步模式为NORMAL
        * @note 图片文件本身已fsync，元数据索引允许容忍断电丢失最近一次提交
        */
        void set_sync_mode();

    private:
        CDbBase m_database;
        CDbBase m_captureDirDatabase;
        std::mutex m_mutex;
    };

} /* namespace Db */