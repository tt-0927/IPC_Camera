/**
 * @FilePath     : record_file_manage.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-01 09:01:32
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-05 15:52:18
 * @Description  : 录制文件管理
 */

#pragma once

#include "record_file_database.h"
#include <vector>
#include "Singleton.h"
#include "event_define.h"
#include <map>
#include "m3u8.h"
#include "replay_define.h"
#include <filesystem>
#include <atomic>

namespace fs = std::filesystem;

class RecordFileManage : public CSingleton<RecordFileManage>
{
    RecordFileManage() = default;

public:
    typedef struct EventFile
    {
        M3U8 *m3u8 = nullptr;
        Event::Info_S stEventInfo;
    } EventFile_S;

    typedef struct EventType
    {
        int nChnId = 0;
        int nType = 0;
        int nUniqueId = 0;
        bool operator<(const EventType &other) const
        {
            /* 不同通道 | 不同事件 |  不同类型互斥 */
            if (nChnId != other.nChnId)
            {
                return nChnId < other.nChnId;
            }
            /* 相同事件互斥 */
            if (nUniqueId != other.nUniqueId)
            {
                return nUniqueId < other.nUniqueId;
            }
            /* 相同时间戳的事件按类型排序 */
            return nType < other.nType;
        }
    } EventType_S;

    ~RecordFileManage() = default;
    friend class CSingleton<RecordFileManage>;

    /**
     * @brief 初始化
     * @return * int  <0:失败
     */
    int init();
    /**
     * @brief 反初始化
     */
    void deinit();
    /**
     * @brief 添加m3u8文件信息
     * @param stnfo m3u8文件信息
     * @return int <0:失败
     */
    int add(Record_NS::FileInfo_S stnfo);
    /**
     * @brief 添加ts文件信息
     * @param stnfo ts文件信息
     * @return int  <0:失败
     */
    int add(Record_NS::TsFileInfo_S stnfo);
    /**
     * @brief 删除m3u8文件信息
     * @param stnfo m3u8文件信息
     * @return int  <0:失败
     */
    int del(Record_NS::FileInfo_S stnfo);
    int del(Event::RetrievalCond_S &stCond, std::string strTargetTableName = std::string());
    /**
     * @brief 更新m3u8文件信息
     * @param stUserUpdateInfo m3u8文件信息
     * @return int  <0:失败
     */
    int update(Record_NS::UpdateInfo_S stUserUpdateInfo);

    /**
     * @brief 更新ts录像文件信息
     * @param stUserUpdateInfo m3u8文件信息
     * @param stUserUpdateInfo 目标表格名字
     * @return int  <0:失败
     */
    int update(Record_NS::TsFileInfo_S stTsFileInfo, std::string strTargetTableName = std::string());
    /**
     * @brief 查找m3u8文件信息
     * @param stFind 查找条件
     * @param infos 输出信息
     * @return int <0:失败
     */
    int find(Record_NS::Find_S stFind, std::vector<Record_NS::FileInfo_S> &infos);
    /**
     * @brief 查找m3u8文件信息
     * @param stFind 查找条件
     * @param infos 输出信息
     * @return int <0:失败
     */
    int find(Record_NS::Find_S stFind, std::vector<Record_NS::FindResult_S> &infos);
    /**
     * @brief 查找ts文件信息
     * @param stFind 查找条件
     * @param infos 输出信息
     * @return int <0:失败
     */
    int find(Record_NS::TsFind_S stTsFind, Record_NS::TsFileInfo_S &stTsFileInfo);
    
    /**
     * @brief 获取表中有多少条ts文件信息
     * @param stFind 查找条件
     * @param strTargetTableName 目标表格名字
     * @return int <0:失败
     */
    int getTableDataCount(Event::RetrievalCond_S &stCond, std::string strTargetTableName = std::string());
    
    /**
     * @brief 获取表格的分页信息
     * @param stFind 查找条件
     * @param stPageInfo 输出表格分页信息
     * @param strTargetTableName 目标表格名字
     * @return int <0:失败
     */
    int getTablePageInfo(Event::RetrievalCond_S &stCond, Common::PageInfo_S &stPageInfo, std::string strTargetTableName = std::string());

    /**
     * @brief 获取表中的ts文件信息
     * @param stFind 查找条件
     * @param stPageInfo 输出ts文件信息
     * @param stPageInfo 表格分页信息
     * @param strTargetTableName 目标表格名字
     * @return int <0:失败
     */
    int searchByRecordTs(Event::RetrievalCond_S &stCond, std::vector<::Record_NS::TsFileInfo_S> &TsFileInfos, Common::PageInfo_S &stPageInfo, std::string strTargetTableName = std::string());

    /**
     * @brief 视频检索，锁定等相关
     * @param stEventCond 
     * @param infos 
     * @return int 
     */
    int retrieval(Record_NS::RetrievalCond_S stEventCond, std::vector<Record_NS::FileInfo_S> &infos);
    int retrieval(Record_NS::RetrievalCond_S stEventCond, std::vector<Record_NS::FileInfo_S> &infos, Common::PageInfo_S &stPageInfo);

    // /**
    //  * @brief 查找ts文件信息
    //  * @param stFind 查找条件
    //  * @param infos 输出信息
    //  * @return int <0:失败
    //  */
    // int find(Record_NS::Find_S stFind, std::vector<Record_NS::FindResult_S> &infos);
    /**
     * @brief 根据通道id获取m3u8文件信息
     * @param stnfo m3u8文件信息
     * @return int <0:失败
     */
    int get_itemInfo(Record_NS::FileInfo_S &stnfo);


    // int create_eventVideo(Event::Info_S &stEventInfo);

    /**
     * @brief 添加事件触发m3u8信息
     * @param strM3u8Path m3u8路径
     * @param strM3u8FileName m3u8文件名
     * @param stEventInfo 事件信息
     * @return int <0:失败
     */
    int add_eventVideo(std::string &strM3u8Path, std::string &strM3u8FileName, Event::Info_S &stEventInfo);
    /* 临时视频文件 */
    int create_temporaryVideo(const std::string filename, int nChnId);
    /**
     * @brief 根据通道id获取配额大小 单位GB
     * @param nChnId 通道id
     * @return double  
     */
    double  get_channel_size(std::string strPath);

    /**
     * @brief 处理缓存文件
     */
    void deal_cacheFile();

    /**
     * @brief 查找录制目录下对应的日期是否有对应的m3u8文件
     * @param strPath 目录路径
     * @param strPrefix 查找字段
     * @return std::vector<std::string> 存在对应m3u8的目录 
     */
    std::vector<std::string> findM3u8Dates(const std::string &strPath, const std::string &strPrefix);

    /**
     * @brief 循环录制
     */
    int loop_write();
    
    /**
     * @brief 删除符合条件的录制文件目录
     * @param nOldTime 修改系统时间前的时间戳
     * @return int <0:失败
     */
    int rm_recordDir(const fs::path strRecordBasePath, time_t nTime);

    /**
     * @brief 删除符合条件的录制ts文件
     * @param nOldTime 修改系统时间前的时间戳
     * @return int <0:失败
     */
    int rm_recordTsFile(const fs::path strRecordBasePath, time_t nTime);

    /**
     * @brief 删除符合条件的录制ts文件数据库信息
     * @param nOldTime 修改系统时间前的时间戳
     * @return int <0:失败
     */
    int del_recordfilemanageDbInfo(time_t nTime);

    /**
     * @brief 删除符合条件的事件数据库信息
     * @param nOldTime 修改系统时间前的时间戳
     * @return int <0:失败
     */
    int del_eventmanageDbInfo(time_t nTime);

    /**
     * @brief 同步ts文件与m3u8的修改
     * @param nOldTime 修改系统时间前的时间戳
     * @return int <0:失败
     */
    int truncateM3U8(time_t nTime);
    
    /* 录制文件管理线程 */
    void record_file_manage_thread();

    /**
     * @brief 设置循环录制标志位
     * @param bLoopWrite 是否启动循环录制标志位，true：启动，false：不启动
     * @return int <0:失败
     */
    void setLoopWrite(bool bLoopWrite);

    /**
     * @brief 格式化sd卡同步数据库数据
     * @return 
     */
    int formatSDCardSyncRecordDb();

    /**
     * @brief   : 时间发生跳变时，处理要进行删除的录制文件以及相关数据库的信息，并重启录制
     * @param    {time_t} nTime：当前时间
     */
    void dealTimeChange(time_t nTime);
private:
    /* ts文件处理 */
    int deal_tsFile(Record_NS::TsFileInfo_S stTsFileInfo);
    /* 视频合并，生成m3u8文件 */
    void merge_video(std::vector<Record_NS::TsFileInfo_S> &tsFileInfos);
    int deal_eventFile(Record_NS::TsFileInfo_S stTsFileInfo, int &nEventType);

private:
    /* 是否停止录制文件管理线程 */
    std::atomic_bool m_bRun = false;
    /* 根据通道id存储连续的ts文件 */
    std::map<int, std::vector<Record_NS::TsFileInfo_S>> m_tsFileInfosMap;
    /* 事件 */
    std::map<EventType_S, EventFile_S> m_eventFileMap;
    std::mutex m_eventMutex;
    /* 循环录制标志位 */
    std::atomic_bool m_bLoopWriteFlag = false;
    // int parse(std::string m3u8, std::vector<VideoTime_S> videoTimes);
};