/**
 * @FilePath     : isp_scene_schedule_policy.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 11:45:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-14 14:25:34
 * @Description  : 场景计划校验纯策略实现
 */

#include "isp_scene_schedule_policy.h"

#include <algorithm>
#include <set>

#include "IpcRet.h"

namespace
{
/* 一天的总秒数 */
constexpr int SECONDS_PER_DAY = 24 * 60 * 60;

/**
 * @brief   : 判断两个时间段是否重叠
 * @param    {int} a_start：区间A起始秒
 * @param    {int} a_end：区间A结束秒
 * @param    {int} b_start：区间B起始秒
 * @param    {int} b_end：区间B结束秒
 * @return   {bool} true：重叠
 * @note    : 支持跨午夜区间(start>end)。空区间(start==end)不与任何区间重叠。
 */
bool intervals_overlap(int a_start, int a_end, int b_start, int b_end)
{
    if (a_start == a_end || b_start == b_end)
    {
        return false;
    }

    bool a_cross = a_start > a_end;
    bool b_cross = b_start > b_end;

    if (a_cross && b_cross)
    {
        /* 两个跨午夜区间都覆盖午夜点，必然重叠 */
        return true;
    }

    if (a_cross)
    {
        /* A覆盖[a_start,86400)和[0,a_end)，B覆盖[b_start,b_end) */
        return (b_end > a_start) || (b_start < a_end);
    }

    if (b_cross)
    {
        /* B覆盖[b_start,86400)和[0,b_end)，A覆盖[a_start,a_end) */
        return (a_end > b_start) || (a_start < b_end);
    }

    /* 两个非跨午夜区间 */
    return a_start < b_end && b_start < a_end;
}
} // namespace

namespace IspSceneSchedulePolicy_NS
{

int normalize_scene_schedule(ISP::SceneSchedule_S &stConfig, const ISP::IspCapabilityProfile_S &stProfile)
{
    if (!stProfile.stScene.bSupportSchedule)
    {
        return ERR_UNSUPPORT;
    }

    /* 禁用计划允许空月份集合 */
    if (!stConfig.bEnable)
    {
        return OK;
    }

    /* 检查重复月份 */
    std::set<ISP::MonthOfYear_E> setMonths;
    for (const auto &stMonth : stConfig.aMonthSchedules)
    {
        if (setMonths.count(stMonth.enMonthfYear) > 0)
        {
            return ERR_PARAM;
        }
        setMonths.insert(stMonth.enMonthfYear);
    }

    for (const auto &stMonth : stConfig.aMonthSchedules)
    {
        const auto &vecTimes = stMonth.aSceneTimes;

        for (size_t i = 0; i < vecTimes.size(); ++i)
        {
            const auto &stTime = vecTimes[i];

            /* 校验时间范围 */
            if (stTime.nStartTime < 0 || stTime.nStartTime >= SECONDS_PER_DAY)
            {
                return ERR_PARAM;
            }

            if (stTime.nEndTime < 0 || stTime.nEndTime > SECONDS_PER_DAY)
            {
                return ERR_PARAM;
            }

            /* 校验场景类型是否被能力画像支持 */
            bool bSceneSupported = std::find(stProfile.stScene.vecSupportedScenes.begin(),
                                             stProfile.stScene.vecSupportedScenes.end(),
                                             stTime.enSceneType) != stProfile.stScene.vecSupportedScenes.end();
            if (!bSceneSupported)
            {
                return ERR_UNSUPPORT;
            }

            /* 检查同月区间重叠 */
            for (size_t j = i + 1; j < vecTimes.size(); ++j)
            {
                if (intervals_overlap(stTime.nStartTime, stTime.nEndTime,
                                      vecTimes[j].nStartTime, vecTimes[j].nEndTime))
                {
                    return ERR_PARAM;
                }
            }
        }
    }

    return OK;
}

} // namespace IspSceneSchedulePolicy_NS
