/**
 * @Author: xushou
 * @Date: 2026-11-03
 * @Description: 人员聚集检测子检测器（复用 pmnm_detect 跟踪结果）
 */

#include "crowd_gathering_sub_detector.hpp"
#include "event_configure.h"
#include "alarm_define.h"
#include "common_process.h"
#include "dlog.h"
#include "share_data.h"
#include <chrono>
#include <string>

CCrowdGatheringSubDetector::CCrowdGatheringSubDetector()
    : m_processor(Event::Type::CROWD_GATHERING)
{
}

CCrowdGatheringSubDetector::~CCrowdGatheringSubDetector()
{
}

void CCrowdGatheringSubDetector::updateConfig()
{
    Alarm::CrowdGathering_S stCfg;
    CEventConfigure::instance()->get_configure(stCfg);

    std::vector<CrowdGatheringRule_S> vecRules;
    if (stCfg.bEnable)
    {
        for (size_t i = 0; i < stCfg.aRule.size(); ++i)
        {
            const auto& stAlarmRule = stCfg.aRule[i];
            CrowdGatheringRule_S stRule;
            stRule.nRuleId = static_cast<int>(i);
            stRule.nSensitivity = stAlarmRule.nObjectOccup;

            for (const auto& pt : stAlarmRule.stRegion.aPoint)
            {
                float fX = pt.fX * PIXEL_WIDTH_AI / 1920.0f;
                float fY = pt.fY * PIXEL_HEIGHT_AI / 1080.0f;
                stRule.vecPolygon.emplace_back(static_cast<int>(fX), static_cast<int>(fY));
            }

            if (stRule.vecPolygon.size() >= 3)
            {
                vecRules.push_back(stRule);
            }
        }
    }

    m_processor.setRules(vecRules);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_bEnable = stCfg.bEnable;
    }

    dlog_info("CROWD_GATHERING_SUB: 配置更新 启用[%d] 规则数[%zu]",
              stCfg.bEnable, vecRules.size());
}

int CCrowdGatheringSubDetector::process(const cv::Mat& rgbMat,
                                         const std::vector<TrackedTarget_S>& vecPersonTargets,
                                         const cv::Mat& fullRgbMat,
                                         int nChannel)
{
    (void)rgbMat;

    bool bEnable = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        bEnable = m_bEnable;
    }

    std::vector<CrowdGatheringTrigger_S> vecTriggers;
    if (bEnable)
    {
        const int64_t nProcessTsMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        m_processor.process(nProcessTsMs, vecPersonTargets, vecTriggers);
    }

    const bool bTriggered = !vecTriggers.empty();
#ifdef ENABLE_TVSDK_SRC
    EventTriggerContext_S stContext;
    stContext.nChnId = nChannel;
    stContext.enEventType = Event::Type_E::CROWD_GATHERING;
    stContext.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    if (bTriggered)
    {
        const CrowdGatheringTrigger_S& stTrigger = vecTriggers.front();
        stContext.mapAttrs["rule_id"] = std::to_string(stTrigger.nRuleId);
        dlog_info("CROWD_GATHERING_SUB: 规则[%d] 面积占比[%.2f] 阈值[%u]，触发聚集判定",
                  stTrigger.nRuleId, stTrigger.fAreaRatio, stTrigger.nThreshold);

        /* 多目标聚集不写单目标框 仅带整帧大图 */
        if (!fullRgbMat.empty())
        {
            encode_mat_to_tvsdk_image(fullRgbMat, stContext.stPanoramaImage, JPEG_QUALITY_PANORAMA);
        }
    }

    m_stateMachine.handleAlarmState(bTriggered, stContext);
#else
    (void)fullRgbMat;
    m_stateMachine.handleAlarmState(bTriggered, Event::Type_E::CROWD_GATHERING);
#endif

    dlog_debug("CROWD_GATHERING_SUB: 推理完成 人员数[%zu] 触发规则数[%zu] 聚集标志[%d]",
               vecPersonTargets.size(), vecTriggers.size(), bTriggered ? 1 : 0);

    return 0;
}
