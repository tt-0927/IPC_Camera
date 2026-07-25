/**
 * @FilePath     : hvf_detect_common.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-26 16:20:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-01 14:32:22
 * @Description  : HVF 公共基础能力定义
 */

#include "hvf_detect_common.hpp"

#include <algorithm>

#include "dlog.h"
#include "IpcRet.h"
#include "video_frame_jpeg_encoder.hpp"

namespace HVFDetectInternal
{
namespace
{
Common::RectInfo_S build_object_rect(const ot_aidetect_object &stObject)
{
    Common::RectInfo_S stRect;
    stRect.nX1 = stObject.detect_rect.x;
    stRect.nY1 = stObject.detect_rect.y;
    stRect.nX2 = stObject.detect_rect.x + stObject.detect_rect.width;
    stRect.nY2 = stObject.detect_rect.y + stObject.detect_rect.height;
    return stRect;
}

bool encode_hvf_target_image(ot_video_frame_info *pFrameInfo,
                             const Common::RectInfo_S &stRectInfo,
                             EventTvSdkImage_S &stImage)
{
    stImage = EventTvSdkImage_S();
    if (pFrameInfo == nullptr)
    {
        return false;
    }

    const int nOrigW = stRectInfo.nX2 - stRectInfo.nX1;
    const int nOrigH = stRectInfo.nY2 - stRectInfo.nY1;
    if (nOrigW <= 0 || nOrigH <= 0)
    {
        dlog_warn("HVF事件特写图跳过，原始目标框无效 [%d,%d,%d,%d]",
                  stRectInfo.nX1,
                  stRectInfo.nY1,
                  stRectInfo.nX2,
                  stRectInfo.nY2);
        return false;
    }

    constexpr float TARGET_SCALE_RATIO = 1.5f;
    const int nMaxSide = std::max(nOrigW, nOrigH);
    const int nSquareSize = static_cast<int>(nMaxSide * TARGET_SCALE_RATIO);
    const int nCenterX = (stRectInfo.nX1 + stRectInfo.nX2) / 2;
    const int nCenterY = (stRectInfo.nY1 + stRectInfo.nY2) / 2;

    Common::RectInfo_S stCropRect;
    stCropRect.nX1 = nCenterX - nSquareSize / 2;
    stCropRect.nY1 = nCenterY - nSquareSize / 2;
    stCropRect.nX2 = stCropRect.nX1 + nSquareSize;
    stCropRect.nY2 = stCropRect.nY1 + nSquareSize;

    const int nFrameW = pFrameInfo->video_frame.width;
    const int nFrameH = pFrameInfo->video_frame.height;
    stCropRect.nX1 = std::max(0, stCropRect.nX1);
    stCropRect.nY1 = std::max(0, stCropRect.nY1);
    stCropRect.nX2 = std::min(nFrameW, stCropRect.nX2);
    stCropRect.nY2 = std::min(nFrameH, stCropRect.nY2);

    stCropRect.nX1 = ALIGN_BACK(stCropRect.nX1, 16);
    stCropRect.nY1 = ALIGN_BACK(stCropRect.nY1, 4);
    stCropRect.nX2 = ALIGN_UP(stCropRect.nX2, 16);
    stCropRect.nY2 = ALIGN_UP(stCropRect.nY2, 4);
    stCropRect.nX2 = std::min(nFrameW, stCropRect.nX2);
    stCropRect.nY2 = std::min(nFrameH, stCropRect.nY2);

    if (stCropRect.nX2 <= stCropRect.nX1 || stCropRect.nY2 <= stCropRect.nY1)
    {
        dlog_warn("HVF事件特写图裁剪框对齐后无效 [%d,%d,%d,%d]",
                  stCropRect.nX1,
                  stCropRect.nY1,
                  stCropRect.nX2,
                  stCropRect.nY2);
        return false;
    }

    const unsigned int unDstWidth = stCropRect.nX2 - stCropRect.nX1;
    const unsigned int unDstHeight = stCropRect.nY2 - stCropRect.nY1;

    ot_video_frame_info stDstFrameInfo;
    if (TD_SUCCESS != mppVgs_create_video_frame_info(
                          unDstWidth, unDstHeight, OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420, &stDstFrameInfo))
    {
        dlog_warn("HVF事件特写图创建 VGS 帧失败 [%u x %u]", unDstWidth, unDstHeight);
        return false;
    }

    ot_rect stVgsRect;
    stVgsRect.x = stCropRect.nX1;
    stVgsRect.y = stCropRect.nY1;
    stVgsRect.width = unDstWidth;
    stVgsRect.height = unDstHeight;

    if (TD_SUCCESS != mppVgs_crop(pFrameInfo, &stDstFrameInfo, &stVgsRect))
    {
        dlog_warn("HVF事件特写图 VGS 裁剪失败");
        mppVgs_destroy_video_frame_info(&stDstFrameInfo);
        return false;
    }

    const int nEncodeRet = AiAppCommon::encode_video_frame_to_jpeg_memory(&stDstFrameInfo, stImage);
    mppVgs_destroy_video_frame_info(&stDstFrameInfo);

    if (nEncodeRet != OK)
    {
        dlog_warn("HVF事件特写图 JPEG 编码失败");
        stImage = EventTvSdkImage_S();
        return false;
    }

    stImage.strTag = "target";
    return true;
}
} // namespace

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
        /* 仅收集存活状态（NEW/UPDATE）的 track_id；
         * DIE 状态表示目标已消失，不纳入当前帧活跃集合，
         * 使 cleanupLostTargets 能正确回收其占用的内部索引 */
        if (pstObjectClass->objects[i].track_status != OT_AIDETECT_TRACK_STATUS_DIE)
        {
            stTrackIds.insert(pstObjectClass->objects[i].track_id);
        }
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

int to_tvsdk_object_type(ot_aidetect_class enAiDetectClass)
{
    switch (enAiDetectClass)
    {
    case OT_AIDETECT_CLASS_HUMAN:    return 1;
    case OT_AIDETECT_CLASS_VEHICLE:  return 2;
    default:                         return 0;
    }
}

void fill_hvf_tvsdk_event_context(EventTriggerContext_S &stEventContext,
                                  const SHVFProcessContext &stCtx,
                                  const ot_aidetect_object_of_one_class *pstObjectClass,
                                  const ot_aidetect_object &stObject,
                                  int nRuleId)
{
#ifdef ENABLE_TVSDK_SRC
    stEventContext.mapAttrs["rule_id"] = std::to_string(std::max(0, nRuleId));
    stEventContext.nTargetId = static_cast<int>(stObject.track_id);
    stEventContext.nObjectType = pstObjectClass ? to_tvsdk_object_type(pstObjectClass->class_type) : 0;
    stEventContext.fConfidence = stObject.detect_confidence;

    const Common::RectInfo_S stRect = build_object_rect(stObject);
    stEventContext.nLeft = stRect.nX1;
    stEventContext.nTop = stRect.nY1;
    stEventContext.nRight = stRect.nX2;
    stEventContext.nBottom = stRect.nY2;

    if (stCtx.pFrameInfo == nullptr)
    {
        dlog_warn("HVF TVSDK上下文未填图: event[%d], rule[%d], target[%u], frame=null",
                  static_cast<int>(stEventContext.enEventType),
                  nRuleId,
                  stObject.track_id);
        return;
    }

    if (AiAppCommon::encode_video_frame_to_jpeg_memory(stCtx.pFrameInfo, stEventContext.stPanoramaImage) != OK)
    {
        dlog_warn("HVF事件全景图 JPEG 编码失败，事件类型[%d]", static_cast<int>(stEventContext.enEventType));
        stEventContext.stPanoramaImage = EventTvSdkImage_S();
    }

    const bool bTargetOk = encode_hvf_target_image(stCtx.pFrameInfo, stRect, stEventContext.stTargetImage);
    dlog_info("HVF TVSDK上下文填充: event[%d], rule[%d], target[%u], objType[%d], rect[%d,%d,%d,%d], "
              "frame[%ux%u], panorama[%zu], targetImg[%zu], targetOk[%d]",
              static_cast<int>(stEventContext.enEventType),
              nRuleId,
              stObject.track_id,
              stEventContext.nObjectType,
              stEventContext.nLeft,
              stEventContext.nTop,
              stEventContext.nRight,
              stEventContext.nBottom,
              stCtx.pFrameInfo->video_frame.width,
              stCtx.pFrameInfo->video_frame.height,
              stEventContext.stPanoramaImage.vecJpeg.size(),
              stEventContext.stTargetImage.vecJpeg.size(),
              bTargetOk ? 1 : 0);
#else
    (void)stEventContext;
    (void)stCtx;
    (void)pstObjectClass;
    (void)stObject;
    (void)nRuleId;
#endif
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
