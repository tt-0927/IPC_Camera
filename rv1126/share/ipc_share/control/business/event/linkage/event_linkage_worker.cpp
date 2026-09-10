/**
 * @FilePath     : event_linkage_worker.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-15 16:29:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-03 16:30:24
 * @Description  : 事件联动异步任务工作线程实现
 */

#include "event_linkage_worker.h"

#include <algorithm>
#include <chrono>
#include <pthread.h>

#include "event_linkage_dict.h"
#include "time_utils.h"
#include "dlog.h"
#include "IpcRet.h"

namespace
{
/* 异步联动队列最大长度：6个事件 * 每事件最多5个异步联动，预留2个抖动余量 */
constexpr size_t MAX_LINKAGE_QUEUE_SIZE = 32;

/* 联动任务默认有效期，过期任务直接丢弃，避免旧报警挤占实时联动 */
constexpr long long LINKAGE_TASK_EXPIRE_MS = 5000;

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

/**
 * @brief   : 判断两个报警 IO 列表是否存在交集
 * @param    {std::vector<int>} &vecLeft 左侧 IO 列表
 * @param    {std::vector<int>} &vecRight 右侧 IO 列表
 * @return   {bool} true：存在相同 IO false：不存在相同 IO
 */
bool has_same_alarm_output(const std::vector<int> &vecLeft, const std::vector<int> &vecRight)
{
    for (const auto &nLeft : vecLeft)
    {
        if (std::find(vecRight.begin(), vecRight.end(), nLeft) != vecRight.end())
        {
            return true;
        }
    }

    return false;
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
    LinkageTask_S stQueueTask = stTask;
    const long long llEnqueueTimeMs = TimeUtils_NS::get_monotonicTimestampMs();
    if (stQueueTask.llTimestamp <= 0)
    {
        stQueueTask.llTimestamp = llEnqueueTimeMs;
    }
    if (stQueueTask.llExpireTimeMs <= 0)
    {
        stQueueTask.llExpireTimeMs = llEnqueueTimeMs + LINKAGE_TASK_EXPIRE_MS;
    }

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (m_linkageQueue.size() >= MAX_LINKAGE_QUEUE_SIZE)
        {
            dlog_warn("联动任务队列已满，丢弃新任务 - 事件: %s, 联动类型: %d, 优先级: %d, 队列长度: %zu",
                      EventLinkageDict::get_event_name(stQueueTask.stContext.enEventType).c_str(),
                      static_cast<int>(stQueueTask.enLinkageType),
                      stQueueTask.nPriority,
                      m_linkageQueue.size());
            return;
        }

        /* 统一进入优先队列，由工作线程按优先级顺序取出处理 */
        m_linkageQueue.push(stQueueTask);
    }

    dlog_info("添加联动任务到队列 - 事件: %s, 联动类型: %d, 优先级: %d",
              EventLinkageDict::get_event_name(stQueueTask.stContext.enEventType).c_str(),
              static_cast<int>(stQueueTask.enLinkageType),
              stQueueTask.nPriority);
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

        if (is_task_expired(stTask))
        {
            dlog_warn("联动任务已过期，丢弃 - 事件: %s, 联动类型: %d, 优先级: %d",
                      EventLinkageDict::get_event_name(stTask.stContext.enEventType).c_str(),
                      static_cast<int>(stTask.enLinkageType),
                      stTask.nPriority);
            continue;
        }


        /*
         * 闪光任务从登记到异步线程置位之间存在极短启动窗口；该窗口也按“正在执行”处理，
         * 避免人脸抓拍、人脸比对和移动侦测同时触发时重复启动灯光override。
         */
         bool bFlashingTaskStarting = false;
         if (stTask.enLinkageType == LinkageType_E::FLASHING_LIGHT)
         {
             std::lock_guard<std::mutex> lock(m_currentLinkageMutex);
             auto itCurrent = m_currentLinkages.find(LinkageType_E::FLASHING_LIGHT);
             bFlashingTaskStarting = itCurrent != m_currentLinkages.end();
         }
        /* 同类联动若仍在执行，先判断当前任务是否允许抢占 */
        // if (is_task_running(stTask.enLinkageType) && !handle_running_task_conflict(stTask))
         /*
         * SOUND任务登记到m_currentLinkages后，异步线程还要经过调度和播放锁，随后才会
         * 置running=true。把这段时间也视为“启动中”，避免第二条声音进入异步队列，
         * 在第一条播放完后又继续播报。
         */
         bool bSoundTaskStarting = false;
         int nStartingSoundPriority = INT_MAX;
         if (stTask.enLinkageType == LinkageType_E::SOUND && !is_task_running(LinkageType_E::SOUND))
         {
             std::lock_guard<std::mutex> lock(m_currentLinkageMutex);
             auto itCurrent = m_currentLinkages.find(LinkageType_E::SOUND);
             if (itCurrent != m_currentLinkages.end())
             {
                 bSoundTaskStarting = true;
                 nStartingSoundPriority = itCurrent->second.nPriority;
             }
         }
 
         if (bSoundTaskStarting && stTask.nPriority < nStartingSoundPriority)
         {
             /*
              * 高优先级人脸比对到达时，给已登记的抓拍声音最多500ms完成启动置位。
              * 置位后走正常抢占；若旧任务自行退出，则新任务直接执行。
              */
             constexpr int SOUND_START_WAIT_STEP_MS = 10;
             constexpr int SOUND_START_WAIT_MAX_MS = 500;
             int nWaitMs = 0;
             while (!is_task_running(LinkageType_E::SOUND) && nWaitMs < SOUND_START_WAIT_MAX_MS)
             {
                 bool bStillStarting = false;
                 {
                     std::lock_guard<std::mutex> lock(m_currentLinkageMutex);
                     bStillStarting = m_currentLinkages.count(LinkageType_E::SOUND) > 0;
                 }
                 if (!bStillStarting)
                 {
                     break;
                 }
                 std::this_thread::sleep_for(std::chrono::milliseconds(SOUND_START_WAIT_STEP_MS));
                 nWaitMs += SOUND_START_WAIT_STEP_MS;
             }
 
             std::lock_guard<std::mutex> lock(m_currentLinkageMutex);
             bSoundTaskStarting = m_currentLinkages.count(LinkageType_E::SOUND) > 0 &&
                                  !is_task_running(LinkageType_E::SOUND);
         }
 
         /* 同类联动若正在执行，或声光任务仍处于异步启动窗口，先处理冲突。 */
         if ((is_task_running(stTask.enLinkageType) || bFlashingTaskStarting || bSoundTaskStarting) &&
             !handle_running_task_conflict(stTask))
        {
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(m_currentLinkageMutex);
            /* 登记当前联动运行态，供外部查询和后续抢占判断 */
            m_currentLinkages[stTask.enLinkageType] = stTask;
            m_linkagePriorities[stTask.enLinkageType].store(stTask.nPriority);
        }

        dlog_info("处理联动任务 - 事件: %s, 联动类型: %d, 优先级: %d",
                  EventLinkageDict::get_event_name(stTask.stContext.enEventType).c_str(),
                  static_cast<int>(stTask.enLinkageType),
                  stTask.nPriority);

        /* 异步执行具体联动动作，避免阻塞工作线程继续收任务 */
        auto future = std::async(std::launch::async, [this, stTask]() {
            auto &bRunningFlag = m_linkageRunningFlags[stTask.enLinkageType];
            m_asyncAction.execute(stTask, bRunningFlag);

            std::lock_guard<std::mutex> lock(m_currentLinkageMutex);
            auto it = m_currentLinkages.find(stTask.enLinkageType);
            if (it != m_currentLinkages.end() &&
                it->second.llTimestamp == stTask.llTimestamp &&
                it->second.nPriority == stTask.nPriority)
            {
                /* 任务结束后只清理自己的运行态，避免被打断的旧任务误删新任务 */
                m_linkagePriorities[stTask.enLinkageType].store(INT_MAX);
                m_currentLinkages.erase(it);
            }
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

bool EventLinkageWorker::is_task_expired(const LinkageTask_S &stTask) const
{
    return stTask.llExpireTimeMs > 0 && TimeUtils_NS::get_monotonicTimestampMs() > stTask.llExpireTimeMs;
}

bool EventLinkageWorker::handle_running_task_conflict(const LinkageTask_S &stTask)
{
    /* 人脸比对成功使用专用音频，不能用通用报警音路径参与“同音频”去重。 */
    const bool bUseDedicatedFaceAudio = stTask.stContext.enEventType == Event::Type_E::FACE_COMPARE_SUCCESS;
    if (stTask.enLinkageType == LinkageType_E::SOUND)
    {
        std::string strNewAudioPath;
        // if (m_asyncAction.get_audio_file_path(strNewAudioPath) == OK)
        if (!bUseDedicatedFaceAudio && m_asyncAction.get_audio_file_path(strNewAudioPath) == OK)
        {
            const std::string strPlayingAudio = m_asyncAction.get_playing_audio_path();
            if (strPlayingAudio == strNewAudioPath)
            {
                dlog_info("新声音联动任务与当前播放的音频相同(%s)，执行合并去重，直接丢弃", strNewAudioPath.c_str());
                return false;
            }
        }
    }

    if (check_and_interrupt(stTask.enLinkageType, stTask.nPriority))
    {
        /* 最多等待5秒，给被打断的旧任务留出退出时间 */
        int nWaitCount = 0;
        while (is_task_running(stTask.enLinkageType) && nWaitCount < 50)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ++nWaitCount;
        }

        if (is_task_running(stTask.enLinkageType))
        {
            dlog_warn("旧联动任务未在等待窗口内退出，丢弃新任务 - 联动类型: %d, 优先级: %d",
                      static_cast<int>(stTask.enLinkageType),
                      stTask.nPriority);
            return false;
        }

        return true;
    }

    if (stTask.enLinkageType == LinkageType_E::ALARM_IO)
    {
        std::lock_guard<std::mutex> lock(m_currentLinkageMutex);
        auto it = m_currentLinkages.find(LinkageType_E::ALARM_IO);
        if (it != m_currentLinkages.end() && has_same_alarm_output(it->second.vecAlarmOutputNum, stTask.vecAlarmOutputNum))
        {
            dlog_info("报警IO联动任务与当前输出IO重复，执行合并去重，直接丢弃");
            return false;
        }
    }

    dlog_info("同类联动任务正在执行，新任务优先级(%d)低于或等于当前任务，为避免队列积压直接丢弃 - 联动类型: %d",
              stTask.nPriority,
              static_cast<int>(stTask.enLinkageType));
    return false;
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
