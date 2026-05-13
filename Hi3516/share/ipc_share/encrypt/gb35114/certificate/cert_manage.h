/**
 * @FilePath     : cert_manage.h
 * @Author       : zhouzirui
 * @Date         : 2025-04-09 16:26:31
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-04-21 17:05:53
 * @Description  : 证书管理模块 基于GmSSL X.509
 */
#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cstring>
#include <fstream>
#include <ctime>
#include <filesystem>

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
#define SM2_PRIVATE_KEY_ENCRYPT_PASSWORD "itc2023"
//info /*----------------------- 密钥管理 -----------------------*/

/*证书类型 X509_CERT_TYPE*/
typedef enum
{
    CERT_TYPE_UNKNOWN = -1,  // 未知类型（默认值或错误状态）
    CERT_TYPE_SELF_SIGNED,  // 自签名根证书
    CERT_TYPE_INTERMEDIATE, // 中间证书
    CERT_TYPE_SIGNING,      // 签名证书（终端实体签名用途）
    CERT_TYPE_ENCRYPTION,   // 加密证书（终端实体加密用途）
    CERT_TYPE_MAX,
} CertType_E;

/*证书颁发者或主体属性*/
typedef struct CertIssuerSubjectInfo
{
    std::string strCountry;            // 国家代码
    std::string strLocality;           // 地区名称
    std::string strStateOrProvince;    // 州或省名称
    std::string strOrganization;       // 组织名称
    std::string strOrganizationalUnit; // 部门名称
    std::string strCommonName;         // 通用名称
    SM2_KEY *pKey;                     // SM2密钥指针
    void clear()
    {
        strCountry = "CN";
        strLocality = "Panyu";
        strStateOrProvince = "Guangzhou";
        strOrganization = "itc";
        strOrganizationalUnit = "Shijue";
        strCommonName = "CA";
        pKey = nullptr;
    }
} CertIssuerSubjectInfo_S;

/*证书生成参数结构体*/
typedef struct CertParams
{
    CertType_E enCertType;                     // 证书类型
    CertIssuerSubjectInfo_S stCertIssuerInfo;  // 证书颁发者属性
    CertIssuerSubjectInfo_S stCertSubjectInfo; // 证书主体属性
    int nValidityDays;                         // 有效期天数
    int nKeyUsage;                             // 密钥用法标志位
    bool bIsCa;                                // 是否为CA证书
    int nX509Version;                          // X.509证书版本 X509_Version
    std::string strCertPath;                   // 证书存储路径
    int nPathLen;                              // 用于限制CA证书能签发的下级证书链深度
    void clear()
    {
        enCertType = CERT_TYPE_UNKNOWN;
        stCertIssuerInfo.clear();
        stCertSubjectInfo.clear();
        nValidityDays = 3650;
        nKeyUsage = X509_KU_KEY_CERT_SIGN | X509_KU_CRL_SIGN;
        bIsCa = true;
        nX509Version = X509_version_v3;
        strCertPath = "";
        nPathLen = 0;
    }
} CertParams_S;

// 证书管理类
class CCertManage
{
public:
    CCertManage();
    ~CCertManage();

    // 生成SM2密钥对
    int generateKeyPair();

    // 从文件读取SM2密钥
    int readKeyPair_from_file();

    // 生成证书签名请求，用于给服务端签发设备证书
    int generateCSR(CertParams_S &stParams, std::string &strCsrDer);

    // 生成X.509证书
    int generateCertificate(CertParams_S &stParams);

    // 吊销证书
    int revokeCertificate(const std::string &serial_number,
                           int reason_code,
                           std::string &crl_der);

    // 从文件读取对应X.509证书内容
    int readCertificate_from_file();

    //生成证书请求
    // int generateCertificate_request();
    
    // 数字签名
    int digitalSignature(const uint8_t *pInputData, size_t szInputLen, uint8_t *pOutData, size_t *pOutLen);

private:
    /*SM2证书密钥*/
    SM2_KEY stSM2Key[CERT_TYPE_MAX];
    
    /*SM2根证书数据*/
    std::string strRootCertDer;
    /*SM2中间证书数据*/
    std::string strMiddleCertDer;
    /*SM2签名证书数据*/
    std::string strSigningCertDer;
    /*SM2加密证书数据*/
    std::string strEncryptionCertDer;

    /*设置证书 颁发者/主体 名称*/
    int setIssuerSubjectName(const CertIssuerSubjectInfo_S &stInfo, uint8_t *pName, size_t *pNameLen, size_t maxLen);

    /*保存证书至文件*/
    int saveCertToFile(CertType_E enCertType, const uint8_t *pData, size_t szLen);

    /*保存证书请求至文件*/
    int saveCsrToFile(const uint8_t *pData, size_t szLen);

    // 保存SM2密钥对
    int saveKeyPairToFile(CertType_E enCertType,SM2_KEY &stSM2Key);

    /**
     * Base64 编码函数
     * @param input 要编码的二进制数据
     * @param inlen 输入数据的长度
     * @param output 存储编码结果的缓冲区
     * @param outlen 输入时为输出缓冲区大小，输出时为实际编码后的长度
     * @return 成功返回0，失败返回-1
     */
    int base64Encode(const uint8_t *input, size_t inlen, uint8_t *output, size_t *outlen);

    /**
     * Base64 解码函数
     * @param input 要解码的Base64字符串
     * @param inlen 输入字符串的长度
     * @param output 存储解码结果的缓冲区
     * @param outlen 输入时为输出缓冲区大小，输出时为实际解码后的长度
     * @return 成功返回0，失败返回-1
     */
    int base64Decode(const uint8_t *input, size_t inlen, uint8_t *output, size_t *outlen);
};
