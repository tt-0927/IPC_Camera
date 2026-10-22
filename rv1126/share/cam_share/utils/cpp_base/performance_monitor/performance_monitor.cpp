/**
 * @FilePath     : performance_monitor.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-04-29 11:07:09
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-08 16:34:40
 * @Description  : 性能监控功能函数 用于监控程序运行时的CPU使用率、内存占用和NPU状态
 */

#include "performance_monitor.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>

#include "IpcRet.h"

/* 文件路径最大长度 */
#define MONITOR_MAX_PATH_LEN 256
/* 缓冲区最大长度 */
#define MONITOR_MAX_BUF_LEN 512
/* CPU使用率百分比转换系数 */
#define MONITOR_STATICS_CONVERT_NUM 100
/* 文件读取偏移量，用于从文件末尾开始向前读取 */
#define MONITOR_FILE_SEEK_POSITION (-512)
/* NPU设备ID，目前只支持设备0 */
#define MONITOR_NPU_DEVICE_ID 0
/* NPU信息文件路径 */
#define MONITOR_NPU_INFO_PATH "/proc/umap/svp_npu"
/* Linux 线程名上限为 16 字节，额外预留结束符和未知状态文本。 */
#define MONITOR_THREAD_NAME_LEN 32

/**
 * CPU信息结构体
 * 用于存储从/proc/stat或/proc/{pid}/stat文件中读取的CPU使用情况数据
 */
typedef struct MonitorCpuInfo
{
    unsigned long long ullUser;     /* 用户态CPU时间 */
    unsigned long long ullSys;      /* 系统态CPU时间 */
    unsigned long long ullNice;     /* 优先级调度的CPU时间 */
    unsigned long long ullIdle;     /* 空闲CPU时间 */
    unsigned long long ullUnknown1; /* iowait - I/O等待时间 */
    unsigned long long ullUnknown2; /* irq - 硬中断处理时间 */
    unsigned long long ullUnknown3; /* softirq - 软中断处理时间 */
} MonitorCpuInfo_S;

/**
 * 全局变量
 */
static pthread_t gs_monitorThreadId;             /* 监控线程ID */
static pthread_mutex_t gs_monitorLock;           /* 线程同步互斥锁 */
static bool gs_bMonitorIsRunning = false;        /* 监控线程运行状态标志 */
static unsigned int gs_uMonitorMmzInitValue = 0; /* MMZ内存初始使用值 */
static bool gs_bMonitorNpuAvailable = false;     /* NPU是否可用标志 */
static MonitorNpuInfo_S gs_stMonitorNpuInfoPrev; /* 上一次NPU信息，用于计算差值 */
/* memory: 检查点只保存上一份固定大小快照，不在业务线程中分配内存。 */
static pthread_mutex_t gs_memoryCheckpointLock = PTHREAD_MUTEX_INITIALIZER;
/* 上一次已经输出的进程内存快照。 */
static MonitorMemoryInfo_S gs_stMemoryCheckpointPrev;
/* 是否已经存在上一份快照，用于计算阶段增量。 */
static bool gs_bMemoryCheckpointHasPrev = false;
/* lock: 防止多个业务线程同时输出 smaps 明细导致串口行交错。 */
static pthread_mutex_t gs_threadStackLogLock = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief   : 单个线程栈映射的临时统计数据
 * @note    : 该结构只在测试检查点中使用，不作为业务状态保存。
 */
typedef struct MonitorThreadStackInfo
{
    unsigned long nTid;
    bool bMainStack;
    unsigned long nSizeKb;
    unsigned long nRssKb;
    unsigned long nPssKb;
    unsigned long nPrivateCleanKb;
    unsigned long nPrivateDirtyKb;
    unsigned long nSwapKb;
} MonitorThreadStackInfo_S;

/**
 * 内部函数前置声明
 */
/* 获取系统CPU数据 */
static void monitor_getCpuData(MonitorCpuInfo_S *pCpuInfo);
/* 获取指定进程的CPU数据 */
static void monitor_getCpuDataPid(unsigned int nPid, MonitorCpuInfo_S *pCpuInfo);
/* 计算指定进程的CPU使用率 */
static double monitor_getPidUsedCpu(unsigned int nPid);
/* 获取当前进程的系统内存使用情况 */
static int monitor_getProcMeminfoOsMem(void);
/* 获取系统MMZ内存使用情况 */
static unsigned int monitor_mmzMem(void);
/* 计算两个无符号 KB 计数的有符号差值，避免回收后发生下溢。 */
static long long monitor_memory_delta(unsigned long nCurrent, unsigned long nPrevious);
/* 判断 smaps 当前行是否为一条映射头。 */
static bool monitor_is_smaps_mapping_line(const char *pcLine);
/* 从 smaps 映射头解析 [stack] 或 [stack:<tid>]。 */
static bool monitor_parse_stack_mapping(const char *pcLine,
                                        unsigned long nDefaultTid,
                                        unsigned long *pnTid,
                                        bool *pbMainStack);
/* 读取线程 comm 名称，读取失败时使用 tid 作为回退名称。 */
static void monitor_read_thread_name(unsigned long nTid, char *pszName, std::size_t nNameSize);
/* 从单个线程的 task/smaps 中读取该线程栈映射。 */
static bool monitor_read_thread_stack_mapping(FILE *pSmapsFile,
                                              unsigned long nDefaultTid,
                                              MonitorThreadStackInfo_S *pstInfo);
/* 输出单个线程栈映射，并累加汇总数据。 */
static void monitor_emit_thread_stack(const char *pcStage,
                                      const MonitorThreadStackInfo_S *pstInfo,
                                      unsigned long *pnStackMaps,
                                      MonitorThreadStackInfo_S *pstTotal);
/* 输出无法从 task/smaps 定位栈映射的线程，避免把“未找到”误报成“栈为0”。 */
static void monitor_emit_missing_thread_stack(const char *pcStage, unsigned long nTid, const char *pcReason);
/* 输出所有线程的 smaps 栈映射。 */
static int monitor_log_thread_stack_checkpoint(const char *pcStage);
/* 输出进程内存快照，可选择是否更新相邻检查点的增量基线。 */
static int monitor_log_memory_snapshot_internal(const char *pcStage, bool bUpdateDeltaBase);
/* 测试版本的毫秒等待；正式版本不引入等待。 */
static void monitor_sleep_for_checkpoint(unsigned int nDelayMs);
/* 初始化NPU监控 */
static int monitor_npuInit(void);
/* 获取NPU使用信息 */
static int monitor_getNpuInfo(MonitorNpuInfo_S *pInfo);
/* 监控线程主函数，用于周期性收集并输出CPU和内存使用信息 */
static void *monitor_cpuMemCalc(void *pArgs);

/**
 * 初始化性能监控模块
 *
 * @return 成功返回0，失败返回负数
 */
int perfMonitor_init(void)
{
    if (gs_bMonitorIsRunning)
    {
        printf("[性能监控] 监控已经在运行中\n");
        return ERR;
    }

    /* 初始化互斥锁 */
    if (pthread_mutex_init(&gs_monitorLock, NULL) != 0)
    {
        printf("[性能监控] 互斥锁初始化失败\n");
        return ERR;
    }

    /* 获取初始MMZ内存使用值 */
    gs_uMonitorMmzInitValue = monitor_mmzMem();

    /* 初始化NPU监控 */
    monitor_npuInit();

    /* 创建监控线程 */
    gs_bMonitorIsRunning = true;
    if (pthread_create(&gs_monitorThreadId, NULL, monitor_cpuMemCalc, NULL) != 0)
    {
        printf("[性能监控] 创建监控线程失败\n");
        pthread_mutex_destroy(&gs_monitorLock);
        gs_bMonitorIsRunning = false;
        return ERR;
    }

    printf("[性能监控] 监控已启动\n");
    return OK;
}

/**
 * 去初始化性能监控模块
 *
 * @return 成功返回0，失败返回负数
 */
int perfMonitor_uninit(void)
{
    if (!gs_bMonitorIsRunning)
    {
        printf("[性能监控] 监控未运行\n");
        return ERR;
    }

    /* 设置停止标志 */
    pthread_mutex_lock(&gs_monitorLock);
    gs_bMonitorIsRunning = false;
    pthread_mutex_unlock(&gs_monitorLock);

    /* 等待线程结束 */
    if (pthread_join(gs_monitorThreadId, NULL) != 0)
    {
        printf("[性能监控] 等待监控线程结束失败\n");
        return ERR;
    }

    /* 销毁互斥锁 */
    pthread_mutex_destroy(&gs_monitorLock);

    printf("[性能监控] 监控已停止\n");
    return OK;
}

/**
 * 获取系统CPU数据
 *
 * @param pCpuInfo CPU信息结构体指针
 */
static void monitor_getCpuData(MonitorCpuInfo_S *pCpuInfo)
{
    char szTemp[MONITOR_MAX_PATH_LEN + 1] = {0};
    FILE *pFd = NULL;

    pFd = fopen("/proc/stat", "r");
    if (pFd == NULL)
    {
        printf("[性能监控] 打开/proc/stat文件失败\n");
        return;
    }

    memset(pCpuInfo, 0, sizeof(MonitorCpuInfo_S));
    if (fscanf(pFd, "%s %llu %llu %llu %llu %llu %llu %llu",
               szTemp,
               &(pCpuInfo->ullUser),
               &(pCpuInfo->ullSys),
               &(pCpuInfo->ullNice),
               &(pCpuInfo->ullIdle),
               &(pCpuInfo->ullUnknown1),
               &(pCpuInfo->ullUnknown2),
               &(pCpuInfo->ullUnknown3)) == -1)
    {
        printf("[性能监控] 读取CPU数据失败\n");
    }

    fclose(pFd);
}

/**
 * 获取指定进程的CPU数据
 *
 * @param nPid 进程ID
 * @param pCpuInfo CPU信息结构体指针
 */
static void monitor_getCpuDataPid(unsigned int nPid, MonitorCpuInfo_S *pCpuInfo)
{
    char szThreadStatFile[MONITOR_MAX_PATH_LEN + 1] = {0};
    FILE *pFd = NULL;

    if (snprintf(szThreadStatFile, MONITOR_MAX_PATH_LEN, "/proc/%u/stat", nPid) < 0)
    {
        printf("[性能监控] 生成进程状态文件路径失败\n");
        return;
    }

    pFd = fopen(szThreadStatFile, "r");
    if (pFd == NULL)
    {
        printf("[性能监控] 打开进程状态文件失败: %s\n", szThreadStatFile);
        return;
    }

    if (fscanf(pFd, "%*d %*s %*s %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %llu %llu",
               &(pCpuInfo->ullUser), &(pCpuInfo->ullSys)) == -1)
    {
        printf("[性能监控] 读取进程CPU数据失败\n");
    }

    fclose(pFd);
}

/**
 * 获取指定进程的CPU使用率
 *
 * @param nPid 进程ID
 * @return 进程CPU使用率(百分比)
 */
static double monitor_getPidUsedCpu(unsigned int nPid)
{
    double dUser = 0.0;
    double dSys = 0.0;
    double dNice = 0.0;
    double dIdle = 0.0;
    double dUnknown1 = 0.0;
    double dUnknown2 = 0.0;
    double dUnknown3 = 0.0;
    double dTotal = 0.0;
    double dCpuUsage = 0.0;
    MonitorCpuInfo_S stSysTotalOldCpu;
    MonitorCpuInfo_S stSysTotalNewCpu;
    MonitorCpuInfo_S stPidTotalOldCpu;
    MonitorCpuInfo_S stPidTotalNewCpu;

    monitor_getCpuData(&stSysTotalOldCpu);
    monitor_getCpuDataPid(nPid, &stPidTotalOldCpu);
    sleep(1);
    monitor_getCpuData(&stSysTotalNewCpu);
    monitor_getCpuDataPid(nPid, &stPidTotalNewCpu);

    dUser = (double)(stSysTotalNewCpu.ullUser - stSysTotalOldCpu.ullUser);
    dSys = (double)(stSysTotalNewCpu.ullSys - stSysTotalOldCpu.ullSys);
    dNice = (double)(stSysTotalNewCpu.ullNice - stSysTotalOldCpu.ullNice);
    dIdle = (double)(stSysTotalNewCpu.ullIdle - stSysTotalOldCpu.ullIdle);
    dUnknown1 = (double)(stSysTotalNewCpu.ullUnknown1 - stSysTotalOldCpu.ullUnknown1);
    dUnknown2 = (double)(stSysTotalNewCpu.ullUnknown2 - stSysTotalOldCpu.ullUnknown2);
    dUnknown3 = (double)(stSysTotalNewCpu.ullUnknown3 - stSysTotalOldCpu.ullUnknown3);
    dTotal = dUser + dSys + dNice + dIdle + dUnknown1 + dUnknown2 + dUnknown3;

    dUser = (double)(stPidTotalNewCpu.ullUser - stPidTotalOldCpu.ullUser);
    dSys = (double)(stPidTotalNewCpu.ullSys - stPidTotalOldCpu.ullSys);
    dCpuUsage += ((dUser + dSys) * MONITOR_STATICS_CONVERT_NUM / dTotal);

    return dCpuUsage;
}

/**
 * 获取当前进程的内存使用情况(OS内存)
 *
 * @return 内存使用量(KB)
 */
static int monitor_getProcMeminfoOsMem(void)
{
    char szFileName[MONITOR_MAX_PATH_LEN + 1] = {0};
    FILE *pFd = NULL;
    char szLine[MONITOR_MAX_PATH_LEN] = {0};
    int nVmrss = 0;

    pid_t nPid = getpid();
    if (sprintf(szFileName, "/proc/%d/status", nPid) < 0)
    {
        printf("[性能监控] 生成进程状态文件路径失败\n");
        return nVmrss;
    }

    pFd = fopen(szFileName, "r");
    if (pFd == NULL)
    {
        printf("[性能监控] 打开文件失败: %s\n", szFileName);
        return nVmrss;
    }

    /* 读取文件内容并获取VmRSS的值 */
    while (fgets(szLine, sizeof(szLine), pFd) != NULL)
    {
        if (sscanf(szLine, "VmRSS: %d kB", &nVmrss) == 1)
        {
            break;
        }
    }

    fclose(pFd);
    return nVmrss;
}

/**
 * 获取MMZ内存使用情况
 *
 * @return MMZ内存使用量(KB)
 */
static unsigned int monitor_mmzMem(void)
{
    FILE *pFd = NULL;
    char szBuf[MONITOR_MAX_BUF_LEN + 1] = {0};
    unsigned int nMem = 0;
    char *pContent = NULL;

    pFd = fopen("/proc/umap/media-mem", "r");
    if (pFd == NULL)
    {
        return nMem;
    }

    fseek(pFd, MONITOR_FILE_SEEK_POSITION, SEEK_END);
    while (fgets(szBuf, sizeof(szBuf) - 1, pFd) != NULL)
    {
        if ((strstr(szBuf, "total size") != NULL) && ((pContent = strstr(szBuf, "used=")) != NULL))
        {
            if (sscanf(pContent, "used=%dKB", &nMem) == -1)
            {
                printf("[性能监控] 读取MMZ内存数据失败\n");
            }
            break;
        }
    }

    fclose(pFd);
    return nMem;
}

/**
 * @brief   : 获取当前进程及系统的内存快照
 * @param   {MonitorMemoryInfo_S*} pInfo：输出内存快照，不能为 nullptr
 * @return  {int} OK：成功；ERR_PARAM_NULL：参数为空；ERR_OPEN：无法读取进程状态
 */
int perfMonitor_getMemoryInfo(MonitorMemoryInfo_S *pInfo)
{
    FILE *pStatusFile = NULL;
    FILE *pSmapsFile = NULL;
    FILE *pMeminfoFile = NULL;
    char szLine[MONITOR_MAX_PATH_LEN + 1] = {0};

    if (pInfo == NULL)
    {
        return ERR_PARAM_NULL;
    }

    memset(pInfo, 0, sizeof(MonitorMemoryInfo_S));

    /* /proc/self/status 能同时提供 RSS、峰值、匿名页和线程数，避免读取命令行工具。 */
    pStatusFile = fopen("/proc/self/status", "r");
    if (pStatusFile == NULL)
    {
        return ERR_OPEN;
    }

    while (fgets(szLine, sizeof(szLine), pStatusFile) != NULL)
    {
        (void)sscanf(szLine, "VmSize: %lu kB", &pInfo->nVmSizeKb);
        (void)sscanf(szLine, "VmPeak: %lu kB", &pInfo->nVmPeakKb);
        (void)sscanf(szLine, "VmRSS: %lu kB", &pInfo->nVmRssKb);
        (void)sscanf(szLine, "VmHWM: %lu kB", &pInfo->nVmHwmKb);
        (void)sscanf(szLine, "VmData: %lu kB", &pInfo->nVmDataKb);
        (void)sscanf(szLine, "VmStk: %lu kB", &pInfo->nVmStkKb);
        (void)sscanf(szLine, "RssAnon: %lu kB", &pInfo->nRssAnonKb);
        (void)sscanf(szLine, "RssFile: %lu kB", &pInfo->nRssFileKb);
        (void)sscanf(szLine, "RssShmem: %lu kB", &pInfo->nRssShmemKb);
        (void)sscanf(szLine, "VmSwap: %lu kB", &pInfo->nVmSwapKb);
        (void)sscanf(szLine, "Threads: %lu", &pInfo->nThreads);
    }
    fclose(pStatusFile);

    /* PSS 将共享库页按进程分摊，适合和 VmRSS 一起判断真实物理内存压力。 */
    pSmapsFile = fopen("/proc/self/smaps_rollup", "r");
    if (pSmapsFile != NULL)
    {
        while (fgets(szLine, sizeof(szLine), pSmapsFile) != NULL)
        {
            if (sscanf(szLine, "Pss: %lu kB", &pInfo->nPssKb) == 1)
            {
                break;
            }
        }
        fclose(pSmapsFile);
    }

    /* MemAvailable 是整机剩余可用内存，用于判断模块增量是否逼近 OOM 水位。 */
    pMeminfoFile = fopen("/proc/meminfo", "r");
    if (pMeminfoFile != NULL)
    {
        while (fgets(szLine, sizeof(szLine), pMeminfoFile) != NULL)
        {
            if (sscanf(szLine, "MemAvailable: %lu kB", &pInfo->nSystemAvailableKb) == 1)
            {
                break;
            }
        }
        fclose(pMeminfoFile);
    }

    /* MMZ/VB 等媒体内存通常不完整计入 VmRSS，单独保存平台统计值。 */
    pInfo->nMmzUsedKb = static_cast<unsigned long>(monitor_mmzMem());
    return OK;
}

/**
 * @brief   : 计算两个无符号内存计数的有符号差值
 * @param   {unsigned long} nCurrent：当前计数
 * @param   {unsigned long} nPrevious：上一次计数
 * @return  {long long} 当前计数减去上一次计数
 */
static long long monitor_memory_delta(unsigned long nCurrent, unsigned long nPrevious)
{
    return static_cast<long long>(nCurrent) - static_cast<long long>(nPrevious);
}

/**
 * @brief   : 判断 smaps 当前行是否为映射头
 * @param   {const char*} pcLine：smaps 文本行
 * @return  {bool} true：映射头；false：属性行或空行
 */
static bool monitor_is_smaps_mapping_line(const char *pcLine)
{
    if (pcLine == NULL)
    {
        return false;
    }

    unsigned long nStart = 0UL;
    unsigned long nEnd = 0UL;
    char szPermissions[8] = {0};
    return sscanf(pcLine, "%lx-%lx %7s", &nStart, &nEnd, szPermissions) == 3;
}

/**
 * @brief   : 从 smaps 映射头解析线程栈 tid
 * @param   {const char*} pcLine：smaps 映射头
 * @param   {unsigned long} nDefaultTid：task/smaps 对应的 tid，进程级 smaps 传 0
 * @param   {unsigned long*} pnTid：线程 tid 输出
 * @param   {bool*} pbMainStack：是否为主线程 [stack]
 * @return  {bool} true：当前映射是线程栈；false：其他映射
 */
static bool monitor_parse_stack_mapping(const char *pcLine,
                                       unsigned long nDefaultTid,
                                       unsigned long *pnTid,
                                       bool *pbMainStack)
{
    if (pcLine == NULL || pnTid == NULL || pbMainStack == NULL)
    {
        return false;
    }

    const char *pcStack = strstr(pcLine, "[stack");
    if (pcStack == NULL)
    {
        return false;
    }

    if (strncmp(pcStack, "[stack]", 7U) == 0)
    {
        *pnTid = nDefaultTid == 0UL ? static_cast<unsigned long>(getpid()) : nDefaultTid;
        *pbMainStack = *pnTid == static_cast<unsigned long>(getpid());
        return true;
    }

    if (strncmp(pcStack, "[stack:", 7U) != 0)
    {
        return false;
    }

    unsigned long nTid = 0UL;
    char cEnd = '\0';
    if (sscanf(pcStack + 7, "%lu%c", &nTid, &cEnd) != 2 || cEnd != ']' || nTid == 0UL)
    {
        return false;
    }

    *pnTid = nTid;
    *pbMainStack = false;
    return true;
}

/**
 * @brief   : 读取线程 comm 名称
 * @param   {unsigned long} nTid：线程 tid
 * @param   {char*} pszName：名称输出缓冲区
 * @param   {std::size_t} nNameSize：缓冲区长度
 * @return  {void}
 */
static void monitor_read_thread_name(unsigned long nTid, char *pszName, std::size_t nNameSize)
{
    if (pszName == NULL || nNameSize == 0U)
    {
        return;
    }

    pszName[0] = '\0';
    char szPath[MONITOR_MAX_PATH_LEN + 1] = {0};
    const int nPathLen = snprintf(szPath,
                                  sizeof(szPath),
                                  "/proc/self/task/%lu/comm",
                                  nTid);
    if (nPathLen > 0 && static_cast<std::size_t>(nPathLen) < sizeof(szPath))
    {
        FILE *pCommFile = fopen(szPath, "r");
        if (pCommFile != NULL)
        {
            (void)fgets(pszName, static_cast<int>(nNameSize), pCommFile);
            fclose(pCommFile);
        }
    }

    if (pszName[0] == '\0')
    {
        (void)snprintf(pszName, nNameSize, "tid-%lu", nTid);
        return;
    }

    for (std::size_t i = 0U; i < nNameSize && pszName[i] != '\0'; ++i)
    {
        if (pszName[i] == '\r' || pszName[i] == '\n' || pszName[i] == '\t' || pszName[i] == ' ')
        {
            pszName[i] = '_';
        }
    }
}

/**
 * @brief   : 从单个线程的 task/smaps 中读取线程栈统计
 * @param   {FILE*} pSmapsFile：已打开的 /proc/self/task/<tid>/smaps 文件
 * @param   {unsigned long} nDefaultTid：当前 task 目录对应的 tid
 * @param   {MonitorThreadStackInfo_S*} pstInfo：线程栈统计输出
 * @return  {bool} true：成功找到栈映射；false：未找到或读取失败
 * @note    : 不同 Linux 内核对进程级 smaps 的工作线程栈命名不一致，
 *            以 task/<tid>/smaps 中的 [stack] 作为主路径，避免依赖 [stack:<tid>]。
 */
static bool monitor_read_thread_stack_mapping(FILE *pSmapsFile,
                                              unsigned long nDefaultTid,
                                              MonitorThreadStackInfo_S *pstInfo)
{
    if (pSmapsFile == NULL || pstInfo == NULL || nDefaultTid == 0UL)
    {
        return false;
    }

    char szLine[MONITOR_MAX_BUF_LEN + 1] = {0};
    MonitorThreadStackInfo_S stCurrent = {0};
    bool bInStackMapping = false;

    while (fgets(szLine, sizeof(szLine), pSmapsFile) != NULL)
    {
        if (monitor_is_smaps_mapping_line(szLine))
        {
            if (bInStackMapping)
            {
                *pstInfo = stCurrent;
                return true;
            }

            memset(&stCurrent, 0, sizeof(stCurrent));
            unsigned long nTid = 0UL;
            bool bMainStack = false;
            bInStackMapping = monitor_parse_stack_mapping(szLine, nDefaultTid, &nTid, &bMainStack);
            if (bInStackMapping && nTid != nDefaultTid)
            {
                /* review: task/smaps 若仍带有其它线程标签，只接受当前 task 对应的映射。 */
                bInStackMapping = false;
            }
            if (bInStackMapping)
            {
                stCurrent.nTid = nDefaultTid;
                stCurrent.bMainStack = bMainStack;
            }
            continue;
        }

        if (!bInStackMapping)
        {
            continue;
        }

        (void)sscanf(szLine, "Size: %lu kB", &stCurrent.nSizeKb);
        (void)sscanf(szLine, "Rss: %lu kB", &stCurrent.nRssKb);
        (void)sscanf(szLine, "Pss: %lu kB", &stCurrent.nPssKb);
        (void)sscanf(szLine, "Private_Clean: %lu kB", &stCurrent.nPrivateCleanKb);
        (void)sscanf(szLine, "Private_Dirty: %lu kB", &stCurrent.nPrivateDirtyKb);
        (void)sscanf(szLine, "Swap: %lu kB", &stCurrent.nSwapKb);
    }

    if (bInStackMapping)
    {
        *pstInfo = stCurrent;
        return true;
    }
    return false;
}

/**
 * @brief   : 输出单个线程栈映射并累加汇总值
 * @param   {const char*} pcStage：统计阶段名称
 * @param   {const MonitorThreadStackInfo_S*} pstInfo：单个线程栈数据
 * @param   {unsigned long*} pnStackMaps：栈映射数量输出
 * @param   {MonitorThreadStackInfo_S*} pstTotal：汇总数据输出
 * @return  {void}
 */
static void monitor_emit_thread_stack(const char *pcStage,
                                      const MonitorThreadStackInfo_S *pstInfo,
                                      unsigned long *pnStackMaps,
                                      MonitorThreadStackInfo_S *pstTotal)
{
    if (pcStage == NULL || pstInfo == NULL || pnStackMaps == NULL || pstTotal == NULL)
    {
        return;
    }

    char szThreadName[MONITOR_THREAD_NAME_LEN] = {0};
    monitor_read_thread_name(pstInfo->nTid, szThreadName, sizeof(szThreadName));
    printf("[THREADSTACK] stage=%s source=task_smaps tid=%lu name=%s main=%d found=1 Size=%lu Rss=%lu Pss=%lu "
           "PrivateClean=%lu PrivateDirty=%lu Swap=%lu\n",
           pcStage,
           pstInfo->nTid,
           szThreadName,
           pstInfo->bMainStack ? 1 : 0,
           pstInfo->nSizeKb,
           pstInfo->nRssKb,
           pstInfo->nPssKb,
           pstInfo->nPrivateCleanKb,
           pstInfo->nPrivateDirtyKb,
           pstInfo->nSwapKb);

    ++(*pnStackMaps);
    pstTotal->nSizeKb += pstInfo->nSizeKb;
    pstTotal->nRssKb += pstInfo->nRssKb;
    pstTotal->nPssKb += pstInfo->nPssKb;
    pstTotal->nPrivateCleanKb += pstInfo->nPrivateCleanKb;
    pstTotal->nPrivateDirtyKb += pstInfo->nPrivateDirtyKb;
    pstTotal->nSwapKb += pstInfo->nSwapKb;
}

/**
 * @brief   : 输出未找到线程栈映射的状态
 * @param   {const char*} pcStage：统计阶段名称
 * @param   {unsigned long} nTid：线程 tid
 * @param   {const char*} pcReason：未找到原因
 * @return  {void}
 */
static void monitor_emit_missing_thread_stack(const char *pcStage, unsigned long nTid, const char *pcReason)
{
    if (pcStage == NULL || pcReason == NULL)
    {
        return;
    }

    char szThreadName[MONITOR_THREAD_NAME_LEN] = {0};
    monitor_read_thread_name(nTid, szThreadName, sizeof(szThreadName));
    printf("[THREADSTACK] stage=%s source=task_smaps tid=%lu name=%s main=%d found=0 "
           "reason=%s Size=0 Rss=0 Pss=0 PrivateClean=0 PrivateDirty=0 Swap=0\n",
           pcStage,
           nTid,
           szThreadName,
           nTid == static_cast<unsigned long>(getpid()) ? 1 : 0,
           pcReason);
}

/**
 * @brief   : 读取并输出所有线程的 smaps 栈映射
 * @param   {const char*} pcStage：统计阶段名称
 * @return  {int} OK：成功；非OK：读取失败
 */
static int monitor_log_thread_stack_checkpoint(const char *pcStage)
{
    DIR *pTaskDir = NULL;
    struct dirent *pstEntry = NULL;
    MonitorThreadStackInfo_S stTotal = {0};
    const unsigned long nPid = static_cast<unsigned long>(getpid());
    unsigned long nThreadCount = 0UL;
    unsigned long nStackMaps = 0UL;
    unsigned long nMissingStacks = 0UL;

    if (pcStage == NULL)
    {
        return ERR_PARAM_NULL;
    }

    if (pthread_mutex_lock(&gs_threadStackLogLock) != 0)
    {
        return ERR;
    }

    pTaskDir = opendir("/proc/self/task");
    if (pTaskDir == NULL)
    {
        (void)pthread_mutex_unlock(&gs_threadStackLogLock);
        return ERR_OPEN;
    }

    while ((pstEntry = readdir(pTaskDir)) != NULL)
    {
        unsigned long nTid = 0UL;
        char cEnd = '\0';
        if (sscanf(pstEntry->d_name, "%lu%c", &nTid, &cEnd) != 1 || nTid == 0UL)
        {
            continue;
        }

        ++nThreadCount;
        char szSmapsPath[MONITOR_MAX_PATH_LEN + 1] = {0};
        const int nPathLen = snprintf(szSmapsPath,
                                      sizeof(szSmapsPath),
                                      "/proc/self/task/%lu/smaps",
                                      nTid);
        if (nPathLen <= 0 || static_cast<std::size_t>(nPathLen) >= sizeof(szSmapsPath))
        {
            ++nMissingStacks;
            monitor_emit_missing_thread_stack(pcStage, nTid, "path_too_long");
            continue;
        }

        FILE *pSmapsFile = fopen(szSmapsPath, "r");
        if (pSmapsFile == NULL)
        {
            ++nMissingStacks;
            monitor_emit_missing_thread_stack(pcStage, nTid, "open_failed");
            continue;
        }

        MonitorThreadStackInfo_S stCurrent = {0};
        const bool bFound = monitor_read_thread_stack_mapping(pSmapsFile, nTid, &stCurrent);
        fclose(pSmapsFile);
        if (!bFound)
        {
            ++nMissingStacks;
            monitor_emit_missing_thread_stack(pcStage, nTid, "stack_mapping_not_found");
            continue;
        }

        stCurrent.nTid = nTid;
        stCurrent.bMainStack = nTid == nPid;
        monitor_emit_thread_stack(pcStage, &stCurrent, &nStackMaps, &stTotal);
    }
    closedir(pTaskDir);

    printf("[THREADSTACK_SUMMARY] stage=%s source=task_smaps pid=%d threads=%lu maps=%lu missing=%lu "
           "Size=%lu Rss=%lu Pss=%lu PrivateClean=%lu PrivateDirty=%lu Swap=%lu\n",
           pcStage,
           static_cast<int>(getpid()),
           nThreadCount,
           nStackMaps,
           nMissingStacks,
           stTotal.nSizeKb,
           stTotal.nRssKb,
           stTotal.nPssKb,
           stTotal.nPrivateCleanKb,
           stTotal.nPrivateDirtyKb,
           stTotal.nSwapKb);
    (void)fflush(stdout);
    (void)pthread_mutex_unlock(&gs_threadStackLogLock);
    return OK;
}

/**
 * @brief   : 输出进程内存快照
 * @param   {const char*} pcStage：统计阶段名称
 * @param   {bool} bUpdateDeltaBase：是否更新相邻检查点基线
 * @return  {int} OK：成功；非OK：读取或加锁失败
 */
static int monitor_log_memory_snapshot_internal(const char *pcStage, bool bUpdateDeltaBase)
{
    if (pcStage == NULL)
    {
        return ERR_PARAM_NULL;
    }

#if ENABLE_MEMORY_CHECKPOINT
    MonitorMemoryInfo_S stCurrent;
    const int nRet = perfMonitor_getMemoryInfo(&stCurrent);
    if (nRet < OK)
    {
        return nRet;
    }

    if (pthread_mutex_lock(&gs_memoryCheckpointLock) != 0)
    {
        return ERR;
    }

    long long nDeltaVmRss = 0;
    long long nDeltaPss = 0;
    long long nDeltaVmData = 0;
    long long nDeltaMmz = 0;
    if (bUpdateDeltaBase && gs_bMemoryCheckpointHasPrev)
    {
        nDeltaVmRss = monitor_memory_delta(stCurrent.nVmRssKb, gs_stMemoryCheckpointPrev.nVmRssKb);
        nDeltaPss = monitor_memory_delta(stCurrent.nPssKb, gs_stMemoryCheckpointPrev.nPssKb);
        nDeltaVmData = monitor_memory_delta(stCurrent.nVmDataKb, gs_stMemoryCheckpointPrev.nVmDataKb);
        nDeltaMmz = monitor_memory_delta(stCurrent.nMmzUsedKb, gs_stMemoryCheckpointPrev.nMmzUsedKb);
    }

    /* grep 友好的一行格式，字段名固定，便于串口日志或脚本直接导入表格。 */
    printf("%s stage=%s pid=%d VmRSS=%lu Pss=%lu VmHWM=%lu VmSize=%lu VmData=%lu "
           "VmStk=%lu RssAnon=%lu RssFile=%lu RssShmem=%lu VmSwap=%lu Threads=%lu "
           "MemAvailable=%lu MMZUsed=%lu dVmRSS=%lld dPss=%lld dVmData=%lld dMMZ=%lld\n",
           bUpdateDeltaBase ? "[MEMCHECK]" : "[MEMSNAP]",
           pcStage,
           static_cast<int>(getpid()),
           stCurrent.nVmRssKb,
           stCurrent.nPssKb,
           stCurrent.nVmHwmKb,
           stCurrent.nVmSizeKb,
           stCurrent.nVmDataKb,
           stCurrent.nVmStkKb,
           stCurrent.nRssAnonKb,
           stCurrent.nRssFileKb,
           stCurrent.nRssShmemKb,
           stCurrent.nVmSwapKb,
           stCurrent.nThreads,
           stCurrent.nSystemAvailableKb,
           stCurrent.nMmzUsedKb,
           nDeltaVmRss,
           nDeltaPss,
           nDeltaVmData,
           nDeltaMmz);
    (void)fflush(stdout);

    if (bUpdateDeltaBase)
    {
        gs_stMemoryCheckpointPrev = stCurrent;
        gs_bMemoryCheckpointHasPrev = true;
    }
    (void)pthread_mutex_unlock(&gs_memoryCheckpointLock);
#else
    (void)bUpdateDeltaBase;
#endif

#if ENABLE_THREAD_STACK_CHECKPOINT
    const int nStackRet = monitor_log_thread_stack_checkpoint(pcStage);
    if (nStackRet != OK)
    {
        printf("[THREADSTACK] stage=%s error=%d\n", pcStage, nStackRet);
#if !ENABLE_MEMORY_CHECKPOINT
        return nStackRet;
#endif
    }
#endif
    return OK;
}

/**
 * @brief   : 测试版本延迟等待
 * @param   {unsigned int} nDelayMs：等待时间，单位为毫秒
 * @return  {void}
 */
static void monitor_sleep_for_checkpoint(unsigned int nDelayMs)
{
#if ENABLE_MEMORY_CHECKPOINT || ENABLE_THREAD_STACK_CHECKPOINT
    /* 分段等待避免把毫秒数直接转换为 useconds_t 后在 32 位平台溢出。 */
    while (nDelayMs >= 1000U)
    {
        (void)sleep(1);
        nDelayMs -= 1000U;
    }
    if (nDelayMs > 0U)
    {
        (void)usleep(static_cast<useconds_t>(nDelayMs) * 1000U);
    }
#else
    (void)nDelayMs;
#endif
}

/**
 * @brief   : 输出带阶段名称的内存检查点
 * @param   {const char*} pcStage：检查点名称
 * @return  {int} OK：成功；ERR_PARAM_NULL：阶段名称为空；其他值表示快照读取失败
 */
int perfMonitor_logMemoryCheckpoint(const char *pcStage)
{
    if (pcStage == NULL)
    {
        return ERR_PARAM_NULL;
    }

    return monitor_log_memory_snapshot_internal(pcStage, true);
}

/**
 * @brief   : 输出不改变增量基线的进程内存快照
 * @param   {const char*} pcStage：快照阶段名称
 * @return  {int} OK：成功；ERR_PARAM_NULL：阶段名称为空；其他值表示快照读取失败
 */
int perfMonitor_logMemorySnapshot(const char *pcStage)
{
    if (pcStage == NULL)
    {
        return ERR_PARAM_NULL;
    }

    return monitor_log_memory_snapshot_internal(pcStage, false);
}

/**
 * @brief   : 延迟指定时间后输出内存检查点
 * @param   {const char*} pcStage：检查点名称
 * @param   {unsigned int} nDelayMs：等待时间，单位为毫秒
 * @return  {int} OK：成功；ERR_PARAM_NULL：阶段名称为空；其他值表示快照读取失败
 */
int perfMonitor_logMemoryCheckpointDelayed(const char *pcStage, unsigned int nDelayMs)
{
    if (pcStage == NULL)
    {
        return ERR_PARAM_NULL;
    }

    monitor_sleep_for_checkpoint(nDelayMs);
    return perfMonitor_logMemoryCheckpoint(pcStage);
}

/**
 * @brief   : 延迟指定时间后输出不改变增量基线的进程内存快照
 * @param   {const char*} pcStage：快照阶段名称
 * @param   {unsigned int} nDelayMs：等待时间，单位为毫秒
 * @return  {int} OK：成功；ERR_PARAM_NULL：阶段名称为空；其他值表示快照读取失败
 */
int perfMonitor_logMemorySnapshotDelayed(const char *pcStage, unsigned int nDelayMs)
{
    if (pcStage == NULL)
    {
        return ERR_PARAM_NULL;
    }

    monitor_sleep_for_checkpoint(nDelayMs);
    return perfMonitor_logMemorySnapshot(pcStage);
}

/**
 * @brief   : 输出当前进程全部线程的 smaps 栈映射明细
 * @param   {const char*} pcStage：统计阶段名称
 * @return  {int} OK：成功；ERR_PARAM_NULL：阶段名称为空；其他值表示 smaps 读取失败
 */
int perfMonitor_logThreadStackCheckpoint(const char *pcStage)
{
    if (pcStage == NULL)
    {
        return ERR_PARAM_NULL;
    }

#if ENABLE_THREAD_STACK_CHECKPOINT
    return monitor_log_thread_stack_checkpoint(pcStage);
#else
    (void)pcStage;
    return OK;
#endif
}

/**
 * 初始化NPU监控
 *
 * @return 成功返回0，失败返回负数
 */
static int monitor_npuInit(void)
{
    FILE *pFd = NULL;

    /* 尝试打开NPU信息文件验证NPU是否可用 */
    pFd = fopen(MONITOR_NPU_INFO_PATH, "r");
    if (pFd == NULL)
    {
        printf("[性能监控] NPU设备不可用，无法打开%s\n", MONITOR_NPU_INFO_PATH);
        gs_bMonitorNpuAvailable = false;
        return ERR;
    }

    /* NPU可用，关闭文件并设置标志 */
    fclose(pFd);
    gs_bMonitorNpuAvailable = true;

    /* 初始化上一次的NPU信息为0 */
    memset(&gs_stMonitorNpuInfoPrev, 0, sizeof(MonitorNpuInfo_S));

    printf("[性能监控] NPU监控初始化成功\n");
    return OK;
}

/**
 * 获取NPU使用信息
 *
 * @param pInfo NPU信息结构体指针
 * @return 成功返回0，失败返回负数
 */
static int monitor_getNpuInfo(MonitorNpuInfo_S *pInfo)
{
    FILE *pFd = NULL;
    char szBuf[MONITOR_MAX_BUF_LEN + 1] = {0};
    char szLine[MONITOR_MAX_BUF_LEN + 1] = {0};
    int nDeviceId = -1;

    if (!gs_bMonitorNpuAvailable || pInfo == NULL)
    {
        return ERR;
    }

    /* 清空结构体 */
    memset(pInfo, 0, sizeof(MonitorNpuInfo_S));

    pFd = fopen(MONITOR_NPU_INFO_PATH, "r");
    if (pFd == NULL)
    {
        printf("[性能监控] 打开NPU信息文件失败\n");
        return ERR;
    }

    /* 读取NPU信息文件 */
    while (fgets(szLine, sizeof(szLine) - 1, pFd) != NULL)
    {
        /* 查找设备ID所在行 */
        if (strstr(szLine, "device_id") != NULL && strstr(szLine, "hw_status") != NULL)
        {
            /* 跳过该行，读取下一行获取实际数据 */
            if (fgets(szBuf, sizeof(szBuf) - 1, pFd) != NULL)
            {
                if (sscanf(szBuf, "%d %d %*d %*d %*d %d",
                           &nDeviceId, &pInfo->nHwStatus, &pInfo->nMacUtilization) < 3)
                {
                    printf("[性能监控] 解析NPU设备信息失败\n");
                }

                /* 只处理指定设备ID的NPU */
                if (nDeviceId != MONITOR_NPU_DEVICE_ID)
                {
                    continue;
                }
            }
        }

        /* 查找错误计数和超时计数行 */
        if (strstr(szLine, "timeout_err_cnt") != NULL && strstr(szLine, "hw_err_cnt") != NULL)
        {
            /* 跳过该行，读取下一行获取实际数据 */
            if (fgets(szBuf, sizeof(szBuf) - 1, pFd) != NULL)
            {
                if (sscanf(szBuf, "%d %d %d %*d %d %lld",
                           &pInfo->nTimeoutErrCnt, &pInfo->nHwErrCnt, &pInfo->nAicpuErrCnt,
                           &pInfo->nHwUtilization, &pInfo->llTotalRunningTime) < 5)
                {
                    printf("[性能监控] 解析NPU错误信息和利用率失败\n");
                }
            }
        }

        /* 查找中断计数行 */
        if (strstr(szLine, "irq_cnt_last_sec") != NULL && strstr(szLine, "max_irq_cnt_per_sec") != NULL)
        {
            /* 跳过该行，读取下一行获取实际数据 */
            if (fgets(szBuf, sizeof(szBuf) - 1, pFd) != NULL)
            {
                if (sscanf(szBuf, "%*d %d %d %d",
                           &pInfo->nIrqCntLastSec, &pInfo->nMaxIrqCntPerSec, &pInfo->nTotalIrqCnt) < 3)
                {
                    printf("[性能监控] 解析NPU中断信息失败\n");
                }
            }
        }
    }

    fclose(pFd);
    return OK;
}

/**
 * 获取当前NPU使用信息
 *
 * @param pInfo NPU信息结构体指针
 * @return 成功返回0，失败返回负数
 */
int perfMonitor_getNpuInfo(MonitorNpuInfo_S *pInfo)
{
    if (!gs_bMonitorNpuAvailable || pInfo == NULL)
    {
        /* NPU不可用或参数无效 */
        memset(pInfo, 0, sizeof(MonitorNpuInfo_S));
        return ERR;
    }

    return monitor_getNpuInfo(pInfo);
}

/**
 * @brief   : 获取网卡最大带宽 (Mbps)
 * @param    {string&} strAdapterName：网卡名称 eg：eth0
 * @return   {double}网卡最大带宽
 */
double getMaxBandwidth(const std::string &strAdapterName)
{
    std::string speed_path = "/sys/class/net/" + strAdapterName + "/speed";
    std::ifstream speed_file(speed_path);

    if (!speed_file.is_open())
    {
        std::cerr << "无法读取网卡速度文件: " << speed_path << std::endl;
        return -1;
    }

    int speed_mbps;
    speed_file >> speed_mbps;
    speed_file.close();

    return static_cast<double>(speed_mbps);
}

/**
 * @brief   : 读取网卡统计信息
 * @param    {string&} strAdapterName：网卡名称 eg：eth0
 * @param    {NetworkStats&} stStats：网络统计信息
 * @return   {bool} true：成功 false：失败
 */
bool getNetworkStats(const std::string &strAdapterName, NetworkStats &stStats)
{
    std::ifstream proc_file("/proc/net/dev");
    if (!proc_file.is_open())
    {
        std::cerr << "无法打开 /proc/net/dev" << std::endl;
        return false;
    }

    std::string line;
    bool found = false;

    // 跳过前两行标题
    std::getline(proc_file, line);
    std::getline(proc_file, line);

    while (std::getline(proc_file, line))
    {
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos)
            continue;

        std::string iface_name = line.substr(0, colon_pos);
        // 去除空白字符
        iface_name.erase(0, iface_name.find_first_not_of(" \t"));
        iface_name.erase(iface_name.find_last_not_of(" \t") + 1);

        if (iface_name == strAdapterName)
        {
            std::string data = line.substr(colon_pos + 1);
            if (sscanf(data.c_str(), "%lu %lu %*u %*u %*u %*u %*u %*u %lu %lu",
                       &stStats.rx_bytes, &stStats.rx_packets,
                       &stStats.tx_bytes, &stStats.tx_packets) == 4)
            {
                found = true;
                break;
            }
        }
    }

    proc_file.close();
    return found;
}

/**
 * @brief   : 计算带宽使用率和剩余带宽
 * @param    {string&} strAdapterName：网卡名称 eg：eth0
 * @param    {BandwidthInfo&} stBandwidthInfo：带宽信息
 * @return   {bool} true：成功 false：失败
 */
bool getAvailableBandwidth(const std::string &strAdapterName, BandwidthInfo &stBandwidthInfo)
{
    NetworkStats stats1, stats2;

    // 获取网卡最大带宽
    stBandwidthInfo.max_bandwidth_mbps = getMaxBandwidth(strAdapterName);
    if (stBandwidthInfo.max_bandwidth_mbps < 0)
    {
        return false;
    }

    // 第一次采样
    if (!getNetworkStats(strAdapterName, stats1))
    {
        std::cerr << "无法获取网卡 " << strAdapterName << " 的统计信息" << std::endl;
        return false;
    }

    // 等待1秒
    sleep(1);

    // 第二次采样
    if (!getNetworkStats(strAdapterName, stats2))
    {
        std::cerr << "无法获取网卡 " << strAdapterName << " 的统计信息" << std::endl;
        return false;
    }

    // 计算1秒内的字节数变化
    unsigned long rx_bytes_diff = stats2.rx_bytes - stats1.rx_bytes;
    unsigned long tx_bytes_diff = stats2.tx_bytes - stats1.tx_bytes;

    // 转换为Mbps (字节/秒 * 8 / 1000000)
    stBandwidthInfo.current_rx_mbps = (rx_bytes_diff * 8.0) / 1000000.0;
    stBandwidthInfo.current_tx_mbps = (tx_bytes_diff * 8.0) / 1000000.0;

    // 计算剩余带宽
    stBandwidthInfo.available_rx_mbps = stBandwidthInfo.max_bandwidth_mbps - stBandwidthInfo.current_rx_mbps;
    stBandwidthInfo.available_tx_mbps = stBandwidthInfo.max_bandwidth_mbps - stBandwidthInfo.current_tx_mbps;

    // 确保剩余带宽不为负数
    if (stBandwidthInfo.available_rx_mbps < 0)
        stBandwidthInfo.available_rx_mbps = 0;
    if (stBandwidthInfo.available_tx_mbps < 0)
        stBandwidthInfo.available_tx_mbps = 0;

    return true;
}

/**
 * @brief   : 获取并打印eth0网卡的剩余带宽
 * @param    {string&} strAdapterName：网卡名称 eg：eth0
 */
void printBandwidthInfo(const std::string &strAdapterName)
{
    BandwidthInfo stBandwidthInfo;

    if (getAvailableBandwidth(strAdapterName, stBandwidthInfo))
    {
        std::cout << "网卡: " << strAdapterName << std::endl;
        std::cout << "最大带宽: " << stBandwidthInfo.max_bandwidth_mbps << " Mbps" << std::endl;
        std::cout << "当前接收速率: " << stBandwidthInfo.current_rx_mbps << " Mbps" << std::endl;
        std::cout << "当前发送速率: " << stBandwidthInfo.current_tx_mbps << " Mbps" << std::endl;
        std::cout << "剩余接收带宽: " << stBandwidthInfo.available_rx_mbps << " Mbps" << std::endl;
        std::cout << "剩余发送带宽: " << stBandwidthInfo.available_tx_mbps << " Mbps" << std::endl;
        std::cout << "接收带宽使用率: " << (stBandwidthInfo.current_rx_mbps / stBandwidthInfo.max_bandwidth_mbps * 100) << "%" << std::endl;
        std::cout << "发送带宽使用率: " << (stBandwidthInfo.current_tx_mbps / stBandwidthInfo.max_bandwidth_mbps * 100) << "%" << std::endl;
    }
    else
    {
        std::cout << "获取网卡 " << strAdapterName << " 带宽信息失败" << std::endl;
    }
}

/**
 * CPU、内存和NPU计算线程函数
 *
 * @param pArgs 线程参数(未使用)
 * @return NULL
 */
static void *monitor_cpuMemCalc(void *pArgs)
{
    printf("[性能监控] 监控线程已启动\n");
    unsigned long long nLoopCnt = 0;
    double dCpuUsed = 0.0;
    double dVmRessUsed = 0.0;
    int nOsMemUseTmp = 0;
    unsigned int nMmzMemTmp = 0;
    double dMmzMem = 0.0;
    double dCpuUsedTmp = 0;
    MonitorNpuInfo_S stNpuInfo;
    double dAvgMacUtilization = 0.0;
    double dAvgHwUtilization = 0.0;

    /*等待程序起立完成*/
    sleep(5);
    while (1)
    {
        pthread_mutex_lock(&gs_monitorLock);
        if (!gs_bMonitorIsRunning)
        {
            pthread_mutex_unlock(&gs_monitorLock);
            break;
        }
        pthread_mutex_unlock(&gs_monitorLock);

        nOsMemUseTmp = monitor_getProcMeminfoOsMem();
        nMmzMemTmp = monitor_mmzMem() - gs_uMonitorMmzInitValue;
        dCpuUsedTmp = monitor_getPidUsedCpu((unsigned int)getpid());

        /* 输出基本的CPU和内存监控信息 */
        printf("[性能监控] CPU使用率 = %.2f%%, 系统内存使用 = %d KB, MMZ内存使用 = %d KB",
               dCpuUsedTmp, nOsMemUseTmp, nMmzMemTmp);
        
        /* 添加NPU监控信息 */
        if (gs_bMonitorNpuAvailable && monitor_getNpuInfo(&stNpuInfo) == 0)
        {
            printf(", NPU MAC利用率 = %d%%, NPU HW利用率 = %d%%\n",
                   stNpuInfo.nMacUtilization, stNpuInfo.nHwUtilization);

            /* 累加NPU利用率以计算平均值 */
            dAvgMacUtilization += stNpuInfo.nMacUtilization;
            dAvgHwUtilization += stNpuInfo.nHwUtilization;
        }
        else
        {
            printf("\n");
        }

        /*获取并打印eth0网卡的剩余带宽*/
        printBandwidthInfo("eth0");

        dCpuUsed += dCpuUsedTmp;
        dMmzMem += nMmzMemTmp;
        dVmRessUsed += (double)nOsMemUseTmp;
        ++nLoopCnt;
        sleep(1);
    }

    if (nLoopCnt)
    {
        printf("[性能监控] 统计次数: %llu, 平均CPU使用率 = %.2f%%, 平均系统内存使用 = %.2f KB, 平均MMZ内存使用 = %.2f KB",
               nLoopCnt, dCpuUsed / (double)nLoopCnt, dVmRessUsed / (double)nLoopCnt, dMmzMem / (double)nLoopCnt);

        /* 输出NPU平均利用率 */
        if (gs_bMonitorNpuAvailable)
        {
            printf(", 平均NPU MAC利用率 = %.2f%%, 平均NPU HW利用率 = %.2f%%\n",
                   dAvgMacUtilization / (double)nLoopCnt, dAvgHwUtilization / (double)nLoopCnt);
        }
        else
        {
            printf("\n");
        }
    }

    printf("[性能监控] 监控线程已结束\n");
    return NULL;
}
