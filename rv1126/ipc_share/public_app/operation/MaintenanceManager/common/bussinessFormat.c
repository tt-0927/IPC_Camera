/*
 * @Author       : suzhl
 * @Date         : 2024-08-12 17:16:58
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-07-15 19:32:29
 * @FilePath     : bussinessFormat.c
 * @Description  : 业务封装
 */
#include "bussinessFormat.h"
#include "dlog.h"

cJSON *return_upgradePackInfoCopy(UpgradeDataItem_S *pUpgradeData, int nDataSize)
{
    // 创建根对象
    cJSON *pRootJson = cJSON_CreateObject();
    if (pRootJson == NULL)
    {
        return NULL;
    }

    // 创建 data 数组
    cJSON *pDataArray = cJSON_CreateArray();
    if (pDataArray == NULL)
    {
        cJSON_Delete(pRootJson);
        return NULL;
    }
    cJSON_AddItemToObject(pRootJson, "data", pDataArray);
    dlog_info("nDataSize:%d", nDataSize);
    // 遍历结构体数组，将每个结构体的内容加入到 JSON 数组中
    for (int i = 0; i < nDataSize; i++)
    {
        cJSON *pDataItem = cJSON_CreateObject();
        if (pDataItem == NULL)
        {
            cJSON_Delete(pRootJson);
            return NULL;
        }
        cJSON_AddStringToObject(pDataItem, "version", pUpgradeData[i].version);
        cJSON_AddNumberToObject(pDataItem, "id", pUpgradeData[i].id);
        cJSON_AddItemToArray(pDataArray, pDataItem);
    }
    return pRootJson;
}

cJSON *return_autoUpdateVersion(const char *pVersion, const int nId)
{
    cJSON *pRootJson = NULL;
    pRootJson = cJSON_CreateObject();
    cJSON_AddStringToObject(pRootJson, "version", pVersion);
    cJSON_AddNumberToObject(pRootJson, "id", nId);
    return pRootJson;
}

cJSON *return_upgradePackUrl(int nIsDown)
{
    cJSON *pRootJson = NULL;
    pRootJson = cJSON_CreateObject();
    cJSON_AddNumberToObject(pRootJson, "is_down", nIsDown);
    return pRootJson;
}

cJSON *return_upgradeProgress(int nProgress, const char *pFilePath)
{
    cJSON *pRootJson = NULL;
    pRootJson = cJSON_CreateObject();
    cJSON_AddNumberToObject(pRootJson, "progress", nProgress);
    cJSON_AddStringToObject(pRootJson, "file_path", pFilePath);
    return pRootJson;
}