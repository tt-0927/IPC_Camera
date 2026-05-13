/**
 * @FilePath     : face_compare_http_event_publisher.hpp
 * @Description  : Face compare HTTP event publisher
 */

#pragma once

#include "face_compare_sdk_event_publisher.hpp"

namespace FaceDetectInternal
{
/**
 * @brief   : 人脸比对 HTTP 事件推送器
 */
class CFaceCompareHttpEventPublisher
{
public:
    /**
     * @brief   : 判断人脸比对 HTTP 推送是否启用
     * @return   {bool} true：启用 false：未启用
     */
    bool isEnabled() const;

    /**
     * @brief   : 通过 HTTP 推送人脸比对结果
     * @param    {FaceCompareSdkResult_S} &stResult：人脸比对结果
     * @return   {bool} true：已加入 HTTP 推送队列 false：未推送
     */
    bool publish(const FaceCompareSdkResult_S &stResult) const;
};
} // namespace FaceDetectInternal
