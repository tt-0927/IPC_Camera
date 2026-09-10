/**
 * @FilePath     : crypto_provider.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-06-12
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-22 09:12:37
 * @Description  : 国密算法 Provider 抽象接口
 */

#pragma once

#include "gb35114_crypto_define.h"
#include "IpcRet.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief   : 国密算法 Provider 抽象接口
 * @note    : 共享层只依赖该抽象接口，不依赖 OpenSSL、海思或瑞芯微等具体实现。
 *            具体 Provider 由业务层在启动阶段注入到 CCryptoManager。
 */
class ICryptoProvider
{
public:
    virtual ~ICryptoProvider() = default;

    /**
     * @brief   : 初始化 Provider
     * @return   {IpcRet_E} OK：成功，非 OK：失败
     * @note    : 软件 Provider 可仅维护状态；硬件 Provider 在此注册硬件资源或 OpenSSL Provider。
     */
    virtual IpcRet_E init()
    {
        return OK;
    }

    /**
     * @brief   : 去初始化 Provider
     * @return   {IpcRet_E} OK：成功，非 OK：失败
     */
    virtual IpcRet_E deinit()
    {
        return OK;
    }

    /**
     * @brief   : 查询 Provider 是否可用
     * @return   {bool} true：可用，false：不可用
     */
    virtual bool is_ready() const
    {
        return true;
    }

    /**
     * @brief   : 获取 Provider 名称
     * @return   {const char*} Provider 名称
     * @note    : 用于日志定位，不用于 RTTI 类型判断。
     */
    virtual const char *name() const
    {
        return "unknown";
    }

    /**
     * @brief   : 生成随机数
     * @param    {size_t} length：随机数字节长度
     * @param    {bool} hex_output：是否输出十六进制字符串
     * @return   {std::string} 随机数（二进制或十六进制字符串）
     */
    virtual std::string rand_bytes(size_t length, bool hex_output = false) = 0;

    /**
     * @brief   : 生成随机数并写入调用方缓冲区
     * @param    {uint8_t*} buf：随机数输出缓冲区
     * @param    {size_t} buflen：随机数字节长度
     * @return   {int} OK：成功，ERR_PARAM/ERR：失败
     */
    virtual int rand_bytes(uint8_t *buf, size_t buflen) = 0;

    /**
     * @brief   : SM2 签名
     * @param    {std::string} privkey_path：私钥文件路径
     * @param    {std::string} pass：私钥密码
     * @param    {std::string} data：待签名数据
     * @param    {std::string} id：SM2 签名 ID
     * @return   {std::string} 签名结果
     */
    virtual std::string sm2_sign(const std::string &privkey_path,
                                 const std::string &pass,
                                 const std::string &data,
                                 const std::string &id = Gb35114Crypto_NS::SM2_DEFAULT_ID) = 0;

    /**
     * @brief   : SM2 验签
     * @param    {std::string} pubkey_path：公钥文件路径
     * @param    {std::string} data：原始数据
     * @param    {std::string} signature：签名结果
     * @param    {std::string} id：SM2 签名 ID
     * @return   {bool} true：验证成功，false：验证失败
     */
    virtual bool sm2_verify(const std::string &pubkey_path,
                            const std::string &data,
                            const std::string &signature,
                            const std::string &id = Gb35114Crypto_NS::SM2_DEFAULT_ID) = 0;

    /**
     * @brief   : SM2 私钥解密
     * @param    {std::vector<uint8_t>} input：密文数据
     * @param    {std::string} key_path：私钥文件路径
     * @return   {std::string} 解密后的明文
     */
    virtual std::string sm2_decrypt(const std::vector<uint8_t> &input, const std::string &key_path) = 0;

    /**
     * @brief   : SM3 杂凑
     * @param    {std::vector<uint8_t>} data：输入数据
     * @return   {std::vector<uint8_t>} 32 字节杂凑值
     */
    virtual std::vector<uint8_t> sm3_hash(const std::vector<uint8_t> &data) = 0;

    /**
     * @brief   : SM4-CBC 加密
     * @param    {std::string} key：16 字节密钥（十六进制字符串）
     * @param    {std::string} iv：16 字节 IV（十六进制字符串）
     * @param    {std::string} plaintext：明文数据
     * @return   {std::string} 密文（Base64 编码）
     */
    virtual std::string sm4_encrypt_cbc(const std::string &key, const std::string &iv, const std::string &plaintext) = 0;

    /**
     * @brief   : SM4-CBC 解密
     * @param    {std::string} key：16 字节密钥（十六进制字符串）
     * @param    {std::string} iv：16 字节 IV（十六进制字符串）
     * @param    {std::string} ciphertext：密文数据（Base64 编码）
     * @return   {std::string} 明文
     */
    virtual std::string sm4_decrypt_cbc(const std::string &key, const std::string &iv, const std::string &ciphertext) = 0;

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
     * @return   {bool} true：成功，false：失败
     */
    virtual bool reqgen(const std::string &country,
                        const std::string &state,
                        const std::string &locality,
                        const std::string &organization,
                        const std::string &organization_unit,
                        const std::string &common_name,
                        const std::string &key_file,
                        const std::string &password,
                        const std::string &output_file) = 0;

    /**
     * @brief   : 解析证书
     * @param    {std::string} cert_path：证书文件路径
     * @return   {std::string} 证书信息
     */
    virtual std::string certparse(const std::string &cert_path) = 0;

    /**
     * @brief   : 解析证书请求
     * @param    {std::string} req_path：CSR 文件路径
     * @return   {std::string} CSR 信息
     */
    virtual std::string reqparse(const std::string &req_path) = 0;

    /**
     * @brief   : 解析 CRL
     * @param    {std::string} crl_path：CRL 文件路径
     * @return   {std::string} CRL 信息
     */
    virtual std::string crlparse(const std::string &crl_path) = 0;

    /**
     * @brief   : 验证 CRL
     * @param    {std::string} crl_path：CRL 文件路径
     * @param    {std::string} cert_path：签发者证书路径
     * @return   {std::string} 验证结果文本
     */
    virtual std::string crlverify(const std::string &crl_path, const std::string &cert_path) = 0;

    /**
     * @brief   : SM2 数字签名
     * @param    {std::vector<uint8_t>} input_data：待签名数据
     * @param    {std::string&} out_data：签名输出
     * @param    {std::string} key_path：私钥路径
     * @return   {int} OK：成功，非 OK：失败
     */
    virtual int digital_signature(const std::vector<uint8_t> &input_data, std::string &out_data, const std::string &key_path) = 0;

    /**
     * @brief   : SM2 验签
     * @param    {std::vector<uint8_t>} sign_data：原始签名数据
     * @param    {std::vector<uint8_t>} verify_sign：待验证签名
     * @param    {std::string} cert_path：证书路径
     * @return   {int} OK：验签成功，非 OK：失败
     */
    virtual int verify_signature(const std::vector<uint8_t> &sign_data,
                                 const std::vector<uint8_t> &verify_sign,
                                 const std::string &cert_path) = 0;

    /**
     * @brief   : Base64 编码
     * @param    {std::vector<uint8_t>} input：二进制数据
     * @return   {std::vector<uint8_t>} Base64 编码数据
     */
    virtual std::vector<uint8_t> base64_encode(const std::vector<uint8_t> &input) = 0;

    /**
     * @brief   : Base64 编码
     * @param    {std::string} input：原始字符串
     * @return   {std::vector<uint8_t>} Base64 编码数据
     */
    virtual std::vector<uint8_t> base64_encode(const std::string &input) = 0;

    /**
     * @brief   : Base64 解码
     * @param    {std::string} input：Base64 字符串
     * @return   {std::vector<uint8_t>} 二进制数据
     */
    virtual std::vector<uint8_t> base64_decode(const std::string &input) = 0;

    /**
     * @brief   : Base64 解码
     * @param    {const char*} input：Base64 字符串指针
     * @return   {std::vector<uint8_t>} 二进制数据
     */
    virtual std::vector<uint8_t> base64_decode(const char *input) = 0;

    /**
     * @brief   : Base64 编码为字符串
     * @param    {std::string} input：原始字符串
     * @return   {std::string} Base64 编码字符串
     */
    virtual std::string base64_encode_to_string(const std::string &input) = 0;

    /**
     * @brief   : Base64 编码为字符串
     * @param    {std::vector<uint8_t>} input：二进制数据
     * @return   {std::string} Base64 编码字符串
     */
    virtual std::string base64_encode_to_string(const std::vector<uint8_t> &input) = 0;

    /**
     * @brief   : 生成 SM2 密钥对（含公钥文件输出）
     * @param    {std::string} pass：私钥加密密码
     * @param    {std::string} privkey_path：私钥文件输出路径
     * @param    {std::string} pubkey_path：公钥文件输出路径
     * @return   {int} OK：成功，非 OK：失败
     */
    virtual int sm2keygen(const std::string &pass, const std::string &privkey_path, const std::string &pubkey_path) = 0;

    /**
     * @brief   : 生成 SM2 密钥对（仅私钥文件输出）
     * @param    {std::string} pass：私钥加密密码
     * @param    {std::string} privkey_path：私钥文件输出路径
     * @return   {int} OK：成功，非 OK：失败
     */
    virtual int sm2keygen(const std::string &pass, const std::string &privkey_path) = 0;
};
