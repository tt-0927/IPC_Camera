/**
 * @FilePath     : event_linkage_worker.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-15 16:29:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-16 14:21:30
 * @Description  : 事件联动异步任务工作线程实现
 */

#include "event_linkage_worker.h"

#include <chrono>
#include <pthread.h>

#include "event_linkage_dict.h"
#include "dlog.h"
#include "IpcRet.h"

namespace
{
/**
 * @brief   : 初始化支持抢占的联动类型运行态
 * @param    {std::map<LinkageType_E, std::atomic<bool>>} &mapRunningFlags 运行标志表
 * @param    {std::map<LinkageType_E, std::atomic<int>>} &mapPriorities 优先级表
 */
void init_runtime_flags(std::map<LinkageType_E, std::atomic<bool>> &mapRunningFlags,
                        std::map<LinkageType_E, std::atomic<int>> &mapPriorities)
{
    const std::vector<LinkageType_E> vecTypes = {
        LinkageType_E::EMAIL, LinkageType_E::SOUND, LinkageType_E::FLASHING_LIGHT, LinkageType_E::ALARM_IO, LinkageType_E::LOG,
    };

    for (const auto &enType : vecTypes)
    {
        mapRunningFlags[enType].store(false);
        mapPriorities[enType].store(INT_MAX);
    }
}
} // namespace

EventLinkageWorker::EventLinkageWorker(EventLinkageAsyncAction &stAsyncAction)
    : m_asyncAction(stAsyncAction),
      m_bLinkageThreadRunning(false)
{
    init_runtime_flags(m_linkageRunningFlags, m_linkagePriorities);
}

EventLinkageWorker::~EventLinkageWorker()
{
    deinit();
}

int EventLinkageWorker::init()
{
    if (m_bLinkageThreadRunning.load())
    {
        return OK;
    }

    m_bLinkageThreadRunning.store(true);
    m_linkageThread = std::thread(&EventLinkageWorker::task_loop, this);
    return OK;
}

int EventLinkageWorker::deinit()
{
    if (!m_bLinkageThreadRunning.load())
    {
        return OK;
    }

    /* 先通知工作线程退出，避免继续从队列中取出新任务 */
    m_bLinkageThreadRunning.store(false);
    m_queueCV.notify_all();

    if (m_linkageThread.joinable())
    {
        m_linkageThread.join();
    }

    /* 关闭所有可抢占联动的运行标志，让执行中的异步动作尽快结束 */
    for (auto &item : m_linkageRunningFlags)
    {
        item.second.store(false);
    }

    {
        std::lock_guard<std::mutex> lock(m_asyncTaskMutex);
        /* 等待已经启动的异步任务收尾，防止对象析构后仍访问成员 */
        for (auto &task : m_asyncTasks)
        {
            task.wait();
        }
        m_asyncTasks.clear();
    }

    return OK;
}

void EventLinkageWorker::pushTask(const LinkageTask_S &stTask)
{
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        /* 统一进入优先队列，由工作线程按优先级顺序取出处理 */
        m_linkageQueue.push(stTask);
    }

    dlog_info("添加联动任务到队列 - 事件: %s, 联动类型: %d, 优先级: %d",
              EventLinkageDict::get_event_name(stTask.stContext.enEventType).c_str(),
              static_cast<int>(stTask.enLinkageType),
              stTask.nPriority);
    m_queueCV.notify_one();
}

bool EventLinkageWorker::stopTask(LinkageType_E enLinkageType)
{
    if (!m_linkageRunningFlags.count(enLinkageType))
    {
        return false;
    }

    const bool bWasRunning = m_linkageRunningFlags[enLinkageType].load();
    if (bWasRunning)
    {
        m_linkageRunningFlags[enLinkageType].store(false);
    }

    return bWasRunning;
}

std::atomic<bool> &EventLinkageWorker::getRunningFlag(LinkageType_E enLinkageType)
{
    return m_linkageRunningFlags[enLinkageType];
}

void EventLinkageWorker::task_loop()
{
    pthread_setname_np(pthread_self(), "EventLinkTask");
    dlog_info("联动任务处理线程启动");

    while (m_bLinkageThreadRunning.load())
    {
        LinkageTask_S stTask;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            /* 等待新任务或退出信号，避免线程空转占用CPU */
            m_queueCV.wait(lock, [this]() { return !m_linkageQueue.empty() || !m_bLinkageThreadRunning.load(); });
            if (!m_bLinkageThreadRunning.load())
            {
                break;
            }

            if (m_linkageQueue.empty())
            {
                continue;
            }

            /* 取出当前优先级最高的联动任务 */
            stTask = m_linkageQueue.top();
            m_linkageQueue.pop();
        }

        dlog_info("处理联动任务 - 事件: %s, 联动类型: %d, 优先级: %d",
                  EventLinkageDict::get_event_name(stTask.stContext.enEventType).c_str(),
                  static_cast<int>(stTask.enLinkageType),
                  stTask.nPriority);

        /* 同类联动若仍在执行，先判断当前任务是否允许抢占 */
        if (is_task_running(stTask.enLinkageType))
        {
            if (check_and_interrupt(stTask.enLinkageType, stTask.nPriority))
            {
                /* 最多等待5秒，给被打断的旧任务留出退出时间 */
                int nWaitCount = 0;
                while (is_task_running(stTask.enLinkageType) && nWaitCount < 50)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    ++nWaitCount;
                }
            }
            else
            {
                /* 旧任务优先级更高时，新任务回到队列稍后再试 */
                requeue_task(stTask);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_currentLinkageMutex);
            /* 登记当前联动运行态，供外部查询和后续抢占判断 */
            m_currentLinkages[stTask.enLinkageType] = stTask;
            m_linkagePriorities[stTask.enLinkageType].store(stTask.nPriority);
        }

        /* 异步执行具体联动动作，避免阻塞工作线程继续收任务 */
        auto future = std::async(std::launch::async, [this, stTask]() {
            auto &bRunningFlag = m_linkageRunningFlags[stTask.enLinkageType];
            m_asyncAction.execute(stTask, bRunningFlag);

            std::lock_guard<std::mutex> lock(m_currentLinkageMutex);
            /* 任务结束后及时清空运行态，恢复该联动类型的可执行状态 */
            m_linkagePriorities[stTask.enLinkageType].store(INT_MAX);
            m_currentLinkages.erase(stTask.enLinkageType);
        });

        {
            std::lock_guard<std::mutex> lock(m_asyncTaskMutex);
            m_asyncTasks.emplace_back(std::move(future));
        }

        /* 顺手清理已经完成的future，避免容器持续增长 */
        cleanup_finished_async_tasks();
    }

    dlog_info("联动任务处理线程退出");
}

bool EventLinkageWorker::check_and_interrupt(LinkageType_E enLinkageType, int nPriority)
{
    if (!m_linkageRunningFlags.count(enLinkageType) || !m_linkageRunningFlags[enLinkageType].load())
    {
        return false;
    }

    /* 当前正在执行的任务优先级，用来判断是否允许被新任务打断 */
    const int nCurrentPriority = m_linkagePriorities[enLinkageType].load();
    if (nPriority < nCurrentPriority)
    {
        dlog_info("新任务优先级(%d)高于当前任务(%d)，执行打断 - 联动类型: %d",
                  nPriority,
                  nCurrentPriority,
                  static_cast<int>(enLinkageType));
        m_linkageRunningFlags[enLinkageType].store(false);
        return true;
    }

    return false;
}

bool EventLinkageWorker::is_task_running(LinkageType_E enLinkageType)
{
    return m_linkageRunningFlags.count(enLinkageType) && m_linkageRunningFlags[enLinkageType].load();
}

void EventLinkageWorker::requeue_task(const LinkageTask_S &stTask)
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    /* 同类任务暂时不能执行时，重新放回优先队列等待下一轮调度 */
    m_linkageQueue.push(stTask);
    m_queueCV.notify_one();
}

void EventLinkageWorker::cleanup_finished_async_tasks()
{
    std::lock_guard<std::mutex> lock(m_asyncTaskMutex);
    for (auto it = m_asyncTasks.begin(); it != m_asyncTasks.end();)
    {
        /* 仅回收已经完成的future，未结束的任务继续保留 */
        if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            it->wait();
            it = m_asyncTasks.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
