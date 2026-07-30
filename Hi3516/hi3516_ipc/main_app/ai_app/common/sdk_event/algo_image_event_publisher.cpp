/**
 * @FilePath     : algo_image_event_publisher.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 19:29:55
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-29 14:22:30
 * @Description  : AI 算法 SDK 图片事件推送公共类实现
 */

#include "algo_image_event_publisher.hpp"

#include <cstring>
#include <chrono>
#include <memory>

#include "control_manage.h"
#include "dlog.h"

#ifdef ENABLE_TVSDK_SRC
#include "NetTVSDKServer.h"
#endif

namespace AiAppCommon
{
namespace
{
long long current_timestamp_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}
} // namespace

bool CAlgoImageEventPublisher::publishImageObject(const SdkImageObjectRequest_S &stRequest) const
{
#ifndef ENABLE_TVSDK_SRC
    (void)stRequest;
    return false;
#else
    if (!hasClient())
    {
        return false;
    }

    if (stRequest.pvecJpeg == nullptr || stRequest.pvecJpeg->empty())
    {
        dlog_warn("SDK 图片事件推送跳过，JPEG 图片为空");
        return false;
    }

    if (stRequest.pvecJpeg->size() > NET_PIC_DATA_MAX_LEN)
    {
        dlog_warn("SDK 图片事件推送跳过，图片大小[%zu]超过协议上限[%u]",
                  stRequest.pvecJpeg->size(),
                  static_cast<unsigned int>(NET_PIC_DATA_MAX_LEN));
        return false;
    }

    /* TVSDK AI_OBJECT 告警结构，只由公共推送层填充协议字段 */
    std::unique_ptr<NET_AlarmAiObjectInfo_S> pInfo(new NET_AlarmAiObjectInfo_S());
    std::memset(pInfo.get(), 0, sizeof(*pInfo));
    pInfo->uAlarmType = stRequest.unAlarmType;
    pInfo->uChannel = static_cast<UINT32>(normalizeChannel(stRequest.nChnId));
    pInfo->uObjectType = stRequest.unObjectType;
    pInfo->fConfidence = stRequest.fConfidence;
    pInfo->nLeft = stRequest.stRect.nX1;
    pInfo->nTop = stRequest.stRect.nY1;
    pInfo->nRight = stRequest.stRect.nX2;
    pInfo->nBottom = stRequest.stRect.nY2;
    pInfo->llTimestampMs = stRequest.llTimestampMs > 0 ? stRequest.llTimestampMs : current_timestamp_ms();
    std::memcpy(pInfo->byImgData, stRequest.pvecJpeg->data(), stRequest.pvecJpeg->size());
    pInfo->uImgLen = static_cast<UINT32>(stRequest.pvecJpeg->size());

    const int nRet = ControlManage::instance()->tvsdk_push_alarm(static_cast<int>(pInfo->uAlarmType),
                                                                 pInfo.get(),
                                                                 sizeof(*pInfo));
    if (nRet != 0)
    {
        dlog_warn("SDK 图片事件推送失败，alarm[%u] ret[%d]", pInfo->uAlarmType, nRet);
        return false;
    }

    dlog_info("SDK 图片事件推送成功，alarm[%u] 通道[%u] 时间戳[%lld] 图片长度[%u] 置信度[%.3f]",
              pInfo->uAlarmType,
              pInfo->uChannel,
              static_cast<long long>(pInfo->llTimestampMs),
              pInfo->uImgLen,
              pInfo->fConfidence);
    return true;
#endif
}
} // namespace AiAppCommon
