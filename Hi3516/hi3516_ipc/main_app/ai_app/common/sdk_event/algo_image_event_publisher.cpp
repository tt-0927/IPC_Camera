/**
 * @FilePath     : algo_image_event_publisher.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 19:29:55
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-29 14:22:30
 * @Description  : AI 算法 SDK 图片事件推送公共类实现
 */

#include "algo_image_event_publisher.hpp"

#include <algorithm>
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

bool CAlgoImageEventPublisher::publishCapture(const SdkCaptureRequest_S &stRequest) const
{
#ifndef ENABLE_TVSDK_SRC
    (void)stRequest;
    return false;
#else
    if (!hasClient())
    {
        return false;
    }

    NET_AlarmCaptureInfo_S stInfo = {};
    stInfo.uAlarmType = stRequest.unAlarmType;
    stInfo.uChannel = static_cast<UINT32>(normalizeChannel(stRequest.nChnId));
    stInfo.uCaptureType = stRequest.unCaptureType;
    stInfo.llTimestampMs = stRequest.llTimestampMs > 0 ? stRequest.llTimestampMs : current_timestamp_ms();
    stInfo.uPanoramaWidth = stRequest.unPanoramaWidth;
    stInfo.uPanoramaHeight = stRequest.unPanoramaHeight;

    bool bHasImage = false;
    if (stRequest.pvecPanoramaJpeg && !stRequest.pvecPanoramaJpeg->empty())
    {
        if (stRequest.pvecPanoramaJpeg->size() > NET_PIC_DATA_MAX_LEN)
        {
            dlog_warn("SDK通用抓拍推送跳过，全景图大小[%zu]超过协议上限[%u]",
                      stRequest.pvecPanoramaJpeg->size(),
                      static_cast<unsigned int>(NET_PIC_DATA_MAX_LEN));
            return false;
        }
        stInfo.stPanoramaImg.pData = const_cast<BYTE *>(stRequest.pvecPanoramaJpeg->data());
        stInfo.stPanoramaImg.uDataLen = static_cast<UINT32>(stRequest.pvecPanoramaJpeg->size());
        bHasImage = true;
    }

    const size_t uTargetCount = std::min(stRequest.vecTargets.size(),
                                         static_cast<size_t>(NET_CAPTURE_CROP_MAX_NUM));
    for (size_t i = 0; i < uTargetCount; ++i)
    {
        const SdkCaptureTargetRequest_S &stRequestTarget = stRequest.vecTargets[i];
        if (!stRequestTarget.pvecJpeg || stRequestTarget.pvecJpeg->empty())
        {
            continue;
        }
        if (stRequestTarget.pvecJpeg->size() > NET_PIC_DATA_MAX_LEN)
        {
            dlog_warn("SDK通用抓拍目标图超限，跳过目标[%zu]，大小[%zu]，上限[%u]",
                      i,
                      stRequestTarget.pvecJpeg->size(),
                      static_cast<unsigned int>(NET_PIC_DATA_MAX_LEN));
            continue;
        }

        NET_CropImage_S &stCrop = stInfo.stCropImages[stInfo.uCropCount];
        stCrop.uCropX = static_cast<UINT32>(std::max(0, stRequestTarget.stRect.nX1));
        stCrop.uCropY = static_cast<UINT32>(std::max(0, stRequestTarget.stRect.nY1));
        stCrop.uCropWidth = static_cast<UINT32>(std::max(0, stRequestTarget.stRect.nX2 - stRequestTarget.stRect.nX1));
        stCrop.uCropHeight = static_cast<UINT32>(std::max(0, stRequestTarget.stRect.nY2 - stRequestTarget.stRect.nY1));
        stCrop.uTargetType = stRequestTarget.unObjectType;
        stCrop.fConfidence = stRequestTarget.fConfidence;
        stCrop.nTrackID = stRequestTarget.nTrackId;
        stCrop.stImage.pData = const_cast<BYTE *>(stRequestTarget.pvecJpeg->data());
        stCrop.stImage.uDataLen = static_cast<UINT32>(stRequestTarget.pvecJpeg->size());
        ++stInfo.uCropCount;
        bHasImage = true;
    }

    if (!bHasImage)
    {
        dlog_warn("SDK通用抓拍推送跳过，未生成有效 JPEG 图片");
        return false;
    }

    /* 发送函数同步完成 SDK JSON 序列化；本函数返回后调用方可释放 JPEG vector。 */
    const int nRet = ControlManage::instance()->tvsdk_push_alarm(static_cast<int>(stInfo.uAlarmType),
                                                                 &stInfo,
                                                                 sizeof(stInfo));
    if (nRet != 0)
    {
        dlog_warn("SDK通用抓拍推送失败，alarm[0x%x] ret[%d] 全景[%u] 目标数[%u]",
                  stInfo.uAlarmType,
                  nRet,
                  stInfo.stPanoramaImg.uDataLen,
                  stInfo.uCropCount);
        return false;
    }

    dlog_info("SDK通用抓拍推送成功，alarm[0x%x] 通道[%u] 全景[%u] 目标数[%u]",
              stInfo.uAlarmType,
              stInfo.uChannel,
              stInfo.stPanoramaImg.uDataLen,
              stInfo.uCropCount);
    return true;
#endif
}
} // namespace AiAppCommon
