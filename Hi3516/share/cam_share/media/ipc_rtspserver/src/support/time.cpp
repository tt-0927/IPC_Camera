/**
 * @FilePath     : time.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 时间与随机数实现
 */

#include "support/time.h"

#include <cerrno>
#include <ctime>
#include <unistd.h>

#if defined(__linux__)
#include <sys/random.h>
#endif

namespace ipc_rtsp
{
namespace
{
/** NTP 纪元（1900-01-01）与 Unix 纪元的秒差。 */
constexpr std::uint64_t kNtpEpochOffset = 2208988800ULL;
} // namespace

namespace detail
{

std::int64_t NowMonotonicUs()
{
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<std::int64_t>(ts.tv_sec) * 1000000 + ts.tv_nsec / 1000;
}

std::int64_t NowMonotonicMs()
{
    return NowMonotonicUs() / 1000;
}

void NowRealtime(std::uint32_t *sec, std::uint32_t *nsec)
{
    timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);
    if (sec != nullptr)
    {
        *sec = static_cast<std::uint32_t>(ts.tv_sec);
    }
    if (nsec != nullptr)
    {
        *nsec = static_cast<std::uint32_t>(ts.tv_nsec);
    }
}

std::uint64_t RandomU64()
{
    std::uint64_t value = 0;
#if defined(__linux__)
    const ssize_t n = getrandom(&value, sizeof value, 0);
    if (n == static_cast<ssize_t>(sizeof value))
    {
        return value;
    }
#endif
    /* 退化路径：仅用于极早期内核/受限环境，足以保证进程内唯一性。 */
    const std::int64_t now_us = NowMonotonicUs();
    value = static_cast<std::uint64_t>(now_us) * 0x9E3779B97F4A7C15ULL;
    value ^= static_cast<std::uint64_t>(getpid()) << 32;
    return value;
}

std::uint32_t RandomU32()
{
    return static_cast<std::uint32_t>(RandomU64() >> 16);
}

void SampleRealtimeNtp(std::uint32_t *ntp_sec, std::uint32_t *ntp_frac, std::int64_t *mono_us)
{
    timespec rt{};
    clock_gettime(CLOCK_REALTIME, &rt);

    /* 下面的 MONOTONIC 与上面的 REALTIME 是两次独立采样，非严格同一时刻；
     * 间隔为微秒级，对 RTCP SR 的 NTP/RTP 映射足够。 */
    const timespec mt = [&]()
    {
        timespec tmp{};
        clock_gettime(CLOCK_MONOTONIC, &tmp);
        return tmp;
    }();

    const std::uint64_t unix_sec = static_cast<std::uint64_t>(rt.tv_sec);
    if (ntp_sec != nullptr)
    {
        *ntp_sec = static_cast<std::uint32_t>(unix_sec + kNtpEpochOffset);
    }
    if (ntp_frac != nullptr)
    {
        /* 2^32 / 1e9 ≈ 4.294967296 */
        *ntp_frac = static_cast<std::uint32_t>(static_cast<double>(rt.tv_nsec) * 4.294967296);
    }
    if (mono_us != nullptr)
    {
        *mono_us = static_cast<std::int64_t>(mt.tv_sec) * 1000000 + mt.tv_nsec / 1000;
    }
}

} // namespace detail
} // namespace ipc_rtsp
