/**
 * @FilePath     : stream_performance_service.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-28 17:09:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-28 17:37:21
 * @Description  : stream 进程线程性能聚合与 /proc 全线程快照服务实现
 */

#include "stream_performance_service.h"

#include <dirent.h>
#include <pthread.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "IpcRet.h"
#include "dlog.h"
#include "thread_performance_registry.h"

namespace
{
    constexpr const char *STREAM_PERFORMANCE_CONFIG_FILE = "/opt/cam/.config/stream_performance.conf";
    constexpr int REPORT_INTERVAL_MIN_SEC = 1;
    constexpr int REPORT_INTERVAL_MAX_SEC = 60;

    struct ProcThreadSample_S
    {
        std::string strName;
        char cState{'?'};
        uint64_t u64CpuTick{0};
        uint64_t u64VoluntaryContextSwitch{0};
        uint64_t u64NonvoluntaryContextSwitch{0};
    };

    std::unordered_map<int, ProcThreadSample_S> g_mapPreviousSamples;

    bool parse_bool_value(const std::string &strValue)
    {
        return strValue == "1" || strValue == "true" || strValue == "TRUE" || strValue == "on";
    }

    const char *get_proc_thread_category(const std::string &strName)
    {
        if (strName == "get_vencStream" || strName == "get_vpssStream" || strName == "ot_ai_get" ||
            strName == "ot_aenc_get" || strName == "ot_adec_send" || strName == "ot_adec_dec")
        {
            return "MEDIA";
        }
        if (strName == "GarbageDetect" || strName == "FaceDetect" || strName == "FaceDetW")
        {
            return "AI";
        }
        if (strName.find("Onvif") == 0 || strName.find("Web") == 0 || strName.find("WS_") == 0 ||
            strName == "LibWSServerRun" || strName == "SIPRtpServer")
        {
            return "SERVICE_IO";
        }
        if (strName.find("Record") == 0 || strName.find("SD") == 0 || strName == "CapCtrlRun")
        {
            return "STORAGE_RECORD";
        }
        if (strName == "PlatformLogin" || strName == "PlatformHeart" || strName.find("Heart") == 0)
        {
            return "PLATFORM_OR_SDK";
        }
        if (strName.find("Event") == 0 || strName.find("Fill") == 0 || strName.find("Io") == 0 ||
            strName == "RegTimerthr")
        {
            return "EVENT_PERIPHERAL";
        }
        if (strName.find("ISP") == 0)
        {
            return "ISP_SDK";
        }
        return "UNKNOWN";
    }

    uint64_t get_percentile_us(const ThreadPerf_NS::MetricSnapshot_S &stMetric, uint32_t u32Percent)
    {
        if (stMetric.u64Count == 0)
        {
            return 0;
        }

        const uint64_t u64Target = (stMetric.u64Count * u32Percent + 99) / 100;
        uint64_t u64Accumulated = 0;
        for (size_t u32Bucket = 0; u32Bucket < stMetric.au64Histogram.size(); ++u32Bucket)
        {
            u64Accumulated += stMetric.au64Histogram[u32Bucket];
            if (u64Accumulated >= u64Target)
            {
                /* perf: 分桶仅保留数量级，报告上界避免把 P95/P99 低估成桶下界。 */
                return u32Bucket + 1 < stMetric.au64Histogram.size() ? (1ULL << (u32Bucket + 1)) : stMetric.u64MaxUs;
            }
        }
        return stMetric.u64MaxUs;
    }

    double to_ms(uint64_t u64Us)
    {
        return static_cast<double>(u64Us) / 1000.0;
    }

    bool read_proc_thread_sample(int nTid, ProcThreadSample_S &rstSample)
    {
        std::ostringstream ssPath;
        ssPath << "/proc/self/task/" << nTid;
        const std::string strThreadPath = ssPath.str();

        std::ifstream statFile(strThreadPath + "/stat");
        std::string strStatLine;
        if (!std::getline(statFile, strStatLine))
        {
            return false;
        }

        const size_t u32Left = strStatLine.find('(');
        const size_t u32Right = strStatLine.rfind(')');
        if (u32Left == std::string::npos || u32Right == std::string::npos || u32Right + 2 >= strStatLine.size())
        {
            return false;
        }

        rstSample.strName = strStatLine.substr(u32Left + 1, u32Right - u32Left - 1);
        std::istringstream statStream(strStatLine.substr(u32Right + 2));
        std::array<unsigned long long, 10> au64Skip{};
        unsigned long long u64UserTick = 0;
        unsigned long long u64SystemTick = 0;
        statStream >> rstSample.cState;
        for (auto &ru64Value : au64Skip)
        {
            statStream >> ru64Value;
        }
        statStream >> u64UserTick >> u64SystemTick;
        if (statStream.fail())
        {
            return false;
        }
        rstSample.u64CpuTick = u64UserTick + u64SystemTick;

        std::ifstream statusFile(strThreadPath + "/status");
        std::string strStatusLine;
        while (std::getline(statusFile, strStatusLine))
        {
            if (strStatusLine.find("voluntary_ctxt_switches:") == 0)
            {
                rstSample.u64VoluntaryContextSwitch = std::strtoull(strStatusLine.c_str() + 24, nullptr, 10);
            }
            else if (strStatusLine.find("nonvoluntary_ctxt_switches:") == 0)
            {
                rstSample.u64NonvoluntaryContextSwitch = std::strtoull(strStatusLine.c_str() + 27, nullptr, 10);
            }
        }
        return true;
    }
} // namespace

CStreamPerformanceService::~CStreamPerformanceService()
{
    deinit();
}

int CStreamPerformanceService::init()
{
    load_config();
    if (!m_bEnabled)
    {
        dlog_info("线程性能统计未启用，配置文件:%s", STREAM_PERFORMANCE_CONFIG_FILE);
        return OK;
    }

    if (m_bRunning.exchange(true))
    {
        return OK;
    }

    ThreadPerf_NS::CThreadPerformanceRegistry::instance().set_enabled(true);
    m_stReportThread = std::thread(&CStreamPerformanceService::report_loop, this);
    dlog_info("线程性能统计已启用，报告周期:%d秒，proc快照:%d，逐线程详情:%d",
              m_nReportIntervalSec,
              m_bProcThreadSnapshotEnabled ? 1 : 0,
              m_bProcThreadDetailEnabled ? 1 : 0);
    return OK;
}

int CStreamPerformanceService::deinit()
{
    ThreadPerf_NS::CThreadPerformanceRegistry::instance().set_enabled(false);
    m_bRunning.store(false);
    if (m_stReportThread.joinable())
    {
        m_stReportThread.join();
    }
    return OK;
}

void CStreamPerformanceService::load_config()
{
    m_bEnabled = false;
    m_bProcThreadSnapshotEnabled = true;
    m_bProcThreadDetailEnabled = false;
    m_nReportIntervalSec = 10;

    const char *pEnvironmentEnable = std::getenv("STREAM_PERF_ENABLE");
    if (pEnvironmentEnable)
    {
        m_bEnabled = parse_bool_value(pEnvironmentEnable);
    }

    std::ifstream configFile(STREAM_PERFORMANCE_CONFIG_FILE);
    std::string strLine;
    while (std::getline(configFile, strLine))
    {
        const size_t u32Equal = strLine.find('=');
        if (u32Equal == std::string::npos)
        {
            continue;
        }
        const std::string strKey = strLine.substr(0, u32Equal);
        const std::string strValue = strLine.substr(u32Equal + 1);
        if (strKey == "enable")
        {
            m_bEnabled = parse_bool_value(strValue);
        }
        else if (strKey == "proc_thread_snapshot_enable")
        {
            m_bProcThreadSnapshotEnabled = parse_bool_value(strValue);
        }
        else if (strKey == "proc_thread_detail_enable")
        {
            m_bProcThreadDetailEnabled = parse_bool_value(strValue);
        }
        else if (strKey == "report_interval_sec")
        {
            const int nInterval = std::atoi(strValue.c_str());
            m_nReportIntervalSec = std::max(REPORT_INTERVAL_MIN_SEC, std::min(REPORT_INTERVAL_MAX_SEC, nInterval));
        }
    }
}

void CStreamPerformanceService::report_loop()
{
    pthread_setname_np(pthread_self(), "StreamPerfRpt");
    int nElapsedSec = 0;
    while (m_bRunning.load())
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (!m_bRunning.load())
        {
            break;
        }
        if (++nElapsedSec < m_nReportIntervalSec)
        {
            continue;
        }
        nElapsedSec = 0;
        report_probe_statistics();
        if (m_bProcThreadSnapshotEnabled)
        {
            report_process_threads();
        }
    }
}

void CStreamPerformanceService::report_probe_statistics()
{
    std::array<ThreadPerf_NS::ProbeSnapshot_S, static_cast<size_t>(ThreadPerf_NS::Probe_E::TOTAL)> astSnapshots;
    const size_t u32SnapshotCount = ThreadPerf_NS::CThreadPerformanceRegistry::instance().snapshot_and_reset(
        astSnapshots.data(), astSnapshots.size());

    for (size_t u32Index = 0; u32Index < u32SnapshotCount; ++u32Index)
    {
        const ThreadPerf_NS::ProbeSnapshot_S &stSnapshot = astSnapshots[u32Index];
        if (stSnapshot.u64SuccessCount == 0 && stSnapshot.u64TimeoutCount == 0 && stSnapshot.u64ErrorCount == 0 &&
            stSnapshot.u64QueuePushCount == 0 && stSnapshot.u64QueuePopCount == 0)
        {
            continue;
        }

        const auto &stInterval = stSnapshot.astMetrics[static_cast<size_t>(ThreadPerf_NS::Metric_E::LOOP_INTERVAL)];
        const auto &stWait = stSnapshot.astMetrics[static_cast<size_t>(ThreadPerf_NS::Metric_E::WAIT_TIME)];
        const auto &stWork = stSnapshot.astMetrics[static_cast<size_t>(ThreadPerf_NS::Metric_E::WORK_TIME)];
        const auto &stLock = stSnapshot.astMetrics[static_cast<size_t>(ThreadPerf_NS::Metric_E::LOCK_WAIT_TIME)];
        const auto &stQueueWait = stSnapshot.astMetrics[static_cast<size_t>(ThreadPerf_NS::Metric_E::QUEUE_WAIT_TIME)];
        const auto &stEndToEnd = stSnapshot.astMetrics[static_cast<size_t>(ThreadPerf_NS::Metric_E::END_TO_END_TIME)];

        dlog_info("[thread-perf][%s] name=%s success=%llu timeout=%llu error=%llu miss=%llu "
                  "interval_p99=%.3fms wait_p99=%.3fms work_p95=%.3fms work_p99=%.3fms lock_p99=%.3fms "
                  "queue_wait_p99=%.3fms e2e_p99=%.3fms qpush=%llu qpop=%llu qreplace=%llu qdrop=%llu qdepth_max=%u",
                  stSnapshot.stDescriptor.pCategory,
                  stSnapshot.stDescriptor.pName,
                  static_cast<unsigned long long>(stSnapshot.u64SuccessCount),
                  static_cast<unsigned long long>(stSnapshot.u64TimeoutCount),
                  static_cast<unsigned long long>(stSnapshot.u64ErrorCount),
                  static_cast<unsigned long long>(stSnapshot.u64DeadlineMissCount),
                  to_ms(get_percentile_us(stInterval, 99)),
                  to_ms(get_percentile_us(stWait, 99)),
                  to_ms(get_percentile_us(stWork, 95)),
                  to_ms(get_percentile_us(stWork, 99)),
                  to_ms(get_percentile_us(stLock, 99)),
                  to_ms(get_percentile_us(stQueueWait, 99)),
                  to_ms(get_percentile_us(stEndToEnd, 99)),
                  static_cast<unsigned long long>(stSnapshot.u64QueuePushCount),
                  static_cast<unsigned long long>(stSnapshot.u64QueuePopCount),
                  static_cast<unsigned long long>(stSnapshot.u64QueueReplaceCount),
                  static_cast<unsigned long long>(stSnapshot.u64QueueDropCount),
                  stSnapshot.u32QueueDepthMax);
    }
}

void CStreamPerformanceService::report_process_threads()
{
    DIR *pTaskDir = opendir("/proc/self/task");
    if (!pTaskDir)
    {
        dlog_warn("[thread-perf][proc] 打开/proc/self/task失败");
        return;
    }

    const long nClockTick = sysconf(_SC_CLK_TCK);
    std::unordered_map<int, ProcThreadSample_S> mapCurrentSamples;
    uint32_t u32RunningCount = 0;
    uint32_t u32SleepingCount = 0;
    uint32_t u32OtherStateCount = 0;
    struct dirent *pstEntry = nullptr;
    while ((pstEntry = readdir(pTaskDir)) != nullptr)
    {
        const int nTid = std::atoi(pstEntry->d_name);
        if (nTid <= 0)
        {
            continue;
        }
        ProcThreadSample_S stSample;
        if (!read_proc_thread_sample(nTid, stSample))
        {
            continue;
        }
        if (stSample.cState == 'R')
        {
            ++u32RunningCount;
        }
        else if (stSample.cState == 'S' || stSample.cState == 'D')
        {
            ++u32SleepingCount;
        }
        else
        {
            ++u32OtherStateCount;
        }
        mapCurrentSamples.emplace(nTid, stSample);
        const auto itPrevious = g_mapPreviousSamples.find(nTid);
        if (itPrevious == g_mapPreviousSamples.end())
        {
            dlog_info("[thread-perf][proc] tid=%d name=%s category=%s state=%c new_thread=1",
                      nTid, stSample.strName.c_str(), get_proc_thread_category(stSample.strName), stSample.cState);
            continue;
        }

        const ProcThreadSample_S &stPrevious = itPrevious->second;
        const uint64_t u64CpuDeltaTick = stSample.u64CpuTick - stPrevious.u64CpuTick;
        const uint64_t u64VoluntaryDelta = stSample.u64VoluntaryContextSwitch - stPrevious.u64VoluntaryContextSwitch;
        const uint64_t u64NonvoluntaryDelta = stSample.u64NonvoluntaryContextSwitch - stPrevious.u64NonvoluntaryContextSwitch;
        const double dCpuPercent = nClockTick > 0 ? (static_cast<double>(u64CpuDeltaTick) * 100.0 /
                                                       (static_cast<double>(nClockTick) * m_nReportIntervalSec)) : 0.0;
        if (m_bProcThreadDetailEnabled || dCpuPercent > 0.1 || u64NonvoluntaryDelta > 0)
        {
            dlog_info("[thread-perf][proc] tid=%d name=%s category=%s state=%c cpu=%.2f%% vcsw=%llu nvcsw=%llu",
                      nTid,
                      stSample.strName.c_str(),
                      get_proc_thread_category(stSample.strName),
                      stSample.cState,
                      dCpuPercent,
                      static_cast<unsigned long long>(u64VoluntaryDelta),
                      static_cast<unsigned long long>(u64NonvoluntaryDelta));
        }
    }
    closedir(pTaskDir);
    dlog_info("[thread-perf][proc-summary] total=%u running=%u sleep_or_io=%u other=%u",
              static_cast<uint32_t>(mapCurrentSamples.size()), u32RunningCount, u32SleepingCount, u32OtherStateCount);
    g_mapPreviousSamples.swap(mapCurrentSamples);
}
