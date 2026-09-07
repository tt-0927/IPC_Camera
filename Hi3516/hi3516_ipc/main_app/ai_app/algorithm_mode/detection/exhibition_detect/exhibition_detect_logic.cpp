/**
 * @FilePath     : exhibition_detect_logic.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 16:00:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 16:30:00
 * @Description  : 展馆人流统计/人员密度检测配置与规则逻辑实现
 */

#include "exhibition_detect_logic.hpp"

#include "event_configure.h"

void CExhibitionDetectLogic::apply_algo_config(const Event::AlgorithmConfig &stAlgoConfig)
{
    std::lock_guard<std::mutex> lock(m_configMutex);

    /* 人流总开关关闭：直接禁用，规则状态保留到下次开启时校验 */
    if (stAlgoConfig.nEnPeopleFlowStatistics == 0)
    {
        /* 从生效态切到关闭态时，跟踪器旧 Track 需清空，避免再次开启后残留轨迹 */
        if (m_bAlgoEnabled && m_stConfig.bEnable)
        {
            m_bTrackerRebuildPending = true;
        }
        m_bAlgoEnabled = false;
    }
    else
    {
        m_bAlgoEnabled = true;

        /* 与 HVF 人流统计共用同一份配置，读取完整配置一次性应用；
         * 读取失败时 stInfo 保持默认（bEnable=false），处理器不使能 */
        Alarm::PeopleFlowStatistics_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);

        /* 规则几何/方向变化时清空旧 Track 状态，避免旧轨迹跨新规则线产生伪跨线；
         * 生效态切到关闭态同样重建（配置使能关闭） */
        if (is_rule_changed_unlocked(stInfo) ||
            (m_bAlgoEnabled && m_stConfig.bEnable && !stInfo.bEnable))
        {
            m_bTrackerRebuildPending = true;
        }
        m_stConfig = stInfo;
    }

#if CAP_AI_PEOPLE_DENSITY_PIPELINE
    /* 人员密度：head 跟踪器在密度关闭期间不更新，生效状态翻转时重建清空陈旧 Track */
    const bool bDensityWasActive = m_bDensityAlgoEnabled && m_stDensityConfig.bEnable;
    if (stAlgoConfig.nEnPeopleDensityDetection == 0)
    {
        m_bDensityAlgoEnabled = false;
    }
    else
    {
        m_bDensityAlgoEnabled = true;

        /* 与 HVF 人员密度 V2 共用同一份配置，读取完整配置一次性应用 */
        Alarm::PeopleDensityDetection_S stDensityInfo;
        CEventConfigure::instance()->get_configure(stDensityInfo);
        m_stDensityConfig = stDensityInfo;
    }

    const bool bDensityNowActive = m_bDensityAlgoEnabled && m_stDensityConfig.bEnable;
    if (bDensityWasActive != bDensityNowActive)
    {
        m_bTrackerRebuildPending = true;
    }
#endif
}

bool CExhibitionDetectLogic::is_enabled() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_bAlgoEnabled || m_bDensityAlgoEnabled;
}

bool CExhibitionDetectLogic::is_flow_active() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_bAlgoEnabled && m_stConfig.bEnable;
}

bool CExhibitionDetectLogic::is_density_active() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_bDensityAlgoEnabled && m_stDensityConfig.bEnable;
}

int CExhibitionDetectLogic::tracker_max_age() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_nTrackerMaxAge;
}

int CExhibitionDetectLogic::tracker_min_hits() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_nTrackerMinHits;
}

float CExhibitionDetectLogic::tracker_iou_threshold() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_fTrackerIouThreshold;
}

int CExhibitionDetectLogic::head_tracker_max_age() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_nHeadTrackerMaxAge;
}

int CExhibitionDetectLogic::head_tracker_min_hits() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_nHeadTrackerMinHits;
}

float CExhibitionDetectLogic::head_tracker_iou_threshold() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_fHeadTrackerIouThreshold;
}

int CExhibitionDetectLogic::tracker_max_tracks() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_nTrackerMaxTracks;
}

bool CExhibitionDetectLogic::consume_tracker_rebuild_request()
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    const bool bPending = m_bTrackerRebuildPending;
    m_bTrackerRebuildPending = false;
    return bPending;
}

void CExhibitionDetectLogic::snapshot_config(Alarm::PeopleFlowStatistics_S &stOut) const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    stOut = m_stConfig;
}

void CExhibitionDetectLogic::snapshot_density_config(Alarm::PeopleDensityDetection_S &stOut) const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    stOut = m_stDensityConfig;
}

bool CExhibitionDetectLogic::is_rule_changed_unlocked(const Alarm::PeopleFlowStatistics_S &stNewConfig) const
{
    const Alarm::PeopleFlowRuleLine_S &stOldLine = m_stConfig.stRuleLine;
    const Alarm::PeopleFlowRuleLine_S &stNewLine = stNewConfig.stRuleLine;

    /* 规则线起点/终点/方向任一变化即视为规则变化 */
    if (stOldLine.stStartPos.fX != stNewLine.stStartPos.fX || stOldLine.stStartPos.fY != stNewLine.stStartPos.fY ||
        stOldLine.stEndPos.fX != stNewLine.stEndPos.fX || stOldLine.stEndPos.fY != stNewLine.stEndPos.fY ||
        stOldLine.enDirection != stNewLine.enDirection)
    {
        return true;
    }

    /* 检测区域点集变化即视为规则变化（首轮配置固定为四边形） */
    const Alarm::Region_S &stOldRegion = m_stConfig.stDetectRegion;
    const Alarm::Region_S &stNewRegion = stNewConfig.stDetectRegion;
    if (stOldRegion.nPointNum != stNewRegion.nPointNum)
    {
        return true;
    }
    for (unsigned int nIdx = 0; nIdx < stNewRegion.nPointNum; ++nIdx)
    {
        if (nIdx >= stOldRegion.aPoint.size() || nIdx >= stNewRegion.aPoint.size())
        {
            return true;
        }
        const Common::PosF_S &stOldPoint = stOldRegion.aPoint[nIdx];
        const Common::PosF_S &stNewPoint = stNewRegion.aPoint[nIdx];
        if (stOldPoint.fX != stNewPoint.fX || stOldPoint.fY != stNewPoint.fY)
        {
            return true;
        }
    }
    return false;
}
