/*
 * @FilePath     : upgrade_define.h
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-08-26 11:29:57
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2025-02-18 09:31:14
 * @Description  :
 */
#ifndef _UPGRADE_COMMON_H_
#define _UPGRADE_COMMON_H_

#include <pthread.h>
#include "share_define.h"

#define SYSCMDLEN (1024)

#define TMP_STATUS    "/tmp/upgrade_tmp"

#define UPGRADE_PATH        "/opt/cam"
#define UPGRADE_BAK         "/opt/course"
#define UPGRADE_SUFFIX_PATH "/opt/course"
#define UPGRADE_BAK_DIR     "/opt/course/cam"
#define TI_UPGRADE_TAR      "/opt/course/update.tar.gz"

/* 升级状态 */
typedef enum _TiUpgradeRuslut_
{
    TI_UPGRADE_NULL              = -1, /*未升级*/
    TI_UPGRADE_RUNING            = 0,  /*升级中*/
    TI_UPGRADE_RUNFAIL           = 1,  /*升级失败*/
    TI_UPGRADE_RUNSUCCESS        = 2,  /*升级成功*/
    TI_UPGRADE_OTHERUPDATE_FAIL  = 3,  /*其他服务器升级失败*/
} TiUpgradeRuslut_E;

typedef struct
{
    char  achPath[1024]; 	/* 升级路径 */
} UpgradeInfo_S;

typedef struct
{
    pthread_mutex_t   mutex;
    TiUpgradeRuslut_E enStatus;
} Upgrade_Handle_t;

#pragma pack(1)

typedef struct package
{
    char          id[4];          // 用于标识正常的升级文件
    char          version[48];    // 用于标识版本号
    char          md5[33];        // md5加密字符串
    long long int len;
    int           reserves;
} Upgrade_Package_t;

#pragma pack()

#endif