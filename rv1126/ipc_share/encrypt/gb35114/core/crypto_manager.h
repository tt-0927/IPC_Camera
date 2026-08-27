/**
 * @FilePath     : crypto_manager.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-06-10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-12 16:05:59
 * @Description  : 密码学模块管理类（统一门面）
 */

#pragma once

#include "IpcRet.h"
#include "Singleton.h"
#include "crypto_provider.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief   : 密码学模块管理类（统一门面）
 * @note    : 统一管理 Provider 生命周期，提供国密算法统一接口
 *            共享层代码通过此类访问密码学功能，不直接依赖具体 Provider 实现
 *            业务层通过 set_provider() 注入具体实现（OpenSSL 软实现或 HiProvider 硬件加速）
 */
class CCryptoManager : public CSingleton<CCryptoManager>
{
public:
    CCryptoManager();
    ~CCryptoManager();

    /**
     * @brief   : 初始化密码学模块
     * @note    : 调用前必须先通过 set_provider() 设置 Provider
     * @return   {IpcRet_E} OK: 成功，ERR: 失败
     */
    IpcRet_E init();

    /**
     * @brief   : 去初始化密码学模块
     * @return   {IpcRet_E} OK: 成功
     */
    IpcRet_E deinit();

    /**
     * @brief   : 获取当前的 ICryptoProvider 实例
     * @return   {ICryptoProvider*} Provider 指针
     */
    ICryptoProvider *get_provider() const;

    /**
     * @brief   : 设置 ICryptoProvider 实例
     * @param    {ICryptoProvider*} provider：Provider 指针
     * @note    : 必须在 init() 之前调用
     */
    IpcRet_E set_provider(ICryptoProvider *provider);

    /**
     * @brief   : 判断密码学模块是否已经初始化
     * @return   {bool} true：已初始化，false：未初始化
     */
    bool is_initialized() const;

    /**
     * @brief   : 判断当前 Provider 是否可用
     * @return   {bool} true：可用，false：不可用
     */
    bool is_ready() const;

    /**
     * @brief   : 执行国密算法自测
     * @return   {IpcRet_E} OK：自测通过，非 OK：自测失败
     * @note    : 自测会在 /tmp 下创建临时密钥和 CSR 文件，默认不在 init() 中执行。
     */
    IpcRet_E self_test();

    /* ============ 统一接口 - 委托给当前 Provider ============ */

    /**
     * @brief   : 生成随机数
     * @param    {size_t} length：随机数字节长度
     * @param    {bool} hex_output：是否输出十六进制字符串
     * @return   {std::string} 随机数（二进制或十六进制字符串）
     */
    std::string rand_bytes(size_t length, bool hex_output = false);

    /**
     * @brief   : 生成随机数并写入调用方缓冲区
     * @param    {uint8_t*} buf：随机数输出缓冲区
     * @param    {size_t} buflen：随机数字节长度
     * @return   {int} OK：成功，ERR_PARAM/ERR：失败
     */
    int rand_bytes(uint8_t *buf, size_t buflen);

    /**
     * @brief   : SM3 杂凑
     * @param    {std::vector<uint8_t>} data：输入数据
     * @return   {std::vector<uint8_t>} 32 字节杂凑值
     */
    std::vector<uint8_t> sm3_hash(const std::vector<uint8_t> &data);

    /**
     * @brief   : SM2 签名
     * @param    {std::string} privkey_path：私钥文件路径
     * @param    {std::string} pass：私钥密码
     * @param    {std::string} data：待签名数据
     * @param    {std::string} id：SM2 签名 ID
     * @return   {std::string} 签名结果
     */
    std::string sm2_sign(const std::string &privkey_path, const std::string &pass,
                         const std::string &data,
                         const std::string &id = Gb35114Crypto_NS::SM2_DEFAULT_ID);

    /**
     * @brief   : SM2 验签
     * @param    {std::string} pubkey_path：公钥文件路径
     * @param    {std::string} data：原始数据
     * @param    {std::string} signature：签名结果
     * @param    {std::string} id：SM2 签名 ID
     * @return   {bool} true: 验证成功，false: 验证失败
     */
    bool sm2_verify(const std::string &pubkey_path, const std::string &data,
                    const std::string &signature,
                    const std::string &id = Gb35114Crypto_NS::SM2_DEFAULT_ID);

    /**
     * @brief   : SM2 私钥解密
     * @param    {std::vector<uint8_t>} input：密文数据
     * @param    {std::string} key_path：私钥文件路径
     * @return   {std::string} 解密后的明文
     */
    std::string sm2_decrypt(const std::vector<uint8_t> &input, const std::string &key_path);

    /**
     * @brief   : SM4-CBC 加密
     * @param    {std::string} key：16 字节密钥（十六进制字符串）
     * @param    {std::string} iv：16 字节 IV（十六进制字符串）
     * @param    {std::string} plaintext：明文数据
     * @return   {std::string} 密文（Base64 编码）
     */
    std::string sm4_encrypt_cbc(const std::string &key, const std::string &iv, const std::string &plaintext);

    /**
     * @brief   : SM4-CBC 解密
     * @param    {std::string} key：16 字节密钥（十六进制字符串）
     * @param    {std::string} iv：16 字节 IV（十六进制字符串）
     * @param    {std::string} ciphertext：密文数据（Base64 编码）
     * @return   {std::string} 明文
     */
    std::string sm4_decrypt_cbc(const std::string &key, const std::string &iv, const std::string &ciphertext);

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
    bool reqgen(const std::string &country, const std::string &state, const std::string &locality,
                const std::string &organization, const std::string &organization_unit,
                const std::string &common_name, const std::string &key_file,
                const std::string &password, const std::string &output_file);

    /**
     * @brief   : 解析证书
     * @param    {std::string} cert_path：证书文件路径
     * @return   {std::string} 证书信息
     */
    std::string certparse(const std::string &cert_path);

    /**
     * @brief   : 解析证书请求
     * @param    {std::string} req_path：CSR 文件路径
     * @return   {std::string} CSR 信息
     */
    std::string reqparse(const std::string &req_path);

    /**
     * @brief   : 解析 CRL
     * @param    {std::string} crl_path：CRL 文件路径
     * @return   {std::string} CRL 信息
     */
    std::string crlparse(const std::string &crl_path);

    /**
     * @brief   : 验证 CRL
     * @param    {std::string} crl_path：CRL 文件路径
     * @param    {std::string} cert_path：签发者证书路径
     * @return   {std::string} 验证结果文本
     */
    std::string crlverify(const std::string &crl_path, const std::string &cert_path);

    /**
     * @brief   : SM2 数字签名
     * @param    {std::vector<uint8_t>} input_data：待签名数据
     * @param    {std::string&} out_data：签名输出
     * @param    {std::string} key_path：私钥路径
     * @return   {int} OK：成功，非 OK：失败
     */
    int digital_signature(const std::vector<uint8_t> &input_data, std::string &out_data, const std::string &key_path);

    /**
     * @brief   : SM2 验签
     * @param    {std::vector<uint8_t>} sign_data：原始签名数据
     * @param    {std::vector<uint8_t>} verify_sign：待验证签名
     * @param    {std::string} cert_path：证书路径
     * @return   {int} OK：验签成功，非 OK：失败
     */
    int verify_signature(const std::vector<uint8_t> &sign_data, const std::vector<uint8_t> &verify_sign,
                         const std::string &cert_path);

    /**
     * @brief   : Base64 编码
     * @param    {std::vector<uint8_t>} input：二进制数据
     * @return   {std::vector<uint8_t>} Base64 编码数据
     */
    std::vector<uint8_t> base64_encode(const std::vector<uint8_t> &input);

    /**
     * @brief   : Base64 编码
     * @param    {std::string} input：原始字符串
     * @return   {std::vector<uint8_t>} Base64 编码数据
     */
    std::vector<uint8_t> base64_encode(const std::string &input);

    /**
     * @brief   : Base64 解码
     * @param    {std::string} input：Base64 字符串
     * @return   {std::vector<uint8_t>} 二进制数据
     */
    std::vector<uint8_t> base64_decode(const std::string &input);

    /**
     * @brief   : Base64 解码
     * @param    {const char*} input：Base64 字符串指针
     * @return   {std::vector<uint8_t>} 二进制数据
     */
    std::vector<uint8_t> base64_decode(const char *input);

    /**
     * @brief   : Base64 编码为字符串
     * @param    {std::string} input：原始字符串
     * @return   {std::string} Base64 编码字符串
     */
    std::string base64_encode_to_string(const std::string &input);

    /**
     * @brief   : Base64 编码为字符串
     * @param    {std::vector<uint8_t>} input：二进制数据
     * @return   {std::string} Base64 编码字符串
     */
    std::string base64_encode_to_string(const std::vector<uint8_t> &input);

    /**
     * @brief   : 生成 SM2 密钥对（含公钥文件输出）
     * @param    {std::string} pass：私钥加密密码
     * @param    {std::string} privkey_path：私钥文件输出路径（PKCS8 PEM 格式）
     * @param    {std::string} pubkey_path：公钥文件输出路径（PEM 格式）
     */
    int sm2keygen(const std::string &pass, const std::string &privkey_path, const std::string &pubkey_path);

    /**
     * @brief   : 生成 SM2 密钥对（仅私钥文件输出）
     * @param    {std::string} pass：私钥加密密码
     * @param    {std::string} privkey_path：私钥文件输出路径（PKCS8 PEM 格式）
     */
    int sm2keygen(const std::string &pass, const std::string &privkey_path);

private:
    /* ICryptoProvider 实例指针 */
    ICryptoProvider *m_pProvider = nullptr;
    /* 初始化标志 */
    bool m_bInitialized = false;

    /**
     * @brief   : 国密算法自测内部实现
     * @return   {IpcRet_E} OK：自测通过，非 OK：自测失败
     */
    IpcRet_E run_self_test();
};
