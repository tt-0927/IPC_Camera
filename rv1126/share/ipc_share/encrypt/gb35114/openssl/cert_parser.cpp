/**
 * @FilePath     : cert_parser.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-26 16:49:40
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-27 16:55:13
 * @Description  : 证书结构化解析模块
 */

#include "cert_parser.h"
#include "dlog.h"

#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <sstream>
#include <iomanip>

/**
 * @brief   : 静态辅助函数，从 X509_NAME 中提取指定 NID 的值
 * @param    {X509_NAME*} name：X509 名称结构体
 * @param    {int} nid：NID 标识符（如 NID_commonName）
 * @return   {std::string} 提取的字段值，失败返回空字符串
 */
static std::string get_name_entry_by_nid(X509_NAME *name, int nid)
{
    if (!name)
        return "";

    int idx = X509_NAME_get_index_by_NID(name, nid, -1);
    if (idx < 0)
        return "";

    X509_NAME_ENTRY *entry = X509_NAME_get_entry(name, idx);
    if (!entry)
        return "";

    ASN1_STRING *str = X509_NAME_ENTRY_get_data(entry);
    if (!str)
        return "";

    const unsigned char *data = ASN1_STRING_get0_data(str);
    int len = ASN1_STRING_length(str);
    return std::string(reinterpret_cast<const char *>(data), len);
}

/**
 * @brief   : 静态辅助函数，格式化 ASN1_TIME 为字符串
 * @param    {const ASN1_TIME*} time：ASN1 时间结构体
 * @return   {std::string} 格式化后的时间字符串，失败返回空字符串
 */
static std::string format_asn1_time(const ASN1_TIME *time)
{
    if (!time)
        return "";

    BIO *bio = BIO_new(BIO_s_mem());
    if (!bio)
        return "";

    if (ASN1_TIME_print(bio, time) != 1)
    {
        BIO_free(bio);
        return "";
    }

    BUF_MEM *bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free(bio);

    return result;
}

/**
 * @brief   : 静态辅助函数，序列号转十六进制字符串
 * @param    {const ASN1_INTEGER*} serial：ASN1 整数序列号
 * @return   {std::string} 十六进制字符串，失败返回空字符串
 * @note    : 使用 BIGNUM 进行 ASN1_INTEGER 到十六进制的转换
 */
static std::string serial_to_hex(const ASN1_INTEGER *serial)
{
    if (!serial)
        return "";

    // memory: BIGNUM 由 ASN1_INTEGER_to_BN 分配，需手动释放
    BIGNUM *bn = ASN1_INTEGER_to_BN(serial, nullptr);
    if (!bn)
        return "";

    char *hex = BN_bn2hex(bn);
    std::string result;
    if (hex)
    {
        result = hex;
        OPENSSL_free(hex);
    }
    BN_free(bn);

    return result;
}

/**
 * @brief   : 从文件加载 X509 证书
 * @param    {std::string} &strCertPath 证书文件路径
 * @return   {X509*} 成功返回证书对象，失败返回 nullptr
 * @note    : 调用者负责通过 X509_free 释放返回对象。
 */
static X509 *load_x509_certificate(const std::string &strCertPath)
{
    if (strCertPath.empty())
    {
        return nullptr;
    }

    FILE *pFile = fopen(strCertPath.c_str(), "r");
    if (pFile == nullptr)
    {
        dlog_error("load_x509_certificate: 无法打开证书文件 %s", strCertPath.c_str());
        return nullptr;
    }

    X509 *pCert = PEM_read_X509(pFile, nullptr, nullptr, nullptr);
    if (pCert == nullptr)
    {
        rewind(pFile);
        pCert = d2i_X509_fp(pFile, nullptr);
    }
    fclose(pFile);

    if (pCert == nullptr)
    {
        dlog_error("load_x509_certificate: 无法解析证书文件 %s", strCertPath.c_str());
    }
    return pCert;
}

/**
 * @brief   : 解析证书文件
 * @param    {const std::string &} cert_path：证书文件路径（PEM 或 DER 格式）
 * @param    {CertInfo_S &} info：证书信息输出结构体
 * @return   {bool} true：成功，false：失败
 * @note    : 支持 PEM 和 DER 两种格式，自动检测
 */
bool parse_certificate(const std::string &cert_path, CertInfo_S &info)
{
    if (cert_path.empty())
    {
        dlog_error("parse_certificate: 证书路径为空");
        return false;
    }

    FILE *fp = fopen(cert_path.c_str(), "r");
    if (!fp)
    {
        dlog_error("parse_certificate: 无法打开证书文件 %s", cert_path.c_str());
        return false;
    }

    // step 1: 尝试 PEM 格式读取
    X509 *cert = PEM_read_X509(fp, nullptr, nullptr, nullptr);
    if (!cert)
    {
        // step 2: PEM 失败，尝试 DER 格式读取
        rewind(fp);
        cert = d2i_X509_fp(fp, nullptr);
    }
    fclose(fp);

    if (!cert)
    {
        dlog_error("parse_certificate: 无法解析证书文件 %s", cert_path.c_str());
        return false;
    }

    // step 3: 提取序列号
    ASN1_INTEGER *serial = X509_get_serialNumber(cert);
    info.strSerialNumber = serial_to_hex(serial);

    // step 4: 提取 Subject CN
    X509_NAME *subject = X509_get_subject_name(cert);
    info.strSubjectCN = get_name_entry_by_nid(subject, NID_commonName);

    // step 5: 提取 Issuer CN
    X509_NAME *issuer = X509_get_issuer_name(cert);
    info.strIssuerCN = get_name_entry_by_nid(issuer, NID_commonName);

    // step 6: 提取有效期
    info.strNotBefore = format_asn1_time(X509_get_notBefore(cert));
    info.strNotAfter = format_asn1_time(X509_get_notAfter(cert));

    // step 7: 提取 KeyUsage 扩展
    ASN1_BIT_STRING *keyUsage = static_cast<ASN1_BIT_STRING *>(X509_get_ext_d2i(cert, NID_key_usage, nullptr, nullptr));
    if (keyUsage)
    {
        info.bKeyUsageDigitalSignature = (keyUsage->data[0] & KU_DIGITAL_SIGNATURE) != 0;
        info.bKeyUsageKeyCertSign = (keyUsage->data[0] & KU_KEY_CERT_SIGN) != 0;
        info.bKeyUsageCRLSign = (keyUsage->data[0] & KU_CRL_SIGN) != 0;
        ASN1_BIT_STRING_free(keyUsage);
    }

    // step 8: 提取 BasicConstraints 扩展
    BASIC_CONSTRAINTS *basicConstraints = static_cast<BASIC_CONSTRAINTS *>(
        X509_get_ext_d2i(cert, NID_basic_constraints, nullptr, nullptr));
    if (basicConstraints)
    {
        info.bBasicConstraintsCA = basicConstraints->ca != 0;
        if (basicConstraints->pathlen)
        {
            info.iPathLenConstraint = ASN1_INTEGER_get(basicConstraints->pathlen);
        }
        BASIC_CONSTRAINTS_free(basicConstraints);
    }

    // step 9: 提取 SubjectAltName 扩展
    GENERAL_NAMES *sanNames = static_cast<GENERAL_NAMES *>(X509_get_ext_d2i(cert, NID_subject_alt_name, nullptr, nullptr));
    if (sanNames)
    {
        int numNames = sk_GENERAL_NAME_num(sanNames);
        for (int i = 0; i < numNames; i++)
        {
            GENERAL_NAME *genName = sk_GENERAL_NAME_value(sanNames, i);
            if (genName->type == GEN_DNS || genName->type == GEN_IPADD)
            {
                ASN1_STRING *str = nullptr;
                if (genName->type == GEN_DNS)
                {
                    str = genName->d.dNSName;
                }
                else
                {
                    str = genName->d.iPAddress;
                }
                const unsigned char *data = ASN1_STRING_get0_data(str);
                int len = ASN1_STRING_length(str);
                info.vecSubjectAltName.push_back(std::string(reinterpret_cast<const char *>(data), len));
            }
        }
        sk_GENERAL_NAME_pop_free(sanNames, GENERAL_NAME_free);
    }

    // memory: 释放证书对象
    X509_free(cert);
    dlog_info("parse_certificate: 成功解析证书 %s", cert_path.c_str());
    return true;
}

/**
 * @brief   : 解析证书请求文件
 * @param    {const std::string &} csr_path：CSR 文件路径
 * @param    {CertInfo_S &} info：证书请求信息输出结构体
 * @return   {bool} true：成功，false：失败
 * @note    : 支持 PEM 和 DER 两种格式，仅提取 Subject CN
 */
bool parse_csr(const std::string &csr_path, CertInfo_S &info)
{
    if (csr_path.empty())
    {
        dlog_error("parse_csr: CSR路径为空");
        return false;
    }

    FILE *fp = fopen(csr_path.c_str(), "r");
    if (!fp)
    {
        dlog_error("parse_csr: 无法打开CSR文件 %s", csr_path.c_str());
        return false;
    }

    /* 尝试PEM格式读取 */
    X509_REQ *req = PEM_read_X509_REQ(fp, nullptr, nullptr, nullptr);
    if (!req)
    {
        /* 尝试DER格式读取 */
        rewind(fp);
        req = d2i_X509_REQ_fp(fp, nullptr);
    }
    fclose(fp);

    if (!req)
    {
        dlog_error("parse_csr: 无法解析CSR文件 %s", csr_path.c_str());
        return false;
    }

    /* 提取Subject CN */
    X509_NAME *subject = X509_REQ_get_subject_name(req);
    info.strSubjectCN = get_name_entry_by_nid(subject, NID_commonName);

    /* 提取其他Subject字段 */
    info.strSerialNumber = "";
    info.strIssuerCN = "";
    info.strNotBefore = "";
    info.strNotAfter = "";
    info.bKeyUsageDigitalSignature = false;
    info.bKeyUsageKeyCertSign = false;
    info.bKeyUsageCRLSign = false;
    info.bBasicConstraintsCA = false;
    info.iPathLenConstraint = -1;
    info.vecSubjectAltName.clear();

    X509_REQ_free(req);
    dlog_info("parse_csr: 成功解析CSR %s", csr_path.c_str());
    return true;
}

/**
 * @brief   : 解析 CRL 文件
 * @param    {const std::string &} crl_path：CRL 文件路径
 * @param    {std::string &} info_str：CRL 信息输出字符串
 * @return   {bool} true：成功，false：失败
 * @note    : 支持 PEM 和 DER 两种格式，使用 OpenSSL 文本输出格式化
 */
bool parse_crl(const std::string &crl_path, std::string &info_str)
{
    if (crl_path.empty())
    {
        dlog_error("parse_crl: CRL路径为空");
        return false;
    }

    FILE *fp = fopen(crl_path.c_str(), "r");
    if (!fp)
    {
        dlog_error("parse_crl: 无法打开CRL文件 %s", crl_path.c_str());
        return false;
    }

    /* 尝试PEM格式读取 */
    X509_CRL *crl = PEM_read_X509_CRL(fp, nullptr, nullptr, nullptr);
    if (!crl)
    {
        /* 尝试DER格式读取 */
        rewind(fp);
        crl = d2i_X509_CRL_fp(fp, nullptr);
    }
    fclose(fp);

    if (!crl)
    {
        dlog_error("parse_crl: 无法解析CRL文件 %s", crl_path.c_str());
        return false;
    }

    /* 使用OpenSSL文本输出格式化CRL信息 */
    BIO *bio = BIO_new(BIO_s_mem());
    if (!bio)
    {
        X509_CRL_free(crl);
        return false;
    }

    if (X509_CRL_print(bio, crl) != 1)
    {
        BIO_free(bio);
        X509_CRL_free(crl);
        return false;
    }

    BUF_MEM *bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    info_str = std::string(bufferPtr->data, bufferPtr->length);

    BIO_free(bio);
    X509_CRL_free(crl);

    dlog_info("parse_crl: 成功解析CRL %s", crl_path.c_str());
    return true;
}

/**
 * @brief   : 验证 CRL 签名
 * @param    {const std::string &} crl_path：CRL 文件路径
 * @param    {const std::string &} issuer_cert_path：签发者证书路径
 * @return   {bool} true：验证成功，false：验证失败
 * @note    : 使用签发者证书的公钥验证 CRL 签名
 */
bool verify_crl(const std::string &crl_path, const std::string &issuer_cert_path)
{
    if (crl_path.empty() || issuer_cert_path.empty())
    {
        dlog_error("verify_crl: 参数为空");
        return false;
    }

    /* 加载CRL */
    FILE *fp = fopen(crl_path.c_str(), "r");
    if (!fp)
    {
        dlog_error("verify_crl: 无法打开CRL文件 %s", crl_path.c_str());
        return false;
    }

    X509_CRL *crl = PEM_read_X509_CRL(fp, nullptr, nullptr, nullptr);
    if (!crl)
    {
        rewind(fp);
        crl = d2i_X509_CRL_fp(fp, nullptr);
    }
    fclose(fp);

    if (!crl)
    {
        dlog_error("verify_crl: 无法解析CRL文件 %s", crl_path.c_str());
        return false;
    }

    /* 加载签发者证书 */
    fp = fopen(issuer_cert_path.c_str(), "r");
    if (!fp)
    {
        dlog_error("verify_crl: 无法打开证书文件 %s", issuer_cert_path.c_str());
        X509_CRL_free(crl);
        return false;
    }

    X509 *issuer_cert = PEM_read_X509(fp, nullptr, nullptr, nullptr);
    if (!issuer_cert)
    {
        rewind(fp);
        issuer_cert = d2i_X509_fp(fp, nullptr);
    }
    fclose(fp);

    if (!issuer_cert)
    {
        dlog_error("verify_crl: 无法解析证书文件 %s", issuer_cert_path.c_str());
        X509_CRL_free(crl);
        return false;
    }

    /* 验证CRL签名 */
    EVP_PKEY *pubkey = X509_get_pubkey(issuer_cert);
    if (!pubkey)
    {
        dlog_error("verify_crl: 无法获取证书公钥");
        X509_free(issuer_cert);
        X509_CRL_free(crl);
        return false;
    }

    int ret = X509_CRL_verify(crl, pubkey);
    EVP_PKEY_free(pubkey);
    X509_free(issuer_cert);
    X509_CRL_free(crl);

    if (ret != 1)
    {
        dlog_error("verify_crl: CRL验证失败");
        return false;
    }

    dlog_info("verify_crl: CRL验证成功");
    return true;
}

/**
 * @brief   : 使用 CA 证书验证设备证书签名
 * @param    {std::string} &cert_path 待验证证书路径
 * @param    {std::string} &issuer_cert_path 签发者 CA 证书路径
 * @return   {bool} true：验证成功，false：验证失败
 * @note    : 签名数学验证是硬门槛；兼容 LiveGBS 使用同一 CA 私钥签名但 Issuer DN 不规范的证书。
 */
bool verify_certificate_by_ca(const std::string &cert_path, const std::string &issuer_cert_path)
{
    if (cert_path.empty() || issuer_cert_path.empty())
    {
        dlog_error("verify_certificate_by_ca: 参数为空");
        return false;
    }

    X509 *pCert = load_x509_certificate(cert_path);
    if (pCert == nullptr)
    {
        return false;
    }

    X509 *pIssuerCert = load_x509_certificate(issuer_cert_path);
    if (pIssuerCert == nullptr)
    {
        X509_free(pCert);
        return false;
    }

    /*
     * step: 先记录标准 X.509 签发关系结果，再独立验证签名数学关系。
     * LiveGBS 某些版本会使用 CA 私钥签名设备证书，但写入不同的 Issuer DN。
     */
    const int nIssuedRet = X509_check_issued(pIssuerCert, pCert);

    EVP_PKEY *pPubKey = X509_get_pubkey(pIssuerCert);
    if (pPubKey == nullptr)
    {
        dlog_error("verify_certificate_by_ca: 无法获取 CA 公钥");
        X509_free(pIssuerCert);
        X509_free(pCert);
        return false;
    }

    const int nSignatureRet = X509_verify(pCert, pPubKey);
    EVP_PKEY_free(pPubKey);
    X509_free(pIssuerCert);
    X509_free(pCert);

    if (nSignatureRet != 1)
    {
        dlog_error("verify_certificate_by_ca: 设备证书签名验证失败, issued_ret=%d(%s), signature_ret=%d",
                   nIssuedRet,
                   X509_verify_cert_error_string(nIssuedRet),
                   nSignatureRet);
        return false;
    }

    if (nIssuedRet != X509_V_OK)
    {
        /*
         * warn: 仅在当前 CA 公钥已验证签名有效时兼容 DN 不一致。
         * 数学签名证明设备证书由当前 CA 私钥签发，不能退化为仅比较 CN。
         */
        dlog_warn("verify_certificate_by_ca: 签名有效但签发关系不规范，按LiveGBS兼容模式放行, "
                  "issued_ret=%d(%s), signature_ret=%d",
                  nIssuedRet,
                  X509_verify_cert_error_string(nIssuedRet),
                  nSignatureRet);
        return true;
    }

    dlog_info("verify_certificate_by_ca: 标准X.509签发关系和设备证书签名验证成功");
    return true;
}
