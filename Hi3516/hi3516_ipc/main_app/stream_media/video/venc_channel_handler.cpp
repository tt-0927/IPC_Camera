/**
 * @FilePath     : venc_channel_handler.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-01-08 09:49:07
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-20 17:30:00
 * @Description  : VENC通道处理策略实现
 */

#include "venc_channel_handler.h"

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
/* JPEG帧最大尺寸 */
constexpr int MAX_JPEG_FRAME = 5 * 1024 * 1024;

/* IDR帧请求间隔时间（毫秒） */
constexpr int IDR_REQUEST_INTERVAL_MS = 5500;
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

CJpegChannelHandler::CJpegChannelHandler() : m_nFrameCount(0)
{
    m_prevFrame.reserve(MAX_JPEG_FRAME / 2);
}

void CJpegChannelHandler::handleFrame(const VencFrameView_S &stFrame, CStreamVideoConfig &configManager, int nChannel)
{
    /* JPEG通道直接处理VENC pack原始数据，不创建VideoFrame副本。 */
    if (stFrame.pData && stFrame.nDataLen > 0)
    {
        sendFrameData(stFrame.pData, stFrame.nDataLen);
    }
}

int CJpegChannelHandler::sendFrameData(const uint8_t *pData, int nDataLen)
{
    if (!pData || nDataLen <= 0)
    {
        return ERR;
    }

    /* 检查帧大小 */
    if (nDataLen > MAX_JPEG_FRAME)
    {
        dlog_error("JPEG帧过大: %d", nDataLen);
        m_nFrameCount = 0;
        m_prevFrame.clear();
        return ERR;
    }

    /* 两帧合并为一张完整图片 */
    if ((m_nFrameCount & 1) == 0) /* 偶数帧：缓存 */
    {
        m_prevFrame.assign(pData, pData + nDataLen);
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
            m_prevFrame.assign(pData, pData + nDataLen);
            ++m_nFrameCount;
            return OK;
        }

        /* 检查合并后的大小 */
        const size_t totalSize = m_prevFrame.size() + nDataLen;
        if (totalSize > MAX_JPEG_FRAME)
        {
            dlog_error("合并后JPEG帧过大");
            m_nFrameCount = 0;
            m_prevFrame.clear();
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

            return OK;
        }
        catch (const std::bad_alloc &)
        {
            dlog_error("JPEG帧内存分配失败，自动恢复");
            m_nFrameCount = 0;
            m_prevFrame.clear();
            return ERR;
        }
    }
}
