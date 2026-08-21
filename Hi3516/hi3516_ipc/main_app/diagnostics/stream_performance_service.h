/**
 * @FilePath     : stream_performance_service.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-28 17:09:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-28 17:23:19
 * @Description  : stream 进程线程性能聚合与 /proc 全线程快照服务
 */
#pragma once

#include <atomic>
#include <thread>

#include "Singleton.h"

/**
 * @brief   : 汇总线程热路径统计并采集进程内全部 Linux 线程状态
 * @note    : 默认关闭；仅在诊断配置启用后创建低频报告线程。
 */
class CStreamPerformanceService : public CSingleton<CStreamPerformanceService>
{
private:
    CStreamPerformanceService() = default;

public:
    ~CStreamPerformanceService();
    friend class CSingleton<CStreamPerformanceService>;

    /**
     * @brief   : 初始化性能观测服务
     * @param    {无}
     * @return   {int} 0：成功，非0：失败
     * @note    : 配置文件不存在或 enable=0 时保持关闭并返回成功。
     */
    int init();

    /**
     * @brief   : 停止性能观测服务
     * @param    {无}
     * @return   {int} 0：成功，非0：失败
     */
    int deinit();

private:
    /**
     * @brief   : 读取诊断配置文件
     * @param    {无}
     * @return   {void}
     */
    void load_config();

    /**
     * @brief   : 低频聚合与 /proc 采样线程入口
     * @param    {无}
     * @return   {void}
     */
    void report_loop();

    /**
     * @brief   : 输出业务线程耗时聚合统计
     * @param    {无}
     * @return   {void}
     */
    void report_probe_statistics();

    /**
     * @brief   : 输出 /proc/self/task 中所有线程的资源快照
     * @param    {无}
     * @return   {void}
     */
    void report_process_threads();

private:
    std::atomic<bool> m_bRunning{false};
    std::thread m_stReportThread;
    bool m_bEnabled{false};
    bool m_bProcThreadSnapshotEnabled{true};
    /* info: 开启后每个报告窗口逐条输出所有 TID，默认仅输出新建或活跃线程以控制日志量。 */
    bool m_bProcThreadDetailEnabled{false};
    int m_nReportIntervalSec{10};
};
