/**
 * @FilePath     : gm_cert_manage.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-08-23 09:47:13
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-26 11:34:09
 * @Description  : 国密证书管理
 */

#include "gm_cert_manage.h"

CGmCertManage::CGmCertManage()
{
    update_cert_num();
}

CGmCertManage::~CGmCertManage()
{
}

std::string CGmCertManage::create_cert_request(Network::GmCertNetworkType_E enType)
{
    // todo gmssl中未体现证书持有者的网络格式
    // if (enType == Network::GmCertNetworkType_E::PUBLIC_SECURITY_INFORMATION_NETWORK)
    // {
    // }
    // else if (enType == Network::GmCertNetworkType_E::VIDEO_DEDICATED_NETWORK)
    // {
    // }
    /* 判断是否已经生成过了 */
    if (access(m_strReqFile.c_str(), F_OK))
    {
        Network::GB28181Client_S stGbClient;
        Convert::read_file(GB28181_CONFIG_FILE, stGbClient);
        
        /* 未设置国标28181 SIP用户认证ID */
        if(stGbClient.userId.empty())
        {
            return std::string();
        }
        /* 组装通用名称 */
        m_strCommonName = stGbClient.userId + "_NULL";
        /* 创建证书请求文件 */
        CGmSSL::instance()->reqgen(m_strCountry,
                                   m_strState,
                                   m_strLocality,
                                   m_strOrganization,
                                   m_strOrganizationUnit,
                                   m_strCommonName,
                                   m_strKeyFile,
                                   m_strPassword,
                                   m_strReqFile);
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

    std::string strCrlInfo = CGmSSL::instance()->crlparse(strCrlPath);
    if (strCrlInfo.empty())
    {
        dlog_error("证书吊销列表解析返回内容为空, 路径: %s", strCrlPath.c_str());
        return ERR;
    }

    /* 解析颁发者 (issuer中的commonName) */
    std::regex  issuerRegex(R"(issuer[\s\S]*?commonName:\s*(.*?)\s*thisUpdate)");
    std::smatch issuerMatch;
    if (std::regex_search(strCrlInfo, issuerMatch, issuerRegex) && issuerMatch.size() > 1)
    {
        stInfo.strIssuer = trim(issuerMatch[1].str());
    }

    /* 解析CRL更新时间 (thisUpdate) */
    std::regex  thisUpdateRegex(R"(thisUpdate:\s*(.+))");
    std::smatch thisUpdateMatch;
    if (std::regex_search(strCrlInfo, thisUpdateMatch, thisUpdateRegex) && thisUpdateMatch.size() > 1)
    {
        stInfo.strThisUpdate = convert_date_format(trim(thisUpdateMatch[1].str()));
    }

    /* 解析CRL下次更新时间 (nextUpdate) */
    std::regex  nextUpdateRegex(R"(nextUpdate:\s*(.+))");
    std::smatch nextUpdateMatch;
    if (std::regex_search(strCrlInfo, nextUpdateMatch, nextUpdateRegex) && nextUpdateMatch.size() > 1)
    {
        stInfo.strNextUpdate = convert_date_format(trim(nextUpdateMatch[1].str()));
    }

    /* 解析crlExtensions块中的CRL编号 (CRLNumber) */
    std::regex  crlNumberRegex(R"(extnID: CRLNumber[\s\S]*?CRLNumber:\s*(\d+))");
    std::smatch crlNumberMatch;
    if (std::regex_search(strCrlInfo, crlNumberMatch, crlNumberRegex) && crlNumberMatch.size() > 1)
    {
        stInfo.nCrlNumber = std::stoi(crlNumberMatch[1].str());
    }

    /* 解析被吊销的证书列表 */
    std::regex revokedCertRegex(
        R"(RevokedCertificate[\s\S]*?(?=RevokedCertificate|\s*crlExtensions|\s*signatureAlgorithm|$))");
    std::sregex_iterator iter(strCrlInfo.begin(), strCrlInfo.end(), revokedCertRegex);
    std::sregex_iterator end;

    for (; iter != end; ++iter)
    {
        const std::smatch         &match = *iter;
        Network::RevokedCertInfo_S revokedCert;

        /* 获取完整的 RevokedCertificate 块内容 */
        std::string certBlock = match[0].str();

        /* 在证书块中解析证书序列号 (userCertificate) */
        std::regex  serialRegex(R"(userCertificate:\s*([A-F0-9]+))");
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
        std::regex  reasonRegex(R"(reasonCode:\s*([^\s\n]+))");
        std::smatch reasonMatch;
        if (std::regex_search(certBlock, reasonMatch, reasonRegex) && reasonMatch.size() > 1)
        {
            revokedCert.strReasonCode = reasonMatch[1].str();
        }

        /* 在证书块中解析失效日期 (invalidityDate) */
        std::regex  invalidityRegex(R"(invalidityDate:\s*([^\n]+))");
        std::smatch invalidityMatch;
        if (std::regex_search(certBlock, invalidityMatch, invalidityRegex) && invalidityMatch.size() > 1)
        {
            std::string invalidityDate = invalidityMatch[1].str();
            revokedCert.strInvalidityDate = convert_date_format(trim(invalidityDate));
        }

        stInfo.vecRevokedCerts.push_back(revokedCert);
    }

    /* 移动证书至对应路径 (如果目标路径有效) */
    if (!stInfo.strPath.empty() && strCrlPath != stInfo.strPath)
    {
        std::ostringstream ossCmd;
        ossCmd << "mv " << strCrlPath << " " << stInfo.strPath;
        int nRet = std::system(ossCmd.str().c_str());
        if (nRet != 0)
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

    std::string strCrlInfo = CGmSSL::instance()->crlverify(strCrlPath, strCertPath);
    if(strCrlInfo.empty())
    {
        dlog_error("证书吊销列表有误");
        return false;
    }

    if(strCrlInfo != "Verification success")
    {
        dlog_error("证书吊销列表验证失败");
        return false;
    }
    return true;
}

int CGmCertManage::cert_parse(const std::string strPath, Network::GmCertFileInfo_S &stInfo)
{
    /* 参数验证 */
    if (strPath.empty())
    {
        dlog_error("输入证书路径为空");
        return ERR;
    }
    if (stInfo.strPath.empty())
    {
        dlog_warn("目标证书路径为空, 证书将不会被移动");
    }

    std::string strCertInfo = CGmSSL::instance()->certparse(strPath);
    if(strCertInfo.empty())
    {
        dlog_error("证书解析返回内容为空, 路径: %s", strPath.c_str());
        return ERR;
    }

    /* 更新证书序号 */
    update_cert_num();

    /* 解析证书序列号 (serialNumber) */
    std::regex  serialRegex(R"(serialNumber:\s*([A-F0-9]+))");
    std::smatch serialMatch;
    if (std::regex_search(strCertInfo, serialMatch, serialRegex))
    {
        stInfo.strSerialNum = serialMatch[1].str();
    }

    /* 解析使用者 (subject中的commonName) */
    std::regex subjectRegex(R"(subject[\s\S]*?commonName:\s*(.*?)(?=\s*(?:serialNumber:\s*[A-Fa-f0-9]+\s*)?subjectPulbicKeyInfo|\s*extensions|\s*signatureAlgorithm|\s*$))");
    std::smatch subjectMatch;
    if (std::regex_search(strCertInfo, subjectMatch, subjectRegex) && subjectMatch.size() > 1)
    {
        dlog_debug("subjectMatch[1].str():%s",subjectMatch[1].str().c_str());
        stInfo.strUser = trim(subjectMatch[1].str());
    }

    /* 解析证书功能 (KeyUsage 和 ExtKeyUsage) */
    std::string function = "本地设备证书"; // 默认为本地设备证书

    /* 检查是否为CA证书 */
    /* 检查BasicConstraints扩展中是否包含CA :TRUE */
    std::regex basicConstraintsRegex(R"(cA:\s*true)", std::regex::icase);
    if (std::regex_search(strCertInfo, basicConstraintsRegex))
    {
        function = "CA证书";
    }
    else
    {
        /* 检查KeyUsage中是否包含keyCertSign */
        std::regex keyCertSignRegex(R"(KeyUsage[\s\S]*?keyCertSign)");
        if (std::regex_search(strCertInfo, keyCertSignRegex))
        {
            function = "CA证书";
        }
        // else
        // {
        //     /* 通过文件名判断（如果包含ca、CA、Ca等） */
        //     std::string lowerCasePath = strPath;
        //     std::transform(lowerCasePath.begin(), lowerCasePath.end(), lowerCasePath.begin(), ::tolower);
        //     if (lowerCasePath.find("ca") != std::string::npos)
        //     {
        //         function = "CA证书";
        //     }
        // }
    }

    /* 证书功能 */
    stInfo.strFunction = function;

    /* 解析有效期开始时间 (notBefore) */
    std::regex  notBeforeRegex(R"(notBefore:\s*(.*?)\s*notAfter:)");
    std::smatch notBeforeMatch;
    if (std::regex_search(strCertInfo, notBeforeMatch, notBeforeRegex) && notBeforeMatch.size() > 1)
    {
        stInfo.strEffectiveDate = convert_date_format(trim(notBeforeMatch[1].str()));
    }

    /* 解析有效期结束时间 (notAfter) */
    std::regex  notAfterRegex(R"(notAfter:\s*(.*?)\s*subject)");
    std::smatch notAfterMatch;
    if (std::regex_search(strCertInfo, notAfterMatch, notAfterRegex) && notAfterMatch.size() > 1)
    {
        stInfo.strExpiraDate = convert_date_format(trim(notAfterMatch[1].str()));
    }

    /* 设置证书序号 */
    stInfo.nNum = m_nCertNum++;

    /* 移动证书至对应路径 (如果目标路径有效) */
    if (!stInfo.strPath.empty() && strPath != stInfo.strPath)
    {
        std::ostringstream ossCmd;
        ossCmd << "mv " << strPath << " " << stInfo.strPath;
        int nRet = std::system(ossCmd.str().c_str());
        if (nRet != 0)
        {
            dlog_error("移动文件失败: %s -> %s, 返回码: %d", strPath.c_str(), stInfo.strPath.c_str(), nRet);
            return ERR;
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

std::string CGmCertManage::trim(const std::string& str)
{
    const std::string whitespace = " \t\r\n";
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return "";

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

std::string CGmCertManage::convert_date_format(const std::string& dateStr)
{
    std::string months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    std::istringstream iss(dateStr);
    std::string weekday, month, day, time, year;

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
