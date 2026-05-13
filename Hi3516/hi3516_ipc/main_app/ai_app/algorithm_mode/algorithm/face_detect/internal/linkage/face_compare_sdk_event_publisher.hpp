/**
 * @FilePath     : face_compare_sdk_event_publisher.hpp
 * @Description  : 人脸比对 SDK 事件推送器
 */

#pragma once

#include <string>
#include <vector>

#include "algo_sdk_event_publisher_base.hpp"

namespace FaceDetectInternal
{
/* 人脸比对 SDK 图片推送请求，图片数据由调用方保证本次同步推送期间有效 */
struct FaceCompareSdkResult_S
{
    bool bSuccess = false;
    int nFaceId = -1;
    float fSimilarity = 0.0f;
    int nChnId = 0;
    std::string strFaceName;
    std::string strFaceLibName;
    std::string strLibFacePath;
    std::string strCapFacePath;
    std::string strCapImagePath;
    const std::vector<unsigned char> *pvecCaptureJpeg = nullptr;
    const std::vector<unsigned char> *pvecLibFaceJpeg = nullptr;
};

class CFaceCompareSdkEventPublisher : public AiAppCommon::CAlgoSdkEventPublisherBase
{
public:
    /**
     * @brief   : 推送人脸比对 SDK 图片事件
     * @param    {FaceCompareSdkResult_S} &stResult：人脸比对结果
     * @return   {bool} true：推送成功 false：未推送或推送失败
     */
    bool publish(const FaceCompareSdkResult_S &stResult) const;
};
} // namespace FaceDetectInternal
