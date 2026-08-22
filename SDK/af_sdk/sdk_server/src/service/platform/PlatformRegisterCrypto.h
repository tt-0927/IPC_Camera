/**
 * @file PlatformRegisterCrypto.h
 * @author Codex
 * @date 2026-08-22
 * @brief Declares hybrid encryption for MQTT device-registration credentials.
 * @change 2026-08-22 Codex Initial implementation for platform migration.
 */
#pragma once

#include <string>

/**
 * @class CPlatformRegisterCrypto
 * @brief Encrypts RTSP credentials with RSA-OAEP-SHA256 and AES-256-GCM.
 */
class CPlatformRegisterCrypto
{
public:
    /**
     * @struct EncryptedCredential_S
     * @brief Base64-encoded registration credential envelope.
     */
    struct EncryptedCredential_S
    {
        std::string strAlgorithm;
        std::string strKeyId;
        std::string strEncryptedKey;
        std::string strNonce;
        std::string strCiphertext;
        std::string strTag;
    };

    /**
     * @brief Encrypts one credential JSON document for platform registration.
     * @author Codex
     * @param [IN] strPublicKeyPath PEM RSA public-key path.
     * @param [IN] strKeyId Platform key identifier.
     * @param [IN] strPlaintext Credential JSON containing URL, account and password.
     * @param [IN] strAad Exact authenticated registration metadata.
     * @param [OUT] stOutput Base64-encoded hybrid encryption envelope.
     * @param [OUT] strError Non-sensitive failure description.
     * @return True when every cryptographic operation succeeds.
     */
    static bool EncryptCredential(const std::string &strPublicKeyPath,
                                  const std::string &strKeyId,
                                  const std::string &strPlaintext,
                                  const std::string &strAad,
                                  EncryptedCredential_S &stOutput,
                                  std::string &strError);

    /**
     * @brief Securely overwrites a mutable string before releasing its storage.
     * @author Codex
     * @param [INOUT] strSensitive Sensitive value to cleanse and clear.
     * @return No return value.
     */
    static void CleanseString(std::string &strSensitive);

private:
    CPlatformRegisterCrypto() = delete;
};
