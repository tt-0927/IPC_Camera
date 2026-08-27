/**
 * @file StreamHandler.cpp
 * @author xiejh (xiejh@kfb.cn)
 * @date 2024-10-29
 *
 * @brief 处理AiAppStreamClient的数据
 */
#include "stream_handler.hpp"

/* 开始获取数据 */
bool CStreamHandler::start()
{

    return true;
}

/* 停止获取数据 */
bool CStreamHandler::stop()
{

    return true;
}

/* NV12 缩小 (优先 RGA 硬件缩放, 失败回退 CPU INTER_AREA) */
static bool scaleNv12(const char *pSrc, int nSrcW, int nSrcH,
                      int nDstW, int nDstH, std::vector<char> &scaleBuf,
                      std::shared_ptr<char[]> &pOut)
{
    if (pSrc == nullptr || nSrcW <= 0 || nSrcH <= 0 ||
        nDstW <= 0 || nDstH <= 0 || (nDstW & 1) || (nDstH & 1) ||
        nDstW > nSrcW || nDstH > nSrcH)
    {
        return false;
    }

    const size_t nDstSize = static_cast<size_t>(nDstW) * nDstH * 3 / 2;

    /* RGA 硬件缩放: NV12 -> NV12 一次完成, 4K 帧下大幅降低 CPU 占用 */
    pOut = std::shared_ptr<char[]>(new char[nDstSize]);
    if (pOut && pOut.get() &&
        rga_image_transform(const_cast<char *>(pSrc), nSrcW, nSrcH, RK_FORMAT_YCbCr_420_SP,
                            pOut.get(), nDstW, nDstH, RK_FORMAT_YCbCr_420_SP,
                            0, 0, nSrcW, nSrcH, 0))
    {
        return true;
    }
    pOut.reset();
    dlog_warn("RGA 缩放失败, 回退 CPU 缩放");

    /* CPU 回退: Y 平面单通道 + UV 平面双通道 INTER_AREA, 避免交错字节被跨通道平均 */
    scaleBuf.resize(nDstSize);

    /* Y 平面: 单通道缩放 */
    cv::Mat srcY(nSrcH, nSrcW, CV_8UC1, const_cast<char *>(pSrc));
    cv::Mat dstY(nDstH, nDstW, CV_8UC1, scaleBuf.data());
    cv::resize(srcY, dstY, cv::Size(nDstW, nDstH), 0, 0, cv::INTER_AREA);

    /* UV 平面: 按 (Cb,Cr) 双通道缩放, 避免交错字节被跨通道平均 */
    cv::Mat srcUV(nSrcH / 2, nSrcW / 2, CV_8UC2,
                  const_cast<char *>(pSrc) + static_cast<size_t>(nSrcW) * nSrcH);
    cv::Mat dstUV(nDstH / 2, nDstW / 2, CV_8UC2,
                  scaleBuf.data() + static_cast<size_t>(nDstW) * nDstH);
    cv::resize(srcUV, dstUV, cv::Size(nDstW / 2, nDstH / 2), 0, 0, cv::INTER_AREA);

    pOut = std::shared_ptr<char[]>(new char[nDstSize]);
    memcpy(pOut.get(), scaleBuf.data(), nDstSize);
    return true;
}

std::shared_ptr<char[]> CStreamHandler::allocFullFrame(const char *pSrc, int nLen)
{
    if (pSrc == nullptr || nLen <= 0)
    {
        return nullptr;
    }

    /* 帧尺寸变化时重建池: 仅当所有槽均无引用才可安全重建, 否则本次走临时分配 */
    if (m_fullFrameSize != static_cast<size_t>(nLen))
    {
        bool bAllFree = true;
        for (int i = 0; i < kFullPoolSlots; i++)
        {
            if (m_fullPoolRefs[i].load(std::memory_order_acquire) != 0)
            {
                bAllFree = false;
                break;
            }
        }
        if (bAllFree)
        {
            for (int i = 0; i < kFullPoolSlots; i++)
            {
                delete[] m_fullPoolBuf[i];
                m_fullPoolBuf[i] = new char[nLen];
            }
            m_fullFrameSize = nLen;
        }
    }

    /* 优先复用空闲池槽: 跳过仍有引用的槽, 保证已有帧内容不被覆盖 */
    for (int i = 0; i < kFullPoolSlots; i++)
    {
        const int idx = (m_fullSlot + 1 + i) % kFullPoolSlots;
        if (m_fullPoolBuf[idx] == nullptr || m_fullFrameSize != static_cast<size_t>(nLen))
        {
            continue;
        }
        if (m_fullPoolRefs[idx].load(std::memory_order_acquire) == 0)
        {
            memcpy(m_fullPoolBuf[idx], pSrc, nLen);
            m_fullSlot = idx;
            m_fullPoolRefs[idx].store(1, std::memory_order_release);
            return std::shared_ptr<char[]>(m_fullPoolBuf[idx],
                [this, idx](char *) { m_fullPoolRefs[idx].fetch_sub(1, std::memory_order_acq_rel); });
        }
    }

    /* 池全忙 (帧积压): 临时分配, 由智能指针正常释放 */
    std::shared_ptr<char[]> pTemp(new char[nLen]);
    if (pTemp && pTemp.get())
    {
        memcpy(pTemp.get(), pSrc, nLen);
    }
    return pTemp;
}

/**
 * @brief 接受媒体数据
 * @param pData 数据结构体
 */
void CStreamHandler::recvDataProcess(const void *pData, int nLen, int nH, int nW)
{
    if (pData == nullptr || nH == 0 || nW == 0)
    {
        dlog_error("无效数据或大小");
        return;
    }

    MediaData_S stRecvData;
    stRecvData.enType = MediaDataType_E::VIDEO_DATA;
    stRecvData.stMediaParam.enPixelFormat = RK_FMT_YUV420SP;

    /* 4K 全分辨率帧: 缩小 1080p 送算法/模型, 原帧保留作人脸特写裁剪源 */
    if (nW > PIXEL_WIDTH_1920 && nLen > 0)
    {
        const int nDstW = PIXEL_WIDTH_1920;
        const int nDstH = (nH * nDstW / nW) & ~1;
        if (nDstH > 0)
        {
            std::shared_ptr<char[]> pScaleData;
            if (scaleNv12(static_cast<const char *>(pData), nW, nH,
                          nDstW, nDstH, m_scaleBuf, pScaleData))
            {
                stRecvData.nSize = static_cast<int64_t>(nDstW) * nDstH * 3 / 2;
                stRecvData.pData = pScaleData;
                stRecvData.stMediaParam.nVideoWidth = nDstW;
                stRecvData.stMediaParam.nVideoHeight = nDstH;

                /* 池化 4K 帧拷贝 (引用计数复用, 保证裁剪时帧内容不被新帧覆盖) */
                stRecvData.pFullData = allocFullFrame(static_cast<const char *>(pData), nLen);
                stRecvData.nFullWidth = nW;
                stRecvData.nFullHeight = nH;

                send_videoData(stRecvData);
                return;
            }
            dlog_warn("4K 帧缩小失败, 按原尺寸转发");
        }
    }

    /* 常规帧: 原样转发 */
    stRecvData.nSize = nLen;
    if (stRecvData.nSize > 0)
    {
        stRecvData.pData = std::shared_ptr<char[]>(new char[stRecvData.nSize]);
        if (NULL == stRecvData.pData || NULL == stRecvData.pData.get())
        {
            dlog_error("创建智能指针失败");
            return;
        }

        memcpy(stRecvData.pData.get(), pData, stRecvData.nSize);
        stRecvData.stMediaParam.nVideoWidth = nW;
        stRecvData.stMediaParam.nVideoHeight = nH;

        send_videoData(stRecvData);
    }
}

void CStreamHandler::recvDataProcess(const Video_NS::VideoFrame_S *pFrame, int nH, int nW)
{
    if (pFrame == nullptr || nH == 0 || nW == 0)
    {
        dlog_error("无效数据或大小");
        return;
    }

    MediaData_S stRecvData;
    stRecvData.nSize = pFrame->nLen;

    if (stRecvData.nSize > 0)
    {
        stRecvData.pData = std::shared_ptr<char[]>(new char[stRecvData.nSize]);
        if (NULL == stRecvData.pData || NULL == stRecvData.pData.get())
        {
            dlog_error("创建智能指针失败");
            return;
        }

        memcpy(stRecvData.pData.get(), pFrame->pData, stRecvData.nSize);

        stRecvData.enType = MediaDataType_E::VIDEO_DATA;
        stRecvData.stMediaParam.enPixelFormat = RK_FMT_YUV420SP;
        stRecvData.stMediaParam.nVideoWidth = nW;
        stRecvData.stMediaParam.nVideoHeight = nH;

        send_videoData(stRecvData);
    }
}

void CStreamHandler::recvDataProcess(const void *pData, int nLength)
{
    if (pData == nullptr || nLength <= 0)
    {
        dlog_error("无效的音频帧");
        return;
    }

    MediaData_S stRecvData;
    stRecvData.nSize = nLength;
    stRecvData.enType = MediaDataType_E::AUDIO_DATA;
    stRecvData.stMediaParam.enBitWidth = AUDIO_BIT_WIDTH_16; 

    stRecvData.pData = std::shared_ptr<char[]>(new char[stRecvData.nSize]);
    if (NULL == stRecvData.pData || NULL == stRecvData.pData.get())
    {
        dlog_error("创建智能指针失败");
        return;
    }

    memcpy(stRecvData.pData.get(), pData, stRecvData.nSize);

    /* 发送至已绑定的槽 */
    send_audioData(stRecvData);
}
