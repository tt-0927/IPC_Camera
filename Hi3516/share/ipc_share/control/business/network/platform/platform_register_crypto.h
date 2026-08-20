/**
 * @FilePath     : platform_register_crypto.h
 * @Description  : MQTT设备注册凭据加密工具
 */

#pragma once

#include <string>

namespace PlatformRegisterCrypto
{
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
 * @brief   : 使用平台RSA公钥封装一次性AES密钥，再以AES-256-GCM加密注册凭据
 * @param    {std::string} strPublicKeyPath：PEM格式RSA公钥路径
 * @param    {std::string} strKeyId：平台公钥标识
 * @param    {std::string} strPlaintext：待保护的JSON明文
 * @param    {std::string} strAad：绑定外层注册字段的认证附加数据
 * @param    {EncryptedCredential_S &} stOutput：Base64编码后的加密信封
 * @param    {std::string &} strError：不包含敏感明文的失败阶段说明
 * @return   {bool} true：加密成功 false：加密失败
 */
bool encrypt_credential(const std::string &strPublicKeyPath,
                        const std::string &strKeyId,
                        const std::string &strPlaintext,
                        const std::string &strAad,
                        EncryptedCredential_S &stOutput,
                        std::string &strError);
} // namespace PlatformRegisterCrypto
