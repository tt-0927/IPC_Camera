/*
 * @Author       : suzhl
 * @Date         : 2024-08-12 17:16:58
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-07-15 19:30:24
 * @FilePath     : businessParse.c
 * @Description  : 业务解析
 */
#include <string.h>
#include "businessParse.h"
#include "cJSON.h"
#include "dlog.h"

int parse_getPackInfo(const char *pMessage, int *nType, int *nPageId, char *pKeyWord, const int nLen)
{
    int nRet = -1;
    cJSON *pNodeData = NULL;
    cJSON *pChildNode = NULL;
    cJSON *pItem = NULL;
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
    /* Data */
    pChildNode = cJSON_GetObjectItem(pNodeData, "Data");
    if (NULL == pChildNode)
    {
       dlog_error("获取节点[Data]信息失败");
        goto EXIT;
    }
    pItem = cJSON_GetObjectItem(pChildNode, "type");
    if (NULL == pChildNode)
    {
       dlog_error("获取节点[type]信息失败");
        goto EXIT;
    }
    *nType = pItem->valueint;
    pItem = cJSON_GetObjectItem(pChildNode, "pageid");
    if (NULL == pChildNode)
    {
       dlog_error("获取节点[type]信息失败");
        goto EXIT;
    }
    *nPageId = pItem->valueint;
    pItem = cJSON_GetObjectItem(pChildNode, "keyword");
    if (NULL == pItem ||
        NULL == pItem->valuestring)
    {
       dlog_error("获取节点[keyword]信息失败");
        goto EXIT;
    }
    strncpy(pKeyWord, pItem->valuestring, nLen);
    nRet = 0;
EXIT:
    if (pNodeData)
    {
        cJSON_Delete(pNodeData);
        pNodeData = NULL;
    }
    return nRet;
}

int parse_uploadPack(const char *pMessage, int *nId, char *pFilePath, const int nLen)
{
    int nRet = -1;
    cJSON *pNodeData = NULL;
    cJSON *pChildNode = NULL;
    cJSON *pItem = NULL;
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
    /* Data */
    pChildNode = cJSON_GetObjectItem(pNodeData, "Data");
    if (NULL == pChildNode)
    {
       dlog_error("获取节点[Data]信息失败");
        goto EXIT;
    }
    pItem = cJSON_GetObjectItem(pChildNode, "id");
    if (NULL == pItem)
    {
       dlog_error("获取节点[id]信息失败");
        goto EXIT;
    }
   dlog_debug("解析出来的id:%d", pItem->valueint);
    *nId = pItem->valueint;
    pItem = cJSON_GetObjectItem(pChildNode, "storage_path");
    if (NULL == pItem ||
        NULL == pItem->valuestring)
    {
       dlog_error("获取节点[storage_path]信息失败");
        goto EXIT;
    }
    strncpy(pFilePath, pItem->valuestring, nLen);
    nRet = 0;
EXIT:
    if (pNodeData)
    {
        cJSON_Delete(pNodeData);
        pNodeData = NULL;
    }
    return nRet;
}

int parse_autoUpdate(const char *pMessage, int *nType, char *pVersion, char *pKeyWord, const int nVersionSize, const int nKeyWordSize)
{
    int nRet = -1;
    cJSON *pNodeData = NULL;
    cJSON *pChildNode = NULL;
    cJSON *pItem = NULL;
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
    /* Data */
    pChildNode = cJSON_GetObjectItem(pNodeData, "Data");
    if (NULL == pChildNode)
    {
       dlog_error("获取节点[Data]信息失败");
        goto EXIT;
    }
    pItem = cJSON_GetObjectItem(pChildNode, "type");
    if (NULL == pChildNode)
    {
       dlog_error("获取节点[type]信息失败");
        goto EXIT;
    }
    *nType = pItem->valueint;
    pItem = cJSON_GetObjectItem(pChildNode, "version");
    if (NULL == pChildNode)
    {
       dlog_error("获取节点[version]信息失败");
        goto EXIT;
    }
    strncpy(pVersion, pItem->valuestring, nVersionSize);
    pItem = cJSON_GetObjectItem(pChildNode, "keyword");
    if (NULL == pItem ||
        NULL == pItem->valuestring)
    {
       dlog_error("获取节点[keyword]信息失败");
        goto EXIT;
    }
    strncpy(pKeyWord, pItem->valuestring, nKeyWordSize);
    nRet = 0;
EXIT:
    if (pNodeData)
    {
        cJSON_Delete(pNodeData);
        pNodeData = NULL;
    }
    return nRet;
}