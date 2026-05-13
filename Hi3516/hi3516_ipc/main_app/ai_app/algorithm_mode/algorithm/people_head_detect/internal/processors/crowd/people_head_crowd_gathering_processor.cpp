/**
 * @FilePath     : people_head_crowd_gathering_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-23 17:05:08
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 20:32:00
 * @Description  : 人头检测-人员聚集处理器实现
 */

#include "people_head_crowd_gathering_processor.hpp"

#include <algorithm>
#include <string>
#include <unistd.h>

#include "dlog.h"

namespace
{
#if CAP_EXHIBITION_OSD_PANEL
/**
 * @brief   : 构造人员数量显示文本
 * @param    {int} nPeopleCount：人员数量
 * @return   {std::string} 人员数量文本
 */
std::string get_people_count_text(int nPeopleCount)
{
    return std::to_string(std::max(0, nPeopleCount)) + "人";
}

/**
 * @brief   : 构造人员聚集展会面板条目
 * @param    {int} nRegionIndex：区域下标
 * @param    {Region_S} &stRegion：区域配置
 * @param    {int} nPeopleCount：区域内人数
 * @param    {bool} bAlarm：是否报警
 * @return   {PanelItem_S} 展会面板条目
 */
OsdPanel::PanelItem_S build_crowd_panel_item(int nRegionIndex,
                                             const Alarm::Region_S &stRegion,
                                             int nPeopleCount,
                                             bool bAlarm)
{
    /* 当前展会面板条目，承载区域标题、人数和报警状态 */
    OsdPanel::PanelItem_S stItem;
    if (nRegionIndex < 0 || nPeopleCount <= 0)
    {
        return stItem;
    }

    stItem.clear();
    stItem.strTitle = "区域 " + std::to_string(nRegionIndex + 1);
    stItem.bAlarm = bAlarm;
    stItem.bHasRect = true;
    stItem.stRect = to_exhibition_panel_rect(stRegion);
    stItem.nSortKey = nRegionIndex + 1;
    stItem.nPriority = (bAlarm ? 100000 : 0) + nPeopleCount;
    stItem.vecFields = { { "人数", get_people_count_text(nPeopleCount) } };
    return stItem;
}
#endif

/**
 * @brief   : 构造人员聚集联动上下文
 * @param    {SPeopleHeadProcessContext} &stContext：单帧处理上下文
 * @param    {int} nRuleId：触发规则下标
 * @param    {int} nPeopleCount：当前区域人数
 * @return   {EventTriggerContext_S} 联动上下文
 */
EventTriggerContext_S build_crowd_context(const PeopleHeadDetectInternal::SPeopleHeadProcessContext &stContext,
                                          int nRuleId,
                                          int nPeopleCount)
{
    /* 人员聚集联动上下文，承载区域规则与人数摘要字段 */
    EventTriggerContext_S stLinkageContext;
    stLinkageContext.enEventType = Event::Type_E::CROWD_GATHERING;
    stLinkageContext.nChnId = stContext.nChnId;
    stLinkageContext.llTimestamp = stContext.llNowMs;
    stLinkageContext.mapAttrs["rule_id"] = std::to_string(nRuleId);
    stLinkageContext.mapAttrs["people_count"] = std::to_string(nPeopleCount);
    return stLinkageContext;
}
} // namespace

namespace PeopleHeadDetectInternal
{
void CPeopleHeadCrowdGatheringProcessor::setEnabled(bool bEnable)
{
    m_stAlgoCfg.bEnable = bEnable;
    if (!bEnable)
    {
        m_alarmStateMachine.reset();
    }
}

void CPeopleHeadCrowdGatheringProcessor::setAlgoParamCfg(const Alarm::CrowdGathering_S &stAlgoCfg,
                                                         int nWidth,
                                                         int nHeight)
{
    dlog_debug("ai_app: 设置人员聚集侦测参数");
    m_stAlgoCfg = stAlgoCfg;
    convert_crowd_config_and_enable(m_stAlgoCfg, nWidth, nHeight);
}

void CPeopleHeadCrowdGatheringProcessor::process(SPeopleHeadProcessContext &stContext)
{
    if (!m_stAlgoCfg.bEnable)
    {
        return;
    }

#if CAP_EXHIBITION_OSD_PANEL
    prepare_exhibition_panel_frame(stContext.pstPanelFrame, Event::Type_E::CROWD_GATHERING, stContext.nWidth, stContext.nHeight);
#endif

    /* 标记本轮检测是否有任何区域触发告警 */
    bool bAnyRegionTriggered = false;
    /* 当前触发报警的区域规则下标，未触发时保持 -1 */
    int nTriggeredRuleId = -1;
    /* 当前触发报警区域的人数，未触发时保持 0 */
    int nTriggeredPeopleCount = 0;
    for (size_t nRuleIndex = 0; nRuleIndex < m_stAlgoCfg.aRule.size(); ++nRuleIndex)
    {
        /* 当前人员聚集规则配置 */
        const auto &stRule = m_stAlgoCfg.aRule[nRuleIndex];
        /* 当前规则对应的检测区域 */
        const auto &stRegion = stRule.stRegion;
        if (stRegion.nPointNum < 3 || stRegion.aPoint.size() < 3)
        {
            dlog_warn("人员聚集侦测-区域[%zu]点数不足，跳过检测", nRuleIndex);
            continue;
        }

        /* 统计当前区域内人头数量 */
        int nPeopleInRegion = 0;
        for (const auto &stBoxData : stContext.vBoxDatas)
        {
            if (!is_in_region(stRegion, stBoxData.stBoxs))
            {
                continue;
            }

            ++nPeopleInRegion;
            add_result_to_vector(stBoxData, stContext.vstRectInfo);
            if (!access("testPrint", F_OK))
            {
                dlog_debug("人员聚集侦测-区域[%zu]: 检测到人头: 置信度=%.2f, 位置=(%d,%d,%d,%d)",
                           nRuleIndex,
                           stBoxData.fConfidence,
                           stBoxData.stBoxs.nX1,
                           stBoxData.stBoxs.nY1,
                           stBoxData.stBoxs.nX2,
                           stBoxData.stBoxs.nY2);
            }
        }

        /* nObjectOccup 为密度比例阈值，沿用旧逻辑转换为人数阈值 */
        const int nAlarmThreshold = std::max(3, std::min(static_cast<int>(stRule.nObjectOccup / 5), 20));
        /* 当前区域是否达到人员聚集报警阈值 */
        const bool bAlarm = nPeopleInRegion >= nAlarmThreshold;
#if CAP_EXHIBITION_OSD_PANEL
        upsert_exhibition_panel_item(stContext.pstPanelFrame,
                                     build_crowd_panel_item(static_cast<int>(nRuleIndex),
                                                            stRegion,
                                                            nPeopleInRegion,
                                                            bAlarm));
#endif
        if (bAlarm)
        {
            bAnyRegionTriggered = true;
            nTriggeredRuleId = static_cast<int>(nRuleIndex);
            nTriggeredPeopleCount = nPeopleInRegion;
            dlog_info("人员聚集侦测-区域[%zu]满足条件: 检测到%d人, 阈值[%d]",
                      nRuleIndex,
                      nPeopleInRegion,
                      nAlarmThreshold);
            break;
        }
    }

    /* 人员聚集联动上下文，报警结束时状态机会复用最近一次激活上下文 */
    EventTriggerContext_S stLinkageContext = build_crowd_context(stContext, nTriggeredRuleId, nTriggeredPeopleCount);
    m_alarmStateMachine.handleAlarmState(bAnyRegionTriggered, stLinkageContext);
}

bool CPeopleHeadCrowdGatheringProcessor::isEnabled() const
{
    return m_stAlgoCfg.bEnable;
}
} // namespace PeopleHeadDetectInternal
