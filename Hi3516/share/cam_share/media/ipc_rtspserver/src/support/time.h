/**
 * @FilePath     : time.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 时间与随机数工具
 */
/*
 * 时钟使用约定（与 RTCP/RTP 映射一致）：
 * - 媒体推进、超时、时长统一用 CLOCK_MONOTONIC；
 * - 只有 RTCP SR 的 NTP 字段用 CLOCK_REALTIME。
 */
#pragma once

#include <cstdint>

namespace ipc_rtsp
{
namespace detail
{

/**
 * 单调时钟（CLOCK_MONOTONIC），微秒。
 *
 * @return 微秒级单调时间戳；仅用于时长/超时计算，不与墙钟对齐
 */
std::int64_t NowMonotonicUs();

/**
 * 单调时钟（CLOCK_MONOTONIC），毫秒（用于超时比较）。
 *
 * @return 毫秒级单调时间戳
 */
std::int64_t NowMonotonicMs();

/**
 * 实时时钟（CLOCK_REALTIME），秒 + 纳秒（用于 RTCP NTP 映射）。
 *
 * @param[out] sec  Unix epoch 秒（截断为 32 位）；可为 nullptr
 * @param[out] nsec 纳秒部分，范围 [0, 1e9)；可为 nullptr
 * @return {void} 无返回值，经出参返回
 */
void NowRealtime(std::uint32_t *sec, std::uint32_t *nsec);

/**
 * 随机 64 位（优先 getrandom，失败退化为时间+pid 组合）。
 *
 * @return 64 位随机值；退化路径仅保证进程内唯一性，非密码学安全
 */
std::uint64_t RandomU64();

/**
 * 随机 32 位。
 *
 * @return 32 位随机值（取自 RandomU64，丢弃低 16 位）
 */
std::uint32_t RandomU32();

/**
 * 采样一个 (monotonic_us, realtime) 对应的 NTP 时间戳。
 *
 * @param[out] ntp_sec  NTP 秒（1900 起算）
 * @param[out] ntp_frac NTP 小数部分
 * @param[out] mono_us  对应的单调微秒，用于把 RTP timestamp 与 NTP 对齐
 * @return {void} 无返回值，经出参返回
 * @note REALTIME 与 MONOTONIC 为两次独立采样，非严格同一时刻（微秒级误差）。
 */
void SampleRealtimeNtp(std::uint32_t *ntp_sec, std::uint32_t *ntp_frac, std::int64_t *mono_us);

} // namespace detail
} // namespace ipc_rtsp
