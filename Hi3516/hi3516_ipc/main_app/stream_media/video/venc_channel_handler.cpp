/**
 * @FilePath     : venc_channel_handler.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-01-08 09:49:07
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-10 14:00:58
 * @Description  : VENC通道处理策略实现
 */

#include "venc_channel_handler.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <new>
#include <vector>

#include "stream_video.h"
#include "push_stream.h"
#include "RtpServer.h"
#include "record_ctrl.h"
#include "capture_ctrl.h"
#include "stream_server.h"
#include "time_utils.h"
#include "dlog.h"
#include "IpcRet.h"

namespace
{
/* JPEG完整图片的硬上限；分辨率估算只用于 reserve 提示，不能替代该边界检查。 */
constexpr std::size_t MAX_JPEG_FRAME = 5U * 1024U * 1024U;

/* JPEG前一段容量提示的最小值，也是未拿到有效分辨率时的保守初始容量。 */
constexpr std::size_t MIN_JPEG_PREV_FRAME_RESERVE = 256U * 1024U;

/* 前一段按约 0.25 byte/pixel 估算，仅作为可增长 vector 的初始容量提示。 */
constexpr std::uint64_t JPEG_PREV_FRAME_BYTES_PER_PIXEL_DIVISOR = 4U;

/* 采用 64 字节对齐，和常见 DMA/编码 buffer 对齐习惯保持一致。 */
constexpr std::size_t JPEG_RESERVE_ALIGNMENT = 64U;

/* 容量明显超过提示值时才回收，避免每次抓图都申请/释放。 */
constexpr std::size_t JPEG_TRIM_CAPACITY_RATIO = 3U;
constexpr std::size_t JPEG_TRIM_CAPACITY_BASE = 2U;

/* IDR帧请求间隔时间（毫秒） */
constexpr int IDR_REQUEST_INTERVAL_MS = 5500;

/**
 * @brief   : 将字节数向上对齐
 * @param   {std::size_t} nValue：原始字节数
 * @param   {std::size_t} nAlignment：对齐粒度
 * @return  {std::size_t} 对齐后的字节数
 */
std::size_t alignUp(std::size_t nValue, std::size_t nAlignment)
{
    if (nAlignment == 0U)
    {
        return nValue;
    }

    const std::size_t nRemainder = nValue % nAlignment;
    if (nRemainder == 0U)
    {
        return nValue;
    }
    return nValue + (nAlignment - nRemainder);
}

/**
 * @brief   : 根据 JPEG 输出分辨率估算前一段缓存的初始容量
 * @param   {int} nWidth：JPEG 输出宽度
 * @param   {int} nHeight：JPEG 输出高度
 * @return  {std::size_t} reserve 提示值
 * @note    : 该值不是码流硬上限；实际 vector 仍可增长到 MAX_JPEG_FRAME 检查允许的范围。
 */
std::size_t calculateJpegPrevFrameReserve(int nWidth, int nHeight)
{
    if (nWidth <= 0 || nHeight <= 0)
    {
        return MIN_JPEG_PREV_FRAME_RESERVE;
    }

    /* 编码器通常按 16 对齐，先用对齐后的有效像素数估算。 */
    const std::uint64_t nAlignedWidth = (static_cast<std::uint64_t>(nWidth) + 15U) / 16U * 16U;
    const std::uint64_t nAlignedHeight = (static_cast<std::uint64_t>(nHeight) + 15U) / 16U * 16U;
    const std::uint64_t nPixelCount = nAlignedWidth * nAlignedHeight;
    const std::uint64_t nEstimatedBytes = (nPixelCount + JPEG_PREV_FRAME_BYTES_PER_PIXEL_DIVISOR - 1U) /
                                          JPEG_PREV_FRAME_BYTES_PER_PIXEL_DIVISOR;

    const std::uint64_t nClampedBytes = std::min<std::uint64_t>(std::max<std::uint64_t>(nEstimatedBytes, MIN_JPEG_PREV_FRAME_RESERVE),
                                                                MAX_JPEG_FRAME / 2U);
    return alignUp(static_cast<std::size_t>(nClampedBytes), JPEG_RESERVE_ALIGNMENT);
}
}

CMainChannelHandler::CMainChannelHandler(CStreamVideo *pStreamVideo) : m_pStreamVideo(pStreamVideo), m_llLastIdrTimestamp(-1)
{
}

void CMainChannelHandler::handleFrame(const VencFrameView_S &stFrame, CStreamVideoConfig &configManager, int nChannel)
{
    if (!stFrame.pData || stFrame.nDataLen <= 0)
    {
        dlog_error("主码流处理器：VENC帧视图无效");
        return;
    }

    const auto &videoConfig = configManager.getVideoConfigs().at(nChannel);

    /* SVAC3编码不发送到RTSP */
    if (videoConfig.enVideoCodec != Video_NS::VideoCodec_E::SVAC3)
    {
        /*
         * perf: 共享帧优先走零拷贝路径（RTSP/RTMP 直接持有引用）；
         * 无共享帧时（构造失败回退）走旧拷贝路径。
         */
        if (stFrame.stSharedFrame.pData)
        {
            CPushStream::instance()->sendVideoData(stFrame.stSharedFrame, stFrame.enVideoCodec, stFrame.eType, true, true);
        }
        else
        {
            CPushStream::instance()->sendVideoData(stFrame.pData, stFrame.nDataLen, stFrame.enVideoCodec, stFrame.eType, true, true);
        }
    }

    /* 发送到GB28181（C风格RTP队列保持独立拷贝，不参与共享） */
    SIP::CRtpServer::instance()->sendVideoData(stFrame.pData, stFrame.nDataLen);

#if CAP_RECORD_USE_MAIN_STREAM
    /* 检查是否在录制状态 */
    if (CRecordCtrl::instance()->get_record_status() == Record_NS::Status_E::RECORD_OPERATION)
    {
        /* 发送到录制模块（共享帧零拷贝入队） */
        if (stFrame.stSharedFrame.pData)
        {
            CStreamServer::instance()->sendVideoData(stFrame.stSharedFrame);
        }
        else
        {
            CStreamServer::instance()->sendVideoData(stFrame.pData, stFrame.nDataLen);
        }

        /* 检查是否需要请求IDR帧 */
        checkAndRequestIdr(configManager, nChannel);
    }
#endif
}

void CMainChannelHandler::checkAndRequestIdr(CStreamVideoConfig &configManager, int nChannel)
{
    const auto &videoConfig = configManager.getVideoConfigs().at(nChannel);

    /* I帧间隔/帧率 >= 5：防止因获取不到I帧导致TS文件时长比预设的6s要长 */
    if (1.0 * videoConfig.nIFrameInterval / videoConfig.getFrameRateAsInt() >= 5)
    {
        long long int currentTime = TimeUtils_NS::get_currentTimestampMs();

        /* 初始化时间戳 */
        if (m_llLastIdrTimestamp == -1)
        {
            m_llLastIdrTimestamp = currentTime;
        }

        /* 每隔5.5秒请求一次IDR帧 */
        if (currentTime - m_llLastIdrTimestamp >= IDR_REQUEST_INTERVAL_MS)
        {
            if (m_pStreamVideo)
            {
                m_pStreamVideo->request_idr(nChannel);
                m_llLastIdrTimestamp = currentTime;
            }
        }
    }
}

CSubChannelHandler::CSubChannelHandler(CStreamVideo *pStreamVideo) : m_pStreamVideo(pStreamVideo), m_llLastIdrTimestamp(-1)
{
}

void CSubChannelHandler::handleFrame(const VencFrameView_S &stFrame, CStreamVideoConfig &configManager, int nChannel)
{
    if (!stFrame.pData || stFrame.nDataLen <= 0)
    {
        dlog_error("子码流处理器：VENC帧视图无效");
        return;
    }

    /* perf: 共享帧优先走零拷贝路径，无共享帧时（构造失败回退）走旧拷贝路径 */
    if (stFrame.stSharedFrame.pData)
    {
        CPushStream::instance()->sendVideoData(stFrame.stSharedFrame, stFrame.enVideoCodec, stFrame.eType, false, true);
    }
    else
    {
        CPushStream::instance()->sendVideoData(stFrame.pData, stFrame.nDataLen, stFrame.enVideoCodec, stFrame.eType, false, true);
    }

#if !CAP_RECORD_USE_MAIN_STREAM
    /* 检查是否在录制状态 */
    if (CRecordCtrl::instance()->get_record_status() == Record_NS::Status_E::RECORD_OPERATION)
    {
        /* 发送到录制模块（共享帧零拷贝入队） */
        if (stFrame.stSharedFrame.pData)
        {
            CStreamServer::instance()->sendVideoData(stFrame.stSharedFrame);
        }
        else
        {
            CStreamServer::instance()->sendVideoData(stFrame.pData, stFrame.nDataLen);
        }

        /* 检查是否需要请求IDR帧 */
        checkAndRequestIdr(configManager, nChannel);
    }
#endif
}

void CSubChannelHandler::checkAndRequestIdr(CStreamVideoConfig &configManager, int nChannel)
{
    const auto &videoConfig = configManager.getVideoConfigs().at(nChannel);

    /* I帧间隔/帧率 >= 5：防止因获取不到I帧导致TS文件时长比预设的6s要长 */
    if (1.0 * videoConfig.nIFrameInterval / videoConfig.getFrameRateAsInt() >= 5)
    {
        long long int currentTime = TimeUtils_NS::get_currentTimestampMs();

        /* 初始化时间戳 */
        if (m_llLastIdrTimestamp == -1)
        {
            m_llLastIdrTimestamp = currentTime;
        }

        /* 每隔5.5秒请求一次IDR帧 */
        if (currentTime - m_llLastIdrTimestamp >= IDR_REQUEST_INTERVAL_MS)
        {
            if (m_pStreamVideo)
            {
                m_pStreamVideo->request_idr(nChannel);
                m_llLastIdrTimestamp = currentTime;
            }
        }
    }
}

CJpegChannelHandler::CJpegChannelHandler() : m_nFrameCount(0), m_prevFrameCapacityHint(MIN_JPEG_PREV_FRAME_RESERVE)
{
    /* memory: JPEG 数据到达前不申请大块缓存，避免初始化阶段常驻 2.5 MiB。 */
}

void CJpegChannelHandler::handleFrame(const VencFrameView_S &stFrame, CStreamVideoConfig &configManager, int nChannel)
{
    /*
     * memory: JPEG 通道直接处理 VENC pack 原始数据，不创建 VideoFrame 副本。
     * 配置只用于给前一段缓存提供按分辨率计算的初始容量提示，不能限制真实码流。
     */
    if (!stFrame.pData || stFrame.nDataLen <= 0)
    {
        return;
    }

    const auto &videoConfigs = configManager.getVideoConfigs();
    if (nChannel >= 0 && static_cast<std::size_t>(nChannel) < videoConfigs.size())
    {
        const auto &jpegConfig = videoConfigs[static_cast<std::size_t>(nChannel)];
        updatePrevFrameCapacityHint(jpegConfig.stVideoResolution.nWidth, jpegConfig.stVideoResolution.nHeight);
    }

    sendFrameData(stFrame.pData, stFrame.nDataLen);
}

bool CJpegChannelHandler::cachePrevFrame(const uint8_t *pData, int nDataLen)
{
    try
    {
        m_prevFrame.assign(pData, pData + nDataLen);
        updateObservedPrevFrameCapacity();
        return true;
    }
    catch (const std::bad_alloc &)
    {
        /* memory: 前一段申请失败时清空状态，避免下一 pack 误与残留数据合并。 */
        dlog_error("JPEG前一段缓存内存分配失败，长度:%d", nDataLen);
        m_nFrameCount = 0;
        m_prevFrame.clear();
        trimPrevFrameCapacity();
        return false;
    }
}

void CJpegChannelHandler::updatePrevFrameCapacityHint(int nWidth, int nHeight)
{
    const std::size_t nCapacityHint = calculateJpegPrevFrameReserve(nWidth, nHeight);
    if (nCapacityHint == m_prevFrameCapacityHint)
    {
        return;
    }

    const std::size_t nPreviousHint = m_prevFrameCapacityHint;
    m_prevFrameCapacityHint = nCapacityHint;

    /* 分辨率升高时提前准备容量，降低首个大 pack 的扩容次数。 */
    if (m_prevFrame.capacity() < nCapacityHint)
    {
        try
        {
            m_prevFrame.reserve(nCapacityHint);
        }
        catch (const std::bad_alloc &)
        {
            /* 让后续 assign/insert 继续按实际数据尝试，硬上限检查仍然有效。 */
            dlog_warn("JPEG前一段缓存 reserve 失败，hint:%zu previous:%zu", nCapacityHint, nPreviousHint);
        }
        return;
    }

    /* 有待合并数据时不能替换 vector，避免释放当前图片的前半段。 */
    if (m_nFrameCount != 0 || m_prevFrame.capacity() == 0U)
    {
        return;
    }

    /* 分辨率降低后，仅在容量明显偏大时回收，避免频繁分配/释放。 */
    if (m_prevFrame.capacity() * JPEG_TRIM_CAPACITY_BASE <= nCapacityHint * JPEG_TRIM_CAPACITY_RATIO)
    {
        return;
    }

    trimPrevFrameCapacity();
}

void CJpegChannelHandler::updateObservedPrevFrameCapacity()
{
    const std::size_t nObservedCapacity = std::min<std::size_t>(m_prevFrame.capacity(), MAX_JPEG_FRAME);
    if (nObservedCapacity > m_prevFrameCapacityHint)
    {
        m_prevFrameCapacityHint = nObservedCapacity;
    }
}

void CJpegChannelHandler::trimPrevFrameCapacity()
{
    if (m_nFrameCount != 0 || m_prevFrameCapacityHint == 0U ||
        m_prevFrame.capacity() * JPEG_TRIM_CAPACITY_BASE <= m_prevFrameCapacityHint * JPEG_TRIM_CAPACITY_RATIO)
    {
        return;
    }

    try
    {
        std::vector<uint8_t> resized;
        resized.reserve(m_prevFrameCapacityHint);
        m_prevFrame.swap(resized);
    }
    catch (const std::bad_alloc &)
    {
        /* 回收不是正确性条件，失败时保留旧容量继续服务。 */
        dlog_warn("JPEG前一段缓存收缩失败，capacity:%zu hint:%zu", m_prevFrame.capacity(), m_prevFrameCapacityHint);
    }
}

int CJpegChannelHandler::sendFrameData(const uint8_t *pData, int nDataLen)
{
    if (!pData || nDataLen <= 0)
    {
        return ERR;
    }

    /* 检查帧大小 */
    if (static_cast<std::size_t>(nDataLen) > MAX_JPEG_FRAME)
    {
        dlog_error("JPEG帧过大: %d", nDataLen);
        m_nFrameCount = 0;
        m_prevFrame.clear();
        trimPrevFrameCapacity();
        return ERR;
    }

    /* 两帧合并为一张完整图片 */
    if ((m_nFrameCount & 1) == 0) /* 偶数帧：缓存 */
    {
        if (!cachePrevFrame(pData, nDataLen))
        {
            return ERR;
        }
        ++m_nFrameCount;
        return OK;
    }
    else /* 奇数帧：合并 */
    {
        /* 检查是否有前一帧 */
        if (m_prevFrame.empty())
        {
            dlog_error("缺少前一帧JPEG数据，自动恢复");
            m_nFrameCount = 0;
            if (!cachePrevFrame(pData, nDataLen))
            {
                return ERR;
            }
            ++m_nFrameCount;
            return OK;
        }

        /* 检查合并后的大小 */
        const std::size_t totalSize = m_prevFrame.size() + static_cast<std::size_t>(nDataLen);
        if (totalSize > MAX_JPEG_FRAME)
        {
            dlog_error("合并后JPEG帧过大");
            m_nFrameCount = 0;
            m_prevFrame.clear();
            trimPrevFrameCapacity();
            return ERR;
        }

        try
        {
            /* 合并两帧 */
            std::vector<uint8_t> combined;
            combined.reserve(totalSize);
            combined.insert(combined.end(), m_prevFrame.begin(), m_prevFrame.end());
            combined.insert(combined.end(), pData, pData + nDataLen);

            /* 清理状态 */
            m_prevFrame.clear();
            m_nFrameCount = 0;

            /* 发送到抓图模块 */
            CCaptureCtrl::instance()->send_frameData(combined.data(), static_cast<int>(combined.size()));
            trimPrevFrameCapacity();

            return OK;
        }
        catch (const std::bad_alloc &)
        {
            dlog_error("JPEG帧内存分配失败，自动恢复");
            m_nFrameCount = 0;
            m_prevFrame.clear();
            trimPrevFrameCapacity();
            return ERR;
        }
    }
}
