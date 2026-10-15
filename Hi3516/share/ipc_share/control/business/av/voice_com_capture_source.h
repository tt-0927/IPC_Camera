#pragma once

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <utility>
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

    /*
     * 读取侧目标水位（帧）。push 侧队列满时丢弃最旧帧，水位一旦顶到
     * MAX_QUEUE_SIZE 就会永久停住、不会自行回落，从而形成固定延迟。
     * 这里在每次读取前丢弃超出本值的陈旧帧，使积压能自愈回落。
     * 4 帧 × AI 帧长 16ms（stream_ai.h 的 EVS_SAMPLES_PER_FRAME）= 64ms。
     * 取值过小会导致欠载（read_pcm_frame 补静音），需结合
     * "VoiceCom上行诊断" 日志中的补静音次数调整。
     */
    static constexpr size_t TARGET_QUEUE_SIZE = 4;

    std::mutex m_mutex;
    std::condition_variable m_cond;
    /*
     * 队列元素为 pair<采集时刻毫秒, PCM 数据>。采集时刻由 push_pcm_frame 写入，
     * read_pcm_frame 读出后用于统计「从采集到交付 SDK」的时长
     * （见 "VoiceCom上行诊断" 日志中的采集时延）。
     */
    std::deque<std::pair<uint64_t, std::vector<uint8_t>>> m_frameQueue;
    std::vector<uint8_t> m_pendingFrame;
    /* 当前 m_pendingFrame 所属帧的采集时刻 */
    uint64_t m_pendingFrameCaptureMs = 0;
    size_t m_pendingOffset = 0;
};