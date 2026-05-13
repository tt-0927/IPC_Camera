/**
 * @FilePath     : performance_monitor.h
 * @Author       : zhouzirui
 * @Date         : 2025-04-29 11:07:07
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-25 19:29:33
 * @Description  : 性能监控头文件 提供监控CPU、内存和NPU使用的功能接口
 */

#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <cstring>

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

// 获取网卡最大带宽 (Mbps)
double getMaxBandwidth(const std::string& interface);

// 读取网卡统计信息
bool getNetworkStats(const std::string& interface, NetworkStats& stats);

// 计算带宽使用率和剩余带宽
bool getAvailableBandwidth(const std::string& interface, BandwidthInfo& bandwidth_info);

// 示例使用函数
void printBandwidthInfo(const std::string& interface);