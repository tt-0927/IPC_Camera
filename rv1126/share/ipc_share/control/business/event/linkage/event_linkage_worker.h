/**
 * @FilePath     : event_linkage_worker.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-15 16:29:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-30 17:46:10
 * @Description  : 事件联动异步任务工作线程
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <future>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "event_linkage_action_async.h"

class EventLinkageWorker
{
public:
    explicit EventLinkageWorker(EventLinkageAsyncAction &stAsyncAction);
    ~EventLinkageWorker();

    /**
     * @brief   : 初始化 worker
     * @return   {int} 0：成功 非0：失败
     */
    int init();

    /**
     * @brief   : 反初始化 worker
     * @return   {int} 0：成功 非0：失败
     */
    int deinit();

    /**
     * @brief   : 添加异步联动任务
     * @param    {LinkageTask_S} &stTask 联动任务
     */
    void pushTask(const LinkageTask_S &stTask);

    /**
     * @brief   : 停止指定联动类型任务
     * @param    {LinkageType_E} enLinkageType 联动类型
     * @return   {bool} true：原本正在运行 false：原本未运行
     */
    bool stopTask(LinkageType_E enLinkageType);

    /**
     * @brief   : 获取联动运行标志
     * @param    {LinkageType_E} enLinkageType 联动类型
     * @return   {std::atomic<bool>} &运行标志引用
     */
    std::atomic<bool> &getRunningFlag(LinkageType_E enLinkageType);

private:
    /**
     * @brief   : 工作线程主循环
     */
    void task_loop();

    /**
     * @brief   : 判断是否需要打断当前任务
     * @param    {LinkageType_E} enLinkageType 联动类型
     * @param    {int} nPriority 新任务优先级
     * @return   {bool} true：已发起打断 false：无需打断
     */
    bool check_and_interrupt(LinkageType_E enLinkageType, int nPriority);

    /**
     * @brief   : 判断指定联动类型是否正在运行
     * @param    {LinkageType_E} enLinkageType 联动类型
     * @return   {bool} true：正在运行 false：未运行
     */
    bool is_task_running(LinkageType_E enLinkageType);

    /**
     * @brief   : 判断任务是否已经过期
     * @param    {LinkageTask_S} &stTask 联动任务
     * @return   {bool} true：已过期 false：未过期
     */
    bool is_task_expired(const LinkageTask_S &stTask) const;

    /**
     * @brief   : 处理同类联动任务运行中的冲突
     * @param    {LinkageTask_S} &stTask 新取出的联动任务
     * @return   {bool} true：任务可以继续执行 false：任务已合并或丢弃
     */
    bool handle_running_task_conflict(const LinkageTask_S &stTask);

    /**
     * @brief   : 回收已完成的异步任务句柄
     */
    void cleanup_finished_async_tasks();

private:
    /* 异步动作执行器引用 */
    EventLinkageAsyncAction &m_asyncAction;

    /* 联动任务优先队列 */
    std::priority_queue<LinkageTask_S> m_linkageQueue;
    /* 队列互斥锁 */
    std::mutex m_queueMutex;
    /* 队列条件变量 */
    std::condition_variable m_queueCV;

    /* worker 主线程 */
    std::thread m_linkageThread;
    /* worker 主线程运行标志 */
    std::atomic<bool> m_bLinkageThreadRunning;

    /* 当前各联动类型正在执行的任务快照 */
    std::map<LinkageType_E, LinkageTask_S> m_currentLinkages;
    /* 当前执行任务互斥锁 */
    std::mutex m_currentLinkageMutex;

    /* 各联动类型运行标志 */
    std::map<LinkageType_E, std::atomic<bool>> m_linkageRunningFlags;
    /* 各联动类型当前优先级 */
    std::map<LinkageType_E, std::atomic<int>> m_linkagePriorities;

    /* 运行中的异步任务句柄集合 */
    std::vector<std::future<void>> m_asyncTasks;
    /* 异步任务句柄互斥锁 */
    std::mutex m_asyncTaskMutex;
};
