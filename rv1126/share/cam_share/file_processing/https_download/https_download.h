/*** 
 * @FilePath     : https_download.h
 * @Author       : huangjunda
 * @Date         : 2024-08-16 15:13:49
 * @LastEditors  : huangjunda
 * @LastEditTime : 2024-11-01 09:25:51
 * @Description  : 
 */

#ifndef __HTTPS_DOWNLOAD_H__
#define __HTTPS_DOWNLOAD_H__

#ifdef __cplusplus
extern "C"{
#endif

#include <stdio.h>
#include <curl/curl.h>

#define HTTPS_SERTIFICATES_LINK "https://curl.se/ca/cacert.pem"
#define HTTPS_SERTIFICATES_PATH "/opt/cam/certificate"
#define HTTPS_UPGRADE_PATH "/data/upgrade"

#define CHECK_INTERVAL 30 * 24 * 60 * 60 // 每 30 天检查一次

/*** 
 * @description : 检查证书是否有效的线程函数
 * @author      : huangjunda
 * @param        {void} *arg
 * @return       {*}
 */
void *cert_check_thread(void *arg);

/*** 
 * @description : 用于写入数据的回调函数
 * @author      : huangjunda
 * @param        {void} *ptr
 * @param        {size_t} size
 * @param        {size_t} nmemb
 * @param        {FILE} *stream
 * @return       {*}
 */
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);

/*** 
 * @description : 下载CA证书
 * @author      : huangjunda
 * @return       {*}
 */
int download_https_cacert();

/*** 
 * @description : 检查证书是否有效
 * @author      : huangjunda
 * @param        {char} *achCertFilePath
 * @return       {*}
 */
int is_certificate_valid();

/*** 
 * @description : 下载HTTPS链接的文件
 * @author      : huangjunda
 * @param        {char} *pUrl
 * @param        {char} *pFileName
 * @return       {*}
 */
int download_https_upgrade_package(const char *pUrl, const char *pFileName);

/*** 
 * @description : 检查升级包
 * @author      : huangjunda
 * @return       {*}
 */
int check_package();

#ifdef __cplusplus
}
#endif
#endif // __HTTPS_DOWNLOAD_H__