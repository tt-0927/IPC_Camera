#pragma once

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

class CVoiceComCaptureSource
{
public:
    static CVoiceComCaptureSource *instance();

    void push_pcm_frame(const uint8_t *pData, int nLen);
    int read_pcm_frame(char *pBuffer, unsigned int nBufferSize, int nFrameBytes);
    void clear();
private:
    CVoiceComCaptureSource() = default;

private:
    static constexpr size_t MAX_QUEUE_SIZE = 50;

    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::deque<std::vector<uint8_t>> m_frameQueue;
    std::vector<uint8_t> m_pendingFrame;
    size_t m_pendingOffset = 0;
};