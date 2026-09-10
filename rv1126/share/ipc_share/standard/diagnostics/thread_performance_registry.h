/**
 * @FilePath     : thread_performance_registry.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-28 17:09:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-28 17:23:19
 * @Description  : 线程轮询与异步任务的低开销性能统计注册表
 */
#pragma once

#include <cstddef>

#include <array>
#include <atomic>
#include <cstdint>

namespace ThreadPerf_NS
{
    constexpr size_t HISTOGRAM_BUCKET_TOTAL = 32;

    enum class Probe_E : uint8_t
    {
        VENC_MAIN = 0,
        VENC_SUB,
        VENC_JPEG,
        VPSS_AI,
        AUDIO_CAPTURE,
        AUDIO_AENC,
        GARBAGE_DETECT,
        FACE_DETECT,
        FACE_DETECT_WORKER,
        FACE_COMPARE,
        PLATFORM_AUTO_LOGIN,
        PLATFORM_HEARTBEAT,
        TOTAL,
    };

    enum class Metric_E : uint8_t
    {
        LOOP_INTERVAL = 0,
        WAIT_TIME,
        WORK_TIME,
        LOCK_WAIT_TIME,
        QUEUE_WAIT_TIME,
        END_TO_END_TIME,
        TOTAL,
    };

    enum class QueueEvent_E : uint8_t
    {
        PUSH = 0,
        POP,
        REPLACE,
        DROP,
    };

    struct ProbeDescriptor_S
    {
        const char *pName;
        const char *pCategory;
        uint32_t u32ExpectedPeriodMs;
        uint32_t u32DeadlineMs;
        bool bRealtime;
    };

    struct MetricSnapshot_S
    {
        uint64_t u64Count;
        uint64_t u64TotalUs;
        uint64_t u64MaxUs;
        std::array<uint64_t, HISTOGRAM_BUCKET_TOTAL> au64Histogram;
    };

    struct ProbeSnapshot_S
    {
        ProbeDescriptor_S stDescriptor;
        std::array<MetricSnapshot_S, static_cast<size_t>(Metric_E::TOTAL)> astMetrics;
        uint64_t u64SuccessCount;
        uint64_t u64TimeoutCount;
        uint64_t u64ErrorCount;
        uint64_t u64DeadlineMissCount;
        uint64_t u64QueuePushCount;
        uint64_t u64QueuePopCount;
        uint64_t u64QueueReplaceCount;
        uint64_t u64QueueDropCount;
        uint32_t u32QueueDepthMax;
    };

    /**
     * @brief   : 提供固定槽位、无锁写入的线程性能统计服务
     * @param    {无}
     * @return   {无}
     * @note    : 热路径仅在运行时开关开启时读取单调时钟并执行原子累加，禁止写日志。
     */
    class CThreadPerformanceRegistry
    {
    public:
        static CThreadPerformanceRegistry &instance();

        /**
         * @brief   : 设置性能统计开关
         * @param    {bool} bEnable：true 开启统计，false 关闭统计
         * @return   {void}
         */
        void set_enabled(bool bEnable);

        /**
         * @brief   : 获取性能统计是否已开启
         * @param    {无}
         * @return   {bool} true：已开启，false：已关闭
         */
        bool is_enabled() const;

        /**
         * @brief   : 获取单调时钟微秒值
         * @param    {无}
         * @return   {uint64_t} 自进程启动以来的单调微秒时间
         */
        static uint64_t now_us();

        /**
         * @brief   : 记录一个耗时样本
         * @param    {Probe_E} enProbe：统计对象
         * @param    {Metric_E} enMetric：耗时指标类型
         * @param    {uint64_t} u64ElapsedUs：耗时，单位微秒
         * @return   {void}
         */
        void record_elapsed(Probe_E enProbe, Metric_E enMetric, uint64_t u64ElapsedUs);

        /**
         * @brief   : 记录循环成功、超时或错误结果
         * @param    {Probe_E} enProbe：统计对象
         * @param    {bool} bSuccess：本轮是否成功
         * @param    {bool} bTimeout：本轮是否因等待超时返回
         * @return   {void}
         */
        void record_result(Probe_E enProbe, bool bSuccess, bool bTimeout);

        /**
         * @brief   : 记录实时线程 deadline miss
         * @param    {Probe_E} enProbe：统计对象
         * @return   {void}
         */
        void record_deadline_miss(Probe_E enProbe);

        /**
         * @brief   : 记录队列操作和当前深度
         * @param    {Probe_E} enProbe：统计对象
         * @param    {QueueEvent_E} enEvent：队列事件
         * @param    {uint32_t} u32Depth：操作后的队列深度
         * @return   {void}
         */
        void record_queue_event(Probe_E enProbe, QueueEvent_E enEvent, uint32_t u32Depth);

        /**
         * @brief   : 读取并清空一个统计窗口
         * @param    {ProbeSnapshot_S} *pstSnapshots：输出快照数组
         * @param    {size_t} u32SnapshotCount：输出数组容量
         * @return   {size_t} 实际写入的统计对象数量
         * @note    : 仅由低频报告线程调用；与热路径并发时允许极少量样本落入相邻窗口。
         */
        size_t snapshot_and_reset(ProbeSnapshot_S *pstSnapshots, size_t u32SnapshotCount);

        /**
         * @brief   : 获取指定统计对象的描述信息
         * @param    {Probe_E} enProbe：统计对象
         * @return   {const ProbeDescriptor_S &} 静态描述信息
         */
        const ProbeDescriptor_S &get_descriptor(Probe_E enProbe) const;

    private:
        CThreadPerformanceRegistry();
        CThreadPerformanceRegistry(const CThreadPerformanceRegistry &) = delete;
        CThreadPerformanceRegistry &operator=(const CThreadPerformanceRegistry &) = delete;

        struct MetricData_S
        {
            std::atomic<uint64_t> u64Count{0};
            std::atomic<uint64_t> u64TotalUs{0};
            std::atomic<uint64_t> u64MaxUs{0};
            std::array<std::atomic<uint64_t>, HISTOGRAM_BUCKET_TOTAL> au64Histogram;

            MetricData_S();
        };

        struct ProbeData_S
        {
            std::array<MetricData_S, static_cast<size_t>(Metric_E::TOTAL)> astMetrics;
            std::atomic<uint64_t> u64SuccessCount{0};
            std::atomic<uint64_t> u64TimeoutCount{0};
            std::atomic<uint64_t> u64ErrorCount{0};
            std::atomic<uint64_t> u64DeadlineMissCount{0};
            std::atomic<uint64_t> u64QueuePushCount{0};
            std::atomic<uint64_t> u64QueuePopCount{0};
            std::atomic<uint64_t> u64QueueReplaceCount{0};
            std::atomic<uint64_t> u64QueueDropCount{0};
            std::atomic<uint32_t> u32QueueDepthMax{0};
        };

        static size_t get_histogram_bucket(uint64_t u64ElapsedUs);
        static void update_max(std::atomic<uint64_t> &ru64Max, uint64_t u64Value);
        static void update_max(std::atomic<uint32_t> &ru32Max, uint32_t u32Value);
        static size_t get_probe_index(Probe_E enProbe);

    private:
        std::atomic<bool> m_bEnabled{false};
        std::array<ProbeData_S, static_cast<size_t>(Probe_E::TOTAL)> m_astProbeData;
        std::array<ProbeDescriptor_S, static_cast<size_t>(Probe_E::TOTAL)> m_astDescriptors;
    };
} // namespace ThreadPerf_NS
