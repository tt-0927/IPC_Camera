/*
 * @FilePath: ca_manage.cpp
 * @Author: tianl
 * @Date: 2024-09-24 19:36:05
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-08-07 11:09:19
 * @Description: CA证书相关操作
 */

#include <filesystem>
#include "ca_manage.h"
#include "dlog.h"
#include "ca_file_database.h"
#include "IpcRet.h"

namespace fs = std::filesystem;

CCaManage::CCaManage()
{
    /* openssl库初始化 */
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
}

CCaManage::~CCaManage()
{
    EVP_cleanup();
    ERR_free_strings();
}

/* 获取受信任证书信息 */
int CCaManage::getTrustInfo(std::vector<Network::CertFileInfo_S> &stCertInfo)
{
    Db::CaFileDatabase::instance()->trust_get_all_items(stCertInfo);
    return 0;
}

/* 安装受信任证书信息 */
int CCaManage::installTrustCert(Network::CertFileInfo_S &stCertFileInfo)
{
    int nRet;
    /* 获取证书信息 */
    nRet = getCertificateInfo(stCertFileInfo.strPath, stCertFileInfo);
    if (nRet < 0)
    {
        dlog_error("获取证书信息失败");
        return -1;
    }

    /* 证书文件添加到数据库 */
    Db::CaFileDatabase::instance()->trust_add(stCertFileInfo);

    return 0;
}

/* 删除受信任证书信息 */
int CCaManage::deleteTrustceCert(Network::CertFileInfo_S &stCertFileInfo)
{
    int nRet;
    /* 删除数据库 */
    nRet = Db::CaFileDatabase::instance()->trust_del(stCertFileInfo);

    if (nRet < 0)
    {
        dlog_error("删除证书文件数据库失败: %s", stCertFileInfo.strPath.c_str());
    }

    /* 删除证书文件 */
    const std::string &strFilePath = stCertFileInfo.strPath;
    std::string cmd = "rm -f " + strFilePath;
    if (std::system(cmd.c_str()) != 0)
    {
        dlog_error("删除证书文件失败: %s", stCertFileInfo.strPath.c_str());
        return -1;
    }
    dlog_debug("成功删除文件: %s", strFilePath.c_str());

    return 0;
}

/* 获取设备证书信息 */
int CCaManage::getDeviceInfo(std::vector<Network::CertFileInfo_S> &stCertInfo)
{
    Db::CaFileDatabase::instance()->device_get_all_items(stCertInfo);
    return 0;
}

/* 安装设备证书 */
int CCaManage::installDeviceCert(Network::CertFileInfo_S &stCertFileInfo)
{
    int nRet;
    /* 获取证书信息 */
    nRet = getCertificateInfo(stCertFileInfo.strPath, stCertFileInfo);
    if (nRet < 0)
    {
        dlog_error("获取证书信息失败");
        return -1;
    }

    /* 证书文件添加到数据库 */
    Db::CaFileDatabase::instance()->device_add(stCertFileInfo);

    return 0;
}

/* 创建安装设备证书 */
int CCaManage::creatInstallDeviceCert(const Network::CertApplyInfo_S &stApplyInfo)
{
    std::string strCertFile;
    Network::CertFileInfo_S stCertInfo;
    int nRet;

    /* 生成证书并获取证书路径 */
    nRet = generateCertificate(stApplyInfo, strCertFile);
    if (nRet < 0)
    {
        dlog_error("生成证书失败");
        return -1;
    }

    /* 获取证书信息 */
    nRet = getCertificateInfo(strCertFile, stCertInfo);
    if (nRet < 0)
    {
        dlog_error("获取证书信息失败");
        return -1;
    }

    stCertInfo.strPath = strCertFile;
    /* 证书文件添加到数据库 */
    Db::CaFileDatabase::instance()->device_add(stCertInfo);

    return 0;
}

/* 删除设备证书 */
int CCaManage::deleteDeviceCert(const Network::CertFileInfo_S &stCertFileInfo)
{
    int nRet;
    /* 删除数据库 */
    nRet = Db::CaFileDatabase::instance()->device_del(stCertFileInfo);

    if (nRet < 0)
    {
        dlog_error("删除证书文件数据库失败: %s", stCertFileInfo.strPath.c_str());
    }

    /* 删除证书文件 */
    const std::string &strFilePath = stCertFileInfo.strPath;
    std::string cmd = "rm -f " + strFilePath;
    if (std::system(cmd.c_str()) != 0)
    {
        dlog_error("删除证书文件失败: %s", stCertFileInfo.strPath.c_str());
        return -1;
    }
    dlog_debug("成功删除文件: %s", strFilePath.c_str());
    return 0;
}

int CCaManage::deleteRequestCsr()
{
    std::string cmd = "rm -f " + std::string(CA_REQ_CSR);
    if (std::system(cmd.c_str()) != 0)
    {
        dlog_error("删除证书请求失败: %s");
        return -1;
    }
    dlog_debug("成功删除文件: %s", CA_REQ_CSR);
    return 0;
}

/* 生成密钥 */
EVP_PKEY *CCaManage::generateKey(int nBits)
{
    /* 为EVP PKEY结构分配内存 */
    EVP_PKEY *pkey = EVP_PKEY_new();
    if (!pkey)
    {
        dlog_error("无法创建EVP_PKEY结构");
        return NULL;
    }

    /* 生成RSA密钥并分配给pkey */
    RSA *prsa = RSA_generate_key(nBits, RSA_F4, NULL, NULL);
    if (!EVP_PKEY_assign_RSA(pkey, prsa))
    {
        dlog_error("无法生成2048位RSA密钥");
        EVP_PKEY_free(pkey);
        return NULL;
    }

    return pkey;
}

/* 生成请求文件 */
int CCaManage::generateCsr(const Network::CertApplyInfo_S &stApplyInfo)
{

    std::string strKeyFile = CA_REQ_KEY;
    std::string strCsrFile = CA_REQ_CSR;

    /* 读取私钥文件 */
    FILE *pKeyFile = fopen(strKeyFile.c_str(), "r");
    if (!pKeyFile)
    {
        dlog_error("无法打开私钥文件: %s", strerror(errno));
        return -1;
    }

    /* 从文件加载私钥 */
    EVP_PKEY *pKey = PEM_read_PrivateKey(pKeyFile, NULL, NULL, NULL);
    fclose(pKeyFile);
    ;

    /* 创建一个新的证书请求对象 */
    X509_REQ *pReq = X509_REQ_new();
    if (!pReq)
    {
        dlog_error("创建X509_REQ失败");
        return -1;
    }

    /* 创建一个新的 X509_NAME 对象 */
    X509_NAME *pName = X509_NAME_new();

    if (!stApplyInfo.strC.empty())
    {
        if (X509_NAME_add_entry_by_txt(pName, "C", MBSTRING_ASC,
                                       reinterpret_cast<const unsigned char *>(stApplyInfo.strC.c_str()), -1, -1, 0) != 1)
        {
            dlog_error("向X509_NAME添加国家名称失败");
            X509_NAME_free(pName);
            X509_REQ_free(pReq);
            return -1;
        }
    }

    if (!stApplyInfo.strST.empty())
    {
        if (X509_NAME_add_entry_by_txt(pName, "ST", MBSTRING_ASC,
                                       reinterpret_cast<const unsigned char *>(stApplyInfo.strST.c_str()), -1, -1, 0) != 1)
        {
            dlog_error("向X509_NAME添加状态名称失败");
            X509_NAME_free(pName);
            X509_REQ_free(pReq);
            return -1;
        }
    }

    if (!stApplyInfo.strL.empty())
    {
        if (X509_NAME_add_entry_by_txt(pName, "L", MBSTRING_ASC,
                                       reinterpret_cast<const unsigned char *>(stApplyInfo.strL.c_str()), -1, -1, 0) != 1)
        {
            dlog_error("向X509_NAME添加地区名称失败");
            X509_NAME_free(pName);
            X509_REQ_free(pReq);
            return -1;
        }
    }

    if (!stApplyInfo.strO.empty())
    {
        if (X509_NAME_add_entry_by_txt(pName, "O", MBSTRING_ASC,
                                       reinterpret_cast<const unsigned char *>(stApplyInfo.strO.c_str()), -1, -1, 0) != 1)
        {
            dlog_error("向X509_NAME添加组织名称失败");
            X509_NAME_free(pName);
            X509_REQ_free(pReq);
            return -1;
        }
    }

    if (!stApplyInfo.strOU.empty())
    {
        if (X509_NAME_add_entry_by_txt(pName, "OU", MBSTRING_ASC,
                                       reinterpret_cast<const unsigned char *>(stApplyInfo.strOU.c_str()), -1, -1, 0) != 1)
        {
            dlog_error("向X509_NAME添加组织单元名称失败");
            X509_NAME_free(pName);
            X509_REQ_free(pReq);
            return -1;
        }
    }

    if (!stApplyInfo.strCN.empty())
    {
        if (X509_NAME_add_entry_by_txt(pName, "CN", MBSTRING_ASC,
                                       reinterpret_cast<const unsigned char *>(stApplyInfo.strCN.c_str()), -1, -1, 0) != 1)
        {
            dlog_error("向X509_NAME添加通用名称失败");
            X509_NAME_free(pName);
            X509_REQ_free(pReq);
            return -1;
        }
    }

    if (!stApplyInfo.strEmail.empty())
    {
        if (X509_NAME_add_entry_by_txt(pName, "emailAddress", MBSTRING_ASC,
                                       reinterpret_cast<const unsigned char *>(stApplyInfo.strEmail.c_str()), -1, -1, 0) != 1)
        {
            dlog_error("向X509_NAME添加邮件地址失败");
            X509_NAME_free(pName);
            X509_REQ_free(pReq);
            return -1;
        }
    }

    /* 设置证书请求的主题名称 */
    X509_REQ_set_subject_name(pReq, pName);

    /* 设置证书请求的私钥 */
    if (X509_REQ_set_pubkey(pReq, pKey) != 1)
    {
        dlog_error("设置X509_REQ公钥失败");
        X509_REQ_free(pReq);
        return -1;
    }

    /* 签署证书请求 */
    if (X509_REQ_sign(pReq, pKey, EVP_sha256()) <= 0)
    {
        dlog_error("签名证书请求失败");
        X509_REQ_free(pReq);
        X509_NAME_free(pName);
        return -1;
    }

    /* 将证书请求写入文件 */
    FILE *pCsrFile = fopen(strCsrFile.c_str(), "wb");
    if (!pCsrFile)
    {
        dlog_error("打开文件失败：%s", strCsrFile.c_str());
        X509_REQ_free(pReq);
        X509_NAME_free(pName);
        return -1;
    }

    if (PEM_write_X509_REQ(pCsrFile, pReq) != 1)
    {
        dlog_error("向文件写入CSR失败");
        fclose(pCsrFile);
        X509_REQ_free(pReq);
        X509_NAME_free(pName);
        return -1;
    }

    fclose(pCsrFile);
    X509_REQ_free(pReq);
    X509_NAME_free(pName);

    return 0;
}

int CCaManage::generateCertificate(const Network::CertApplyInfo_S &stApplyInfo, std::string &strCertFile)
{
    /* 读取中间证书 */
    FILE *pCertFile = fopen(CA_MIDDLE_CERT, "r");
    if (!pCertFile)
    {
        dlog_error("打开中间证书文件失败:%s", CA_MIDDLE_CERT);
        return -1;
    }
    X509 *pInterCert = PEM_read_X509(pCertFile, nullptr, nullptr, nullptr);
    fclose(pCertFile);
    if (!pInterCert)
    {
        dlog_error("读取中级证书失败");
        return -1;
    }

    /* 读取中间证书私钥 */
    FILE *pInterKey_File = fopen(CA_MIDDLE_KEY, "r");
    if (!pInterKey_File)
    {
        dlog_error("打开中间密钥文件失败");
        X509_free(pInterCert);
        return -1;
    }
    EVP_PKEY *pInterKey = PEM_read_PrivateKey(pInterKey_File, nullptr, nullptr, nullptr);
    fclose(pInterKey_File);
    if (!pInterKey)
    {
        dlog_error("读取中间私钥失败");
        X509_free(pInterCert);
        return -1;
    }
    /* 读取设备私钥 */
    FILE *pDeviceKeyFile = fopen(CA_DEVICE_KEY, "r");
    if (!pDeviceKeyFile)
    {
        dlog_error("打开设备私钥文件失败: device.key");
        EVP_PKEY_free(pInterKey);
        X509_free(pInterCert);
        return -1;
    }

    EVP_PKEY *pDeviceKey = PEM_read_PrivateKey(pDeviceKeyFile, nullptr, nullptr, nullptr);
    fclose(pDeviceKeyFile);
    if (!pDeviceKey)
    {
        dlog_error("读取设备私钥失败");
        EVP_PKEY_free(pInterKey);
        X509_free(pInterCert);
        return -1;
    }

    /* 创建一个新的证书请求 */
    X509_REQ *preq = X509_REQ_new();
    // EVP_PKEY *pkey = EVP_PKEY_new();
    //
    ///* 生成RSA密钥，分配给证书请求的公钥 */
    // RSA *prsa = RSA_generate_key(KEY_LENGTH_2048, RSA_F4, NULL, NULL);
    // EVP_PKEY_assign_RSA(pkey, prsa);
    //
    ///* 设置证书请求的公钥 */
    // X509_REQ_set_pubkey(preq, pkey);

    /* 设置证书的主题名称 */
    X509_NAME *pName = X509_NAME_new();
    if (!pName)
    {
        dlog_error("创建X509 NAME对象失败");
        EVP_PKEY_free(pInterKey);
        X509_free(pInterCert);
        return -1;
    }

    /* 设置主题名称的各项字段 */
    if (!stApplyInfo.strC.empty())
    {
        if (X509_NAME_add_entry_by_txt(pName, "C", MBSTRING_ASC,
                                       reinterpret_cast<const unsigned char *>(stApplyInfo.strC.c_str()), -1, -1, 0) != 1)
        {
            dlog_error("向X509 name添加国家名称(C)失败");
            X509_NAME_free(pName);
            EVP_PKEY_free(pDeviceKey);
            EVP_PKEY_free(pInterKey);
            X509_free(pInterCert);
            return -1;
        }
    }

    if (!stApplyInfo.strST.empty())
    {
        if (X509_NAME_add_entry_by_txt(pName, "ST", MBSTRING_ASC,
                                       reinterpret_cast<const unsigned char *>(stApplyInfo.strST.c_str()), -1, -1, 0) != 1)
        {
            dlog_error("向X509 name添加状态名（ST）失败");
            X509_NAME_free(pName);
            EVP_PKEY_free(pDeviceKey);
            EVP_PKEY_free(pInterKey);
            X509_free(pInterCert);
            return -1;
        }
    }

    if (!stApplyInfo.strL.empty())
    {
        if (X509_NAME_add_entry_by_txt(pName, "L", MBSTRING_ASC,
                                       reinterpret_cast<const unsigned char *>(stApplyInfo.strL.c_str()), -1, -1, 0) != 1)
        {
            dlog_error("向X509 name添加位置名称(L)失败");
            X509_NAME_free(pName);
            EVP_PKEY_free(pDeviceKey);
            EVP_PKEY_free(pInterKey);
            X509_free(pInterCert);
            return -1;
        }
    }

    if (!stApplyInfo.strO.empty())
    {
        if (X509_NAME_add_entry_by_txt(pName, "O", MBSTRING_ASC,
                                       reinterpret_cast<const unsigned char *>(stApplyInfo.strO.c_str()), -1, -1, 0) != 1)
        {
            dlog_error("向X509_NAME添加组织名(O)失败");
            X509_NAME_free(pName);
            EVP_PKEY_free(pInterKey);
            EVP_PKEY_free(pDeviceKey);
            X509_free(pInterCert);
            return -1;
        }
    }

    if (!stApplyInfo.strOU.empty())
    {
        if (X509_NAME_add_entry_by_txt(pName, "OU", MBSTRING_ASC,
                                       reinterpret_cast<const unsigned char *>(stApplyInfo.strOU.c_str()), -1, -1, 0) != 1)
        {
            dlog_error("X509_NAME添加OU失败");
            X509_NAME_free(pName);
            EVP_PKEY_free(pDeviceKey);
            EVP_PKEY_free(pInterKey);
            X509_free(pInterCert);
            return -1;
        }
    }

    if (!stApplyInfo.strCN.empty())
    {
        if (X509_NAME_add_entry_by_txt(pName, "CN", MBSTRING_ASC,
                                       reinterpret_cast<const unsigned char *>(stApplyInfo.strCN.c_str()), -1, -1, 0) != 1)
        {
            dlog_error("向X509 name添加CN失败");
            X509_NAME_free(pName);
            EVP_PKEY_free(pDeviceKey);
            EVP_PKEY_free(pInterKey);
            X509_free(pInterCert);
            return -1;
        }
    }

    if (!stApplyInfo.strEmail.empty())
    {
        if (X509_NAME_add_entry_by_txt(pName, "emailAddress", MBSTRING_ASC,
                                       reinterpret_cast<const unsigned char *>(stApplyInfo.strEmail.c_str()), -1, -1, 0) != 1)
        {
            dlog_error("向X509 NAME添加电子邮件地址失败");
            X509_NAME_free(pName);
            EVP_PKEY_free(pDeviceKey);
            EVP_PKEY_free(pInterKey);
            X509_free(pInterCert);
            return -1;
        }
    }
    /* 设置申请者的主题名称 */
    X509_REQ_set_subject_name(preq, pName);
    X509_NAME_free(pName);

    /* 创建证书对象 */
    X509 *pCert = X509_new();
    if (!pCert)
    {
        dlog_error("创建X509对象失败");
        EVP_PKEY_free(pInterKey);
        X509_free(pInterCert);
        EVP_PKEY_free(pDeviceKey);
        return -1;
    }

    /* 设置证书的版本 */
    if (X509_set_version(pCert, CERTIFICATE_VERSION) != 1)
    {
        dlog_error("设置证书版本失败");
        X509_free(pCert);
        EVP_PKEY_free(pInterKey);
        X509_free(pInterCert);
        EVP_PKEY_free(pDeviceKey);
        return -1;
    }

    /* 生成随机序列号 */
    BIGNUM *pBn_serial = BN_new();
    if (!pBn_serial || !BN_rand(pBn_serial, SERIAL_NUMBER_BITS, BN_RAND_TOP_ONE, BN_RAND_BOTTOM_ANY))
    {
        dlog_error("无法生成随机序列号");
        BN_free(pBn_serial);
        X509_free(pCert);
        return -1;
    }

    ASN1_INTEGER *pAsn1_serial = BN_to_ASN1_INTEGER(pBn_serial, nullptr);
    if (!pAsn1_serial)
    {
        dlog_error("无法转换序列号");
        BN_free(pBn_serial);
        X509_free(pCert);
        EVP_PKEY_free(pDeviceKey);
        return -1;
    }

    /* 将序列号转换为 BIGNUM 以便转换为字符串 */
    std::unique_ptr<BIGNUM, decltype(&BN_free)> bn(ASN1_INTEGER_to_BN(pAsn1_serial, nullptr), BN_free);
    if (!bn)
    {
        dlog_error("无法转换序列号");
        return -1;
    }

    /*  将 BIGNUM 转换为十六进制字符串 */
    char *pChHexSerial = BN_bn2hex(bn.get());
    if (!pChHexSerial)
    {
        dlog_error("无法将BIGNUM转换为十六进制字符串");
        return -1;
    }

    std::string strserial(pChHexSerial);
    OPENSSL_free(pChHexSerial);

    /* 用序列号命名证书 */
    strCertFile = CA_DEVICE_PATH + strserial + ".cer";

    /* 设置证书的序列号 */
    X509_set_serialNumber(pCert, pAsn1_serial);

    BN_free(pBn_serial);
    ASN1_INTEGER_free(pAsn1_serial);

    /* 设置证书的有效期 */
    X509_gmtime_adj(X509_get_notBefore(pCert), -OFFSET_EIGHT_HOUR);
    X509_gmtime_adj(X509_get_notAfter(pCert), (long)DAYS_TO_SECONDS(stApplyInfo.nValday) - OFFSET_EIGHT_HOUR);

    /* 设置证书的签发者名称，使用中间证书的主题名称作为签发者 */
    X509_set_issuer_name(pCert, X509_get_subject_name(pInterCert)); // 设置签发者

    /* 设置申请者的主题名称（与证书请求一致） */
    X509_set_subject_name(pCert, X509_REQ_get_subject_name(preq)); // 设置申请者

    /* 设置证书的公钥 */
    X509_set_pubkey(pCert, pDeviceKey);

    /* 初始化 X509V3_CTX 上下文 */
    X509V3_CTX ctx;
    X509V3_set_ctx(&ctx, pInterCert, pCert, nullptr, nullptr, 0); // 使用中间证书作为签发者，当前证书为使用者

    /* 生成使用者密钥标识符，基于当前证书的公钥 */
    X509_EXTENSION *pExt = X509V3_EXT_conf_nid(nullptr, &ctx, NID_subject_key_identifier, "hash");
    if (pExt)
    {
        X509_add_ext(pCert, pExt, -1); // 添加到证书中
        X509_EXTENSION_free(pExt);
    }
    else
    {
        dlog_error("生成主题密钥标识符失败");
        X509_free(pCert);
        EVP_PKEY_free(pInterKey);
        X509_free(pInterCert);
        return -1;
    }

    /* 添加基本约束 */
    X509_EXTENSION *ext_bc = X509V3_EXT_conf_nid(nullptr, nullptr, NID_basic_constraints, "CA:FALSE");
    X509_add_ext(pCert, ext_bc, -1);
    X509_EXTENSION_free(ext_bc);

    /* 固定的 DNS 名称和 IP 地址 */
    const std::vector<std::string> fixed_dns_names = {"127.0.0.1", "[::1]"};
    const std::vector<std::string> fixed_ip_addresses = {"127.0.0.1", "::1"};

    /* 创建 SAN 扩展字符串 */
    std::string strSan;
    strSan += "DNS:" + stApplyInfo.strCN;

    /*  添加固定 DNS 名称 */
    for (const auto &dns : fixed_dns_names)
    {
        strSan += ",DNS:" + dns;
    }

    /* 添加第一个 IP 地址，使用用户输入的 DNS 名称作为 IP 地址 */
    strSan += ",IP:" + stApplyInfo.strCN;

    /* 添加固定 IP 地址 */
    for (const auto &ip : fixed_ip_addresses)
    {
        strSan += ",IP:" + ip;
    }

    /* 添加主题背景备用名称 */
    X509_EXTENSION *ext_san = X509V3_EXT_conf_nid(nullptr, nullptr, NID_subject_alt_name, strSan.c_str());
    X509_add_ext(pCert, ext_san, -1);
    X509_EXTENSION_free(ext_san);

    // 添加密钥用途（Key Usage）扩展
    X509_EXTENSION *ext_ku = X509V3_EXT_conf_nid(nullptr, nullptr, NID_key_usage, "digitalSignature,keyEncipherment");
    X509_add_ext(pCert, ext_ku, -1);
    X509_EXTENSION_free(ext_ku);

    // 添加扩展密钥用途（Extended Key Usage）
    X509_EXTENSION *ext_eku = X509V3_EXT_conf_nid(nullptr, nullptr, NID_ext_key_usage, "serverAuth");
    X509_add_ext(pCert, ext_eku, -1);
    X509_EXTENSION_free(ext_eku);

    /* 使用中间证书的私钥对证书进行签名，使用 SHA256 算法 */
    if (X509_sign(pCert, pInterKey, EVP_sha256()) <= 0)
    {
        dlog_error("证书签名失败");
        X509_free(pCert);
        EVP_PKEY_free(pInterKey);
        X509_free(pInterCert);
        EVP_PKEY_free(pDeviceKey);
        return -1;
    }

    /* 将证书写入文件 */
    FILE *pCertOut = fopen(strCertFile.c_str(), "wb");
    if (!pCertOut)
    {
        dlog_error("打开文件写入证书失败");
        X509_free(pCert);
        EVP_PKEY_free(pInterKey);
        X509_free(pInterCert);
        EVP_PKEY_free(pDeviceKey);
        return -1;
    }
    if (PEM_write_X509(pCertOut, pCert) != 1)
    {
        dlog_error("文件写入证书失败");
        fclose(pCertOut);
        X509_free(pCert);
        EVP_PKEY_free(pInterKey);
        X509_free(pInterCert);
        EVP_PKEY_free(pDeviceKey);
        return -1;
    }
    fclose(pCertOut);

    X509_free(pCert);
    EVP_PKEY_free(pInterKey);
    X509_free(pInterCert);
    EVP_PKEY_free(pDeviceKey);

    return 0;
}

std::string CCaManage::getCertificateExpirationDate(const std::string &strCertPath)
{
    X509 *pCert = nullptr;
    ASN1_TIME *pNotBefore = nullptr;
    ASN1_TIME *pNotAfter = nullptr;
    std::string strExpirationDate;

    /* 打开证书文件 */
    FILE *fp = fopen(strCertPath.c_str(), "r");
    if (fp == nullptr)
    {
        dlog_error("打开证书文件出错：%s", strCertPath.c_str());
        return "";
    }

    /* 读取证书 */
    pCert = PEM_read_X509(fp, nullptr, nullptr, nullptr);
    fclose(fp);
    if (pCert == nullptr)
    {
        dlog_error("读取证书文件出错：%s", strCertPath.c_str());
        return "";
    }

    pNotBefore = X509_get_notBefore(pCert);
    if (pNotBefore == nullptr)
    {
        dlog_error("从证书获取开始日期错误");
        X509_free(pCert);
        return "";
    }

    /* 获取证书的到期时间 */
    pNotAfter = X509_get_notAfter(pCert);
    if (pNotAfter == nullptr)
    {
        dlog_error("从证书获取截止日期错误");
        X509_free(pCert);
        return "";
    }

    /* 把获取到的ASN1_TIME转换为ISO-8601时间戳 */
    std::string strStartDate = convertASN1Time(pNotBefore);
    std::string strEndDate = convertASN1Time(pNotAfter);
    /* 转换为YYYY-MM-DD HH:MM:SS格式的北京时间 */
    strExpirationDate = convertGMTToBeijingTime(strStartDate) + "-" + convertGMTToBeijingTime(strEndDate);
    /* 释放资源 */
    X509_free(pCert);

    return strExpirationDate;
}

std::string CCaManage::convertASN1Time(const ASN1_TIME *pTime)
{
    /* 使用RAII创建一个BIO对象以进行自动清理 */
    std::unique_ptr<BIO, decltype(&BIO_free)> bio(BIO_new(BIO_s_mem()), BIO_free);
    if (!bio)
    {
        dlog_error("创建BIO对象失败");
        return "";
    }

    /* 打印ASN1时间到BIO */
    if (ASN1_TIME_print(bio.get(), pTime) <= 0)
    {
        dlog_error("ASN1 TIME打印失败或未写入数据");
        return "";
    }

    /* 将BIO中的数据读入字符串 */
    char chTempBuffer[DATA_LEN];
    int nBytesRead = BIO_gets(bio.get(), chTempBuffer, sizeof(chTempBuffer));
    if (nBytesRead <= 0)
    {
        dlog_error("BIO无法传输内容");
        return "";
    }

    return std::string(chTempBuffer, nBytesRead);
}

std::string CCaManage::convertGMTToBeijingTime(const std::string &strTimeStr)
{

    /* 去掉 " GMT" 部分 */
    std::string strCleanedTime = strTimeStr;
    std::string::size_type pos = strCleanedTime.find(" GMT");
    if (pos != std::string::npos)
    {
        strCleanedTime = strCleanedTime.substr(0, pos);
    }

    /* 去除所有空格 */
    strCleanedTime.erase(remove_if(strCleanedTime.begin(), strCleanedTime.end(), ::isspace), strCleanedTime.end());

    std::tm tm = {};
    std::istringstream ss(strCleanedTime);

    /* 提取时间的各部分 */
    std::string strMonth = strCleanedTime.substr(0, 3);    // 例如 "May"
    int day = std::stoi(strCleanedTime.substr(3, 2));      // 日：需要修正为2位
    std::string strTimePart = strCleanedTime.substr(5, 8); // 取8个字符，确保获取完整时间部分
    int year = std::stoi(strCleanedTime.substr(13, 4));    // 年：修正位置

    /* 将年份转换为4位（如果是两位年份） */
    if (year < 100)
    {
        year += 2000;
    }

    /* 设置月份 */
    std::string strMonths[] = MONTHS_ARRAY;
    auto it = std::find(std::begin(strMonths), std::end(strMonths), strMonth);
    if (it != std::end(strMonths))
    {
        tm.tm_mon = std::distance(std::begin(strMonths), it); // 月份从0开始
    }
    else
    {
        dlog_error("无效的月份：%s", strMonth.c_str());
        return "";
    }

    tm.tm_mday = day;
    tm.tm_year = year - 1900; // 年份从1900开始

    /* 检查 timePart 的长度 */
    if (strTimePart.length() < 8)
    {
        dlog_error("无效的时间格式%s", strTimePart.c_str());
        return "";
    }

    tm.tm_hour = std::stoi(strTimePart.substr(0, 2));
    tm.tm_min = std::stoi(strTimePart.substr(3, 2));
    tm.tm_sec = std::stoi(strTimePart.substr(6, 2));
    tm.tm_isdst = 0; // 不考虑夏令时

    /* 将 tm 转换为 time_t (GMT 时间) */
    std::time_t gmtTime = std::mktime(&tm);
    if (gmtTime == -1)
    {
        dlog_error("转换 tm 结构体为 time_t 失败");
        return "";
    }

    /* 调整为北京时间 (GMT+8) */
    std::time_t beijingTime = gmtTime + 8 * 3600;

    /* 格式化为 "%Y-%m-%d %H:%M:%S" 字符串 */
    std::tm *pLocalTm = std::localtime(&beijingTime);
    if (!pLocalTm)
    {
        dlog_error("转换 time_t 为 tm 结构体失败");
        return "";
    }

    std::ostringstream oss;
    oss << std::put_time(pLocalTm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string CCaManage::getCertificateSerialNumber(const std::string &strCertPath)
{
    /* 打开证书文件 */
    std::unique_ptr<FILE, decltype(&fclose)> certFile(fopen(strCertPath.c_str(), "r"), fclose);
    if (!certFile)
    {
        dlog_error("无法打开证书文件%s", strCertPath.c_str());
        return "";
    }

    /* 打开证书文件 */
    std::unique_ptr<X509, decltype(&X509_free)> cert(PEM_read_X509(certFile.get(), nullptr, nullptr, nullptr), X509_free);
    if (!cert)
    {
        dlog_error("无法读取证书");
        return "";
    }

    /* 获取证书的序列号 */
    ASN1_INTEGER *pSerialNumber = X509_get_serialNumber(cert.get());
    if (!pSerialNumber)
    {
        dlog_error("无法获取证书序列号");
        return "";
    }

    /* 将序列号转换为 BIGNUM 以便转换为字符串 */
    std::unique_ptr<BIGNUM, decltype(&BN_free)> bn(ASN1_INTEGER_to_BN(pSerialNumber, nullptr), BN_free);
    if (!bn)
    {
        dlog_error("无法转换序列号");
        return "";
    }

    /*  将 BIGNUM 转换为十六进制字符串 */
    char *pChHexSerial = BN_bn2hex(bn.get());
    if (!pChHexSerial)
    {
        dlog_error("无法将BIGNUM转换为十六进制字符串");
        return "";
    }

    std::string strserial(pChHexSerial);
    OPENSSL_free(pChHexSerial);

    return strserial;
}

std::string CCaManage::getFileExtension(const std::string &strFilename)
{
    size_t pos = strFilename.rfind('.');
    if (pos == std::string::npos)
    {
        return "";
    }
    return strFilename.substr(pos + 1);
}

bool CCaManage::isCA(X509 *pCert)
{
    int nRet = X509_check_ca(pCert);
    return nRet == 1;
}

bool CCaManage::isCertificateCA(const std::string &strCertPath)
{
    /* 读取证书文件 */
    std::ifstream certFile(strCertPath, std::ios::binary);
    if (!certFile.is_open())
    {
        dlog_error("打开证书文件失败");
        return false;
    }

    std::string strCertData((std::istreambuf_iterator<char>(certFile)), std::istreambuf_iterator<char>());
    certFile.close();

    /* 将证书数据转换为X509结构 */
    X509 *pCert = nullptr;
    BIO *bio = BIO_new_mem_buf(const_cast<char *>(strCertData.c_str()), -1);
    if (!bio)
    {
        dlog_error("BIO_new_mem_buf 失败");
        return false;
    }

    pCert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    if (!pCert)
    {
        dlog_error("从文件读取证书失败。");
        BIO_free(bio);
        return false;
    }

    BIO_free(bio);

    /* 检查证书是否在有效期内 */
    if (X509_cmp_current_time(X509_get_notBefore(pCert)) >= 0 || X509_cmp_current_time(X509_get_notAfter(pCert)) <= 0)
    {
        dlog_error("证书无效");
        X509_free(pCert);
        return false;
    }

    /* 检查证书的用途是否为CA用途 */
    if (!isCA(pCert))
    {
        dlog_error("不是CA证书。");
        X509_free(pCert);
        return false;
    }

    X509_free(pCert);

    return true;
}

std::string CCaManage::getIssuerCN(const std::string &strCertFilePath)
{
    /* 打开证书文件 */
    FILE *certFile = fopen(strCertFilePath.c_str(), "r");
    if (!certFile)
    {
        dlog_error("打开证书文件失败：%s", strCertFilePath.c_str());
        return "";
    }

    /* 读取证书 */
    X509 *pCert = PEM_read_X509(certFile, nullptr, nullptr, nullptr);
    fclose(certFile);

    if (!pCert)
    {
        dlog_error("从文件读取证书失败:%s", strCertFilePath.c_str());
        return "";
    }

    /* 获取颁发者的X509_NAME对象 */
    X509_NAME *pIssuerName = X509_get_issuer_name(pCert);
    if (!pIssuerName)
    {
        dlog_error("从证书中获取颁发者名称失败");
        X509_free(pCert);
        return "";
    }

    /* 提取颁发者名称中的 CN 字段 */
    char chCnBuffer[DATA_LEN];
    int nCnLength = X509_NAME_get_text_by_NID(pIssuerName, NID_commonName, chCnBuffer, sizeof(chCnBuffer));

    if (nCnLength == -1)
    {
        dlog_error("从发行者名称获取CN失败");
        X509_free(pCert);
        return "";
    }

    /* 释放证书对象 */
    X509_free(pCert);

    return std::string(chCnBuffer);
}

std::string CCaManage::getSubjectCN(const std::string &strCertFilePath)
{
    /* 打开证书文件 */
    FILE *pCertFile = fopen(strCertFilePath.c_str(), "r");
    if (!pCertFile)
    {
        dlog_error("打开证书文件失败:%s", strCertFilePath.c_str());
        return "";
    }

    /* 读取证书 */
    X509 *pCert = PEM_read_X509(pCertFile, nullptr, nullptr, nullptr);
    fclose(pCertFile);

    if (!pCert)
    {
        dlog_error("从文件读取证书失败:%s", strCertFilePath.c_str());
        return "";
    }

    /* 获取使用者的X509_NAME对象 */
    X509_NAME *pSubjectName = X509_get_subject_name(pCert);
    if (!pSubjectName)
    {
        dlog_error("从证书中获取主题名称失败");
        X509_free(pCert);
        return "";
    }

    /* 提取使用者名称中的 CN 字段 */
    char chCnBuffer[DATA_LEN];
    int nCnLength = X509_NAME_get_text_by_NID(pSubjectName, NID_commonName, chCnBuffer, sizeof(chCnBuffer));

    if (nCnLength == -1)
    {
        dlog_error("从主题名称获取CN失败");
        X509_free(pCert);
        return "";
    }

    /* 释放证书对象 */
    X509_free(pCert);

    return std::string(chCnBuffer); // 返回 CN 字段的字符串
}

int CCaManage::compareCertKey(const std::string &strCertPath, const std::string &strKeyPath)
{

    /* 打开证书文件 */
    FILE *pCertFile = fopen(strCertPath.c_str(), "r");
    if (!pCertFile)
    {
        dlog_error("无法打开证书文件:%s", strCertPath.c_str());
        return -1;
    }

    /* 读取证书 */
    X509 *pCert = PEM_read_X509(pCertFile, nullptr, nullptr, nullptr);
    fclose(pCertFile);
    if (!pCert)
    {
        dlog_error("无法加载证书:%s", strCertPath.c_str());
        return -1;
    }

    /* 获取证书中的公钥 */
    EVP_PKEY *pCertPubKey = X509_get_pubkey(pCert);
    if (!pCertPubKey)
    {
        dlog_error("无法提取证书中的公钥");
        X509_free(pCert);
        return -1;
    }

    /* 打开私钥文件 */
    FILE *pKeyFile = fopen(strKeyPath.c_str(), "r");
    if (!pKeyFile)
    {
        dlog_error("无法打开私钥文件:%s", strKeyPath.c_str());
        EVP_PKEY_free(pCertPubKey);
        X509_free(pCert);
        return -1;
    }

    /* 读取私钥 */
    EVP_PKEY *pPrivKey = PEM_read_PrivateKey(pKeyFile, nullptr, nullptr, nullptr);
    fclose(pKeyFile);
    if (!pPrivKey)
    {
        dlog_error("无法加载私钥:%s", strKeyPath.c_str());
        EVP_PKEY_free(pCertPubKey);
        X509_free(pCert);
        return -1;
    }

    /* 比较证书公钥和私钥 */
    int nResult = EVP_PKEY_cmp(pCertPubKey, pPrivKey);

    /* 清理资源 */
    EVP_PKEY_free(pCertPubKey);
    EVP_PKEY_free(pPrivKey);
    X509_free(pCert);

    /* 检查结果 */
    if (nResult == 1)
    {
        return 0;
    }
    else
    {
        dlog_error("证书和私钥不匹配。");
        return -1;
    }
}

int CCaManage::getCertificateInfo(const std::string &strCertFilePath, Network::CertFileInfo_S &stCertInfo)
{

    /* 获取到期日期 */
    stCertInfo.strExpiraDate = getCertificateExpirationDate(strCertFilePath);
    if (stCertInfo.strExpiraDate.empty())
    {
        dlog_error("获取到期日期失败");
        return -1;
    }

    /* 获取证书序列号 */
    stCertInfo.strSerialNum = getCertificateSerialNumber(strCertFilePath);
    if (stCertInfo.strSerialNum.empty())
    {
        dlog_error("获取证书序列号失败");
        return -1;
    }

    /* 获取颁发者名称 */
    stCertInfo.strLicensor = getIssuerCN(strCertFilePath);
    if (stCertInfo.strLicensor.empty())
    {
        dlog_error("获取颁发者名称失败");
        return -1;
    }

    /* 获取使用者名称 */
    stCertInfo.strUser = getSubjectCN(strCertFilePath);
    if (stCertInfo.strUser.empty())
    {
        dlog_error("获取使用者名称失败");
        return -1;
    }

    return 0;
}

bool CCaManage::isCertificateDevice(const std::string& strCertPath) 
{
    FILE* fp = fopen(strCertPath.c_str(), "rb");
    if (!fp)
    {
        dlog_error("打开证书文件失败");
        return false;
    } 

    X509* cert = PEM_read_X509(fp, nullptr, nullptr, nullptr);
    fclose(fp);
    
    if (cert) 
    {
        dlog_info("设备证书验证成功");
        X509_free(cert);
        return true;
    }
    dlog_error("设备证书验证失败");
    return false;
}

int CCaManage::CheckCertExpired(const std::string& strCertPath)
{
    BIO* bio = nullptr;
    X509* cert = nullptr;
    int result = -1;  // 默认返回 -1（过期或错误）

    // 1. 打开证书文件
    bio = BIO_new_file(strCertPath.c_str(), "r");
    if (!bio) {
        return -1;  // 文件打开失败
    }

    // 2. 解析证书
    cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    if (cert) {  // 仅当证书解析成功时处理有效期
        const ASN1_TIME* not_before = X509_get0_notBefore(cert);
        const ASN1_TIME* not_after = X509_get0_notAfter(cert);

        // 3. 转换为 tm 结构（UTC 时间）
        struct tm tm_not_before = {}, tm_not_after = {};
        if (ASN1_TIME_to_tm(not_before, &tm_not_before) &&
            ASN1_TIME_to_tm(not_after, &tm_not_after)) 
            {

            // 4. 获取当前 UTC 时间
            time_t t_now = time(nullptr);
            struct tm tm_current;
            #if defined(_WIN32)
            gmtime_s(&tm_current, &t_now);
            #else
            gmtime_r(&t_now, &tm_current);
            #endif

            // 5. 转换为 time_t 进行比较
            time_t t_not_before = timegm(&tm_not_before);
            time_t t_not_after = timegm(&tm_not_after);
            time_t t_current = timegm(&tm_current);

            // 6. 判断是否在有效期内
            if (t_current >= t_not_before && t_current <= t_not_after) 
            {
                dlog_info("证书在有效期");
                result = 0;  // 有效期内
            }
        }
    }

    // 7. 统一释放资源（即使指针为 NULL 也安全）
    X509_free(cert);
    BIO_free_all(bio);
    return result;
}

int CCaManage::dealUploadDeviceCert()
 {
    const fs::path upload_dir = UPLOAD_PATH;
    const fs::path cert_dir = CA_REQ_PATH;
    int nRet;

    /* 清空上传目录文件 */ 
    //fs::remove_all(upload_dir);

    /*  查找第一个有效证书文件 */
    for (const auto& entry : fs::directory_iterator(upload_dir)) 
    {
        if (entry.is_regular_file()) 
        {
            
            //dlog_info("证书后缀：%s",entry.path().extension().string().c_str());
            if (entry.path().extension() == ".cer" || entry.path().extension() == ".pem" || entry.path().extension() == ".crt" || entry.path().extension() == ".key" || entry.path().extension() == ".der") 
            {
                const auto& src_path = entry.path();

                if (isCertificateDevice(src_path)) 
                {
                    dlog_info("找到设备ca证书:%s",src_path.c_str());

                    if(CheckCertExpired(src_path) != 0)
                    {
                        dlog_error("证书:%s 已过期或无效",src_path.c_str());
                        fs::remove(src_path);
                       return IpcRet_E::ERR_CERT_EXPIRE;
                    }

                    /* 请求密钥和上传的请求证书是否匹配 */
                    if(compareCertKey(src_path, CA_REQ_KEY) != 0)
                    {
                        dlog_error("上传的证书：%s 和请求密钥不匹配 删除证书",src_path.c_str());
                        fs::remove(src_path);
                        return IpcRet_E::ERR_CERT_MATCH_KEY;
                    }

                    std::string new_filename = src_path.filename().string();
                    fs::path dest_path = cert_dir / new_filename;
                    if (!fs::exists(dest_path))
                    {
                        /* 重命名 */
                        fs::rename(src_path, dest_path);
                        Network::CertFileInfo_S stCertFileInfo;
                        stCertFileInfo.strPath = dest_path;
                        /* 安装设备证书 */
                        nRet = installDeviceCert(stCertFileInfo);
                        if(nRet < 0)
                        {
                            /* 删除文件 */
                            dlog_error("安装证书失败,删除文件:%s",dest_path.c_str());
                            fs::remove(dest_path);
                        }
                        return nRet;
                    }
                    else
                    {
                        dlog_error("安装证书失败,证书已存在！删除文件:%s",src_path.c_str());
                        fs::remove(src_path);
                        return IpcRet_E::ERR_CERT_EXIST;
                    }
                    
                }
                else
                {
                    dlog_error("证书格式不正确,删除文件:%s",entry.path().c_str());
                    /* 删除证书 */
                    fs::remove(entry.path());
                    return IpcRet_E::ERR_CERT_FORMAT;
                }
            }
           
        }
        
    }
    return -1;
 }

int CCaManage::dealUploadCaCert()
 {
    const fs::path upload_dir = UPLOAD_PATH;
    const fs::path cert_dir = CA_UPLOAD_TRUST_PATH;
    int nRet;

    /* 清空上传目录文件 */ 
    //fs::remove_all(upload_dir);

    /*  查找第一个有效证书文件 */
    for (const auto& entry : fs::directory_iterator(upload_dir)) 
    {
        if (entry.is_regular_file()) 
        {
            
            //dlog_info("证书后缀：%s",entry.path().extension().string().c_str());
            if (entry.path().extension() == ".cer" || entry.path().extension() == ".pem" || entry.path().extension() == ".crt" || entry.path().extension() == ".key" || entry.path().extension() == ".der") 
            {
                const auto& src_path = entry.path();

                if (isCertificateCA(src_path)) 
                {
                    dlog_info("找到受信任ca证书:%s",src_path.c_str());

                    if(CheckCertExpired(src_path) != 0)
                    {
                        dlog_error("证书:%s 已过期或无效",src_path.c_str());
                        fs::remove(src_path);
                       return IpcRet_E::ERR_CERT_EXPIRE;
                    }

                    std::string new_filename = src_path.filename().string();
                    fs::path dest_path = cert_dir / new_filename;
                    if (!fs::exists(dest_path))
                    {
                        /* 重命名 */
                        fs::rename(src_path, dest_path);
                        Network::CertFileInfo_S stCertFileInfo;
                        stCertFileInfo.strPath = dest_path;
                        /* 安装受信任证书 */
                        nRet = installTrustCert(stCertFileInfo);
                        if(nRet < 0)
                        {
                            /* 删除文件 */
                            dlog_error("安装证书失败,删除文件:%s",dest_path.c_str());
                            fs::remove(dest_path);
                        }
                        //dlog_info("安装CA证书：%s 成功",dest_path.c_str());
                        return nRet;
                    }
                    else
                    {
                        dlog_error("安装证书失败,证书已存在!删除文件:%s",src_path.c_str()); 
                        fs::remove(src_path);
                        return IpcRet_E::ERR_CERT_EXIST;
                    }
                    
                }
                else
                {
                    dlog_error("证书格式不正确,删除文件:%s",entry.path().c_str());
                    /* 删除证书 */
                    fs::remove(entry.path());
                    return IpcRet_E::ERR_CERT_FORMAT;
                }
            }
           
        }
        
    }
    return -1;
 }