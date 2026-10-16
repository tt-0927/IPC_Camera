/**
 * @FilePath     : jpeg_encode_service.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-20
 * @Description  : VENC 硬件 JPEG 编码服务实现。
 */

#include "jpeg_encode_service.hpp"

#include <new>

#include "dlog.h"
#include "IpcRet.h"
#include "mpp_venc_jpeg.h"
#include "stream_venc.h"

namespace AiAppCommon
{

CJpegEncodeService *CJpegEncodeService::instance()
{
    static CJpegEncodeService s_instance;
    return &s_instance;
}

int CJpegEncodeService::ensureChannelLocked()
{
    if (m_bInited)
    {
        return OK;
    }

    mpp_venc_jpeg_attr stAttr;
    stAttr.chn = VENC_CHN_JPEG_USER;
    stAttr.width = JPEG_ENCODE_DEFAULT_WIDTH;
    stAttr.height = JPEG_ENCODE_DEFAULT_HEIGHT;
    stAttr.qfactor = JPEG_ENCODE_DEFAULT_QFACTOR;
    if (mppVencJpeg_create(&stAttr) != 0)
    {
        dlog_error("用户态 JPEG 编码通道创建失败 chn=%d", (int) VENC_CHN_JPEG_USER);
        return ERR;
    }
    m_bInited = true;
    dlog_info("用户态 JPEG 编码通道就绪 chn=%d %ux%u q=%u",
              (int) VENC_CHN_JPEG_USER,
              JPEG_ENCODE_DEFAULT_WIDTH,
              JPEG_ENCODE_DEFAULT_HEIGHT,
              JPEG_ENCODE_DEFAULT_QFACTOR);
    return OK;
}

int CJpegEncodeService::releaseChannel()
{
    std::lock_guard<std::mutex> stLock(m_mutex);
    if (!m_bInited)
    {
        return OK;
    }
    if (mppVencJpeg_destroy(VENC_CHN_JPEG_USER) != 0)
    {
        dlog_error("用户态 JPEG 编码通道销毁失败，通道保留待进程退出回收");
        return ERR;
    }
    m_bInited = false;
    /* 缓冲随通道一并释放，下次编码按需重建 */
    std::vector<std::vector<uint8_t>>().swap(m_vecCropBufs);
    dlog_info("用户态 JPEG 编码通道已释放（人脸算法全部关闭）");
    return OK;
}

int CJpegEncodeService::encodePanorama(const ot_video_frame_info &stFrameInfo, std::vector<uint8_t> &vecJpeg, uint32_t unQfactor)
{
    vecJpeg.clear();
    /* batch_crop 硬性约束：帧尺寸必须等于通道尺寸 */
    if (stFrameInfo.video_frame.width != JPEG_ENCODE_DEFAULT_WIDTH || stFrameInfo.video_frame.height != JPEG_ENCODE_DEFAULT_HEIGHT)
    {
        dlog_warn("全景帧尺寸(%ux%u)与编码通道(%ux%u)不一致，拒绝硬编码",
                  stFrameInfo.video_frame.width,
                  stFrameInfo.video_frame.height,
                  JPEG_ENCODE_DEFAULT_WIDTH,
                  JPEG_ENCODE_DEFAULT_HEIGHT);
        return ERR;
    }

    std::lock_guard<std::mutex> stLock(m_mutex);
    if (ensureChannelLocked() != OK)
    {
        return ERR;
    }
    if (unQfactor != 0 && mppVencJpeg_set_qfactor(VENC_CHN_JPEG_USER, unQfactor) != 0)
    {
        dlog_warn("设置 qfactor=%u 失败，沿用当前质量", unQfactor);
    }

    /* 复用成员缓冲，避免每次编码分配；w*h*2 覆盖单张 JPEG 上限 */
    const size_t szCap = static_cast<size_t>(JPEG_ENCODE_DEFAULT_WIDTH) * JPEG_ENCODE_DEFAULT_HEIGHT * 2;
    if (m_vecCropBufs.empty() || m_vecCropBufs[0].size() < szCap)
    {
        m_vecCropBufs.resize(1);
        try
        {
            m_vecCropBufs[0].resize(szCap);
        }
        catch (const std::bad_alloc &)
        {
            dlog_error("JPEG 输出缓冲分配失败 size=%zu", szCap);
            return ERR;
        }
    }

    uint32_t unLen = 0;
    if (mppVencJpeg_encode_frame(VENC_CHN_JPEG_USER,
                                 &stFrameInfo,
                                 m_vecCropBufs[0].data(),
                                 static_cast<td_u32>(szCap),
                                 &unLen,
                                 JPEG_ENCODE_TIMEOUT_MS) != 0)
    {
        dlog_error("全景硬编码失败");
        return ERR;
    }
    try
    {
        vecJpeg.assign(m_vecCropBufs[0].data(), m_vecCropBufs[0].data() + unLen);
    }
    catch (const std::bad_alloc &)
    {
        dlog_error("JPEG 结果拷贝失败 len=%u", unLen);
        return ERR;
    }
    return OK;
}

int CJpegEncodeService::encodeCrops(const ot_video_frame_info &stFrameInfo,
                                    const std::vector<ot_rect> &vecRects,
                                    std::vector<std::vector<uint8_t>> &vecJpegs,
                                    uint32_t unQfactor)
{
    vecJpegs.clear();
    if (vecRects.empty() || vecRects.size() > MPP_VENC_JPEG_MAX_CROP_NUM)
    {
        return ERR;
    }
    if (stFrameInfo.video_frame.width != JPEG_ENCODE_DEFAULT_WIDTH || stFrameInfo.video_frame.height != JPEG_ENCODE_DEFAULT_HEIGHT)
    {
        dlog_warn("抠图源帧尺寸(%ux%u)与编码通道(%ux%u)不一致，拒绝硬编码",
                  stFrameInfo.video_frame.width,
                  stFrameInfo.video_frame.height,
                  JPEG_ENCODE_DEFAULT_WIDTH,
                  JPEG_ENCODE_DEFAULT_HEIGHT);
        return ERR;
    }

    std::lock_guard<std::mutex> stLock(m_mutex);
    if (ensureChannelLocked() != OK)
    {
        return ERR;
    }
    if (unQfactor != 0 && mppVencJpeg_set_qfactor(VENC_CHN_JPEG_USER, unQfactor) != 0)
    {
        dlog_warn("设置 qfactor=%u 失败，沿用当前质量", unQfactor);
    }

    /* 每张抠图按 w*h*2 预留输出缓冲；缓冲数组跨调用复用（resize 只增不减） */
    try
    {
        m_vecCropBufs.resize(vecRects.size());
    }
    catch (const std::bad_alloc &)
    {
        return ERR;
    }
    std::vector<mpp_venc_jpeg_out> stOuts(vecRects.size());
    for (size_t i = 0; i < vecRects.size(); i++)
    {
        const size_t szCap = static_cast<size_t>(vecRects[i].width) * static_cast<size_t>(vecRects[i].height) * 2;
        if (m_vecCropBufs[i].size() < szCap)
        {
            try
            {
                m_vecCropBufs[i].resize(szCap);
            }
            catch (const std::bad_alloc &)
            {
                dlog_error("抠图[%zu]输出缓冲分配失败 size=%zu", i, szCap);
                return ERR;
            }
        }
        stOuts[i].buf = m_vecCropBufs[i].data();
        stOuts[i].cap = static_cast<td_u32>(m_vecCropBufs[i].size());
        stOuts[i].len = 0;
    }

    if (mppVencJpeg_encode_crops(VENC_CHN_JPEG_USER,
                                 &stFrameInfo,
                                 vecRects.data(),
                                 static_cast<td_u32>(vecRects.size()),
                                 stOuts.data(),
                                 JPEG_ENCODE_TIMEOUT_MS) != 0)
    {
        dlog_error("批量抠图硬编码失败 num=%zu", vecRects.size());
        return ERR;
    }

    try
    {
        vecJpegs.resize(vecRects.size());
        for (size_t i = 0; i < vecRects.size(); i++)
        {
            vecJpegs[i].assign(stOuts[i].buf, stOuts[i].buf + stOuts[i].len);
        }
    }
    catch (const std::bad_alloc &)
    {
        dlog_error("抠图结果拷贝失败");
        return ERR;
    }
    return OK;
}

} // namespace AiAppCommon
