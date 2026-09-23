/**
 * @FilePath     : digest.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : Digest 认证实现
 */

#include "auth/digest.h"

#include "support/md5.h"
#include "support/text.h"
#include "support/time.h"

#include <cstddef>

namespace ipc_rtsp
{
namespace detail
{
namespace
{

/**
 * @brief 常量时间比较，避免通过响应时间推断摘要。
 *
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 逐字节异或累积后判断：长度或任一字节不同返回 false
 */
bool ConstantTimeEquals(StringView lhs, StringView rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    unsigned char diff = 0;
    for (std::size_t i = 0; i < lhs.size(); ++i)
    {
        diff |= static_cast<unsigned char>(lhs[i]) ^ static_cast<unsigned char>(rhs[i]);
    }
    return diff == 0;
}

} // namespace

std::string BuildDigestChallenge(const AuthConfig &config, StringView nonce)
{
    /* 不带 qop：兼容不支持 qop 的老旧 NVR/客户端（客户端若带 qop 仍可校验）。 */
    std::string out = "Digest realm=\"";
    out += config.realm;
    out += "\", nonce=\"";
    out.append(nonce.data(), nonce.size());
    out += "\", algorithm=MD5";
    return out;
}

StringView DigestField(StringView authorization, StringView name)
{
    std::size_t pos = 0;
    while (pos < authorization.size())
    {
        const std::size_t found = authorization.find(name, pos);
        if (found == StringView::npos)
        {
            return {};
        }

        /* 命中位置必须是独立参数名：左侧为串首或分隔符（空格/逗号/分号），
         * 且名字后紧跟 '='；两个条件缺一不可，否则 "uri" 会误匹配
         * "cnonce" 之类含相同子串的名字。不满足则继续向后找。 */
        const bool at_boundary = (found == 0) || (authorization[found - 1] == ' ') || (authorization[found - 1] == ',') ||
                                 (authorization[found - 1] == ';');
        const std::size_t after = found + name.size();
        if (!at_boundary || after >= authorization.size() || authorization[after] != '=')
        {
            pos = found + 1;
            continue;
        }

        /* 跳过 '=' 后的空格，定位值起点。 */
        std::size_t value_begin = after + 1;
        while (value_begin < authorization.size() && authorization[value_begin] == ' ')
        {
            ++value_begin;
        }

        /* 带引号值（realm/uri/cnonce 等通常带引号）：取两引号之间内容；
         * 引号未闭合视为缺失，返回空。 */
        if (value_begin < authorization.size() && authorization[value_begin] == '"')
        {
            const std::size_t value_end = authorization.find('"', value_begin + 1);
            if (value_end == StringView::npos)
            {
                return {};
            }
            return authorization.substr(value_begin + 1, value_end - value_begin - 1);
        }

        /* 裸值：到首个分隔符（逗号/空格）为止。 */
        const std::size_t value_end = authorization.find_first_of(", ", value_begin);
        return authorization.substr(value_begin, (value_end == StringView::npos ? authorization.size() : value_end) - value_begin);
    }
    return {};
}

AuthResult VerifyDigest(const AuthConfig &config,
                        StringView method,
                        StringView uri,
                        StringView authorization,
                        StringView expected_nonce)
{
    if (authorization.empty())
    {
        return AuthResult::Missing;
    }

    const std::size_t scheme_end = authorization.find(' ');
    if (scheme_end == StringView::npos)
    {
        return AuthResult::Malformed;
    }
    if (!EqualsIgnoreCase(authorization.substr(0, scheme_end), "Digest"))
    {
        /* v1 只支持 Digest；Basic 属于后续兼容项。 */
        return AuthResult::Malformed;
    }

    const StringView params = authorization.substr(scheme_end + 1);
    const StringView user = DigestField(params, "username");
    const StringView realm = DigestField(params, "realm");
    const StringView nonce = DigestField(params, "nonce");
    const StringView uri_field = DigestField(params, "uri");
    const StringView response = DigestField(params, "response");
    const StringView qop = DigestField(params, "qop");
    const StringView nc = DigestField(params, "nc");
    const StringView cnonce = DigestField(params, "cnonce");

    if (user.empty() || nonce.empty() || response.empty())
    {
        return AuthResult::Malformed;
    }
    if (!ConstantTimeEquals(nonce, expected_nonce))
    {
        return AuthResult::BadNonce;
    }
    if (!ConstantTimeEquals(user, config.user))
    {
        return AuthResult::BadCredentials;
    }
    if (!realm.empty() && !ConstantTimeEquals(realm, config.realm))
    {
        return AuthResult::BadCredentials;
    }

    /* uri 字段非空时直接采用客户端值参与 HA2 计算，实现不校验其与请求行 URI
     * 一致；缺失时退回请求行 URI。 */
    /* review: 未校验 uri 字段与请求行一致，防重放依赖 nonce 单值。 */
    const StringView effective_uri = uri_field.empty() ? uri : uri_field;

    const std::string ha1 = Md5Hex(config.user + ":" + config.realm + ":" + config.password);
    const std::string ha2 = Md5Hex(std::string(method.data(), method.size()) + ":" + std::string(effective_uri.data(), effective_uri.size()));

    std::string expected;
    if (!qop.empty())
    {
        /* 客户端带任意非空 qop（含 auth-int）时一律按 qop=auth 公式：
         * response = MD5(HA1:nonce:nc:cnonce:qop:HA2)。 */
        expected = Md5Hex(ha1 + ":" + std::string(nonce.data(), nonce.size()) + ":" + std::string(nc.data(), nc.size()) + ":" +
                              std::string(cnonce.data(), cnonce.size()) + ":" + std::string(qop.data(), qop.size()) + ":" + ha2);
    }
    else
    {
        expected = Md5Hex(ha1 + ":" + std::string(nonce.data(), nonce.size()) + ":" + ha2);
    }

    return ConstantTimeEquals(expected, response) ? AuthResult::Ok : AuthResult::BadCredentials;
}

const std::string &NonceStore::Current(std::int64_t now_ms, std::uint32_t ttl_s)
{
    if (nonce_.empty() || (now_ms - created_ms_) > static_cast<std::int64_t>(ttl_s) * 1000)
    {
        Rotate();
        created_ms_ = now_ms;
    }
    return nonce_;
}

bool NonceStore::Matches(StringView nonce) const
{
    return !nonce_.empty() && ConstantTimeEquals(nonce_, nonce);
}

void NonceStore::Rotate()
{
    /* 单次随机数只有 64 位熵，取两次拼成 128 位随机字节再转 32 字符十六进制。 */
    const std::uint64_t random = RandomU64();
    std::uint8_t bytes[16];
    for (int i = 0; i < 8; ++i)
    {
        bytes[i] = static_cast<std::uint8_t>((random >> (8 * i)) & 0xffu);
    }
    const std::uint64_t second = RandomU64();
    for (int i = 0; i < 8; ++i)
    {
        bytes[8 + i] = static_cast<std::uint8_t>((second >> (8 * i)) & 0xffu);
    }
    nonce_ = HexEncode(bytes, sizeof bytes);
}

} // namespace detail
} // namespace ipc_rtsp
