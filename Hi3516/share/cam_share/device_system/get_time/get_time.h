/*
 * @Descripttion: 时间公共库
 * @version: 
 * @Author: fanghongshen
 * @Date: 2021-10-22 15:57:02
 * @LastEditors: fanghongshen
 * @LastEditTime: 2021-10-22 16:06:19
 */

#ifndef _SHARE_GET_TIME_H_
#define _SHARE_GET_TIME_H_

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#define TIMEINTERVAL        ((60) * (15))

typedef struct
{
    int nYears;
    int nMonths;
    int nDays;
    int nHours;
	int nMinutes;
	int nSeconds;
}UpTime_S;

typedef struct
{
    long lnUptime;         /* 总运行时间（秒） */
    UpTime_S upTime;       /* 分解后的运行时间 */
    char achUptime[50];    /* 运行时间的字符串表示 */
} SysUptime_S;

/*** 
 * @description : 转换运行时间戳为字符串
 * @author      : huangjunda
 * @param        {long} m_sec
 * @param        {SysUptime_S} *stUptimeInfo
 * @return       {*}
 */
void sys_calculate_uptime(long m_sec, SysUptime_S *stUptimeInfo);

/**
 * @brief       : 获取当前时间为字符串
 * @author      : zhouzirui
 * @param        {char} *pTime：指向存储结果的缓冲区
 * @param        {int} nLen：指向存储结果的缓冲区大小
 * @return       {*}0：成功，非零：失败
 * @note        : 示例：2019-08-06 05:31:39
 */
int get_time_char(char *pTime,int nLen);

/**
 * @brief       : 获取当前时间为字符串，用于国标GB35114
 * @author      : zhouzirui
 * @param        {char} *pTime：指向存储结果的缓冲区
 * @param        {int} nLen：指向存储结果的缓冲区大小
 * @return       {*}0：成功，非零：失败
 * @note        : 示例：2019-08-06T05:31:39
 */
int get_time_T_char(char *pTime,int nLen);

/**
 * @brief       : 获取当前时间(毫秒)
 * @author      : zhouzirui
 * @return       {double}：时间，失败返回0
 */
double get_time_ms();

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif