/*
 * @Author       : suzhl
 * @Date         : 2024-08-12 17:16:58
 * @LastEditors  : EasonLu
 * @LastEditTime :
 * @FilePath     : bussinessFormat.h
 * @Description  : 业务封装
 */
#include "cJSON.h"
#include "http_communicate.h"

/**
 * @brief  封装升级包列表
 * @param  [UpgradeDataItem_S*] pUpgradeData - 升级包列表结构体指针
 * @param  [int] nDataSize - 升级包个数
 * @return [cJSON] 请求结果： 成功返回cJson数据，失败返回NULL
 * @author lixl
 * @note
 */
cJSON *return_upgradePackInfoCopy(UpgradeDataItem_S *pUpgradeData, int nDataSize);

/**
 * @brief  封装升级包是否下载成功
 * @param  [int] nIsDown - 成功标识
 * @return [cJSON] 请求结果： 成功返回cJson数据，失败返回NULL
 * @author lixl
 * @note
 */
cJSON *return_upgradePackUrl(int nIsDown);

/**
 * @brief  封装升级包下载进度
 * @param  [int] nProgress - 下载进度
 * @param  [int] pFilePath - 文件存储位置
 * @return [cJSON] 请求结果： 成功返回cJson数据，失败返回NULL
 * @author lixl
 * @note
 */
cJSON *return_upgradeProgress(int nProgress, const char *pFilePath);

/**
 * @brief  封装最新升级包
 * @param  [char*] pVersion - 最新包版本
 * @param  [int] nId - 最新包ID
 * @return [cJSON] 请求结果： 成功返回cJson数据，失败返回NULL
 * @author lixl
 * @note
 */
cJSON *return_autoUpdateVersion(const char *pVersion, const int nId);
