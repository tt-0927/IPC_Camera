/**
 * @FilePath     : hi_crypto_provider.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-06-16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-16
 * @Description  : 海思安全子系统 ICryptoProvider 后端
 */

#pragma once

#include "crypto_provider.h"
#include "Singleton.h"

/**
 * @brief   : 海思安全子系统国密算法 Provider
 * @note    : 该类属于业务层 Provider，不放入 ipc_share；共享层仍只依赖 ICryptoProvider。
 */
class CHiCryptoProvider : public ICryptoProvider, public CSingleton<CHiCryptoProvider>
{
public:
    CHiCryptoProvider();
    ~CHiCryptoProvider() override;

    /**
     * @brief   : 初始化海思硬件 Provider 及软件 fallback
     * @return   {IpcRet_E} OK：成功，非 OK：失败
     */
    IpcRet_E init() override;

    /**
     * @brief   : 去初始化海思硬件 Provider
     * @return   {IpcRet_E} OK：成功，非 OK：失败
     */
    IpcRet_E deinit() override;

    /**
     * @brief   : 判断 Provider 是否可用
     * @return   {bool} true：可用，false：不可用
     */
    bool is_ready() const override;

    /**
     * @brief   : 获取 Provider 名称
     * @return   {const char*} Provider 名称
     */
    const char *name() const override;

    std::string rand_bytes(size_t length, bool hex_output = false) override;
    int rand_bytes(uint8_t *buf, size_t buflen) override;
    std::string sm2_sign(const std::string &privkey_path,
                         const std::string &pass,
                         const std::string &data,
                         const std::string &id = Gb35114Crypto_NS::SM2_DEFAULT_ID) override;
    bool sm2_verify(const std::string &pubkey_path,
                    const std::string &data,
                    const std::string &signature,
                    const std::string &id = Gb35114Crypto_NS::SM2_DEFAULT_ID) override;
    std::string sm2_decrypt(const std::vector<uint8_t> &input, const std::string &key_path) override;
    std::vector<uint8_t> sm3_hash(const std::vector<uint8_t> &data) override;
    std::string sm4_encrypt_cbc(const std::string &key, const std::string &iv, const std::string &plaintext) override;
    std::string sm4_decrypt_cbc(const std::string &key, const std::string &iv, const std::string &ciphertext) override;
    bool reqgen(const std::string &country,
                const std::string &state,
                const std::string &locality,
                const std::string &organization,
                const std::string &organization_unit,
                const std::string &common_name,
                const std::string &key_file,
                const std::string &password,
                const std::string &output_file) override;
    std::string certparse(const std::string &cert_path) override;
    std::string reqparse(const std::string &req_path) override;
    std::string crlparse(const std::string &crl_path) override;
    std::string crlverify(const std::string &crl_path, const std::string &cert_path) override;
    int digital_signature(const std::vector<uint8_t> &input_data, std::string &out_data, const std::string &key_path) override;
    int verify_signature(const std::vector<uint8_t> &sign_data,
                         const std::vector<uint8_t> &verify_sign,
                         const std::string &cert_path) override;
    std::vector<uint8_t> base64_encode(const std::vector<uint8_t> &input) override;
    std::vector<uint8_t> base64_encode(const std::string &input) override;
    std::vector<uint8_t> base64_decode(const std::string &input) override;
    std::vector<uint8_t> base64_decode(const char *input) override;
    std::string base64_encode_to_string(const std::string &input) override;
    std::string base64_encode_to_string(const std::vector<uint8_t> &input) override;
    int sm2keygen(const std::string &pass, const std::string &privkey_path, const std::string &pubkey_path) override;
    int sm2keygen(const std::string &pass, const std::string &privkey_path) override;

private:
    bool m_bInitialized = false;
};
