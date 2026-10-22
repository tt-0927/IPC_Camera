/*** 
 * @FilePath     : get_sys_info_interface.h
 * @Author       : zjc
 * @Date         : 2022-2-25 20:21:30
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-04-15 16:50:42
 * @Description  : 
 */

#ifndef _GET_SYS_INFO_INTERFACE_H_
#define _GET_SYS_INFO_INTERFACE_H_

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

// #include "os_que.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DISK_DATA_LEN (65) // 最大64位

enum DISK_STATUS_EN
{
	DISK_OFFLINE,
	DISK_ONLINE,
};

typedef struct
{
	char size[MAX_DISK_DATA_LEN];      /* 总量 */
	char used[MAX_DISK_DATA_LEN];      /* 已使用 */
	char avail[MAX_DISK_DATA_LEN];     /* 未使用 */
	char usageRate[MAX_DISK_DATA_LEN]; /* 使用率 */
	char mountPath[MAX_DISK_DATA_LEN]; /* 挂载路径 */
	enum DISK_STATUS_EN status;        /* 0 未知，1在线 */
} Disk_Info_S;

/**
 * @brief 内存信息结构体
 */
typedef struct
{
	unsigned long ulTotal;     /* 总内存（KB） */
	unsigned long ulFree;      /* 空闲内存（KB） */
	unsigned long ulBuffers;   /* 缓冲区内存（KB） */
	unsigned long ulCached;    /* 缓存内存（KB） */
	unsigned long ulAvailable; /* 可用内存（KB） */
} MemInfo_S;

/**
 * @brief 获取内存信息
 * @param pInfo 内存指针
 * @return int
 */
int get_mem_info(MemInfo_S *pInfo);

/**
 * 计算内存使用率
 */
double calculate_usage(MemInfo_S *pInfo);

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif
