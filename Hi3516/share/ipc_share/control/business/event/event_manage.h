/**
 * @FilePath     : event_manage.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-12-14
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-04 11:40:43
 * @Description  : 事件布防调度管理，负责事件布防时间计算与算法开关状态通知
 */

#pragma once

#include <set>
#include <iomanip>
#include <atomic>
#include <thread>

#include "alarm_define.h"
#include "event_configure.h"
#include "event_resource.h"
#include "Singleton.h"
#include "action_code.h"

extern "C"
{
#include <unistd.h>
}

/* AI 算法控制处理的回调函数类型 */
using AlgoControlDealCallback = std::function<void(int nCode, const char *strData, void *pData)>;

class CEventManage : public CSingleton<CEventManage>
{
public:
    CEventManage();
    ~CEventManage();
    friend class CSingleton<CEventManage>;

    static CEventManage *instance()
    {
        static CEventManage inst;
        return &inst;
    }

    /**
     * @brief   : 设置 AI 算法控制处理的回调
     * @param    {AlgoControlDealCallback} &callback：AI 算法控制处理的回调函数
     */
    void set_algoControlDeal_callback(const AlgoControlDealCallback &callback);

    /**
     * @brief   : 更新算法配置
     * @return   {int} 0：成功 非0：失败
     */
    int send_algo_controlData(const int nCode = AC_SET_ALGORITHM_CONFIG, const char *pJsonData = nullptr, void *pData = nullptr);

    /**
     * @brief   : 更新事件布防时间
     */
    void update_event_schedule();

#ifdef SCENE_INTELLIGENT_ANALYSIS
    /**
     * @brief   : 更新AI场景智能分析布防时间
     */
    void update_ai_analysis_schedule();

    /**
     * @brief   : AI场景智能分析-调度
     * @return   {int} 0：成功 非0：失败
     */
    int update_ai_analysis_switch_status(Event::AlgorithmConfig_S AlgoInfo);
#endif

private:
    /**
     * @brief   : 更新事件开关状态
     * @return   {int} 0：成功 非0：失败
     */
    int update_event_switch_status();

    /**
     * @brief   : 设置事件类型开关状态
     * @param    {AlgorithmConfig_S} &change：算法配置
     * @param    {Type_E} enEventType：事件类型
     * @param    {bool} bEnable：开关状态
     */
    void set_event_type(Event::AlgorithmConfig_S &change, Event::Type_E enEventType, bool bEnable);

    /**
     * @brief   : 计算当前的算法状态
     * @note    : 考虑事件使能状态和当前布防时间，重新计算所有算法的开关状态
     */
    void calculate_current_algorithm_state();

#ifdef SCENE_INTELLIGENT_ANALYSIS
    /**
     * @brief   : 检查画面分析的定时分析状态
     * @param    {LLMImageAnalysis_S} &imageAnalysisConfig：画面分析配置
     * @return   {bool} true：应该开启 false：应该关闭
     */
    bool check_image_analysis_schedule(const Alarm::LLMImageAnalysis_S &imageAnalysisConfig);

    /**
     * @brief   : 检查重复分析模式是否应该执行
     * @param    {RepeatedAnalysisConfig_S} &repeatedConfig：重复分析配置
     * @return   {bool} true：应该执行 false：不应该执行
     */
    bool check_repeated_analysis_schedule(const Alarm::RepeatedAnalysisConfig_S &repeatedConfig);

    /**
     * @brief   : 检查间隔分析模式是否应该执行
     * @param    {IntervalAnalysisConfig_S} &intervalConfig：间隔分析配置
     * @return   {bool} true：应该执行 false：不应该执行
     */
    bool check_interval_analysis_schedule(const Alarm::IntervalAnalysisConfig_S &intervalConfig);
#endif

    /**
     * @brief   : 打印算法配置变化
     * @param    {AlgorithmConfig_S} &oldConfig：旧配置
     * @param    {AlgorithmConfig_S} &newConfig：新配置
     */
    void print_algorithm_config_changes(const Event::AlgorithmConfig_S &oldConfig, const Event::AlgorithmConfig_S &newConfig);

    /**
     * @brief   : 线程函数：周期性检测布防时间并更新算法开关状态
     */
    void run();

private:
    /* AI 算法控制处理的回调 */
    AlgoControlDealCallback m_algoControlDealCallback;
    /* 线程函数运行状态 */
    std::atomic<bool> m_bRunning = false;
    /* 线程函数句柄 */
    std::thread m_thread;
    /* 保护共享资源的互斥锁 */
    std::mutex m_mutex;
    /* 存储不同事件的布防计划 */
    std::set<Alarm::EventSchedule_S> m_stEventSchedule;
    /* 当前算法开关状态快照 */
    Event::AlgorithmConfig_S m_changes;
    /* 上次间隔分析执行时间 */
    time_t m_lastIntervalAnalysisTime;
};
