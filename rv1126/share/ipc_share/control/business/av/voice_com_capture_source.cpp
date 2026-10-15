#include "voice_com_capture_source.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <utility>

#include "dlog.h"

namespace
{
/* 诊断窗口长度（毫秒） */
constexpr uint64_t kVoiceComDiagWindowMs = 1000;

/*
 * 上行采集队列诊断计数。push_pcm_frame 与 read_pcm_frame 均在 m_mutex
 * 保护下更新，故这些变量无需额外同步；窗口起点与快照只在 read_pcm_frame
 * 内维护，打印在锁外执行，避免持锁做日志 IO。
 */
uint64_t gs_nDiagWindowStartMs = 0;
uint64_t gs_nDiagPushFrames = 0;
uint64_t gs_nDiagReadFrames = 0;
uint64_t gs_nDiagDropFrames = 0;
uint64_t gs_nDiagTrimFrames = 0;
uint64_t gs_nDiagSilenceFrames = 0;
uint64_t gs_nDiagDepthSum = 0;
uint64_t gs_nDiagDepthMax = 0;
uint64_t gs_nDiagAgeSumMs = 0;
uint64_t gs_nDiagAgeMaxMs = 0;

/**
 * @brief   : 取单调时钟毫秒值，用于上行采集队列诊断。
 * @return   {uint64_t} 自 steady_clock 起点的毫秒数。
 */
uint64_t voice_com_diag_now_ms()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
}

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
        {
            m_frameQueue.pop_front();
            gs_nDiagDropFrames++;
        }

        /* 记录采集时刻，随帧入队，供读取侧统计端到端时长 */
        m_frameQueue.emplace_back(voice_com_diag_now_ms(), std::move(frame));
        gs_nDiagPushFrames++;
    }

    m_cond.notify_one();
}

int CVoiceComCaptureSource::read_pcm_frame(char *pBuffer, unsigned int nBufferSize, int nFrameBytes)
{
    if (!pBuffer || nBufferSize == 0 || nFrameBytes <= 0)
        return 0;

    if (static_cast<unsigned int>(nFrameBytes) > nBufferSize)
        return 0;

    int nResult = 0;

    /*
     * 诊断窗口快照。仅在窗口到期时填充，供出锁后打印，避免持锁做日志 IO。
     */
    bool bNeedDiagDump = false;
    uint64_t nDiagWindowMs = 0;
    uint64_t nDiagPush = 0;
    uint64_t nDiagRead = 0;
    uint64_t nDiagDrop = 0;
    uint64_t nDiagTrim = 0;
    uint64_t nDiagSilence = 0;
    uint64_t nDiagDepthAvg = 0;
    uint64_t nDiagDepthMax = 0;
    uint64_t nDiagAgeAvg = 0;
    uint64_t nDiagAgeMax = 0;

    {
        std::unique_lock<std::mutex> lock(m_mutex);

        /*
         * VoiceComServer 的 capture_loop 会按 frameIntervalMs 定时拉帧。
         * 这里短暂等待一帧即可，超时返回 0 表示本轮没有可用麦克风数据。
         */
        m_cond.wait_for(lock, std::chrono::milliseconds(20), [this] {
            return m_pendingOffset < m_pendingFrame.size() || !m_frameQueue.empty();
        });

        /*
         * 水位压制：push 侧队列满时丢弃最旧帧，水位一旦顶到 MAX_QUEUE_SIZE
         * 就会永久停住、不会自行回落，从而形成固定延迟。这里在每次读取前
         * 丢弃超出目标水位的陈旧帧，使积压能自愈回落。
         */
        while (m_frameQueue.size() > TARGET_QUEUE_SIZE)
        {
            m_frameQueue.pop_front();
            gs_nDiagTrimFrames++;
        }

        const size_t targetSize = static_cast<size_t>(nFrameBytes);
        size_t copiedSize = 0;

        while (copiedSize < targetSize)
        {
            if (m_pendingOffset >= m_pendingFrame.size())
            {
                if (m_frameQueue.empty())
                    break;

                /* 队列元素为 pair<采集时刻, PCM 数据>，两者一并取出 */
                m_pendingFrame = std::move(m_frameQueue.front().second);
                m_pendingFrameCaptureMs = m_frameQueue.front().first;
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
        {
            nResult = 0;
        }
        else if (copiedSize < targetSize)
        {
            /*
             * SDK 和 NVR 约定按 PCM 固定帧长发送。
             * 当 NVR 要求的帧长大于单个 AI 采集帧时，优先拼接队列中的多帧；
             * 如果当前采集数据仍不足，再补静音，避免上层收到变长 PCM 后播放节奏抖动。
             */
            std::memset(pBuffer + copiedSize, 0, targetSize - copiedSize);
            gs_nDiagSilenceFrames++;
            nResult = nFrameBytes;
        }
        else
        {
            nResult = nFrameBytes;
        }

        /* 统计累计：与 push 同受 m_mutex 保护，无需额外同步 */
        if (nResult > 0)
        {
            gs_nDiagReadFrames++;
            const uint64_t nDepth = static_cast<uint64_t>(m_frameQueue.size());
            gs_nDiagDepthSum += nDepth;
            if (nDepth > gs_nDiagDepthMax)
            {
                gs_nDiagDepthMax = nDepth;
            }

            /*
             * 采集时延 = 本帧写入队列 → 被读出交付 SDK 的时长，即
             * 「从麦克风采集到即将发送」的端到端耗时（不含 SDK 内部发送开销）。
             */
            const uint64_t nAgeMs = voice_com_diag_now_ms() - m_pendingFrameCaptureMs;
            gs_nDiagAgeSumMs += nAgeMs;
            if (nAgeMs > gs_nDiagAgeMaxMs)
            {
                gs_nDiagAgeMaxMs = nAgeMs;
            }
        }

        const uint64_t nNowMs = voice_com_diag_now_ms();
        if (gs_nDiagWindowStartMs == 0)
        {
            gs_nDiagWindowStartMs = nNowMs;
        }
        else if (nNowMs - gs_nDiagWindowStartMs >= kVoiceComDiagWindowMs)
        {
            nDiagWindowMs = nNowMs - gs_nDiagWindowStartMs;
            nDiagPush = gs_nDiagPushFrames;
            nDiagRead = gs_nDiagReadFrames;
            nDiagDrop = gs_nDiagDropFrames;
            nDiagTrim = gs_nDiagTrimFrames;
            nDiagSilence = gs_nDiagSilenceFrames;
            nDiagDepthAvg = (nDiagRead > 0) ? (gs_nDiagDepthSum / nDiagRead) : 0;
            nDiagDepthMax = gs_nDiagDepthMax;
            nDiagAgeAvg = (nDiagRead > 0) ? (gs_nDiagAgeSumMs / nDiagRead) : 0;
            nDiagAgeMax = gs_nDiagAgeMaxMs;
            bNeedDiagDump = true;

            gs_nDiagWindowStartMs = nNowMs;
            gs_nDiagPushFrames = 0;
            gs_nDiagReadFrames = 0;
            gs_nDiagDropFrames = 0;
            gs_nDiagTrimFrames = 0;
            gs_nDiagSilenceFrames = 0;
            gs_nDiagDepthSum = 0;
            gs_nDiagDepthMax = 0;
            gs_nDiagAgeSumMs = 0;
            gs_nDiagAgeMaxMs = 0;
        }
    }

    if (bNeedDiagDump)
    {
        dlog_info("VoiceCom上行诊断: 窗口%llums 写%llu帧 读%llu次 队列深度(均%llu/峰%llu) 满丢%llu 压制%llu 补静音%llu 采集时延(均%llums/峰%llums)",
                  static_cast<unsigned long long>(nDiagWindowMs),
                  static_cast<unsigned long long>(nDiagPush),
                  static_cast<unsigned long long>(nDiagRead),
                  static_cast<unsigned long long>(nDiagDepthAvg),
                  static_cast<unsigned long long>(nDiagDepthMax),
                  static_cast<unsigned long long>(nDiagDrop),
                  static_cast<unsigned long long>(nDiagTrim),
                  static_cast<unsigned long long>(nDiagSilence),
                  static_cast<unsigned long long>(nDiagAgeAvg),
                  static_cast<unsigned long long>(nDiagAgeMax));
    }

    return nResult;
}

void CVoiceComCaptureSource::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_frameQueue.clear();
    m_pendingFrame.clear();
    m_pendingFrameCaptureMs = 0;
    m_pendingOffset = 0;
}
