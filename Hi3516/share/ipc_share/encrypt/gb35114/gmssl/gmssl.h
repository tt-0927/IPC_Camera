/**
 * @FilePath     : gmssl.h
 * @Author       : zhouzirui
 * @Date         : 2025-04-21 10:09:57
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-22 09:20:07
 * @Description  : gmssl命令行封装（历史文件，不参与编译）
 * @deprecated   : 此文件已从 CMake 排除编译。
 *                 调用方已迁移到 CCryptoManager。
 *                 文件保留作历史参考，勿删除。
 */

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <cstdio>
#include <unistd.h>
#include <array>
#include <cstdlib>
#include <mutex>
#include <algorithm>
#include <iterator>
#include <iomanip>

#include "IpcRet.h"
#include "dlog.h"
#include "public_define.h"
#include "path_define.h"

extern "C"
{
#include <gmssl/asn1.h>
#include <gmssl/base64.h>
#include <gmssl/error.h>
#include <gmssl/rand.h>
#include <gmssl/pkcs8.h>
#include <gmssl/mem.h>
#include <gmssl/sm2.h>
#include <gmssl/x509.h>
#include <gmssl/x509_cer.h>
#include <gmssl/x509_crl.h>
#include <gmssl/x509_ext.h>
#include <gmssl/x509_req.h>
#include <gmssl/pem.h>
}

/*sm2算法私钥解密的密码*/
#define SM2_PRIVATE_KEY_ENCRYPT_PASSWORD "12345678"

class CGmSSL
{
private:
    CGmSSL();
    static CGmSSL *m_self;
    static std::mutex m_mutex;

public:
    static CGmSSL *instance()
    {
        if (m_self == nullptr) // 第一层检查
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_self == nullptr) // 第二层检查
            {
                m_self = new CGmSSL();
            }
        }
        return m_self;
    }

    ~CGmSSL();

    // 生成随机数
    std::string rand(size_t length, bool hex_output = false);

    /**
     * @brief   : 随机数获取
     * @param    {uint8_t*} buf：存储随机数缓冲区指针
     * @param    {size_t} buflen：需要多少位随机数
     * @return   {int} OK：成功，非0：失败
     * @note    : 委托给 ICryptoProvider 实现，兼容 C 接口
     */
    int randomNumber_get(uint8_t *buf, size_t buflen);

    // 生成SM2密钥对
    void sm2keygen(const std::string &pass, const std::string &privkey_path,
                   const std::string &pubkey_path);
    void sm2keygen(const std::string &pass, const std::string &privkey_path);

    // SM2签名
    std::string sm2sign(const std::string &privkey_path, const std::string &pass,
                        const std::string &data, const std::string &id = "1234567812345678");

    // SM2验签
    bool sm2verify(const std::string &pubkey_path, const std::string &data,
                   const std::string &signature, const std::string &id = "1234567812345678");

    // 使用SM2私钥解密
    std::string  sm2decrypt(const std::vector<uint8_t> &bytesInput, const char *strKeyPath);

    // SM3杂凑计数
    std::vector<uint8_t> sm3(const std::vector<uint8_t> &bytesInput);

    // SM4加密（CBC模式）
    std::string sm4_encrypt_cbc(const std::string &key, const std::string &iv,
                                const std::string &plaintext);

    // SM4解密（CBC模式）
    std::string sm4_decrypt_cbc(const std::string &key, const std::string &iv,
                                const std::string &ciphertext);

    /**
     * @brief   : 生成证书请求文件
     * @param   {std::string} strCountry：国家代码
     * @param   {std::string} strState：省份或州名
     * @param   {std::string} strLocality：城市或地区
     * @param   {std::string} strOrganization：组织名称
     * @param   {std::string} strOrganizationUnit：组织单位
     * @param   {std::string} strCommonName：通用名称
     * @param   {std::string} strKeyFile：私钥文件路径
     * @param   {std::string} strPassword：私钥文件密码
     * @param   {std::string} strOutputFile：输出的证书请求文件路径
     * @return  {bool} true:成功，失败:false
     * @note    : 对应命令 gmssl reqgen，生成PEM格式的证书请求文件
     */
    bool reqgen(const std::string &strCountry,
               const std::string &strState,
               const std::string &strLocality,
               const std::string &strOrganization,
               const std::string &strOrganizationUnit,
               const std::string &strCommonName,
               const std::string &strKeyFile,
               const std::string &strPassword,
               const std::string &strOutputFile);

    /**
     * @brief       : 生成自签名SM2证书，一般用于根CA签发自己的数字证书
     * @author      : zhouzirui
     * @param        {string} &countryName：证书使用者所属的国家或地区
     * @param        {string} &province：证书使用者所属的省/市/自治区
     * @param        {string} &locality：证书使用者所在地
     * @param        {string} &organization：证书使用者的组织
     * @param        {string} &organizationUnit：证书使用者的组织机构
     * @param        {string} &commonName：证书使用者常用名称
     * @param        {int} days：证书有效期
     * @param        {string} &keyFile：SM2私钥
     * @param        {string} &password：SM2私钥加密口令
     * @param        {string} &certFile：生成的自签名证书文件路径
     * @return       {*}
     */
    std::string certgen(const std::string &countryName,
                        const std::string &province,
                        const std::string &locality,
                        const std::string &organization,
                        const std::string &organizationUnit,
                        const std::string &commonName,
                        int days,
                        const std::string &keyFile,
                        const std::string &password,
                        const std::string &certFile);

    /**
     * @brief   : 证书请求分析
     * @param    {string} &strPath 证书请求路径
     * @return   {string} 证书请求分析结果
     */
    std::string reqparse(const std::string &strPath);

    /**
     * @brief   : 证书吊销列表分析
     * @param    {string} &strPath 证书吊销列表路径
     * @return   {string} 证书吊销列表分析结果
     */
    std::string crlparse(const std::string &strPath);

    /**
     * @brief   : 验证 CRL 的合法性
     * @param    {string} &strCrlPath 待验证证书吊销列表路径
     * @param    {string} &strCertPath 待验证证书路径
     * @return   {string} 验证结果 Verification success:验证成功
     */
    std::string crlverify(const std::string &strCrlPath, const std::string &strCertPath);

    /**
     * @brief   : 证书分析
     * @param    {string} &strPath 证书路径
     * @return   {string} 证书分析结果
     */
    std::string certparse(const std::string &strPath);

    /**
     * @brief       : 从文件读取证书
     * @author      : zhouzirui
     * @param        {string} &strCertPath：证书路径
     * @return       {*}0：成功，非零：失败 
     */ 
    int readCert_from_file(const std::string &strCertPath);

    /**
     * @brief       : sm2数字签名
     * @author      : zhouzirui
     * @param        {vector<uint8_t>} &inputData：待签名数据
     * @param        {string} &strOutData：签名结果
     * @param        {char} *strKeyPath：私钥路径
     * @return       {*}0：成功，非零：失败
     */
    int digitalSignature(const std::vector<uint8_t> &inputData, std::string &strOutData, const char *strKeyPath);

    int verifySignature(const std::vector<uint8_t> &bytesSignData, const std::vector<uint8_t> &strVerifySign, const char *strCertPath);

    /*base64加密*/
    std::vector<uint8_t> base64Encode(const std::vector<uint8_t>& bytesInput);
    std::vector<uint8_t> base64Encode(const std::string &strInput);
    std::string base64EncodeToString(const std::string &strInput);
    std::string base64EncodeToString(const std::vector<uint8_t> &bytesInput);
    /*base64解密*/
    std::vector<uint8_t> base64Decode(const char *pInput);
    std::vector<uint8_t> base64Decode(const std::string &strInput);

    /**
     * @brief       : 容器字节流转16进制字符串
     * @author      : zhouzirui
     * @param        {vector<uint8_t>} &data：容器字节流
     * @return       {*}""：失败，str::string：输出成功
     */
    std::string vectorToHexString(const std::vector<uint8_t> &data);

private:
    /**
     * @brief       : 执行命令并获取输出
     * @author      : zhouzirui
     * @param        {string} &strCmd：执行的命令
     * @return       {*}""：失败，str::string：输出成功
     */
    std::string exec(const std::string &strCmd);

    /*创建临时文件*/
    std::string create_tempfile(const std::string &content);

    /*删除临时文件*/
    void remove_tempfile(const std::string &filename);

    /*SM4加解密通用方法*/
    std::string sm4_crypt(const std::string &key, const std::string &iv,
                          const std::string &data, bool encrypt, bool cbc_mode);

    /**
     * @brief       : 读取文件内容至str::string
     * @author      : zhouzirui
     * @param        {string} &strFilePath：读取的文件路径
     * @return       {*}""：失败，str::string：成功
     */
    std::string readFileToString(const std::string &strFilePath);

    /**
     * @brief       : 读取文件内容至std::vector<uint8_t>
     * @author      : zhouzirui
     * @param        {string} &strFilePath：读取的文件路径
     * @return       {*}空：失败，std::vector<uint8_t>：成功
     */
    std::vector<uint8_t> readFileToBytes(const std::string &strFilePath);

    /**
     * @brief       : 写入str::string数据至文件
     * @author      : zhouzirui
     * @param        {string} &strFilePath：写入的文件路径
     * @param        {string} &strWriteData：写入的数据
     */
    void writeStringToFile(const std::string &strFilePath, const std::string &strWriteData);

    /**
     * @brief       : 写入std::vector<uint8_t>数据至文件
     * @author      : zhouzirui
     * @param        {string} &strFilePath：写入的文件路径
     * @param        {vector<uint8_t>} &vecData：写入的数据
     */
    void writeBytesToFile(const std::string &strFilePath, const std::vector<uint8_t> &vecData);
};
