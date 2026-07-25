/**
 * @FilePath     : openssl_provider.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-26 16:14:46
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-22 09:23:34
 * @Description  : OpenSSL 软件实现骨架
 */

#pragma once

#include "crypto_provider.h"
#include "Singleton.h"

/**
 * @brief   : OpenSSL 国密算法软件实现类
 * @note    : 当不加载硬件 Provider 时，OpenSSL 使用内置的软件实现
 *          直接调用 OpenSSL 3.3.1 标准 API
 */
class COpenSSLProvider : public ICryptoProvider, public CSingleton<COpenSSLProvider>
{
public:
    COpenSSLProvider();
    virtual ~COpenSSLProvider();

    /**
     * @brief   : 初始化 OpenSSL 软件 Provider
     * @return   {IpcRet_E} OK：成功，非 OK：失败
     */
    IpcRet_E init() override;

    /**
     * @brief   : 去初始化 OpenSSL 软件 Provider
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

    /**
     * @brief   : 生成随机数
     * @param    {size_t} length：随机数字节长度
     * @param    {bool} hex_output：是否输出十六进制字符串
     * @return   {std::string} 随机数（二进制或十六进制字符串）
     */
    std::string rand_bytes(size_t length, bool hex_output = false) override;

    /**
     * @brief   : 生成随机数并写入调用方缓冲区
     * @param    {uint8_t*} buf：随机数输出缓冲区
     * @param    {size_t} buflen：随机数字节长度
     * @return   {int} OK：成功，ERR_PARAM/ERR：失败
     */
    int rand_bytes(uint8_t *buf, size_t buflen) override;

    /**
     * @brief   : SM2 签名
     * @param    {std::string} privkey_path：私钥文件路径
     * @param    {std::string} pass：私钥密码
     * @param    {std::string} data：待签名数据
     * @param    {std::string} id：SM2 签名 ID
     * @return   {std::string} 签名结果（Base64 编码）
     */
    std::string sm2_sign(const std::string &privkey_path,
                         const std::string &pass,
                         const std::string &data,
                         const std::string &id = Gb35114Crypto_NS::SM2_DEFAULT_ID) override;

    /**
     * @brief   : SM2 验签
     * @param    {std::string} pubkey_path：公钥或证书文件路径
     * @param    {std::string} data：原始数据
     * @param    {std::string} signature：签名结果
     * @param    {std::string} id：SM2 签名 ID
     * @return   {bool} true: 验证成功，false: 验证失败
     */
    bool sm2_verify(const std::string &pubkey_path,
                    const std::string &data,
                    const std::string &signature,
                    const std::string &id = Gb35114Crypto_NS::SM2_DEFAULT_ID) override;

    /**
     * @brief   : SM2 私钥解密
     * @param    {std::vector<uint8_t>} input：密文数据
     * @param    {std::string} key_path：私钥文件路径
     * @return   {std::string} 解密后的明文
     */
    std::string sm2_decrypt(const std::vector<uint8_t> &input, const std::string &key_path) override;

    /**
     * @brief   : SM3 杂凑
     * @param    {std::vector<uint8_t>} data：输入数据
     * @return   {std::vector<uint8_t>} 32 字节杂凑值
     */
    std::vector<uint8_t> sm3_hash(const std::vector<uint8_t> &data) override;

    /**
     * @brief   : SM4-CBC 加密
     * @param    {std::string} key：16 字节密钥（十六进制字符串）
     * @param    {std::string} iv：16 字节 IV（十六进制字符串）
     * @param    {std::string} plaintext：明文数据
     * @return   {std::string} 密文（Base64 编码）
     */
    std::string sm4_encrypt_cbc(const std::string &key, const std::string &iv, const std::string &plaintext) override;

    /**
     * @brief   : SM4-CBC 解密
     * @param    {std::string} key：16 字节密钥（十六进制字符串）
     * @param    {std::string} iv：16 字节 IV（十六进制字符串）
     * @param    {std::string} ciphertext：密文数据（Base64 编码）
     * @return   {std::string} 明文
     */
    std::string sm4_decrypt_cbc(const std::string &key, const std::string &iv, const std::string &ciphertext) override;

    /**
     * @brief   : 生成证书请求（CSR）
     * @param    {std::string} country：国家代码
     * @param    {std::string} state：省份或州名
     * @param    {std::string} locality：城市或地区
     * @param    {std::string} organization：组织名称
     * @param    {std::string} organization_unit：组织单位
     * @param    {std::string} common_name：通用名称
     * @param    {std::string} key_file：私钥文件路径
     * @param    {std::string} password：私钥密码
     * @param    {std::string} output_file：输出 CSR 文件路径
     * @return   {bool} true: 成功，false: 失败
     */
    bool reqgen(const std::string &country,
                const std::string &state,
                const std::string &locality,
                const std::string &organization,
                const std::string &organization_unit,
                const std::string &common_name,
                const std::string &key_file,
                const std::string &password,
                const std::string &output_file) override;

    /**
     * @brief   : 解析证书
     * @param    {std::string} cert_path：证书文件路径
     * @return   {std::string} 证书信息（结构化文本）
     */
    std::string certparse(const std::string &cert_path) override;

    /**
     * @brief   : 解析证书请求
     * @param    {std::string} req_path：CSR 文件路径
     * @return   {std::string} CSR 信息
     */
    std::string reqparse(const std::string &req_path) override;

    /**
     * @brief   : 解析 CRL
     * @param    {std::string} crl_path：CRL 文件路径
     * @return   {std::string} CRL 信息
     */
    std::string crlparse(const std::string &crl_path) override;

    /**
     * @brief   : 验证 CRL
     * @param    {std::string} crl_path：CRL 文件路径
     * @param    {std::string} cert_path：签发者证书路径
     * @return   {std::string} 验证结果文本
     */
    std::string crlverify(const std::string &crl_path, const std::string &cert_path) override;

    /**
     * @brief   : SM2 数字签名
     * @param    {std::vector<uint8_t>} input_data：待签名数据
     * @param    {std::string&} out_data：签名输出
     * @param    {std::string} key_path：私钥路径
     * @return   {int} OK：成功，非 OK：失败
     */
    int digital_signature(const std::vector<uint8_t> &input_data, std::string &out_data, const std::string &key_path) override;

    /**
     * @brief   : SM2 验签
     * @param    {std::vector<uint8_t>} sign_data：原始签名数据
     * @param    {std::vector<uint8_t>} verify_sign：待验证签名
     * @param    {std::string} cert_path：证书路径
     * @return   {int} OK：验签成功，非 OK：失败
     */
    int verify_signature(const std::vector<uint8_t> &sign_data,
                         const std::vector<uint8_t> &verify_sign,
                         const std::string &cert_path) override;

    /**
     * @brief   : Base64 编码
     * @param    {std::vector<uint8_t>} input：二进制数据
     * @return   {std::vector<uint8_t>} Base64 编码数据
     */
    std::vector<uint8_t> base64_encode(const std::vector<uint8_t> &input) override;

    /**
     * @brief   : Base64 编码
     * @param    {std::string} input：原始字符串
     * @return   {std::vector<uint8_t>} Base64 编码数据
     */
    std::vector<uint8_t> base64_encode(const std::string &input) override;

    /**
     * @brief   : Base64 解码
     * @param    {std::string} input：Base64 字符串
     * @return   {std::vector<uint8_t>} 二进制数据
     */
    std::vector<uint8_t> base64_decode(const std::string &input) override;

    /**
     * @brief   : Base64 解码
     * @param    {const char*} input：Base64 字符串指针
     * @return   {std::vector<uint8_t>} 二进制数据
     */
    std::vector<uint8_t> base64_decode(const char *input) override;

    /**
     * @brief   : Base64 编码为字符串
     * @param    {std::string} input：原始字符串
     * @return   {std::string} Base64 编码字符串
     */
    std::string base64_encode_to_string(const std::string &input) override;

    /**
     * @brief   : Base64 编码为字符串
     * @param    {std::vector<uint8_t>} input：二进制数据
     * @return   {std::string} Base64 编码字符串
     */
    std::string base64_encode_to_string(const std::vector<uint8_t> &input) override;

    /**
     * @brief   : 生成 SM2 密钥对（含公钥文件输出）
     * @param    {std::string} pass：私钥加密密码
     * @param    {std::string} privkey_path：私钥文件输出路径（PKCS8 PEM 格式）
     * @param    {std::string} pubkey_path：公钥文件输出路径（PEM 格式）
     */
    int sm2keygen(const std::string &pass, const std::string &privkey_path, const std::string &pubkey_path) override;

    /**
     * @brief   : 生成 SM2 密钥对（仅私钥文件输出）
     * @param    {std::string} pass：私钥加密密码
     * @param    {std::string} privkey_path：私钥文件输出路径（PKCS8 PEM 格式）
     */
    int sm2keygen(const std::string &pass, const std::string &privkey_path) override;

private:
    bool m_bInitialized = false;
};
