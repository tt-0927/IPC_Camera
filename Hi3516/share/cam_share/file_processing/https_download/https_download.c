/*
 * @FilePath     : https_download.c
 * @Author       : huangjunda
 * @Date         : 2024-08-21 13:49:11
 * @LastEditors: lianghaoyao 709692194@qq.com
 * @LastEditTime: 2025-02-13 20:00:50
 * @Description  : 用于https协议下载文件
 */

#include "https_download.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <curl/curl.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/asn1.h>
#include <time.h>
#include "dlog.h"
#include "edukit_conf.h"

/*** 
 * @description : 检查证书是否有效的线程函数
 * @author      : huangjunda
 * @param        {void} *arg
 * @return       {*}
 */
void *cert_check_thread(void *arg)
{
    int certStatus = -1;

	FILE *fPcert = NULL;
    /* 判断CA证书文件是否存在 */
    fPcert = fopen(CA_CERTIFICATE_FILE, "r");
    if (!fPcert)
    {
        // download_https_cacert();
        dlog(LOG_ERROR, "%s does not exist", CA_CERTIFICATE_FILE);
        return NULL;
    }
    
	fclose(fPcert);
	fPcert = NULL;

    while (1)
    {
        certStatus = is_certificate_valid();
        if (1 == certStatus)
        {
            dlog(LOG_DEBUG, "下载新证书");
            if (0 != download_https_cacert())
            {
                dlog(LOG_DEBUG, "下载证书失败");
            }
            dlog(LOG_DEBUG, "下载新证书成功");
        }

        // 等待一个月
        sleep(CHECK_INTERVAL);
    }
    return NULL;
}

/*** 
 * @description : 用于写入数据的回调函数
 * @author      : huangjunda
 * @param        {void} *ptr
 * @param        {size_t} size
 * @param        {size_t} nmemb
 * @param        {FILE} *stream
 * @return       {*}
 */
size_t https_write_data(void *pBuffer, size_t nSize, size_t nMemb, void *pUserPara)
{
    size_t nTotalSize = nSize * nMemb;
    FILE* fp = (FILE*)pUserPara;
    if (fp)
    {
        size_t written = fwrite(pBuffer, nSize, nMemb, fp);
        return written;
    }
    return 0;
}

/*** 
 * @description : 下载CA证书
 * @author      : huangjunda
 * @return       {*}
 */
int download_https_cacert()
{
    FILE *fPtr = NULL;
    CURL *pCurl = NULL;
    CURLcode nRet = -1;
    char achCmd[128] = {0};

    fPtr = fopen(CA_CERTIFICATE_TMP_FILE, "wb");
    if (!fPtr)
    {
        dlog(LOG_ERROR, "fopen file %s error!", CA_CERTIFICATE_TMP_FILE);
        return nRet;
    }

    /* 打印curl版本信息 */
    dlog(LOG_DEBUG, "curl version: %s", curl_version());
    /* 初始化curl */
    curl_global_init(CURL_GLOBAL_DEFAULT);
    pCurl = curl_easy_init();
    /* 设置远程主机的url地址 */
    curl_easy_setopt(pCurl, CURLOPT_URL, HTTPS_SERTIFICATES_LINK);
    /* 设置回调函数来保存接收数据 */
    curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, https_write_data);
    /* 设置回调函数参数 */
    curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, fPtr);
    /* 设置为1L来启用对对等方（服务器）的证书验证 */
    curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, 1L);
    /* 设置为2L来启用对主机名的验证 */
    curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYHOST, 2L);
    /* 设置证书路径 */
    curl_easy_setopt(pCurl, CURLOPT_CAINFO, CA_CERTIFICATE_FILE);
    /* 设置连接超时时间，单位s */
    curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT, 20);
    /* 设置下载超时时间，单位s */
    curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 240);
    /* 1L：禁用信号，0L：启用信号（这是默认行为），禁用信号可以避免由于信号处理引起的潜在的竞争条件和其他问题 */
    curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1L);
    /* 打开详细信息 */
    curl_easy_setopt(pCurl, CURLOPT_VERBOSE, 1);
    /* 请求数据 */
    nRet = curl_easy_perform(pCurl);
    if (nRet != CURLE_OK)
    {
        switch (nRet)
        {
        case CURLE_UNSUPPORTED_PROTOCOL:
            dlog(LOG_ERROR, "unsupported protocol!");
            break;
        case CURLE_COULDNT_CONNECT:
            dlog(LOG_ERROR, "couldnt connect!");
            break;
        case CURLE_HTTP_RETURNED_ERROR:
            dlog(LOG_ERROR, "https returned error!");
            break;
        case CURLE_READ_ERROR:
            dlog(LOG_ERROR, "read error!");
            break;
        default:
            dlog(LOG_ERROR, "get error! %d", nRet);
            break;
        }
    }

    dlog(LOG_DEBUG, "download_https_cacert nRet: %d", nRet);

    /* 释放curl资源 */
    curl_easy_cleanup(pCurl);
    fclose(fPtr);
    fPtr = NULL;

    memset(achCmd, 0, sizeof(achCmd));
    snprintf(achCmd, sizeof(achCmd), "mv %s %s", CA_CERTIFICATE_TMP_FILE, CA_CERTIFICATE_FILE);
    system(achCmd);

    if (chmod(CA_CERTIFICATE_FILE, 0777) < 0)
    {
        dlog(LOG_ERROR, "无法设置文件权限%s", CA_CERTIFICATE_FILE);
        nRet = -1;
    }

    return nRet;
}

/*** 
 * @description : 检查证书是否有效
 * @author      : huangjunda
 * @return       {*}
 */
int is_certificate_valid()
{
    int nRet = -1;
    FILE *fp = fopen(CA_CERTIFICATE_FILE, "r");
    if (!fp)
    {
        dlog(LOG_ERROR, "证书文件不存在");
        return nRet;
    }

    X509 *cert = PEM_read_X509(fp, NULL, NULL, NULL);
    fclose(fp);

    if (!cert)
    {
        dlog(LOG_ERROR, "无法读取证书: %s", ERR_error_string(ERR_get_error(), NULL));
        return nRet; // 证书无效或无法读取
    }

    // 检查证书的有效期
    ASN1_TIME *notBefore = X509_get_notBefore(cert);
    ASN1_TIME *notAfter = X509_get_notAfter(cert);

    time_t currentTime = time(NULL);
    ASN1_TIME *currentTimeAsn1 = ASN1_TIME_new();
    ASN1_TIME_set(currentTimeAsn1, currentTime);

    // 检查证书是否快到期，例如在30天内
    ASN1_TIME *expireTime = ASN1_TIME_new();
    ASN1_TIME_adj(expireTime, currentTime, CHECK_INTERVAL, 0); // 添加30天

    int notBeforeCmp = ASN1_TIME_compare(notBefore, currentTimeAsn1);
    int notAfterCmp = ASN1_TIME_compare(notAfter, currentTimeAsn1);
    int expireCmp = ASN1_TIME_compare(expireTime, currentTimeAsn1);

    ASN1_TIME_free(currentTimeAsn1);
    ASN1_TIME_free(expireTime);
    X509_free(cert);

    if (notBeforeCmp > 0)
    {
        dlog(LOG_DEBUG, "证书尚未生效");
        return nRet;
    }
    if (notAfterCmp < 0)
    {
        dlog(LOG_DEBUG, "证书已过期");
        return nRet;
    }
    if (expireCmp < 0)
    {
        nRet = 1;
        dlog(LOG_DEBUG, "证书即将在%d天内过期，建议下载新证书", CHECK_INTERVAL);
        return nRet;
    }

    nRet = 0;
    dlog(LOG_DEBUG, "证书有效");
    return nRet;
}

/*** 
 * @description : 下载HTTPS链接的文件
 * @author      : huangjunda
 * @param        {char} *pUrl
 * @param        {char} *pFileName
 * @return       {*}
 */
int download_https_upgrade_package(const char *pUrl, const char *pFileName)
{
    struct stat st;
    FILE *fPcert = NULL;
    FILE *fPupgrade = NULL;
    CURL *pCurl = NULL;
    CURLcode nRet = -1;
    /* 存储完整的文件路径 */
    char achUpgradeFilePath[256] = {0};
    int certStatus = -1;

    /* 检查目录是否存在 */
    if (stat(HTTPS_SERTIFICATES_PATH, &st) == -1)
    {
        /* 如果目录不存在，尝试创建目录 */
        // if (mkdir(HTTPS_SERTIFICATES_PATH, 0755) == -1)
        // {
        //     perror("mkdir");
        //     return nRet;
        // }
        // dlog(LOG_DEBUG, "创建文件夹：%s", HTTPS_SERTIFICATES_PATH);
        // download_https_cacert();
        dlog(LOG_ERROR, "%s does not exist", HTTPS_SERTIFICATES_PATH);
        return nRet;
    }
    else if (!S_ISDIR(st.st_mode))
    {
        dlog(LOG_ERROR, "%s is not a directory", HTTPS_SERTIFICATES_PATH);
        return nRet;
    }

    /* 判断CA证书文件是否存在 */
    fPcert = fopen(CA_CERTIFICATE_FILE, "r");
    if (!fPcert)
    {
        // download_https_cacert();
        dlog(LOG_ERROR, "%s does not exist", CA_CERTIFICATE_FILE);
        return nRet;
    }

    /* 检查证书有效性 */
    certStatus = is_certificate_valid();
    if (1 == certStatus)
    {
        dlog(LOG_DEBUG, "下载新证书");
        if (download_https_cacert() != 0)
        {
            dlog(LOG_DEBUG, "下载证书失败");
            return nRet;
        }
        dlog(LOG_DEBUG, "下载新证书成功");
    }
    else if (-1 == certStatus)
    {
        return nRet;
    }

    /* 拼接出完整的文件路径 */
    snprintf(achUpgradeFilePath, sizeof(achUpgradeFilePath), "%s/%s", HTTPS_UPGRADE_PATH, pFileName);
    fPupgrade = fopen(achUpgradeFilePath, "wb");
    if (!fPupgrade)
    {
        dlog(LOG_ERROR, "fopen file %s error!", achUpgradeFilePath);
        return nRet;
    }

    /* 打印curl版本信息 */
    dlog(LOG_DEBUG, "curl version: %s", curl_version());
    /* 初始化curl */
    curl_global_init(CURL_GLOBAL_DEFAULT);
    pCurl = curl_easy_init();
    /* 设置远程主机的url地址 */
    curl_easy_setopt(pCurl, CURLOPT_URL, pUrl);
    /* 设置回调函数来保存接收数据 */
    curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, https_write_data);
    /* 设置回调函数参数 */
    curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, fPupgrade);
    /* 设置为1L来启用对对等方（服务器）的证书验证 */
    curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, 1L);
    /* 设置为2L来启用对主机名的验证 */
    curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYHOST, 2L);
    /* 设置证书路径 */
    curl_easy_setopt(pCurl, CURLOPT_CAINFO, CA_CERTIFICATE_FILE);
    /* 设置连接超时时间，单位s */
    curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT, 20);
    /* 设置下载超时时间，单位s */
    curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 240);
    /* 1L：禁用信号，0L：启用信号（这是默认行为），禁用信号可以避免由于信号处理引起的潜在的竞争条件和其他问题 */
    curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1L);
    /* 打开详细信息 */
    curl_easy_setopt(pCurl, CURLOPT_VERBOSE, 1);
    /* 请求数据 */
    nRet = curl_easy_perform(pCurl);
    if (nRet != CURLE_OK)
    {
        switch (nRet)
        {
        case CURLE_UNSUPPORTED_PROTOCOL:
            dlog(LOG_ERROR, "unsupported protocol!");
            break;
        case CURLE_COULDNT_CONNECT:
            dlog(LOG_ERROR, "couldnt connect!");
            break;
        case CURLE_HTTP_RETURNED_ERROR:
            dlog(LOG_ERROR, "http returned error!");
            break;
        case CURLE_READ_ERROR:
            dlog(LOG_ERROR, "read error!");
            break;
        case CURLE_OPERATION_TIMEDOUT:
            dlog(LOG_ERROR, "connect timeout!");
            break;
        default:
            dlog(LOG_ERROR, "get error code:%d", nRet);
            break;
        }
    }
    else
    {
        if (chmod(achUpgradeFilePath, 0777) < 0)
        {
            dlog(LOG_ERROR, "无法设置文件权限%s", achUpgradeFilePath);
            nRet = -1;
        }
    }

    /* 释放curl资源 */
    curl_easy_cleanup(pCurl);
    fclose(fPupgrade);
    fclose(fPcert);
    fPupgrade = NULL;
    fPcert = NULL;

    return nRet;
}

/*** 
 * @description : 检查升级包
 * @author      : huangjunda
 * @return       {*}
 */
int check_package()
{
    FILE *fPtr = NULL;
    char achCmd[128] = {0};
    char achBinFileName[128] = {0};
    int nRet = -1;

    memset(achCmd, 0, sizeof(achCmd));
    snprintf(achCmd, sizeof(achCmd), "ls %s/*.bin", HTTPS_UPGRADE_PATH);
    fPtr = popen(achCmd, "r");
    if (NULL == fPtr)
    {
        dlog(LOG_ERROR, "fopen error!");
        return nRet;
    }

    memset(achBinFileName, 0, sizeof(achBinFileName));
    if (0 < fread(achBinFileName, 1, sizeof(achBinFileName), fPtr))
    {
        pclose(fPtr);
        fPtr = NULL;

        memset(achCmd, 0, sizeof(achCmd));
        snprintf(achCmd, sizeof(achCmd), "chmod 777 %s/*.bin", HTTPS_UPGRADE_PATH);
        fPtr = popen(achCmd, "r");
        if (NULL == fPtr)
        {
            dlog(LOG_ERROR, "fopen error!");
            return nRet;
        }

        nRet = 0;
        pclose(fPtr);
        fPtr = NULL;
        return nRet;
    }
    fclose(fPtr);
    fPtr = NULL;

    memset(achCmd, 0, sizeof(achCmd));
    snprintf(achCmd, sizeof(achCmd), "tar -xzvf %s/*.tar.gz -C %s", HTTPS_UPGRADE_PATH, HTTPS_UPGRADE_PATH);
    fPtr = popen(achCmd, "r");
    if (NULL == fPtr)
    {
        dlog(LOG_ERROR, "fopen error!");
        return nRet;
    }
    fclose(fPtr);
    fPtr = NULL;

    memset(achCmd, 0, sizeof(achCmd));
    snprintf(achCmd, sizeof(achCmd), "rm %s/*.tar.gz", HTTPS_UPGRADE_PATH);
    fPtr = popen(achCmd, "r");
    if (NULL == fPtr)
    {
        dlog(LOG_ERROR, "fopen error!");
        return nRet;
    }
    fclose(fPtr);
    fPtr = NULL;

    memset(achCmd, 0, sizeof(achCmd));
    snprintf(achCmd, sizeof(achCmd), "chmod 777 %s/*.bin", HTTPS_UPGRADE_PATH);
    fPtr = popen(achCmd, "r");
    if (NULL == fPtr)
    {
        dlog(LOG_ERROR, "fopen error!");
        return nRet;
    }

    nRet = 0;
    fclose(fPtr);
    fPtr = NULL;

    return nRet;
}
