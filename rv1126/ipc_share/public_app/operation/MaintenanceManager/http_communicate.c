/**
 * @file http_communicate.c
 * @author tianl (tianl@kfb.cn)
 * @date 2025-05-27
 *
 * @brief 运维平台http协议
 */
#include <unistd.h>
#include "http_communicate.h"
#include "curl.h"
#include <sys/statvfs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <sys/file.h>
#include <errno.h>
#include "bl_event.h"
#include "bussinessFormat.h"
#include "dlog.h"

typedef struct
{
    double dltotal;      // 总下载大小
    double dlnow;        // 当前已下载大小
    int percentage_next; // 下载进度百分比
    int percentage;      // 下载进度百分比
    int done;            // 下载是否完成的标志
} DownloadProgress;

DownloadProgress g_progress = {0, 0, 0, 0}; // 初始化进度结构体

/* 存放会话标识 */
static char m_pchApiToken[128] = {0};
/* 存放管理员用户名 */
static char m_achUsername[32] = {0};
/* 存放管理员密码 */
static char m_achPasswd[32] = {0};
/* 存放会话标识 */
static char m_pchFilePath[1024] = {0};

size_t g_nUrlFileSize = 0; // 全局变量存储文件大小

/* 发送运维平台通用接口(http) */
int send_operatePlatform_with_http(const char *pHttpUrl, const int nType, const char *pToken, const cJSON *pSendData, char **pRecvMessage)
{
    int nRet = -1;
    if (NULL == pHttpUrl || NULL == pSendData)
    {
        return nRet;
    }
    char *pJsonBuf = NULL;
    pJsonBuf = cJSON_Print(pSendData);
    char *pOutJson = NULL;
    dlog_debug("发送给运维平台消息:%s", pJsonBuf);
    nRet = http_send(pHttpUrl, nType, pToken, pJsonBuf, &pOutJson);
    *pRecvMessage = pOutJson;
    if (pJsonBuf)
    {
        free(pJsonBuf);
        pJsonBuf = NULL;
    }
    if (pSendData)
    {
        cJSON_Delete(pSendData);
        pSendData = NULL;
    }
    return nRet;
}

/* 基础解析接口 */
int parse_operatePlatformBaseInfo(const char *pMessage, int *nResult, char **pData)
{
    int nRet = -1;
    cJSON *pNodeData = NULL;
    cJSON *pChildNode = NULL;
    if (NULL == pMessage)
    {
        return nRet;
    }
    /*创建操作句柄*/
    pNodeData = cJSON_Parse(pMessage);
    if (NULL == pNodeData)
    {
        dlog_error("传入的Json字符串有问题, 无法创建句柄");
        goto EXIT;
    }
    pChildNode = cJSON_GetObjectItem(pNodeData, "errcode");
    if (pChildNode)
    {
        nRet = pChildNode->valueint;
        goto EXIT;
    }

    pChildNode = cJSON_GetObjectItem(pNodeData, "result");
    if (NULL == pChildNode || (NULL != pChildNode && pChildNode->valueint != 200))
    {
        dlog_warn("接口请求返回失败了");
        goto EXIT;
    }
    else
    {
        *nResult = pChildNode->valueint;
    }

    /* 解析data */
    pChildNode = cJSON_GetObjectItem(pNodeData, "data");
    if (NULL == pChildNode)
    {
        dlog_error("获取【data】异常");
        goto EXIT;
    }
    *pData = cJSON_Print(pChildNode);
    nRet = 0;
EXIT:
    if (pNodeData)
    {
        cJSON_Delete(pNodeData);
        pNodeData = NULL;
    }
    return nRet;
}

/* 解析获取token过期错误码 */
int parse_tokenErrorCode(const char *pMessage, int *nErrorCode)
{
    int nRet = -1;
    cJSON *pNodeData = NULL;
    cJSON *pChildNode = NULL;
    if (NULL == pMessage)
    {
        dlog_error("传入参数异常");
        return nRet;
    }
    /*创建操作句柄*/
    pNodeData = cJSON_Parse(pMessage);
    if (NULL == pNodeData)
    {
        dlog_error("传入的Json字符串有问题, 无法创建句柄");
        goto EXIT;
    }
    /* token */
    pChildNode = cJSON_GetObjectItem(pNodeData, "errcode");
    if (NULL == pChildNode ||
        NULL == pChildNode->valueint)
    {
        goto EXIT;
    }
    *nErrorCode = pChildNode->valueint;
    nRet = 0;
EXIT:
    if (pNodeData)
    {
        cJSON_Delete(pNodeData);
        pNodeData = NULL;
    }
    return nRet;
}

/* 校验token是否过期 */
bool verify_apiToken(const char *pMessage)
{
    int nRet = -1;
    int nErrorCode = 0;
    nRet = parse_tokenErrorCode(pMessage, &nErrorCode);
    if (nRet == 0 && nErrorCode == 4001)
    {
        return true;
    }
    return false;
}

int parse_upgradePackInfoCopy(const char *pData, UpgradeDataItem_S *pUpgradeData, int *pDataSize)
{
    int nRet = -1;
    cJSON *pNodeData = NULL;
    cJSON *pArray = cJSON_CreateArray();
    cJSON *pChildData = NULL;
    cJSON *pItem = NULL;
    if (NULL == pData)
    {
        dlog_error("传入参数异常");
        return nRet;
    }
    /*创建操作句柄*/
    pNodeData = cJSON_Parse(pData);
    if (NULL == pNodeData)
    {
        dlog_error("传入的Json字符串有问题, 无法创建句柄");
        goto EXIT;
    }
    pArray = cJSON_GetObjectItem(pNodeData, "data");
    if (NULL == pArray)
    {
        dlog_error("获取节点[data]信息失败");
        goto EXIT;
    }

    int size = cJSON_GetArraySize(pArray);
    dlog_info("----------------------------");

    *pDataSize = size;
    dlog_info("size: %d", size);
    dlog_info("pArray: %s", cJSON_Print(pArray));
    for (int i = 0; i < size; i++)
    {
        pChildData = cJSON_GetArrayItem(pArray, i);
        if (NULL == pChildData)
        {
            dlog_error("获取节点信息失败");
            goto EXIT;
        }
        pItem = cJSON_GetObjectItem(pChildData, "id");
        if (NULL == pItem)
        {
            dlog_error("获取节点[id]信息失败");
            goto EXIT;
        }
        pUpgradeData[i].id = pItem->valueint;

        dlog_info("pUpgradeData[i].id:%d, i= %d", pUpgradeData[i].id, i);

        pItem = cJSON_GetObjectItem(pChildData, "version");
        if (NULL == pItem || NULL == pItem->valuestring)
        {
            dlog_error("获取节点[version]信息失败");
            goto EXIT;
        }

        strncpy(pUpgradeData[i].version, pItem->valuestring, sizeof(pUpgradeData[i].version) - 1);
        dlog_info("pUpgradeData[i].version:%s, i= %d", pUpgradeData[i].version, i);

        pItem = cJSON_GetObjectItem(pChildData, "file_path");
        if (NULL == pItem || NULL == pItem->valuestring)
        {
            dlog_error("获取节点[file_path]信息失败");
            goto EXIT;
        }
        strncpy(pUpgradeData[i].url, pItem->valuestring, sizeof(pUpgradeData[i].url) - 1);
        dlog_info("pUpgradeData[i].file_path:%s, i= %d", pUpgradeData[i].url, i);
    }

    nRet = 0;
EXIT:
    if (pNodeData)
    {
        cJSON_Delete(pNodeData);
        pNodeData = NULL;
    }
    return nRet;
}

int get_upgradePackInfoCopy(const char *pUsername, const char *pPwd, const char *pKeyWord, const int nType, int nPageId, UpgradeDataItem_S *pUpgradeData, int *pDataSize)
{
    int nRet = -1;
    char *pRecvMessage = NULL;
    int nResult = 0;
    char *pData = NULL;
    cJSON *pRootJson = NULL;
    /* 请求运维平台获取最新升级包 */
    pRootJson = cJSON_CreateObject();
    cJSON_AddNumberToObject(pRootJson, "type", nType);       /* 默认获取软件包 */
    cJSON_AddStringToObject(pRootJson, "keyword", pKeyWord); /* 搜索关键字默认传项目型号 */
    if (nPageId <= 0)
    {
        nPageId = 1;
    }
    cJSON_AddNumberToObject(pRootJson, "pageid", nPageId); /* 分页：默认第一页 */

    dlog_info("GETUPGRADEPACKURL获取token为：%s", m_pchApiToken);

    dlog_info("GETUPGRADEPACKURL获取token为：: %s\n", m_pchApiToken);

    if (m_pchApiToken[0] == "\0")
    {
        dlog_info("GETUPGRADEPACKURL获取token为空");
        return -1002;
    }

    nRet = send_operatePlatform_with_http(GETUPGRADEPACKURL, 1, m_pchApiToken, pRootJson, &pRecvMessage);
    if (0 != nRet)
    {
        dlog_error("请求获取安装包信息失败");
        return nRet;
    }
    dlog_debug("收到运维平台返回的获取升级包信息结果:%s", pRecvMessage);
    /* 校验token是否过期 */
    if (true == verify_apiToken(pRecvMessage))
    {
        return -1002;
    }

    nRet = parse_operatePlatformBaseInfo(pRecvMessage, &nResult, &pData);

    if (nRet == 0 && nResult != 200)
    {
        dlog_error("获取安装包信息失败");
        return nRet;
    }
    /* 解析获取软件包id及版本号 */
    nRet = parse_upgradePackInfoCopy(pData, pUpgradeData, pDataSize);
    if (0 != nRet)
    {
        dlog_error("获取软件包信息失败");
        return nRet;
    }
    return nRet;
}

/* 解析获得升级包下载链接 */
int parse_downUrl(const char *pData, char *pDownUrl, const int nLen)
{
    int nRet = -1;
    cJSON *pNodeData = NULL;
    cJSON *pChildNode = NULL;
    if (NULL == pData)
    {
        dlog_error("传入参数异常");
        return nRet;
    }
    /*创建操作句柄*/
    pNodeData = cJSON_Parse(pData);
    if (NULL == pNodeData)
    {
        dlog_error("传入的Json字符串有问题, 无法创建句柄");
        goto EXIT;
    }
    /* token */
    pChildNode = cJSON_GetObjectItem(pNodeData, "download_url");
    if (NULL == pChildNode ||
        NULL == pChildNode->valuestring)
    {
        dlog_error("获取节点[download_url]信息失败");
        goto EXIT;
    }
    strncpy(pDownUrl, pChildNode->valuestring, nLen);
    nRet = 0;
EXIT:
    if (pNodeData)
    {
        cJSON_Delete(pNodeData);
        pNodeData = NULL;
    }
    return nRet;
}

// 回调函数，用于写入下载的数据
size_t write_callback(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

// 进度回调函数
int progress_callback(void *ptr, double dltotal, double dlnow, double ultotal, double ulnow)
{
    // 更新进度
    g_progress.dltotal = dltotal;
    g_progress.dlnow = dlnow;
    g_progress.percentage_next = g_progress.percentage;
    g_progress.percentage = (int)((dlnow / dltotal) * 100);
    cJSON *pData = NULL;

    if (g_progress.percentage > g_progress.percentage_next)
    {
        dlog_debug("下载文件:[%s] 当前进度:[%d]", m_pchFilePath, g_progress.percentage);
        // pData = return_upgradeProgress(g_progress.percentage, m_pchFilePath);
    }

    // 如果下载完成，设置标志
    if (dlnow >= dltotal)
    {
        g_progress.done = 1;
    }

    return 0;
}

// 判断磁盘容量与文件大小关系
int isDiskSpaceGreater(const char *path)
{
    struct statvfs stat;

    // 获取指定路径的文件系统信息
    if (statvfs(path, &stat) != 0)
    {
        perror("statvfs");
        return -1003;
    }

    // 计算磁盘总容量 (单位为字节)
    unsigned long total_space = stat.f_blocks * stat.f_frsize;

    // 判断是否大于50MB
    if (total_space > g_nUrlFileSize)
    {
        return 0;
    }
    else
    {
        return -1003;
    }
}

size_t header_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t total_size = size * nmemb;
    if (strstr(ptr, "Content-Length"))
    {
        // 提取 Content-Length 中的文件大小
        sscanf(ptr, "Content-Length: %d", &g_nUrlFileSize); // 提取文件大小
        dlog_info("文件大小: %ld bytes\n", g_nUrlFileSize);   // 打印文件大小
    }
    return total_size;
}

// 获取文件大小（通过HTTP HEAD请求获取）
int get_remote_file_size(const char *pUrl)
{
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl)
    {
        // 设置URL
        curl_easy_setopt(curl, CURLOPT_URL, pUrl);

        // 设置为HEAD请求，只获取响应头
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // 只请求头部信息，避免下载文件

        // 设置获取响应头回调函数
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);

        // 跳过 SSL 证书验证（仅用于测试，生产环境中不推荐）
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // 不验证证书
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // 不验证主机

        // 设置userdata为NULL或可以传递其他数据
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &g_nUrlFileSize);

        // 执行请求
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            dlog_error("请求失败: %s\n", curl_easy_strerror(res));
            return -1;
        }

        // 打印获取到的文件大小
        dlog_info("文件大小: %zu bytes\n", g_nUrlFileSize);

        // 清理资源
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return 0;
}

// 上传文件的函数
int upload_file(const char *strUrl, const char *savePath)
{
    int nRet = -1;
    dlog_debug("下载链接:%s 保存目录：%s", strUrl, savePath);
    // 获取文件大小
    // nRet = get_remote_file_size(strUrl);
    // if (nRet != 0) {
    //    dlog_error( "无法获取文件大小\n");
    //    return nRet;
    //}
    //
    char cPathCopy[1024];        // 创建一个复制的路径
    strcpy(cPathCopy, savePath); // 复制路径到新的字符串
                                 //
    // char *pDir = dirname(savePath);  // 获取目录部分
    // nRet = isDiskSpaceGreater(pDir);
    // if(nRet != 0)
    //{
    //    dlog_error("空间不足");
    //    return nRet;
    //}

    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init(); // 初始化libcurl
    curl_version_info_data *version_info = curl_version_info(CURLVERSION_NOW);
    if (!(version_info->features & CURL_VERSION_SSL))
    {
        dlog_debug("错误：libcurl 编译时未启用 SSL/TLS 支持！\n");
    }
    if (curl)
    {
        FILE *pOutputFile = fopen(cPathCopy, "wb"); // 打开要保存的文件流
        if (pOutputFile)
        {
            curl_easy_setopt(curl, CURLOPT_URL, strUrl);                   // 设置下载的URL
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback); // 设置写入数据的回调函数
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, pOutputFile);        // 设置写入数据的文件流

            // 设置下载进度回调函数
            curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
            curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, NULL);

            /* 低速限制设置为 30 字节/秒 */
            curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 30L);
            /* 如果低于30字节/秒，超出这个秒数断开 */
            curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 10L);

            // 启用进度信息
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

            // 启用调试输出，帮助你调试问题
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

            // 跳过 SSL 证书验证（仅用于测试，生产环境中不推荐）
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // 不验证证书
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // 不验证主机

            CURLcode res = curl_easy_perform(curl); // 执行下载操作
            if (res != CURLE_OK)
            {
                dlog_error("下载文件失败, 失败原因: %s\n", curl_easy_strerror(res));
                fclose(pOutputFile);
                curl_easy_cleanup(curl); // 清理libcurl资源
                return -1;
            }
            fclose(pOutputFile);
        }
        else
        {
            dlog_error("打开文件失败\n");
            curl_easy_cleanup(curl); // 清理libcurl资源
            curl_global_cleanup();
            return -1;
        }
        curl_easy_cleanup(curl); // 清理libcurl资源
    }
    else
    {
        dlog_error("初始化libcurl失败\n");
    }
    // 全局清理 libcurl
    curl_global_cleanup();
    nRet = 0;
    return nRet;
}

void extractFileName(const char *url, char *pFileName)
{
    // 找到最后一个 '/' 出现的位置
    const char *lastSlash = strrchr(url, '/');
    if (lastSlash != NULL)
    {
        // 从 '/' 后面开始复制文件名
        strcpy(pFileName, lastSlash + 1);
    }
    else
    {
        // 如果没有 '/'，返回空字符串
        pFileName[0] = '\0';
    }
}

int judge_fileExist(const char *pFilePath)
{
    if (access(pFilePath, F_OK) == 0)
    {
        // 文件存在
        return 1;
    }
    // 文件不存在
    return 0;
}

int delete_file(const char *pFilePath)
{
    int nRet = -1;
    // 最大重试次数
    int nRetryCount = 5;
    // 没过两秒获取一次文件锁
    int nRetryInterval = 2;

    // 判断文件是否存在
    if (!judge_fileExist(pFilePath))
    {
        dlog_error("文件:%s不存在无法删除\n", pFilePath);
        return nRet;
    }

    // 尝试打开文件以获取文件锁
    int fd = open(pFilePath, O_RDONLY);
    if (fd == -1)
    {
        dlog_error("打开文件:%s失败\n", pFilePath);
        return nRet;
    }

    // 判断文件是否被其他程序占用
    while (flock(fd, LOCK_EX | LOCK_NB) == -1)
    {
        if (errno == EWOULDBLOCK)
        {
            // 文件被其他程序占用，等待重试
            if (nRetryCount-- > 0)
            {
                dlog_info("文件:%s被占用，等待 %d 秒后重试...\n", pFilePath, nRetryInterval);
                sleep(nRetryInterval); // 等待指定时间后重试
            }
            else
            {
                dlog_error("文件:%s长时间被占用，放弃删除\n", pFilePath);
                close(fd);
                return nRet;
            }
        }
        else
        {
            dlog_error("获取文件锁失败，文件:%s 错误:%d\n", pFilePath, errno);
            close(fd);
            return nRet;
        }
    }
    // 删除文件
    if (remove(pFilePath) == 0)
    {
        nRet = 0;
        dlog_info("文件:%s删除成功\n", pFilePath);
    }
    else
    {
        dlog_info("文件:%s删除失败\n", pFilePath);
    }

    // 释放文件锁
    flock(fd, LOCK_UN);
    close(fd);

    return nRet;
}

int get_upgradePack(const char *pUsername, const char *pPwd, const int nId, const char *pStoragePath, const char *pFileName)
{
    int nRet = -1;
    // 最大重下次数
    int nCount = 0;
    int nResult = 0;
    // 运维json返回的data数据
    char *pData = NULL;
    // 请求运维json
    cJSON *pRootJson = NULL;
    // 运维json返回整体数据
    char *pRecvMessage = NULL;
    char achPath[256];
    if (nId <= 0)
    {
        dlog_error("请求参数类型错误");
        return nRet;
    }
    /* 请求运维平台获取最新升级包下载链接 */
    pRootJson = cJSON_CreateObject();
    cJSON_AddStringToObject(pRootJson, "type", "download");
    cJSON_AddNumberToObject(pRootJson, "id", nId);
    cJSON_AddNumberToObject(pRootJson, "style", 1);
    nRet = send_operatePlatform_with_http(UPLOADUPGRADEPACKURL, 1, m_pchApiToken, pRootJson, &pRecvMessage);
    if (0 != nRet)
    {
        dlog_error("请求获取安装包下载路径失败");
        return nRet;
    }
    dlog_debug("收到运维平台返回的获取升级包下载路径结果:%s", pRecvMessage);
    /* 校验token是否过期 */
    if (true == verify_apiToken(pRecvMessage))
    {
        return -1002;
    }
    nRet = parse_operatePlatformBaseInfo(pRecvMessage, &nResult, &pData);
    if (nRet == 0 && nResult != 200)
    {
        dlog_error("获取安装包下载路径失败");
        return nRet;
    }
    char achUrl[256] = {0};
    nRet = parse_downUrl(pData, achUrl, sizeof(achUrl) - 1);
    if (0 != nRet)
    {
        dlog_error("获取安装包下载路径失败");
        return nRet;
    }
    dlog_debug("安装包下载路径：%s", achUrl);
    char fileName[256]; // 假设文件名最大长度为 255
    extractFileName(achUrl, fileName);
    if (pStoragePath[strlen(pStoragePath) - 1] != '/')
    {
        strcat(pStoragePath, "/"); // 如果路径末尾没有斜杠，则添加一个斜杠
    }
    dlog_debug("存储路径:%s 文件名称:%s", pStoragePath, fileName);
    strcpy(achPath, pStoragePath); // 复制基础路径
    strcat(achPath, fileName);     // 追加文件名

    strcpy(pFileName, achPath);

    strncpy(m_pchFilePath, achPath, sizeof(m_pchFilePath) - 1);
    dlog_debug(" 拼接后的文件名称:%s", achPath);
    nRet = upload_file(achUrl, achPath);
    if (nRet == -1003)
    {
        return nRet;
    }
    // 获取下载三次
    while (nRet != 0 && nCount < 3)
    {
        delete_file(achPath);
        nRet = upload_file(achUrl, achPath);
        nCount++;
    }
    return nRet;
}

int get_autoUpdate(const char *pUsername, const char *pPwd, const char *pKeyWord, const int nType, const char *pVersion, UpgradeDataItem_S *pUpgradeData, int *pDataSize)
{
    int nRet = -1;
    // 运维平台返回json数据
    char *pRecvMessage = NULL;
    int nResult = 0;
    // json的data数据
    char *pData = NULL;
    // 最大获取升级包次数
    int nCount = 0;

    if (strlen(m_pchApiToken) <= 0)
    {
        /* token异常，重新登录 */
        // login_maintemamce_communicate();
        return -1;
    }

    while (1)
    {
        cJSON *pRootJson = NULL;
        /* 请求运维平台获取最新升级包 */
        pRootJson = cJSON_CreateObject();
        cJSON_AddNumberToObject(pRootJson, "type", nType);       /* 默认获取软件包 */
        cJSON_AddStringToObject(pRootJson, "keyword", pKeyWord); /* 搜索关键字默认传项目型号 */
        cJSON_AddNumberToObject(pRootJson, "pageid", 1);         /* 分页：默认第一页 */

        dlog_info("GETUPGRADEPACKURL获取token为：%s", m_pchApiToken);
        // 调用http接口
        nRet = send_operatePlatform_with_http(GETUPGRADEPACKURL, 1, m_pchApiToken, pRootJson, &pRecvMessage);
        if (0 != nRet)
        {
            dlog_error("请求获取安装包信息失败");
            return nRet;
        }
        dlog_debug("收到运维平台返回的获取升级包信息结果:%s", pRecvMessage);

        // 调用解析返回数据
        nRet = parse_operatePlatformBaseInfo(pRecvMessage, &nResult, &pData);

        /* token是否过期 */
        if (nRet == 4001)
        {
            /* token异常，重新登录 */
            // login_maintemamce_communicate();
            return -1;
        }

        if (nRet == 0 && nResult != 200)
        {
            dlog_error("获取安装包信息失败");
            return nRet;
        }
        /* 解析获取软件包id及版本号 */
        nRet = parse_upgradePackInfoCopy(pData, pUpgradeData, pDataSize);

        dlog_info("pDataSize: %d", *pDataSize);
        // 获取到最新升级包信息
        if (*pDataSize > 0)
        {
            break;
        }
        // 最多请求1分钟
        if (nCount > 20)
        {
            break;
        }
        nCount++;
        sleep(3);
    }
    // 如果运维有返回则发送control
    if (*pDataSize > 0 && nRet == 0)
    {
        if (strcmp(pVersion, pUpgradeData[0].version) != 0)
        {
            return 1;
        }
    }
    // 超出最大访问次数，返回control错误码
    if (nCount > 20)
    {
        return 2;
    }
    if (0 != nRet)
    {
        dlog_error("获取软件包信息失败");
        return nRet;
    }
    return nRet;
}

int set_LoginTocken(const char *pLogTocken)
{
    // 使用 strncpy 拷贝字符串，确保不会超过数组大小
    strncpy(m_pchApiToken, pLogTocken, sizeof(m_pchApiToken) - 1);

    // 确保字符串以空字符结尾
    m_pchApiToken[sizeof(m_pchApiToken) - 1] = '\0';

    // printf("m_pchApiToken: %s\n", m_pchApiToken);
    dlog_info("set_LoginTocken获取token为：%s", m_pchApiToken);

    return 0;
}

/* 比较两个版本号 */
static int compare_versions(const char *v1, const char *v2)
{
    if (v1 == NULL || v2 == NULL)
    {
        return 0;
    }

    while (1)
    {
        // 跳过非数字字符
        while (*v1 != '\0' && !isdigit((unsigned char)*v1))
        {
            v1++;
        }
        while (*v2 != '\0' && !isdigit((unsigned char)*v2))
        {
            v2++;
        }

        // 转换数字部分
        long num1 = 0, num2 = 0;
        if (isdigit((unsigned char)*v1))
        {
            num1 = strtol(v1, (char **)&v1, 10);
        }
        if (isdigit((unsigned char)*v2))
        {
            num2 = strtol(v2, (char **)&v2, 10);
        }

        if (num1 > num2)
        {
            return 1;
        }
        else if (num1 < num2)
        {
            return -1;
        }

        // 如果两个都处理完毕，结束循环
        if ((*v1 == '\0' || !isdigit((unsigned char)*v1)) &&
            (*v2 == '\0' || !isdigit((unsigned char)*v2)))
        {
            break;
        }
    }

    return 0; // 版本相同
}

void get_latestVersion(const UpgradeDataItem_S *pUpgrade, int num_items, char *latest_version, int *latest_id, char *url)
{
    if (pUpgrade == NULL || num_items <= 0 || latest_version == NULL || latest_id == NULL)
    {
        return;
    }

    int latest_index = 0;
    for (int i = 1; i < num_items; i++)
    {
        if (compare_versions(pUpgrade[i].version, pUpgrade[latest_index].version) > 0)
        {
            latest_index = i;
        }
    }

    // 安全复制版本号
    snprintf(latest_version, 128, "%s", pUpgrade[latest_index].version);
    snprintf(url, 128, "%s", pUpgrade[latest_index].url);
    *latest_id = pUpgrade[latest_index].id;

    return;
}