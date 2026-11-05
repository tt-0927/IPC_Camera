/*
 * @Author: zhengxh
 * @Date: 2026-11-02
 * @Description: 社交关系分析事件子检测器实现
 */

#include "social_relationship_sub_detector.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#include "common_process.h"
#include "control_manage.h"
#include "dlog.h"
#include "event_configure.h"
#include "face_recognition_define.hpp"
#include "share_data.h"
#include "time_utils.h"

void CSocialRelationshipSubDetector::updateConfig()
{
    Alarm::SocialRelationshipAnalysis_S stConfig;
    CEventConfigure::instance()->get_configure(stConfig);

    m_processor.setRules(stConfig);

    {
        std::lock_guard<std::mutex> lock(m_mutexConfig);
        m_bEnable = stConfig.bEnable;
    }
}

#ifdef ENABLE_TVSDK_SRC
/* 人脸小图批量直推 单结构承载多张人脸（对照 processTvSdkCaptureResult 组包） */
static void pushFaceBatchToTvSdk(const DetectContext_S &stDetectContext,
                                 const std::vector<int> &vstCandidateIndices,
                                 const cv::Mat &rgbMat)
{
    /* 大图整帧仅编码一次 局部容器存活至推送返回（SDK 返回前完成序列化） */
    EventTvSdkImage_S stPanoramaImage{};
    if (!encode_mat_to_tvsdk_image(rgbMat, stPanoramaImage, JPEG_QUALITY_PANORAMA) ||
        stPanoramaImage.vecJpeg.empty() ||
        stPanoramaImage.vecJpeg.size() > NET_PIC_DATA_MAX_LEN)
    {
        dlog_warn("TVSDK批量人脸推送大图编码失败或超上限");
        return;
    }

    NET_AlarmCaptureInfo_S stInfo{};
    stInfo.uAlarmType = NET_ALARM_SOCIAL_RELATIONSHIP;
    stInfo.uCaptureType = NET_CAPTURE_TYPE_FACE;
    stInfo.uChannel = static_cast<UINT32>(std::max(0, stDetectContext.stFrame.nChannelId));
    stInfo.llTimestampMs = stDetectContext.stFrame.llTimestamp;
    stInfo.uPanoramaWidth = static_cast<UINT32>(rgbMat.cols);
    stInfo.uPanoramaHeight = static_cast<UINT32>(rgbMat.rows);
    stInfo.stPanoramaImg.pData = stPanoramaImage.vecJpeg.data();
    stInfo.stPanoramaImg.uDataLen = static_cast<UINT32>(stPanoramaImage.vecJpeg.size());
    stInfo.stExtraInfo.nFaceExpression = -1; /* 未启用表情分析 不伪造结果 */
    const std::string strTimestamp = TimeUtils_NS::get_currentDateAndTimeNoT();
    std::strncpy(stInfo.stExtraInfo.strTimestamp, strTimestamp.c_str(),
                 sizeof(stInfo.stExtraInfo.strTimestamp) - 1);

    /* 预筛有效人脸框 候选超过上限时取置信度最高的前 NET_CAPTURE_CROP_MAX_NUM 个 */
    struct FaceCandidate_S
    {
        int nX1;
        int nY1;
        int nX2;
        int nY2;
        float fConfidence;
    };
    std::vector<FaceCandidate_S> vstCandidates;
    vstCandidates.reserve(vstCandidateIndices.size());
    for (const int nIndex : vstCandidateIndices)
    {
        const FaceDetectedTarget_S &stFace =
            stDetectContext.vstDetectResult[nIndex].stDetectResult;
        const int nX1 = std::max(0, static_cast<int>(stFace.fX1));
        const int nY1 = std::max(0, static_cast<int>(stFace.fY1));
        const int nX2 = std::min(rgbMat.cols, static_cast<int>(stFace.fX2));
        const int nY2 = std::min(rgbMat.rows, static_cast<int>(stFace.fY2));
        if (nX2 <= nX1 || nY2 <= nY1)
        {
            continue;
        }
        vstCandidates.push_back({nX1, nY1, nX2, nY2, stFace.fConfidence});
    }
    if (vstCandidates.size() > NET_CAPTURE_CROP_MAX_NUM)
    {
        /* 稳定排序 同置信度保持原检测顺序 */
        std::stable_sort(vstCandidates.begin(), vstCandidates.end(),
                         [](const FaceCandidate_S &stA, const FaceCandidate_S &stB) {
                             return stA.fConfidence > stB.fConfidence;
                         });
        vstCandidates.resize(NET_CAPTURE_CROP_MAX_NUM);
    }

    /* 小图逐张编码挂指针 预留容量避免扩容搬移 编码失败仅丢弃该张不影响其余上报 */
    std::vector<EventTvSdkImage_S> vstTargetImages;
    vstTargetImages.reserve(vstCandidates.size());
    for (const FaceCandidate_S &stCandidate : vstCandidates)
    {
        cv::Mat targetMat = rgbMat(cv::Rect(stCandidate.nX1, stCandidate.nY1,
                                            stCandidate.nX2 - stCandidate.nX1,
                                            stCandidate.nY2 - stCandidate.nY1)).clone();
        EventTvSdkImage_S &stTargetImage = vstTargetImages.emplace_back();
        if (!encode_mat_to_tvsdk_image(targetMat, stTargetImage, JPEG_QUALITY_TARGET) ||
            stTargetImage.vecJpeg.empty() ||
            stTargetImage.vecJpeg.size() > NET_PIC_DATA_MAX_LEN)
        {
            vstTargetImages.pop_back();
            continue;
        }

        NET_CropImage_S &stCropImage = stInfo.stCropImages[stInfo.uCropCount++];
        stCropImage.uCropX = static_cast<UINT32>(stCandidate.nX1);
        stCropImage.uCropY = static_cast<UINT32>(stCandidate.nY1);
        stCropImage.uCropWidth = static_cast<UINT32>(stCandidate.nX2 - stCandidate.nX1);
        stCropImage.uCropHeight = static_cast<UINT32>(stCandidate.nY2 - stCandidate.nY1);
        stCropImage.uTargetType = NET_CAPTURE_TYPE_FACE;
        stCropImage.fConfidence = std::isfinite(stCandidate.fConfidence)
                                      ? std::clamp(stCandidate.fConfidence, 0.0F, 1.0F)
                                      : 0.0F;
        stCropImage.nTrackID = -1;
        stCropImage.stImage.pData = stTargetImage.vecJpeg.data();
        stCropImage.stImage.uDataLen = static_cast<UINT32>(stTargetImage.vecJpeg.size());
    }

    if (stInfo.uCropCount == 0)
    {
        return;
    }

    const int nRet = ControlManage::instance()->tvsdk_push_alarm(
        NET_ALARM_CAPTURE_SOCIAL, &stInfo, static_cast<int>(sizeof(stInfo)));
    if (nRet < 0)
    {
        dlog_warn("TVSDK批量人脸推送失败: ret[%d] count[%u]", nRet, stInfo.uCropCount);
    }
    else
    {
        dlog_info("TVSDK批量人脸推送成功: cmd[0x%x] channel[%u] count[%u] "
                  "time[%lld] panorama[%u]",
                  NET_ALARM_CAPTURE_SOCIAL, stInfo.uChannel, stInfo.uCropCount,
                  static_cast<long long>(stInfo.llTimestampMs),
                  stInfo.stPanoramaImg.uDataLen);
    }
}
#endif

int CSocialRelationshipSubDetector::process(const DetectContext_S &stDetectContext)
{
    {
        std::lock_guard<std::mutex> lock(m_mutexConfig);
        if (!m_bEnable)
        {
            return 0;
        }
    }

    /* 灵敏度门槛过滤，候选非空即触发 */
    std::vector<int> vstCandidateIndices;
    m_processor.process(stDetectContext.vstDetectResult, vstCandidateIndices);

#ifdef ENABLE_TVSDK_SRC
    /* 人脸大小图批量直推 状态机会过滤持续中的重复上报 需绕开确保每张都送达 */
    const bool bAlarm = !vstCandidateIndices.empty();
    const cv::Mat &rgbMat = stDetectContext.stFrame.matPanoramaImage;
    if (bAlarm && !rgbMat.empty())
    {
        pushFaceBatchToTvSdk(stDetectContext, vstCandidateIndices, rgbMat);
    }
#endif
    /* 状态机仅驱动事件生命周期 图像信息全部由批量直推承载 */
    m_stateMachine.handleAlarmState(!vstCandidateIndices.empty(),
                                    Event::Type_E::SOCIAL_RELATIONSHIP_ANALYSIS);
    return 0;
}

bool CSocialRelationshipSubDetector::isEventEnabled() const
{
    std::lock_guard<std::mutex> lock(m_mutexConfig);
    return m_bEnable;
}

void CSocialRelationshipSubDetector::reset()
{
    std::lock_guard<std::mutex> lock(m_mutexConfig);
    m_bEnable = false;
}
