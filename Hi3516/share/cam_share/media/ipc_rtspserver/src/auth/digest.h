/**
 * @FilePath     : digest.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : RTSP Digest 认证（RFC 2617 兼容，MD5）
 */

/*
 * smolrtsp 不提供鉴权，产品侧在 DESCRIBE/SETUP 等请求前统一校验。为兼容 IPC/NVR
 * 客户端，challenge 不带 qop；客户端若带 qop/nc/cnonce，则无论取值（含 auth-int）
 * 一律按 qop=auth 公式校验。
 * review: auth-int 未按 RFC 7616 区分（未将 body 计入 HA2）。
 */
#pragma once

#include "ipc_rtsp/config.h"

#include <cstdint>
#include <string>
#include "support/string_view.h"

namespace ipc_rtsp
{
namespace detail
{

/** 认证校验结果。 */
enum class AuthResult
{
    Ok = 0,         /**< 校验通过 */
    Missing,        /**< 没有 Authorization 头 */
    Malformed,      /**< Authorization 头无法解析 */
    BadNonce,       /**< nonce 不匹配（需要重新挑战） */
    BadCredentials, /**< 用户名/密码错误 */
};

/**
 * @brief 生成 `WWW-Authenticate: Digest ...` 的值部分。
 *
 * @param config 鉴权配置（取 realm）
 * @param nonce 本连接的当前 nonce
 * @return 形如 `Digest realm="..." nonce="..." algorithm=MD5` 的值
 */
std::string BuildDigestChallenge(const AuthConfig &config, StringView nonce);

/**
 * @brief 校验 `Authorization: Digest ...`。
 *
 * @param method        请求方法（OPTIONS/DESCRIBE/...）
 * @param uri           请求行 URI；实现不校验其与 Authorization 中 uri 字段的
 *                      一致性——客户端 uri 字段非空时直接参与 HA2 计算
 * @param authorization Authorization 头的完整值
 * @param expected_nonce 本连接当前应持有的 nonce
 * @return 校验结果；BadNonce 时调用方应轮换 nonce 并重新挑战
 */
AuthResult VerifyDigest(const AuthConfig &config,
                        StringView method,
                        StringView uri,
                        StringView authorization,
                        StringView expected_nonce);

/**
 * @brief 解析 `Authorization` 头里的一个参数（name="value" 或 name=value）。
 *
 * @param authorization Authorization 头中 scheme 之后的参数段
 * @param name 参数名（区分大小写）
 * @return 参数值（带引号形式已去除引号）；不存在或格式非法返回空
 */
StringView DigestField(StringView authorization, StringView name);

/**
 * 每连接的 nonce 管理。
 *
 * 线程约定：只在 I/O 线程使用。
 */
class NonceStore
{
public:
    /**
     * @brief 取得当前 nonce；不存在或已过期则重新生成。
     *
     * @param now_ms 当前时间（单调时钟，毫秒）
     * @param ttl_s nonce 有效期（秒）
     * @return 当前 nonce 的引用（成员内保存，I/O 线程内有效）
     */
    const std::string &Current(std::int64_t now_ms, std::uint32_t ttl_s);

    /**
     * @brief 校验客户端 nonce 是否等于当前值（常量时间比较）。
     *
     * @param nonce 客户端给出的 nonce
     * @return 相等返回 true；尚未生成过 nonce 返回 false
     */
    bool Matches(StringView nonce) const;

    /**
     * @brief 强制轮换 nonce（时间回拨等场景），下次 Current() 返回新值。
     *
     * @return {void}
     */
    void Rotate();

private:
    /* 当前 nonce（32 字符十六进制）；空表示尚未生成。 */
    std::string nonce_;
    /* 当前 nonce 的生成时间（单调时钟，毫秒）；与 ttl_s 共同判定过期。 */
    std::int64_t created_ms_ = 0;
};

} // namespace detail
} // namespace ipc_rtsp
