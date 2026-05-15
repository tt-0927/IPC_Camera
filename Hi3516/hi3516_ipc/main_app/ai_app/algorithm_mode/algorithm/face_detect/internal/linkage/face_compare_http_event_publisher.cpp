/**
 * @FilePath     : face_compare_http_event_publisher.cpp
 * @Description  : Face compare HTTP event publisher implementation
 */

#include "face_compare_http_event_publisher.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include "face_http_event_poster.hpp"
#include "face_http_push_config.hpp"
#include "share_define.h"
#include "time_utils.h"

namespace
{
/* HTTP 推送对外保留 SDK 告警命令语义，便于平台按 NET_TV_ALARM_* 体系识别事件类型 */
constexpr const char *FACE_COMPARE_ALARM_COMMAND = "NET_TV_ALARM_FACE_COMPARE";
constexpr int FACE_COMPARE_ALARM_CODE = 0x3007;

/**
 * @brief   : 将相似度转换为百分比字符串
 * @param    {float} fSimilarity：相似度，取值通常为 0.0 到 1.0
 * @return   {std::string} 百分比整数字符串
 */
std::string to_percent_string(float fSimilarity)
{
    const float fClamped = std::max(0.0f, std::min(fSimilarity, 1.0f));
    return std::to_string(static_cast<int>(fClamped * 100.0f));
}

/**
 * @brief   : 添加非空二进制图片字段
 * @param    {FaceHttpPostRequest_S} &stRequest：HTTP 推送请求
 * @param    {std::string} &strName：表单字段名
 * @param    {std::string} &strFileName：上传文件名
 * @param    {std::vector<unsigned char>} *pvecData：图片二进制数据
 * @return   {void}
 */
void add_binary(FaceDetectInternal::FaceHttpPostRequest_S &stRequest,
                const std::string &strName,
                const std::string &strFileName,
                const std::vector<unsigned char> *pvecData)
{
    if (pvecData == nullptr || pvecData->empty())
    {
        return;
    }

    FaceDetectInternal::FaceHttpBinaryField_S stField;
    stField.strName = strName;
    stField.strFileName = strFileName;
    stField.vecData = *pvecData;
    stRequest.vecBinaryFields.emplace_back(std::move(stField));
}
} // namespace

namespace FaceDetectInternal
{
/**
 * @brief   : 判断人脸比对 HTTP 推送是否启用
 * @return   {bool} true：启用 false：未启用
 */
bool CFaceCompareHttpEventPublisher::isEnabled() const
{
    return CFaceHttpPushConfig::isCompareEnabled(CFaceHttpPushConfig::load());
}

/**
 * @brief   : 通过 HTTP 推送人脸比对结果
 * @param    {FaceCompareSdkResult_S} &stResult：人脸比对结果
 * @return   {bool} true：已加入 HTTP 推送队列 false：未推送
 */
bool CFaceCompareHttpEventPublisher::publish(const FaceCompareSdkResult_S &stResult) const
{
    const FaceHttpPushConfig_S stConfig = CFaceHttpPushConfig::load();
    if (!CFaceHttpPushConfig::isCompareEnabled(stConfig))
    {
        return false;
    }

    FaceHttpPostRequest_S stRequest;
    stRequest.strUrl = stConfig.strCompareUrl;
    stRequest.strToken = stConfig.strToken;
    stRequest.strEventType = "FACE_COMPARE";
    stRequest.vecFields.emplace_back("EventType", "FACE_COMPARE");
    stRequest.vecFields.emplace_back("Command", FACE_COMPARE_ALARM_COMMAND);
    stRequest.vecFields.emplace_back("AlarmType", FACE_COMPARE_ALARM_COMMAND);
    stRequest.vecFields.emplace_back("AlarmCode", std::to_string(FACE_COMPARE_ALARM_CODE));
    stRequest.vecFields.emplace_back("DeviceCode", DEVICE_CODE);
    stRequest.vecFields.emplace_back("Channel", std::to_string(stResult.nChnId < 0 ? 0 : stResult.nChnId));
    stRequest.vecFields.emplace_back("TimestampMs", std::to_string(TimeUtils_NS::get_currentTimestampMs()));
    stRequest.vecFields.emplace_back("CompareResult", stResult.bSuccess ? "1" : "0");
    stRequest.vecFields.emplace_back("FaceID", std::to_string(stResult.nFaceId));
    stRequest.vecFields.emplace_back("Similarity", std::to_string(stResult.fSimilarity));
    stRequest.vecFields.emplace_back("SimilarityPercent", to_percent_string(stResult.fSimilarity));
    stRequest.vecFields.emplace_back("FaceName", stResult.strFaceName);
    stRequest.vecFields.emplace_back("FaceLibName", stResult.strFaceLibName);
    stRequest.vecFields.emplace_back("LibFacePath", stResult.strLibFacePath);
    stRequest.vecFields.emplace_back("CaptureFacePath", stResult.strCapFacePath);
    stRequest.vecFields.emplace_back("CaptureImagePath", stResult.strCapImagePath);
    stRequest.vecFields.emplace_back("CaptureFaceImgLen",
                                     std::to_string(stResult.pvecCaptureJpeg ? stResult.pvecCaptureJpeg->size() : 0));
    stRequest.vecFields.emplace_back("LibFaceImgLen",
                                     std::to_string(stResult.pvecLibFaceJpeg ? stResult.pvecLibFaceJpeg->size() : 0));

    add_binary(stRequest, "CaptureFaceImage", "face_compare_capture.jpg", stResult.pvecCaptureJpeg);
    add_binary(stRequest, "LibFaceImage", "face_compare_library.jpg", stResult.pvecLibFaceJpeg);

    return CFaceHttpEventPoster::instance().enqueue(std::move(stRequest));
}
} // namespace FaceDetectInternal
