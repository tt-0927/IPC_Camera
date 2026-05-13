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
#include <memory>

#include "control_manage.h"
#include "dlog.h"

#ifdef ENABLE_TVSDK_SRC
#include "NetTVSDKServer.h"
#endif

namespace AiAppCommon
{
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

    if (stRequest.pvecJpeg->size() > NET_TV_PIC_DATA_MAX_LEN)
    {
        dlog_warn("SDK 图片事件推送跳过，图片大小[%zu]超过协议上限[%u]",
                  stRequest.pvecJpeg->size(),
                  static_cast<unsigned int>(NET_TV_PIC_DATA_MAX_LEN));
        return false;
    }

    /* TVSDK AI_OBJECT 告警结构，只由公共推送层填充协议字段 */
    std::unique_ptr<NET_TV_ALARM_AI_OBJECT_INFO_S> pInfo(new NET_TV_ALARM_AI_OBJECT_INFO_S());
    std::memset(pInfo.get(), 0, sizeof(*pInfo));
    pInfo->dwAlarmType = stRequest.unAlarmType;
    pInfo->dwChannel = static_cast<UINT32>(normalizeChannel(stRequest.nChnId));
    pInfo->dwObjectType = stRequest.unObjectType;
    pInfo->fConfidence = stRequest.fConfidence;
    pInfo->nLeft = stRequest.stRect.nX1;
    pInfo->nTop = stRequest.stRect.nY1;
    pInfo->nRight = stRequest.stRect.nX2;
    pInfo->nBottom = stRequest.stRect.nY2;
    std::memcpy(pInfo->byImgData, stRequest.pvecJpeg->data(), stRequest.pvecJpeg->size());
    pInfo->dwImgLen = static_cast<UINT32>(stRequest.pvecJpeg->size());

    const int nRet = ControlManage::instance()->tvsdk_push_alarm(static_cast<int>(pInfo->dwAlarmType),
                                                                 pInfo.get(),
                                                                 sizeof(*pInfo));
    if (nRet != 0)
    {
        dlog_warn("SDK 图片事件推送失败，alarm[%u] ret[%d]", pInfo->dwAlarmType, nRet);
        return false;
    }

    dlog_info("SDK 图片事件推送成功，alarm[%u] 通道[%u] 图片长度[%u] 置信度[%.3f]",
              pInfo->dwAlarmType,
              pInfo->dwChannel,
              pInfo->dwImgLen,
              pInfo->fConfidence);
    return true;
#endif
}
} // namespace AiAppCommon
