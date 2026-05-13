/**
 * @file http_communicate.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-05-27
 *
 * @brief 运维平台http协议
 */
#ifndef HTTP_COMMUNIACTE_H
#define HTTP_COMMUNIACTE_H

#include "http_send.h"

#define LOGINOPERATEPLATFORM "https://oam.itc-pa.cn/api/v2/user/login"
#define GETUPGRADEPACKURL "https://oam.itc-pa.cn/api/v2/project/software_list"
#define UPLOADUPGRADEPACKURL "https://oam.itc-pa.cn/api/v2/project/download_software"

// 定义数据结构
typedef struct _UpgradeDataItem_
{
    char version[128]; /* 版本号 */
    char url[128];     /* 下载链接 */
    int id;            /* ID */
} UpgradeDataItem_S;

/* 升级包类型 */
typedef enum _UpgradePackType_
{
    ALLPACK = 0,      /* 软件包和固件包 */
    SOFTWAREPACK = 1, /* 软件包 */
    FIRMWAREPACK = 2, /* 固件包 */
} UpgradePackType_E;

/**
 * @brief  获取升级包信息列表
 * @param  [char*] pUsername - 运维平台账号
 * @param  [char*] pPwd - 运维平台密码
 * @param  [int*] pDataSize - 升级包个数
 * @param  [int] nType - 升级包类型
 * @param  [int] nPageId - 请求获取第几页的升级包列表
 * @param  [char*] pKeyWord - 搜索关键字，项目型号
 * @param  [UpgradeDataItem_S*] upgradeData - 升级包信息列表
 * @return [int] 请求结果： 成功返回0，失败返回错误码
 * @author lixl
 * @note
 */
int get_upgradePackInfoCopy(const char *pUsername, const char *pPwd, const char *pKeyWord, const int nType, const int nPageId, UpgradeDataItem_S *pUpgradeData, int *pDataSize);

/**
 * @brief  获取升级包下载路径
 * @param  [char*] pUsername - 运维平台账号
 * @param  [char*] pPwd - 运维平台密码
 * @param  [int] nId - 升级包id
 * @param  [char*] pStoragePath - 升级包下载路径
 * @return [int] 请求结果： 成功返回0，失败返回错误码
 * @author lixl
 * @note
 */
int get_upgradePack(const char *pUsername, const char *pPwd, const int nId, const char *pStoragePath, const char *pFileName);

/**
 * @brief  获取升级包下载路径
 * @param  [char*] pUsername - 运维平台账号
 * @param  [char*] pPwd - 运维平台密码
 * @param  [UpgradeDataItem_S*] pUpgradeData - 升级包信息列表
 * @param  [int] nType - 升级包类型
 * @param  [char*] pVersion - 当前版本号
 * @param  [char*] pKeyWord - 搜索关键字，项目型号
 * @return [int] 请求结果： 成功返回0，失败返回错误码
 * @author lixl
 * @note
 */
int get_autoUpdate(const char *pUsername, const char *pPwd, const char *pKeyWord, const int nType, const char *pVersion, UpgradeDataItem_S *pUpgradeData, int *pDataSize);

int set_LoginTocken(const char *pLogTocken);
/**
 * @brief 获取最新版本号和id
 * @param pUpgrade 升级包信息列表
 * @param num_items 数组大小
 * @param latest_version 最新版本
 * @param latest_id 最新版本id
 * @return true
 * @return false
 */
void get_latestVersion(const UpgradeDataItem_S *pUpgrade, int num_items, char *latest_version, int *latest_id, char *url);

int upload_file(const char *strUrl, const char *savePath);

#endif