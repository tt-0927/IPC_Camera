/**
 * @FilePath     : hvf_detect_common_impl.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-26 16:20:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-28 10:39:00
 * @Description  : HVF 公共模板实现
 */

#pragma once

#include <type_traits>
#include <unistd.h>

#include "get_time.h"
#include "video_frame_jpeg_encoder.hpp"

namespace HVFDetectInternal
{
namespace Detail
{
/**
 * @brief   : 判断当前目标是否需要做置信度过滤
 * @param    {const ot_aidetect_object &} stObject：当前目标
 * @return   {bool} true：需要做置信度过滤 false：无需过滤
 */
inline bool is_need_check_confidence(const ot_aidetect_object &stObject)
{
    return stObject.track_status == OT_AIDETECT_TRACK_STATUS_NEW ||
           stObject.track_status == OT_AIDETECT_TRACK_STATUS_UPDATE;
}

/**
 * @brief   : 判断当前目标是否通过规则置信度阈值
 * @param    {const RuleType &} stRule：事件规则
 * @param    {const ot_aidetect_object &} stObject：当前目标
 * @return   {bool} true：通过 false：未通过
 */
template <typename RuleType>
bool is_confidence_matched(const RuleType &stRule, const ot_aidetect_object &stObject)
{
    if (!is_need_check_confidence(stObject))
    {
        return true;
    }

    /* 当前规则换算后的检测置信度门限 */
    const float fSensitivityThreshold = 1.0f - stRule.nSensitivity / 100.0f;
    return stObject.detect_confidence >= fSensitivityThreshold;
}
} // namespace Detail

template <typename T>
void convert_resolution_and_enable(T &stConfig, int nWidth, int nHeight)
{
    if (stConfig.aRule.empty())
    {
        return;
    }

    /* 是否存在至少一个合法区域 */
    bool bIsInit = false;

    for (auto &stRule : stConfig.aRule)
    {
        stRule.stRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, nWidth, nHeight);
        if (stRule.stRegion.IsValid())
        {
            bIsInit = true;
        }
    }

    if (!bIsInit)
    {
        stConfig.bEnable = false;
    }
}

template <size_t MaxRegions>
void reset_area_status_array(AreaStatus_S (&stStatusArray)[MaxRegions][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM])
{
    for (size_t i = 0; i < MaxRegions; ++i)
    {
        for (size_t j = 0; j < SVP_AIDETECT_MAX_OUTPUT_RECT_NUM; ++j)
        {
            stStatusArray[i][j] = AreaStatus_S();
        }
    }
}

template <size_t MaxRegions>
void reset_boundary_status_array(BoundaryTrackStatus_S (&stStatusArray)[MaxRegions][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM])
{
    for (size_t i = 0; i < MaxRegions; ++i)
    {
        for (size_t j = 0; j < SVP_AIDETECT_MAX_OUTPUT_RECT_NUM; ++j)
        {
            stStatusArray[i][j].reset();
        }
    }
}

#if CAP_EXHIBITION_OSD_PANEL
template <size_t MaxRegions>
void reset_panel_enter_time_array(double (&dEnterTimeArray)[MaxRegions][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM])
{
    for (size_t i = 0; i < MaxRegions; ++i)
    {
        for (size_t j = 0; j < SVP_AIDETECT_MAX_OUTPUT_RECT_NUM; ++j)
        {
            dEnterTimeArray[i][j] = 0;
        }
    }
}
#endif

template <typename RuleType, size_t MaxRegions>
bool process_region_detection(const ot_aidetect_object_of_one_class *pstObjectClass,
                              const std::vector<RuleType> &aRules,
                              AreaStatus_S (&stStatusArray)[MaxRegions][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM],
                              CTargetIndexManager20 &indexManager,
                              CAlarmStateMachine &alarmStateMachine,
                              Event::Type_E enEventType,
                              const char *pszDetectTypeName,
                              const SHVFProcessContext &stCtx
#if CAP_EXHIBITION_OSD_PANEL
                              ,
                              double (*pdPanelEnterTime)[SVP_AIDETECT_MAX_OUTPUT_RECT_NUM]
#endif
)
{
    /* 当前帧是否存在报警 */
    bool bIsAlarm = false;
    const ot_aidetect_object *pAlarmObject = nullptr;
    int nAlarmRuleId = -1;

    if (pstObjectClass)
    {
        /* 当前毫秒时间戳 */
        const double dCurrentTime = get_time_ms();

        for (size_t i = 0; i < pstObjectClass->object_num; ++i)
        {
            /* 当前遍历到的算法目标 */
            const ot_aidetect_object &stObject = pstObjectClass->objects[i];
            /* 当前 track id 对应的内部状态索引 */
            const int nInternalIndex = indexManager.getOrAllocateIndex(stObject.track_id);
            /* 当前目标本帧是否需要继续显示角框 */
            bool bNeedDisplayRect = false;

            if (nInternalIndex < 0 || nInternalIndex >= indexManager.getMaxTargets())
            {
                dlog_warn("无法为track_id %u 分配索引，跳过%s处理", stObject.track_id, pszDetectTypeName);
                continue;
            }

            for (size_t j = 0; j < aRules.size() && j < MaxRegions; ++j)
            {
                /* 当前区域规则 */
                const RuleType &stRule = aRules[j];
                /* 当前区域状态 */
                AreaStatus_S &stAreaStatus = stStatusArray[j][nInternalIndex];
                /* 当前目标是否位于区域内 */
                const bool bInRegion = is_in_region(stRule.stRegion, stObject);

                if constexpr (std::is_same_v<RuleType, Alarm::Intrusion_S>)
                {
                    if (!is_target_match(stRule.aDetectionTarget, pstObjectClass->class_type))
                    {
                        continue;
                    }
                }

                if (!Detail::is_confidence_matched(stRule, stObject))
                {
                    continue;
                }

                if (bInRegion)
                {
                    bNeedDisplayRect = true;
                }

                if (bInRegion && !stAreaStatus.bIsInRegion)
                {
                    stAreaStatus.bIsInRegion = true;
                    stAreaStatus.dEnterTime = dCurrentTime;
#if CAP_EXHIBITION_OSD_PANEL
                    if (enEventType == Event::Type_E::LOITERING_DETECT && pdPanelEnterTime)
                    {
                        pdPanelEnterTime[j][nInternalIndex] = dCurrentTime;
                    }
                    upsert_exhibition_panel_item(stCtx.pstPanelFrame,
                                                 build_hvf_region_panel_item(static_cast<int>(j),
                                                                             pstObjectClass,
                                                                             stObject,
                                                                             false,
                                                                             enEventType,
                                                                             0));
#endif
                    if (!access("testPrint", F_OK))
                    {
                        dlog_debug("目标 ID: %u (内部索引: %d) 进入%s区域 %zu",
                                   stObject.track_id,
                                   nInternalIndex,
                                   pszDetectTypeName,
                                   j + 1);
                    }
                    continue;
                }

                if (!bInRegion && stAreaStatus.bIsInRegion)
                {
                    stAreaStatus.bIsInRegion = false;
                    stAreaStatus.dEnterTime = 0;
#if CAP_EXHIBITION_OSD_PANEL
                    if (enEventType == Event::Type_E::LOITERING_DETECT && pdPanelEnterTime)
                    {
                        pdPanelEnterTime[j][nInternalIndex] = 0;
                    }
#endif
                    if (!access("testPrint", F_OK))
                    {
                        dlog_debug("目标 ID: %u (内部索引: %d) 离开%s区域 %zu",
                                   stObject.track_id,
                                   nInternalIndex,
                                   pszDetectTypeName,
                                   j + 1);
                    }
                    continue;
                }

                if (!bInRegion || !stAreaStatus.bIsInRegion)
                {
                    continue;
                }

                /* 当前目标在区域内持续停留的时长，单位秒 */
                const uint32_t nStayTimeSec = static_cast<uint32_t>((dCurrentTime - stAreaStatus.dEnterTime) / 1000);
#if CAP_EXHIBITION_OSD_PANEL
                /* 面板上需要展示的累计时长，徘徊事件可与报警节拍解耦 */
                uint32_t nPanelDurationSec = nStayTimeSec;
                if (enEventType == Event::Type_E::LOITERING_DETECT && pdPanelEnterTime &&
                    pdPanelEnterTime[j][nInternalIndex] > 0)
                {
                    nPanelDurationSec = static_cast<uint32_t>((dCurrentTime - pdPanelEnterTime[j][nInternalIndex]) /
                                                              1000);
                }
#endif
                if (nStayTimeSec >= stRule.nTimeThreshold)
                {
                    bIsAlarm = true;
                    if (pAlarmObject == nullptr)
                    {
                        pAlarmObject = &stObject;
                        nAlarmRuleId = static_cast<int>(j);
                    }
                    stAreaStatus.dEnterTime = dCurrentTime;
#if CAP_EXHIBITION_OSD_PANEL
                    upsert_exhibition_panel_item(stCtx.pstPanelFrame,
                                                 build_hvf_region_panel_item(static_cast<int>(j),
                                                                             pstObjectClass,
                                                                             stObject,
                                                                             true,
                                                                             enEventType,
                                                                             nPanelDurationSec));
#endif
                    dlog_info("目标 ID: %u (内部索引: %d) 在%s区域 %zu 停留超时，触发报警",
                              stObject.track_id,
                              nInternalIndex,
                              pszDetectTypeName,
                              j + 1);
                    continue;
                }

#if CAP_EXHIBITION_OSD_PANEL
                upsert_exhibition_panel_item(stCtx.pstPanelFrame,
                                             build_hvf_region_panel_item(static_cast<int>(j),
                                                                         pstObjectClass,
                                                                         stObject,
                                                                         false,
                                                                         enEventType,
                                                                         nPanelDurationSec));
#endif
            }

            if (bNeedDisplayRect)
            {
                add_result_to_vector(stObject, stCtx.vstRectInfo);
            }
        }
    }

    EventTriggerContext_S stEventContext;
    stEventContext.enEventType = enEventType;
    stEventContext.nChnId = stCtx.nChnId;
    stEventContext.llTimestamp = stCtx.llTimestamp;
#ifdef ENABLE_TVSDK_SRC
    if (bIsAlarm && pAlarmObject != nullptr)
    {
        fill_hvf_tvsdk_event_context(stEventContext, stCtx, pstObjectClass, *pAlarmObject, nAlarmRuleId);
    }
#endif
    alarmStateMachine.handleAlarmState(bIsAlarm, stEventContext);
    return bIsAlarm;
}

template <size_t MaxRegions>
bool process_region_enter_exit_detection(const ot_aidetect_object_of_one_class *pstObjectClass,
                                         const std::vector<Alarm::EnterExitIntrusion_S> &aRules,
                                         AreaStatus_S (&stStatusArray)[MaxRegions][SVP_AIDETECT_MAX_OUTPUT_RECT_NUM],
                                         CTargetIndexManager20 &indexManager,
                                         CAlarmStateMachine &alarmStateMachine,
                                         Event::Type_E enEventType,
                                         const char *pszDetectTypeName,
                                         const SHVFProcessContext &stCtx
)
{
    /* 当前帧是否存在报警 */
    bool bIsAlarm = false;
    const ot_aidetect_object *pAlarmObject = nullptr;
    int nAlarmRuleId = -1;

    if (pstObjectClass)
    {
        /* 当前毫秒时间戳 */
        const double dCurrentTime = get_time_ms();

        for (size_t i = 0; i < pstObjectClass->object_num; ++i)
        {
            /* 当前遍历到的算法目标 */
            const ot_aidetect_object &stObject = pstObjectClass->objects[i];
            /* 当前 track id 对应的内部状态索引 */
            const int nInternalIndex = indexManager.getOrAllocateIndex(stObject.track_id);
            /* 当前目标本帧是否需要继续显示角框 */
            bool bNeedDisplayRect = false;

            if (nInternalIndex < 0 || nInternalIndex >= indexManager.getMaxTargets())
            {
                dlog_warn("无法为track_id %u 分配索引，跳过%s检测处理", stObject.track_id, pszDetectTypeName);
                continue;
            }

            for (size_t j = 0; j < aRules.size() && j < MaxRegions; ++j)
            {
                /* 当前区域规则 */
                const Alarm::EnterExitIntrusion_S &stRule = aRules[j];
                /* 当前区域状态 */
                AreaStatus_S &stAreaStatus = stStatusArray[j][nInternalIndex];
                /* 当前目标是否位于区域内 */
                const bool bInRegion = is_in_region(stRule.stRegion, stObject);

                if (!is_target_match(stRule.aDetectionTarget, pstObjectClass->class_type))
                {
                    continue;
                }

                if (!Detail::is_confidence_matched(stRule, stObject))
                {
                    continue;
                }

                if (bInRegion)
                {
                    bNeedDisplayRect = true;
                }

                if (enEventType == Event::Type_E::ENTER_REGION)
                {
                    if (bInRegion && !stAreaStatus.bIsInRegion)
                    {
                        stAreaStatus.bIsInRegion = true;
                        stAreaStatus.dEnterTime = dCurrentTime;
                        stAreaStatus.bAlarmed = false;
#if CAP_EXHIBITION_OSD_PANEL
                        upsert_exhibition_panel_item(stCtx.pstPanelFrame,
                                                     build_hvf_region_panel_item(static_cast<int>(j),
                                                                                 pstObjectClass,
                                                                                 stObject,
                                                                                 true,
                                                                                 enEventType,
                                                                                 0));
#endif
                        bIsAlarm = true;
                        if (pAlarmObject == nullptr)
                        {
                            pAlarmObject = &stObject;
                            nAlarmRuleId = static_cast<int>(j);
                        }
                        dlog_info("目标 ID: %u (内部索引: %d) 进入区域[%zu]，触发%s报警",
                                  stObject.track_id,
                                  nInternalIndex,
                                  j + 1,
                                  pszDetectTypeName);
                        continue;
                    }

                    if (!bInRegion && stAreaStatus.bIsInRegion)
                    {
                        stAreaStatus.bIsInRegion = false;
                        stAreaStatus.dEnterTime = 0;
                        stAreaStatus.bAlarmed = false;
                        if (!access("testPrint", F_OK))
                        {
                            dlog_debug("目标 ID: %u (内部索引: %d) 离开区域[%zu]",
                                       stObject.track_id,
                                       nInternalIndex,
                                       j + 1);
                        }
                        continue;
                    }

#if CAP_EXHIBITION_OSD_PANEL
                    if (bInRegion && stAreaStatus.bIsInRegion)
                    {
                        upsert_exhibition_panel_item(stCtx.pstPanelFrame,
                                                     build_hvf_region_panel_item(static_cast<int>(j),
                                                                                 pstObjectClass,
                                                                                 stObject,
                                                                                 false,
                                                                                 enEventType,
                                                                                 0));
                    }
#endif
                    continue;
                }

                if (bInRegion && !stAreaStatus.bIsInRegion)
                {
                    stAreaStatus.bIsInRegion = true;
                    stAreaStatus.dEnterTime = dCurrentTime;
                    stAreaStatus.bAlarmed = false;
#if CAP_EXHIBITION_OSD_PANEL
                    upsert_exhibition_panel_item(stCtx.pstPanelFrame,
                                                 build_hvf_region_panel_item(static_cast<int>(j),
                                                                             pstObjectClass,
                                                                             stObject,
                                                                             false,
                                                                             enEventType,
                                                                             0));
#endif
                    if (!access("testPrint", F_OK))
                    {
                        dlog_debug("目标 ID: %u (内部索引: %d) 进入区域[%zu]",
                                   stObject.track_id,
                                   nInternalIndex,
                                   j + 1);
                    }
                    continue;
                }

                if (!bInRegion && stAreaStatus.bIsInRegion)
                {
                    stAreaStatus.bIsInRegion = false;
                    stAreaStatus.dEnterTime = 0;
                    stAreaStatus.bAlarmed = false;
                    bNeedDisplayRect = true;
#if CAP_EXHIBITION_OSD_PANEL
                    upsert_exhibition_panel_item(stCtx.pstPanelFrame,
                                                 build_hvf_region_panel_item(static_cast<int>(j),
                                                                             pstObjectClass,
                                                                             stObject,
                                                                             true,
                                                                             enEventType,
                                                                             0));
#endif
                    bIsAlarm = true;
                    if (pAlarmObject == nullptr)
                    {
                        pAlarmObject = &stObject;
                        nAlarmRuleId = static_cast<int>(j);
                    }
                    dlog_info("目标 ID: %u (内部索引: %d) 离开区域[%zu]，触发%s报警",
                              stObject.track_id,
                              nInternalIndex,
                              j + 1,
                              pszDetectTypeName);
                    continue;
                }

#if CAP_EXHIBITION_OSD_PANEL
                if (bInRegion && stAreaStatus.bIsInRegion)
                {
                    upsert_exhibition_panel_item(stCtx.pstPanelFrame,
                                                 build_hvf_region_panel_item(static_cast<int>(j),
                                                                             pstObjectClass,
                                                                             stObject,
                                                                             false,
                                                                             enEventType,
                                                                             0));
                }
#endif
            }

            if (bNeedDisplayRect)
            {
                add_result_to_vector(stObject, stCtx.vstRectInfo);
            }
        }
    }

    EventTriggerContext_S stEventContext;
    stEventContext.enEventType = enEventType;
    stEventContext.nChnId = stCtx.nChnId;
    stEventContext.llTimestamp = stCtx.llTimestamp;
#ifdef ENABLE_TVSDK_SRC
    if (bIsAlarm && pAlarmObject != nullptr)
    {
        fill_hvf_tvsdk_event_context(stEventContext, stCtx, pstObjectClass, *pAlarmObject, nAlarmRuleId);
    }
#endif
    alarmStateMachine.handleAlarmState(bIsAlarm, stEventContext);
    return bIsAlarm;
}
} // namespace HVFDetectInternal
