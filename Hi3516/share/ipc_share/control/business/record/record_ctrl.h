/**
 * @FilePath     : record_ctrl.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-12-16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-31 11:19:32
 * @Description  : 录制控制
 */

#include <map>
#include <atomic>
#include "record_server.h"
#include "record_define.h"
#include "Singleton.h"
#include "alarm_define.h"
#include "event_define.h"
#include "IpcRet.h"

class CRecordCtrl : public CSingleton<CRecordCtrl>
{
    CRecordCtrl();

public:
    /*当天录制时间的信息*/
    typedef struct Info
    {
        /* 通道号 */
        int nChnId = 0;
        /* 星期几 */
        int nDayOfWeek = -1;
        /* 录制时间 */
        std::vector<Record_NS::RecordTime_S> recordTimes;
    } Info_S;
    ~CRecordCtrl();
    friend class CSingleton<CRecordCtrl>;

    /**
     * @brief 初始化录制模块
     * @return IpcRet_E <0:失败, >=0:成功
     */
    IpcRet_E init();

    /**
     * @brief 去初始化录制模块
     * @return IpcRet_E <0:失败, >=0:成功
     */
    IpcRet_E deinit();

    /**
     * @brief   : 更新录制计划
     */
    void update();

    /**
     * @brief   : 更新录制高级参数
     */
    void update_advancedParam();

    /**
     * @brief   : 重置录制状态
     */
    void reset_status();

    /**
     * @brief   : 设置事件触发录制信息
     */
    void set_event_record(bool bEventRecordFlag, Event::Info_S &stEventInfo);

    /**
     * @brief   : 获取触发录制的事件信息
     */
    void get_event_record(Event::Info_S &stEventInfo);

   /**
     * @brief   : 设置触发录制的事件的ts文件信息
     */
    void set_event_ts_info(int stEventInfo, std::string &strPath);

    /**
     * @brief   : 控制录制开启
     */
    void start_record();

    /**
     * @brief   : 控制录制关闭
     */
    void stop_record();

    /**
     * @brief   : 获取录制状态
     * @return   {Status_E} 录制状态
     */
    Record_NS::Status_E get_recordStatus();

    /**
     * @brief   : 设置人为录制信息
     * @param    {Info_S} &stInfo：录制信息
     * @return   {int} 成功：0  失败：小于零
     */
    int set_humanRecord(Record_NS::Info_S &stInfo);

    /**
     * @brief   : 获取人为录制信息
     * @param    {Info_S} &stInfo：录制信息
     */
    void get_humanRecord(Record_NS::Info_S &stInfo);

    /**
     * @brief   : 获取录制计划类型
     * @return    {int} 录制计划类型
     */
    int get_RecordScheduleType();

    /**
     * @brief   : 设置事件录制m3u8信息
     * @param    {std::string} &strM3u8Path m3u8路径
     * @param    {std::string} &strM3u8FileName m3u8文件名
     * @return
     */
    void set_eventM3u8Path(std::string &strM3u8Path, std::string &strM3u8FileName);

    /**
     * @brief   : 获取录制状态
     * @return 录制状态
     */
    int get_record_status();

    /**
     * @brief   : 设置stream进程与record进程连接状态
     * @param    {bool} bRecordProcessStatus true：连接成功 false：连接失败
     * @return   {int} 成功：0  失败：其他
     */
    int set_record_process_status(bool bRecordProcessStatus);

    /**
     * 更新记录的日期信息
     */
    void update_recordDate();

    /**
     * 检查是否是新的一天
     * @return 如果是新的一天返回true，否则返回false
     */
    bool is_newDay();

    /**
    * @brief  : 获取录制是否因存储满停止
    */
    bool isStoppedDueToStorage() const;

    /**
    * @brief  : 获取最后一次录制停止的时间戳
    */
    const std::string& getLastRecordStopTime() const;
    
private:
    /**
     * @brief   : 填充录制信息
     * @param    {Info_S} &stInfo：录制信息
     */
    void fill_info(Record_NS::Info_S &stInfo);

    /**
     * @brief   : 录制控制
     * @param    {int} nStatus：录制状态
     * @return   {int} 成功：0  失败：小于零
     */
    int ctrl(int nStatus);

private:
    // info /*----------------------- 私有线程函数 -----------------------*/
    /**
     * @brief   : 线程函数：检测录制计划
     */
    void run();

private:
    /*存储一周七天录制时间的容器*/
    std::vector<Info_S> m_infos;
    /*用于保护共享资源的互斥锁*/
    std::mutex m_mutex;
    /*记录录制状态*/
    Record_NS::Status_E m_enRecordStatus = Record_NS::NO_OPERATION;
    /*是否停止录制判断字段*/
    bool m_stop = false;
    /*人为录制信息*/
    Record_NS::Info_S m_stHumanRecordInfo;
    /*是否停止检测录制计划线程函数*/
    std::atomic_bool m_bRun = false;
    /* 默认录制通道 */
    int m_ChnId = 0;
    /* 当前时间录制计划类型：0当前无录制计划，1定时录制，2事件录制 */
    int m_RecordScheduleType = 0;
    /* 高级录制参数 */
    Record_NS::AdvancedParam_S m_advancedParam;
    /* 是否开启删除文件 */
    bool m_bDelIng = false;

    /* 事件触发录制标志位 */
    bool m_eventRecordFlag = false;
    /* 事件计划时间内触发录制的事件信息 */
    Event::Info_S m_eventInfo;
    /* 事件录制计划时间段触发录制的m3u8文件路径 */
    std::string m_strEventM3u8Path;
    /* 事件录制计划时间段触发录制的m3u8文件名 */
    std::string m_strEventM3u8FileName;
    /* stream进程与record进程连接状态 true：连接成功 false：连接失败 */
    bool m_bRecordProcessStatus = false;
    /*用于判断日期变更*/
    std::string m_strCurRecordDate;

    /*录制因存储满停止的时间戳*/
    std::string m_strLastRecordStopTime; 
    /* 是否录像停止*/
    bool m_bStoppedDueToStorage = false;
};
