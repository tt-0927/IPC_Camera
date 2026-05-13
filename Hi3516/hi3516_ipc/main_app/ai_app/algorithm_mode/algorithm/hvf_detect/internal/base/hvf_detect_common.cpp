/**
 * @FilePath     : hvf_detect_common.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-26 16:20:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-28 10:39:03
 * @Description  : HVF 公共基础能力定义
 */

#include "hvf_detect_common.hpp"

#include <algorithm>

namespace HVFDetectInternal
{
bool is_target_match(const std::vector<int> &aDetectTarget, const ot_aidetect_class &enAiDetectClass)
{
    if (aDetectTarget.empty())
    {
        return false;
    }

    /* 当前 AI 类别映射后的业务检测目标枚举 */
    Alarm::DetectionTarget_E enDetectionTarget;
    switch (enAiDetectClass)
    {
    case OT_AIDETECT_CLASS_HUMAN:
        enDetectionTarget = Alarm::HUMAN_DETECTION;
        break;
    case OT_AIDETECT_CLASS_VEHICLE:
        enDetectionTarget = Alarm::CAR_DETECTION;
        break;
    case OT_AIDETECT_CLASS_FACE:
        enDetectionTarget = Alarm::OTHER_DETECTION;
        break;
    default:
        return false;
    }

    return std::find(aDetectTarget.begin(), aDetectTarget.end(), static_cast<int>(enDetectionTarget)) !=
           aDetectTarget.end();
}

const ot_aidetect_object_of_one_class *find_object_class(const ot_aidetect_result_array &stResult,
                                                         ot_aidetect_class enClassType)
{
    for (size_t i = 0; i < stResult.class_num; ++i)
    {
        if (stResult.object_class[i].class_type == enClassType)
        {
            return &stResult.object_class[i];
        }
    }

    return nullptr;
}

std::set<int> collect_track_ids(const ot_aidetect_object_of_one_class *pstObjectClass)
{
    /* 当前类别下收集到的全部 track id */
    std::set<int> stTrackIds;
    if (!pstObjectClass)
    {
        return stTrackIds;
    }

    for (size_t i = 0; i < pstObjectClass->object_num; ++i)
    {
        stTrackIds.insert(pstObjectClass->objects[i].track_id);
    }

    return stTrackIds;
}

std::set<int> collect_all_track_ids(const ot_aidetect_result_array &stResult)
{
    /* 当前整帧收集到的全部 track id */
    std::set<int> stTrackIds;

    for (size_t i = 0; i < stResult.class_num; ++i)
    {
        for (size_t j = 0; j < stResult.object_class[i].object_num; ++j)
        {
            stTrackIds.insert(stResult.object_class[i].objects[j].track_id);
        }
    }

    return stTrackIds;
}

void convert_boundary_and_enable(Alarm::BoundaryDetection_S &stConfig, int nWidth, int nHeight)
{
    if (stConfig.aRule.empty())
    {
        return;
    }

    /* 是否存在至少一条合法警戒线 */
    bool bIsInit = false;

    for (auto &stRule : stConfig.aRule)
    {
        stRule.stStartPos.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, nWidth, nHeight);
        stRule.stEndPos.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, nWidth, nHeight);
        if ((stRule.stStartPos.fX != stRule.stEndPos.fX) || (stRule.stStartPos.fY != stRule.stEndPos.fY))
        {
            bIsInit = true;
        }
    }

    if (!bIsInit)
    {
        stConfig.bEnable = false;
    }
}

#if CAP_EXHIBITION_OSD_PANEL
std::string get_hvf_target_text(ot_aidetect_class enClassType)
{
    switch (enClassType)
    {
    case OT_AIDETECT_CLASS_HUMAN:
        return "人";
    case OT_AIDETECT_CLASS_VEHICLE:
        return "车";
    default:
        return "其他";
    }
}

std::string get_hvf_loiter_duration_text(uint32_t nDurationSec)
{
    return std::to_string(nDurationSec) + "秒";
}

std::vector<OsdPanel::PanelField_S> build_hvf_region_panel_fields(Event::Type_E enEventType,
                                                                  const ot_aidetect_object_of_one_class *pstObjectClass,
                                                                  const ot_aidetect_object &stObject,
                                                                  bool bAlarm,
                                                                  uint32_t nDurationSec)
{
    /* 当前条目对应的 track id 文本 */
    const std::string strTrackId = std::to_string(stObject.track_id);
    /* 当前条目对应的置信度文本 */
    const std::string strConfidence = get_exhibition_panel_confidence_text(stObject.detect_confidence);
    /* 当前条目对应的状态文本 */
    const std::string strStatus = get_exhibition_panel_status_text(bAlarm);

    if (enEventType == Event::Type_E::LOITERING_DETECT)
    {
        return {
            {           "ID",                                 strTrackId },
            { "徘徊时长", get_hvf_loiter_duration_text(nDurationSec) },
            {    "置信度",                              strConfidence },
            {       "状态",                                  strStatus }
        };
    }

    return {
        {    "目标", pstObjectClass ? get_hvf_target_text(pstObjectClass->class_type) : "其他" },
        {        "ID",                                                                  strTrackId },
        { "置信度",                                                               strConfidence },
        {    "状态",                                                                   strStatus }
    };
}

OsdPanel::PanelItem_S build_hvf_region_panel_item(int nRegionIndex,
                                                  const ot_aidetect_object_of_one_class *pstObjectClass,
                                                  const ot_aidetect_object &stObject,
                                                  bool bAlarm,
                                                  Event::Type_E enEventType,
                                                  uint32_t nDurationSec)
{
    /* 当前目标转换后的候选面板项 */
    OsdPanel::PanelItem_S stCandidate;
    if (!pstObjectClass || nRegionIndex < 0)
    {
        return stCandidate;
    }

    stCandidate.clear();
    stCandidate.strTitle = "区域 " + std::to_string(nRegionIndex + 1);
    stCandidate.bAlarm = bAlarm;
    stCandidate.bHasRect = true;
    stCandidate.stRect = to_exhibition_panel_rect(stObject);
    stCandidate.nSortKey = nRegionIndex + 1;
    stCandidate.nPriority = build_exhibition_panel_priority(bAlarm, stObject.detect_confidence);
    stCandidate.vecFields = build_hvf_region_panel_fields(enEventType, pstObjectClass, stObject, bAlarm, nDurationSec);
    return stCandidate;
}
#endif
} // namespace HVFDetectInternal
