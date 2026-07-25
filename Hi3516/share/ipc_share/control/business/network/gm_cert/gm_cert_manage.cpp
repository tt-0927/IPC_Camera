/**
 * @FilePath     : gm_cert_manage.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-08-23 09:47:13
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-25 10:06:51
 * @Description  : 国密证书管理
 */

#include "gm_cert_manage.h"
#include "openssl/cert_parser.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <regex>
#include <unistd.h>

namespace
{

/**
 * @brief   : 拷贝普通文件内容
 * @param    {std::string} &strSrcPath：源文件路径
 * @param    {std::string} &strDstPath：目标文件路径
 * @return   {int} OK：成功，非 OK：失败
 * @note    : 用于跨文件系统移动回退路径，避免 system("mv ...") 受空格、括号和 shell 元字符影响。
 */
int copy_file_content(const std::string &strSrcPath, const std::string &strDstPath)
{
    FILE *pSrcFile = std::fopen(strSrcPath.c_str(), "rb");
    if (pSrcFile == nullptr)
    {
        dlog_error("打开源文件失败: %s, errno=%d, error=%s", strSrcPath.c_str(), errno, std::strerror(errno));
        return ERR;
    }

    FILE *pDstFile = std::fopen(strDstPath.c_str(), "wb");
    if (pDstFile == nullptr)
    {
        dlog_error("打开目标文件失败: %s, errno=%d, error=%s", strDstPath.c_str(), errno, std::strerror(errno));
        std::fclose(pSrcFile);
        return ERR;
    }

    char achBuffer[4096] = { 0 };
    size_t nReadSize = 0;
    while ((nReadSize = std::fread(achBuffer, 1, sizeof(achBuffer), pSrcFile)) > 0)
    {
        if (std::fwrite(achBuffer, 1, nReadSize, pDstFile) != nReadSize)
        {
            dlog_error("写入目标文件失败: %s, errno=%d, error=%s", strDstPath.c_str(), errno, std::strerror(errno));
            std::fclose(pDstFile);
            std::fclose(pSrcFile);
            return ERR;
        }
    }

    if (std::ferror(pSrcFile))
    {
        dlog_error("读取源文件失败: %s, errno=%d, error=%s", strSrcPath.c_str(), errno, std::strerror(errno));
        std::fclose(pDstFile);
        std::fclose(pSrcFile);
        return ERR;
    }

    if (std::fclose(pDstFile) != 0)
    {
        dlog_error("关闭目标文件失败: %s, errno=%d, error=%s", strDstPath.c_str(), errno, std::strerror(errno));
        std::fclose(pSrcFile);
        return ERR;
    }

    std::fclose(pSrcFile);
    return OK;
}

/**
 * @brief   : 移动普通文件到目标路径
 * @param    {std::string} &strSrcPath：源文件路径
 * @param    {std::string} &strDstPath：目标文件路径
 * @return   {int} OK：成功，非 OK：失败
 * @note    : 证书上传文件名来自网页，必须绕开 shell，兼容空格、括号等合法文件名字符。
 */
int move_file_safely(const std::string &strSrcPath, const std::string &strDstPath)
{
    if (strSrcPath.empty() || strDstPath.empty())
    {
        dlog_error("移动文件路径为空: %s -> %s", strSrcPath.c_str(), strDstPath.c_str());
        return ERR_PARAM;
    }

    if (strSrcPath == strDstPath)
    {
        return OK;
    }

    errno = 0;
    if (std::rename(strSrcPath.c_str(), strDstPath.c_str()) == 0)
    {
        return OK;
    }

    const int nRenameErr = errno;
    if (nRenameErr != EXDEV)
    {
        dlog_error("重命名文件失败: %s -> %s, errno=%d, error=%s",
                   strSrcPath.c_str(),
                   strDstPath.c_str(),
                   nRenameErr,
                   std::strerror(nRenameErr));
        return ERR;
    }

    const std::string strTmpPath = strDstPath + ".tmp";
    /* info: 跨文件系统移动需要复制后替换，先写临时文件避免目标证书被半截内容覆盖 */
    std::remove(strTmpPath.c_str());
    if (copy_file_content(strSrcPath, strTmpPath) != OK)
    {
        std::remove(strTmpPath.c_str());
        return ERR;
    }

    if (std::rename(strTmpPath.c_str(), strDstPath.c_str()) != 0)
    {
        dlog_error("替换目标文件失败: %s -> %s, errno=%d, error=%s",
                   strTmpPath.c_str(),
                   strDstPath.c_str(),
                   errno,
                   std::strerror(errno));
        std::remove(strTmpPath.c_str());
        return ERR;
    }

    if (std::remove(strSrcPath.c_str()) != 0)
    {
        dlog_error("删除源文件失败: %s, errno=%d, error=%s", strSrcPath.c_str(), errno, std::strerror(errno));
        return ERR;
    }

    return OK;
}

/**
 * @brief   : 规范化证书序列号
 * @param    {std::string} &strSerialNum 原始证书序列号
 * @return   {std::string} 去分隔符、去前导零并转大写后的序列号
 * @note    : OpenSSL、GmSSL、CRL 文本输出可能存在冒号、大小写或前导零差异。
 */
std::string normalize_cert_serial(const std::string &strSerialNum)
{
    std::string strNormalized;
    strNormalized.reserve(strSerialNum.size());

    for (char ch : strSerialNum)
    {
        if (std::isxdigit(static_cast<unsigned char>(ch)))
        {
            strNormalized.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
        }
    }

    const size_t nNonZeroPos = strNormalized.find_first_not_of('0');
    if (nNonZeroPos == std::string::npos)
    {
        return strNormalized.empty() ? std::string() : std::string("0");
    }

    return strNormalized.substr(nNonZeroPos);
}

}

CGmCertManage::CGmCertManage()
{
    update_cert_num();
}

CGmCertManage::~CGmCertManage()
{
}

int CGmCertManage::ensure_device_key(bool bForceRegenerate)
{
    if (!CGb35114KeyStoreManager::instance()->is_ready())
    {
        dlog_error("GB35114 KeyStore 未就绪，无法准备设备 SM2 私钥");
        return ERR_UNINIT;
    }

    const IpcRet_E nRet = CGb35114KeyStoreManager::instance()->ensure_device_sm2_key(m_strPassword, bForceRegenerate);
    if (nRet != OK)
    {
        dlog_error("准备设备 SM2 私钥失败: ret=%d", nRet);
        return nRet;
    }

    const std::string strKeyPath = CGb35114KeyStoreManager::instance()->device_sm2_key_path();
    if (strKeyPath.empty() || access(strKeyPath.c_str(), F_OK) != 0)
    {
        dlog_error("设备 SM2 私钥准备后不存在: %s", strKeyPath.c_str());
        return ERR;
    }

    dlog_info("设备 SM2 私钥已准备: %s", strKeyPath.c_str());
    return OK;
}

std::string CGmCertManage::create_cert_request(Network::GmCertNetworkType_E enType)
{
    // todo: gmssl 中未体现证书持有者的网络格式
    // if (enType == Network::GmCertNetworkType_E::PUBLIC_SECURITY_INFORMATION_NETWORK)
    // {
    // }
    // else if (enType == Network::GmCertNetworkType_E::VIDEO_DEDICATED_NETWORK)
    // {
    // }

    /*
     * note: 用户点击创建证书请求时，CSR 应按当前 GB28181 配置重新生成，不做“关键信息变化”缓存判断。
     * note: CSR 是当前配置快照，生成成本低；设备 SM2 私钥代表设备身份，只在缺失或兼容迁移失败时重建。
     * idea: 后续若 Subject 扩展平台类型、组织信息、设备序列号等字段，应提炼 CCertSubjectBuilder 或
     * idea: CCertRequestProfile 统一组装，不在本函数堆叠配置差异判断。
     */
    Network::GB28181Client_S stGbClient;
    Convert::read_file(GB28181_CONFIG_FILE, stGbClient);

    /* 未设置国标28181 SIP用户认证ID */
    if (stGbClient.userId.empty())
    {
        return std::string();
    }
    /* 组装通用名称 */
    const std::string strModuleId = CGb35114KeyStoreManager::instance()->crypto_module_id();
    m_strCommonName = stGbClient.userId + "_" + (strModuleId.empty() ? "NULL" : strModuleId);
    if (!CCryptoManager::instance()->is_ready())
    {
        dlog_error("CCryptoManager 未就绪，无法创建证书请求文件");
        return std::string();
    }
    if (ensure_device_key(false) != OK)
    {
        return std::string();
    }
    const std::string strKeyFile = CGb35114KeyStoreManager::instance()->device_sm2_key_path();
    if (strKeyFile.empty())
    {
        dlog_error("设备 SM2 私钥路径为空，无法创建证书请求文件");
        return std::string();
    }

    /* 创建证书请求文件 */
    if (!CCryptoManager::instance()->reqgen(m_strCountry,
                                            m_strState,
                                            m_strLocality,
                                            m_strOrganization,
                                            m_strOrganizationUnit,
                                            m_strCommonName,
                                            strKeyFile,
                                            m_strPassword,
                                            m_strReqFile))
    {
        dlog_warn("创建证书请求文件失败，尝试重新生成设备私钥后重试: %s", m_strReqFile.c_str());
        if (ensure_device_key(true) != OK)
        {
            return std::string();
        }
        if (!CCryptoManager::instance()->reqgen(m_strCountry,
                                                m_strState,
                                                m_strLocality,
                                                m_strOrganization,
                                                m_strOrganizationUnit,
                                                m_strCommonName,
                                                strKeyFile,
                                                m_strPassword,
                                                m_strReqFile))
        {
            dlog_error("重新生成设备私钥后创建证书请求文件仍失败: %s", m_strReqFile.c_str());
            return std::string();
        }
    }

    return m_strReqFile;
}

int CGmCertManage::crl_parse(const std::string strCrlPath, Network::GmCrlFileInfo_S &stInfo)
{
    /* 参数验证 */
    if (strCrlPath.empty())
    {
        dlog_error("输入crl路径为空");
        return ERR;
    }
    if (stInfo.strPath.empty())
    {
        dlog_warn("目标crl路径为空, crl将不会被移动");
    }
    if (!CCryptoManager::instance()->is_ready())
    {
        dlog_error("CCryptoManager 未就绪，无法解析证书吊销列表");
        return ERR;
    }

    std::string strCrlInfo = CCryptoManager::instance()->crlparse(strCrlPath);
    if (strCrlInfo.empty())
    {
        dlog_error("证书吊销列表解析返回内容为空, 路径: %s", strCrlPath.c_str());
        return ERR;
    }

    /* 解析颁发者，兼容旧 GmSSL ASN.1 文本和 OpenSSL X509_CRL_print 文本 */
    std::regex issuerRegex(R"(issuer[\s\S]*?commonName:\s*(.*?)\s*thisUpdate)", std::regex::icase);
    std::smatch issuerMatch;
    if (std::regex_search(strCrlInfo, issuerMatch, issuerRegex) && issuerMatch.size() > 1)
    {
        stInfo.strIssuer = trim(issuerMatch[1].str());
    }
    else
    {
        std::regex opensslIssuerRegex(R"(Issuer:\s*([^\n]+))", std::regex::icase);
        if (std::regex_search(strCrlInfo, issuerMatch, opensslIssuerRegex) && issuerMatch.size() > 1)
        {
            std::string strIssuer = trim(issuerMatch[1].str());
            std::regex cnRegex(R"(CN\s*=\s*([^,\n]+))", std::regex::icase);
            std::smatch cnMatch;
            stInfo.strIssuer = std::regex_search(strIssuer, cnMatch, cnRegex) && cnMatch.size() > 1 ? trim(cnMatch[1].str())
                                                                                                    : strIssuer;
        }
    }

    /* 解析CRL更新时间 (thisUpdate) */
    std::regex thisUpdateRegex(R"(thisUpdate:\s*(.+))", std::regex::icase);
    std::smatch thisUpdateMatch;
    if (std::regex_search(strCrlInfo, thisUpdateMatch, thisUpdateRegex) && thisUpdateMatch.size() > 1)
    {
        stInfo.strThisUpdate = convert_date_format(trim(thisUpdateMatch[1].str()));
    }
    else
    {
        std::regex opensslThisUpdateRegex(R"(Last Update:\s*(.+))", std::regex::icase);
        if (std::regex_search(strCrlInfo, thisUpdateMatch, opensslThisUpdateRegex) && thisUpdateMatch.size() > 1)
        {
            stInfo.strThisUpdate = convert_date_format(trim(thisUpdateMatch[1].str()));
        }
    }

    /* 解析CRL下次更新时间 (nextUpdate) */
    std::regex nextUpdateRegex(R"(nextUpdate:\s*(.+))", std::regex::icase);
    std::smatch nextUpdateMatch;
    if (std::regex_search(strCrlInfo, nextUpdateMatch, nextUpdateRegex) && nextUpdateMatch.size() > 1)
    {
        stInfo.strNextUpdate = convert_date_format(trim(nextUpdateMatch[1].str()));
    }
    else
    {
        std::regex opensslNextUpdateRegex(R"(Next Update:\s*(.+))", std::regex::icase);
        if (std::regex_search(strCrlInfo, nextUpdateMatch, opensslNextUpdateRegex) && nextUpdateMatch.size() > 1)
        {
            stInfo.strNextUpdate = convert_date_format(trim(nextUpdateMatch[1].str()));
        }
    }

    /* 解析crlExtensions块中的CRL编号 (CRLNumber) */
    std::regex crlNumberRegex(R"(extnID: CRLNumber[\s\S]*?CRLNumber:\s*(\d+))", std::regex::icase);
    std::smatch crlNumberMatch;
    if (std::regex_search(strCrlInfo, crlNumberMatch, crlNumberRegex) && crlNumberMatch.size() > 1)
    {
        stInfo.nCrlNumber = std::stoi(crlNumberMatch[1].str());
    }
    else
    {
        std::regex opensslCrlNumberRegex(R"(X509v3 CRL Number:\s*\n\s*([0-9A-Fa-f]+))", std::regex::icase);
        if (std::regex_search(strCrlInfo, crlNumberMatch, opensslCrlNumberRegex) && crlNumberMatch.size() > 1)
        {
            stInfo.nCrlNumber = std::stoi(crlNumberMatch[1].str());
        }
    }

    /* 解析被吊销的证书列表 */
    std::regex revokedCertRegex(R"(RevokedCertificate[\s\S]*?(?=RevokedCertificate|\s*crlExtensions|\s*signatureAlgorithm|$))");
    std::sregex_iterator iter(strCrlInfo.begin(), strCrlInfo.end(), revokedCertRegex);
    std::sregex_iterator end;

    for (; iter != end; ++iter)
    {
        const std::smatch &match = *iter;
        Network::RevokedCertInfo_S revokedCert;

        /* 获取完整的 RevokedCertificate 块内容 */
        std::string certBlock = match[0].str();

        /* 在证书块中解析证书序列号 (userCertificate) */
        std::regex serialRegex(R"(userCertificate:\s*([A-F0-9]+))");
        std::smatch serialMatch;
        if (std::regex_search(certBlock, serialMatch, serialRegex) && serialMatch.size() > 1)
        {
            revokedCert.strSerialNum = serialMatch[1].str();
        }

        /* 在证书块中解析吊销日期 (revocationDate) */
        std::regex revocationDateRegex(R"(revocationDate:\s*([^\n]+))");
        std::smatch revocationDateMatch;
        if (std::regex_search(certBlock, revocationDateMatch, revocationDateRegex) && revocationDateMatch.size() > 1)
        {
            std::string revocationDate = revocationDateMatch[1].str();
            revokedCert.strRevocationDate = convert_date_format(trim(revocationDate));
        }

        /* 在证书块中解析吊销原因 (reasonCode) */
        std::regex reasonRegex(R"(reasonCode:\s*([^\s\n]+))");
        std::smatch reasonMatch;
        if (std::regex_search(certBlock, reasonMatch, reasonRegex) && reasonMatch.size() > 1)
        {
            revokedCert.strReasonCode = reasonMatch[1].str();
        }

        /* 在证书块中解析失效日期 (invalidityDate) */
        std::regex invalidityRegex(R"(invalidityDate:\s*([^\n]+))");
        std::smatch invalidityMatch;
        if (std::regex_search(certBlock, invalidityMatch, invalidityRegex) && invalidityMatch.size() > 1)
        {
            std::string invalidityDate = invalidityMatch[1].str();
            revokedCert.strInvalidityDate = convert_date_format(trim(invalidityDate));
        }

        stInfo.vecRevokedCerts.push_back(revokedCert);
    }

    /*
     * info: OpenSSL X509_CRL_print 输出使用 Serial Number/Revocation Date/CRL Reason Code，
     * 与旧 GmSSL ASN.1 文本不同。上面的旧格式解析不到吊销项时，按 OpenSSL 文本再解析一次。
     */
    if (stInfo.vecRevokedCerts.empty())
    {
        std::regex opensslRevokedCertRegex(R"(Serial Number:\s*([A-Fa-f0-9]+)[\s\S]*?(?=Serial Number:|Signature Algorithm:|$))",
                                           std::regex::icase);
        std::sregex_iterator opensslIter(strCrlInfo.begin(), strCrlInfo.end(), opensslRevokedCertRegex);
        for (; opensslIter != end; ++opensslIter)
        {
            const std::smatch &match = *opensslIter;
            Network::RevokedCertInfo_S revokedCert;
            std::string certBlock = match[0].str();

            revokedCert.strSerialNum = match.size() > 1 ? trim(match[1].str()) : "";

            std::regex revocationDateRegex(R"(Revocation Date:\s*([^\n]+))", std::regex::icase);
            std::smatch revocationDateMatch;
            if (std::regex_search(certBlock, revocationDateMatch, revocationDateRegex) && revocationDateMatch.size() > 1)
            {
                revokedCert.strRevocationDate = convert_date_format(trim(revocationDateMatch[1].str()));
            }

            std::regex reasonRegex(R"(CRL Reason Code:\s*\n\s*([^\n]+))", std::regex::icase);
            std::smatch reasonMatch;
            if (std::regex_search(certBlock, reasonMatch, reasonRegex) && reasonMatch.size() > 1)
            {
                revokedCert.strReasonCode = trim(reasonMatch[1].str());
            }

            std::regex invalidityRegex(R"(Invalidity Date:\s*\n\s*([^\n]+))", std::regex::icase);
            std::smatch invalidityMatch;
            if (std::regex_search(certBlock, invalidityMatch, invalidityRegex) && invalidityMatch.size() > 1)
            {
                revokedCert.strInvalidityDate = convert_date_format(trim(invalidityMatch[1].str()));
            }

            if (!revokedCert.strSerialNum.empty())
            {
                stInfo.vecRevokedCerts.push_back(revokedCert);
            }
        }
    }

    dlog_info("CRL解析完成: issuer=%s, thisUpdate=%s, nextUpdate=%s, crlNumber=%d, revokedCount=%zu",
              stInfo.strIssuer.c_str(),
              stInfo.strThisUpdate.c_str(),
              stInfo.strNextUpdate.c_str(),
              stInfo.nCrlNumber,
              stInfo.vecRevokedCerts.size());

    /* 移动证书至对应路径 (如果目标路径有效) */
    if (!stInfo.strPath.empty() && strCrlPath != stInfo.strPath)
    {
        int nRet = move_file_safely(strCrlPath, stInfo.strPath);
        if (nRet != OK)
        {
            dlog_error("移动文件失败: %s -> %s, 返回码: %d", strCrlPath.c_str(), stInfo.strPath.c_str(), nRet);
            return ERR;
        }
    }

    return OK;
}

bool CGmCertManage::crl_verify(const std::string &strCrlPath, const std::string &strCertPath)
{
    /* 参数验证 */
    if (strCrlPath.empty() || strCertPath.empty())
    {
        dlog_error("参数为空");
        return false;
    }
    if (!CCryptoManager::instance()->is_ready())
    {
        dlog_error("CCryptoManager 未就绪，无法验证证书吊销列表");
        return false;
    }

    CertInfo_S stCaCertInfo;
    if (!parse_certificate(strCertPath, stCaCertInfo))
    {
        dlog_error("CRL验证失败，无法解析签发者证书: %s", strCertPath.c_str());
        return false;
    }
    if (!stCaCertInfo.bBasicConstraintsCA || !stCaCertInfo.bKeyUsageKeyCertSign || !stCaCertInfo.bKeyUsageCRLSign)
    {
        /*
         * info: cRLSign 仅在当前CA需要签发和验证CRL时强制要求。
         * 缺少该用途的CA仍可用于证书链验证，但不能作为CRL签发者。
         */
        dlog_error("CRL验证失败，签发者证书无CRL签发权限: ca=%d, keyCertSign=%d, cRLSign=%d",
                   stCaCertInfo.bBasicConstraintsCA,
                   stCaCertInfo.bKeyUsageKeyCertSign,
                   stCaCertInfo.bKeyUsageCRLSign);
        return false;
    }

    std::string strCrlInfo = CCryptoManager::instance()->crlverify(strCrlPath, strCertPath);
    if (strCrlInfo.empty())
    {
        dlog_error("证书吊销列表有误");
        return false;
    }

    if (strCrlInfo != "Verification success")
    {
        dlog_error("证书吊销列表验证失败");
        return false;
    }
    return true;
}

bool CGmCertManage::cert_verify(const std::string &strCertPath, const std::string &strCaCertPath)
{
    /* 参数验证 */
    if (strCertPath.empty() || strCaCertPath.empty())
    {
        dlog_error("证书验证参数为空");
        return false;
    }

    if (!verify_certificate_by_ca(strCertPath, strCaCertPath))
    {
        dlog_error("设备证书签发者验证失败: cert=%s, issuer=%s", strCertPath.c_str(), strCaCertPath.c_str());
        return false;
    }

    return true;
}

bool CGmCertManage::is_cert_revoked(const std::string &strCertPath)
{
    if (strCertPath.empty())
    {
        dlog_error("证书吊销状态检查失败，证书路径为空");
        return true;
    }

    Network::GmCrlFileInfo_S stCrlInfo;
    CGmCertConfigure::instance()->get_configure(stCrlInfo);
    const std::string strCrlPath = stCrlInfo.strPath.empty() ? std::string(GM_CA_TRUST_CRL) : stCrlInfo.strPath;

    /*
     * info: 设备允许未上传 CRL 的部署方式。只有 CRL 文件存在并且有吊销项时，才参与证书状态判断。
     */
    if (strCrlPath.empty() || access(strCrlPath.c_str(), F_OK) != 0 || stCrlInfo.vecRevokedCerts.empty())
    {
        return false;
    }

    CertInfo_S certInfo;
    if (!parse_certificate(strCertPath, certInfo))
    {
        dlog_error("证书吊销状态检查失败，无法解析证书: %s", strCertPath.c_str());
        return true;
    }

    const std::string strCertSerial = normalize_cert_serial(certInfo.strSerialNumber);
    if (strCertSerial.empty())
    {
        dlog_error("证书吊销状态检查失败，证书序列号为空: %s", strCertPath.c_str());
        return true;
    }

    for (const auto &stRevokedCert : stCrlInfo.vecRevokedCerts)
    {
        if (strCertSerial == normalize_cert_serial(stRevokedCert.strSerialNum))
        {
            dlog_error("证书已被 CRL 吊销: cert=%s, serial=%s", strCertPath.c_str(), strCertSerial.c_str());
            return true;
        }
    }

    return false;
}

int CGmCertManage::cert_parse(const std::string strPath, Network::GmCertFileInfo_S &stInfo)
{
    /* 参数验证 */
    if (strPath.empty())
    {
        dlog_error("输入证书路径为空");
        return ERR_CERT_FORMAT;
    }
    if (stInfo.strPath.empty())
    {
        dlog_warn("目标证书路径为空, 证书将不会被移动");
    }

    /* 使用结构化解析 */
    CertInfo_S certInfo;
    if (!parse_certificate(strPath, certInfo))
    {
        dlog_error("证书解析失败, 路径: %s", strPath.c_str());
        return ERR_CERT_FORMAT;
    }

    if (stInfo.strPath == GM_CA_TRUST_CERT)
    {
        /*
         * info: CA 槽位用于设备证书签发链和 CRL 验签，必须具备证书签发与 CRL 签发能力。
         */
        if (!certInfo.bBasicConstraintsCA || !certInfo.bKeyUsageKeyCertSign || !certInfo.bKeyUsageCRLSign)
        {
            dlog_error("CA证书用途不合法: ca=%d, keyCertSign=%d, cRLSign=%d",
                       certInfo.bBasicConstraintsCA,
                       certInfo.bKeyUsageKeyCertSign,
                       certInfo.bKeyUsageCRLSign);
            return ERR_CERT_FORMAT;
        }
        stInfo.strFunction = "CA证书";
    }

    if (stInfo.strPath == GM_PLATFORM_CERT)
    {
        if (!certInfo.bKeyUsageDigitalSignature)
        {
            dlog_error("平台证书用途不合法，缺少数字签名用途: ca=%d, keyCertSign=%d, cRLSign=%d",
                       certInfo.bBasicConstraintsCA,
                       certInfo.bKeyUsageKeyCertSign,
                       certInfo.bKeyUsageCRLSign);
            return ERR_CERT_FORMAT;
        }

        /*
         * review: GB35114 标准平台证书通常不应具备 CA 签发能力；LiveGBS 自签名平台证书会携带
         * CA:TRUE 和 keyCertSign，因此以下严格终端实体约束仅保留为审查依据，当前不启用。
         * if (certInfo.bBasicConstraintsCA || certInfo.bKeyUsageKeyCertSign || certInfo.bKeyUsageCRLSign)
         * {
         *     dlog_error("平台证书包含签发者用途: ca=%d, keyCertSign=%d, cRLSign=%d",
         *                certInfo.bBasicConstraintsCA,
         *                certInfo.bKeyUsageKeyCertSign,
         *                certInfo.bKeyUsageCRLSign);
         *     return ERR_CERT_FORMAT;
         * }
         */
        stInfo.strFunction = "平台证书";
    }

    /*
     * info: 上传到设备证书槽位的证书不能具备 CA 签发能力，防止把 CA/中间 CA 误作为本地设备证书。
     */
    if (stInfo.strPath == GM_CA_DEVICE_CERT)
    {
        if (certInfo.bBasicConstraintsCA || certInfo.bKeyUsageKeyCertSign || certInfo.bKeyUsageCRLSign)
        {
            dlog_error("设备证书用途不合法: ca=%d, keyCertSign=%d, cRLSign=%d",
                       certInfo.bBasicConstraintsCA,
                       certInfo.bKeyUsageKeyCertSign,
                       certInfo.bKeyUsageCRLSign);
            return ERR_CERT_FORMAT;
        }
        stInfo.strFunction = "本地设备证书";
    }

    /* 更新证书序号 */
    update_cert_num();

    /* 映射结构化数据到现有数据结构 */
    stInfo.strSerialNum = certInfo.strSerialNumber;
    stInfo.strUser = certInfo.strSubjectCN;

    /* 有效期 */
    stInfo.strEffectiveDate = convert_date_format(certInfo.strNotBefore);
    stInfo.strExpiraDate = convert_date_format(certInfo.strNotAfter);

    /* 设置证书序号 */
    stInfo.nNum = m_nCertNum++;

    /* 移动证书至对应路径 (如果目标路径有效) */
    if (!stInfo.strPath.empty() && strPath != stInfo.strPath)
    {
        int nRet = move_file_safely(strPath, stInfo.strPath);
        if (nRet != OK)
        {
            dlog_error("移动文件失败: %s -> %s, 返回码: %d", strPath.c_str(), stInfo.strPath.c_str(), nRet);
            return nRet;
        }
    }

    return OK;
}

void CGmCertManage::update_cert_num()
{
    std::set<::Network::GmCertFileInfo_S> astInfo;
    CGmCertConfigure::instance()->get_configure(astInfo);
    m_nCertNum = astInfo.size() + 1;
}

std::string CGmCertManage::trim(const std::string &str)
{
    const std::string whitespace = " \t\r\n";
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return "";

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

std::string CGmCertManage::convert_date_format(const std::string &dateStr)
{
    std::string months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    std::istringstream iss(dateStr);
    std::string weekday, month, day, time, year;

    /* info: 兼容 OpenSSL X509_CRL_print 日期格式，例如 "Jun 16 09:30:14 2026 GMT" */
    if (iss >> month >> day >> time >> year)
    {
        int monthNum = 0;
        for (int i = 0; i < 12; i++)
        {
            if (months[i] == month)
            {
                monthNum = i + 1;
                break;
            }
        }

        if (monthNum > 0)
        {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%s-%02d-%02d %s", year.c_str(), monthNum, std::stoi(day), time.c_str());
            return std::string(buffer);
        }
    }

    iss.clear();
    iss.str(dateStr);
    if (iss >> weekday >> month >> day >> time >> year)
    {
        /* 找到月份对应的数字 */
        int monthNum = 1;
        for (int i = 0; i < 12; i++)
        {
            if (months[i] == month)
            {
                monthNum = i + 1;
                break;
            }
        }

        /* 格式化输出为 YYYY-MM-DD HH:MM:SS 格式 */
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%s-%02d-%02d %s", year.c_str(), monthNum, std::stoi(day), time.c_str());
        return std::string(buffer);
    }

    /* 如果解析失败，返回原始字符串 */
    return dateStr;
}
