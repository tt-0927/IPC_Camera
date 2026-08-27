/**
 * @file NetSdkLog.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief NetSdkLog 模块接口与类型定义
 * 功能说明：
 * 1. 声明 NetSdkLog 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#ifndef NETSDK_LOG_H
#define NETSDK_LOG_H

#ifdef __cplusplus    /*C++编译器包含的宏，例如用g++编译时，该宏就存在，则下面的语句extern "C"才会被执行*/
extern "C" {          /*C++编译器才能支持，C编译器不支持*/
#endif

#include <stdio.h>
#include <stdbool.h>


#ifndef NETSDK_DEBUG_INFO
    #ifdef _MSC_VER
        #define NETSDK_DEBUG_INFO(fmt, ...) printf("\033[33m[%s:%s:%d]" fmt "\r\n", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
    #else
        #define NETSDK_DEBUG_INFO(fmt, args...) printf("\033[33m[%s:%s:%d]" fmt "\r\n", __FILE__, __func__, __LINE__, ##args)
    #endif
#endif

#define NETSDK_LOG_TRACE 0 /* 跟踪日志 */
#define NETSDK_LOG_DEBUG 1 /* debug日志 */
#define NETSDK_LOG_INFO 2  /* info日志 */
#define NETSDK_LOG_WARN 3  /* 警告日志 */
#define NETSDK_LOG_ERROR 4 /* 错误日志 */

#ifndef NETSDK_LOG_COLOR_NONE
    #define NETSDK_LOG_COLOR_NONE			  "\033[m"
#endif
#ifndef NETSDK_LOG_COLOR_RED
    #define NETSDK_LOG_COLOR_RED				"\033[0;32;31m"
#endif
#ifndef NETSDK_LOG_COLOR_GREEN
    #define NETSDK_LOG_COLOR_GREEN			"\033[0;32;32m"
#endif
#ifndef NETSDK_LOG_COLOR_LIGHT_PURPLE
   #define NETSDK_LOG_COLOR_LIGHT_PURPLE	    "\033[1;35m"
#endif
#ifndef NETSDK_LOG_COLOR_BLUE
   #define NETSDK_LOG_COLOR_BLUE			    "\033[0;32;34m"
#endif
#ifndef NETSDK_LOG_COLOR_YELLOW
   #define NETSDK_LOG_COLOR_YELLOW			"\033[1;33m"
#endif

typedef void* NetSdkLogHandle_S;

/* 初始化日志
 * logname:日志名称，不能跟其他日志名重复，否则会导致段错误
 * logfile:日志的完整路径跟名称：如：/opt/course/log/control.log
 *  */
int initSdkLog(char* logname,char* logfile);

/* 初始化对应日志等级的日志文件,使用前一定得先调用initSdkLog()初始化日志环境
 * logname:日志名称，不能跟其他日志名重复，否则会导致段错误
 * logfile:日志的完整路径跟名称：如：/opt/course/log/control.log
 * level:日志等级，目前单独的日志只支持warn，error，user
 * */
int initSdkLogLevel(char* logname,char* logfile,int level);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief    : 初始化滚动日志 (按大小和数量分割)
 * @note     : 使用滚动日志策略，当文件达到指定大小时进行切换。不能与 initSdkLog() 同时使用。
 * @param    {char*} logname: 日志名称，不能跟其他日志名重复，否则会导致段错误
 * @param    {char*} logfile: 日志的完整路径跟名称：如：/opt/course/log/control.log
 * @param    {int} max_file_size: 单个日志文件的最大大小（单位：字节）
 * @param    {int} max_files: 最大保留的日志文件数量
 * @return   {int} 0：成功，非0：失败
 */
int initSdkLogBySize(char *logname, char *logfile, int max_file_size, int max_files);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief    : 初始化对应等级的滚动日志文件 (按大小和数量分割)
 * @note     : 使用前一定得先调用 initSdkLogBySize()初始化日志环境
 * @param    {char*} logname: 日志名称，不能跟其他日志名重复，否则会导致段错误
 * @param    {char*} logfile: 日志的完整路径跟名称：如：/opt/course/log/control.log
 * @param    {int} level: 日志等级，目前单独的日志只支持warn，error，user
 * @param    {int} max_file_size: 单个日志文件的最大大小（单位：字节）
 * @param    {int} max_files: 最大保留的日志文件数量
 * @return   {int} 0：成功，非0：失败
 */
int initSdkLogLevelBySize(char *logname, char *logfile, int level, int max_file_size, int max_files);

/* 设置日志输出等级
 * level:日志等级
 * */
int setLogLevel(int level);

/* 反初始化日志 */
int uninitSdkLog();

/*设置同步输出控制台*/
int syncPrintf(bool bStatus);

/*获取当前时间*/
int getCurrTime(char *outputBuf, char *timeFormat);

/*设置立即刷新等级*/
int setFlushLevel(int nLevel);

/*查找并跳过ANSI转义序列 */
void skip_ansi_escape_sequences(char *str);

/* 打印 */
int netSdk_log(int level, const char *filename_in, int line_in, const char *funcname_in, const char *format, ...);

#define NETSDK_LOG_MESSAGE_TRACE(format, ...) \
    do { \
        (void)netSdk_log(NETSDK_LOG_TRACE, __FILE__, __LINE__, __FUNCTION__, NETSDK_LOG_COLOR_LIGHT_PURPLE format NETSDK_LOG_COLOR_NONE "\r", ##__VA_ARGS__); \
    } while (0)
#define NETSDK_LOG_MESSAGE_DEBUG(format, ...) \
    do { \
        (void)netSdk_log(NETSDK_LOG_DEBUG, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__); \
    } while (0)
#define NETSDK_LOG_MESSAGE_INFO(format, ...) \
    do { \
        (void)netSdk_log(NETSDK_LOG_INFO, __FILE__, __LINE__, __FUNCTION__, format "\r", ##__VA_ARGS__); \
    } while (0)
#define NETSDK_LOG_MESSAGE_WARN(format, ...) \
    do { \
        (void)netSdk_log(NETSDK_LOG_WARN, __FILE__, __LINE__, __FUNCTION__, format "\r", ##__VA_ARGS__); \
    } while (0)
#define NETSDK_LOG_MESSAGE_ERROR(format, ...) \
    do { \
        (void)netSdk_log(NETSDK_LOG_ERROR, __FILE__, __LINE__, __FUNCTION__, format "\r", ##__VA_ARGS__); \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* NETSDK_LOG_H */
