/**
* @FilePath     : upgrade_logic.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-08-26 11:22:17
 * @LastEditors  : leiyy
 * @LastEditTime : 2025-12-24 09:38:30
 * @Description  : 升级逻辑模块
 */

#include "upgrade_logic.h"
#include <cstring>
#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "upgrade_define.h"
#include "data_manage.h"
#include "dlog.h"

/*流式解压，管道有瓶颈上限，不能太大*/
#define READ_BUF_SIZE 16384  //16K

/**
 * @brief 检测路径
 */
static IpcRet_E check_path_exsit(char *pchPath)
{
    if (NULL == pchPath) return ERR_PARAM_NULL;
    if (0 != access(pchPath, F_OK))
    {
        dlog_error("升级文件路径不存在 [%s]", pchPath);
        return ERR_NOT_EXIST;
    }
    return OK;
}

/**
 * @brief 备份网络配置
 */
static void network_config_backup()
{
    dlog_info("正在备份网络配置...");
    system("cp -a /etc/resolv.conf /etc/resolv.conf.bak");
    system("cp -a /etc/init.d/S80network /etc/init.d/S80network.bak");
    system("chattr -i /etc/resolv.conf");
}

/**
 * @brief 恢复网络配置
 */
static void network_config_restore()
{
    dlog_info("正在恢复网络配置...");
    if (access("/etc/resolv.conf.bak", F_OK) == 0) {
        system("cp /etc/resolv.conf.bak /etc/resolv.conf");
        system("rm /etc/resolv.conf.bak");
    }
    if (access("/etc/init.d/S80network.bak", F_OK) == 0) {
        system("cp /etc/init.d/S80network.bak /etc/init.d/S80network");
        system("rm /etc/init.d/S80network.bak");
    }
}

/**
 * @brief 流式计算MD5-不占硬盘
 */
static IpcRet_E verify_md5_stream(int fd, int64_t len, const char *expected_md5)
{
    unsigned char buffer[READ_BUF_SIZE];
    int64_t remain = len;
    ssize_t nRead = 0;
    
    MD5_CTX ctx; 
    unsigned char digest[16];
    char pcMd5[33] = {0};
    int i;

    MD5Init1(&ctx); 

    while (remain > 0)
    {
        size_t to_read = (remain > READ_BUF_SIZE) ? READ_BUF_SIZE : remain;
        nRead = read(fd, buffer, to_read);
        if (nRead <= 0) {
            dlog_error("计算MD5读取失败");
            return ERR_FREAD;
        }
        MD5Update1(&ctx, buffer, nRead); // 累加计算
        remain -= nRead;
    }

    MD5Final1(digest, &ctx); // 结束

    for (i = 0; i < 16; i++) snprintf(&pcMd5[i * 2], 3, "%02x", digest[i]);

    if (strcmp(pcMd5, expected_md5) != 0) {
        dlog_error("MD5校验失败: 计算值[%s] != 期望值[%s]", pcMd5, expected_md5);
        return ERR_SYSTEM_UPGRADE;
    }
    
    dlog_info("MD5校验通过");
    return OK;
}

/**
 * @brief 流式解压安装
 */
static IpcRet_E install_stream(int fd, int64_t len, const char *destPath)
{
    FILE *pipe_tar = NULL;
    unsigned char buffer[READ_BUF_SIZE];
    int64_t remain = len;
    ssize_t nRead = 0;
    char cmd[256] = {0};


    // 确保目录存在
    if (access(destPath, F_OK) != 0) {
        snprintf(cmd, sizeof(cmd), "mkdir -p %s", destPath);
        system(cmd);
    }

    snprintf(cmd, sizeof(cmd), "tar -zxf - -C %s", destPath);
    dlog_info("执行解压: %s", cmd);

    pipe_tar = popen(cmd, "w");
    if (NULL == pipe_tar) {
        dlog_error("无法启动tar管道");
        return ERR_OPEN;
    }

    while (remain > 0)
    {
        size_t to_read = (remain > READ_BUF_SIZE) ? READ_BUF_SIZE : remain;
        nRead = read(fd, buffer, to_read);
        if (nRead <= 0) {
            pclose(pipe_tar);
            return ERR_FREAD;
        }

        if (fwrite(buffer, 1, nRead, pipe_tar) != nRead) {

            dlog_error("写入管道失败");
            pclose(pipe_tar);
            return ERR_FWRITE; 
        }

        remain -= nRead;
    }

    int status = pclose(pipe_tar);
    if (status != 0) {
        int exit_code = 0;
        if (WIFEXITED(status)) {
            exit_code = WEXITSTATUS(status);
        }
        
        dlog_error("tar执行异常, 原始状态: %d, 退出码: %d", status, exit_code);
        
        return ERR_SYSTEM_UPGRADE; 
    }

    return OK;
}


static IpcRet_E process_upgrade(char *pchSrcFile)
{
    int srcFd = -1;
    Upgrade_Package_t stInfo;
    IpcRet_E ret = OK;
    off_t currentPos = 0;
    const char *targetPath = NULL;
    bool bNeedNetworkBackup = false;

    srcFd = open(pchSrcFile, O_RDONLY);
    if (srcFd < 0) {
        dlog_error("打开升级包失败: %s", pchSrcFile);
        return ERR_OPEN;
    }

    // 循环读取所有的包头
    while (read(srcFd, &stInfo, sizeof(Upgrade_Package_t)) > 0)
    {
        // 记录数据区开始的位置
        currentPos = lseek(srcFd, 0, SEEK_CUR);

        dlog_info("扫描到包: ID[%s] Ver[%s] Len[%lld] Type[%d]", 
                  stInfo.id, stInfo.version, stInfo.len, stInfo.type);

        // 判断是否为本机升级包
        if (strcmp(stInfo.id, UPGRADE_ID) == 0)
        {
            //类型选择策略
            switch (stInfo.type)
            {
            case UPGRADE_TYPE_APP:
                dlog_info(">>> 识别为: 系统/APP升级 <<<");
                targetPath = PATH_INSTALL_ROOT;
                bNeedNetworkBackup = true;
                break;

            case UPGRADE_TYPE_AI:
                dlog_info(">>> 识别为: AI模型升级 <<<");
                targetPath = PATH_INSTALL_AI;
                bNeedNetworkBackup = false; 
                break;

            default:
                dlog_warn("未知类型[%d], 默认按系统升级处理", stInfo.type);
                targetPath = PATH_INSTALL_ROOT;
                bNeedNetworkBackup = true;
                break;
            }

            // 校验MD5 (流式，不占空间)
            ret = verify_md5_stream(srcFd, stInfo.len, stInfo.md5);
            if (ret != OK) {
                close(srcFd);
                return ret;
            }

            if (bNeedNetworkBackup) network_config_backup();

            // 空间极度紧张的时候，AI模型升级可以需要先删除旧文件
            if (stInfo.type == UPGRADE_TYPE_AI) {
                // 危险操作：为了腾出空间，先删除旧模型
                // 如果升级失败，旧模型也没了。根据实际情况决定是否开启
                // char rmCmd[256];
                // snprintf(rmCmd, sizeof(rmCmd), "rm -rf %s/*", PATH_INSTALL_AI);
                // system(rmCmd);
            }

            // 回退指针，开始流式解压
            lseek(srcFd, currentPos, SEEK_SET);
            ret = install_stream(srcFd, stInfo.len, targetPath);

            if (bNeedNetworkBackup) network_config_restore();

            if (ret != OK) {
                dlog_error("升级包安装失败");
                close(srcFd);
                return ret;
            }
            
            dlog_info("升级包安装成功");
        }
        else
        {
            dlog_warn("非本机升级包，跳过");
            // 直接跳过数据区，不读不写
            lseek(srcFd, stInfo.len, SEEK_CUR);
            ret = ERR;
        }
    }

    close(srcFd);
    return ret;
}

/**
 * @brief 线程入口
 */
void *upgrade_process_thread(void *pArgv)
{
    UpgradeInfo_S *pInfo = (UpgradeInfo_S *)pArgv;
    IpcRet_E ret = OK;

    // 检测文件
    if (check_path_exsit(pInfo->achPath) != OK) {
        dataManage_set_upgradeStatus(TI_UPGRADE_RUNFAIL);
        free(pInfo);
        return NULL;
    }

    // 执行升级
    ret = process_upgrade(pInfo->achPath);

    // 删除源包
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -f '%s'", pInfo->achPath);
    system(cmd);

    if (ret != OK) {
        dataManage_set_upgradeStatus(TI_UPGRADE_RUNFAIL);
        dlog_error("升级流程失败");
    } else {
        dataManage_set_upgradeStatus(TI_UPGRADE_RUNSUCCESS);
        dlog_info("升级流程成功");

        sleep(2);
        // 执行重启
        system("sync");
        if (system("reboot") != 0) {
            system("/opt/bl/shell/rebootDocker.sh");
        }
    }

    free(pInfo);
    return NULL;
}

/**
 * @brief 外部调用接口
 */
IpcRet_E upgrade_start(UpgradeInfo_S stUpgradeInfo)
{
    TiUpgradeRuslut_E enStatus;
    dataManage_get_upgradeStatus(&enStatus);

    if (TI_UPGRADE_RUNING == enStatus) {
        dlog_error("正在升级中...");
        return ERR;
    }

    UpgradeInfo_S *pstInfo = (UpgradeInfo_S *)malloc(sizeof(UpgradeInfo_S));
    if (!pstInfo) return ERR;
    
    // 复制参数
    memcpy(pstInfo, &stUpgradeInfo, sizeof(UpgradeInfo_S));

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); // 分离线程，自动回收

    int nRet = pthread_create(&tid, &attr, upgrade_process_thread, pstInfo);
    if (nRet != 0) {
        dlog_error("创建升级线程失败");
        free(pstInfo);
        dataManage_set_upgradeStatus(TI_UPGRADE_RUNFAIL);
        pthread_attr_destroy(&attr);
        return ERR_CREATE;
    }

    dataManage_set_upgradeStatus(TI_UPGRADE_RUNING);
    pthread_attr_destroy(&attr);
    dlog_info("升级线程已启动");
    
    return OK;
}
