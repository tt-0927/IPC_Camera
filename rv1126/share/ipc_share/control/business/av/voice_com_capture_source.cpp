#include "voice_com_capture_source.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <utility>

CVoiceComCaptureSource *CVoiceComCaptureSource::instance()
{
    static CVoiceComCaptureSource s_instance;
    return &s_instance;
}

void CVoiceComCaptureSource::push_pcm_frame(const uint8_t *pData, int nLen)
{
    if (!pData || nLen <= 0)
        return;

    std::vector<uint8_t> frame(static_cast<size_t>(nLen));
    std::memcpy(frame.data(), pData, static_cast<size_t>(nLen));

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        /*
         * 对讲是实时业务，SDK/NVR 读取慢时不能让音频采集线程阻塞。
         * 队列满后丢弃最旧帧，保证后续读到的是尽量新的麦克风音频。
         */
        while (m_frameQueue.size() >= MAX_QUEUE_SIZE)
            m_frameQueue.pop_front();

        m_frameQueue.emplace_back(std::move(frame));
    }

    m_cond.notify_one();
}

int CVoiceComCaptureSource::read_pcm_frame(char *pBuffer, unsigned int nBufferSize, int nFrameBytes)
{
    if (!pBuffer || nBufferSize == 0 || nFrameBytes <= 0)
        return 0;

    if (static_cast<unsigned int>(nFrameBytes) > nBufferSize)
        return 0;

    std::unique_lock<std::mutex> lock(m_mutex);

    /*
     * VoiceComServer 的 capture_loop 会按 frameIntervalMs 定时拉帧。
     * 这里短暂等待一帧即可，超时返回 0 表示本轮没有可用麦克风数据。
     */
    m_cond.wait_for(lock, std::chrono::milliseconds(20), [this] {
        return m_pendingOffset < m_pendingFrame.size() || !m_frameQueue.empty();
    });

    const size_t targetSize = static_cast<size_t>(nFrameBytes);
    size_t copiedSize = 0;

    while (copiedSize < targetSize)
    {
        if (m_pendingOffset >= m_pendingFrame.size())
        {
            if (m_frameQueue.empty())
                break;

            m_pendingFrame = std::move(m_frameQueue.front());
            m_frameQueue.pop_front();
            m_pendingOffset = 0;
        }

        const size_t available = m_pendingFrame.size() - m_pendingOffset;
        const size_t copySize = std::min(targetSize - copiedSize, available);
        std::memcpy(pBuffer + copiedSize, m_pendingFrame.data() + m_pendingOffset, copySize);
        copiedSize += copySize;
        m_pendingOffset += copySize;

        if (m_pendingOffset >= m_pendingFrame.size())
        {
            m_pendingFrame.clear();
            m_pendingOffset = 0;
        }
    }

    if (copiedSize == 0)
        return 0;

    if (copiedSize < targetSize)
    {
        /*
         * SDK 和 NVR 约定按 PCM 固定帧长发送。
         * 当 NVR 要求的帧长大于单个 AI 采集帧时，优先拼接队列中的多帧；
         * 如果当前采集数据仍不足，再补静音，避免上层收到变长 PCM 后播放节奏抖动。
         */
        std::memset(pBuffer + copiedSize, 0, targetSize - copiedSize);
        return nFrameBytes;
    }

    return nFrameBytes;
}

void CVoiceComCaptureSource::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_frameQueue.clear();
    m_pendingFrame.clear();
    m_pendingOffset = 0;
}
