/**
 * @FilePath     : crypto_manager.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-06-10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-22 09:10:42
 * @Description  : 密码学模块管理类实现（统一门面）
 */

#include "crypto_manager.h"
#include "dlog.h"
#include "IpcRet.h"

#include <sstream>
#include <iomanip>
#include <unistd.h>

/* Provider 未设置时按当前接口返回类型提前返回 */
#define RETURN_IF_PROVIDER_UNINIT(ret)                       \
    do                                                       \
    {                                                        \
        if (!m_pProvider)                                    \
        {                                                    \
            dlog_error("[CCryptoManager] Provider 未初始化");\
            return ret;                                      \
        }                                                    \
    }                                                        \
    while (0)

/* ===================== 本地辅助函数 ===================== */

/**
 * @brief   : 字节数组转十六进制字符串
 * @param    {vector<uint8_t>} &data：字节数组
 * @return   {std::string} 十六进制字符串（小写）
 */
static std::string bytes_to_hex(const std::vector<uint8_t> &data)
{
    std::ostringstream oss;
    for (auto b : data)
    {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return oss.str();
}

/* ===================== 构造/析构 ===================== */

CCryptoManager::CCryptoManager()
{
}

CCryptoManager::~CCryptoManager()
{
    if (m_bInitialized)
    {
        deinit();
    }
}

/* ===================== 初始化/反初始化 ===================== */

IpcRet_E CCryptoManager::init()
{
    if (m_bInitialized)
    {
        dlog_warn("[CCryptoManager] 已初始化");
        return OK;
    }

    /* 检查 Provider 是否已设置 */
    if (!m_pProvider)
    {
        dlog_error("[CCryptoManager] Provider 未设置，请先调用 set_provider()");
        return ERR;
    }

    IpcRet_E ret = m_pProvider->init();
    if (ret != OK)
    {
        dlog_error("[CCryptoManager] Provider 初始化失败: %s, ret=%d", m_pProvider->name(), ret);
        return ret;
    }

    m_bInitialized = true;
    dlog_info("[CCryptoManager] init 完成，当前 Provider: %s", m_pProvider->name());
    return OK;
}

IpcRet_E CCryptoManager::deinit()
{
    if (!m_bInitialized)
    {
        return OK;
    }

    dlog_trace("[CCryptoManager] deinit 开始，当前 Provider: %s", m_pProvider ? m_pProvider->name() : "null");
    if (m_pProvider)
    {
        m_pProvider->deinit();
    }
    m_pProvider = nullptr;
    m_bInitialized = false;
    dlog_trace("[CCryptoManager] deinit 完成");
    return OK;
}

/* ===================== Provider 管理 ===================== */

IpcRet_E CCryptoManager::set_provider(ICryptoProvider *provider)
{
    if (m_bInitialized)
    {
        dlog_error("[CCryptoManager] 已初始化，禁止运行期替换 Provider");
        return ERR;
    }

    if (!provider)
    {
        m_pProvider = nullptr;
        return OK;
    }

    m_pProvider = provider;
    dlog_info("[CCryptoManager] Provider 已设置: %s", m_pProvider->name());
    return OK;
}

ICryptoProvider *CCryptoManager::get_provider() const
{
    return m_pProvider;
}

bool CCryptoManager::is_initialized() const
{
    return m_bInitialized;
}

bool CCryptoManager::is_ready() const
{
    return m_bInitialized && m_pProvider && m_pProvider->is_ready();
}

IpcRet_E CCryptoManager::self_test()
{
    if (!is_ready())
    {
        dlog_error("[CCryptoManager] Provider 未就绪，无法执行自测");
        return ERR_UNINIT;
    }

    return run_self_test();
}

/* ===================== 统一接口实现 ===================== */

std::string CCryptoManager::rand_bytes(size_t length, bool hex_output)
{
    RETURN_IF_PROVIDER_UNINIT("");
    return m_pProvider->rand_bytes(length, hex_output);
}

int CCryptoManager::rand_bytes(uint8_t *buf, size_t buflen)
{
    RETURN_IF_PROVIDER_UNINIT(ERR);
    return m_pProvider->rand_bytes(buf, buflen);
}

std::vector<uint8_t> CCryptoManager::sm3_hash(const std::vector<uint8_t> &data)
{
    RETURN_IF_PROVIDER_UNINIT({});
    return m_pProvider->sm3_hash(data);
}

std::string CCryptoManager::sm2_sign(const std::string &privkey_path,
                                     const std::string &pass,
                                     const std::string &data,
                                     const std::string &id)
{
    RETURN_IF_PROVIDER_UNINIT("");
    return m_pProvider->sm2_sign(privkey_path, pass, data, id);
}

bool CCryptoManager::sm2_verify(const std::string &pubkey_path,
                                const std::string &data,
                                const std::string &signature,
                                const std::string &id)
{
    RETURN_IF_PROVIDER_UNINIT(false);
    return m_pProvider->sm2_verify(pubkey_path, data, signature, id);
}

std::string CCryptoManager::sm2_decrypt(const std::vector<uint8_t> &input, const std::string &key_path)
{
    RETURN_IF_PROVIDER_UNINIT("");
    return m_pProvider->sm2_decrypt(input, key_path);
}

std::string CCryptoManager::sm4_encrypt_cbc(const std::string &key, const std::string &iv, const std::string &plaintext)
{
    RETURN_IF_PROVIDER_UNINIT("");
    return m_pProvider->sm4_encrypt_cbc(key, iv, plaintext);
}

std::string CCryptoManager::sm4_decrypt_cbc(const std::string &key, const std::string &iv, const std::string &ciphertext)
{
    RETURN_IF_PROVIDER_UNINIT("");
    return m_pProvider->sm4_decrypt_cbc(key, iv, ciphertext);
}

bool CCryptoManager::reqgen(const std::string &country,
                            const std::string &state,
                            const std::string &locality,
                            const std::string &organization,
                            const std::string &organization_unit,
                            const std::string &common_name,
                            const std::string &key_file,
                            const std::string &password,
                            const std::string &output_file)
{
    RETURN_IF_PROVIDER_UNINIT(false);
    return m_pProvider
        ->reqgen(country, state, locality, organization, organization_unit, common_name, key_file, password, output_file);
}

std::string CCryptoManager::certparse(const std::string &cert_path)
{
    RETURN_IF_PROVIDER_UNINIT("");
    return m_pProvider->certparse(cert_path);
}

std::string CCryptoManager::reqparse(const std::string &req_path)
{
    RETURN_IF_PROVIDER_UNINIT("");
    return m_pProvider->reqparse(req_path);
}

std::string CCryptoManager::crlparse(const std::string &crl_path)
{
    RETURN_IF_PROVIDER_UNINIT("");
    return m_pProvider->crlparse(crl_path);
}

std::string CCryptoManager::crlverify(const std::string &crl_path, const std::string &cert_path)
{
    RETURN_IF_PROVIDER_UNINIT("");
    return m_pProvider->crlverify(crl_path, cert_path);
}

int CCryptoManager::digital_signature(const std::vector<uint8_t> &input_data, std::string &out_data, const std::string &key_path)
{
    RETURN_IF_PROVIDER_UNINIT(ERR);
    return m_pProvider->digital_signature(input_data, out_data, key_path);
}

int CCryptoManager::verify_signature(const std::vector<uint8_t> &sign_data,
                                     const std::vector<uint8_t> &verify_sign,
                                     const std::string &cert_path)
{
    RETURN_IF_PROVIDER_UNINIT(ERR);
    return m_pProvider->verify_signature(sign_data, verify_sign, cert_path);
}

std::vector<uint8_t> CCryptoManager::base64_encode(const std::vector<uint8_t> &input)
{
    RETURN_IF_PROVIDER_UNINIT({});
    return m_pProvider->base64_encode(input);
}

std::vector<uint8_t> CCryptoManager::base64_encode(const std::string &input)
{
    RETURN_IF_PROVIDER_UNINIT({});
    return m_pProvider->base64_encode(input);
}

std::vector<uint8_t> CCryptoManager::base64_decode(const std::string &input)
{
    RETURN_IF_PROVIDER_UNINIT({});
    return m_pProvider->base64_decode(input);
}

std::vector<uint8_t> CCryptoManager::base64_decode(const char *input)
{
    RETURN_IF_PROVIDER_UNINIT({});
    return m_pProvider->base64_decode(input);
}

std::string CCryptoManager::base64_encode_to_string(const std::string &input)
{
    RETURN_IF_PROVIDER_UNINIT("");
    return m_pProvider->base64_encode_to_string(input);
}

std::string CCryptoManager::base64_encode_to_string(const std::vector<uint8_t> &input)
{
    RETURN_IF_PROVIDER_UNINIT("");
    return m_pProvider->base64_encode_to_string(input);
}

int CCryptoManager::sm2keygen(const std::string &pass, const std::string &privkey_path, const std::string &pubkey_path)
{
    RETURN_IF_PROVIDER_UNINIT(ERR);
    return m_pProvider->sm2keygen(pass, privkey_path, pubkey_path);
}

int CCryptoManager::sm2keygen(const std::string &pass, const std::string &privkey_path)
{
    RETURN_IF_PROVIDER_UNINIT(ERR);
    return m_pProvider->sm2keygen(pass, privkey_path);
}

/* ===================== 测试逻辑 ===================== */

IpcRet_E CCryptoManager::run_self_test()
{
    dlog_warn("============= CCryptoManager::self_test() 开始 =============");

    /* 1. 测试随机数生成 */
    std::string strRand = m_pProvider->rand_bytes(16, true);
    dlog_info("[Test 1] 随机数生成 (16字节, Hex): %s", strRand.c_str());
    if (strRand.length() != 32)
    {
        dlog_error("[Test 1] 随机数生成失败，Hex 长度错误");
        return ERR;
    }
    else
    {
        dlog_info("[Test 1] 随机数生成验证通过");
    }

    /* 2. 测试 SM3 杂凑 */
    std::vector<uint8_t> vecMsg = { 'H', 'e', 'l', 'l', 'o' };
    std::vector<uint8_t> vecHash = m_pProvider->sm3_hash(vecMsg);
    std::string strHashHex = bytes_to_hex(vecHash);
    dlog_info("[Test 2] SM3 杂凑验证 MSG=Hello, Hash: %s", strHashHex.c_str());
    if (vecHash.size() != 32)
    {
        dlog_error("[Test 2] SM3 杂凑长度错误");
        return ERR;
    }
    else
    {
        dlog_info("[Test 2] SM3 杂凑验证通过");
    }

    /* 3. 测试 SM4-CBC 加解密 */
    std::string strPlain = "Hello, Majesty! This is GM Crypto test.";
    std::string strKeyHex = "0123456789ABCDEFFEDCBA9876543210";
    std::string strIvHex = "0123456789ABCDEFFEDCBA9876543210";
    std::string strCipher = m_pProvider->sm4_encrypt_cbc(strKeyHex, strIvHex, strPlain);
    dlog_info("[Test 3] SM4-CBC 加密, CipherText(Base64): %s", strCipher.c_str());
    std::string strDecrypt = m_pProvider->sm4_decrypt_cbc(strKeyHex, strIvHex, strCipher);
    dlog_info("[Test 3] SM4-CBC 解密, DecryptText: %s", strDecrypt.c_str());
    if (strDecrypt != strPlain)
    {
        dlog_error("[Test 3] SM4-CBC 加解密自测试失败");
        return ERR;
    }
    else
    {
        dlog_info("[Test 3] SM4-CBC 加解密验证通过");
    }

    /* 4. 测试证书请求 (CSR) 生成与解析 */
    std::string strCountry = "CN";
    std::string strState = "GuangDong";
    std::string strLocality = "GuangZhou";
    std::string strOrg = "itc";
    std::string strOrgUnit = "ShiJue2";
    std::string strCN = "TEST_DEVICE_ID_NULL";
    std::string strTempKey = "/tmp/test_sm2.key";
    std::string strTempPubKey = "/tmp/test_sm2.pub";
    std::string strTempCSR = "/tmp/test_sm2.csr";

    /* 4.1 使用 Provider 生成测试密钥对 */
    if (m_pProvider->sm2keygen(Gb35114Crypto_NS::SM2_PRIVATE_KEY_ENCRYPT_PASSWORD, strTempKey, strTempPubKey) != OK)
    {
        dlog_error("[Test 4] SM2 密钥生成失败: %s", strTempKey.c_str());
        return ERR;
    }
    dlog_info("[Test 4] SM2 密钥生成成功: %s", strTempKey.c_str());

    /* 4.2 生成 CSR */
    bool bReqRet = m_pProvider->reqgen(strCountry,
                                       strState,
                                       strLocality,
                                       strOrg,
                                       strOrgUnit,
                                       strCN,
                                       strTempKey,
                                       Gb35114Crypto_NS::SM2_PRIVATE_KEY_ENCRYPT_PASSWORD,
                                       strTempCSR);
    dlog_info("[Test 4] CSR 请求文件生成结果: %s, 路径: %s", bReqRet ? "成功" : "失败", strTempCSR.c_str());

    /* 4.3 解析 CSR */
    std::string strCsrInfo = m_pProvider->reqparse(strTempCSR);
    dlog_info("[Test 4] CSR 解析结果 (CN = %s): \n%s", strCN.c_str(), strCsrInfo.c_str());

    if (bReqRet)
    {
        dlog_info("[Test 4] 证书请求生成与解析自测通过");
    }
    else
    {
        dlog_error("[Test 4] 证书自检测失败");
        unlink(strTempKey.c_str());
        unlink(strTempPubKey.c_str());
        unlink(strTempCSR.c_str());
        return ERR;
    }

    /* 5. 测试注册链路使用的 SM2 加密私钥签名和验签 */
    std::vector<uint8_t> vecSignMsg = { 'G', 'B', '3', '5', '1', '1', '4', '-', 'R', 'E', 'G' };
    std::string strSignature;
    if (m_pProvider->digital_signature(vecSignMsg, strSignature, strTempKey) != OK)
    {
        dlog_error("[Test 5] SM2 数字签名失败");
        unlink(strTempKey.c_str());
        unlink(strTempPubKey.c_str());
        unlink(strTempCSR.c_str());
        return ERR;
    }

    std::string strSignMsg(reinterpret_cast<const char *>(vecSignMsg.data()), vecSignMsg.size());
    if (!m_pProvider->sm2_verify(strTempPubKey, strSignMsg, strSignature, Gb35114Crypto_NS::SM2_DEFAULT_ID))
    {
        dlog_error("[Test 5] SM2 数字签名验签失败");
        unlink(strTempKey.c_str());
        unlink(strTempPubKey.c_str());
        unlink(strTempCSR.c_str());
        return ERR;
    }
    dlog_info("[Test 5] SM2 加密私钥签名自测通过");

    /* 清理临时文件 */
    unlink(strTempKey.c_str());
    unlink(strTempPubKey.c_str());
    unlink(strTempCSR.c_str());
    dlog_info("[Test 4] 临时自测密钥、公钥及 CSR 已清理");

    dlog_warn("============= CCryptoManager::self_test() 结束 =============");
    return OK;
}
