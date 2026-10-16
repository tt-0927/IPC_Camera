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

    /**
     * @brief   : 写入一帧采集 PCM 数据（非阻塞，队列满时丢弃最旧帧）
     * @param    {const uint8_t} *pData：PCM 数据指针
     * @param    {int} nLen：数据长度（字节）
     * @return   无
     */
    void push_pcm_frame(const uint8_t *pData, int nLen);
    int read_pcm_frame(char *pBuffer, unsigned int nBufferSize, int nFrameBytes);
    void clear();
private:
    CVoiceComCaptureSource() = default;

private:
    /*
     * 队列上限。队列满时丢弃最旧帧，保证后续读到的是尽量新的麦克风音频。
     * 取值不宜过大：水位一旦顶到本值就会停住、不会自行回落，从而形成固定延迟
     * （延迟 ≈ 水位帧数 × AI 帧长，本平台 AI 帧长 64ms）。
     */
    static constexpr size_t MAX_QUEUE_SIZE = 10;

    /*
     * 读取侧目标水位（帧）。push 侧队列满时丢弃最旧帧，水位一旦顶到
     * MAX_QUEUE_SIZE 就会永久停住、不会自行回落，从而形成固定延迟。
     * 这里在每次读取前丢弃超出本值的陈旧帧，使积压能自愈回落。
     *
     * 水位直接决定固定延迟：延迟 ≈ 水位帧数 × AI 帧长。
     * 实测（2026-10-16）：本平台音频格式为 AAC，stream_ai.cpp 取
     * AACLC_SAMPLES_PER_FRAME = 1024 采样 @16kHz，故 AI 帧长为 64ms
     * （注意：stream_ai.h 中 EVS_SAMPLES_PER_FRAME(256)=16ms 的注释不适用于
     * AAC 路径）。另需注意：上游写入速率与 SDK 读取速率实测相等
     * （均约 15.6 帧/秒），水位不会因速率差自行下降，只能由本值决定。
     *
     * 实测记录（重要）：曾下调至 2，上机后出现大量
     * `mppAenc_getFrame failed, error code 0xA0178016`（OT_ERR_AENC_BUF_EMPTY）
     * 报错，并伴随「RTMP 等待音频超时，降级为纯视频推流」；而下调至 3 与 1
     * 均无此问题。该现象在代码路径上无法解释（本值仅作用于 read_pcm_frame 的
     * 读取裁剪，与 AENC 无调用关系），根因未定位，故 2 暂视为不可用值。
     *
     * 取值过小会导致欠载（read_pcm_frame 补静音），需观察补静音次数调整。
     */
    static constexpr size_t TARGET_QUEUE_SIZE = 3;

    std::mutex m_mutex;
    std::condition_variable m_cond;
    /* 待发送给 SDK 的采集 PCM 帧队列 */
    std::deque<std::vector<uint8_t>> m_frameQueue;
    std::vector<uint8_t> m_pendingFrame;
    size_t m_pendingOffset = 0;
};
