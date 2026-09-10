/**
 * @FilePath     : isp_scene_scheduler.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:47:02
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-16 11:30:00
 * @Description  : 共享ISP场景计划调度器实现
 */

#include "isp_scene_scheduler.h"

#include <chrono>

#include "IpcRet.h"
#include "dlog.h"
#include "isp_scene_schedule_policy.h"

CIspSceneScheduler::CIspSceneScheduler(const IspSchedulerClock_S &stClock,
                                       const ISP::IspCapabilityProfile_S &stProfile,
                                       ScheduleIntentCallback fnCallback)
    : m_stClock(stClock), m_stProfile(stProfile), m_fnCallback(fnCallback),
      m_bRunning(false),
      m_enLastSubmittedScene(ISP::SCENE_NORMAL), m_bLastSubmittedActive(false)
{
}

CIspSceneScheduler::~CIspSceneScheduler()
{
    stop();
}

int CIspSceneScheduler::start()
{
    if (m_bRunning)
    {
        return OK;
    }
    m_bRunning = true;
    m_stThread = std::thread(&CIspSceneScheduler::worker_loop, this);
    return OK;
}

int CIspSceneScheduler::stop()
{
    m_bRunning = false;
    if (m_stThread.joinable())
    {
        m_stThread.join();
    }
    return OK;
}

int CIspSceneScheduler::update(const ISP::SceneSchedule_S &stSchedule)
{
    ISP::SceneSchedule_S stCopy = stSchedule;
    int nRet = IspSceneSchedulePolicy_NS::normalize_scene_schedule(stCopy, m_stProfile);
    if (nRet != OK)
    {
        dlog_error("场景计划校验失败: %d", nRet);
        return nRet;
    }

    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        m_stSchedule = stCopy;
    }
    return OK;
}

void CIspSceneScheduler::worker_loop()
{
    while (m_bRunning)
    {
        /* lock: 配置写入来自网页线程，worker只在此处复制一次，后续裁决不占用配置锁。 */
        ISP::SceneSchedule_S stSchedule;
        {
            std::lock_guard<std::mutex> stLock(m_mtx);
            stSchedule = m_stSchedule;
        }

        /* 每轮都按当前本地时间重新解析，手动校时、跨日和跨月无需额外通知链路。 */
        ISP::SceneType_E enCurrentScene = ISP::SCENE_NORMAL;
        const bool bCurrentActive = resolve_current_scene(stSchedule, enCurrentScene);

        /* 仅在计划目标发生变化时提交；实际参数重放和硬件失败重试仍由reconciler负责。 */
        if (enCurrentScene != m_enLastSubmittedScene || bCurrentActive != m_bLastSubmittedActive)
        {
            const int nRet = m_fnCallback(enCurrentScene, bCurrentActive);
            if (nRet != OK)
            {
                dlog_warn("场景计划提交意图失败: %d, 下轮重新尝试", nRet);
            }
            else
            {
                if (bCurrentActive)
                {
                    dlog_info("场景计划意图已提交, scene:%d", static_cast<int>(enCurrentScene));
                }
                else
                {
                    dlog_info("场景计划未命中，已清除计划覆盖并回落当前场景");
                }
            }
            if (nRet == OK)
            {
                m_enLastSubmittedScene = enCurrentScene;
                m_bLastSubmittedActive = bCurrentActive;
            }
        }

        /* 不使用条件变量；下一秒统一重新读取时间和计划，停止最多等待一个周期。 */
        std::this_thread::sleep_for(std::chrono::seconds(SCHEDULE_CHECK_INTERVAL_SEC));
    }
}

bool CIspSceneScheduler::resolve_current_scene(const ISP::SceneSchedule_S &stSchedule,
                                               ISP::SceneType_E &enScene) const
{
    if (!stSchedule.bEnable)
    {
        return false;
    }

    const IspSchedulerTime_S stCurrentTime = m_stClock.fnGetCurrentTime();
    const int nMonth = stCurrentTime.nMonth;
    const int nDaySeconds = stCurrentTime.nDaySeconds;

    for (const auto &stMonth : stSchedule.aMonthSchedules)
    {
        if (static_cast<int>(stMonth.enMonthfYear) != nMonth)
        {
            continue;
        }

        for (const auto &stTime : stMonth.aSceneTimes)
        {
            if (stTime.nStartTime == stTime.nEndTime)
            {
                continue;
            }

            bool bMatch = false;
            if (stTime.nStartTime < stTime.nEndTime)
            {
                /* 普通区间 [start, end) */
                bMatch = (nDaySeconds >= stTime.nStartTime && nDaySeconds < stTime.nEndTime);
            }
            else
            {
                /* 跨午夜区间 [start, 86400) ∪ [0, end) */
                bMatch = (nDaySeconds >= stTime.nStartTime || nDaySeconds < stTime.nEndTime);
            }

            if (bMatch)
            {
                enScene = stTime.enSceneType;
                return true;
            }
        }
    }

    return false;
}
