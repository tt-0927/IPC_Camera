/**
 * @FilePath     : platform_register_crypto.cpp
 * @Description  : MQTT设备注册凭据加密工具实现
 */

#include "platform_register_crypto.h"

#include <array>
#include <climits>
#include <memory>
#include <vector>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>

namespace
{
constexpr size_t AES_256_KEY_LEN = 32;
constexpr size_t GCM_NONCE_LEN = 12;
constexpr size_t GCM_TAG_LEN = 16;

class OpenSslCleanseGuard
{
public:
    OpenSslCleanseGuard(void *pData, size_t nSize) : m_pData(pData), m_nSize(nSize)
    {
    }

    ~OpenSslCleanseGuard()
    {
        if (m_pData != nullptr && m_nSize > 0)
        {
            OPENSSL_cleanse(m_pData, m_nSize);
        }
    }

private:
    void *m_pData;
    size_t m_nSize;
};

bool is_openssl_length_valid(size_t nSize)
{
    return nSize <= static_cast<size_t>(INT_MAX);
}

std::string base64_encode(const unsigned char *pData, size_t nSize)
{
    if (pData == nullptr || nSize == 0 || !is_openssl_length_valid(nSize))
    {
        return "";
    }

    const size_t nOutputSize = 4 * ((nSize + 2) / 3);
    std::string strEncoded(nOutputSize, '\0');
    const int nWritten = EVP_EncodeBlock(reinterpret_cast<unsigned char *>(&strEncoded[0]),
                                         pData,
                                         static_cast<int>(nSize));
    if (nWritten <= 0)
    {
        return "";
    }

    strEncoded.resize(static_cast<size_t>(nWritten));
    return strEncoded;
}

bool aes_256_gcm_encrypt(const std::array<unsigned char, AES_256_KEY_LEN> &arrKey,
                         const std::array<unsigned char, GCM_NONCE_LEN> &arrNonce,
                         const std::string &strPlaintext,
                         const std::string &strAad,
                         std::vector<unsigned char> &vecCiphertext,
                         std::array<unsigned char, GCM_TAG_LEN> &arrTag)
{
    if (!is_openssl_length_valid(strPlaintext.size()) || !is_openssl_length_valid(strAad.size()))
    {
        return false;
    }

    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> pCtx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!pCtx)
    {
        return false;
    }

    if (EVP_EncryptInit_ex(pCtx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(pCtx.get(), EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(arrNonce.size()), nullptr) != 1 ||
        EVP_EncryptInit_ex(pCtx.get(), nullptr, nullptr, arrKey.data(), arrNonce.data()) != 1)
    {
        return false;
    }

    int nOutputLen = 0;
    if (!strAad.empty() &&
        EVP_EncryptUpdate(pCtx.get(),
                          nullptr,
                          &nOutputLen,
                          reinterpret_cast<const unsigned char *>(strAad.data()),
                          static_cast<int>(strAad.size())) != 1)
    {
        return false;
    }

    vecCiphertext.assign(strPlaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_gcm()), 0);
    if (!strPlaintext.empty() &&
        EVP_EncryptUpdate(pCtx.get(),
                          vecCiphertext.data(),
                          &nOutputLen,
                          reinterpret_cast<const unsigned char *>(strPlaintext.data()),
                          static_cast<int>(strPlaintext.size())) != 1)
    {
        return false;
    }

    int nFinalLen = 0;
    if (EVP_EncryptFinal_ex(pCtx.get(), vecCiphertext.data() + nOutputLen, &nFinalLen) != 1 ||
        EVP_CIPHER_CTX_ctrl(pCtx.get(), EVP_CTRL_GCM_GET_TAG, static_cast<int>(arrTag.size()), arrTag.data()) != 1)
    {
        return false;
    }

    vecCiphertext.resize(static_cast<size_t>(nOutputLen + nFinalLen));
    return true;
}

bool rsa_oaep_sha256_encrypt(const std::string &strPublicKeyPath,
                             const std::array<unsigned char, AES_256_KEY_LEN> &arrKey,
                             std::vector<unsigned char> &vecEncryptedKey)
{
    std::unique_ptr<BIO, decltype(&BIO_free)> pBio(BIO_new_file(strPublicKeyPath.c_str(), "rb"), BIO_free);
    if (!pBio)
    {
        return false;
    }

    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> pPublicKey(PEM_read_bio_PUBKEY(pBio.get(), nullptr, nullptr, nullptr),
                                                                    EVP_PKEY_free);
    if (!pPublicKey || EVP_PKEY_base_id(pPublicKey.get()) != EVP_PKEY_RSA)
    {
        return false;
    }

    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> pCtx(EVP_PKEY_CTX_new(pPublicKey.get(), nullptr),
                                                                       EVP_PKEY_CTX_free);
    if (!pCtx || EVP_PKEY_encrypt_init(pCtx.get()) != 1 ||
        EVP_PKEY_CTX_set_rsa_padding(pCtx.get(), RSA_PKCS1_OAEP_PADDING) <= 0 ||
        EVP_PKEY_CTX_set_rsa_oaep_md(pCtx.get(), EVP_sha256()) <= 0 ||
        EVP_PKEY_CTX_set_rsa_mgf1_md(pCtx.get(), EVP_sha256()) <= 0)
    {
        return false;
    }

    size_t nEncryptedKeyLen = 0;
    if (EVP_PKEY_encrypt(pCtx.get(), nullptr, &nEncryptedKeyLen, arrKey.data(), arrKey.size()) != 1 ||
        nEncryptedKeyLen == 0)
    {
        return false;
    }

    vecEncryptedKey.assign(nEncryptedKeyLen, 0);
    if (EVP_PKEY_encrypt(pCtx.get(), vecEncryptedKey.data(), &nEncryptedKeyLen, arrKey.data(), arrKey.size()) != 1)
    {
        return false;
    }

    vecEncryptedKey.resize(nEncryptedKeyLen);
    return true;
}
} // namespace

namespace PlatformRegisterCrypto
{
bool encrypt_credential(const std::string &strPublicKeyPath,
                        const std::string &strKeyId,
                        const std::string &strPlaintext,
                        const std::string &strAad,
                        EncryptedCredential_S &stOutput,
                        std::string &strError)
{
    stOutput = EncryptedCredential_S();
    strError.clear();

    if (strPublicKeyPath.empty() || strKeyId.empty() || strPlaintext.empty())
    {
        strError = "invalid encryption parameters";
        return false;
    }

    std::array<unsigned char, AES_256_KEY_LEN> arrKey = {};
    std::array<unsigned char, GCM_NONCE_LEN> arrNonce = {};
    std::array<unsigned char, GCM_TAG_LEN> arrTag = {};
    OpenSslCleanseGuard stKeyGuard(arrKey.data(), arrKey.size());

    if (RAND_bytes(arrKey.data(), static_cast<int>(arrKey.size())) != 1 ||
        RAND_bytes(arrNonce.data(), static_cast<int>(arrNonce.size())) != 1)
    {
        strError = "random generation failed";
        return false;
    }

    std::vector<unsigned char> vecCiphertext;
    if (!aes_256_gcm_encrypt(arrKey, arrNonce, strPlaintext, strAad, vecCiphertext, arrTag))
    {
        strError = "AES-GCM encryption failed";
        return false;
    }

    std::vector<unsigned char> vecEncryptedKey;
    if (!rsa_oaep_sha256_encrypt(strPublicKeyPath, arrKey, vecEncryptedKey))
    {
        strError = "RSA public-key encryption failed";
        return false;
    }

    stOutput.strAlgorithm = "RSA-OAEP-SHA256+A256GCM";
    stOutput.strKeyId = strKeyId;
    stOutput.strEncryptedKey = base64_encode(vecEncryptedKey.data(), vecEncryptedKey.size());
    stOutput.strNonce = base64_encode(arrNonce.data(), arrNonce.size());
    stOutput.strCiphertext = base64_encode(vecCiphertext.data(), vecCiphertext.size());
    stOutput.strTag = base64_encode(arrTag.data(), arrTag.size());

    if (stOutput.strEncryptedKey.empty() || stOutput.strNonce.empty() ||
        stOutput.strCiphertext.empty() || stOutput.strTag.empty())
    {
        stOutput = EncryptedCredential_S();
        strError = "Base64 encoding failed";
        return false;
    }

    return true;
}
} // namespace PlatformRegisterCrypto
