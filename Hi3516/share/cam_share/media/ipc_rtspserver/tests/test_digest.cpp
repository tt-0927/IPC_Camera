/**
 * @FilePath     : test_digest.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : Digest 认证测试（MD5 向量与校验流程）
 */

#include "auth/digest.h"

#include "support/md5.h"
#include "support/text.h"

#include "test_support.h"

using namespace ipc_rtsp;
using namespace ipc_rtsp::detail;

namespace
{

/** 构造标准 Digest 鉴权配置。
 *
 * @return 预填 realm/user/password 的鉴权配置。
 */
AuthConfig MakeConfig()
{
    AuthConfig config;
    config.mode = AuthMode::Digest;
    config.realm = "Itc Streaming Server";
    config.user = "admin";
    config.password = "zfrl@168";
    return config;
}

} // namespace

RTSP_TEST_CASE(md5_known_vectors)
{
    /* RFC 1321 附录 A.5 测试向量。 */
    RTSP_CHECK_EQ(Md5Hex(std::string("")), std::string("d41d8cd98f00b204e9800998ecf8427e"));
    RTSP_CHECK_EQ(Md5Hex(std::string("abc")), std::string("900150983cd24fb0d6963f7d28e17f72"));
    RTSP_CHECK_EQ(Md5Hex(std::string("message digest")), std::string("f96b697d7cb7938d525a2f31aaf161d0"));
    RTSP_CHECK_EQ(Md5Hex(std::string("abcdefghijklmnopqrstuvwxyz")), std::string("c3fcd3d76192e4007dfb496cca67e13b"));
    RTSP_CHECK_EQ(Md5Hex(std::string("12345678901234567890123456789012345678901234567890123456789012345678901234567890")),
                  std::string("57edf4a22be3c955ac49da2e2107b67a"));
}

RTSP_TEST_CASE(md5_long_input_across_blocks)
{
    /* 跨越 64 字节块边界（含 padding 分支）。 */
    std::string text(1000, 'a');
    std::string expected = Md5Hex(text);
    RTSP_CHECK_EQ(expected.size(), static_cast<std::size_t>(32));
    /* 与分段更新结果一致。 */
    Md5Context ctx;
    Md5Init(&ctx);
    Md5Update(&ctx, text.data(), 137);
    Md5Update(&ctx, text.data() + 137, text.size() - 137);
    std::uint8_t digest[16];
    Md5Final(&ctx, digest);
    RTSP_CHECK_EQ(HexEncode(digest, sizeof digest), expected);
}

RTSP_TEST_CASE(digest_challenge_and_verify_without_qop)
{
    const AuthConfig config = MakeConfig();
    const std::string challenge = BuildDigestChallenge(config, "abc123");
    RTSP_CHECK(challenge.find("realm=\"Itc Streaming Server\"") != std::string::npos);
    RTSP_CHECK(challenge.find("nonce=\"abc123\"") != std::string::npos);

    /* 按 RFC 2617 计算期望摘要。 */
    const std::string uri = "rtsp://127.0.0.1:554/Streaming/Channels/101";
    const std::string ha1 = Md5Hex(config.user + ":" + config.realm + ":" + config.password);
    const std::string ha2 = Md5Hex(std::string("DESCRIBE") + ":" + uri);
    const std::string response = Md5Hex(ha1 + ":abc123:" + ha2);

    const std::string header = "Digest username=\"admin\", realm=\"" + config.realm + "\", nonce=\"abc123\", uri=\"" + uri +
                               "\", response=\"" + response + "\"";
    RTSP_CHECK(VerifyDigest(config, "DESCRIBE", uri, header, "abc123") == AuthResult::Ok);
}

RTSP_TEST_CASE(digest_verify_with_qop_auth)
{
    /* 场景：qop=auth 的 Digest 应答；预期按 RFC 2617 的 qop 算法拼装并校验通过。 */
    const AuthConfig config = MakeConfig();
    const std::string uri = "/Streaming/Channels/101";
    const std::string nonce = "n0nce";
    const std::string cnonce = "c0ffee";
    const std::string nc = "00000001";

    const std::string ha1 = Md5Hex(config.user + ":" + config.realm + ":" + config.password);
    const std::string ha2 = Md5Hex(std::string("SETUP") + ":" + uri);
    const std::string response = Md5Hex(ha1 + ":" + nonce + ":" + nc + ":" + cnonce + ":auth:" + ha2);

    const std::string header = "Digest username=\"admin\", realm=\"" + config.realm + "\", nonce=\"" + nonce + "\", uri=\"" + uri +
                               "\", qop=auth, nc=" + nc + ", cnonce=\"" + cnonce + "\", response=\"" + response + "\"";
    RTSP_CHECK(VerifyDigest(config, "SETUP", uri, header, nonce) == AuthResult::Ok);
}

RTSP_TEST_CASE(digest_rejects_wrong_password_and_nonce)
{
    /* 场景：四种非法鉴权输入；预期各自返回对应的失败结果。 */
    const AuthConfig config = MakeConfig();
    const std::string uri = "/Streaming/Channels/101";
    const std::string nonce = "n1";

    const std::string wrong_ha1 = Md5Hex(std::string("admin:") + config.realm + ":wrong");
    const std::string ha2 = Md5Hex(std::string("PLAY") + ":" + uri);
    const std::string response = Md5Hex(wrong_ha1 + ":" + nonce + ":" + ha2);
    const std::string header = "Digest username=\"admin\", nonce=\"" + nonce + "\", uri=\"" + uri + "\", response=\"" + response + "\"";
    /* 密码错误：response 对不上。 */
    RTSP_CHECK(VerifyDigest(config, "PLAY", uri, header, nonce) == AuthResult::BadCredentials);
    /* nonce 与服务端当前值不一致。 */
    RTSP_CHECK(VerifyDigest(config, "PLAY", uri, header, "other-nonce") == AuthResult::BadNonce);
    /* Authorization 头缺失。 */
    RTSP_CHECK(VerifyDigest(config, "PLAY", uri, "", nonce) == AuthResult::Missing);
    /* 非 Digest 方案（Basic）。 */
    RTSP_CHECK(VerifyDigest(config, "PLAY", uri, "Basic YWRtaW46cHdk", nonce) == AuthResult::Malformed);
}

RTSP_TEST_CASE(digest_field_parser_handles_quotes_and_spacing)
{
    /* 场景：字段值带引号/不带引号、逗号与空格混排；预期都能取到正确值，缺失字段返回空。 */
    const std::string header = "Digest username=\"admin\",nonce=abc, uri=\"/a/b\", qop=auth";
    RTSP_CHECK(DigestField(header, "username") == "admin");
    RTSP_CHECK(DigestField(header, "nonce") == "abc");
    RTSP_CHECK(DigestField(header, "uri") == "/a/b");
    RTSP_CHECK(DigestField(header, "qop") == "auth");
    RTSP_CHECK(DigestField(header, "missing").empty());
}

RTSP_TEST_CASE(nonce_store_rotates_and_matches)
{
    NonceStore store;
    /* 注意：Current 返回内部引用，取值时必须拷贝，否则会随轮换一起变化。 */
    const std::string first = store.Current(0, 300);
    RTSP_CHECK(!first.empty());
    RTSP_CHECK(store.Matches(first));
    RTSP_CHECK(store.Matches(store.Current(100 * 1000, 300))); /* TTL 内不轮换 */

    /* 超过 TTL 后轮换：旧 nonce 失效。 */
    const std::string second = store.Current(400 * 1000, 300);
    RTSP_CHECK(second != first);
    RTSP_CHECK(store.Matches(second));
    RTSP_CHECK(!store.Matches(first));
}
