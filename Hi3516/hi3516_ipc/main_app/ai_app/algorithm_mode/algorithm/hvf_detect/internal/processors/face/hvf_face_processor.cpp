/**
 * @FilePath     : hvf_face_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-26 16:20:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-28 10:43:02
 * @Description  : HVF 人脸侦测处理器实现
 */

#include "hvf_face_processor.hpp"

#include "internal/base/hvf_detect_common.hpp"

namespace
{
/**
 * @brief   : 计算人脸侦测实际使用的置信度门限
 * @param    {const Alarm::FaceDetection_S &} stAlgoCfg：人脸侦测配置
 * @return   {float} 置信度门限
 */
float get_face_sensitivity_threshold(const Alarm::FaceDetection_S &stAlgoCfg)
{
    return 1.0f - stAlgoCfg.nSensitivity / 100.0f;
}
} // namespace

namespace HVFDetectInternal
{
void CHVFFaceProcessor::setEnabled(bool bEnable)
{
    m_stFaceDetCfg.bEnable = bEnable;
}

void CHVFFaceProcessor::setAlgoParamCfg(const Alarm::FaceDetection_S &stAlgoCfg, int nWidth, int nHeight)
{
    dlog_debug("ai_app: 设置人脸侦测参数");
    m_stFaceDetCfg = stAlgoCfg;
    m_stFaceDetCfg.stRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, nWidth, nHeight);
}

void CHVFFaceProcessor::process(SHVFProcessContext &stContext)
{
    /* 当前帧是否满足人脸报警条件 */
    bool bIsAlarm = false;
    /* 人脸侦测单独使用的角框数组 */
    std::vector<Common::RectInfo_S> vstRectInfo;
    /* 当前帧人脸类别算法结果 */
    const ot_aidetect_object_of_one_class *pstObjectClass = find_object_class(stContext.stResult, OT_AIDETECT_CLASS_FACE);

    if (pstObjectClass)
    {
        /* 当前人脸规则换算后的实际置信度门限 */
        const float fSensitivityThreshold = get_face_sensitivity_threshold(m_stFaceDetCfg);
        for (size_t i = 0; i < pstObjectClass->object_num; ++i)
        {
            /* 当前遍历到的人脸目标 */
            const ot_aidetect_object &stObject = pstObjectClass->objects[i];
            if (stObject.detect_confidence < fSensitivityThreshold)
            {
                continue;
            }

            if (!is_in_region(m_stFaceDetCfg.stRegion, stObject))
            {
                continue;
            }

            if (m_stFaceDetCfg.bDynamicAnalysisEnable)
            {
                add_result_to_vector(stObject, vstRectInfo);
            }
            bIsAlarm = true;
        }
    }

    if (bIsAlarm && m_stFaceDetCfg.bDynamicAnalysisEnable)
    {
        send_detectionResult_to_osd(stContext.nWidth, stContext.nHeight, vstRectInfo);
    }

    EventTriggerContext_S stEventContext;
    stEventContext.enEventType = Event::Type_E::FACE_DETECT;
    stEventContext.nChnId = stContext.nChnId;
    stEventContext.llTimestamp = stContext.llTimestamp;
#ifdef ENABLE_TVSDK_SRC
    if (bIsAlarm && stContext.pFrameInfo != nullptr)
    {
        auto pPayload = std::make_shared<EventTvSdkPayload_S>();
        pPayload->enType = get_tvsdk_payload_type(stEventContext.enEventType);
        if (AiAppCommon::encode_video_frame_to_jpeg_memory(stContext.pFrameInfo, pPayload->stPanoramaImage) == OK)
        {
            stEventContext.pTvSdkPayload = pPayload;
        }
    }
#endif
    m_alarmStateMachine.handleAlarmState(bIsAlarm, stEventContext);
}

bool CHVFFaceProcessor::isEnabled() const
{
    return m_stFaceDetCfg.bEnable;
}
} // namespace HVFDetectInternal
