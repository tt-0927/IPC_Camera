/**
 * @FilePath     : openssl_provider.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-26 16:15:49
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-22 09:23:28
 * @Description  : OpenSSL 国密算法软件 Provider 实现
 */

#include "openssl_provider.h"
#include "cert_parser.h"
#include "dlog.h"
#include "IpcRet.h"

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include <openssl/ec.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/x509.h>
#include <cstdio>
#include <sstream>
#include <iomanip>

/* OpenSSL Provider 侧保持与 GmSSL RAND_BYTES_MAX_SIZE 一致，避免反向依赖 gmssl/rand.h */
static constexpr size_t OPENSSL_PROVIDER_RAND_BYTES_MAX_SIZE = 256;

/**
 * @brief   : 静态辅助函数，十六进制字符串转字节数组
 * @param    {const std::string &} hex：十六进制字符串（如 "1A2B3C"）
 * @return   {std::vector<uint8_t>} 字节数组
 * @note    : 输入字符串长度必须为偶数，每两个字符转换为一个字节
 */
static std::vector<uint8_t> hex_to_bytes(const std::string &hex)
{
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2)
    {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

/**
 * @brief   : 静态辅助函数，字节数组转十六进制字符串
 * @param    {const std::vector<uint8_t> &} bytes：字节数组
 * @return   {std::string} 十六进制字符串（小写，如 "1a2b3c"）
 */
static std::string bytes_to_hex(const std::vector<uint8_t> &bytes)
{
    std::ostringstream oss;
    for (auto b : bytes)
    {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return oss.str();
}

/**
 * @brief   : 从 PEM 公钥文件或 PEM 证书文件中读取 SM2 公钥
 * @param    {FILE*} fp：已打开的 PEM 文件句柄
 * @param    {std::string} path：文件路径，仅用于错误日志
 * @return   {EVP_PKEY*} 公钥对象，调用方负责 EVP_PKEY_free；失败返回 nullptr
 * @note    : 旧 gmssl 验签使用 -cert 参数，GB35114 注册链路传入的是 CA 证书路径。
 */
static EVP_PKEY *read_public_key_from_pem_or_cert(FILE *fp, const std::string &path)
{
    EVP_PKEY *pkey = PEM_read_PUBKEY(fp, nullptr, nullptr, nullptr);
    if (pkey)
    {
        return pkey;
    }

    ERR_clear_error();
    std::rewind(fp);

    X509 *cert = PEM_read_X509(fp, nullptr, nullptr, nullptr);
    if (!cert)
    {
        dlog_error("sm2_verify: 无法按公钥或证书读取 PEM 文件 %s", path.c_str());
        return nullptr;
    }

    pkey = X509_get_pubkey(cert);
    X509_free(cert);
    if (!pkey)
    {
        dlog_error("sm2_verify: 无法从证书中提取公钥 %s", path.c_str());
        return nullptr;
    }

    return pkey;
}

/**
 * @brief   : COpenSSLProvider 构造函数
 * @note    : 软件 Provider 依赖 OpenSSL 默认库上下文懒加载算法实现
 */
COpenSSLProvider::COpenSSLProvider()
{
    m_bInitialized = false;
}

/**
 * @brief   : COpenSSLProvider 析构函数
 */
COpenSSLProvider::~COpenSSLProvider()
{
    if (m_bInitialized)
    {
        deinit();
    }
}

IpcRet_E COpenSSLProvider::init()
{
    if (m_bInitialized)
    {
        return OK;
    }

    /* info: OpenSSL 3.x 算法通过默认库上下文懒加载，软件 Provider 只维护统一生命周期状态 */
    m_bInitialized = true;
    dlog_info("[COpenSSLProvider] init 完成");
    return OK;
}

IpcRet_E COpenSSLProvider::deinit()
{
    if (!m_bInitialized)
    {
        return OK;
    }

    m_bInitialized = false;
    dlog_info("[COpenSSLProvider] deinit 完成");
    return OK;
}

bool COpenSSLProvider::is_ready() const
{
    return m_bInitialized;
}

const char *COpenSSLProvider::name() const
{
    return "openssl";
}

/**
 * @brief   : 生成随机数
 * @param    {size_t} length：随机数字节长度
 * @param    {bool} hex_output：是否输出十六进制字符串
 * @return   {std::string} 随机数（二进制或十六进制字符串）
 */
std::string COpenSSLProvider::rand_bytes(size_t length, bool hex_output)
{
    if (length == 0 || length > OPENSSL_PROVIDER_RAND_BYTES_MAX_SIZE)
    {
        dlog_error("rand_bytes: 长度错误（1-%zu）", OPENSSL_PROVIDER_RAND_BYTES_MAX_SIZE);
        return "";
    }

    std::vector<uint8_t> buf(length);
    if (RAND_bytes(buf.data(), static_cast<int>(length)) != 1)
    {
        dlog_error("rand_bytes: RAND_bytes 失败");
        return "";
    }

    if (hex_output)
    {
        return bytes_to_hex(buf);
    }
    else
    {
        return std::string(reinterpret_cast<char *>(buf.data()), buf.size());
    }
}

/**
 * @brief   : 生成随机数并写入调用方缓冲区
 * @param    {uint8_t*} buf：随机数输出缓冲区
 * @param    {size_t} buflen：随机数字节长度
 * @return   {int} OK：成功，ERR_PARAM/ERR：失败
 */
int COpenSSLProvider::rand_bytes(uint8_t *buf, size_t buflen)
{
    if (buf == nullptr || buflen == 0 || buflen > OPENSSL_PROVIDER_RAND_BYTES_MAX_SIZE)
    {
        dlog_error("rand_bytes(buffer): 参数错误");
        return ERR_PARAM;
    }

    if (RAND_bytes(buf, static_cast<int>(buflen)) != 1)
    {
        dlog_error("rand_bytes(buffer): RAND_bytes 失败");
        return ERR;
    }

    return OK;
}

/**
 * @brief   : SM2 签名
 * @param    {const std::string &} privkey_path：私钥文件路径
 * @param    {const std::string &} pass：私钥密码
 * @param    {const std::string &} data：待签名数据
 * @param    {const std::string &} id：SM2 签名 ID
 * @return   {std::string} 签名结果（Base64 编码）
 * @note    : 使用 EVP_DigestSign 接口进行 SM2 签名，支持 SM3 杂凑算法
 */
std::string COpenSSLProvider::sm2_sign(const std::string &privkey_path,
                                       const std::string &pass,
                                       const std::string &data,
                                       const std::string &id)
{
    FILE *fp = fopen(privkey_path.c_str(), "r");
    if (!fp)
    {
        dlog_error("sm2_sign: 无法打开私钥文件 %s", privkey_path.c_str());
        return "";
    }

    EVP_PKEY *pkey = PEM_read_PrivateKey(fp, nullptr, nullptr, const_cast<char *>(pass.c_str()));
    fclose(fp);
    if (!pkey)
    {
        dlog_error("sm2_sign: 无法读取私钥");
        return "";
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
        dlog_error("sm2_sign: 无法创建 EVP_MD_CTX");
        EVP_PKEY_free(pkey);
        return "";
    }

    if (EVP_DigestSignInit(ctx, nullptr, EVP_sm3(), nullptr, pkey) != 1)
    {
        dlog_error("sm2_sign: EVP_DigestSignInit 失败");
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return "";
    }

    EVP_PKEY_CTX *pkey_ctx = EVP_MD_CTX_get_pkey_ctx(ctx);
    if (!pkey_ctx || EVP_PKEY_CTX_set1_id(pkey_ctx, reinterpret_cast<const uint8_t *>(id.data()), id.length()) != 1)
    {
        dlog_error("sm2_sign: 无法设置 SM2 ID");
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return "";
    }

    if (EVP_DigestSignUpdate(ctx, data.data(), data.size()) != 1)
    {
        dlog_error("sm2_sign: EVP_DigestSignUpdate 失败");
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return "";
    }

    size_t sig_len = 0;
    if (EVP_DigestSignFinal(ctx, nullptr, &sig_len) != 1)
    {
        dlog_error("sm2_sign: EVP_DigestSignFinal(获取长度) 失败");
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return "";
    }

    std::vector<uint8_t> signature(sig_len);
    if (EVP_DigestSignFinal(ctx, signature.data(), &sig_len) != 1)
    {
        dlog_error("sm2_sign: EVP_DigestSignFinal 失败");
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return "";
    }

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    signature.resize(sig_len);

    auto base64 = base64_encode(signature);
    return std::string(reinterpret_cast<char *>(base64.data()), base64.size());
}

/**
 * @brief   : SM2 验签
 * @param    {const std::string &} pubkey_path：公钥或证书文件路径
 * @param    {const std::string &} data：原始数据
 * @param    {const std::string &} signature：签名结果
 * @param    {const std::string &} id：SM2 签名 ID
 * @return   {bool} true: 验证成功，false: 验证失败
 */
bool COpenSSLProvider::sm2_verify(const std::string &pubkey_path,
                                  const std::string &data,
                                  const std::string &signature,
                                  const std::string &id)
{
    FILE *fp = fopen(pubkey_path.c_str(), "r");
    if (!fp)
    {
        dlog_error("sm2_verify: 无法打开公钥或证书文件 %s", pubkey_path.c_str());
        return false;
    }

    EVP_PKEY *pkey = read_public_key_from_pem_or_cert(fp, pubkey_path);
    fclose(fp);
    if (!pkey)
    {
        return false;
    }

    auto signature_bytes = base64_decode(signature);
    if (signature_bytes.empty())
    {
        dlog_error("sm2_verify: 签名 Base64 解码失败");
        EVP_PKEY_free(pkey);
        return false;
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
        dlog_error("sm2_verify: 无法创建 EVP_MD_CTX");
        EVP_PKEY_free(pkey);
        return false;
    }

    if (EVP_DigestVerifyInit(ctx, nullptr, EVP_sm3(), nullptr, pkey) != 1)
    {
        dlog_error("sm2_verify: EVP_DigestVerifyInit 失败");
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }

    EVP_PKEY_CTX *pkey_ctx = EVP_MD_CTX_get_pkey_ctx(ctx);
    if (!pkey_ctx || EVP_PKEY_CTX_set1_id(pkey_ctx, reinterpret_cast<const uint8_t *>(id.data()), id.length()) != 1)
    {
        dlog_error("sm2_verify: 无法设置 SM2 ID");
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }

    if (EVP_DigestVerifyUpdate(ctx, data.data(), data.size()) != 1)
    {
        dlog_error("sm2_verify: EVP_DigestVerifyUpdate 失败");
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }

    const int ret = EVP_DigestVerifyFinal(ctx, signature_bytes.data(), signature_bytes.size());
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return ret == 1;
}

/**
 * @brief   : SM2 私钥解密
 * @param    {const std::vector<uint8_t> &} input：密文数据
 * @param    {const std::string &} key_path：私钥文件路径
 * @return   {std::string} 解密后的明文
 */
std::string COpenSSLProvider::sm2_decrypt(const std::vector<uint8_t> &input, const std::string &key_path)
{
    if (input.empty())
    {
        dlog_error("sm2_decrypt: 输入数据为空");
        return "";
    }

    FILE *fp = fopen(key_path.c_str(), "r");
    if (!fp)
    {
        dlog_error("sm2_decrypt: 无法打开私钥文件 %s", key_path.c_str());
        return "";
    }

    EVP_PKEY *pkey = PEM_read_PrivateKey(fp,
                                         nullptr,
                                         nullptr,
                                         const_cast<char *>(Gb35114Crypto_NS::SM2_PRIVATE_KEY_ENCRYPT_PASSWORD));
    fclose(fp);
    if (!pkey)
    {
        dlog_error("sm2_decrypt: 无法读取私钥");
        return "";
    }

    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    if (!ctx)
    {
        dlog_error("sm2_decrypt: 无法创建 EVP_PKEY_CTX");
        EVP_PKEY_free(pkey);
        return "";
    }

    if (EVP_PKEY_decrypt_init(ctx) != 1)
    {
        dlog_error("sm2_decrypt: EVP_PKEY_decrypt_init 失败");
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return "";
    }

    size_t out_len = 0;
    if (EVP_PKEY_decrypt(ctx, nullptr, &out_len, input.data(), input.size()) != 1)
    {
        dlog_error("sm2_decrypt: EVP_PKEY_decrypt(获取长度) 失败");
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return "";
    }

    std::vector<uint8_t> out_buf(out_len);
    if (EVP_PKEY_decrypt(ctx, out_buf.data(), &out_len, input.data(), input.size()) != 1)
    {
        dlog_error("sm2_decrypt: EVP_PKEY_decrypt 失败");
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return "";
    }

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    out_buf.resize(out_len);

    return std::string(reinterpret_cast<char *>(out_buf.data()), out_buf.size());
}

/**
 * @brief   : SM3 杂凑
 * @param    {const std::vector<uint8_t> &} data：输入数据
 * @return   {std::vector<uint8_t>} 32 字节杂凑值
 */
std::vector<uint8_t> COpenSSLProvider::sm3_hash(const std::vector<uint8_t> &data)
{
    if (data.empty())
    {
        dlog_error("sm3_hash: 输入数据为空");
        return std::vector<uint8_t>();
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
        dlog_error("sm3_hash: 无法创建 EVP_MD_CTX");
        return std::vector<uint8_t>();
    }

    if (EVP_DigestInit_ex(ctx, EVP_sm3(), nullptr) != 1)
    {
        dlog_error("sm3_hash: EVP_DigestInit_ex 失败");
        EVP_MD_CTX_free(ctx);
        return std::vector<uint8_t>();
    }

    if (EVP_DigestUpdate(ctx, data.data(), data.size()) != 1)
    {
        dlog_error("sm3_hash: EVP_DigestUpdate 失败");
        EVP_MD_CTX_free(ctx);
        return std::vector<uint8_t>();
    }

    std::vector<uint8_t> hash(EVP_MD_size(EVP_sm3()));
    unsigned int hash_len = 0;
    if (EVP_DigestFinal_ex(ctx, hash.data(), &hash_len) != 1)
    {
        dlog_error("sm3_hash: EVP_DigestFinal_ex 失败");
        EVP_MD_CTX_free(ctx);
        return std::vector<uint8_t>();
    }

    EVP_MD_CTX_free(ctx);
    hash.resize(hash_len);
    return hash;
}

/**
 * @brief   : SM4-CBC 加密
 * @param    {const std::string &} key：16 字节密钥（十六进制字符串）
 * @param    {const std::string &} iv：16 字节 IV（十六进制字符串）
 * @param    {const std::string &} plaintext：明文数据
 * @return   {std::string} 密文（Base64 编码）
 */
std::string COpenSSLProvider::sm4_encrypt_cbc(const std::string &key, const std::string &iv, const std::string &plaintext)
{
    if (key.length() != 32 || iv.length() != 32)
    {
        dlog_error("sm4_encrypt_cbc: 密钥或 IV 长度错误（应为 32 字符十六进制）");
        return "";
    }

    std::vector<uint8_t> key_bytes = hex_to_bytes(key);
    std::vector<uint8_t> iv_bytes = hex_to_bytes(iv);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        dlog_error("sm4_encrypt_cbc: 无法创建 EVP_CIPHER_CTX");
        return "";
    }

    if (EVP_EncryptInit_ex(ctx, EVP_sm4_cbc(), nullptr, key_bytes.data(), iv_bytes.data()) != 1)
    {
        dlog_error("sm4_encrypt_cbc: EVP_EncryptInit_ex 失败");
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }

    std::vector<uint8_t> ciphertext(plaintext.size() + EVP_CIPHER_block_size(EVP_sm4_cbc()));
    int len = 0;
    int ciphertext_len = 0;

    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, reinterpret_cast<const uint8_t *>(plaintext.data()), plaintext.size()) !=
        1)
    {
        dlog_error("sm4_encrypt_cbc: EVP_EncryptUpdate 失败");
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1)
    {
        dlog_error("sm4_encrypt_cbc: EVP_EncryptFinal_ex 失败");
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    ciphertext.resize(ciphertext_len);

    auto base64 = base64_encode_to_string(ciphertext);
    return base64;
}

/**
 * @brief   : SM4-CBC 解密
 * @param    {const std::string &} key：16 字节密钥（十六进制字符串）
 * @param    {const std::string &} iv：16 字节 IV（十六进制字符串）
 * @param    {const std::string &} ciphertext：密文数据（Base64 编码）
 * @return   {std::string} 明文
 */
std::string COpenSSLProvider::sm4_decrypt_cbc(const std::string &key, const std::string &iv, const std::string &ciphertext)
{
    if (key.length() != 32 || iv.length() != 32)
    {
        dlog_error("sm4_decrypt_cbc: 密钥或 IV 长度错误（应为 32 字符十六进制）");
        return "";
    }

    std::vector<uint8_t> key_bytes = hex_to_bytes(key);
    std::vector<uint8_t> iv_bytes = hex_to_bytes(iv);
    std::vector<uint8_t> ciphertext_bytes = base64_decode(ciphertext);
    if (ciphertext_bytes.empty())
    {
        dlog_error("sm4_decrypt_cbc: Base64 解码失败");
        return "";
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        dlog_error("sm4_decrypt_cbc: 无法创建 EVP_CIPHER_CTX");
        return "";
    }

    if (EVP_DecryptInit_ex(ctx, EVP_sm4_cbc(), nullptr, key_bytes.data(), iv_bytes.data()) != 1)
    {
        dlog_error("sm4_decrypt_cbc: EVP_DecryptInit_ex 失败");
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }

    std::vector<uint8_t> plaintext(ciphertext_bytes.size() + EVP_CIPHER_block_size(EVP_sm4_cbc()));
    int len = 0;
    int plaintext_len = 0;

    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext_bytes.data(), static_cast<int>(ciphertext_bytes.size())) != 1)
    {
        dlog_error("sm4_decrypt_cbc: EVP_DecryptUpdate 失败");
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    plaintext_len = len;

    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1)
    {
        dlog_error("sm4_decrypt_cbc: EVP_DecryptFinal_ex 失败");
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    plaintext.resize(plaintext_len);

    return std::string(reinterpret_cast<char *>(plaintext.data()), plaintext.size());
}

/**
 * @brief   : 生成证书请求（CSR）
 * @param    {const std::string &} country：国家代码
 * @param    {const std::string &} state：省份或州名
 * @param    {const std::string &} locality：城市或地区
 * @param    {const std::string &} organization：组织名称
 * @param    {const std::string &} organization_unit：组织单位
 * @param    {const std::string &} common_name：通用名称
 * @param    {const std::string &} key_file：私钥文件路径
 * @param    {const std::string &} password：私钥密码
 * @param    {const std::string &} output_file：输出 CSR 文件路径
 * @return   {bool} true: 成功，false: 失败
 */
bool COpenSSLProvider::reqgen(const std::string &country,
                              const std::string &state,
                              const std::string &locality,
                              const std::string &organization,
                              const std::string &organization_unit,
                              const std::string &common_name,
                              const std::string &key_file,
                              const std::string &password,
                              const std::string &output_file)
{
    if (key_file.empty() || output_file.empty())
    {
        dlog_error("reqgen: 私钥文件或输出文件路径为空");
        return false;
    }

    /* 加载私钥 */
    FILE *fp = fopen(key_file.c_str(), "r");
    if (!fp)
    {
        dlog_error("reqgen: 无法打开私钥文件 %s", key_file.c_str());
        return false;
    }

    EVP_PKEY *pkey = PEM_read_PrivateKey(fp, nullptr, nullptr, const_cast<char *>(password.c_str()));
    fclose(fp);
    if (!pkey)
    {
        unsigned long ulErr = ERR_get_error();
        char szErr[256] = { 0 };
        ERR_error_string_n(ulErr, szErr, sizeof(szErr));
        dlog_error("reqgen: 无法读取私钥: %s, openssl_error=%s", key_file.c_str(), szErr);
        return false;
    }

    /* 创建X509_REQ */
    X509_REQ *req = X509_REQ_new();
    if (!req)
    {
        dlog_error("reqgen: 无法创建X509_REQ");
        EVP_PKEY_free(pkey);
        return false;
    }

    /* 设置版本 */
    if (X509_REQ_set_version(req, 0) != 1)
    {
        dlog_error("reqgen: 设置版本失败");
        X509_REQ_free(req);
        EVP_PKEY_free(pkey);
        return false;
    }

    /* 设置Subject名称 */
    X509_NAME *name = X509_NAME_new();
    if (!name)
    {
        dlog_error("reqgen: 无法创建X509_NAME");
        X509_REQ_free(req);
        EVP_PKEY_free(pkey);
        return false;
    }

    if (!country.empty())
    {
        X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, reinterpret_cast<const unsigned char *>(country.c_str()), -1, -1, 0);
    }
    if (!state.empty())
    {
        X509_NAME_add_entry_by_txt(name, "ST", MBSTRING_ASC, reinterpret_cast<const unsigned char *>(state.c_str()), -1, -1, 0);
    }
    if (!locality.empty())
    {
        X509_NAME_add_entry_by_txt(name, "L", MBSTRING_ASC, reinterpret_cast<const unsigned char *>(locality.c_str()), -1, -1, 0);
    }
    if (!organization.empty())
    {
        X509_NAME_add_entry_by_txt(name,
                                   "O",
                                   MBSTRING_ASC,
                                   reinterpret_cast<const unsigned char *>(organization.c_str()),
                                   -1,
                                   -1,
                                   0);
    }
    if (!organization_unit.empty())
    {
        X509_NAME_add_entry_by_txt(name,
                                   "OU",
                                   MBSTRING_ASC,
                                   reinterpret_cast<const unsigned char *>(organization_unit.c_str()),
                                   -1,
                                   -1,
                                   0);
    }
    if (!common_name.empty())
    {
        X509_NAME_add_entry_by_txt(name,
                                   "CN",
                                   MBSTRING_ASC,
                                   reinterpret_cast<const unsigned char *>(common_name.c_str()),
                                   -1,
                                   -1,
                                   0);
    }

    if (X509_REQ_set_subject_name(req, name) != 1)
    {
        dlog_error("reqgen: 设置Subject名称失败");
        X509_NAME_free(name);
        X509_REQ_free(req);
        EVP_PKEY_free(pkey);
        return false;
    }
    X509_NAME_free(name);

    /* 设置公钥 */
    if (X509_REQ_set_pubkey(req, pkey) != 1)
    {
        dlog_error("reqgen: 设置公钥失败");
        X509_REQ_free(req);
        EVP_PKEY_free(pkey);
        return false;
    }

    /* 使用SM3签名 */
    if (X509_REQ_sign(req, pkey, EVP_sm3()) <= 0)
    {
        dlog_error("reqgen: SM3签名失败");
        X509_REQ_free(req);
        EVP_PKEY_free(pkey);
        return false;
    }

    EVP_PKEY_free(pkey);

    /* 写入输出文件 */
    fp = fopen(output_file.c_str(), "w");
    if (!fp)
    {
        dlog_error("reqgen: 无法创建输出文件 %s", output_file.c_str());
        X509_REQ_free(req);
        return false;
    }

    if (PEM_write_X509_REQ(fp, req) != 1)
    {
        dlog_error("reqgen: 写入CSR文件失败");
        fclose(fp);
        X509_REQ_free(req);
        return false;
    }

    fclose(fp);
    X509_REQ_free(req);

    dlog_info("reqgen: 成功生成CSR %s", output_file.c_str());
    return true;
}

/**
 * @brief   : 解析证书
 * @param    {const std::string &} cert_path：证书文件路径
 * @return   {std::string} 证书信息（结构化文本）
 */
std::string COpenSSLProvider::certparse(const std::string &cert_path)
{
    CertInfo_S info;
    if (!parse_certificate(cert_path, info))
    {
        return "";
    }

    std::ostringstream oss;
    oss << "Certificate:\n";
    oss << "    serialNumber: " << info.strSerialNumber << "\n";
    oss << "    subject:\n";
    oss << "        commonName: " << info.strSubjectCN << "\n";
    oss << "    issuer:\n";
    oss << "        commonName: " << info.strIssuerCN << "\n";
    oss << "    notBefore: " << info.strNotBefore << "\n";
    oss << "    notAfter: " << info.strNotAfter << "\n";
    oss << "    extensions:\n";
    oss << "        KeyUsage:\n";
    oss << "            digitalSignature: " << (info.bKeyUsageDigitalSignature ? "true" : "false") << "\n";
    oss << "            keyCertSign: " << (info.bKeyUsageKeyCertSign ? "true" : "false") << "\n";
    oss << "            cRLSign: " << (info.bKeyUsageCRLSign ? "true" : "false") << "\n";
    oss << "        BasicConstraints:\n";
    oss << "            cA: " << (info.bBasicConstraintsCA ? "true" : "false") << "\n";
    if (info.iPathLenConstraint >= 0)
    {
        oss << "            pathLenConstraint: " << info.iPathLenConstraint << "\n";
    }
    if (!info.vecSubjectAltName.empty())
    {
        oss << "        SubjectAltName:\n";
        for (const auto &san : info.vecSubjectAltName)
        {
            oss << "            " << san << "\n";
        }
    }

    return oss.str();
}

/**
 * @brief   : 解析证书请求
 * @param    {const std::string &} req_path：CSR 文件路径
 * @return   {std::string} CSR 信息
 */
std::string COpenSSLProvider::reqparse(const std::string &req_path)
{
    CertInfo_S info;
    if (!parse_csr(req_path, info))
    {
        return "";
    }

    std::ostringstream oss;
    oss << "Certificate Request:\n";
    oss << "    subject:\n";
    oss << "        commonName: " << info.strSubjectCN << "\n";

    return oss.str();
}

/**
 * @brief   : 解析 CRL
 * @param    {const std::string &} crl_path：CRL 文件路径
 * @return   {std::string} CRL 信息
 */
std::string COpenSSLProvider::crlparse(const std::string &crl_path)
{
    std::string info_str;
    if (!parse_crl(crl_path, info_str))
    {
        return "";
    }
    return info_str;
}

/**
 * @brief   : 验证 CRL
 * @param    {const std::string &} crl_path：CRL 文件路径
 * @param    {const std::string &} cert_path：签发者证书路径
 * @return   {std::string} 验证结果文本
 */
std::string COpenSSLProvider::crlverify(const std::string &crl_path, const std::string &cert_path)
{
    if (verify_crl(crl_path, cert_path))
    {
        return "Verification success";
    }
    return "";
}

/**
 * @brief   : SM2 数字签名
 * @param    {const std::vector<uint8_t> &} input_data：待签名数据
 * @param    {std::string &} out_data：签名输出
 * @param    {const std::string &} key_path：私钥路径
 * @return   {int} OK：成功，非 OK：失败
 */
int COpenSSLProvider::digital_signature(const std::vector<uint8_t> &input_data,
                                        std::string &out_data,
                                        const std::string &key_path)
{
    if (input_data.empty() || key_path.empty())
    {
        dlog_error("digital_signature: 参数错误");
        return ERR_PARAM;
    }

    std::string data_str(reinterpret_cast<const char *>(input_data.data()), input_data.size());
    out_data = sm2_sign(key_path, Gb35114Crypto_NS::SM2_PRIVATE_KEY_ENCRYPT_PASSWORD, data_str, Gb35114Crypto_NS::SM2_DEFAULT_ID);
    if (out_data.empty())
    {
        dlog_error("digital_signature: SM2 签名失败");
        return ERR;
    }

    return OK;
}

/**
 * @brief   : SM2 验签
 * @param    {const std::vector<uint8_t> &} sign_data：原始签名数据
 * @param    {const std::vector<uint8_t> &} verify_sign：待验证签名
 * @param    {const std::string &} cert_path：证书路径
 * @return   {int} OK：验签成功，非 OK：失败
 */
int COpenSSLProvider::verify_signature(const std::vector<uint8_t> &sign_data,
                                       const std::vector<uint8_t> &verify_sign,
                                       const std::string &cert_path)
{
    if (sign_data.empty() || verify_sign.empty() || cert_path.empty())
    {
        dlog_error("verify_signature: 参数错误");
        return ERR_PARAM;
    }

    std::string data_str(reinterpret_cast<const char *>(sign_data.data()), sign_data.size());
    auto sign_base64 = base64_encode(verify_sign);
    std::string sign_str(reinterpret_cast<char *>(sign_base64.data()), sign_base64.size());

    bool ret = sm2_verify(cert_path, data_str, sign_str, Gb35114Crypto_NS::SM2_DEFAULT_ID);
    if (!ret)
    {
        dlog_error("verify_signature: SM2 验签失败");
        return ERR;
    }

    return OK;
}

/**
 * @brief   : Base64 编码
 * @param    {const std::vector<uint8_t> &} input：二进制数据
 * @return   {std::vector<uint8_t>} Base64 编码数据
 */
std::vector<uint8_t> COpenSSLProvider::base64_encode(const std::vector<uint8_t> &input)
{
    BIO *bio = BIO_new(BIO_s_mem());
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);

    BIO_write(bio, input.data(), static_cast<int>(input.size()));
    BIO_flush(bio);

    BUF_MEM *bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    std::vector<uint8_t> result(bufferPtr->data, bufferPtr->data + bufferPtr->length);

    BIO_free_all(bio);
    return result;
}

/**
 * @brief   : Base64 编码
 * @param    {const std::string &} input：原始字符串
 * @return   {std::vector<uint8_t>} Base64 编码数据
 */
std::vector<uint8_t> COpenSSLProvider::base64_encode(const std::string &input)
{
    std::vector<uint8_t> input_vec(input.begin(), input.end());
    return base64_encode(input_vec);
}

/**
 * @brief   : Base64 解码
 * @param    {const std::string &} input：Base64 字符串
 * @return   {std::vector<uint8_t>} 二进制数据
 */
std::vector<uint8_t> COpenSSLProvider::base64_decode(const std::string &input)
{
    BIO *bio = BIO_new_mem_buf(input.data(), static_cast<int>(input.length()));
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);

    std::vector<uint8_t> result(input.length());
    int decoded_len = BIO_read(bio, result.data(), static_cast<int>(input.length()));
    if (decoded_len < 0)
    {
        dlog_error("base64_decode: 解码失败");
        BIO_free_all(bio);
        return std::vector<uint8_t>();
    }
    result.resize(decoded_len);

    BIO_free_all(bio);
    return result;
}

/**
 * @brief   : Base64 解码
 * @param    {const char*} input：Base64 字符串指针
 * @return   {std::vector<uint8_t>} 二进制数据
 */
std::vector<uint8_t> COpenSSLProvider::base64_decode(const char *input)
{
    if (!input)
        return std::vector<uint8_t>();
    return base64_decode(std::string(input));
}

/**
 * @brief   : Base64 编码为字符串
 * @param    {const std::string &} input：原始字符串
 * @return   {std::string} Base64 编码字符串
 */
std::string COpenSSLProvider::base64_encode_to_string(const std::string &input)
{
    std::vector<uint8_t> input_vec(input.begin(), input.end());
    auto encoded = base64_encode(input_vec);
    return std::string(encoded.begin(), encoded.end());
}

/**
 * @brief   : Base64 编码为字符串
 * @param    {const std::vector<uint8_t> &} input：二进制数据
 * @return   {std::string} Base64 编码字符串
 */
std::string COpenSSLProvider::base64_encode_to_string(const std::vector<uint8_t> &input)
{
    auto encoded = base64_encode(input);
    return std::string(encoded.begin(), encoded.end());
}

/**
 * @brief   : 生成 SM2 密钥对（含公钥文件输出）
 * @note    : 使用 OpenSSL EVP_PKEY_CTX 生成 SM2 密钥；私钥以 PKCS8 AES256-CBC 加密格式写入 PEM；
 *            公钥以 SubjectPublicKeyInfo PEM 格式写入
 */
int COpenSSLProvider::sm2keygen(const std::string &pass, const std::string &privkey_path, const std::string &pubkey_path)
{
    if (privkey_path.empty())
    {
        dlog_error("[COpenSSLProvider] sm2keygen: 私钥输出路径为空");
        return ERR_PARAM;
    }

    /* 生成 SM2 密钥对 */
    EVP_PKEY_CTX *pCtx = EVP_PKEY_CTX_new_id(EVP_PKEY_SM2, nullptr);
    if (!pCtx)
    {
        dlog_error("[COpenSSLProvider] sm2keygen: 创建 EVP_PKEY_CTX 失败");
        return ERR;
    }

    if (EVP_PKEY_keygen_init(pCtx) <= 0)
    {
        dlog_error("[COpenSSLProvider] sm2keygen: keygen_init 失败");
        EVP_PKEY_CTX_free(pCtx);
        return ERR;
    }

    EVP_PKEY *pKey = nullptr;
    if (EVP_PKEY_keygen(pCtx, &pKey) <= 0)
    {
        dlog_error("[COpenSSLProvider] sm2keygen: keygen 失败");
        EVP_PKEY_CTX_free(pCtx);
        return ERR;
    }
    EVP_PKEY_CTX_free(pCtx);

    /* 写入 PKCS8 加密私钥到文件 */
    FILE *pPrivFile = fopen(privkey_path.c_str(), "wb");
    if (!pPrivFile)
    {
        dlog_error("[COpenSSLProvider] sm2keygen: 无法打开私钥输出文件: %s", privkey_path.c_str());
        EVP_PKEY_free(pKey);
        return ERR;
    }
    /* PEM_write_PKCS8PrivateKey 使用 AES256-CBC 加密私钥 */
    int nRet = PEM_write_PKCS8PrivateKey(pPrivFile, pKey, EVP_aes_256_cbc(), pass.c_str(), (int) pass.size(), nullptr, nullptr);
    fclose(pPrivFile);
    if (nRet != 1)
    {
        dlog_error("[COpenSSLProvider] sm2keygen: 写入加密私钥文件失败: %s", privkey_path.c_str());
        EVP_PKEY_free(pKey);
        return ERR;
    }
    dlog_info("[COpenSSLProvider] sm2keygen: 私钥已写入: %s", privkey_path.c_str());

    if (pubkey_path.empty())
    {
        EVP_PKEY_free(pKey);
        return OK;
    }

    /* 写入 SubjectPublicKeyInfo PEM 公钥到文件 */
    FILE *pPubFile = fopen(pubkey_path.c_str(), "wb");
    if (!pPubFile)
    {
        dlog_error("[COpenSSLProvider] sm2keygen: 无法打开公钥输出文件: %s", pubkey_path.c_str());
        EVP_PKEY_free(pKey);
        return ERR;
    }
    nRet = PEM_write_PUBKEY(pPubFile, pKey);
    fclose(pPubFile);
    if (nRet != 1)
    {
        dlog_error("[COpenSSLProvider] sm2keygen: 写入公钥文件失败: %s", pubkey_path.c_str());
        EVP_PKEY_free(pKey);
        return ERR;
    }
    else
    {
        dlog_info("[COpenSSLProvider] sm2keygen: 公钥已写入: %s", pubkey_path.c_str());
    }

    EVP_PKEY_free(pKey);
    return OK;
}

/**
 * @brief   : 生成 SM2 密钥对（仅私钥文件输出）
 * @note    : 委托给含公钥输出的重载，传入空字符串跳过公钥写入
 */
int COpenSSLProvider::sm2keygen(const std::string &pass, const std::string &privkey_path)
{
    return sm2keygen(pass, privkey_path, std::string());
}
