/**
 * @FilePath     : upgrade_logic.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-08-26 11:22:17
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-04 10:50:35
 * @Description  : 升级逻辑模块（流式校验、解压与上传临时目录清理）
 */

#include "upgrade_logic.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

#include "upgrade_define.h"
#include "data_manage.h"
#include "dlog.h"
#include "md5lib.h"

static IpcRet_E remove_upload_entry(const char *pchPath, int *pnRemovedCount);

/**
 * @brief   : 清理指定目录中的全部目录项
 * @param    {const char*} pchDirectory：待清理目录路径
 * @param    {int*} pnRemovedCount：已删除目录项数量
 * @return   {IpcRet_E} OK：成功，其他：清理失败
 */
static IpcRet_E remove_upload_directory_contents(const char *pchDirectory,
                                                 int *pnRemovedCount)
{
    if (NULL == pchDirectory || NULL == pnRemovedCount)
    {
        dlog_error("清理上传目录时传入参数为空");
        return ERR_PARAM_NULL;
    }

    /* 目录句柄保证遍历期间目录状态有效，关闭失败也要纳入清理结果。 */
    DIR *pDir = opendir(pchDirectory);
    if (NULL == pDir)
    {
        dlog_error("打开上传目录失败[%s] errno[%d]", pchDirectory, errno);
        return ERR_OPEN;
    }

    /* 记录目录遍历和子项删除过程中出现的最严重错误。 */
    IpcRet_E enRetCode = OK;
    /* 当前目录项描述，由 readdir 返回且只在本轮循环内使用。 */
    struct dirent *pstEntry = NULL;
    while (true)
    {
        /* readdir 返回 NULL 既可能是目录结束，也可能是读取目录失败，需要显式区分。 */
        errno = 0;
        pstEntry = readdir(pDir);
        if (NULL == pstEntry)
        {
            if (errno != 0)
            {
                dlog_error("读取上传目录失败[%s] errno[%d]", pchDirectory, errno);
                enRetCode = ERR_FILE_ERR;
            }
            break;
        }

        if (0 == strcmp(pstEntry->d_name, ".") ||
            0 == strcmp(pstEntry->d_name, ".."))
        {
            continue;
        }

        /* 使用固定大小缓冲区构造子项路径，避免路径拼接越界。 */
        char achChildPath[PATH_MAX] = {0};
        /* snprintf 返回未截断时的完整路径长度，用于识别截断结果。 */
        int nPathLength = snprintf(achChildPath,
                                   sizeof(achChildPath),
                                   "%s/%s",
                                   pchDirectory,
                                   pstEntry->d_name);
        if (nPathLength < 0 || nPathLength >= (int)sizeof(achChildPath))
        {
            dlog_error("上传目录项路径过长[%s/%s]", pchDirectory, pstEntry->d_name);
            enRetCode = ERR_FILE_ERR;
            continue;
        }

        /* 子项删除结果不能被目录遍历的后续成功项覆盖。 */
        IpcRet_E enChildRetCode = remove_upload_entry(achChildPath, pnRemovedCount);
        if (enChildRetCode != OK)
        {
            enRetCode = enChildRetCode;
        }
    }

    if (closedir(pDir) != 0)
    {
        dlog_error("关闭上传目录失败[%s] errno[%d]", pchDirectory, errno);
        enRetCode = ERR_FILE_ERR;
    }

    return enRetCode;
}

/**
 * @brief   : 递归删除上传目录中的单个目录项
 * @param    {const char*} pchPath：待删除的目录项路径
 * @param    {int*} pnRemovedCount：已删除目录项数量
 * @return   {IpcRet_E} OK：成功，其他：删除失败
 * @note    : 使用 lstat 判断目录项，禁止跟随软链接，避免清理范围越界。
 */
static IpcRet_E remove_upload_entry(const char *pchPath, int *pnRemovedCount)
{
    if (NULL == pchPath || NULL == pnRemovedCount)
    {
        dlog_error("清理上传目录时传入参数为空");
        return ERR_PARAM_NULL;
    }

    /* lstat 结果只用于判断目录项类型，避免因判断而跟随软链接。 */
    struct stat stFileInfo = {};
    if (lstat(pchPath, &stFileInfo) != 0)
    {
        if (ENOENT == errno)
        {
            return OK;
        }

        dlog_error("检查上传目录项失败[%s] errno[%d]", pchPath, errno);
        return ERR_FILE_ERR;
    }

    if (S_ISDIR(stFileInfo.st_mode))
    {
        /* 子目录即使存在部分删除失败，也继续尝试清理可删除项。 */
        IpcRet_E enRetCode = remove_upload_directory_contents(pchPath, pnRemovedCount);
        if (rmdir(pchPath) != 0 && ENOENT != errno)
        {
            dlog_error("删除上传子目录失败[%s] errno[%d]", pchPath, errno);
            enRetCode = ERR_FILE_ERR;
        }
        else
        {
            ++(*pnRemovedCount);
        }
        return enRetCode;
    }

    if (unlink(pchPath) != 0 && ENOENT != errno)
    {
        dlog_error("删除上传目录项失败[%s] errno[%d]", pchPath, errno);
        return ERR_FILE_ERR;
    }

    ++(*pnRemovedCount);
    return OK;
}

/**
 * @brief   : 清空运行时上传目录内容
 * @return   {IpcRet_E} OK：成功，其他：清理失败
 * @note    : 保留 `/tmp/upload` 根目录；固定路径且拒绝根目录软链接，避免误删目录外数据。
 */
static IpcRet_E cleanup_upload_directory()
{
    /* 根目录属性用于确认固定清理目标不是软链接或普通文件。 */
    struct stat stUploadDir = {};
    if (lstat(UPLOAD_RUNTIME_PATH, &stUploadDir) != 0)
    {
        if (ENOENT == errno)
        {
            dlog_info("运行时上传目录不存在，跳过清理[%s]", UPLOAD_RUNTIME_PATH);
            return OK;
        }

        dlog_error("检查运行时上传目录失败[%s] errno[%d]", UPLOAD_RUNTIME_PATH, errno);
        return ERR_FILE_ERR;
    }

    /* ! 根目录必须是普通目录，禁止清理指向其他位置的软链接。 */
    if (!S_ISDIR(stUploadDir.st_mode))
    {
        dlog_error("运行时上传路径不是普通目录，拒绝清理[%s]", UPLOAD_RUNTIME_PATH);
        return ERR_FILE_ERR;
    }

    /* 记录递归清理的最终结果和已处理目录项数量。 */
    IpcRet_E enRetCode = OK;
    int nRemovedCount = 0;
    enRetCode = remove_upload_directory_contents(UPLOAD_RUNTIME_PATH, &nRemovedCount);

    if (enRetCode == OK)
    {
        dlog_info("运行时上传目录清理完成[%s]，清理项目数[%d]", UPLOAD_RUNTIME_PATH, nRemovedCount);
    }
    else
    {
        dlog_error("运行时上传目录清理不完整[%s]，已清理项目数[%d]", UPLOAD_RUNTIME_PATH, nRemovedCount);
    }

    return enRetCode;
}

/**
 * @brief   : 执行上传目录清理并记录调用上下文
 * @param    {const char*} pchReason：清理触发原因
 * @return   {void}
 */
static void cleanup_upload_directory_with_log(const char *pchReason)
{
    /* 清理结果只用于日志，不能覆盖已经完成的升级状态。 */
    IpcRet_E enRetCode = cleanup_upload_directory();
    if (enRetCode != OK)
    {
        dlog_error("上传目录清理失败，触发原因[%s] 返回码[%d]",
                   (NULL == pchReason) ? "未知" : pchReason,
                   enRetCode);
    }
}

/**
 * @brief   : 读取一个完整的升级包头
 * @param    {int} nFd：升级包文件描述符
 * @param    {Upgrade_Package_t*} pstInfo：升级包头输出
 * @param    {bool*} pbEndOfFile：是否在读取包头前到达文件尾
 * @return   {IpcRet_E} OK：读取成功或正常文件尾，其他：读取失败
 */
static IpcRet_E read_package_header(int nFd,
                                    Upgrade_Package_t *pstInfo,
                                    bool *pbEndOfFile)
{
    if (NULL == pstInfo || NULL == pbEndOfFile)
    {
        dlog_error("读取升级包头时传入参数为空");
        return ERR_PARAM_NULL;
    }

    *pbEndOfFile = false;
    /* 已读取的包头字节数，支持一次 read 未读满固定包头。 */
    size_t nReadTotal = 0;
    while (nReadTotal < sizeof(Upgrade_Package_t))
    {
        /* 本次 read 实际读取的字节数。 */
        ssize_t nReadSize = read(nFd,
                                 reinterpret_cast<char *>(pstInfo) + nReadTotal,
                                 sizeof(Upgrade_Package_t) - nReadTotal);
        if (nReadSize < 0)
        {
            if (EINTR == errno)
            {
                continue;
            }

            dlog_error("读取升级包头失败 errno[%d]", errno);
            return ERR_FREAD;
        }

        if (0 == nReadSize)
        {
            if (0 == nReadTotal)
            {
                *pbEndOfFile = true;
                return OK;
            }

            dlog_error("升级包头不完整，实际读取[%zu] 期望[%zu]",
                       nReadTotal,
                       sizeof(Upgrade_Package_t));
            return ERR_FREAD;
        }

        nReadTotal += (size_t)nReadSize;
    }

    return OK;
}

/**
 * @brief   : 检测升级文件路径是否存在
 * @param    {char*} pchPath：文件路径
 * @return   {IpcRet_E} OK 成功，其他失败
 */
static IpcRet_E check_path_exsit(char *pchPath)
{
    if (NULL == pchPath)
    {
        dlog_error("传入参数为空");
        return ERR_PARAM_NULL;
    }

    if (0 != access(pchPath, F_OK))
    {
        dlog_error("升级文件路径不存在 [%s]", pchPath);
        return ERR_NOT_EXIST;
    }

    return OK;
}

/**
 * @brief   : 备份网络配置文件，防止升级包覆盖后丢失
 * @note    : 仅备份 resolv.conf 和 S80network，其他网络配置不在此范围
 */
static void network_config_backup()
{
    dlog_info("正在备份网络配置");
    system("cp -a /etc/resolv.conf /etc/resolv.conf.bak");
    system("cp -a /etc/init.d/S80network /etc/init.d/S80network.bak");
}

/**
 * @brief   : 恢复网络配置文件
 * @note    : 从 .bak 文件还原，还原成功后删除备份文件
 */
static void network_config_restore()
{
    dlog_info("正在恢复网络配置");
    if (access("/etc/resolv.conf.bak", F_OK) == 0)
    {
        system("cp /etc/resolv.conf.bak /etc/resolv.conf");
        system("rm /etc/resolv.conf.bak");
    }
    if (access("/etc/init.d/S80network.bak", F_OK) == 0)
    {
        system("cp /etc/init.d/S80network.bak /etc/init.d/S80network");
        system("rm /etc/init.d/S80network.bak");
    }
}

/**
 * @brief   : 流式计算 MD5，不产生中间文件
 * @param    {int} fd：文件描述符，当前位置应为数据区起始
 * @param    {int64_t} len：待校验数据的字节数
 * @param    {const char*} expected_md5：期望的 MD5 十六进制字符串（33 字节含 \0）
 * @return   {IpcRet_E} OK 校验通过，ERR_FREAD 读取失败，ERR_SYSTEM_UPGRADE MD5 不匹配
 * @note    : 读完后 fd 位于数据区末尾，调用方需 lseek 回退后再进行解压
 */
static IpcRet_E verify_md5_stream(int fd, int64_t len, const char *expected_md5)
{
    unsigned char buffer[READ_BUF_SIZE];
    int64_t remain = len;
    ssize_t nRead = 0;

    MD5_CTX ctx;
    unsigned char digest[16];
    char pcMd5[33] = {0};

    MD5Init1(&ctx);

    while (remain > 0)
    {
        size_t to_read = (remain > READ_BUF_SIZE) ? READ_BUF_SIZE : remain;
        nRead = read(fd, buffer, to_read);
        if (nRead <= 0)
        {
            dlog_error("流式 MD5 读取失败 remain[%lld] nRead[%zd]", (long long)remain, nRead);
            return ERR_FREAD;
        }
        MD5Update1(&ctx, buffer, nRead);
        remain -= nRead;
    }

    MD5Final1(digest, &ctx);

    for (int i = 0; i < 16; i++)
    {
        snprintf(&pcMd5[i * 2], 3, "%02x", digest[i]);
    }

    if (strcmp(pcMd5, expected_md5) != 0)
    {
        dlog_error("MD5 校验失败: 计算值[%s] != 期望值[%s]", pcMd5, expected_md5);
        return ERR_SYSTEM_UPGRADE;
    }

    dlog_info("MD5 校验通过");
    return OK;
}

/**
 * @brief   : 流式解压安装，通过管道将数据逐块送入 tar
 * @param    {int} fd：文件描述符，当前位置应为数据区起始
 * @param    {int64_t} len：待解压数据的字节数
 * @param    {const char*} destPath：解压目标目录
 * @return   {IpcRet_E} OK 成功，ERR_OPEN 管道打开失败，ERR_FREAD/ERR_FWRITE 读写失败，
 *              ERR_SYSTEM_UPGRADE tar 退出异常
 * @note    : 使用 popen 创建 tar 子进程，通过 fwrite 将数据流式写入管道
 */
static IpcRet_E install_stream(int fd, int64_t len, const char *destPath)
{
    unsigned char buffer[READ_BUF_SIZE];
    int64_t remain = len;
    ssize_t nRead = 0;
    char cmd[256] = {0};

    snprintf(cmd, sizeof(cmd), "tar -zxf - -C %s", destPath);
    dlog_info("执行流式解压: %s", cmd);

    FILE *pipe_tar = popen(cmd, "w");
    if (NULL == pipe_tar)
    {
        dlog_error("无法启动 tar 管道 errno[%d]", errno);
        return ERR_OPEN;
    }

    while (remain > 0)
    {
        size_t to_read = (remain > READ_BUF_SIZE) ? READ_BUF_SIZE : remain;
        nRead = read(fd, buffer, to_read);
        if (nRead <= 0)
        {
            dlog_error("流式解压读取失败 remain[%lld]", (long long)remain);
            pclose(pipe_tar);
            return ERR_FREAD;
        }

        if (fwrite(buffer, 1, nRead, pipe_tar) != (size_t)nRead)
        {
            dlog_error("写入 tar 管道失败");
            pclose(pipe_tar);
            return ERR_FWRITE;
        }

        remain -= nRead;
    }

    /* pclose 会等待 tar 子进程结束并返回其退出状态 */
    int status = pclose(pipe_tar);
    if (status != 0)
    {
        int exit_code = 0;
        if (WIFEXITED(status))
        {
            exit_code = WEXITSTATUS(status);
        }
        dlog_error("tar 解压异常 状态[%d] 退出码[%d]", status, exit_code);
        return ERR_SYSTEM_UPGRADE;
    }

    dlog_info("流式解压完成");
    return OK;
}

/**
 * @brief   : 流式升级主流程，替代旧版拆包+校验+解压三步流程
 * @param    {char*} pchSrcFile：升级包 .bin 文件的绝对路径
 * @return   {IpcRet_E} OK 成功，其他失败
 * @note    : 处理流程：
 *            1. 打开 .bin 文件
 *            2. 循环读取 Upgrade_Package_t 包头
 *            3. 对 id == UPGRADE_ID 的包：流式 MD5 校验 → lseek 回退 → 流式解压
 *            4. 对非本机包：lseek 跳过数据区
 *            5. 处理完成后删除源 .bin 文件
 *          全程无中间文件（不写 update.tar.gz），峰值磁盘占用仅为 .bin 本身
 */
static IpcRet_E process_upgrade(char *pchSrcFile)
{
    if (NULL == pchSrcFile)
    {
        dlog_error("传入参数为空");
        return ERR_PARAM_NULL;
    }

    int srcFd = open(pchSrcFile, O_RDONLY);
    if (srcFd < 0)
    {
        dlog_error("打开升级包失败 [%s]", pchSrcFile);
        return ERR_OPEN;
    }

    /* 源文件属性快照，用于校验每个包的数据区不会超出上传文件范围。 */
    struct stat stSrcFile = {};
    if (fstat(srcFd, &stSrcFile) != 0)
    {
        dlog_error("获取升级包属性失败[%s] errno[%d]", pchSrcFile, errno);
        close(srcFd);
        return ERR_FILE_ERR;
    }

    Upgrade_Package_t stInfo;
    IpcRet_E enRetCode = ERR;

    /* 循环读取所有的包头 */
    while (true)
    {
        /* 读取包头时区分正常文件尾和不完整包头。 */
        bool bEndOfFile = false;
        /* 包头读取结果。 */
        IpcRet_E enReadRetCode = read_package_header(srcFd, &stInfo, &bEndOfFile);
        if (enReadRetCode != OK)
        {
            close(srcFd);
            return enReadRetCode;
        }
        if (bEndOfFile)
        {
            break;
        }

        /* 记录数据区起始位置，用于 MD5 校验后 lseek 回退 */
        off_t currentPos = lseek(srcFd, 0, SEEK_CUR);
        if (currentPos < 0)
        {
            dlog_error("获取升级包数据起始位置失败 errno[%d]", errno);
            close(srcFd);
            return ERR_FILE_ERR;
        }

        /* 使用文件快照约束包长度，避免异常长度导致 lseek 越界。 */
        int64_t nFileSize = (int64_t)stSrcFile.st_size;
        int64_t nDataStart = (int64_t)currentPos;
        if (stInfo.len <= 0 ||
            nDataStart > nFileSize ||
            stInfo.len > nFileSize - nDataStart)
        {
            dlog_error("升级包长度非法 len[%lld] dataStart[%lld] fileSize[%lld]",
                       (long long)stInfo.len,
                       (long long)nDataStart,
                       (long long)nFileSize);
            close(srcFd);
            return ERR_PARAM;
        }

        if (stInfo.md5[sizeof(stInfo.md5) - 1] != '\0')
        {
            dlog_error("升级包 MD5 字段未正确结束");
            close(srcFd);
            return ERR_PARAM;
        }

        dlog_info("扫描到包: ID[%.*s] Ver[%.*s] Len[%lld]",
                  (int)sizeof(stInfo.id),
                  stInfo.id,
                  (int)sizeof(stInfo.version),
                  stInfo.version,
                  (long long)stInfo.len);

        if (memcmp(stInfo.id, UPGRADE_ID, sizeof(stInfo.id)) != 0)
        {
            dlog_warn("非本机升级包 ID[%.*s]，跳过",
                      (int)sizeof(stInfo.id),
                      stInfo.id);
            if (lseek(srcFd, (off_t)stInfo.len, SEEK_CUR) < 0)
            {
                dlog_error("跳过非本机升级包失败 errno[%d]", errno);
                close(srcFd);
                return ERR_FILE_ERR;
            }
            continue;
        }

        /* === 本机升级包 === */

        /* step: 流式 MD5 校验（不占磁盘空间） */
        enRetCode = verify_md5_stream(srcFd, stInfo.len, stInfo.md5);
        if (enRetCode != OK)
        {
            dlog_error("MD5 校验失败");
            close(srcFd);
            return enRetCode;
        }

        /* step: lseek 回退到数据区起始位置，准备流式解压 */
        if (lseek(srcFd, currentPos, SEEK_SET) < 0)
        {
            dlog_error("回退升级包数据位置失败 errno[%d]", errno);
            close(srcFd);
            return ERR_FILE_ERR;
        }

        /* step: 备份网络配置（防止升级包覆盖） */
        network_config_backup();

        /* step: 流式解压到根目录 */
        enRetCode = install_stream(srcFd, stInfo.len, PATH_INSTALL_ROOT);

        /* step: 恢复网络配置 */
        network_config_restore();

        if (enRetCode != OK)
        {
            dlog_error("升级包安装失败");
            close(srcFd);
            return enRetCode;
        }

        dlog_info("升级包安装成功");
    }

    close(srcFd);

    /* 删除源 .bin 文件 */
    if (unlink(pchSrcFile) != 0 && ENOENT != errno)
    {
        dlog_error("删除源升级包失败[%s] errno[%d]", pchSrcFile, errno);
        return ERR_FILE_ERR;
    }

    return enRetCode;
}

/**
 * @brief   : 升级程序线程入口
 * @param    {void*} pArgv：UpgradeInfo_S 指针，由调用方 malloc 分配，本函数负责释放
 * @return   {void*} NULL
 */
void *upgrade_process_thread(void *pArgv)
{
    UpgradeInfo_S *pstUpgradeInfo = (UpgradeInfo_S *)pArgv;
    IpcRet_E enRetCode = OK;

    if (NULL == pstUpgradeInfo)
    {
        dlog_error("升级线程参数为空");
        cleanup_upload_directory_with_log("升级线程参数为空");
        dataManage_set_upgradeStatus(TI_UPGRADE_RUNFAIL);
        return NULL;
    }

    /* 检测升级文件路径是否存在 */
    enRetCode = check_path_exsit(pstUpgradeInfo->achPath);
    if (enRetCode < OK)
    {
        cleanup_upload_directory_with_log("升级路径不存在");
        dataManage_set_upgradeStatus(TI_UPGRADE_RUNFAIL);
        dlog_error("升级失败-升级路径不存在[%s]", pstUpgradeInfo->achPath);
        free(pstUpgradeInfo);
        return NULL;
    }

    /* 执行流式升级 */
    enRetCode = process_upgrade(pstUpgradeInfo->achPath);

    if (enRetCode < OK)
    {
        cleanup_upload_directory_with_log("升级处理失败");
        dlog_error("升级失败");
        dataManage_set_upgradeStatus(TI_UPGRADE_RUNFAIL);
        free(pstUpgradeInfo);
        return NULL;
    }

    dlog_trace("升级成功");
    cleanup_upload_directory_with_log("升级处理成功");
    dataManage_set_upgradeStatus(TI_UPGRADE_RUNSUCCESS);

    free(pstUpgradeInfo);
    sleep(2);

    /* 执行重启 */
    int nRet = system("sync && reboot");
    if (nRet != 0)
    {
        /* ! 主重启命令失败时，使用备用重启脚本 */
        nRet = system("/opt/bl/shell/rebootDocker.sh");
    }

    return NULL;
}

/**
 * @brief   : 开始升级（外部调用接口）
 * @param    {UpgradeInfo_S} stUpgradeInfo：升级信息
 * @return   {IpcRet_E} OK 成功，ERR 正在升级中，ERR_CREATE 线程创建失败
 * @note    : 创建分离线程执行升级，调用方无需等待完成
 */
IpcRet_E upgrade_start(UpgradeInfo_S stUpgradeInfo)
{
    IpcRet_E enRetCode = OK;
    TiUpgradeRuslut_E enStatus;
    enRetCode = dataManage_get_upgradeStatus(&enStatus);
    if (enRetCode != OK)
    {
        cleanup_upload_directory_with_log("读取升级状态失败");
        dlog_error("读取升级状态失败 返回码[%d]", enRetCode);
        return enRetCode;
    }

    /* 升级中则拒绝新请求 */
    if (TI_UPGRADE_RUNING == enStatus)
    {
        dlog_error("正在升级中, 无法再次升级");
        return ERR;
    }

    UpgradeInfo_S *pstUpgradeInfo = (UpgradeInfo_S *)malloc(sizeof(UpgradeInfo_S));
    if (pstUpgradeInfo == NULL)
    {
        cleanup_upload_directory_with_log("分配升级线程参数失败");
        dataManage_set_upgradeStatus(TI_UPGRADE_RUNFAIL);
        return ERR;
    }
    memset(pstUpgradeInfo, 0, sizeof(UpgradeInfo_S));
    snprintf(pstUpgradeInfo->achPath, sizeof(pstUpgradeInfo->achPath),
             "%s", stUpgradeInfo.achPath);

    pthread_t tid;
    pthread_attr_t attr_upgrade;
    /* 线程属性返回码；属性初始化失败时不能继续创建升级线程。 */
    int nAttrRet = pthread_attr_init(&attr_upgrade);
    if (nAttrRet != 0)
    {
        cleanup_upload_directory_with_log("初始化升级线程属性失败");
        dataManage_set_upgradeStatus(TI_UPGRADE_RUNFAIL);
        dlog_error("初始化升级线程属性失败 返回码[%d]", nAttrRet);
        free(pstUpgradeInfo);
        return ERR_CREATE;
    }

    nAttrRet = pthread_attr_setscope(&attr_upgrade, PTHREAD_SCOPE_SYSTEM);
    if (nAttrRet != 0)
    {
        /* 某些精简 libc 不支持设置线程竞争范围，该属性不影响升级正确性，降级继续执行。 */
        dlog_warn("设置升级线程竞争范围失败，继续创建线程 返回码[%d]", nAttrRet);
    }

    nAttrRet = pthread_attr_setdetachstate(&attr_upgrade, PTHREAD_CREATE_DETACHED);
    if (nAttrRet != 0)
    {
        cleanup_upload_directory_with_log("设置升级线程分离属性失败");
        dataManage_set_upgradeStatus(TI_UPGRADE_RUNFAIL);
        dlog_error("设置升级线程分离属性失败 返回码[%d]", nAttrRet);
        pthread_attr_destroy(&attr_upgrade);
        free(pstUpgradeInfo);
        return ERR_CREATE;
    }

    /* step: 在线程创建前先标记为升级中，避免升级线程快速失败后被外层覆盖为升级中。 */
    enRetCode = dataManage_set_upgradeStatus(TI_UPGRADE_RUNING);
    if (enRetCode != OK)
    {
        cleanup_upload_directory_with_log("设置升级状态失败");
        dlog_error("设置升级状态失败 返回码[%d]", enRetCode);
        pthread_attr_destroy(&attr_upgrade);
        free(pstUpgradeInfo);
        return enRetCode;
    }

    int nRet = pthread_create(&tid, &attr_upgrade,
                              upgrade_process_thread, (void *)pstUpgradeInfo);
    if (nRet)
    {
        cleanup_upload_directory_with_log("升级线程创建失败");
        dataManage_set_upgradeStatus(TI_UPGRADE_RUNFAIL);
        dlog_error("开始升级线程失败");
        enRetCode = ERR_CREATE;
        free(pstUpgradeInfo);
    }
    else
    {
        dlog_info("创建升级线程成功");
    }

    pthread_attr_destroy(&attr_upgrade);

    return enRetCode;
}
