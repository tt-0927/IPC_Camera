/**
 * @FilePath     : hisi_frame_image_provider.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : 海思原始帧图片能力实现
 */

#include "hisi_frame_image_provider.hpp"

#include <algorithm>

#include "IpcRet.h"
#include "dlog.h"
#include "normalized_geometry.hpp"
#include "share_data.h"
#include "video_frame_jpeg_encoder.hpp"

extern "C"
{
#include "mpp_vgs.h"
}

namespace HVFDetectInternal
{
namespace
{
/* 目标图扩大比例，以较长边为基准 */
constexpr float TARGET_IMAGE_SCALE_RATIO = 1.5f;
}

void CHisiFrameImageProvider::set_frame(const std::shared_ptr<ot_video_frame_info> &pFrameInfo)
{
    m_pFrameInfo = pFrameInfo;
}

void CHisiFrameImageProvider::clear_frame()
{
    m_pFrameInfo.reset();
}

int CHisiFrameImageProvider::build_panorama(EventStatistics_NS::ImagePayload_S &stImage)
{
    stImage = EventStatistics_NS::ImagePayload_S();
    if (m_pFrameInfo == nullptr)
    {
        dlog_warn("人流统计全景图失败：Frame Lease 为空");
        return ERR;
    }

    EventTvSdkImage_S stTvSdkImage;
    if (AiAppCommon::encode_video_frame_to_jpeg_memory(m_pFrameInfo.get(), stTvSdkImage) != OK)
    {
        dlog_warn("人流统计全景图编码失败");
        return ERR;
    }

    stImage.vecJpeg = std::move(stTvSdkImage.vecJpeg);
    stImage.nWidth = static_cast<int>(m_pFrameInfo->video_frame.width);
    stImage.nHeight = static_cast<int>(m_pFrameInfo->video_frame.height);
    stImage.strTag = "panorama";
    return OK;
}

int CHisiFrameImageProvider::build_target(const AiPipeline_NS::NormalizedRect_S &stRect, EventStatistics_NS::ImagePayload_S &stImage)
{
    stImage = EventStatistics_NS::ImagePayload_S();
    if (m_pFrameInfo == nullptr)
    {
        dlog_warn("人流统计目标图失败：Frame Lease 为空");
        return ERR;
    }

    if (!AiPipeline_NS::Geometry_NS::is_valid_normalized_rect(stRect))
    {
        dlog_warn("人流统计目标图失败：归一化目标框无效");
        return ERR;
    }

    /* 归一化目标框映射到实际源帧像素坐标 */
    const int nFrameW = static_cast<int>(m_pFrameInfo->video_frame.width);
    const int nFrameH = static_cast<int>(m_pFrameInfo->video_frame.height);
    const AiPipeline_NS::FrameSize_S stFrameSize{ static_cast<uint32_t>(nFrameW), static_cast<uint32_t>(nFrameH) };
    const Common::RectInfo_S stRectPixels = AiPipeline_NS::Geometry_NS::denormalize_rect(stRect, stFrameSize);

    const int nOrigW = stRectPixels.nX2 - stRectPixels.nX1;
    const int nOrigH = stRectPixels.nY2 - stRectPixels.nY1;
    if (nOrigW <= 0 || nOrigH <= 0)
    {
        dlog_warn("人流统计目标图失败：原始目标框无效 [%d,%d,%d,%d]",
                  stRectPixels.nX1,
                  stRectPixels.nY1,
                  stRectPixels.nX2,
                  stRectPixels.nY2);
        return ERR;
    }

    /* 以较长边为基准扩大 1.5 倍，作为正方形边长 */
    const int nMaxSide = std::max(nOrigW, nOrigH);
    const int nSquareSize = static_cast<int>(static_cast<float>(nMaxSide) * TARGET_IMAGE_SCALE_RATIO);

    /* 以原框中心点为基准计算正方形裁剪区域 */
    const int nCenterX = (stRectPixels.nX1 + stRectPixels.nX2) / 2;
    const int nCenterY = (stRectPixels.nY1 + stRectPixels.nY2) / 2;

    Common::RectInfo_S stCropRect;
    stCropRect.nX1 = nCenterX - nSquareSize / 2;
    stCropRect.nY1 = nCenterY - nSquareSize / 2;
    stCropRect.nX2 = stCropRect.nX1 + nSquareSize;
    stCropRect.nY2 = stCropRect.nY1 + nSquareSize;

    /* 边界检查，限制在图像有效范围内 */
    stCropRect.nX1 = std::max(0, stCropRect.nX1);
    stCropRect.nY1 = std::max(0, stCropRect.nY1);
    stCropRect.nX2 = std::min(nFrameW, stCropRect.nX2);
    stCropRect.nY2 = std::min(nFrameH, stCropRect.nY2);

    /* VGS 硬件对齐约束：宽度 16 字节对齐，高度 4 字节对齐 */
    /* 使用 ALIGN_UP 确保对齐后有效区域不缩小，避免小目标框归零 */
    stCropRect.nX1 = ALIGN_BACK(stCropRect.nX1, 16);
    stCropRect.nY1 = ALIGN_BACK(stCropRect.nY1, 4);
    stCropRect.nX2 = ALIGN_UP(stCropRect.nX2, 16);
    stCropRect.nY2 = ALIGN_UP(stCropRect.nY2, 4);

    /* 二次边界检查，防止对齐后超出图像范围 */
    stCropRect.nX2 = std::min(nFrameW, stCropRect.nX2);
    stCropRect.nY2 = std::min(nFrameH, stCropRect.nY2);

    if (stCropRect.nX2 <= stCropRect.nX1 || stCropRect.nY2 <= stCropRect.nY1)
    {
        dlog_warn("人流统计目标图失败：裁剪框对齐后无效 [%d,%d,%d,%d]", stCropRect.nX1, stCropRect.nY1, stCropRect.nX2, stCropRect.nY2);
        return ERR;
    }

    /* 裁剪后目标图宽高 */
    const unsigned int unDstWidth = static_cast<unsigned int>(stCropRect.nX2 - stCropRect.nX1);
    const unsigned int unDstHeight = static_cast<unsigned int>(stCropRect.nY2 - stCropRect.nY1);

    /* 创建裁剪目标帧 */
    ot_video_frame_info stDstFrameInfo;
    if (TD_SUCCESS != mppVgs_create_video_frame_info(unDstWidth, unDstHeight, OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420, &stDstFrameInfo))
    {
        dlog_warn("人流统计目标图创建 VGS 帧失败 [%u x %u]", unDstWidth, unDstHeight);
        return ERR;
    }

    /* VGS 裁剪区域 */
    ot_rect stVgsRect;
    stVgsRect.x = stCropRect.nX1;
    stVgsRect.y = stCropRect.nY1;
    stVgsRect.width = static_cast<td_s32>(unDstWidth);
    stVgsRect.height = static_cast<td_s32>(unDstHeight);

    /* VGS 裁剪，失败时释放临时帧后返回 */
    int nRet = ERR;
    if (TD_SUCCESS != mppVgs_crop(m_pFrameInfo.get(), &stDstFrameInfo, &stVgsRect))
    {
        dlog_warn("人流统计目标图 VGS 裁剪失败");
        nRet = ERR;
    }
    else
    {
        /* 编码为 JPEG 内存数据 */
        EventTvSdkImage_S stTvSdkImage;
        if (AiAppCommon::encode_video_frame_to_jpeg_memory(&stDstFrameInfo, stTvSdkImage) == OK)
        {
            stImage.vecJpeg = std::move(stTvSdkImage.vecJpeg);
            stImage.nWidth = static_cast<int>(unDstWidth);
            stImage.nHeight = static_cast<int>(unDstHeight);
            stImage.strTag = "target";
            nRet = OK;
        }
        else
        {
            dlog_warn("人流统计目标图 JPEG 编码失败");
            nRet = ERR;
        }
    }

    /* 单一清理出口，任何裁剪/编码失败都会释放 VGS 临时帧 */
    mppVgs_destroy_video_frame_info(&stDstFrameInfo);
    return nRet;
}
} // namespace HVFDetectInternal
