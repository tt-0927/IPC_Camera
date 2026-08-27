/**
 * @FilePath     : event_abnormal_detector.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-06-04
 * @Description  : 异常报警检测器，负责管理系统异常状态的轮询检测与联动触发
 */

#pragma once

#include <map>
#include <atomic>
#include <thread>
#include <mutex>

#include "alarm_define.h"
#include "event_define.h"
#include "Singleton.h"
#include "system_monitor.h"

/**
 * @brief   : 异常报警检测器
 * @note    : 系统启动时即启动检测线程，持续轮询全部 5 种异常类型（硬盘满、硬盘错误、
 *            网络断开、IP 冲突、非法访问）。检测到异常时始终记录日志，
 *            并根据已配置的联动信息触发相应的联动动作。
 *            AC 2305 SET 接口仅用于更新联动配置，不影响检测线程的运行。
 */
class CAbnormalDetector : public CSingleton<CAbnormalDetector>
{
public:
    CAbnormalDetector();
    ~CAbnormalDetector();
    friend class CSingleton<CAbnormalDetector>;

    /**
     * @brief   : 初始化异常检测器，加载已持久化的联动配置并启动检测线程
     * @return   {int} 0：成功，非0：失败
     * @note    : 系统启动时由 ControlManage::init_business() 调用，
     *            检测线程一旦启动即持续运行直到进程退出
     */
    int init();

    /**
     * @brief   : 反初始化，停止检测线程
     * @return   {int} 0：成功，非0：失败
     * @note    : 系统关闭时由 ControlManage::deinit_business() 调用
     */
    int deinit();

    /**
     * @brief   : 更新指定异常类型的联动配置
     * @param    {Alarm::AbnormalType_E} enType：异常类型
     * @param    {Alarm::LinkageList_S} stLinkageList：新的联动配置
     * @note    : 由 SetAbnormalAlarmInfo 调用，仅更新配置不影响检测线程
     */
    void update_linkage_config(Alarm::AbnormalType_E enType,
                               const Alarm::LinkageList_S &stLinkageList);

private:
    /**
     * @brief   : 检测线程函数
     * @note    : 每 5 秒遍历全部 5 种异常类型，检测异常状态并触发事件处理
     */
    void detection_loop();

    /**
     * @brief   : 检查单个异常类型是否处于异常状态
     * @param    {Alarm::AbnormalType_E} enType：异常类型
     * @param    {CSystemMonitor::SystemStatus} &stStatus：当前系统状态快照
     * @return   {bool} true：处于异常状态 false：正常
     */
    bool check_abnormal_status(Alarm::AbnormalType_E enType,
                               const CSystemMonitor::SystemStatus &stStatus);

    /**
     * @brief   : 将异常类型映射为事件框架的事件类型
     * @param    {Alarm::AbnormalType_E} enType：异常类型
     * @return   {Event::Type_E} 对应的事件类型，无法映射时返回 INVALID
     */
    Event::Type_E map_to_event_type(Alarm::AbnormalType_E enType);

    /**
     * @brief   : 触发异常事件处理（记录日志 + 执行联动）
     * @param    {Alarm::AbnormalType_E} enType：异常类型
     * @param    {Event::Type_E} enEventType：对应的事件类型
     */
    void trigger_event(Alarm::AbnormalType_E enType, Event::Type_E enEventType);

private:
    /* 各异常类型的联动配置表，key 为异常类型，value 为该类型对应的联动列表 */
    std::map<Alarm::AbnormalType_E, Alarm::LinkageList_S> m_mapLinkageConfig;
    /* 保护联动配置表的互斥锁 */
    std::mutex m_configMutex;
    /* 检测线程运行标志 */
    std::atomic<bool> m_bRunning{false};
    /* 检测线程句柄 */
    std::thread m_thread;
    /* 检测间隔（秒） */
    static constexpr int DETECTION_INTERVAL_SEC = 5;
    /* 异常类型总数 */
    static constexpr int ABNORMAL_TYPE_COUNT = 5;
};
