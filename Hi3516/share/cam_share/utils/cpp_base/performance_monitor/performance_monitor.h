/**
 * @FilePath     : performance_monitor.h
 * @Author       : zhouzirui
 * @Date         : 2025-04-29 11:07:07
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-08 16:45:03
 * @Description  : 性能监控头文件 提供监控CPU、内存和NPU使用的功能接口
 */

#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <cstring>

/*
 * 只在需要做板端内存分阶段统计的测试版本中打开。关闭时，调用点会被编译
 * 成无副作用的快速返回，不引入常驻线程，也不会改变正式版本的运行行为。
 */
#ifndef ENABLE_MEMORY_CHECKPOINT
#define ENABLE_MEMORY_CHECKPOINT 0
#endif

/*
 * 线程栈明细默认跟随分阶段内存检查开启；如需只看进程总量，可在测试构建中显式设为 0。
 * 该统计只读取 /proc/self/smaps，不创建常驻线程，也不影响正式构建的默认行为。
 */
#ifndef ENABLE_THREAD_STACK_CHECKPOINT
#define ENABLE_THREAD_STACK_CHECKPOINT ENABLE_MEMORY_CHECKPOINT
#endif

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
}

/**
 * @brief   : 进程内存快照
 * @note    : 所有数值单位均为 KB；MMZUsed 不属于进程 VmRSS，必须单独观察。
 */
typedef struct MonitorMemoryInfo
{
    /* 进程虚拟地址空间。 */
    unsigned long nVmSizeKb;
    /* 进程虚拟地址空间峰值。 */
    unsigned long nVmPeakKb;
    /* 进程驻留内存。 */
    unsigned long nVmRssKb;
    /* 进程驻留内存峰值。 */
    unsigned long nVmHwmKb;
    /* 数据段、堆等进程私有虚拟内存。 */
    unsigned long nVmDataKb;
    /* 主线程栈虚拟内存。 */
    unsigned long nVmStkKb;
    /* 按共享页分摊后的进程驻留内存。 */
    unsigned long nPssKb;
    /* 匿名页驻留内存。 */
    unsigned long nRssAnonKb;
    /* 文件页驻留内存。 */
    unsigned long nRssFileKb;
    /* 共享内存驻留内存。 */
    unsigned long nRssShmemKb;
    /* 已换出的进程页。 */
    unsigned long nVmSwapKb;
    /* 进程线程数。 */
    unsigned long nThreads;
    /* 系统 MemAvailable。 */
    unsigned long nSystemAvailableKb;
    /* /proc/umap/media-mem 中的 used。 */
    unsigned long nMmzUsedKb;
} MonitorMemoryInfo_S;

/**
 * NPU监控信息结构体
 * 包含NPU使用率和相关统计信息
 */
typedef struct MonitorNpuInfo
{
    int nHwStatus;                /* NPU硬件状态 */
    int nMacUtilization;          /* MAC(乘加运算单元)利用率 */
    int nHwUtilization;           /* 硬件利用率 */
    int nIrqCntLastSec;           /* 上一秒中断次数 */
    int nMaxIrqCntPerSec;         /* 每秒最大中断次数 */
    int nTotalIrqCnt;             /* 总中断次数 */
    int nTimeoutErrCnt;           /* 超时错误次数 */
    int nHwErrCnt;                /* 硬件错误次数 */
    int nAicpuErrCnt;             /* AI CPU错误次数 */
    long long llTotalRunningTime; /* 总运行时间 */
} MonitorNpuInfo_S;

/*网络统计信息*/
struct NetworkStats {
    unsigned long rx_bytes;
    unsigned long tx_bytes;
    unsigned long rx_packets;
    unsigned long tx_packets;
};

/*带宽信息*/
struct BandwidthInfo {
    double max_bandwidth_mbps;      // 最大带宽 (Mbps)
    double current_rx_mbps;         // 当前接收速率 (Mbps)
    double current_tx_mbps;         // 当前发送速率 (Mbps)
    double available_rx_mbps;       // 剩余接收带宽 (Mbps)
    double available_tx_mbps;       // 剩余发送带宽 (Mbps)
};

/**
 * 初始化性能监控模块
 * 创建一个后台线程，每秒统计一次CPU、内存和NPU使用情况
 *
 * 该函数会初始化互斥锁，记录初始MMZ内存使用量作为基线，
 * 然后创建监控线程开始周期性监控。
 *
 * @return 成功返回0，失败返回负数
 *         -1: 监控已经在运行
 *         -2: 互斥锁初始化失败
 *         -3: 创建监控线程失败
 */
int perfMonitor_init(void);

/**
 * 去初始化性能监控模块
 * 停止监控线程并输出平均统计结果
 *
 * 该函数会设置停止标志，等待监控线程结束，
 * 并销毁互斥锁。监控线程结束前会输出整个监控
 * 期间的平均CPU使用率、内存使用情况和NPU使用情况。
 *
 * @return 成功返回0，失败返回负数
 *         -1: 监控未运行
 *         -2: 等待监控线程结束失败
 */
int perfMonitor_uninit(void);

/**
 * 获取当前NPU使用信息
 *
 * 该函数获取当前NPU的使用情况并填充到提供的结构体中。
 * 如果NPU不可用或读取失败，结构体会被清零。
 *
 * @param pInfo NPU信息结构体指针，用于存储获取的信息
 * @return 成功返回0，失败返回负数
 *         -1: NPU设备不可用
 *         -2: 读取NPU信息失败
 */
int perfMonitor_getNpuInfo(MonitorNpuInfo_S *pInfo);

/**
 * @brief   : 获取当前进程及系统的内存快照
 * @param   {MonitorMemoryInfo_S*} pInfo：输出内存快照，不能为 nullptr
 * @return  {int} OK：成功；ERR_PARAM_NULL：参数为空；ERR_OPEN：无法读取进程状态
 * @note    : 数据来自 /proc/self/status、/proc/meminfo 和平台 MMZ 统计接口。
 */
int perfMonitor_getMemoryInfo(MonitorMemoryInfo_S *pInfo);

/**
 * @brief   : 输出带阶段名称的内存检查点
 * @param   {const char*} pcStage：检查点名称，建议使用稳定的英文点号路径
 * @return  {int} OK：成功；ERR_PARAM_NULL：阶段名称为空；其他值表示快照读取失败
 * @note    : 只有 ENABLE_MEMORY_CHECKPOINT=1 时输出 [MEMCHECK] 日志；默认不输出。
 */
int perfMonitor_logMemoryCheckpoint(const char *pcStage);

/**
 * @brief   : 输出不改变增量基线的进程内存快照
 * @param   {const char*} pcStage：快照阶段名称
 * @return  {int} OK：成功；ERR_PARAM_NULL：阶段名称为空；其他值表示快照读取失败
 * @note    : 该接口用于一个大阶段内部插入多个观测点，不更新
 *            perfMonitor_logMemoryCheckpoint() 使用的上一份快照。
 */
int perfMonitor_logMemorySnapshot(const char *pcStage);

/**
 * @brief   : 延迟指定时间后输出内存检查点
 * @param   {const char*} pcStage：检查点名称
 * @param   {unsigned int} nDelayMs：等待时间，单位为毫秒
 * @return  {int} OK：成功；ERR_PARAM_NULL：阶段名称为空；其他值表示快照读取失败
 * @note    : 该接口会阻塞调用线程，仅建议在内存测试流程中按需使用，不要在正式启动路径逐点调用。
 */
int perfMonitor_logMemoryCheckpointDelayed(const char *pcStage, unsigned int nDelayMs);

/**
 * @brief   : 延迟指定时间后输出不改变增量基线的进程内存快照
 * @param   {const char*} pcStage：快照阶段名称
 * @param   {unsigned int} nDelayMs：等待时间，单位为毫秒
 * @return  {int} OK：成功；ERR_PARAM_NULL：阶段名称为空；其他值表示快照读取失败
 * @note    : 仅建议在测试版本中使用；正式构建不会等待。
 */
int perfMonitor_logMemorySnapshotDelayed(const char *pcStage, unsigned int nDelayMs);

/**
 * @brief   : 输出当前进程全部线程的 smaps 栈映射明细
 * @param   {const char*} pcStage：统计阶段名称
 * @return  {int} OK：成功；ERR_PARAM_NULL：阶段名称为空；其他值表示 smaps 读取失败
 * @note    : 优先逐个读取 /proc/self/task/<tid>/smaps 中的 [stack] 映射，
 *            同时兼容 task/smaps 中可能出现的 [stack:<tid>] 命名，输出
 *            Size/Rss/Pss/Private_Dirty；可复用于 ISP、AI、OSD、音频和网络线程分析。
 *            默认仅在 ENABLE_THREAD_STACK_CHECKPOINT=1 时输出。
 */
int perfMonitor_logThreadStackCheckpoint(const char *pcStage);

// 获取网卡最大带宽 (Mbps)
double getMaxBandwidth(const std::string& interface);

// 读取网卡统计信息
bool getNetworkStats(const std::string& interface, NetworkStats& stats);

// 计算带宽使用率和剩余带宽
bool getAvailableBandwidth(const std::string& interface, BandwidthInfo& bandwidth_info);

// 示例使用函数
void printBandwidthInfo(const std::string& interface);
