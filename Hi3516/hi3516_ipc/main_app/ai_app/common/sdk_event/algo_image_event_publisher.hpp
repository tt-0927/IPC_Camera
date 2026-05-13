/**
 * @FilePath     : algo_image_event_publisher.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 19:29:55
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 20:21:38
 * @Description  : AI 算法 SDK 图片事件推送公共类
 */

#pragma once

#include <vector>

#include "algo_sdk_event_publisher_base.hpp"
#include "common_process.h"

namespace AiAppCommon
{
/* SDK AI_OBJECT 图片推送请求，业务侧只填通用目标字段和 JPEG 数据 */
struct SdkImageObjectRequest_S
{
    /* TVSDK 告警类型 */
    unsigned int unAlarmType = 0;
    /* TVSDK 目标类型 */
    unsigned int unObjectType = 0;
    /* 通道号 */
    int nChnId = 0;
    /* 目标框坐标 */
    Common::RectInfo_S stRect;
    /* 目标置信度 */
    float fConfidence = 0.0f;
    /* JPEG 图片二进制数据指针，由调用方保证本次同步推送期间有效 */
    const std::vector<unsigned char> *pvecJpeg = nullptr;
};

class CAlgoImageEventPublisher : public CAlgoSdkEventPublisherBase
{
protected:
    /**
     * @brief   : 推送 AI_OBJECT 图片事件到 TVSDK
     * @param    {SdkImageObjectRequest_S} &stRequest：图片事件推送请求
     * @return   {bool} true：推送成功 false：未推送或推送失败
     */
    bool publishImageObject(const SdkImageObjectRequest_S &stRequest) const;
};
} // namespace AiAppCommon
