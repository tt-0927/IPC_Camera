/**
 * @FilePath     : face_capture_sdk_event_publisher.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 19:30:16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 20:18:21
 * @Description  : 人脸抓拍 SDK 事件推送器实现
 */

#include "face_capture_sdk_event_publisher.hpp"

#include <algorithm>
#include <utility>

#ifdef ENABLE_TVSDK_SRC
#include "NetTVSDKServer.h"
#endif

namespace
{
/* 保持原 SDK 约定：3 表示人脸目标。 */
constexpr unsigned int FACE_CAPTURE_TVSDK_OBJECT_TYPE_FACE = 3;
constexpr size_t FACE_CAPTURE_MAX_TARGETS = 8;

/**
 * @brief   : 获取人脸抓拍统一 TVSDK 告警类型
 * @return   {unsigned int} TVSDK 告警类型，未启用 TVSDK 时返回 0
 */
unsigned int get_face_capture_alarm_type()
{
#ifndef ENABLE_TVSDK_SRC
    return 0;
#else
    /* NET_AlarmCaptureInfo_S 必须配套 0x6100 抓拍事件族，不能复用旧 AI_OBJECT 的 0x3002。 */
    return NET_ALARM_CAPTURE_FACE;
#endif
}
} // namespace

namespace FaceDetectInternal
{
void CFaceCaptureSdkEventPublisher::publish(const std::vector<FaceCaptureTarget_S> &vecTargets,
                                            ot_video_frame_info *pFrameInfo,
                                            int nChnId,
                                            long long llTimestampMs,
                                            const BuildPanoramaImageFunc &fnBuildPanoramaImage,
                                            const BuildTargetImageFunc &fnBuildTargetImage)
{
    if (pFrameInfo == nullptr || vecTargets.empty())
    {
        return;
    }

#ifndef ENABLE_TVSDK_SRC
    (void)nChnId;
    (void)llTimestampMs;
    (void)fnBuildPanoramaImage;
    (void)fnBuildTargetImage;
    return;
#else

    if (!hasClient())
    {
        /* 无客户端时不编码图片，但仍标记首帧已处理，避免持续报警阶段补发全景图 */
        setEventActive(nChnId, true);
        return;
    }

    /*
     * 原实现将全景图和每个目标拆成多条 AI_OBJECT 告警，客户端无法判断它们属于同一次抓拍。
     * 统一抓拍协议要求在同一条告警中打包全景图及全部目标小图，因此只在告警首帧发布一次。
     */
    if (isEventActive(nChnId))
    {
        return;
    }

    std::vector<unsigned char> vecPanoramaJpeg;
    if (fnBuildPanoramaImage)
    {
        fnBuildPanoramaImage(pFrameInfo, vecPanoramaJpeg);
    }

    const size_t uTargetCount = std::min(vecTargets.size(), FACE_CAPTURE_MAX_TARGETS);
    std::vector<std::vector<unsigned char>> vecCropJpegs;
    std::vector<AiAppCommon::SdkCaptureTargetRequest_S> vecCaptureTargets;
    vecCropJpegs.reserve(uTargetCount);
    vecCaptureTargets.reserve(uTargetCount);
    for (size_t i = 0; i < uTargetCount; ++i)
    {
        vecCropJpegs.emplace_back();
        if (!fnBuildTargetImage || !fnBuildTargetImage(vecTargets[i].stRect,
                                                        pFrameInfo,
                                                        i + 1,
                                                        vecCropJpegs.back()))
        {
            vecCropJpegs.pop_back();
            continue;
        }

        AiAppCommon::SdkCaptureTargetRequest_S stTargetRequest;
        stTargetRequest.stRect = vecTargets[i].stRect;
        stTargetRequest.unObjectType = FACE_CAPTURE_TVSDK_OBJECT_TYPE_FACE;
        stTargetRequest.fConfidence = vecTargets[i].fConfidence;
        /* 当前算法只提供瞳距，不把它误当作跨帧跟踪 ID。 */
        stTargetRequest.nTrackId = -1;
        stTargetRequest.pvecJpeg = &vecCropJpegs.back();
        vecCaptureTargets.emplace_back(stTargetRequest);
    }

    AiAppCommon::SdkCaptureRequest_S stRequest;
    stRequest.unAlarmType = get_face_capture_alarm_type();
    stRequest.unCaptureType = NET_CAPTURE_TYPE_FACE;
    stRequest.nChnId = nChnId;
    stRequest.unPanoramaWidth = pFrameInfo->video_frame.width;
    stRequest.unPanoramaHeight = pFrameInfo->video_frame.height;
    stRequest.llTimestampMs = llTimestampMs;
    stRequest.pvecPanoramaJpeg = &vecPanoramaJpeg;
    stRequest.vecTargets = std::move(vecCaptureTargets);
    if (publishCapture(stRequest))
    {
        setEventActive(nChnId, true);
    }
#endif
}
} // namespace FaceDetectInternal
