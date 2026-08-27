/**
 * @FilePath     : cert_manage.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-04-09 16:26:35
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-22 09:18:46
 * @Description  :
 */

#include "cert_manage.h"
#include "IpcRet.h"
#include "dlog.h"
#include "public_define.h"
#include "gmssl/gmssl.h"
#include "path_define.h"
#include "get_time.h"

/*国密证书序列号长度默认16字节(至少8字节（64位），X.509最大为20字节）推荐16字节（128位）*/
#define SM_CERT_SERIAL_LENGTH_DEFAULT (16)

CCertManage::CCertManage()
{
    memset(&stSM2Key, 0, sizeof(stSM2Key));
}

CCertManage::~CCertManage()
{
}

int CCertManage::generateKeyPair()
{
    /*生成SM2密钥对*/
    // dlog_check_return_print(sm2_key_generate(&stSM2RootKey), TRUE, "生成SM2根证书密钥失败");
    // dlog_check_return_print(sm2_key_generate(&stSM2MiddleKey), TRUE, "生成SM2中间证书密钥失败");
    // dlog_check_return_print(sm2_key_generate(&stSM2SigningKey), TRUE, "生成SM2签名证书密钥失败");
    // dlog_check_return_print(sm2_key_generate(&stSM2EncryptionKey), TRUE, "生成SM2加密证书密钥失败");

    for (int i = CERT_TYPE_SELF_SIGNED; i < CERT_TYPE_MAX; i++)
    {
        dlog_check_return_print(sm2_key_generate(&stSM2Key[i]), TRUE, "生成[CertType_E:%d]的SM2密钥失败", i);
        dlog_check_return_print(saveKeyPairToFile((CertType_E) i, stSM2Key[i]), OK, "保存[CertType_E:%d]SM2私钥至文件失败", i);
    }

    return OK;
}

int CCertManage::readKeyPair_from_file()
{
    /*获取文件所在的文件夹路径*/
    if (access(SM_SIGNING_CERT_KEY_PATH, F_OK) != 0)
    {
        dlog_error("签名证书文件 %s 不存在\n", SM_SIGNING_CERT_KEY_PATH);
        return ERR;
    }

    FILE *fp = nullptr;
    /*打开文件*/
    if (!(fp = fopen(SM_SIGNING_CERT_KEY_PATH, "r+")))
    {
        dlog_error("打开私钥%s文件失败", SM_SIGNING_CERT_KEY_PATH);
        return ERR;
    }
    /*从 PEM 文件读取私钥*/
    if (sm2_private_key_info_decrypt_from_pem(&stSM2Key[CERT_TYPE_SIGNING], SM2_PRIVATE_KEY_ENCRYPT_PASSWORD, fp) != TRUE)
    {
        dlog_error("从PEM文件读取私钥失败");
        if (fp)
        {
            fclose(fp);
            fp = nullptr;
        }
        return ERR;
    }
    if (fp)
    {
        fclose(fp);
        fp = nullptr;
    }

    return OK;
}

int CCertManage::setIssuerSubjectName(const CertIssuerSubjectInfo_S &stInfo, uint8_t *pName, size_t *pNameLen, size_t maxLen)
{
    if (pName == NULL || pNameLen == NULL)
    {
        dlog_error("指针为空");
        return ERR_PTR_NULL;
    }

    *pNameLen = 0;

    /*构建X.509证书 颁发者/主体 名称*/
    /*向名称中添加国家名称属性*/
    dlog_check_return(x509_name_add_country_name(pName, pNameLen, maxLen, stInfo.strCountry.c_str()), TRUE);
    /*向名称中添加州或省名称属性*/
    dlog_check_return(x509_name_add_state_or_province_name(pName,
                                                           pNameLen,
                                                           maxLen,
                                                           ASN1_TAG_PrintableString,
                                                           (uint8_t *) stInfo.strStateOrProvince.c_str(),
                                                           stInfo.strStateOrProvince.size()),
                      TRUE);
    /*向名称中添加地区名称属性*/
    dlog_check_return(x509_name_add_locality_name(pName,
                                                  pNameLen,
                                                  maxLen,
                                                  ASN1_TAG_PrintableString,
                                                  (uint8_t *) stInfo.strLocality.c_str(),
                                                  stInfo.strLocality.size()),
                      TRUE);
    /*向名称中添加组织名称属性*/
    dlog_check_return(x509_name_add_organization_name(pName,
                                                      pNameLen,
                                                      maxLen,
                                                      ASN1_TAG_PrintableString,
                                                      (uint8_t *) stInfo.strOrganization.c_str(),
                                                      stInfo.strOrganization.size()),
                      TRUE);
    /*向名称中添加部门名称属性*/
    dlog_check_return(x509_name_add_organizational_unit_name(pName,
                                                             pNameLen,
                                                             maxLen,
                                                             ASN1_TAG_PrintableString,
                                                             (uint8_t *) stInfo.strOrganizationalUnit.c_str(),
                                                             stInfo.strOrganizationalUnit.size()),
                      TRUE);
    /*向名称中添加通用名称属性*/
    dlog_check_return(x509_name_add_common_name(pName,
                                                pNameLen,
                                                maxLen,
                                                ASN1_TAG_PrintableString,
                                                (uint8_t *) stInfo.strCommonName.c_str(),
                                                stInfo.strCommonName.size()),
                      TRUE);

    return OK;
}

int CCertManage::generateCSR(CertParams_S &stParams, std::string &strCsrDer)
{
    /*存放X.509证书请求缓冲区*/
    uint8_t aCsrBuf[521];
    /*存放X.509证书请求缓冲区长度*/
    size_t szCsrLen = 0;
    uint8_t *p = aCsrBuf;
    /*存放证书主体名称缓冲区*/
    uint8_t aSubjectName[256] = { 0 };
    /*存放证书主体名称缓冲区长度*/
    size_t szSubjectNameLen = 0;
    /*存放证书请求拓展属性缓冲区*/
    uint8_t attrs_buf[512];
    /*存放证书请求拓展属性缓冲区长度*/
    size_t attrs_len = 0;

    /*设置证书主体密钥*/
    // stParams.stCertSubjectInfo.pKey = &stSM2Key[CERT_TYPE_SIGNING];
    stParams.stCertSubjectInfo.pKey = &stSM2Key[stParams.enCertType];
    /*设置证书主体名称*/
    dlog_check_return(setIssuerSubjectName(stParams.stCertSubjectInfo, aSubjectName, &szSubjectNameLen, sizeof(aSubjectName)),
                      OK);

    //! 签名者ID 后续设置为企业标识符，而非默认值。在SM2签名中，iD参与生成Z值（哈希中间值），使签名只能被特定标识符对应的公钥验证
    std::string strSignerId = SM2_DEFAULT_ID;

    dlog_debug("生成证书请求");
    /*生成证书请求*/
    dlog_check_return(x509_req_sign_to_der(stParams.nX509Version,
                                           aSubjectName,
                                           szSubjectNameLen,
                                           stParams.stCertSubjectInfo.pKey,
                                           attrs_buf,
                                           attrs_len,
                                           OID_sm2sign_with_sm3,
                                           stParams.stCertSubjectInfo.pKey,
                                           strSignerId.c_str(),
                                           strSignerId.size(),
                                           &p,
                                           &szCsrLen),
                      TRUE);

    /*打印证书请求信息*/
    dlog_check_return(x509_req_print(stdout, 0, 4, "证书请求", aCsrBuf, szCsrLen), TRUE);
    /*存储证书数据至缓冲区*/
    strCsrDer.assign(reinterpret_cast<char *>(aCsrBuf), szCsrLen);
    /*保存证书至文件*/
    saveCsrToFile(aCsrBuf, szCsrLen);

    return OK;
}

int CCertManage::generateCertificate(CertParams_S &stParams)
{
    /*存放X.509证书缓冲区*/
    uint8_t aCertBuf[4096] = { 0 };
    /*存放X.509证书缓冲区长度*/
    size_t szCertLen = 0;
    /*存放证书颁发者名称缓冲区*/
    uint8_t aIssuerName[512] = { 0 };
    /*存放证书颁发者名称缓冲区长度*/
    size_t szIssuerNameLen = 0;
    /*存放证书主体名称缓冲区*/
    uint8_t aSubjectName[512] = { 0 };
    /*存放证书主体名称缓冲区长度*/
    size_t szSubjectNameLen = 0;
    /*存放证书扩展项缓冲区长度*/
    uint8_t aExts[512] = { 0 };
    /*存放证书扩展项缓冲区长度*/
    size_t szExtsLen = 0;
    /*有效起始日期*/
    time_t timeNotBefore;
    /*有效终止日期*/
    time_t timeNotAfter;

    time(&timeNotBefore);
    /*设置证书有效期*/
    dlog_check_return(x509_validity_add_days(&timeNotAfter, timeNotBefore, stParams.nValidityDays), TRUE);
    /*设置证书颁发者名称*/
    dlog_check_return(setIssuerSubjectName(stParams.stCertIssuerInfo, aIssuerName, &szIssuerNameLen, sizeof(aIssuerName)), OK);
    /*设置证书主体名称*/
    dlog_check_return(setIssuerSubjectName(stParams.stCertSubjectInfo, aSubjectName, &szSubjectNameLen, sizeof(aSubjectName)),
                      OK);
    /*添加扩展项*/
    /*添加基本约束扩展 限制CA证书能签发的下级证书链深度*/
    dlog_check_return(
        x509_exts_add_basic_constraints(aExts, &szExtsLen, sizeof(aExts), X509_critical, stParams.bIsCa, stParams.nPathLen),
        TRUE);
    /*添加密钥用法扩展 允许签发终端证书、允许签发子CRL（可选，根据CA策略）等*/
    dlog_check_return(x509_exts_add_key_usage(aExts, &szExtsLen, sizeof(aExts), X509_critical, stParams.nKeyUsage), TRUE);

    uint8_t *p = aCertBuf;
    /*存放序列号缓冲区*/
    uint8_t aSerial[20] = { 0 };
    /*利用随机数生成序列号*/
    dlog_check_return(CGmSSL::instance()->randomNumber_get(aSerial, SM_CERT_SERIAL_LENGTH_DEFAULT), OK);
    //! 签名者ID 后续设置为企业标识符，而非默认值。在SM2签名中，iD参与生成Z值（哈希中间值），使签名只能被特定标识符对应的公钥验证
    std::string strSignerId = SM2_DEFAULT_ID;
    /*指向存储证书缓冲区指针*/
    std::string *pCertDer;

    switch (stParams.enCertType)
    {
    case CERT_TYPE_SELF_SIGNED:
        stParams.stCertIssuerInfo.pKey = &stSM2Key[CERT_TYPE_SELF_SIGNED];
        stParams.stCertSubjectInfo.pKey = &stSM2Key[CERT_TYPE_SELF_SIGNED];
        pCertDer = &strRootCertDer;
        break;
    case CERT_TYPE_INTERMEDIATE:
        stParams.stCertIssuerInfo.pKey = &stSM2Key[CERT_TYPE_SELF_SIGNED];
        stParams.stCertSubjectInfo.pKey = &stSM2Key[CERT_TYPE_INTERMEDIATE];
        pCertDer = &strMiddleCertDer;
        break;
    case CERT_TYPE_SIGNING:
        stParams.stCertIssuerInfo.pKey = &stSM2Key[CERT_TYPE_INTERMEDIATE];
        stParams.stCertSubjectInfo.pKey = &stSM2Key[CERT_TYPE_SIGNING];
        pCertDer = &strSigningCertDer;
        break;
    case CERT_TYPE_ENCRYPTION:
        stParams.stCertIssuerInfo.pKey = &stSM2Key[CERT_TYPE_INTERMEDIATE];
        stParams.stCertSubjectInfo.pKey = &stSM2Key[CERT_TYPE_ENCRYPTION];
        pCertDer = &strEncryptionCertDer;
        break;
    default:
        dlog_error("未知证书类型");
        return ERR;
    }

    /*将证书信息编码为 DER 格式并签名*/
    dlog_check_return(x509_cert_sign_to_der(stParams.nX509Version,
                                            aSerial,
                                            sizeof(aSerial),      // 随机序列号
                                            OID_sm2sign_with_sm3, // 签名算法标识
                                            aIssuerName,
                                            szIssuerNameLen, // 颁发者名称
                                            timeNotBefore,
                                            timeNotAfter, // 有效起始日期,有效终止日期
                                            aSubjectName,
                                            szSubjectNameLen,                // 主体名称
                                            stParams.stCertSubjectInfo.pKey, // 主体公钥指针
                                            nullptr,
                                            0, // 颁发者唯一ID
                                            nullptr,
                                            0, // 主体唯一ID
                                            aExts,
                                            szExtsLen,                      // 扩展项
                                            stParams.stCertIssuerInfo.pKey, // 签名私钥指针
                                            strSignerId.c_str(),
                                            strSignerId.size(), // 默认SM2 ID定义
                                            &p,
                                            &szCertLen), // 存放X.509证书缓冲区
                      TRUE);

    /*存储证书数据至缓冲区*/
    pCertDer->assign(reinterpret_cast<char *>(aCertBuf), szCertLen);

    /*打印证书信息*/
    // x509_cert_print(stderr, 0, 0, "IntermediateCertificate", aCertBuf, szCertLen);

    /*保存证书至文件*/
    dlog_check_return(saveCertToFile(stParams.enCertType, aCertBuf, szCertLen), OK);

    dlog_trace("证书生成签名成功");
    return OK;
}

int CCertManage::revokeCertificate(const std::string &serial_number, int reason_code, std::string &crl_der)
{
    uint8_t crl_buf[4096];
    size_t crl_len = 0;
    time_t revoke_time;

    // 设置吊销时间
    time(&revoke_time);

    /* gmssl/x509_crl.h: 创建CRL条目 */
    uint8_t *p = crl_buf;
    // if (x509_crl(
    //         reinterpret_cast<const uint8_t *>(serial_number.data()),
    //         serial_number.size(),
    //         revoke_time,
    //         reason_code,
    //         -1,         // 无失效日期
    //         nullptr, 0, // 无证书颁发者
    //         &p, &crl_len) != 1)
    // {
    //     last_error_ = "Add revoked entry failed";
    //     return false;
    // }

    crl_der.assign(reinterpret_cast<char *>(crl_buf), crl_len);
    return OK;
}

int CCertManage::readCertificate_from_file()
{
    /*获取文件所在的文件夹路径*/
    if (access(SM_SIGNING_CERT_PATH, F_OK) != 0)
    {
        dlog_error("签名证书文件 %s 不存在\n", SM_SIGNING_CERT_PATH);
        return ERR;
    }
    /*存放X.509证书缓冲区*/
    uint8_t aCertBuf[4096] = { 0 };
    /*存放X.509证书缓冲区长度*/
    size_t szCertLen = 0;
    FILE *fp = nullptr;
    /*打开文件*/
    if (!(fp = fopen(SM_SIGNING_CERT_PATH, "r+")))
    {
        dlog_error("打开%s文件失败", SM_SIGNING_CERT_PATH);
        return ERR;
    }
    /*从 PEM 文件读取证书*/
    if (x509_cert_from_pem(aCertBuf, &szCertLen, sizeof(aCertBuf), fp) != TRUE)
    {
        dlog_error("从PEM文件读取证书数据至%s失败", SM_SIGNING_CERT_PATH);
        if (fp)
        {
            fclose(fp);
            fp = nullptr;
        }
        return ERR;
    }
    if (fp)
    {
        fclose(fp);
        fp = nullptr;
    }

    /*存储证书数据至缓冲区*/
    strSigningCertDer.assign(reinterpret_cast<char *>(aCertBuf), szCertLen);
    dlog_check_return(x509_cert_print(stderr,
                                      0,
                                      4,
                                      "签名证书",
                                      reinterpret_cast<const uint8_t *>(strSigningCertDer.c_str()),
                                      strSigningCertDer.size()),
                      TRUE);

    return OK;
}

// int CCertManage::generateCertificate_request()
// {

// }

int CCertManage::digitalSignature(const uint8_t *pInputData, size_t szInputLen, uint8_t *pOutData, size_t *pOutLen)
{
    SM2_SIGNATURE stSig;
    uint8_t aHashDigest[32];
    uint8_t sig[SM2_MAX_SIGNATURE_SIZE];
    size_t siglen;
    uint8_t aOutDer[SM2_MAX_SIGNATURE_SIZE];
    uint8_t *pOutDer = aOutDer;
    size_t szOutDerLen = 0;
    SM3_CTX stSM3Ctx;
    sm3_init(&stSM3Ctx);
    sm3_update(&stSM3Ctx, pInputData, szInputLen);
    sm3_finish(&stSM3Ctx, aHashDigest);

    for (size_t i = 0; i < 32; i++)
    {
        printf("%02x", aHashDigest[i]); // 以 2 位十六进制打印
    }
    printf("\n");

    SM2_SIGN_CTX stCtx;
    dlog_check_return(sm2_sign_init(&stCtx, &stSM2Key[CERT_TYPE_SIGNING], SM2_DEFAULT_ID, strlen(SM2_DEFAULT_ID)), TRUE);
    dlog_check_return(sm2_sign_update(&stCtx, aHashDigest, sizeof(aHashDigest)), TRUE);
    dlog_check_return(sm2_sign_finish(&stCtx, pOutDer, &szOutDerLen), TRUE);

    printf("DER Signature (%zu bytes):\n", szOutDerLen);
    for (size_t i = 0; i < szOutDerLen; i++)
    {
        printf("%02x", pOutDer[i]);
    }
    printf("\n");

    // dlog_check_return(sm2_sign(&stSM2SigningKey, aHashDigest, sig, &siglen), TRUE);
    // dlog_check_return(sm2_do_sign(&stSM2SigningKey, aHashDigest, &stSig), TRUE);
    // dlog_check_return(sm2_signature_to_der(&stSig, &pOutDer, &szOutDerLen), TRUE);
    base64Encode(pOutDer, szOutDerLen, pOutData, pOutLen);
    printf("Base64 (%zu bytes):\n", *pOutLen);
    for (size_t i = 0; i < *pOutLen; i++)
    {
        putchar(pOutData[i]); // 直接打印 ASCII 字符
    }
    printf("\n");

    return OK;
}

// info /*----------------------- 私有函数 -----------------------*/

int CCertManage::saveCertToFile(CertType_E enCertType, const uint8_t *pData, size_t szLen)
{
    FILE *fp = nullptr;
    std::string strFilePath;
    switch (enCertType)
    {
    case CERT_TYPE_SELF_SIGNED:
        strFilePath = SM_ROOT_CERT_PATH;
        break;
    case CERT_TYPE_INTERMEDIATE:
        strFilePath = SM_MIDDLE_CERT_PATH;
        break;
    case CERT_TYPE_SIGNING:
        strFilePath = SM_SIGNING_CERT_PATH;
        break;
    case CERT_TYPE_ENCRYPTION:
        strFilePath = SM_ENCRYPTION_CERT_PATH;
        break;
    default:
        break;
    }

    /*获取文件所在的文件夹路径*/
    std::filesystem::path folderPath = std::filesystem::path(strFilePath).parent_path();

    /*检查文件夹是否存在，如果不存在则创建*/
    if (!std::filesystem::exists(folderPath))
    {
        if (!std::filesystem::create_directories(folderPath))
        {
            dlog_error("创建文件夹 %s 失败", folderPath.string().c_str());
            return ERR;
        }
    }

    /*打开文件*/
    if (!(fp = fopen(strFilePath.c_str(), "w+")))
    {
        dlog_error("打开%s文件失败", strFilePath.c_str());
        return ERR;
    }
    /*将证书转换为 PEM 格式并写入文件*/
    if (x509_cert_to_pem(pData, szLen, fp) != TRUE)
    {
        dlog_error("将证书转换为 PEM 格式并写入%s文件失败", strFilePath.c_str());
        if (fp)
        {
            fclose(fp);
            fp = nullptr;
        }
        return ERR;
    }
    if (fp)
    {
        fclose(fp);
        fp = nullptr;
    }

    return OK;
}

int CCertManage::saveCsrToFile(const uint8_t *pData, size_t szLen)
{
    FILE *fp = nullptr;
    std::string strFilePath;
    // char aTime[64] = {0};
    // get_time_T_char(aTime, sizeof(aTime));
    strFilePath = std::string(SM_CERT_REQUEST_PATH) + "request_" + std::to_string(static_cast<int>(get_time_ms())) + ".csr";
    dlog_debug("time:%s", strFilePath.c_str());

    /*获取文件所在的文件夹路径*/
    std::filesystem::path folderPath = std::filesystem::path(strFilePath).parent_path();

    /*检查文件夹是否存在，如果不存在则创建*/
    if (!std::filesystem::exists(folderPath))
    {
        if (!std::filesystem::create_directories(folderPath))
        {
            dlog_error("创建文件夹 %s 失败", folderPath.string().c_str());
            return ERR;
        }
    }

    /*打开文件*/
    if (!(fp = fopen(strFilePath.c_str(), "w+")))
    {
        dlog_error("打开%s文件失败", strFilePath.c_str());
        return ERR;
    }
    /*将证书请求转换为 PEM 格式并写入文件*/
    if (x509_req_to_pem(pData, szLen, fp) != TRUE)
    {
        dlog_error("将证书请求转换为 PEM 格式并写入%s文件失败", strFilePath.c_str());

        if (fp)
        {
            fclose(fp);
            fp = nullptr;
        }
        return ERR;
    }
    if (fp)
    {
        fclose(fp);
        fp = nullptr;
    }

    return OK;
}

int CCertManage::saveKeyPairToFile(CertType_E enCertType, SM2_KEY &stSM2Key)
{
    FILE *fp = nullptr;
    std::string strFilePath;
    switch (enCertType)
    {
    case CERT_TYPE_SELF_SIGNED:
        strFilePath = SM_ROOT_CERT_KEY_PATH;
        break;
    case CERT_TYPE_INTERMEDIATE:
        strFilePath = SM_MIDDLE_CERT_KEY_PATH;
        break;
    case CERT_TYPE_SIGNING:
        strFilePath = SM_SIGNING_CERT_KEY_PATH;
        break;
    case CERT_TYPE_ENCRYPTION:
        strFilePath = SM_ENCRYPTION_CERT_KEY_PATH;
        break;
    default:
        break;
    }

    /*获取文件所在的文件夹路径*/
    std::filesystem::path folderPath = std::filesystem::path(strFilePath).parent_path();
    /*检查文件夹是否存在，如果不存在则创建*/
    if (!std::filesystem::exists(folderPath))
    {
        if (!std::filesystem::create_directories(folderPath))
        {
            dlog_error("创建文件夹 %s 失败", folderPath.string().c_str());
            return ERR;
        }
    }

    /*打开文件*/
    if (!(fp = fopen(strFilePath.c_str(), "w+")))
    {
        dlog_error("打开%s文件失败", strFilePath.c_str());
        return ERR;
    }
    /*保存sm2私钥到pem文件，使用加密密码加密后保存*/
    // sm2_public_key_info_to_pem(&stSM2Key, fp);
    if (sm2_private_key_info_encrypt_to_pem(&stSM2Key, SM2_PRIVATE_KEY_ENCRYPT_PASSWORD, fp) != TRUE)
    {
        dlog_error("保存sm2私钥至%s失败", strFilePath.c_str());
        if (fp)
        {
            fclose(fp);
            fp = nullptr;
        }
        return ERR;
    }

    if (fp)
    {
        fclose(fp);
        fp = nullptr;
    }

    return OK;
}

int CCertManage::base64Encode(const uint8_t *input, size_t inlen, uint8_t *output, size_t *outlen)
{
    BASE64_CTX ctx;
    uint8_t *p = output;
    int len;
    size_t total_len = 0;

    if (input == nullptr || output == nullptr || outlen == nullptr || *outlen < BASE64_ENCODE_LENGTH(inlen))
    {
        return -1;
    }

    base64_encode_init(&ctx);
    dlog_check_return(base64_encode_update(&ctx, input, inlen, p, &len), TRUE);
    p += len;
    total_len += len;

    base64_encode_finish(&ctx, p, &len);
    p += len;
    total_len += len;

    *outlen = total_len;
    return 0;
}

int CCertManage::base64Decode(const uint8_t *input, size_t inlen, uint8_t *output, size_t *outlen)
{
    BASE64_CTX ctx;
    uint8_t *p = output;
    int len;
    size_t total_len = 0;

    if (!input || !output || !outlen || *outlen < BASE64_DECODE_LENGTH(inlen))
    {
        return -1;
    }

    base64_decode_init(&ctx);
    dlog_check_return(base64_decode_update(&ctx, input, inlen, p, &len), TRUE);
    p += len;
    total_len += len;

    dlog_check_return(base64_decode_finish(&ctx, p, &len), TRUE);
    p += len;
    total_len += len;

    *outlen = total_len;
    return 0;
}