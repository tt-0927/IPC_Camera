/**
 * @FilePath     : hi_crypto_provider.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-06-16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-22 09:37:53
 * @Description  : 海思安全子系统 ICryptoProvider 后端实现
 */

#include "hi_crypto_provider.h"

#include "cipher_context.h"
#include "dlog.h"
#include "openssl_provider.h"

#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/asn1.h>
#include <openssl/err.h>
#include <openssl/param_build.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace
{
/* SM3 摘要固定长度 */
constexpr size_t SM3_DIGEST_LEN = 32;
/* SM4 block/key/iv 固定长度 */
constexpr size_t SM4_BLOCK_LEN = 16;
/* SM4 key/iv 以 16 字节十六进制文本输入 */
constexpr size_t SM4_HEX_LEN = 32;
/* SM2 私钥、公钥坐标、签名 r/s 固定长度 */
constexpr size_t SM2_KEY_LEN = 32;
/* 海思 SM2 raw 密文固定附加长度：0x04 + X + Y + C3 */
constexpr size_t SM2_CIPHER_ADD_LEN = 97;

/**
 * @brief   : 字节数组转十六进制字符串
 * @param    {uint8_t*} data：字节数组指针
 * @param    {size_t} len：字节长度
 * @return   {std::string} 小写十六进制字符串
 */
std::string bytes_to_hex(const uint8_t *data, size_t len)
{
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i)
    {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    return oss.str();
}

/**
 * @brief   : 十六进制字符转数值
 * @param    {char} ch：十六进制字符
 * @return   {int} 0-15：成功，-1：非法字符
 */
int hex_value(char ch)
{
    if (ch >= '0' && ch <= '9')
    {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f')
    {
        return ch - 'a' + 10;
    }
    if (ch >= 'A' && ch <= 'F')
    {
        return ch - 'A' + 10;
    }
    return -1;
}

/**
 * @brief   : 十六进制字符串转字节数组
 * @param    {std::string} hex：十六进制字符串
 * @param    {std::vector<uint8_t>&} out：输出字节数组
 * @return   {bool} true：成功，false：失败
 */
bool hex_to_bytes(const std::string &hex, std::vector<uint8_t> &out)
{
    if (hex.size() % 2 != 0)
    {
        return false;
    }

    out.clear();
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2)
    {
        int high = hex_value(hex[i]);
        int low = hex_value(hex[i + 1]);
        if (high < 0 || low < 0)
        {
            return false;
        }
        out.push_back(static_cast<uint8_t>((high << 4) | low));
    }
    return true;
}

/**
 * @brief   : 对 SM4-CBC 明文执行 PKCS#7 padding
 * @param    {std::string} plaintext：明文
 * @return   {std::vector<uint8_t>} padding 后的块对齐数据
 */
std::vector<uint8_t> pkcs7_pad(const std::string &plaintext)
{
    std::vector<uint8_t> padded(plaintext.begin(), plaintext.end());
    size_t pad_len = SM4_BLOCK_LEN - (padded.size() % SM4_BLOCK_LEN);
    if (pad_len == 0)
    {
        pad_len = SM4_BLOCK_LEN;
    }
    padded.insert(padded.end(), pad_len, static_cast<uint8_t>(pad_len));
    return padded;
}

/**
 * @brief   : 对 SM4-CBC 解密结果执行 PKCS#7 unpadding
 * @param    {std::vector<uint8_t>&} data：待处理数据
 * @return   {bool} true：padding 合法，false：padding 非法
 */
bool pkcs7_unpad(std::vector<uint8_t> &data)
{
    if (data.empty() || data.size() % SM4_BLOCK_LEN != 0)
    {
        return false;
    }

    uint8_t pad = data.back();
    if (pad == 0 || pad > SM4_BLOCK_LEN || pad > data.size())
    {
        return false;
    }

    for (size_t i = 0; i < pad; ++i)
    {
        if (data[data.size() - 1 - i] != pad)
        {
            return false;
        }
    }
    data.resize(data.size() - pad);
    return true;
}

/**
 * @brief   : 通过 cipher_context 执行一次 SM4-CBC raw block 加解密
 * @param    {std::vector<uint8_t>} key：16 字节 SM4 key
 * @param    {std::vector<uint8_t>} iv：16 字节 CBC IV
 * @param    {std::vector<uint8_t>} input：16 字节对齐输入
 * @param    {std::vector<uint8_t>&} output：输出数据
 * @param    {td_bool} encrypt：TD_TRUE 加密，TD_FALSE 解密
 * @return   {int} OK：成功，非 OK：失败
 */
int hi_sm4_cbc_crypt_raw(const std::vector<uint8_t> &key,
                         const std::vector<uint8_t> &iv,
                         const std::vector<uint8_t> &input,
                         std::vector<uint8_t> &output,
                         td_bool encrypt)
{
    CipherContextNeedParam_S need_param = {};
    need_param.bEnableSymc = TD_TRUE;

    CipherContext_S *ctx = cipherContext_alloc(need_param);
    if (ctx == nullptr)
    {
        return ERR;
    }

    int ret = ctx->cipherContext_init(ctx);
    if (ret != TD_SUCCESS)
    {
        cipherContext_release(ctx);
        return ERR;
    }

    output.assign(input.size(), 0);
    td_u32 output_len = 0;
    ret = ctx->cipherContext_sm4_cbc_crypt(ctx,
                                           key.data(),
                                           static_cast<td_u32>(key.size()),
                                           iv.data(),
                                           static_cast<td_u32>(iv.size()),
                                           input.data(),
                                           static_cast<td_u32>(input.size()),
                                           output.data(),
                                           static_cast<td_u32>(output.size()),
                                           &output_len,
                                           encrypt);
    (td_void) ctx->cipherContext_uninit(ctx);
    cipherContext_release(ctx);

    if (ret != TD_SUCCESS)
    {
        return ERR;
    }
    output.resize(output_len);
    return OK;
}

/**
 * @brief   : 从 EVP_PKEY 的 BIGNUM 参数导出 32 字节定长值
 * @param    {EVP_PKEY*} pkey：OpenSSL 密钥对象
 * @param    {const char*} name：参数名
 * @param    {std::vector<uint8_t>&} out：输出 32 字节数组
 * @return   {bool} true：成功，false：失败
 */
bool get_bn_param_fixed(EVP_PKEY *pkey, const char *name, std::vector<uint8_t> &out)
{
    BIGNUM *bn = nullptr;
    if (pkey == nullptr || EVP_PKEY_get_bn_param(pkey, name, &bn) != 1 || bn == nullptr)
    {
        return false;
    }

    out.assign(SM2_KEY_LEN, 0);
    int ret = BN_bn2binpad(bn, out.data(), static_cast<int>(out.size()));
    BN_free(bn);
    return ret == static_cast<int>(out.size());
}

/**
 * @brief   : 从 PEM 私钥读取 SM2 私钥和公钥坐标
 * @param    {std::string} path：私钥路径
 * @param    {std::string} pass：私钥密码
 * @param    {std::vector<uint8_t>&} priv：输出私钥
 * @param    {std::vector<uint8_t>&} pub_x：输出公钥 X
 * @param    {std::vector<uint8_t>&} pub_y：输出公钥 Y
 * @return   {bool} true：成功，false：失败
 */
bool read_sm2_private_key_raw(const std::string &path,
                              const std::string &pass,
                              std::vector<uint8_t> &priv,
                              std::vector<uint8_t> &pub_x,
                              std::vector<uint8_t> &pub_y)
{
    FILE *fp = fopen(path.c_str(), "r");
    if (fp == nullptr)
    {
        dlog_error("[CHiCryptoProvider] 无法打开 SM2 私钥文件: %s", path.c_str());
        return false;
    }

    EVP_PKEY *pkey = PEM_read_PrivateKey(fp, nullptr, nullptr, const_cast<char *>(pass.c_str()));
    fclose(fp);
    if (pkey == nullptr)
    {
        dlog_error("[CHiCryptoProvider] 无法读取 SM2 私钥: %s", path.c_str());
        return false;
    }

    bool ok = get_bn_param_fixed(pkey, OSSL_PKEY_PARAM_PRIV_KEY, priv) &&
              get_bn_param_fixed(pkey, OSSL_PKEY_PARAM_EC_PUB_X, pub_x) &&
              get_bn_param_fixed(pkey, OSSL_PKEY_PARAM_EC_PUB_Y, pub_y);
    EVP_PKEY_free(pkey);
    return ok;
}

/**
 * @brief   : 从 PEM 公钥或证书读取 SM2 公钥坐标
 * @param    {std::string} path：公钥或证书路径
 * @param    {std::vector<uint8_t>&} pub_x：输出公钥 X
 * @param    {std::vector<uint8_t>&} pub_y：输出公钥 Y
 * @return   {bool} true：成功，false：失败
 */
bool read_sm2_public_key_raw(const std::string &path, std::vector<uint8_t> &pub_x, std::vector<uint8_t> &pub_y)
{
    FILE *fp = fopen(path.c_str(), "r");
    if (fp == nullptr)
    {
        dlog_error("[CHiCryptoProvider] 无法打开 SM2 公钥或证书文件: %s", path.c_str());
        return false;
    }

    EVP_PKEY *pkey = PEM_read_PUBKEY(fp, nullptr, nullptr, nullptr);
    if (pkey == nullptr)
    {
        ERR_clear_error();
        std::rewind(fp);
        X509 *cert = PEM_read_X509(fp, nullptr, nullptr, nullptr);
        if (cert != nullptr)
        {
            pkey = X509_get_pubkey(cert);
            X509_free(cert);
        }
    }
    fclose(fp);

    if (pkey == nullptr)
    {
        dlog_error("[CHiCryptoProvider] 无法从文件读取 SM2 公钥: %s", path.c_str());
        return false;
    }

    bool ok = get_bn_param_fixed(pkey, OSSL_PKEY_PARAM_EC_PUB_X, pub_x) &&
              get_bn_param_fixed(pkey, OSSL_PKEY_PARAM_EC_PUB_Y, pub_y);
    EVP_PKEY_free(pkey);
    return ok;
}

/**
 * @brief   : DER ECDSA/SM2 签名转硬件 r/s
 * @param    {std::vector<uint8_t>} der：DER 签名
 * @param    {std::vector<uint8_t>&} r：输出 32 字节 r
 * @param    {std::vector<uint8_t>&} s：输出 32 字节 s
 * @return   {bool} true：成功，false：失败
 */
bool der_to_rs(const std::vector<uint8_t> &der, std::vector<uint8_t> &r, std::vector<uint8_t> &s)
{
    const unsigned char *p = der.data();
    ECDSA_SIG *sig = d2i_ECDSA_SIG(nullptr, &p, static_cast<long>(der.size()));
    if (sig == nullptr)
    {
        return false;
    }

    const BIGNUM *bn_r = nullptr;
    const BIGNUM *bn_s = nullptr;
    ECDSA_SIG_get0(sig, &bn_r, &bn_s);
    r.assign(SM2_KEY_LEN, 0);
    s.assign(SM2_KEY_LEN, 0);
    bool ok = BN_bn2binpad(bn_r, r.data(), static_cast<int>(r.size())) == static_cast<int>(r.size()) &&
              BN_bn2binpad(bn_s, s.data(), static_cast<int>(s.size())) == static_cast<int>(s.size());
    ECDSA_SIG_free(sig);
    return ok;
}

/**
 * @brief   : 硬件 r/s 签名转 DER ECDSA/SM2 签名
 * @param    {std::vector<uint8_t>} r：32 字节 r
 * @param    {std::vector<uint8_t>} s：32 字节 s
 * @param    {std::vector<uint8_t>&} der：输出 DER 签名
 * @return   {bool} true：成功，false：失败
 */
bool rs_to_der(const std::vector<uint8_t> &r, const std::vector<uint8_t> &s, std::vector<uint8_t> &der)
{
    BIGNUM *bn_r = BN_bin2bn(r.data(), static_cast<int>(r.size()), nullptr);
    BIGNUM *bn_s = BN_bin2bn(s.data(), static_cast<int>(s.size()), nullptr);
    ECDSA_SIG *sig = ECDSA_SIG_new();
    if (bn_r == nullptr || bn_s == nullptr || sig == nullptr || ECDSA_SIG_set0(sig, bn_r, bn_s) != 1)
    {
        BN_free(bn_r);
        BN_free(bn_s);
        ECDSA_SIG_free(sig);
        return false;
    }

    bn_r = nullptr;
    bn_s = nullptr;
    int len = i2d_ECDSA_SIG(sig, nullptr);
    if (len <= 0)
    {
        ECDSA_SIG_free(sig);
        return false;
    }

    der.assign(static_cast<size_t>(len), 0);
    unsigned char *p = der.data();
    bool ok = i2d_ECDSA_SIG(sig, &p) == len;
    ECDSA_SIG_free(sig);
    return ok;
}

/**
 * @brief   : 调用海思 PKE 完成 SM2 签名，输出 DER 签名
 * @param    {std::vector<uint8_t>} priv：raw 私钥
 * @param    {std::vector<uint8_t>} pub_x：raw 公钥 X
 * @param    {std::vector<uint8_t>} pub_y：raw 公钥 Y
 * @param    {std::string} data：待签名数据
 * @param    {std::string} id：SM2 ID
 * @param    {std::vector<uint8_t>&} der：输出 DER 签名
 * @return   {int} OK：成功，非 OK：失败
 */
int hi_sm2_sign_der(const std::vector<uint8_t> &priv,
                    const std::vector<uint8_t> &pub_x,
                    const std::vector<uint8_t> &pub_y,
                    const std::string &data,
                    const std::string &id,
                    std::vector<uint8_t> &der)
{
    CipherContextNeedParam_S need_param = {};
    need_param.bEnablePke = TD_TRUE;
    CipherContext_S *ctx = cipherContext_alloc(need_param);
    if (ctx == nullptr)
    {
        return ERR;
    }

    std::vector<uint8_t> hash(SM3_DIGEST_LEN, 0);
    std::vector<uint8_t> sig_r(SM2_KEY_LEN, 0);
    std::vector<uint8_t> sig_s(SM2_KEY_LEN, 0);
    int ret = ctx->cipherContext_init(ctx);
    if (ret == TD_SUCCESS)
    {
        ret = ctx->cipherContext_sm2_dsa_hash(ctx,
                                              reinterpret_cast<const td_u8 *>(id.data()),
                                              static_cast<td_u32>(id.size()),
                                              pub_x.data(),
                                              static_cast<td_u32>(pub_x.size()),
                                              pub_y.data(),
                                              static_cast<td_u32>(pub_y.size()),
                                              reinterpret_cast<const td_u8 *>(data.data()),
                                              static_cast<td_u32>(data.size()),
                                              hash.data(),
                                              static_cast<td_u32>(hash.size()));
    }
    if (ret == TD_SUCCESS)
    {
        ret = ctx->cipherContext_sm2_sign(ctx,
                                          priv.data(),
                                          static_cast<td_u32>(priv.size()),
                                          hash.data(),
                                          static_cast<td_u32>(hash.size()),
                                          sig_r.data(),
                                          static_cast<td_u32>(sig_r.size()),
                                          sig_s.data(),
                                          static_cast<td_u32>(sig_s.size()));
    }
    (td_void) ctx->cipherContext_uninit(ctx);
    cipherContext_release(ctx);

    if (ret != TD_SUCCESS || !rs_to_der(sig_r, sig_s, der))
    {
        return ERR;
    }
    return OK;
}

/**
 * @brief   : 调用海思 PKE 完成 SM2 DER 签名验签
 * @param    {std::vector<uint8_t>} pub_x：raw 公钥 X
 * @param    {std::vector<uint8_t>} pub_y：raw 公钥 Y
 * @param    {std::string} data：原始数据
 * @param    {std::string} id：SM2 ID
 * @param    {std::vector<uint8_t>} der：DER 签名
 * @return   {int} OK：成功，非 OK：失败
 */
int hi_sm2_verify_der(const std::vector<uint8_t> &pub_x,
                      const std::vector<uint8_t> &pub_y,
                      const std::string &data,
                      const std::string &id,
                      const std::vector<uint8_t> &der)
{
    std::vector<uint8_t> sig_r;
    std::vector<uint8_t> sig_s;
    if (!der_to_rs(der, sig_r, sig_s))
    {
        return ERR;
    }

    CipherContextNeedParam_S need_param = {};
    need_param.bEnablePke = TD_TRUE;
    CipherContext_S *ctx = cipherContext_alloc(need_param);
    if (ctx == nullptr)
    {
        return ERR;
    }

    std::vector<uint8_t> hash(SM3_DIGEST_LEN, 0);
    int ret = ctx->cipherContext_init(ctx);
    if (ret == TD_SUCCESS)
    {
        ret = ctx->cipherContext_sm2_dsa_hash(ctx,
                                              reinterpret_cast<const td_u8 *>(id.data()),
                                              static_cast<td_u32>(id.size()),
                                              pub_x.data(),
                                              static_cast<td_u32>(pub_x.size()),
                                              pub_y.data(),
                                              static_cast<td_u32>(pub_y.size()),
                                              reinterpret_cast<const td_u8 *>(data.data()),
                                              static_cast<td_u32>(data.size()),
                                              hash.data(),
                                              static_cast<td_u32>(hash.size()));
    }
    if (ret == TD_SUCCESS)
    {
        ret = ctx->cipherContext_sm2_verify(ctx,
                                            pub_x.data(),
                                            static_cast<td_u32>(pub_x.size()),
                                            pub_y.data(),
                                            static_cast<td_u32>(pub_y.size()),
                                            hash.data(),
                                            static_cast<td_u32>(hash.size()),
                                            sig_r.data(),
                                            static_cast<td_u32>(sig_r.size()),
                                            sig_s.data(),
                                            static_cast<td_u32>(sig_s.size()));
    }
    (td_void) ctx->cipherContext_uninit(ctx);
    cipherContext_release(ctx);
    return ret == TD_SUCCESS ? OK : ERR;
}

/**
 * @brief   : ASN1_INTEGER 转 32 字节定长数组
 * @param    {ASN1_INTEGER*} value：ASN.1 INTEGER
 * @param    {std::vector<uint8_t>&} out：输出 32 字节数组
 * @return   {bool} true：成功，false：失败
 */
bool asn1_integer_to_fixed32(const ASN1_INTEGER *value, std::vector<uint8_t> &out)
{
    BIGNUM *bn = ASN1_INTEGER_to_BN(value, nullptr);
    if (bn == nullptr)
    {
        return false;
    }

    out.assign(SM2_KEY_LEN, 0);
    int ret = BN_bn2binpad(bn, out.data(), static_cast<int>(out.size()));
    BN_free(bn);
    return ret == static_cast<int>(out.size());
}

/**
 * @brief   : OpenSSL SM2 DER 密文转海思 raw 密文
 * @param    {std::vector<uint8_t>} der：OpenSSL DER 密文
 * @param    {std::vector<uint8_t>&} raw：输出海思 raw 密文，格式为 0x04||X||Y||C3||C2
 * @return   {bool} true：成功，false：失败
 * @note    : OpenSSL SM2 DER 常见结构为 SEQUENCE{x INTEGER,y INTEGER,hash OCTET STRING,cipher OCTET STRING}。
 */
bool sm2_cipher_der_to_hisi_raw(const std::vector<uint8_t> &der, std::vector<uint8_t> &raw)
{
    const unsigned char *p = der.data();
    STACK_OF(ASN1_TYPE) *seq = d2i_ASN1_SEQUENCE_ANY(nullptr, &p, static_cast<long>(der.size()));
    if (seq == nullptr || sk_ASN1_TYPE_num(seq) != 4)
    {
        sk_ASN1_TYPE_pop_free(seq, ASN1_TYPE_free);
        return false;
    }

    ASN1_TYPE *x_type = sk_ASN1_TYPE_value(seq, 0);
    ASN1_TYPE *y_type = sk_ASN1_TYPE_value(seq, 1);
    ASN1_TYPE *hash_type = sk_ASN1_TYPE_value(seq, 2);
    ASN1_TYPE *cipher_type = sk_ASN1_TYPE_value(seq, 3);
    std::vector<uint8_t> x;
    std::vector<uint8_t> y;
    bool ok = x_type != nullptr && y_type != nullptr && hash_type != nullptr && cipher_type != nullptr &&
              x_type->type == V_ASN1_INTEGER && y_type->type == V_ASN1_INTEGER && hash_type->type == V_ASN1_OCTET_STRING &&
              cipher_type->type == V_ASN1_OCTET_STRING && asn1_integer_to_fixed32(x_type->value.integer, x) &&
              asn1_integer_to_fixed32(y_type->value.integer, y) && hash_type->value.octet_string != nullptr &&
              hash_type->value.octet_string->length == static_cast<int>(SM3_DIGEST_LEN) &&
              cipher_type->value.octet_string != nullptr && cipher_type->value.octet_string->length > 0;
    if (ok)
    {
        ASN1_OCTET_STRING *hash = hash_type->value.octet_string;
        ASN1_OCTET_STRING *cipher = cipher_type->value.octet_string;
        raw.clear();
        raw.reserve(static_cast<size_t>(cipher->length) + SM2_CIPHER_ADD_LEN);
        raw.push_back(0x04);
        raw.insert(raw.end(), x.begin(), x.end());
        raw.insert(raw.end(), y.begin(), y.end());
        raw.insert(raw.end(), hash->data, hash->data + hash->length);
        raw.insert(raw.end(), cipher->data, cipher->data + cipher->length);
    }

    sk_ASN1_TYPE_pop_free(seq, ASN1_TYPE_free);
    return ok;
}

/**
 * @brief   : 调用海思 PKE 解密 SM2 raw 密文
 * @param    {std::vector<uint8_t>} priv：raw 私钥
 * @param    {std::vector<uint8_t>} raw_cipher：海思 raw 密文
 * @param    {std::vector<uint8_t>&} plain：输出明文
 * @return   {int} OK：成功，非 OK：失败
 */
int hi_sm2_decrypt_raw(const std::vector<uint8_t> &priv, const std::vector<uint8_t> &raw_cipher, std::vector<uint8_t> &plain)
{
    if (priv.size() != SM2_KEY_LEN || raw_cipher.size() <= SM2_CIPHER_ADD_LEN)
    {
        return ERR_PARAM;
    }

    CipherContextNeedParam_S need_param = {};
    need_param.bEnablePke = TD_TRUE;
    CipherContext_S *ctx = cipherContext_alloc(need_param);
    if (ctx == nullptr)
    {
        return ERR;
    }

    plain.assign(raw_cipher.size() - SM2_CIPHER_ADD_LEN, 0);
    td_u32 plain_len = 0;
    int ret = ctx->cipherContext_init(ctx);
    if (ret == TD_SUCCESS)
    {
        ret = ctx->cipherContext_sm2_decrypt(ctx,
                                             priv.data(),
                                             static_cast<td_u32>(priv.size()),
                                             raw_cipher.data(),
                                             static_cast<td_u32>(raw_cipher.size()),
                                             plain.data(),
                                             static_cast<td_u32>(plain.size()),
                                             &plain_len);
    }
    (td_void) ctx->cipherContext_uninit(ctx);
    cipherContext_release(ctx);

    if (ret != TD_SUCCESS)
    {
        return ERR;
    }
    plain.resize(plain_len);
    return OK;
}

/**
 * @brief   : 调用海思 PKE 生成 SM2 raw 密钥对
 * @param    {std::vector<uint8_t>&} priv：输出 32 字节私钥
 * @param    {std::vector<uint8_t>&} pub_x：输出 32 字节公钥 X
 * @param    {std::vector<uint8_t>&} pub_y：输出 32 字节公钥 Y
 * @return   {int} OK：成功，非 OK：失败
 */
int hi_sm2_keygen_raw(std::vector<uint8_t> &priv, std::vector<uint8_t> &pub_x, std::vector<uint8_t> &pub_y)
{
    CipherContextNeedParam_S need_param = {};
    need_param.bEnablePke = TD_TRUE;
    CipherContext_S *ctx = cipherContext_alloc(need_param);
    if (ctx == nullptr)
    {
        return ERR;
    }

    priv.assign(SM2_KEY_LEN, 0);
    pub_x.assign(SM2_KEY_LEN, 0);
    pub_y.assign(SM2_KEY_LEN, 0);
    int ret = ctx->cipherContext_init(ctx);
    if (ret == TD_SUCCESS)
    {
        ret = ctx->cipherContext_sm2_keygen(ctx,
                                            priv.data(),
                                            static_cast<td_u32>(priv.size()),
                                            pub_x.data(),
                                            static_cast<td_u32>(pub_x.size()),
                                            pub_y.data(),
                                            static_cast<td_u32>(pub_y.size()));
    }
    (td_void) ctx->cipherContext_uninit(ctx);
    cipherContext_release(ctx);
    return ret == TD_SUCCESS ? OK : ERR;
}

/**
 * @brief   : 输出 OpenSSL 错误栈首个错误
 * @param    {const char*} context：错误上下文
 * @return   {void}
 */
void log_openssl_error(const char *context)
{
    unsigned long err = ERR_get_error();
    if (err == 0)
    {
        dlog_error("[CHiCryptoProvider] %s, openssl_error=none", context);
        return;
    }

    char err_buf[256] = { 0 };
    ERR_error_string_n(err, err_buf, sizeof(err_buf));
    dlog_error("[CHiCryptoProvider] %s, openssl_error=%s", context, err_buf);
    ERR_clear_error();
}

/**
 * @brief   : 尝试按指定 OpenSSL keymgmt 类型导入 SM2 raw key
 * @param    {const char*} key_type：OpenSSL keymgmt 名称，空表示使用 legacy EVP_PKEY_SM2 id
 * @param    {BIGNUM*} priv_bn：私钥 BN
 * @param    {std::vector<uint8_t>} pub_key：未压缩公钥点
 * @return   {EVP_PKEY*} 非空：成功，nullptr：失败
 */
EVP_PKEY *try_create_sm2_pkey_from_raw(const char *key_type, const BIGNUM *priv_bn, const std::vector<uint8_t> &pub_key)
{
    OSSL_PARAM_BLD *bld = OSSL_PARAM_BLD_new();
    EVP_PKEY_CTX *ctx = key_type == nullptr ? EVP_PKEY_CTX_new_id(EVP_PKEY_SM2, nullptr)
                                            : EVP_PKEY_CTX_new_from_name(nullptr, key_type, nullptr);
    EVP_PKEY *pkey = nullptr;
    OSSL_PARAM *params = nullptr;
    bool ok = bld != nullptr && ctx != nullptr &&
              OSSL_PARAM_BLD_push_utf8_string(bld, OSSL_PKEY_PARAM_GROUP_NAME, "SM2", 0) == 1 &&
              OSSL_PARAM_BLD_push_BN_pad(bld, OSSL_PKEY_PARAM_PRIV_KEY, priv_bn, SM2_KEY_LEN) == 1 &&
              OSSL_PARAM_BLD_push_octet_string(bld, OSSL_PKEY_PARAM_PUB_KEY, pub_key.data(), pub_key.size()) == 1;
    if (ok)
    {
        params = OSSL_PARAM_BLD_to_param(bld);
        ok = params != nullptr && EVP_PKEY_fromdata_init(ctx) == 1 &&
             EVP_PKEY_fromdata(ctx, &pkey, EVP_PKEY_KEYPAIR, params) == 1 && pkey != nullptr;
    }

    OSSL_PARAM_free(params);
    EVP_PKEY_CTX_free(ctx);
    OSSL_PARAM_BLD_free(bld);
    if (!ok)
    {
        EVP_PKEY_free(pkey);
        return nullptr;
    }
    return pkey;
}

/**
 * @brief   : 用海思 raw SM2 密钥材料构造 OpenSSL EVP_PKEY
 * @param    {std::vector<uint8_t>} priv：32 字节私钥
 * @param    {std::vector<uint8_t>} pub_x：32 字节公钥 X
 * @param    {std::vector<uint8_t>} pub_y：32 字节公钥 Y
 * @return   {EVP_PKEY*} 非空：成功，nullptr：失败，调用方负责 EVP_PKEY_free
 * @note    : OpenSSL 只承担 PEM/X.509 文件编码职责，密钥随机生成由海思 PKE 完成。
 */
EVP_PKEY *create_sm2_pkey_from_raw(const std::vector<uint8_t> &priv,
                                   const std::vector<uint8_t> &pub_x,
                                   const std::vector<uint8_t> &pub_y)
{
    if (priv.size() != SM2_KEY_LEN || pub_x.size() != SM2_KEY_LEN || pub_y.size() != SM2_KEY_LEN)
    {
        return nullptr;
    }

    BIGNUM *priv_bn = BN_bin2bn(priv.data(), static_cast<int>(priv.size()), nullptr);
    if (priv_bn == nullptr)
    {
        return nullptr;
    }

    /* SM2 公钥按未压缩点格式封装，供 OpenSSL PEM 写入和 CSR 设置公钥使用。 */
    std::vector<uint8_t> pub_key(1 + SM2_KEY_LEN * 2, 0);
    pub_key[0] = 0x04;
    std::copy(pub_x.begin(), pub_x.end(), pub_key.begin() + 1);
    std::copy(pub_y.begin(), pub_y.end(), pub_key.begin() + 1 + SM2_KEY_LEN);

    /*
     * note: 板端 OpenSSL 的 SM2 导入能力依赖 provider/legacy 组合。优先使用 EVP_PKEY_SM2，
     * 再尝试 SM2/EC keymgmt，避免因 keymgmt 名称差异阻断硬件 keygen 后的 PEM 封装。
     */
    EVP_PKEY *pkey = try_create_sm2_pkey_from_raw(nullptr, priv_bn, pub_key);
    if (pkey == nullptr)
    {
        pkey = try_create_sm2_pkey_from_raw("SM2", priv_bn, pub_key);
    }
    if (pkey == nullptr)
    {
        pkey = try_create_sm2_pkey_from_raw("EC", priv_bn, pub_key);
    }
    BN_free(priv_bn);
    if (pkey == nullptr)
    {
        log_openssl_error("raw SM2 key 导入 EVP_PKEY 失败");
        return nullptr;
    }
    return pkey;
}

/**
 * @brief   : 输出 fallback 原因
 * @param    {const char*} method：方法名
 * @param    {const char*} reason：回退原因
 * @return   {void}
 */
__attribute__((unused)) void log_fallback_once(bool &logged, const char *method, const char *reason)
{
    static std::mutex s_mtxLog;
    std::lock_guard<std::mutex> lock(s_mtxLog);
    if (!logged)
    {
        dlog_warn("[CHiCryptoProvider] %s 回退到 OpenSSL: %s", method, reason);
        logged = true;
    }
}
} // namespace

CHiCryptoProvider::CHiCryptoProvider()
{
}

CHiCryptoProvider::~CHiCryptoProvider()
{
    if (m_bInitialized)
    {
        deinit();
    }
}

IpcRet_E CHiCryptoProvider::init()
{
    if (m_bInitialized)
    {
        return OK;
    }

    IpcRet_E ret = COpenSSLProvider::instance()->init();
    if (ret != OK)
    {
        dlog_error("[CHiCryptoProvider] OpenSSL fallback 初始化失败, ret=%d", ret);
        return ret;
    }

    m_bInitialized = true;
    dlog_info("[CHiCryptoProvider] init 完成: hardware=TRNG,SM3,SM4-CBC,SM2-keygen,SM2-sign,SM2-verify,SM2-decrypt "
              "fallback=Base64,CSR,CERT,CRL");
    return OK;
}

IpcRet_E CHiCryptoProvider::deinit()
{
    if (!m_bInitialized)
    {
        return OK;
    }

    COpenSSLProvider::instance()->deinit();
    m_bInitialized = false;
    return OK;
}

bool CHiCryptoProvider::is_ready() const
{
    return m_bInitialized && COpenSSLProvider::instance()->is_ready();
}

const char *CHiCryptoProvider::name() const
{
    return "hisi";
}

std::string CHiCryptoProvider::rand_bytes(size_t length, bool hex_output)
{
    if (length == 0 || length > 1024)
    {
        dlog_error("[CHiCryptoProvider] rand_bytes 参数错误, length=%zu", length);
        return "";
    }

    std::vector<uint8_t> data(length, 0);
    if (rand_bytes(data.data(), data.size()) != OK)
    {
        return "";
    }

    if (hex_output)
    {
        return bytes_to_hex(data.data(), data.size());
    }
    return std::string(reinterpret_cast<const char *>(data.data()), data.size());
}

int CHiCryptoProvider::rand_bytes(uint8_t *buf, size_t buflen)
{
    if (buf == nullptr || buflen == 0 || buflen > 1024)
    {
        return ERR_PARAM;
    }

    CipherContextNeedParam_S need_param = {};
    need_param.bEnableTrng = TD_TRUE;
    CipherContext_S *ctx = cipherContext_alloc(need_param);
    if (ctx == nullptr)
    {
        return ERR;
    }

    int ret = ctx->cipherContext_init(ctx);
    if (ret == TD_SUCCESS)
    {
        ret = ctx->cipherContext_trng_get_bytes(ctx, buf, static_cast<td_u32>(buflen));
    }
    (td_void) ctx->cipherContext_uninit(ctx);
    cipherContext_release(ctx);

    return ret == TD_SUCCESS ? OK : ERR;
}

std::vector<uint8_t> CHiCryptoProvider::sm3_hash(const std::vector<uint8_t> &data)
{
    CipherContextNeedParam_S need_param = {};
    need_param.bEnableHash = TD_TRUE;
    CipherContext_S *ctx = cipherContext_alloc(need_param);
    if (ctx == nullptr)
    {
        return {};
    }

    std::vector<uint8_t> hash(SM3_DIGEST_LEN, 0);
    td_u32 out_len = 0;
    int ret = ctx->cipherContext_init(ctx);
    if (ret == TD_SUCCESS)
    {
        const td_u8 *input = data.empty() ? nullptr : data.data();
        ret = ctx->cipherContext_sm3_compute(ctx,
                                             input,
                                             static_cast<td_u32>(data.size()),
                                             hash.data(),
                                             static_cast<td_u32>(hash.size()),
                                             &out_len);
    }
    (td_void) ctx->cipherContext_uninit(ctx);
    cipherContext_release(ctx);

    if (ret != TD_SUCCESS || out_len != SM3_DIGEST_LEN)
    {
        return {};
    }
    return hash;
}

std::string CHiCryptoProvider::sm4_encrypt_cbc(const std::string &key, const std::string &iv, const std::string &plaintext)
{
    std::vector<uint8_t> key_bytes;
    std::vector<uint8_t> iv_bytes;
    if (key.size() != SM4_HEX_LEN || iv.size() != SM4_HEX_LEN || !hex_to_bytes(key, key_bytes) || !hex_to_bytes(iv, iv_bytes))
    {
        dlog_error("[CHiCryptoProvider] sm4_encrypt_cbc key/iv 参数错误");
        return "";
    }

    std::vector<uint8_t> padded = pkcs7_pad(plaintext);
    std::vector<uint8_t> cipher;
    if (hi_sm4_cbc_crypt_raw(key_bytes, iv_bytes, padded, cipher, TD_TRUE) != OK)
    {
        dlog_error("[CHiCryptoProvider] sm4_encrypt_cbc 硬件加密失败");
        return "";
    }

    return COpenSSLProvider::instance()->base64_encode_to_string(cipher);
}

std::string CHiCryptoProvider::sm4_decrypt_cbc(const std::string &key, const std::string &iv, const std::string &ciphertext)
{
    std::vector<uint8_t> key_bytes;
    std::vector<uint8_t> iv_bytes;
    if (key.size() != SM4_HEX_LEN || iv.size() != SM4_HEX_LEN || !hex_to_bytes(key, key_bytes) || !hex_to_bytes(iv, iv_bytes))
    {
        dlog_error("[CHiCryptoProvider] sm4_decrypt_cbc key/iv 参数错误");
        return "";
    }

    std::vector<uint8_t> cipher = COpenSSLProvider::instance()->base64_decode(ciphertext);
    if (cipher.empty() || cipher.size() % SM4_BLOCK_LEN != 0)
    {
        dlog_error("[CHiCryptoProvider] sm4_decrypt_cbc 密文格式错误");
        return "";
    }

    std::vector<uint8_t> plain;
    if (hi_sm4_cbc_crypt_raw(key_bytes, iv_bytes, cipher, plain, TD_FALSE) != OK || !pkcs7_unpad(plain))
    {
        dlog_error("[CHiCryptoProvider] sm4_decrypt_cbc 硬件解密失败");
        return "";
    }

    return std::string(reinterpret_cast<const char *>(plain.data()), plain.size());
}

std::string CHiCryptoProvider::sm2_sign(const std::string &privkey_path,
                                        const std::string &pass,
                                        const std::string &data,
                                        const std::string &id)
{
    std::vector<uint8_t> priv;
    std::vector<uint8_t> pub_x;
    std::vector<uint8_t> pub_y;
    std::vector<uint8_t> der;
    if (!read_sm2_private_key_raw(privkey_path, pass, priv, pub_x, pub_y) ||
        hi_sm2_sign_der(priv, pub_x, pub_y, data, id, der) != OK)
    {
        dlog_error("[CHiCryptoProvider] sm2_sign 硬件签名失败");
        return "";
    }

    return COpenSSLProvider::instance()->base64_encode_to_string(der);
}

bool CHiCryptoProvider::sm2_verify(const std::string &pubkey_path,
                                   const std::string &data,
                                   const std::string &signature,
                                   const std::string &id)
{
    std::vector<uint8_t> pub_x;
    std::vector<uint8_t> pub_y;
    std::vector<uint8_t> der = COpenSSLProvider::instance()->base64_decode(signature);
    if (der.empty() || !read_sm2_public_key_raw(pubkey_path, pub_x, pub_y))
    {
        dlog_error("[CHiCryptoProvider] sm2_verify 参数或公钥解析失败");
        return false;
    }

    return hi_sm2_verify_der(pub_x, pub_y, data, id, der) == OK;
}

std::string CHiCryptoProvider::sm2_decrypt(const std::vector<uint8_t> &input, const std::string &key_path)
{
    std::vector<uint8_t> priv;
    std::vector<uint8_t> pub_x;
    std::vector<uint8_t> pub_y;
    std::vector<uint8_t> raw_cipher;
    std::vector<uint8_t> plain;

    if (!read_sm2_private_key_raw(key_path, Gb35114Crypto_NS::SM2_PRIVATE_KEY_ENCRYPT_PASSWORD, priv, pub_x, pub_y))
    {
        dlog_error("[CHiCryptoProvider] sm2_decrypt 读取私钥失败: %s", key_path.c_str());
        return "";
    }

    if (!sm2_cipher_der_to_hisi_raw(input, raw_cipher))
    {
        dlog_error("[CHiCryptoProvider] sm2_decrypt DER 密文转海思 raw 密文失败, len=%zu", input.size());
        return "";
    }

    if (hi_sm2_decrypt_raw(priv, raw_cipher, plain) != OK)
    {
        dlog_error("[CHiCryptoProvider] sm2_decrypt 硬件解密失败, raw_len=%zu", raw_cipher.size());
        return "";
    }

    return std::string(reinterpret_cast<const char *>(plain.data()), plain.size());
}

bool CHiCryptoProvider::reqgen(const std::string &country,
                               const std::string &state,
                               const std::string &locality,
                               const std::string &organization,
                               const std::string &organization_unit,
                               const std::string &common_name,
                               const std::string &key_file,
                               const std::string &password,
                               const std::string &output_file)
{
    // static bool s_bLogged = false;
    // log_fallback_once(s_bLogged, "reqgen", "CSR/X.509 编码不属于海思安全子系统算法能力");
    return COpenSSLProvider::instance()
        ->reqgen(country, state, locality, organization, organization_unit, common_name, key_file, password, output_file);
}

std::string CHiCryptoProvider::certparse(const std::string &cert_path)
{
    // static bool s_bLogged = false;
    // log_fallback_once(s_bLogged, "certparse", "X.509 证书解析不属于海思安全子系统算法能力");
    return COpenSSLProvider::instance()->certparse(cert_path);
}

std::string CHiCryptoProvider::reqparse(const std::string &req_path)
{
    // static bool s_bLogged = false;
    // log_fallback_once(s_bLogged, "reqparse", "CSR 解析不属于海思安全子系统算法能力");
    return COpenSSLProvider::instance()->reqparse(req_path);
}

std::string CHiCryptoProvider::crlparse(const std::string &crl_path)
{
    // static bool s_bLogged = false;
    // log_fallback_once(s_bLogged, "crlparse", "CRL 解析不属于海思安全子系统算法能力");
    return COpenSSLProvider::instance()->crlparse(crl_path);
}

std::string CHiCryptoProvider::crlverify(const std::string &crl_path, const std::string &cert_path)
{
    // static bool s_bLogged = false;
    // log_fallback_once(s_bLogged, "crlverify", "CRL 验证依赖 X.509 公钥对象，不属于海思安全子系统直接算法能力");
    return COpenSSLProvider::instance()->crlverify(crl_path, cert_path);
}

int CHiCryptoProvider::digital_signature(const std::vector<uint8_t> &input_data,
                                         std::string &out_data,
                                         const std::string &key_path)
{
    if (input_data.empty() || key_path.empty())
    {
        return ERR_PARAM;
    }

    std::string data_str(reinterpret_cast<const char *>(input_data.data()), input_data.size());
    out_data = sm2_sign(key_path, Gb35114Crypto_NS::SM2_PRIVATE_KEY_ENCRYPT_PASSWORD, data_str, Gb35114Crypto_NS::SM2_DEFAULT_ID);
    return out_data.empty() ? ERR : OK;
}

int CHiCryptoProvider::verify_signature(const std::vector<uint8_t> &sign_data,
                                        const std::vector<uint8_t> &verify_sign,
                                        const std::string &cert_path)
{
    if (sign_data.empty() || verify_sign.empty() || cert_path.empty())
    {
        return ERR_PARAM;
    }

    std::vector<uint8_t> pub_x;
    std::vector<uint8_t> pub_y;
    if (!read_sm2_public_key_raw(cert_path, pub_x, pub_y))
    {
        return ERR;
    }

    std::string data_str(reinterpret_cast<const char *>(sign_data.data()), sign_data.size());
    return hi_sm2_verify_der(pub_x, pub_y, data_str, Gb35114Crypto_NS::SM2_DEFAULT_ID, verify_sign);
}

std::vector<uint8_t> CHiCryptoProvider::base64_encode(const std::vector<uint8_t> &input)
{
    // static bool s_bLogged = false;
    // log_fallback_once(s_bLogged, "base64_encode", "Base64 编码不属于海思安全子系统算法能力");
    return COpenSSLProvider::instance()->base64_encode(input);
}

std::vector<uint8_t> CHiCryptoProvider::base64_encode(const std::string &input)
{
    // static bool s_bLogged = false;
    // log_fallback_once(s_bLogged, "base64_encode", "Base64 编码不属于海思安全子系统算法能力");
    return COpenSSLProvider::instance()->base64_encode(input);
}

std::vector<uint8_t> CHiCryptoProvider::base64_decode(const std::string &input)
{
    // static bool s_bLogged = false;
    // log_fallback_once(s_bLogged, "base64_decode", "Base64 解码不属于海思安全子系统算法能力");
    return COpenSSLProvider::instance()->base64_decode(input);
}

std::vector<uint8_t> CHiCryptoProvider::base64_decode(const char *input)
{
    // static bool s_bLogged = false;
    // log_fallback_once(s_bLogged, "base64_decode", "Base64 解码不属于海思安全子系统算法能力");
    return COpenSSLProvider::instance()->base64_decode(input);
}

std::string CHiCryptoProvider::base64_encode_to_string(const std::string &input)
{
    // static bool s_bLogged = false;
    // log_fallback_once(s_bLogged, "base64_encode_to_string", "Base64 编码不属于海思安全子系统算法能力");
    return COpenSSLProvider::instance()->base64_encode_to_string(input);
}

std::string CHiCryptoProvider::base64_encode_to_string(const std::vector<uint8_t> &input)
{
    // static bool s_bLogged = false;
    // log_fallback_once(s_bLogged, "base64_encode_to_string", "Base64 编码不属于海思安全子系统算法能力");
    return COpenSSLProvider::instance()->base64_encode_to_string(input);
}

int CHiCryptoProvider::sm2keygen(const std::string &pass, const std::string &privkey_path, const std::string &pubkey_path)
{
    if (privkey_path.empty())
    {
        dlog_error("[CHiCryptoProvider] sm2keygen 私钥输出路径为空");
        return ERR_PARAM;
    }

    std::vector<uint8_t> priv;
    std::vector<uint8_t> pub_x;
    std::vector<uint8_t> pub_y;
    if (hi_sm2_keygen_raw(priv, pub_x, pub_y) != OK)
    {
        dlog_error("[CHiCryptoProvider] sm2keygen 硬件生成 SM2 密钥失败");
        return ERR;
    }

    EVP_PKEY *pkey = create_sm2_pkey_from_raw(priv, pub_x, pub_y);
    if (pkey == nullptr)
    {
        dlog_error("[CHiCryptoProvider] sm2keygen raw 密钥封装 EVP_PKEY 失败");
        return ERR;
    }

    FILE *priv_file = fopen(privkey_path.c_str(), "wb");
    if (priv_file == nullptr)
    {
        dlog_error("[CHiCryptoProvider] sm2keygen 无法打开私钥输出文件: %s", privkey_path.c_str());
        EVP_PKEY_free(pkey);
        return ERR;
    }
    int ret = PEM_write_PKCS8PrivateKey(priv_file,
                                        pkey,
                                        EVP_aes_256_cbc(),
                                        pass.c_str(),
                                        static_cast<int>(pass.size()),
                                        nullptr,
                                        nullptr);
    fclose(priv_file);
    if (ret != 1)
    {
        dlog_error("[CHiCryptoProvider] sm2keygen 写入加密私钥文件失败: %s", privkey_path.c_str());
        EVP_PKEY_free(pkey);
        return ERR;
    }
    dlog_info("[CHiCryptoProvider] sm2keygen 私钥已写入: %s", privkey_path.c_str());

    if (pubkey_path.empty())
    {
        EVP_PKEY_free(pkey);
        return OK;
    }

    FILE *pub_file = fopen(pubkey_path.c_str(), "wb");
    if (pub_file == nullptr)
    {
        dlog_error("[CHiCryptoProvider] sm2keygen 无法打开公钥输出文件: %s", pubkey_path.c_str());
        EVP_PKEY_free(pkey);
        return ERR;
    }
    ret = PEM_write_PUBKEY(pub_file, pkey);
    fclose(pub_file);
    if (ret != 1)
    {
        dlog_error("[CHiCryptoProvider] sm2keygen 写入公钥文件失败: %s", pubkey_path.c_str());
        EVP_PKEY_free(pkey);
        return ERR;
    }

    dlog_info("[CHiCryptoProvider] sm2keygen 公钥已写入: %s", pubkey_path.c_str());
    EVP_PKEY_free(pkey);
    return OK;
}

int CHiCryptoProvider::sm2keygen(const std::string &pass, const std::string &privkey_path)
{
    return sm2keygen(pass, privkey_path, std::string());
}
