/**
 * @FilePath     : face_compare_sdk_event_publisher.cpp
 * @Description  : 人脸比对 SDK 事件推送器实现
 */

#include "face_compare_sdk_event_publisher.hpp"

#include <algorithm>
#include <cstring>

#include "control_manage.h"
#include "dlog.h"
#include "time_utils.h"

#ifdef ENABLE_TVSDK_SRC
#include "NetTVSDKServer.h"
#endif

namespace
{
/**
 * @brief   : 获取人脸比对 TVSDK 告警类型
 * @return   {unsigned int} TVSDK 告警类型，未启用 TVSDK 时返回 0
 */
unsigned int get_face_compare_alarm_type()
{
#ifndef ENABLE_TVSDK_SRC
    return 0;
#else
    return NET_TV_ALARM_FACE_COMPARE;
#endif
}

template <size_t N>
void copy_string(char (&dst)[N], const std::string &src)
{
    std::memset(dst, 0, N);
    if (src.empty())
    {
        return;
    }

    const size_t nCopyLen = std::min(src.size(), N - 1);
    std::memcpy(dst, src.data(), nCopyLen);
}

template <size_t N>
unsigned int copy_image(unsigned char (&dst)[N], const std::vector<unsigned char> *pvecImage)
{
    if (pvecImage == nullptr || pvecImage->empty())
    {
        return 0;
    }

    const size_t nCopyLen = std::min(pvecImage->size(), N);
    std::memcpy(dst, pvecImage->data(), nCopyLen);
    return static_cast<unsigned int>(nCopyLen);
}
} // namespace

namespace FaceDetectInternal
{
bool CFaceCompareSdkEventPublisher::publish(const FaceCompareSdkResult_S &stResult) const
{
#ifndef ENABLE_TVSDK_SRC
    (void)stResult;
    return false;
#else
    if (!hasClient())
    {
        return false;
    }

    NET_TV_ALARM_FACE_COMPARE_INFO_S stInfo;
    std::memset(&stInfo, 0, sizeof(stInfo));
    stInfo.dwAlarmType = get_face_compare_alarm_type();
    stInfo.dwChannel = static_cast<UINT32>(normalizeChannel(stResult.nChnId));
    stInfo.llTimestampMs = TimeUtils_NS::get_currentTimestampMs();
    stInfo.nEventId = 0;
    stInfo.nCompResult = stResult.bSuccess ? 1 : 0;
    stInfo.nFaceId = stResult.nFaceId;
    stInfo.nSimilarity = static_cast<INT32>(std::max(0.0f, std::min(stResult.fSimilarity, 1.0f)) * 100.0f);
    copy_string(stInfo.szFaceName, stResult.strFaceName);
    copy_string(stInfo.szFaceLibName, stResult.strFaceLibName);
    copy_string(stInfo.szLibFacePath, stResult.strLibFacePath);
    copy_string(stInfo.szCapFacePath, stResult.strCapFacePath);
    copy_string(stInfo.szCapImagePath, stResult.strCapImagePath);
    stInfo.dwLibFaceImgLen = copy_image(stInfo.byLibFaceImg, stResult.pvecLibFaceJpeg);
    stInfo.dwCapFaceImgLen = copy_image(stInfo.byCapFaceImg, stResult.pvecCaptureJpeg);

    const int nRet = ControlManage::instance()->tvsdk_push_alarm(static_cast<int>(stInfo.dwAlarmType),
                                                                 &stInfo,
                                                                 sizeof(stInfo));
    if (nRet != 0)
    {
        dlog_warn("人脸比对 SDK 事件推送失败，ret[%d] id[%d] similarity[%.3f]",
                  nRet,
                  stInfo.nFaceId,
                  stResult.fSimilarity);
        return false;
    }

    dlog_info("人脸比对 SDK 事件推送成功，通道[%u] result[%d] id[%d] name[%s] lib[%s] similarity[%d] capLen[%u] libLen[%u]",
              stInfo.dwChannel,
              stInfo.nCompResult,
              stInfo.nFaceId,
              stInfo.szFaceName,
              stInfo.szFaceLibName,
              stInfo.nSimilarity,
              stInfo.dwCapFaceImgLen,
              stInfo.dwLibFaceImgLen);
    return true;
#endif
}
} // namespace FaceDetectInternal
