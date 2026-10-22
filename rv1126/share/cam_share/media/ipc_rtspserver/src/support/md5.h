/**
 * @FilePath     : md5.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : MD5（RFC 1321）实现
 */
/*
 * 仅用于 RTSP Digest 认证（HTTP Digest 兼容）。实现自包含，不依赖 OpenSSL：
 * 目标板（musl 工具链）不保证有 crypto 库，而 Digest 只需要 MD5。
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace ipc_rtsp
{
namespace detail
{

/**
 * MD5 上下文。
 *
 * @note 使用顺序固定：Md5Init → Md5Update（可多次）→ Md5Final；Final 之后
 *       上下文状态被填充破坏，不可复用，需重新 Md5Init。
 */
struct Md5Context
{
    /* 中间散列状态（A/B/C/D，Init 时重置为标准初始向量）。 */
    std::uint32_t state[4] = { 0x67452301u, 0xefcdab89u, 0x98badcfeu, 0x10325476u };
    /* 累计已处理的比特数（每次 Update 累加 len*8）。 */
    std::uint64_t bit_count = 0;
    /* 尾部不足 64 字节的待处理数据暂存。 */
    std::uint8_t buffer[64] = { 0 };
    /* buffer 内有效字节数，恒 < 64；凑满 64 即消费并归零。 */
    std::size_t buffer_len = 0;
};

/**
 * 初始化上下文。
 *
 * @param ctx 目标上下文，不可为 nullptr
 * @return {void} 无返回值
 */
void Md5Init(Md5Context *ctx);

/**
 * 追加一段数据。
 *
 * @param ctx  已 Init 的上下文，不可为 nullptr
 * @param data 输入数据；可为 nullptr（此时 len 必须为 0）
 * @param len  输入字节数
 * @return {void} 无返回值
 */
void Md5Update(Md5Context *ctx, const void *data, std::size_t len);

/**
 * 结束计算并输出 16 字节摘要。
 *
 * @param ctx    已 Update 过的上下文；调用后不可复用
 * @param digest 输出缓冲，接收 16 字节摘要（状态字按小端序拼接）
 * @return {void} 无返回值
 */
void Md5Final(Md5Context *ctx, std::uint8_t digest[16]);

/**
 * 计算数据的 MD5 十六进制小写字符串。
 *
 * @param data 输入数据；可为 nullptr（此时或 len 为 0 时等效于空输入）
 * @param len  输入字节数
 * @return 32 字符小写十六进制串
 */
std::string Md5Hex(const void *data, std::size_t len);

/**
 * 计算字符串的 MD5 十六进制小写字符串。
 *
 * @param text 输入字符串
 * @return 32 字符小写十六进制串
 */
std::string Md5Hex(const std::string &text);

} // namespace detail
} // namespace ipc_rtsp
