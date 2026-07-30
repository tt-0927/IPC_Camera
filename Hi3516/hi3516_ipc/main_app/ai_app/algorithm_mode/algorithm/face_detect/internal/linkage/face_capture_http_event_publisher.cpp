/**
 * @FilePath     : face_capture_http_event_publisher.cpp
 * @Description  : Face capture HTTP event publisher implementation
 */

#include "face_capture_http_event_publisher.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include "face_http_event_poster.hpp"
#include "face_http_push_config.hpp"
#include "share_define.h"
#include "time_utils.h"

namespace
{
/* HTTP 推送对外保留 SDK 告警命令语义，便于平台按 NET_ALARM_* 体系识别事件类型 */
constexpr const char *FACE_CAPTURE_ALARM_COMMAND = "NET_ALARM_FACE_CAPTURE";
constexpr int FACE_CAPTURE_ALARM_CODE = 0x3002;

/**
 * @brief   : 整型转换为字符串
 * @param    {int} nValue：整型值
 * @return   {std::string} 字符串结果
 */
std::string to_string_int(int nValue)
{
    return std::to_string(nValue);
}

/**
 * @brief   : 浮点数转换为字符串
 * @param    {float} fValue：浮点值
 * @return   {std::string} 字符串结果
 */
std::string to_string_float(float fValue)
{
    return std::to_string(fValue);
}

/**
 * @brief   : 添加非空二进制图片字段
 * @param    {FaceHttpPostRequest_S} &stRequest：HTTP 推送请求
 * @param    {std::string} &strName：表单字段名
 * @param    {std::string} &strFileName：上传文件名
 * @param    {std::vector<unsigned char>} &vecData：图片二进制数据
 * @return   {void}
 */
void add_binary(FaceDetectInternal::FaceHttpPostRequest_S &stRequest,
                const std::string &strName,
                const std::string &strFileName,
                const std::vector<unsigned char> &vecData)
{
    if (vecData.empty())
    {
        return;
    }

    FaceDetectInternal::FaceHttpBinaryField_S stField;
    stField.strName = strName;
    stField.strFileName = strFileName;
    stField.vecData = vecData;
    stRequest.vecBinaryFields.emplace_back(std::move(stField));
}
} // namespace

namespace FaceDetectInternal
{
/**
 * @brief   : 判断人脸抓拍 HTTP 推送是否启用
 * @return   {bool} true：启用 false：未启用
 */
bool CFaceCaptureHttpEventPublisher::isEnabled() const
{
    return CFaceHttpPushConfig::isCaptureEnabled(CFaceHttpPushConfig::load());
}

/**
 * @brief   : 通过 HTTP 推送人脸抓拍结果
 * @param    {std::vector<FaceCaptureTarget_S>} &vecTargets：抓拍目标列表
 * @param    {ot_video_frame_info} *pFrameInfo：当前视频帧
 * @param    {int} nChnId：通道号
 * @param    {BuildPanoramaImageFunc} &fnBuildPanoramaImage：全景图构建回调
 * @param    {BuildTargetImageFunc} &fnBuildTargetImage：目标小图构建回调
 * @return   {bool} true：已加入 HTTP 推送队列 false：未推送
 */
bool CFaceCaptureHttpEventPublisher::publish(const std::vector<FaceCaptureTarget_S> &vecTargets,
                                             ot_video_frame_info *pFrameInfo,
                                             int nChnId,
                                             const BuildPanoramaImageFunc &fnBuildPanoramaImage,
                                             const BuildTargetImageFunc &fnBuildTargetImage) const
{
    const FaceHttpPushConfig_S stConfig = CFaceHttpPushConfig::load();
    if (!CFaceHttpPushConfig::isCaptureEnabled(stConfig) || pFrameInfo == nullptr || vecTargets.empty())
    {
        return false;
    }

    FaceHttpPostRequest_S stRequest;
    stRequest.strUrl = stConfig.strCaptureUrl;
    stRequest.strToken = stConfig.strToken;
    stRequest.strEventType = "FACE_CAPTURE";
    stRequest.vecFields.emplace_back("EventType", "FACE_CAPTURE");
    stRequest.vecFields.emplace_back("Command", FACE_CAPTURE_ALARM_COMMAND);
    stRequest.vecFields.emplace_back("AlarmType", FACE_CAPTURE_ALARM_COMMAND);
    stRequest.vecFields.emplace_back("AlarmCode", to_string_int(FACE_CAPTURE_ALARM_CODE));
    stRequest.vecFields.emplace_back("DeviceCode", DEVICE_CODE);
    stRequest.vecFields.emplace_back("Channel", to_string_int(nChnId < 0 ? 0 : nChnId));
    stRequest.vecFields.emplace_back("TimestampMs", std::to_string(TimeUtils_NS::get_currentTimestampMs()));
    stRequest.vecFields.emplace_back("TargetCount", to_string_int(static_cast<int>(vecTargets.size())));

    std::vector<unsigned char> vecPanoramaJpeg;
    if (fnBuildPanoramaImage && fnBuildPanoramaImage(pFrameInfo, vecPanoramaJpeg))
    {
        add_binary(stRequest, "PanoramaImage", "face_capture_panorama.jpg", vecPanoramaJpeg);
    }

    for (size_t i = 0; i < vecTargets.size(); ++i)
    {
        const FaceCaptureTarget_S &stTarget = vecTargets[i];
        const std::string strPrefix = "Targets[" + std::to_string(i) + "].";
        stRequest.vecFields.emplace_back(strPrefix + "Left", to_string_int(stTarget.stRect.nX1));
        stRequest.vecFields.emplace_back(strPrefix + "Top", to_string_int(stTarget.stRect.nY1));
        stRequest.vecFields.emplace_back(strPrefix + "Right", to_string_int(stTarget.stRect.nX2));
        stRequest.vecFields.emplace_back(strPrefix + "Bottom", to_string_int(stTarget.stRect.nY2));
        stRequest.vecFields.emplace_back(strPrefix + "Confidence", to_string_float(stTarget.fConfidence));
        stRequest.vecFields.emplace_back(strPrefix + "Ipd", to_string_int(stTarget.nIpd));

        std::vector<unsigned char> vecTargetJpeg;
        if (fnBuildTargetImage && fnBuildTargetImage(stTarget.stRect, pFrameInfo, i + 1, vecTargetJpeg))
        {
            add_binary(stRequest,
                       "TargetImages[" + std::to_string(i) + "]",
                       "face_capture_target_" + std::to_string(i + 1) + ".jpg",
                       vecTargetJpeg);
        }
    }

    return CFaceHttpEventPoster::instance().enqueue(std::move(stRequest));
}
} // namespace FaceDetectInternal
