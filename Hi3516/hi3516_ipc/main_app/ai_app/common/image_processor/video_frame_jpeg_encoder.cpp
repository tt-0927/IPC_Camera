/**
 * @FilePath     : video_frame_jpeg_encoder.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-29 13:50:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-10-10 14:22:05
 * @Description  : 视频帧 JPEG 编码公共工具实现
 */

#include "video_frame_jpeg_encoder.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <unistd.h>

#include "dlog.h"
#include "ffmpeg_image.h"
#include "IpcRet.h"
#include "jpeg_encode_service.hpp"
#include "share_data.h"

#ifdef ENABLE_TVSDK_SRC
#include "control_manage.h"
#endif

namespace AiAppCommon
{
/* FFmpeg 兼容路径默认 JPEG 编码质量 95，人脸抓拍场景需要高质量 */
static constexpr int DEFAULT_JPEG_QUALITY = 95;

/*
 * 编码调试开关：板端 touch /opt/cam/testJpegEncode 后生效（沿用 testPrint 调试惯例）：
 *   1. 无 TVSDK 客户端订阅时也临时编码（验证编码链路不依赖客户端连接）；
 *   2. 每次编码成功把结果覆盖写到 /tmp/jpeg_debug_latest.jpg 并打印后端与尺寸。
 */
static const char *JPEG_ENCODE_DEBUG_FLAG = "/opt/cam/bin/testJpegEncode";
static const char *JPEG_ENCODE_DEBUG_DUMP = "/tmp/jpeg_debug_latest.jpg";

static bool jpeg_debug_enabled()
{
    return access(JPEG_ENCODE_DEBUG_FLAG, F_OK) == 0;
}

static void jpeg_debug_dump(const char *pBackend, ot_video_frame_info *pFrameInfo, const std::vector<uint8_t> &vecJpeg)
{
    if (!jpeg_debug_enabled() || vecJpeg.empty())
    {
        return;
    }
    std::ofstream file(JPEG_ENCODE_DEBUG_DUMP, std::ios::binary | std::ios::trunc);
    if (file.is_open())
    {
        file.write(reinterpret_cast<const char *>(vecJpeg.data()), static_cast<std::streamsize>(vecJpeg.size()));
        file.close();
        dlog_info("[JPEG调试] 后端:%s 输入:%ux%u 输出:%zu字节 落盘:%s",
                  pBackend,
                  pFrameInfo->video_frame.width,
                  pFrameInfo->video_frame.height,
                  vecJpeg.size(),
                  JPEG_ENCODE_DEBUG_DUMP);
    }
}

/*
 * VENC 硬编码后端分派：VPSS AI 通道原始帧（1920x1080 非压缩）走 batch_crop 硬编码
 * （板端实测约 10ms），其余尺寸（VGS 裁剪后的目标小图等）暂走 FFmpeg 软编码兼容路径，
 * 待人脸抓拍迁移新架构后统一改为"整帧 + 抠图框"硬编码并下线 FFmpeg 分支。
 */
static int encode_jpeg_by_dispatch(ot_video_frame_info *pFrameInfo, std::vector<uint8_t> &vecJpeg)
{
    if (pFrameInfo->video_frame.width == JPEG_ENCODE_DEFAULT_WIDTH && pFrameInfo->video_frame.height == JPEG_ENCODE_DEFAULT_HEIGHT &&
        pFrameInfo->video_frame.compress_mode == OT_COMPRESS_MODE_NONE)
    {
        if (CJpegEncodeService::instance()->encodePanorama(*pFrameInfo, vecJpeg) == OK)
        {
            jpeg_debug_dump("VENC硬编码", pFrameInfo, vecJpeg);
            return OK;
        }
        return ERR;
    }
    return ERR;
}

/* FFmpeg 软编码兼容路径（VGS 裁剪小帧过渡期使用，内存编码经临时文件中转） */
static int encode_jpeg_by_ffmpeg(ot_video_frame_info *pFrameInfo, std::vector<uint8_t> &vecJpeg)
{
    CFfmpegImage image;
    image.SetQuality(DEFAULT_JPEG_QUALITY);
    const std::string strTempPath = "/tmp/ai_event_frame_compat.jpg";
    if (!image.Open(strTempPath, pFrameInfo->video_frame.width, pFrameInfo->video_frame.height))
    {
        return ERR;
    }
    char *pData = reinterpret_cast<char *>(pFrameInfo->video_frame.virt_addr[0]);
    const int nDataSize = pFrameInfo->video_frame.width * pFrameInfo->video_frame.height * 3 / 2;
    const bool bEncodeOk = image.SendFrame(pData, nDataSize);
    image.Close();
    if (!bEncodeOk)
    {
        std::remove(strTempPath.c_str());
        return ERR;
    }

    std::ifstream file(strTempPath, std::ios::binary);
    if (!file.is_open())
    {
        std::remove(strTempPath.c_str());
        return ERR;
    }
    file.seekg(0, std::ios::end);
    const std::streampos nFileSize = file.tellg();
    if (nFileSize <= 0)
    {
        file.close();
        std::remove(strTempPath.c_str());
        return ERR;
    }
    file.seekg(0, std::ios::beg);
    vecJpeg.resize(static_cast<size_t>(nFileSize));
    file.read(reinterpret_cast<char *>(vecJpeg.data()), static_cast<std::streamsize>(nFileSize));
    const bool bReadOk = file.gcount() == nFileSize;
    file.close();
    std::remove(strTempPath.c_str());
    return bReadOk ? OK : ERR;
}

/* 统一编码入口：硬编码优先，非通道尺寸帧回退 FFmpeg 兼容路径 */
static int encode_jpeg_common(ot_video_frame_info *pFrameInfo, std::vector<uint8_t> &vecJpeg)
{
    vecJpeg.clear();
    if (encode_jpeg_by_dispatch(pFrameInfo, vecJpeg) == OK)
    {
        return OK;
    }
    if (encode_jpeg_by_ffmpeg(pFrameInfo, vecJpeg) == OK)
    {
        jpeg_debug_dump("FFmpeg软编码", pFrameInfo, vecJpeg);
        return OK;
    }
    return ERR;
}

int encode_video_frame_to_jpeg_file(ot_video_frame_info *pFrameInfo, const std::string &strFilename)
{
    if (pFrameInfo == nullptr || strFilename.empty())
    {
        dlog_error("视频帧 JPEG 编码参数无效");
        return ERR_PARAM_NULL;
    }

    std::vector<uint8_t> vecJpeg;
    if (encode_jpeg_common(pFrameInfo, vecJpeg) != OK)
    {
        return ERR;
    }

    std::ofstream file(strFilename, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
    {
        dlog_error("创建 JPEG 文件失败[%s]", strFilename.c_str());
        return ERR;
    }
    file.write(reinterpret_cast<const char *>(vecJpeg.data()), static_cast<std::streamsize>(vecJpeg.size()));
    const bool bWriteOk = (file.good() && vecJpeg.size() > 0);
    file.flush();
    file.close();
    return bWriteOk ? OK : ERR;
}

int encode_video_frame_to_jpeg_memory(ot_video_frame_info *pFrameInfo, EventTvSdkImage_S &stImage)
{
    stImage = EventTvSdkImage_S();
    if (pFrameInfo == nullptr)
    {
        dlog_error("视频帧 JPEG 内存编码参数无效");
        return ERR_PARAM_NULL;
    }

    /* 硬编码路径直接产出内存数据，不再经临时文件中转 */
    std::vector<uint8_t> vecJpeg;
    if (encode_jpeg_common(pFrameInfo, vecJpeg) != OK)
    {
        return ERR;
    }

    stImage.vecJpeg = std::move(vecJpeg);
    stImage.nWidth = pFrameInfo->video_frame.width;
    stImage.nHeight = pFrameInfo->video_frame.height;
    stImage.strTag = "panorama";
    return OK;
}

int encode_event_image_to_jpeg_memory(ot_video_frame_info *pSrcFrameInfo,
                                      ot_video_frame_info *pAlgoFrame,
                                      const Common::Rect_S &stDetectRect,
                                      int nDetectWidth,
                                      int nDetectHeight,
                                      EventTvSdkImage_S &stImage)
{
    stImage = EventTvSdkImage_S();

    /* 源帧缺失：退回算法分辨率帧整幅编码（源帧理论必达，此为保底路径） */
    if (pSrcFrameInfo == nullptr)
    {
        if (pAlgoFrame == nullptr)
        {
            dlog_error("事件图编码参数无效：源帧与算法帧均为空");
            return ERR_PARAM_NULL;
        }
        return encode_video_frame_to_jpeg_memory(pAlgoFrame, stImage);
    }

    const int nFrameWidth = static_cast<int>(pSrcFrameInfo->video_frame.width);
    const int nFrameHeight = static_cast<int>(pSrcFrameInfo->video_frame.height);

    /* 区域覆盖全屏（专家模式即此形态）：源帧整幅编码，1080p 自动走 VENC 全景图通道 */
    const bool bFullFrame = stDetectRect.nX <= 0 && stDetectRect.nY <= 0 && stDetectRect.nX + stDetectRect.nWidth >= nDetectWidth &&
                            stDetectRect.nY + stDetectRect.nHeight >= nDetectHeight;
    if (bFullFrame || stDetectRect.isEmpty())
    {
        return encode_video_frame_to_jpeg_memory(pSrcFrameInfo, stImage);
    }

    /* 局部区域：算法坐标等比映射回源帧坐标（参考人脸抓拍 alignCropRect 的逆变换） */
    Common::Rect_S stRect = stDetectRect;
    if (!stRect.ConvertResolution(nDetectWidth, nDetectHeight, nFrameWidth, nFrameHeight))
    {
        return encode_video_frame_to_jpeg_memory(pSrcFrameInfo, stImage);
    }

    /*
     * batch_crop 硬编码约束（CV610）：x 8 / y 2 像素对齐、宽高 >= 32x32、不越界；
     * 右下角向外对齐后夹回帧内；对齐吞掉尺寸不足最小抠图框时向内侧补齐。
     */
    const int nMinSize = static_cast<int>(JPEG_ENCODE_CROP_MIN_SIZE);
    int nX1 = ALIGN_BACK(stRect.nX, 8);
    int nY1 = ALIGN_BACK(stRect.nY, 2);
    int nX2 = std::min(ALIGN_UP(stRect.nX + stRect.nWidth, 8), nFrameWidth);
    int nY2 = std::min(ALIGN_UP(stRect.nY + stRect.nHeight, 2), nFrameHeight);
    if (nX2 - nX1 < nMinSize)
    {
        nX1 = std::max(0, nX2 - nMinSize);
        nX2 = std::min(nFrameWidth, nX1 + nMinSize);
    }
    if (nY2 - nY1 < nMinSize)
    {
        nY1 = std::max(0, nY2 - nMinSize);
        nY2 = std::min(nFrameHeight, nY1 + nMinSize);
    }
    if (nX2 - nX1 < nMinSize || nY2 - nY1 < nMinSize)
    {
        /* 源帧自身小于最小抠图尺寸：退整幅出图 */
        return encode_video_frame_to_jpeg_memory(pSrcFrameInfo, stImage);
    }

    ot_rect stCrop;
    stCrop.x = nX1;
    stCrop.y = nY1;
    stCrop.width = static_cast<td_u32>(nX2 - nX1);
    stCrop.height = static_cast<td_u32>(nY2 - nY1);

    std::vector<ot_rect> vecRects;
    vecRects.push_back(stCrop);
    std::vector<std::vector<uint8_t>> vecJpegs;
    if (CJpegEncodeService::instance()->encodeCrops(*pSrcFrameInfo, vecRects, vecJpegs) == OK && vecJpegs.size() == 1 &&
        !vecJpegs[0].empty())
    {
        stImage.vecJpeg = std::move(vecJpegs[0]);
        stImage.nWidth = static_cast<int>(stCrop.width);
        stImage.nHeight = static_cast<int>(stCrop.height);
        stImage.strTag = "panorama";
        jpeg_debug_dump("VENC硬编码抠图", pSrcFrameInfo, stImage.vecJpeg);
        return OK;
    }

    /* 硬编码失败（源帧非 1920x1080、通道异常等）：退整幅，1080p 仍走硬编码分派 */
    return encode_video_frame_to_jpeg_memory(pSrcFrameInfo, stImage);
}

bool tvsdk_event_image_required()
{
    /* 调试模式：无 TVSDK 订阅也临时编码，用于不依赖客户端验证编码链路 */
    if (jpeg_debug_enabled())
    {
        return true;
    }
#ifdef ENABLE_TVSDK_SRC
    /*
     * perf: 无 TVSDK 客户端订阅时编码结果无人消费，直接跳过；
     * 硬编码虽快（1080p 约 10ms），跳过仍可避免无效的硬件通道排队与内存拷贝。
     */
    return ControlManage::instance()->tvsdk_get_client_count() > 0;
#else
    /* 未编译TVSDK源码时告警图无人消费，恒跳过 */
    return false;
#endif
}
} // namespace AiAppCommon
