/**
 * @FilePath     : upgrade_logic.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-08-26 11:22:17
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-10-11 09:38:30
 * @Description  : 升级逻辑模块
 */

#include "upgrade_logic.h"

#include <cstring>
#include <iostream>
#include <sys/stat.h>
#include <utility>
#include <fcntl.h>

#include "upgrade_define.h"
#include "data_manage.h"
#include "dlog.h"
#include "md5lib.h"
#include <unistd.h>

/**
 * @brief 检测升级文件路径是否存在
 * @param [char*] pchPath: 文件路径
 * @return [*] IpcRet_E::OK 成功  其他失败
 * @note
 */
static IpcRet_E check_path_exsit(char *pchPath)
{
    if (NULL == pchPath)
    {
        dlog_error("传入参数为空");
        return ERR_PARAM_NULL;
    }

    int nRet = -1;

    nRet = access(pchPath, F_OK);
    if (0 != nRet)
    {
        dlog_error("升级文件路径不存在 [%s]", pchPath);
        return ERR_NOT_EXIST;
    }

    return OK;
}

/**
 * @brief 解出一个包
 * @param [FILE*] pSrc: 源文件句柄
 * @param [FILE*] pDst: 目标文件句柄
 * @param [Upgrade_Package_t] stUpgradePackInfo: 升级包信息
 * @return [*] IpcRet_E::OK 成功  其他失败
 * @note
 */
static IpcRet_E unpack_upgrade_file(int nSrcFd, int nDstFd, Upgrade_Package_t stUpgradePackInfo)
{
    int32_t nRet = -1;

    int8_t anBuffer[2050] = {0};
    int nWriteSize = 0;
    int nHaveReadSize = 0; // 已经读取了多长个字节

    int nResidueSize = 0; // 剩余多少个字节没读取

    /* 读取内容信息 */
    while (nHaveReadSize < stUpgradePackInfo.len)
    {
        memset(anBuffer, 0x0, 2050);

        nResidueSize = stUpgradePackInfo.len - nHaveReadSize;
        nResidueSize = (nResidueSize > 2050) ? 2050 : nResidueSize;

        nRet = read(nSrcFd, anBuffer, nResidueSize);
        if (nRet <= 0)
        {
            if (0 == nRet)
            {
                dlog_trace("读取到文件尾 [%d]", nHaveReadSize);
                break;
            }

            dlog_error("读取升级包失败");
            return ERR_FREAD;
        }

        nWriteSize = write(nDstFd, (char *)anBuffer, nRet);
        if (nWriteSize != nRet)
        {
            dlog_error("写文件失败 [%d]-[%d]", nWriteSize, nRet);
        }

        /* 累计读取了多少字节 */
        nHaveReadSize += nRet;
    }

    return OK;
}

/**
 * @brief 拆解升级包
 * @param [char*] pchSrcFile: 升级包文件路径
 * @param [int*] nOtherUngradeNum: 有多少个其他升级包
 * @param [Upgrade_Package_t*] pchUpgradePackInfo: 当前系统升级包头信息
 * @return [*] IpcRet_E::OK 成功  其他失败
 * @note
 */
static IpcRet_E unpack_more_upgrade(
    char *pchSrcFile,
    int *nOtherUngradeNum,
    Upgrade_Package_t *pchUpgradePackInfo)
{
    int32_t nRet = -1;
    IpcRet_E enRetCode = OK;
    Upgrade_Package_t stUnpackInfo;
    char achTmpFile[128] = {0};
    int nPackIndex = 0;
    char achCmd[512] = {0};

    int nSourceFd = -1;
    int nTargetFd = -1;

    if (NULL == pchSrcFile ||
        NULL == nOtherUngradeNum)
    {
        dlog_error("传入参数为空");
        return ERR_PARAM_NULL;
    }

    *nOtherUngradeNum = 0;

    /* 打开升级包 */
    nSourceFd = open(pchSrcFile, O_RDONLY);
    if (0 > nSourceFd)
    {
        dlog_error("打开文件失败 [%s]", pchSrcFile);
        return ERR_OPEN;
    }

    /* 函数feod(fp):文件未结束，返回0，结束返回非零值 */
    while ((nRet = read(nSourceFd, (char *)&stUnpackInfo, sizeof(Upgrade_Package_t))) > 0)
    {
        /* 打开新文件 */
        snprintf(achTmpFile, sizeof(achTmpFile), "%s/update_%d.bin", UPGRADE_SUFFIX_PATH, nPackIndex);
        nTargetFd = open(achTmpFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (0 > nTargetFd)
        {
            dlog_error("打开新文件失败 [%s]", achTmpFile);
            return ERR_OPEN;
        }

        dlog_trace("文件头信息 升级包版本[%s] 升级包长度[%lld] 升级包md5[%s]",
                   stUnpackInfo.version, stUnpackInfo.len, stUnpackInfo.md5);

        /* 判断是否是当前系统的升级包，若是，则不写头，直接生成tar包 */
        /* 若不是，则需要写头，生成bin包，送到其他服务器解包升级 */
        if (0 != strcmp(stUnpackInfo.id, UPGRADE_ID))
        {
            int nWriteSize = 0;
            dlog_error("当前升级包不是本系统升级包 ID[%s]", stUnpackInfo.id);
            nWriteSize = write(nTargetFd, (char *)&stUnpackInfo, sizeof(Upgrade_Package_t));
            if (nWriteSize < 0)
            {
                dlog_error("写头信息失败");
                return ERR_FWRITE;
            }
        }

        /* 逐个解包 */
        enRetCode = unpack_upgrade_file(nSourceFd, nTargetFd, stUnpackInfo);
        /* 关闭目标文件 */
        close(nTargetFd);
        nTargetFd = -1;

        if (enRetCode < OK)
        {
            memset(achCmd, 0, sizeof(achCmd));
            snprintf(achCmd, sizeof(achCmd), "rm -f '%s'", achTmpFile);
            if (system(achCmd) != 0)
            {
                dlog_error("执行命令[%s]失败", achCmd);
            }
            continue;
        }

        dlog_trace("解析包成功 index[%d] 文件[%s] id[%s] md5[%s] version[%s] fileLen[%lld]",
                   nPackIndex, achTmpFile, stUnpackInfo.id,
                   stUnpackInfo.md5, stUnpackInfo.version,
                   stUnpackInfo.len);

        /* 判断是否是当前系统的升级包 */
        if (0 == strcmp(stUnpackInfo.id, UPGRADE_ID))
        {
            /* 当前系统的升级包 */
            /* 移动出来 */
            memset(achCmd, 0, sizeof(achCmd));
            snprintf(achCmd, sizeof(achCmd), "mv %s %s", achTmpFile, TI_UPGRADE_TAR);
            if (system(achCmd) != 0)
            {
                dlog_error("执行命令[%s]失败", achCmd);
            }

            /* 保存当前系统升级包的头信息，用于后面校验 */
            memcpy(pchUpgradePackInfo, &stUnpackInfo, sizeof(Upgrade_Package_t));
        }
        else
        {
            dlog_warn("index[%d] 文件[%s]不是当前系统包,进行删除", nPackIndex, achTmpFile);
            /* 刪除该包 */
            memset(achCmd, 0, sizeof(achCmd));
            snprintf(achCmd, sizeof(achCmd), "rm -f '%s'", achTmpFile);
            if (system(achCmd) != 0)
            {
                dlog_error("执行命令[%s]失败", achCmd);
            }
            nPackIndex++; /* 不是当前系统包，继续累加 */
        }
    }

    /* 关闭源文件 */
    close(nSourceFd);

    /* 删除源文件 */
    unlink(pchSrcFile);

    *nOtherUngradeNum = nPackIndex;

    return OK;
}

/**
 * @brief 备份程序
 * @return [*] IpcRet_E::OK 成功  其他失败
 * @note
 */
static IpcRet_E upgrade_backups_app()
{
    int nRet = 0;
    char anCmd[SYSCMDLEN] = {0};
    snprintf(anCmd, sizeof(anCmd), "rm -r %s", UPGRADE_BAK_DIR);
    dlog_debug("删除备份文件[%s]", anCmd);
    if (system(anCmd) != 0)
    {
        dlog_error("执行命令[%s]失败", anCmd);
    }
    memset(anCmd, 0, sizeof(anCmd));
    /* 备份程序(选项 L 将符号链接转换为源文件 fat32格式的文件系统不支持符号链接) */
    snprintf(anCmd, sizeof(anCmd), "cp -rfL %s %s", UPGRADE_PATH, UPGRADE_BAK);
    dlog_debug("备份文件[%s]", anCmd);
    nRet = system(anCmd);
    if (0 != nRet)
    {
        dlog_error("备份升级程序失败 [%d] [%s]->[%s]",
                   nRet, UPGRADE_PATH, UPGRADE_BAK);
        return ERR;
    }

    return OK;
}

/**
 * @brief 升级失败处理
 * @return [*] IpcRet_E::OK 成功  其他失败
 * @note
 */
static IpcRet_E upgrade_failed_dispose()
{
    int nRet = 0;
    char anCmd[SYSCMDLEN] = {0};

    /* 升级失败 */
    sprintf(anCmd, "rm -rf '%s'", UPGRADE_PATH);
    nRet = system(anCmd);
    if (nRet < 0)
    {
        dlog_error("执行命令失败 [%d]-[%s]", nRet, anCmd);
        return ERR;
    }

    /* 恢复 */
    memset(anCmd, 0, sizeof(anCmd));
    sprintf(anCmd, "mv -f %s/bl /opt", UPGRADE_BAK);
    nRet = system(anCmd);
    if (nRet < 0)
    {
        dlog_error("执行恢复命令失败 [%d]-[%s]", nRet, anCmd);
        return ERR;
    }

    return OK;
}

static int64_t get_file_size(char *filename)
{
    struct stat f_stat;
    printf("......%p %s....\n", filename, filename);
    if (stat(filename, &f_stat) == -1)
    {
        return -1;
    }
    // printf("%lld\n", (int64_t)f_stat.st_size);
    return (int64_t)f_stat.st_size;
}

/**
 * @brief 升级程序
 * @return [*] IpcRet_E::OK 成功  其他失败
 * @note
 */
static IpcRet_E upgrade_app()
{
    int nRet = 0;

    IpcRet_E enRetCode = OK;

    char anCmd[SYSCMDLEN] = {0};

    /* 备份程序 */
    // enRetCode = upgrade_backups_app();
    // if (enRetCode < OK)
    // {
    //     dlog_error("备份程序失败");
    //     return enRetCode;
    // }

    /* 备份有关网络的文件 */
    sprintf(anCmd, "cp -a /etc/resolv.conf /etc/resolv.conf.bak");
    if (system(anCmd) != 0)
    {
        dlog_error("执行命令[%s]失败", anCmd);
    }
    memset(anCmd, 0, sizeof(anCmd));
    sprintf(anCmd, "cp -a /etc/init.d/S80network /etc/init.d/S80network.bak");
    if (system(anCmd) != 0)
    {
        dlog_error("执行命令[%s]失败", anCmd);
    }

    /* 解压升级包 到根目录下 */
    sprintf((char *)anCmd, "tar -xvf %s -C /", TI_UPGRADE_TAR);
    nRet = system(anCmd);
    if (0 != nRet)
    {
        dlog_error("解压升级包失败 [%d]-[%s]", nRet, anCmd);
        upgrade_failed_dispose();
        enRetCode = ERR;
        return enRetCode;
    }

    /* 检测是否解压完 */
    while (1)
    {
        memset(anCmd, 0, sizeof(anCmd));
        sprintf(anCmd, "ps | grep tar |grep -v 'grep' > %s", TMP_STATUS);
        if (system(anCmd) != 0)
        {
            dlog_error("执行命令[%s]失败", anCmd);
        }
        if (get_file_size(TMP_STATUS) > 0)
        {
            memset(anCmd, 0, sizeof(anCmd));
            /* 删除临时文件 */
            sprintf(anCmd, "rm -rf '%s'", TMP_STATUS);
            if (system(anCmd) != 0)
            {
                dlog_error("执行命令[%s]失败", anCmd);
            }
            break;
        }
        else
        {
            break;
        }
    }

    /* 还原有关网络的文件 */
    memset(anCmd, 0, sizeof(anCmd));
    sprintf(anCmd, "mv /etc/resolv.conf.bak /etc/resolv.conf");
    if (system(anCmd) != 0)
    {
        dlog_error("执行命令[%s]失败", anCmd);
    }
    memset(anCmd, 0, sizeof(anCmd));
    sprintf(anCmd, "mv /etc/init.d/S80network.bak /etc/init.d/S80network");
    if (system(anCmd) != 0)
    {
        dlog_error("执行命令[%s]失败", anCmd);
    }

    return enRetCode;
}

/**
 * @brief 删除升级包
 * @return [*] IpcRet_E::OK 成功  其他失败
 * @note
 */
static IpcRet_E uninstall_pack()
{

    char anCmd[SYSCMDLEN] = {0};

    /* 卸载升级包*/
    sprintf(anCmd, "rm -f '%s'", TI_UPGRADE_TAR);
    if (system(anCmd) != 0)
    {
        dlog_error("执行命令[%s]失败", anCmd);
    }

    return OK;
}

/**
 * @brief 升级函数
 * @param [char*] pchPath: 升级包了路径
 * @return [*] IpcRet_E::OK 成功  其他失败
 * @note
 */
static IpcRet_E upgrade_process(char *pchPath)
{
    if (NULL == pchPath)
    {
        dlog_error("传入参数为空");
        return ERR_PARAM_NULL;
    }

    IpcRet_E enRetCode = OK;

    /* 文件长度 */
    int64_t nFileLen = 0;
    char anSrcPath[1024] = {0};
    strncpy(anSrcPath, pchPath, sizeof(anSrcPath));

    char *pchMd5 = NULL;
    /* 有多少个其他升级包 */
    int nOtherUngradeNum = 0;

    /* 当前系统升级包头信息 */
    Upgrade_Package_t stUpgradePackInfo;
    memset(&stUpgradePackInfo, 0, sizeof(stUpgradePackInfo));

    /* 去掉bin文件的头,把文件放入硬盘中 */
    enRetCode = unpack_more_upgrade(anSrcPath, &nOtherUngradeNum, &stUpgradePackInfo);
    if (enRetCode < OK)
    {
        dlog_error("去掉bin文件的头-失败");
        goto EXIT;
    }

    dlog_trace("解包成功 升级包数量[%d] 路径[%s]", nOtherUngradeNum, pchPath);

    /* 获取文件长度进行校验 */
    nFileLen = get_file_size(TI_UPGRADE_TAR);
    if (nFileLen != stUpgradePackInfo.len)
    {
        enRetCode = ERR;
        dlog_error("包长度异常 [%ld]-[%lld]", nFileLen, stUpgradePackInfo.len);
        goto EXIT;
    }

    /* md5校验 */
    pchMd5 = MDFile(TI_UPGRADE_TAR);
    if (NULL == pchMd5)
    {
        enRetCode = ERR_SYSTEM_UPGRADE;
        dlog_error("计算md5值失败");
        goto EXIT;
    }
    dlog_trace("当前升级包MD5值[%s]", pchMd5);

    if (0 != strcmp(pchMd5, stUpgradePackInfo.md5))
    {
        enRetCode = ERR_SYSTEM_UPGRADE;
        dlog_error("md5值异常 [%s]-[%s]", pchMd5, stUpgradePackInfo.md5);
        goto EXIT;
    }

    /* 升级 */
    enRetCode = upgrade_app();
    if (enRetCode < OK)
    {
        dlog_error("升级失败");
        goto EXIT;
    }

EXIT:
    /* 删除升级包 */
    uninstall_pack();
    return enRetCode;
}

/**
 * @brief 升级程序线程
 * @param [void*] pArgv: 参数
 * @return [*] 无
 * @note
 */
void *upgrade_process_thread(void *pArgv)
{
    UpgradeInfo_S *pstUpgradeInfo = (UpgradeInfo_S *)pArgv;

    int nRet = -1;
    IpcRet_E enRetCode = OK;

    /* 检测升级文件路径是否存在 */
    enRetCode = check_path_exsit(pstUpgradeInfo->achPath);
    if (enRetCode < OK)
    {
        /* 设置升级失败 */
        dataManage_set_upgradeStatus(TI_UPGRADE_RUNFAIL);
        dlog_error("升级失败-升级路径不存在[%s]", pstUpgradeInfo->achPath);
        free(pstUpgradeInfo);

        return NULL;
    }

    /* 开始升级 */
    enRetCode = upgrade_process(pstUpgradeInfo->achPath);
    if (enRetCode < OK)
    {
        dlog_error("升级失败");
        dataManage_set_upgradeStatus(TI_UPGRADE_RUNFAIL);
        free(pstUpgradeInfo);
        return NULL;
    }

    dlog_trace("升级成功");
    dataManage_set_upgradeStatus(TI_UPGRADE_RUNSUCCESS);

    free(pstUpgradeInfo);
    sleep(2);
    nRet = system("sync && reboot");
    if (nRet != 0)
    {
        /* 执行额外重启脚本 */
        nRet = system("/opt/bl/shell/rebootDocker.sh");
    }

    return NULL;
}

/* 开始升级 */
IpcRet_E upgrade_start(UpgradeInfo_S stUpgradeInfo)
{
    int nRet = -1;
    IpcRet_E enRetCode = OK;
    TiUpgradeRuslut_E enStatus;
    dataManage_get_upgradeStatus(&enStatus);

    /* 升级中 */
    if (TI_UPGRADE_RUNING == enStatus)
    {
        dlog_error("正在升级中, 无法再次升级");
        return ERR;
    }

    UpgradeInfo_S *pstUpgradeInfo = (UpgradeInfo_S *)malloc(sizeof(UpgradeInfo_S));
    if (pstUpgradeInfo == NULL)
    {
        return ERR;
    }
    memset(pstUpgradeInfo, 0, sizeof(UpgradeInfo_S));
    snprintf(pstUpgradeInfo->achPath, sizeof(pstUpgradeInfo->achPath), "%s", stUpgradeInfo.achPath);

    pthread_t tid;
    pthread_attr_t attr_upgrade;
    pthread_attr_init(&attr_upgrade);
    pthread_attr_setscope(&attr_upgrade, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setdetachstate(&attr_upgrade, PTHREAD_CREATE_DETACHED);
    nRet = pthread_create(&tid, &attr_upgrade, upgrade_process_thread, (void *)((pstUpgradeInfo)));
    if (nRet)
    {
        /* 设置当前状态 */
        dataManage_set_upgradeStatus(TI_UPGRADE_RUNFAIL);

        dlog_error("开始升级线程失败");
        enRetCode = ERR_CREATE;
    }
    else
    {
        /* 设置当前状态 */
        dataManage_set_upgradeStatus(TI_UPGRADE_RUNING);
        dlog_info("创建升级线程成功");
    }

    pthread_attr_destroy(&attr_upgrade);

    return enRetCode;
}