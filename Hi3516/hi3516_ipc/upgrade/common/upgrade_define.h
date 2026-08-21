/**
 * @FilePath     : upgrade_define.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-08 10:24:46
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-04 08:45:53
 * @Description  : 升级模块公共定义
 */

#ifndef _UPGRADE_COMMON_H_
#define _UPGRADE_COMMON_H_

#include <pthread.h>
#include "share_define.h"

#define SYSCMDLEN (1024)

/* 流式读取缓冲区大小，用于 MD5 校验和管道解压，避免一次性占用大量内存 */
#define READ_BUF_SIZE 16384  /* 16KB */

/* 系统升级包解压目标路径（根目录） */
#define PATH_INSTALL_ROOT "/"

#define TMP_STATUS    "/tmp/upgrade_tmp"

/* 运行期间的临时上传目录，升级失败或成功后只清理该目录内容 */
#define UPLOAD_RUNTIME_PATH "/tmp/upload"

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
