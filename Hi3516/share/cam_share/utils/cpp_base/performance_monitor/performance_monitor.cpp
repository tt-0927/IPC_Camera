/**
 * @FilePath     : performance_monitor.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-04-29 11:07:09
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-25 19:30:51
 * @Description  : 性能监控功能函数 用于监控程序运行时的CPU使用率、内存占用和NPU状态
 */

#include "performance_monitor.h"

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