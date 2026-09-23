/**
 * @FilePath     : md5.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 11:35:00
 * @Description  : MD5（RFC 1321）实现
 */

#include "support/md5.h"

#include <cstring>

namespace ipc_rtsp
{
namespace detail
{
namespace
{

/** 每轮的循环左移位数。 */
constexpr std::uint32_t kShift[64] = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
};

/* K[i] = floor(2^32 * |sin(i + 1)|)（RFC 1321 T 表）；用编译期常量而非运行期
 * 计算的懒初始化，消除多线程首次调用的数据竞争隐患与首次鉴权的 64 次三角函数
 * 开销。正确性由 test_digest 的 RFC 1321 标准向量回归保证。 */
constexpr std::uint32_t kTable[64] = {
    0xd76aa478u, 0xe8c7b756u, 0x242070dbu, 0xc1bdceeeu, 0xf57c0fafu, 0x4787c62au, 0xa8304613u, 0xfd469501u, 0x698098d8u, 0x8b44f7afu,
    0xffff5bb1u, 0x895cd7beu, 0x6b901122u, 0xfd987193u, 0xa679438eu, 0x49b40821u, 0xf61e2562u, 0xc040b340u, 0x265e5a51u, 0xe9b6c7aau,
    0xd62f105du, 0x02441453u, 0xd8a1e681u, 0xe7d3fbc8u, 0x21e1cde6u, 0xc33707d6u, 0xf4d50d87u, 0x455a14edu, 0xa9e3e905u, 0xfcefa3f8u,
    0x676f02d9u, 0x8d2a4c8au, 0xfffa3942u, 0x8771f681u, 0x6d9d6122u, 0xfde5380cu, 0xa4beea44u, 0x4bdecfa9u, 0xf6bb4b60u, 0xbebfbc70u,
    0x289b7ec6u, 0xeaa127fau, 0xd4ef3085u, 0x04881d05u, 0xd9d4d039u, 0xe6db99e5u, 0x1fa27cf8u, 0xc4ac5665u, 0xf4292244u, 0x432aff97u,
    0xab9423a7u, 0xfc93a039u, 0x655b59c3u, 0x8f0ccc92u, 0xffeff47du, 0x85845dd1u, 0x6fa87e4fu, 0xfe2ce6e0u, 0xa3014314u, 0x4e0811a1u,
    0xf7537e82u, 0xbd3af235u, 0x2ad7d2bbu, 0xeb86d391u,
};

inline std::uint32_t RotateLeft(std::uint32_t value, std::uint32_t bits)
{
    return (value << bits) | (value >> (32u - bits));
}

void Transform(std::uint32_t state[4], const std::uint8_t block[64])
{
    std::uint32_t m[16];
    for (int i = 0; i < 16; ++i)
    {
        m[i] = static_cast<std::uint32_t>(block[i * 4]) | (static_cast<std::uint32_t>(block[i * 4 + 1]) << 8) |
               (static_cast<std::uint32_t>(block[i * 4 + 2]) << 16) | (static_cast<std::uint32_t>(block[i * 4 + 3]) << 24);
    }

    std::uint32_t a = state[0];
    std::uint32_t b = state[1];
    std::uint32_t c = state[2];
    std::uint32_t d = state[3];

    for (std::uint32_t i = 0; i < 64; ++i)
    {
        std::uint32_t f = 0;
        std::uint32_t g = 0;

        if (i < 16)
        {
            f = (b & c) | (~b & d);
            g = i;
        }
        else if (i < 32)
        {
            f = (d & b) | (~d & c);
            g = (5u * i + 1u) % 16u;
        }
        else if (i < 48)
        {
            f = b ^ c ^ d;
            g = (3u * i + 5u) % 16u;
        }
        else
        {
            f = c ^ (b | ~d);
            g = (7u * i) % 16u;
        }

        const std::uint32_t tmp = d;
        d = c;
        c = b;
        b = b + RotateLeft(a + f + kTable[i] + m[g], kShift[i]);
        a = tmp;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

} // namespace

void Md5Init(Md5Context *ctx)
{
    ctx->state[0] = 0x67452301u;
    ctx->state[1] = 0xefcdab89u;
    ctx->state[2] = 0x98badcfeu;
    ctx->state[3] = 0x10325476u;
    ctx->bit_count = 0;
    ctx->buffer_len = 0;
}

void Md5Update(Md5Context *ctx, const void *data, std::size_t len)
{
    const std::uint8_t *bytes = static_cast<const std::uint8_t *>(data);
    ctx->bit_count += static_cast<std::uint64_t>(len) * 8u;

    while (len > 0)
    {
        const std::size_t space = 64u - ctx->buffer_len;
        const std::size_t take = len < space ? len : space;
        std::memcpy(ctx->buffer + ctx->buffer_len, bytes, take);
        ctx->buffer_len += take;
        bytes += take;
        len -= take;

        if (ctx->buffer_len == 64u)
        {
            Transform(ctx->state, ctx->buffer);
            ctx->buffer_len = 0;
        }
    }
}

void Md5Final(Md5Context *ctx, std::uint8_t digest[16])
{
    const std::uint64_t bit_count = ctx->bit_count;
    const std::uint8_t padding[64] = { 0x80 };

    const std::size_t pad_len = (ctx->buffer_len < 56u) ? (56u - ctx->buffer_len) : (120u - ctx->buffer_len);
    Md5Update(ctx, padding, pad_len);

    std::uint8_t length_bytes[8];
    for (int i = 0; i < 8; ++i)
    {
        length_bytes[i] = static_cast<std::uint8_t>((bit_count >> (8 * i)) & 0xffu);
    }
    Md5Update(ctx, length_bytes, sizeof length_bytes);

    for (int i = 0; i < 4; ++i)
    {
        digest[i * 4] = static_cast<std::uint8_t>(ctx->state[i] & 0xffu);
        digest[i * 4 + 1] = static_cast<std::uint8_t>((ctx->state[i] >> 8) & 0xffu);
        digest[i * 4 + 2] = static_cast<std::uint8_t>((ctx->state[i] >> 16) & 0xffu);
        digest[i * 4 + 3] = static_cast<std::uint8_t>((ctx->state[i] >> 24) & 0xffu);
    }
}

std::string Md5Hex(const void *data, std::size_t len)
{
    Md5Context ctx;
    Md5Init(&ctx);
    Md5Update(&ctx, data, len);

    std::uint8_t digest[16];
    Md5Final(&ctx, digest);

    static const char kHex[] = "0123456789abcdef";
    std::string out;
    out.resize(32);
    for (int i = 0; i < 16; ++i)
    {
        out[static_cast<std::size_t>(i) * 2] = kHex[digest[i] >> 4];
        out[static_cast<std::size_t>(i) * 2 + 1] = kHex[digest[i] & 0x0fu];
    }
    return out;
}

std::string Md5Hex(const std::string &text)
{
    return Md5Hex(text.data(), text.size());
}

} // namespace detail
} // namespace ipc_rtsp
