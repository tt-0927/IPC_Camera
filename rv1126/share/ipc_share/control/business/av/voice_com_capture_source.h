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
    /*
     * 队列上限。原值 50 偏大：队列满时丢弃最旧帧，会使积压稳定停在 50 帧，
     * 按 AI 帧长 16ms/40ms 折算即 800ms/2s 的固定延迟，且不会自行回落。
     * 收到 10 帧（160ms/400ms）后即开始追帧，既保留"读到的尽量是新音频"的
     * 设计意图，又不会累积出秒级延迟。
     */
    static constexpr size_t MAX_QUEUE_SIZE = 10;

    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::deque<std::vector<uint8_t>> m_frameQueue;
    std::vector<uint8_t> m_pendingFrame;
    size_t m_pendingOffset = 0;
};