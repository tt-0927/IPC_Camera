/*
 * @FilePath: CAManager.cpp
 * @Author: tianl
 * @Date: 2024-09-24 19:36:05
 * @LastEditors: tianl
 * @LastEditTime: 2024-09-26 11:42:16
 * @Description: CA证书相关操作
 */

#include "CAManager.hpp"

CAManager::CAManager()
{
    /* openssl库初始化 */
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
}

CAManager::~CAManager()
{
    EVP_cleanup();
    ERR_free_strings();
}

EVP_PKEY *CAManager::generateKey(int nBits)
{
    /* 为EVP PKEY结构分配内存 */
    EVP_PKEY *pkey = EVP_PKEY_new();
    if (!pkey)
    {
        std::cerr << "Unable to create EVP_PKEY structure." << std::endl;
        return NULL;
    }

    /* 生成RSA密钥并分配给pkey */
    RSA *prsa = RSA_generate_key(nBits, RSA_F4, NULL, NULL);
    if (!EVP_PKEY_assign_RSA(pkey, prsa))
    {
        std::cerr << "Unable to generate 2048-bit RSA key." << std::endl;
        EVP_PKEY_free(pkey);
        return NULL;
    }

    return pkey;
}

int CAManager::generateCsr(const CertAPPlyInfo_S &stApplyInfo)
{
    std::string strKeyFile = CA_REQ_KEY;
    std::string strCsrFile = CA_REQ_CSR;

    /* 读取私钥文件 */
    FILE *pKeyFile = fopen(strKeyFile.c_str(), "r");
    if (!pKeyFile)
    {
        std::cerr << "无法打开私钥文件: " << strKeyFile << std::endl;
        return -1;
    }

    /* 从文件加载私钥 */
    EVP_PKEY *pKey = PEM_read_PrivateKey(pKeyFile, NULL, NULL, NULL);
    fclose(pKeyFile);

    /* 创建一个新的证书请求对象 */
    X509_REQ *pReq = X509_REQ_new();
    if (!pReq)
    {
        std::cerr << "Failed to create X509_REQ object" << std::endl;
        return -1;
    }

    /* 创建一个新的 X509_NAME 对象 */
    X509_NAME *pName = X509_NAME_new();

    if (!stApplyInfo.strC.empty())
    {
        if (X509_NAME_add_entry_by_txt(pName, "C", MBSTRING_ASC,
                                       reinterpret_cast<const unsigned char *>(stApplyInfo.strC.c_str()), -1, -1, 0) != 1)
        {
            std::cerr << "Failed to add country name to X509_NAME" << std::endl;
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
            std::cerr << "Failed to add state name to X509_NAME" << std::endl;
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
            std::cerr << "Failed to add locality name to X509_NAME" << std::endl;
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
            std::cerr << "Failed to add organization name to X509_NAME" << std::endl;
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
            std::cerr << "Failed to add organizational unit name to X509_NAME" << std::endl;
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
            std::cerr << "Failed to add common name to X509_NAME" << std::endl;
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
            std::cerr << "Failed to add email address to X509_NAME" << std::endl;
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
        std::cerr << "Failed to set public key for X509_REQ" << std::endl;
        X509_REQ_free(pReq);
        return -1;
    }

    /* 签署证书请求 */
    if (X509_REQ_sign(pReq, pKey, EVP_sha256()) <= 0)
    {
        std::cerr << "Failed to sign certificate request" << std::endl;
        X509_REQ_free(pReq);
        X509_NAME_free(pName);
        return -1;
    }

    /* 将证书请求写入文件 */
    FILE *pCsrFile = fopen(strCsrFile.c_str(), "wb");
    if (!pCsrFile)
    {
        std::cerr << "Failed to open file: " << strCsrFile << std::endl;
        X509_REQ_free(pReq);
        X509_NAME_free(pName);
        return -1;
    }

    if (PEM_write_X509_REQ(pCsrFile, pReq) != 1)
    {
        std::cerr << "Failed to write CSR to file" << std::endl;
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

int CAManager::generateCertificate(const CertAPPlyInfo_S &stApplyInfo, const std::string &strCertFile, int nValday)
{
    /* 读取中间证书 */
    FILE *pCertFile = fopen(CA_MIDDLE_CERT, "r");
    if (!pCertFile)
    {
        std::cerr << "打开中间证书文件失败" << std::endl;
        std::cerr << "中间证书路径：" << CA_MIDDLE_CERT << std::endl;
        return -1;
    }
    X509 *pInterCert = PEM_read_X509(pCertFile, nullptr, nullptr, nullptr);
    fclose(pCertFile);
    if (!pInterCert)
    {
        std::cerr << "Failed to read intermediate certificate" << std::endl;
        return -1;
    }

    /* 读取中间证书私钥 */
    FILE *pInterKey_File = fopen(CA_MIDDLE_KEY, "r");
    if (!pInterKey_File)
    {
        std::cerr << "Failed to open intermediate key file" << std::endl;
        X509_free(pInterCert);
        return -1;
    }
    EVP_PKEY *pInterKey = PEM_read_PrivateKey(pInterKey_File, nullptr, nullptr, nullptr);
    fclose(pInterKey_File);
    if (!pInterKey)
    {
        std::cerr << "Failed to read intermediate key" << std::endl;
        X509_free(pInterCert);
        return -1;
    }

    /* 创建一个新的证书请求 */
    X509_REQ *preq = X509_REQ_new();
    EVP_PKEY *pkey = EVP_PKEY_new();

    /* 生成RSA密钥，分配给证书请求的公钥 */
    RSA *prsa = RSA_generate_key(KEY_LENGTH_2048, RSA_F4, NULL, NULL);
    EVP_PKEY_assign_RSA(pkey, prsa);

    /* 设置证书请求的公钥 */
    X509_REQ_set_pubkey(preq, pkey);

    /* 设置证书的主题名称 */
    X509_NAME *pName = X509_NAME_new();
    if (!pName)
    {
        std::cerr << "Failed to create X509_NAME object" << std::endl;
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
            std::cerr << "Failed to add country name (C) to X509_NAME" << std::endl;
            X509_NAME_free(pName);
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
            std::cerr << "Failed to add state name (ST) to X509_NAME" << std::endl;
            X509_NAME_free(pName);
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
            std::cerr << "Failed to add locality name (L) to X509_NAME" << std::endl;
            X509_NAME_free(pName);
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
            std::cerr << "Failed to add organization name (O) to X509_NAME" << std::endl;
            X509_NAME_free(pName);
            EVP_PKEY_free(pInterKey);
            X509_free(pInterCert);
            return -1;
        }
    }

    if (!stApplyInfo.strOU.empty())
    {
        if (X509_NAME_add_entry_by_txt(pName, "OU", MBSTRING_ASC,
                                       reinterpret_cast<const unsigned char *>(stApplyInfo.strOU.c_str()), -1, -1, 0) != 1)
        {
            std::cerr << "Failed to add organizational unit name (OU) to X509_NAME" << std::endl;
            X509_NAME_free(pName);
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
            std::cerr << "Failed to add common name (CN) to X509_NAME" << std::endl;
            X509_NAME_free(pName);
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
            std::cerr << "Failed to add email address to X509_NAME" << std::endl;
            X509_NAME_free(pName);
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
        std::cerr << "Failed to create X509 object" << std::endl;
        EVP_PKEY_free(pInterKey);
        X509_free(pInterCert);
        return -1;
    }

    /* 设置证书的版本 */
    if (X509_set_version(pCert, CERTIFICATE_VERSION) != 1)
    {
        std::cerr << "Failed to set certificate version" << std::endl;
        X509_free(pCert);
        EVP_PKEY_free(pInterKey);
        X509_free(pInterCert);
        return -1;
    }

    /* 生成随机序列号 */
    BIGNUM *pBn_serial = BN_new();
    if (!pBn_serial || !BN_rand(pBn_serial, SERIAL_NUMBER_BITS, BN_RAND_TOP_ONE, BN_RAND_BOTTOM_ANY))
    {
        std::cerr << "无法生成随机序列号" << std::endl;
        BN_free(pBn_serial);
        X509_free(pCert);
        return -1;
    }

    ASN1_INTEGER *pAsn1_serial = BN_to_ASN1_INTEGER(pBn_serial, nullptr);
    if (!pAsn1_serial)
    {
        std::cerr << "无法转换序列号" << std::endl;
        BN_free(pBn_serial);
        X509_free(pCert);
        return -1;
    }

    /* 设置证书的序列号 */
    X509_set_serialNumber(pCert, pAsn1_serial);

    BN_free(pBn_serial);
    ASN1_INTEGER_free(pAsn1_serial);

    /* 设置证书的有效期 */
    X509_gmtime_adj(X509_get_notBefore(pCert), 0);
    X509_gmtime_adj(X509_get_notAfter(pCert), (long)DAYS_TO_SECONDS(nValday));

    /* 设置证书的签发者名称，使用中间证书的主题名称作为签发者 */
    X509_set_issuer_name(pCert, X509_get_subject_name(pInterCert)); // 设置签发者

    /* 设置申请者的主题名称（与证书请求一致） */
    X509_set_subject_name(pCert, X509_REQ_get_subject_name(preq)); // 设置申请者

    /* 设置证书的公钥 */
    X509_set_pubkey(pCert, pkey);

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
        std::cerr << "Failed to generate Subject Key Identifier" << std::endl;
        X509_free(pCert);
        EVP_PKEY_free(pInterKey);
        X509_free(pInterCert);
        return -1;
    }

    /* 添加基本约束 */
    X509_EXTENSION *ext_bc = X509V3_EXT_conf_nid(nullptr, nullptr, NID_basic_constraints, "CA:FALSE");
    X509_add_ext(pCert, ext_bc, -1);
    X509_EXTENSION_free(ext_bc);

    /* 使用中间证书的私钥对证书进行签名，使用 SHA256 算法 */
    if (X509_sign(pCert, pInterKey, EVP_sha256()) <= 0)
    {
        std::cerr << "Failed to sign certificate" << std::endl;
        X509_free(pCert);
        EVP_PKEY_free(pInterKey);
        X509_free(pInterCert);
        return -1;
    }

    /* 将证书写入文件 */
    FILE *pCertOut = fopen(strCertFile.c_str(), "wb");
    if (!pCertOut)
    {
        std::cerr << "Failed to open file for writing certificate" << std::endl;
        X509_free(pCert);
        EVP_PKEY_free(pInterKey);
        X509_free(pInterCert);
        return -1;
    }
    if (PEM_write_X509(pCertOut, pCert) != 1)
    {
        std::cerr << "Failed to write certificate to file" << std::endl;
        fclose(pCertOut);
        X509_free(pCert);
        EVP_PKEY_free(pInterKey);
        X509_free(pInterCert);
        return -1;
    }
    fclose(pCertOut);

    X509_free(pCert);
    EVP_PKEY_free(pInterKey);
    X509_free(pInterCert);

    return 0;
}

std::string CAManager::getCertificateExpirationDate(const std::string &strCertPath)
{
    X509 *pCert = nullptr;
    ASN1_TIME *pNotAfter = nullptr;
    std::string strExpirationDate;
    time_t expirationTime;

    /* 打开证书文件 */
    FILE *fp = fopen(strCertPath.c_str(), "r");
    if (fp == nullptr)
    {
        std::cerr << "Error opening certificate file: " << strCertPath << std::endl;
        return "";
    }

    /* 读取证书 */
    pCert = PEM_read_X509(fp, nullptr, nullptr, nullptr);
    fclose(fp);
    if (pCert == nullptr)
    {
        std::cerr << "Error reading certificate from file: " << strCertPath << std::endl;
        return "";
    }

    /* 获取证书的到期时间 */
    pNotAfter = X509_get_notAfter(pCert);
    if (pNotAfter == nullptr)
    {
        std::cerr << "Error getting expiration date from certificate." << std::endl;
        X509_free(pCert);
        return "";
    }

    /* 把获取到的ASN1_TIME转换为ISO-8601时间戳 */
    std::string strTmpDate = convertASN1Time(pNotAfter);
    /* 转换为YYYY-MM-DD HH:MM:SS格式的北京时间 */
    strExpirationDate = convertGMTToBeijingTime(strTmpDate);
    /* 释放资源 */
    X509_free(pCert);

    return strExpirationDate;
}

std::string CAManager::convertASN1Time(const ASN1_TIME *pTime)
{
    /* 使用RAII创建一个BIO对象以进行自动清理 */
    std::unique_ptr<BIO, decltype(&BIO_free)> bio(BIO_new(BIO_s_mem()), BIO_free);
    if (!bio)
    {
        std::cerr << "Failed to create BIO object." << std::endl;
        return "";
    }

    /* 打印ASN1时间到BIO */
    if (ASN1_TIME_print(bio.get(), pTime) <= 0)
    {
        std::cerr << "ASN1_TIME_print failed or wrote no data." << std::endl;
        return "";
    }

    /* 将BIO中的数据读入字符串 */
    char chTempBuffer[DATA_LEN];
    int nBytesRead = BIO_gets(bio.get(), chTempBuffer, sizeof(chTempBuffer));
    if (nBytesRead <= 0)
    {
        std::cerr << "BIO_gets failed to transfer contents." << std::endl;
        return "";
    }

    return std::string(chTempBuffer, nBytesRead);
}

std::string CAManager::convertGMTToBeijingTime(const std::string &strTimeStr)
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

    /* 设置月份 */
    std::string strMonths[] = MONTHS_ARRAY;
    auto it = std::find(std::begin(strMonths), std::end(strMonths), strMonth);
    if (it != std::end(strMonths))
    {
        tm.tm_mon = std::distance(std::begin(strMonths), it); // 月份从0开始
    }
    else
    {
        std::cerr << "无效的月份: " << strMonth << std::endl;
        return "";
    }

    tm.tm_mday = day;
    tm.tm_year = year - 1900; // 年份从1900开始

    /* 检查 timePart 的长度 */
    if (strTimePart.length() < 8)
    {
        std::cerr << "无效的时间格式: " << strTimePart << std::endl;
        return "";
    }

    tm.tm_hour = std::stoi(strTimePart.substr(0, 2));
    tm.tm_min = std::stoi(strTimePart.substr(3, 2));
    tm.tm_sec = std::stoi(strTimePart.substr(6, 2));
    tm.tm_isdst = -1; // 让系统自动判断是否为夏令时

    // 打印 tm 结构体的内容
    std::cout << "tm.tm_year: " << tm.tm_year << " (years since 1900)" << std::endl;
    std::cout << "tm.tm_mon: " << tm.tm_mon << " (0-11 for Jan-Dec)" << std::endl;
    std::cout << "tm.tm_mday: " << tm.tm_mday << " (1-31)" << std::endl;
    std::cout << "tm.tm_hour: " << tm.tm_hour << " (0-23)" << std::endl;
    std::cout << "tm.tm_min: " << tm.tm_min << " (0-59)" << std::endl;
    std::cout << "tm.tm_sec: " << tm.tm_sec << " (0-59)" << std::endl;
    std::cout << "tm.tm_isdst: " << tm.tm_isdst << " (-1 if unknown)" << std::endl;

    /* 将 tm 转换为 time_t (GMT 时间) */
    std::time_t gmtTime = std::mktime(&tm);
    if (gmtTime == -1)
    {
        std::cerr << "转换 tm 结构体为 time_t 失败。" << std::endl;
        return "";
    }

    /* 调整为北京时间 (GMT+8) */
    std::time_t beijingTime = gmtTime + 8 * 3600;

    /* 格式化为 "%Y-%m-%d %H:%M:%S" 字符串 */
    std::tm *pLocalTm = std::localtime(&beijingTime);
    if (!pLocalTm)
    {
        std::cerr << "转换 time_t 为 tm 结构体失败。" << std::endl;
        return "";
    }

    std::ostringstream oss;
    oss << std::put_time(pLocalTm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string CAManager::getCertificateSerialNumber(const std::string &strCertPath)
{
    /* 打开证书文件 */
    std::unique_ptr<FILE, decltype(&fclose)> certFile(fopen(strCertPath.c_str(), "r"), fclose);
    if (!certFile)
    {
        std::cerr << "Unable to open certificate file: " << strCertPath << std::endl;
        return "";
    }

    /* 打开证书文件 */
    std::unique_ptr<X509, decltype(&X509_free)> cert(PEM_read_X509(certFile.get(), nullptr, nullptr, nullptr), X509_free);
    if (!cert)
    {
        std::cerr << "Unable to read certificate" << std::endl;
        return "";
    }

    /* 获取证书的序列号 */
    ASN1_INTEGER *pSerialNumber = X509_get_serialNumber(cert.get());
    if (!pSerialNumber)
    {
        std::cerr << "Unable to get certificate serial number" << std::endl;
        return "";
    }

    /* 将序列号转换为 BIGNUM 以便转换为字符串 */
    std::unique_ptr<BIGNUM, decltype(&BN_free)> bn(ASN1_INTEGER_to_BN(pSerialNumber, nullptr), BN_free);
    if (!bn)
    {
        std::cerr << "Unable to convert serial number" << std::endl;
        return "";
    }

    /*  将 BIGNUM 转换为十六进制字符串 */
    char *pChHexSerial = BN_bn2hex(bn.get());
    if (!pChHexSerial)
    {
        std::cerr << "Unable to convert BIGNUM to hex string" << std::endl;
        return "";
    }

    std::string strserial(pChHexSerial);
    OPENSSL_free(pChHexSerial);

    return strserial;
}

std::string CAManager::getFileExtension(const std::string &strFilename)
{
    size_t pos = strFilename.rfind('.');
    if (pos == std::string::npos)
    {
        return "";
    }
    return strFilename.substr(pos + 1);
}

bool CAManager::isCA(X509 *pCert)
{
    int nRet = X509_check_ca(pCert);
    return nRet == 1;
}

bool CAManager::isCertificateCA(const std::string &strCertPath)
{
    /* 检查文件后缀 */
    std::string strFileExtension = getFileExtension(strCertPath);
    if (strFileExtension != "cer" && strFileExtension != "pem" && strFileExtension != "crt")
    {
        std::cerr << "Certificate file extension is not supported." << std::endl;
        return false;
    }

    /* 读取证书文件 */
    std::ifstream certFile(strCertPath, std::ios::binary);
    if (!certFile.is_open())
    {
        std::cerr << "Failed to open certificate file." << std::endl;
        return false;
    }

    std::string strCertData((std::istreambuf_iterator<char>(certFile)), std::istreambuf_iterator<char>());
    certFile.close();

    /* 将证书数据转换为X509结构 */
    X509 *pCert = nullptr;
    BIO *bio = BIO_new_mem_buf(const_cast<char *>(strCertData.c_str()), -1);
    if (!bio)
    {
        std::cerr << "BIO_new_mem_buf failed." << std::endl;
        return false;
    }

    pCert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    if (!pCert)
    {
        std::cerr << "Failed to read certificate from file." << std::endl;
        BIO_free(bio);
        return false;
    }

    BIO_free(bio);

    /* 检查证书是否在有效期内 */
    if (X509_cmp_current_time(X509_get_notBefore(pCert)) >= 0 || X509_cmp_current_time(X509_get_notAfter(pCert)) <= 0)
    {
        std::cerr << "Certificate is not valid." << std::endl;
        X509_free(pCert);
        return false;
    }

    /* 检查证书的用途是否为CA用途 */
    if (!isCA(pCert))
    {
        std::cerr << "Certificate is not a CA certificate." << std::endl;
        X509_free(pCert);
        return false;
    }

    X509_free(pCert);

    return true;
}

std::string CAManager::getIssuerCN(const std::string &strCertFilePath)
{
    /* 打开证书文件 */
    FILE *certFile = fopen(strCertFilePath.c_str(), "r");
    if (!certFile)
    {
        std::cerr << "Failed to open certificate file: " << strCertFilePath << std::endl;
        return "";
    }

    /* 读取证书 */
    X509 *pCert = PEM_read_X509(certFile, nullptr, nullptr, nullptr);
    fclose(certFile);

    if (!pCert)
    {
        std::cerr << "Failed to read certificate from file: " << strCertFilePath << std::endl;
        return "";
    }

    /* 获取颁发者的X509_NAME对象 */
    X509_NAME *pIssuerName = X509_get_issuer_name(pCert);
    if (!pIssuerName)
    {
        std::cerr << "Failed to get issuer name from certificate" << std::endl;
        X509_free(pCert);
        return "";
    }

    /* 提取颁发者名称中的 CN 字段 */
    char chCnBuffer[DATA_LEN];
    int nCnLength = X509_NAME_get_text_by_NID(pIssuerName, NID_commonName, chCnBuffer, sizeof(chCnBuffer));

    if (nCnLength == -1)
    {
        std::cerr << "Failed to get CN from issuer name" << std::endl;
        X509_free(pCert);
        return "";
    }

    /* 释放证书对象 */
    X509_free(pCert);

    return std::string(chCnBuffer);
}

std::string CAManager::getSubjectCN(const std::string &strCertFilePath)
{
    /* 打开证书文件 */
    FILE *pCertFile = fopen(strCertFilePath.c_str(), "r");
    if (!pCertFile)
    {
        std::cerr << "Failed to open certificate file: " << strCertFilePath << std::endl;
        return "";
    }

    /* 读取证书 */
    X509 *pCert = PEM_read_X509(pCertFile, nullptr, nullptr, nullptr);
    fclose(pCertFile);

    if (!pCert)
    {
        std::cerr << "Failed to read certificate from file: " << strCertFilePath << std::endl;
        return "";
    }

    /* 获取使用者的X509_NAME对象 */
    X509_NAME *pSubjectName = X509_get_subject_name(pCert);
    if (!pSubjectName)
    {
        std::cerr << "Failed to get subject name from certificate" << std::endl;
        X509_free(pCert);
        return "";
    }

    /* 提取使用者名称中的 CN 字段 */
    char chCnBuffer[DATA_LEN];
    int nCnLength = X509_NAME_get_text_by_NID(pSubjectName, NID_commonName, chCnBuffer, sizeof(chCnBuffer));

    if (nCnLength == -1)
    {
        std::cerr << "Failed to get CN from subject name" << std::endl;
        X509_free(pCert);
        return "";
    }

    /* 释放证书对象 */
    X509_free(pCert);

    return std::string(chCnBuffer); // 返回 CN 字段的字符串
}

int CAManager::compareCertKey(const std::string &strCertPath, const std::string &strKeyPath)
{

    /* 打开证书文件 */
    FILE *pCertFile = fopen(strCertPath.c_str(), "r");
    if (!pCertFile)
    {
        std::cerr << "无法打开证书文件: " << strCertPath << std::endl;
        return -1;
    }

    /* 读取证书 */
    X509 *pCert = PEM_read_X509(pCertFile, nullptr, nullptr, nullptr);
    fclose(pCertFile);
    if (!pCert)
    {
        std::cerr << "无法加载证书: " << strCertPath << std::endl;
        return -1;
    }

    /* 获取证书中的公钥 */
    EVP_PKEY *pCertPubKey = X509_get_pubkey(pCert);
    if (!pCertPubKey)
    {
        std::cerr << "无法提取证书中的公钥。" << std::endl;
        X509_free(pCert);
        return -1;
    }

    /* 打开私钥文件 */
    FILE *pKeyFile = fopen(strKeyPath.c_str(), "r");
    if (!pKeyFile)
    {
        std::cerr << "无法打开私钥文件: " << strKeyPath << std::endl;
        EVP_PKEY_free(pCertPubKey);
        X509_free(pCert);
        return -1;
    }

    /* 读取私钥 */
    EVP_PKEY *pPrivKey = PEM_read_PrivateKey(pKeyFile, nullptr, nullptr, nullptr);
    fclose(pKeyFile);
    if (!pPrivKey)
    {
        std::cerr << "无法加载私钥: " << strKeyPath << std::endl;
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
        std::cerr << "证书和私钥不匹配。" << std::endl;
        return -1;
    }
}

int CAManager::getCertificateInfo(const std::string &strCertFilePath, CertFileInfo_S &stCertInfo)
{

    /* 获取到期日期 */
    stCertInfo.strExpiraDate = getCertificateExpirationDate(strCertFilePath);
    if (stCertInfo.strExpiraDate.empty())
    {
        std::cerr << "获取到期日期失败" << std::endl;
        return -1;
    }

    /* 获取证书序列号 */
    stCertInfo.strSerialNum = getCertificateSerialNumber(strCertFilePath);
    if (stCertInfo.strSerialNum.empty())
    {
        std::cerr << "获取证书序列号失败" << std::endl;
        return -1;
    }

    /* 获取颁发者名称 */
    stCertInfo.strLicensor = getIssuerCN(strCertFilePath);
    if (stCertInfo.strLicensor.empty())
    {
        std::cerr << "获取颁发者名称失败" << std::endl;
        return -1;
    }

    /* 获取使用者名称 */
    stCertInfo.strUser = getSubjectCN(strCertFilePath);
    if (stCertInfo.strUser.empty())
    {
        std::cerr << "获取使用者名称失败" << std::endl;
        return -1;
    }

    return 0;
}
