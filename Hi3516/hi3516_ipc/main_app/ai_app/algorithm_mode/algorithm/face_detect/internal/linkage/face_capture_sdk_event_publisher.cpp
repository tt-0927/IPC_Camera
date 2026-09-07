/**
 * @FilePath     : face_capture_sdk_event_publisher.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 19:30:16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 20:18:21
 * @Description  : 人脸抓拍 SDK 事件推送器实现
 */

#include "face_capture_sdk_event_publisher.hpp"

#ifdef ENABLE_TVSDK_SRC
#include "NetTVSDKServer.h"
#endif

namespace
{
/* TVSDK 约定的人脸目标类型，当前协议只显式标注人/车，3 预留为人脸目标 */
constexpr unsigned int FACE_CAPTURE_TVSDK_OBJECT_TYPE_FACE = 3;

/**
 * @brief   : 获取人脸抓拍 TVSDK 告警类型
 * @return   {unsigned int} TVSDK 告警类型，未启用 TVSDK 时返回 0
 */
unsigned int get_face_capture_alarm_type()
{
#ifndef ENABLE_TVSDK_SRC
    return 0;
#else
    return NET_ALARM_FACE_CAPTURE;
#endif
}
} // namespace

namespace FaceDetectInternal
{
void CFaceCaptureSdkEventPublisher::publish(const std::vector<FaceCaptureTarget_S> &vecTargets,
                                            ot_video_frame_info *pFrameInfo,
                                            int nChnId,
                                            const BuildPanoramaImageFunc &fnBuildPanoramaImage,
                                            const BuildTargetImageFunc &fnBuildTargetImage)
{
    if (pFrameInfo == nullptr || vecTargets.empty())
    {
        return;
    }

    if (!hasClient())
    {
        /* 无客户端时不编码图片，但仍标记首帧已处理，避免持续报警阶段补发全景图 */
        setEventActive(nChnId, true);
        return;
    }

    /* 当前待推送的 SDK JPEG 图片二进制数据，复用于全景图和每张目标小图 */
    std::vector<unsigned char> vecSdkJpeg;
    if (!isEventActive(nChnId))
    {
        if (fnBuildPanoramaImage && fnBuildPanoramaImage(pFrameInfo, vecSdkJpeg))
        {
            /* 全景图按 640x384 整帧范围填充目标信息，供客户端识别为事件首张大图 */
            FaceCaptureTarget_S stPanoramaTarget;
            stPanoramaTarget.stRect.nX1 = 0;
            stPanoramaTarget.stRect.nY1 = 0;
            stPanoramaTarget.stRect.nX2 = PIXEL_WIDTH_640;
            stPanoramaTarget.stRect.nY2 = PIXEL_HEIGHT_384;
            stPanoramaTarget.fConfidence = 1.0f;
            stPanoramaTarget.nIpd = 0;
            publishFaceImage(stPanoramaTarget, vecSdkJpeg, nChnId);
        }
        setEventActive(nChnId, true);
    }

    for (size_t i = 0; i < vecTargets.size(); ++i)
    {
        vecSdkJpeg.clear();
        if (!fnBuildTargetImage || !fnBuildTargetImage(vecTargets[i].stRect, pFrameInfo, i + 1, vecSdkJpeg))
        {
            continue;
        }
        publishFaceImage(vecTargets[i], vecSdkJpeg, nChnId);
    }
}

bool CFaceCaptureSdkEventPublisher::publishFaceImage(const FaceCaptureTarget_S &stTarget,
                                                     const std::vector<unsigned char> &vecJpeg,
                                                     int nChnId) const
{
    AiAppCommon::SdkImageObjectRequest_S stRequest;
    stRequest.unAlarmType = get_face_capture_alarm_type();
    stRequest.unObjectType = FACE_CAPTURE_TVSDK_OBJECT_TYPE_FACE;
    stRequest.nChnId = nChnId;
    stRequest.stRect = stTarget.stRect;
    stRequest.fConfidence = stTarget.fConfidence;
    stRequest.pvecJpeg = &vecJpeg;
    return publishImageObject(stRequest);
}
} // namespace FaceDetectInternal
