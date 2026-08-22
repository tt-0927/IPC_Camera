/**
 * @file PlatformRegisterCrypto.cpp
 * @author Codex
 * @date 2026-08-22
 * @brief Implements hybrid encryption for MQTT device-registration credentials.
 * @change 2026-08-22 Codex Initial implementation for platform migration.
 */

#include "PlatformRegisterCrypto.h"

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
constexpr std::size_t PLATFORM_CRYPTO_AES_KEY_LENGTH = 32;
constexpr std::size_t PLATFORM_CRYPTO_GCM_NONCE_LENGTH = 12;
constexpr std::size_t PLATFORM_CRYPTO_GCM_TAG_LENGTH = 16;

/**
 * @class COpenSslCleanseGuard
 * @brief Cleanses a fixed sensitive buffer on every return path.
 */
class COpenSslCleanseGuard
{
public:
    /**
     * @brief Stores the sensitive memory range.
     * @author Codex
     */
    COpenSslCleanseGuard(void *pData, std::size_t uSize)
        : m_pData(pData),
          m_uSize(uSize)
    {
    }

    /**
     * @brief Cleanses the sensitive memory range.
     * @author Codex
     */
    ~COpenSslCleanseGuard()
    {
        if (m_pData != nullptr && m_uSize > 0)
        {
            OPENSSL_cleanse(m_pData, m_uSize);
        }
    }

    COpenSslCleanseGuard(const COpenSslCleanseGuard &) = delete;
    COpenSslCleanseGuard &operator=(const COpenSslCleanseGuard &) = delete;

private:
    void *m_pData;
    std::size_t m_uSize;
};

/**
 * @brief Checks whether OpenSSL integer-length APIs can represent a buffer.
 * @author Codex
 * @param [IN] uSize Buffer length.
 * @return True when the length fits in a signed int.
 */
static bool IsOpenSslLengthValid(std::size_t uSize)
{
    return uSize <= static_cast<std::size_t>(INT_MAX);
}

/**
 * @brief Encodes a binary buffer as standard Base64 without line breaks.
 * @author Codex
 * @param [IN] pData Binary input.
 * @param [IN] uSize Binary input length.
 * @return Encoded text or an empty string on failure.
 */
static std::string Base64Encode(const unsigned char *pData, std::size_t uSize)
{
    if (pData == nullptr || uSize == 0 || !IsOpenSslLengthValid(uSize))
    {
        return std::string();
    }

    const std::size_t uOutputSize = 4U * ((uSize + 2U) / 3U);
    std::string strEncoded(uOutputSize, '\0');
    const int nWritten = EVP_EncodeBlock(
        reinterpret_cast<unsigned char *>(&strEncoded[0]),
        pData,
        static_cast<int>(uSize));
    if (nWritten <= 0)
    {
        return std::string();
    }

    strEncoded.resize(static_cast<std::size_t>(nWritten));
    return strEncoded;
}

/**
 * @brief Encrypts plaintext and AAD with AES-256-GCM.
 * @author Codex
 * @return True when ciphertext and authentication tag are produced.
 */
static bool EncryptAes256Gcm(
    const std::array<unsigned char, PLATFORM_CRYPTO_AES_KEY_LENGTH> &aKey,
    const std::array<unsigned char, PLATFORM_CRYPTO_GCM_NONCE_LENGTH> &aNonce,
    const std::string &strPlaintext,
    const std::string &strAad,
    std::vector<unsigned char> &aCiphertext,
    std::array<unsigned char, PLATFORM_CRYPTO_GCM_TAG_LENGTH> &aTag)
{
    if (!IsOpenSslLengthValid(strPlaintext.size()) || !IsOpenSslLengthValid(strAad.size()))
    {
        return false;
    }

    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> pContext(
        EVP_CIPHER_CTX_new(),
        EVP_CIPHER_CTX_free);
    if (!pContext)
    {
        return false;
    }

    if (EVP_EncryptInit_ex(pContext.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(pContext.get(),
                            EVP_CTRL_GCM_SET_IVLEN,
                            static_cast<int>(aNonce.size()),
                            nullptr) != 1 ||
        EVP_EncryptInit_ex(pContext.get(), nullptr, nullptr, aKey.data(), aNonce.data()) != 1)
    {
        return false;
    }

    int nOutputLength = 0;
    if (!strAad.empty() &&
        EVP_EncryptUpdate(pContext.get(),
                          nullptr,
                          &nOutputLength,
                          reinterpret_cast<const unsigned char *>(strAad.data()),
                          static_cast<int>(strAad.size())) != 1)
    {
        return false;
    }

    aCiphertext.assign(strPlaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_gcm()), 0);
    if (!strPlaintext.empty() &&
        EVP_EncryptUpdate(pContext.get(),
                          aCiphertext.data(),
                          &nOutputLength,
                          reinterpret_cast<const unsigned char *>(strPlaintext.data()),
                          static_cast<int>(strPlaintext.size())) != 1)
    {
        return false;
    }

    int nFinalLength = 0;
    if (EVP_EncryptFinal_ex(pContext.get(),
                            aCiphertext.data() + nOutputLength,
                            &nFinalLength) != 1 ||
        EVP_CIPHER_CTX_ctrl(pContext.get(),
                            EVP_CTRL_GCM_GET_TAG,
                            static_cast<int>(aTag.size()),
                            aTag.data()) != 1)
    {
        return false;
    }

    aCiphertext.resize(static_cast<std::size_t>(nOutputLength + nFinalLength));
    return true;
}

/**
 * @brief Encrypts the generated AES key with RSA OAEP SHA-256 and MGF1 SHA-256.
 * @author Codex
 * @return True when the encrypted key is produced.
 */
static bool EncryptRsaOaepSha256(
    const std::string &strPublicKeyPath,
    const std::array<unsigned char, PLATFORM_CRYPTO_AES_KEY_LENGTH> &aKey,
    std::vector<unsigned char> &aEncryptedKey)
{
    std::unique_ptr<BIO, decltype(&BIO_free)> pBio(
        BIO_new_file(strPublicKeyPath.c_str(), "rb"),
        BIO_free);
    if (!pBio)
    {
        return false;
    }

    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> pPublicKey(
        PEM_read_bio_PUBKEY(pBio.get(), nullptr, nullptr, nullptr),
        EVP_PKEY_free);
    if (!pPublicKey || EVP_PKEY_base_id(pPublicKey.get()) != EVP_PKEY_RSA)
    {
        return false;
    }

    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> pContext(
        EVP_PKEY_CTX_new(pPublicKey.get(), nullptr),
        EVP_PKEY_CTX_free);
    if (!pContext || EVP_PKEY_encrypt_init(pContext.get()) != 1 ||
        EVP_PKEY_CTX_set_rsa_padding(pContext.get(), RSA_PKCS1_OAEP_PADDING) <= 0 ||
        EVP_PKEY_CTX_set_rsa_oaep_md(pContext.get(), EVP_sha256()) <= 0 ||
        EVP_PKEY_CTX_set_rsa_mgf1_md(pContext.get(), EVP_sha256()) <= 0)
    {
        return false;
    }

    std::size_t uEncryptedKeyLength = 0;
    if (EVP_PKEY_encrypt(pContext.get(),
                         nullptr,
                         &uEncryptedKeyLength,
                         aKey.data(),
                         aKey.size()) != 1 ||
        uEncryptedKeyLength == 0)
    {
        return false;
    }

    aEncryptedKey.assign(uEncryptedKeyLength, 0);
    if (EVP_PKEY_encrypt(pContext.get(),
                         aEncryptedKey.data(),
                         &uEncryptedKeyLength,
                         aKey.data(),
                         aKey.size()) != 1)
    {
        return false;
    }

    aEncryptedKey.resize(uEncryptedKeyLength);
    return true;
}
}

bool CPlatformRegisterCrypto::EncryptCredential(
    const std::string &strPublicKeyPath,
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

    std::array<unsigned char, PLATFORM_CRYPTO_AES_KEY_LENGTH> aKey{};
    std::array<unsigned char, PLATFORM_CRYPTO_GCM_NONCE_LENGTH> aNonce{};
    std::array<unsigned char, PLATFORM_CRYPTO_GCM_TAG_LENGTH> aTag{};
    COpenSslCleanseGuard stKeyGuard(aKey.data(), aKey.size());

    if (RAND_bytes(aKey.data(), static_cast<int>(aKey.size())) != 1 ||
        RAND_bytes(aNonce.data(), static_cast<int>(aNonce.size())) != 1)
    {
        strError = "random generation failed";
        return false;
    }

    std::vector<unsigned char> aCiphertext;
    if (!EncryptAes256Gcm(aKey, aNonce, strPlaintext, strAad, aCiphertext, aTag))
    {
        strError = "AES-GCM encryption failed";
        return false;
    }

    std::vector<unsigned char> aEncryptedKey;
    if (!EncryptRsaOaepSha256(strPublicKeyPath, aKey, aEncryptedKey))
    {
        strError = "RSA public-key encryption failed";
        return false;
    }

    stOutput.strAlgorithm = "RSA-OAEP-SHA256+A256GCM";
    stOutput.strKeyId = strKeyId;
    stOutput.strEncryptedKey = Base64Encode(aEncryptedKey.data(), aEncryptedKey.size());
    stOutput.strNonce = Base64Encode(aNonce.data(), aNonce.size());
    stOutput.strCiphertext = Base64Encode(aCiphertext.data(), aCiphertext.size());
    stOutput.strTag = Base64Encode(aTag.data(), aTag.size());

    if (stOutput.strEncryptedKey.empty() || stOutput.strNonce.empty() ||
        stOutput.strCiphertext.empty() || stOutput.strTag.empty())
    {
        stOutput = EncryptedCredential_S();
        strError = "Base64 encoding failed";
        return false;
    }
    return true;
}

void CPlatformRegisterCrypto::CleanseString(std::string &strSensitive)
{
    if (!strSensitive.empty())
    {
        OPENSSL_cleanse(&strSensitive[0], strSensitive.size());
    }
    strSensitive.clear();
}
