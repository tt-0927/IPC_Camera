/*** 
 * @FilePath     : isp_sceneCtrl.cpp
 * @Author       : cyc
 * @Date         : 2025-06-20 16:17:20
 * @LastEditors  : cyc
 * @LastEditTime : 2026-01-09 16:47:00
 * @Description  : 图像场景定时控制
 */

#include <chrono>
#include <ctime>
#include <unistd.h>
#include <thread>
#include "isp_sceneCtrl.h"
#include "isp_configure.h"
#include "dlog.h"
#include "isp_scene.h"
#include "time_utils.h"
#include "isp_manage.h"

CSceneCtrl::CSceneCtrl()
{
    m_bRun.store(true, std::memory_order_release);
    std::thread tid;
    tid = std::thread(&CSceneCtrl::run, this);
    tid.detach();
}

CSceneCtrl::~CSceneCtrl()
{
    m_bRun.store(false, std::memory_order_release);
}

/* 获取当前的月份 */
static int getTodayMonth()
{
    /* 获取当前时间点 */ 
    auto now = std::chrono::system_clock::now();

    /* 转换为 time_t */ 
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    /* 转换为 tm 结构 */ 
    std::tm localTime;
    /* 使用线程安全的 localtime_r */
    localtime_r(&t, &localTime); 

    /*note  tm_mon 返回的月份是从 0 开始的（0 表示一月，1 表示二月，依此类推）*/
    /*note 需要加 1 转换为常规的月份表示（1 表示一月，2 表示二月，依此类推） */ 
    return localTime.tm_mon + 1;
}

void CSceneCtrl::update()
{
    dlog_info("更新图像计划");
    ISP::SceneSchedule_S stSchedules;
    CIspConfigure::instance()->get_configure(stSchedules);

    /* 启用的计划 */
    if (!stSchedules.bEnable)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_infos.clear();
        m_bStart.store(false);
        return;
    }

    /* 使用临时变量，减少对 m_infos 的重复操作 */
    std::vector<Info_S> vstTempInfos; 
    vstTempInfos.reserve(12); // 预分配空间
    /* 现在检查是否有有效的计划时间 */
    bool bHasValidSchedule = false;
        
    /* 每月的图像计划 */
    for (const auto& monthSchedule : stSchedules.aMonthSchedules)
    {
        if (monthSchedule.aSceneTimes.empty())
        {
            // dlog_warn("No scene times configured for month: %d", (int)monthSchedule.enMonthfYear);
            continue;
        }

        bHasValidSchedule = true;
        
        Info_S stInfo;
        stInfo.nMonthOfYear = static_cast<int>(monthSchedule.enMonthfYear);
        stInfo.sceneTimes = monthSchedule.aSceneTimes;
        
        /* 验证时间配置的合理性 */ 
        for (const auto& sceneTime : stInfo.sceneTimes)
        {
            if (sceneTime.nStartTime < 0 || sceneTime.nStartTime > 24*3600 ||
                sceneTime.nEndTime < 0 || sceneTime.nEndTime > 24*3600)
            {
                dlog_warn("Invalid time range: %d-%d for month %d", 
                         sceneTime.nStartTime, sceneTime.nEndTime, stInfo.nMonthOfYear);
            }
        }

        vstTempInfos.push_back(std::move(stInfo));
        
    }

    if(bHasValidSchedule)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_infos = std::move(vstTempInfos); 
        m_bStart.store(true);
    }
    else 
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_infos.clear();
        m_bStart.store(false); 
    }
    
}

bool isInRange(ISP::SceneTime_S stRange,int nCurTime) 
{
    /* 处理跨天情况 (如 23:00 到 06:00) */ 
    if (stRange.nStartTime > stRange.nEndTime) 
    {
        return (nCurTime >= stRange.nStartTime) || (nCurTime < stRange.nEndTime);
    }
    else 
    {
        return (nCurTime >= stRange.nStartTime) && (nCurTime < stRange.nEndTime);
    }
}

ISP::SceneType_E CSceneCtrl::determine_current_scene(int nMonth, int nCurTime)
{
    std::lock_guard<std::mutex> lock(m_mutex);
        
    for (const auto &info : m_infos)
    {
        if (info.nMonthOfYear == nMonth)
        {
            for (const auto &sceneTime : info.sceneTimes)
            {
                ISP::SceneTime_S stRange = 
                {
                    sceneTime.enSceneType,
                    sceneTime.nStartTime,
                    sceneTime.nEndTime
                };
                if (isInRange(stRange,nCurTime))
                {
                    return sceneTime.enSceneType;
                }
            }
            break;
        }
    }
    
    return ISP::SCENE_NORMAL;  
    
}

void CSceneCtrl::run()
{
    pthread_setname_np(pthread_self(), "SceneCtrlRun");

    ISP::SceneType_E enLastSceneType;
    CIspConfigure::instance()->get_configure(enLastSceneType);
    while (m_bRun.load(std::memory_order_acquire))
    {
        usleep(100 * 1000);

        if(!m_bStart.load())
        {
            usleep(1000);
            continue;
        }
        /* 当前几月份 */
        int nDayOfMonth = getTodayMonth();
        
        /* 自当天开始的秒数 */
        int nCurrentTime = TimeUtils_NS::getSecondsSinceStartOfDay();
        CIspConfigure::instance()->get_configure(enLastSceneType);
        ISP::SceneType_E enTargetScene = determine_current_scene(nDayOfMonth,nCurrentTime);
         
        if (enTargetScene != enLastSceneType)
        { 
            dlog_info("Scene switched to: %d at %02d:%02d:%02d", 
                enTargetScene, 
                nCurrentTime / 3600, 
                    (nCurrentTime % 3600) / 60, 
                    nCurrentTime % 60);
                CIspManage::instance()->apply_scene_all_params(enTargetScene);
            
        }
    }
}