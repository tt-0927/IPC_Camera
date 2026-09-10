/**
 * @FilePath     : thread_performance_registry.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-28 17:09:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-28 17:23:19
 * @Description  : 线程轮询与异步任务的低开销性能统计注册表实现
 */

#include "thread_performance_registry.h"

#include <ctime>

namespace ThreadPerf_NS
{
    CThreadPerformanceRegistry::MetricData_S::MetricData_S()
    {
        for (auto &ru64Bucket : au64Histogram)
        {
            ru64Bucket.store(0, std::memory_order_relaxed);
        }
    }

    CThreadPerformanceRegistry &CThreadPerformanceRegistry::instance()
    {
        static CThreadPerformanceRegistry stInstance;
        return stInstance;
    }

    CThreadPerformanceRegistry::CThreadPerformanceRegistry()
        : m_astDescriptors({{
            {"venc_main", "MEDIA_REALTIME", 33, 33, true},
            {"venc_sub", "MEDIA_REALTIME", 33, 33, true},
            {"venc_jpeg", "MEDIA_REALTIME", 500, 500, false},
            {"vpss_ai", "AI_INFERENCE", 333, 333, false},
            {"audio_capture", "MEDIA_REALTIME", 64, 64, true},
            {"audio_aenc", "MEDIA_REALTIME", 64, 64, true},
            {"garbage_detect", "AI_INFERENCE", 0, 500, false},
            {"face_detect", "AI_INFERENCE", 0, 500, false},
            {"face_detect_worker", "AI_INFERENCE", 0, 5000, false},
            {"face_compare", "AI_INFERENCE", 0, 5000, false},
            {"platform_auto_login", "PLATFORM_NETWORK", 0, 3000, false},
            {"platform_heartbeat", "PLATFORM_NETWORK", 30000, 3000, false},
        }})
    {
    }

    void CThreadPerformanceRegistry::set_enabled(bool bEnable)
    {
        m_bEnabled.store(bEnable, std::memory_order_release);
    }

    bool CThreadPerformanceRegistry::is_enabled() const
    {
        return m_bEnabled.load(std::memory_order_acquire);
    }

    uint64_t CThreadPerformanceRegistry::now_us()
    {
        struct timespec stTime;
        clock_gettime(CLOCK_MONOTONIC, &stTime);
        return static_cast<uint64_t>(stTime.tv_sec) * 1000000ULL + static_cast<uint64_t>(stTime.tv_nsec) / 1000ULL;
    }

    void CThreadPerformanceRegistry::record_elapsed(Probe_E enProbe, Metric_E enMetric, uint64_t u64ElapsedUs)
    {
        if (!is_enabled())
        {
            return;
        }

        MetricData_S &rstMetric = m_astProbeData[get_probe_index(enProbe)].astMetrics[static_cast<size_t>(enMetric)];
        rstMetric.u64Count.fetch_add(1, std::memory_order_relaxed);
        rstMetric.u64TotalUs.fetch_add(u64ElapsedUs, std::memory_order_relaxed);
        update_max(rstMetric.u64MaxUs, u64ElapsedUs);
        rstMetric.au64Histogram[get_histogram_bucket(u64ElapsedUs)].fetch_add(1, std::memory_order_relaxed);
    }

    void CThreadPerformanceRegistry::record_result(Probe_E enProbe, bool bSuccess, bool bTimeout)
    {
        if (!is_enabled())
        {
            return;
        }

        ProbeData_S &rstProbe = m_astProbeData[get_probe_index(enProbe)];
        if (bSuccess)
        {
            rstProbe.u64SuccessCount.fetch_add(1, std::memory_order_relaxed);
        }
        else if (bTimeout)
        {
            rstProbe.u64TimeoutCount.fetch_add(1, std::memory_order_relaxed);
        }
        else
        {
            rstProbe.u64ErrorCount.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void CThreadPerformanceRegistry::record_deadline_miss(Probe_E enProbe)
    {
        if (is_enabled())
        {
            m_astProbeData[get_probe_index(enProbe)].u64DeadlineMissCount.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void CThreadPerformanceRegistry::record_queue_event(Probe_E enProbe, QueueEvent_E enEvent, uint32_t u32Depth)
    {
        if (!is_enabled())
        {
            return;
        }

        ProbeData_S &rstProbe = m_astProbeData[get_probe_index(enProbe)];
        switch (enEvent)
        {
        case QueueEvent_E::PUSH:
            rstProbe.u64QueuePushCount.fetch_add(1, std::memory_order_relaxed);
            break;
        case QueueEvent_E::POP:
            rstProbe.u64QueuePopCount.fetch_add(1, std::memory_order_relaxed);
            break;
        case QueueEvent_E::REPLACE:
            rstProbe.u64QueueReplaceCount.fetch_add(1, std::memory_order_relaxed);
            break;
        case QueueEvent_E::DROP:
            rstProbe.u64QueueDropCount.fetch_add(1, std::memory_order_relaxed);
            break;
        }
        update_max(rstProbe.u32QueueDepthMax, u32Depth);
    }

    size_t CThreadPerformanceRegistry::snapshot_and_reset(ProbeSnapshot_S *pstSnapshots, size_t u32SnapshotCount)
    {
        if (!pstSnapshots || u32SnapshotCount == 0)
        {
            return 0;
        }

        const size_t u32Count = u32SnapshotCount < m_astProbeData.size() ? u32SnapshotCount : m_astProbeData.size();
        for (size_t u32Probe = 0; u32Probe < u32Count; ++u32Probe)
        {
            ProbeData_S &rstSource = m_astProbeData[u32Probe];
            ProbeSnapshot_S &rstTarget = pstSnapshots[u32Probe];
            rstTarget.stDescriptor = m_astDescriptors[u32Probe];
            rstTarget.u64SuccessCount = rstSource.u64SuccessCount.exchange(0, std::memory_order_relaxed);
            rstTarget.u64TimeoutCount = rstSource.u64TimeoutCount.exchange(0, std::memory_order_relaxed);
            rstTarget.u64ErrorCount = rstSource.u64ErrorCount.exchange(0, std::memory_order_relaxed);
            rstTarget.u64DeadlineMissCount = rstSource.u64DeadlineMissCount.exchange(0, std::memory_order_relaxed);
            rstTarget.u64QueuePushCount = rstSource.u64QueuePushCount.exchange(0, std::memory_order_relaxed);
            rstTarget.u64QueuePopCount = rstSource.u64QueuePopCount.exchange(0, std::memory_order_relaxed);
            rstTarget.u64QueueReplaceCount = rstSource.u64QueueReplaceCount.exchange(0, std::memory_order_relaxed);
            rstTarget.u64QueueDropCount = rstSource.u64QueueDropCount.exchange(0, std::memory_order_relaxed);
            rstTarget.u32QueueDepthMax = rstSource.u32QueueDepthMax.exchange(0, std::memory_order_relaxed);

            for (size_t u32Metric = 0; u32Metric < rstSource.astMetrics.size(); ++u32Metric)
            {
                MetricData_S &rstMetricSource = rstSource.astMetrics[u32Metric];
                MetricSnapshot_S &rstMetricTarget = rstTarget.astMetrics[u32Metric];
                rstMetricTarget.u64Count = rstMetricSource.u64Count.exchange(0, std::memory_order_relaxed);
                rstMetricTarget.u64TotalUs = rstMetricSource.u64TotalUs.exchange(0, std::memory_order_relaxed);
                rstMetricTarget.u64MaxUs = rstMetricSource.u64MaxUs.exchange(0, std::memory_order_relaxed);
                for (size_t u32Bucket = 0; u32Bucket < HISTOGRAM_BUCKET_TOTAL; ++u32Bucket)
                {
                    rstMetricTarget.au64Histogram[u32Bucket] = rstMetricSource.au64Histogram[u32Bucket].exchange(0, std::memory_order_relaxed);
                }
            }
        }
        return u32Count;
    }

    const ProbeDescriptor_S &CThreadPerformanceRegistry::get_descriptor(Probe_E enProbe) const
    {
        return m_astDescriptors[get_probe_index(enProbe)];
    }

    size_t CThreadPerformanceRegistry::get_histogram_bucket(uint64_t u64ElapsedUs)
    {
        size_t u32Bucket = 0;
        while (u64ElapsedUs > 1 && u32Bucket + 1 < HISTOGRAM_BUCKET_TOTAL)
        {
            u64ElapsedUs >>= 1;
            ++u32Bucket;
        }
        return u32Bucket;
    }

    void CThreadPerformanceRegistry::update_max(std::atomic<uint64_t> &ru64Max, uint64_t u64Value)
    {
        uint64_t u64Current = ru64Max.load(std::memory_order_relaxed);
        while (u64Current < u64Value && !ru64Max.compare_exchange_weak(u64Current, u64Value, std::memory_order_relaxed))
        {
        }
    }

    void CThreadPerformanceRegistry::update_max(std::atomic<uint32_t> &ru32Max, uint32_t u32Value)
    {
        uint32_t u32Current = ru32Max.load(std::memory_order_relaxed);
        while (u32Current < u32Value && !ru32Max.compare_exchange_weak(u32Current, u32Value, std::memory_order_relaxed))
        {
        }
    }

    size_t CThreadPerformanceRegistry::get_probe_index(Probe_E enProbe)
    {
        return static_cast<size_t>(enProbe);
    }
} // namespace ThreadPerf_NS
