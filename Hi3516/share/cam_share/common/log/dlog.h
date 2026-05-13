/**
 * @FilePath     : dlog.h
 * @Author       : zhangjunbin
 * @Date         : 2021年3月30日
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-25 14:55:24
 * @Description  : 日志的基础库，基于spdlog封装
 */

#ifndef OS_SHARE_LOG_SPDLOG_BASE_INCLUDE_
#define OS_SHARE_LOG_SPDLOG_BASE_INCLUDE_

#ifdef __cplusplus    /*C++编译器包含的宏，例如用g++编译时，该宏就存在，则下面的语句extern "C"才会被执行*/
extern "C" {          /*C++编译器才能支持，C编译器不支持*/
#endif

#include <stdio.h>
#include <stdbool.h>

#define DEBUG_INFO(fmt, args...) printf("\033[33m[%s:%s:%d]" #fmt "\r\n",__FILE__,__func__,__LINE__, ##args)

#define LOG_TRACE 0		/* 跟踪日志 */
#define LOG_DEBUG 1		/* debug日志 */
#define LOG_INFO 2		/* info日志 */
#define LOG_WARN 3		/* 警告日志 */
#define LOG_ERROR 4		/* 错误日志 */
#define LOG_USER 5		/* 用户操作日志，用于记录用户操作信息 */
#define LOG_FAULT 6		/* 系统故障日志，中文错误信息 */

#define LOG_NONE			"\033[m"
#define LOG_RED				"\033[0;32;31m"
#define LOG_LIGHT_RED		"\033[1;31m"
#define LOG_GREEN			"\033[0;32;32m"
#define LOG_LIGHT_GREEN		"\033[1;32m"
#define LOG_BLUE			"\033[0;32;34m"
#define LOG_LIGHT_BLUE		"\033[1;34m"
#define LOG_DARY_GRAY		"\033[1;30m"
#define LOG_CYAN			"\033[0;36m"
#define LOG_LIGHT_CYAN		"\033[1;36m"
#define LOG_PURPLE			"\033[0;35m"
#define LOG_LIGHT_PURPLE	"\033[1;35m"
#define LOG_BROWN			"\033[0;33m"
#define LOG_YELLOW			"\033[1;33m"
#define LOG_LIGHT_GRAY		"\033[0;37m"
#define LOG_WHITE			"\033[1;37m"

typedef void* dlogHandle_S;

/* 初始化日志
 * logname:日志名称，不能跟其他日志名重复，否则会导致段错误
 * logfile:日志的完整路径跟名称：如：/opt/course/log/control.log
 *  */
int initLog(char* logname,char* logfile);

/* 初始化对应日志等级的日志文件,使用前一定得先调用initLog()初始化日志环境
 * logname:日志名称，不能跟其他日志名重复，否则会导致段错误
 * logfile:日志的完整路径跟名称：如：/opt/course/log/control.log
 * level:日志等级，目前单独的日志只支持warn，error，user
 * */
int initLogLevel(char* logname,char* logfile,int level);

/**
 * @brief    : 初始化滚动日志 (按大小和数量分割)
 * @note     : 使用滚动日志策略，当文件达到指定大小时进行切换。不能与 initLog() 同时使用。
 * @param    {char*} logname: 日志名称，不能跟其他日志名重复，否则会导致段错误
 * @param    {char*} logfile: 日志的完整路径跟名称：如：/opt/course/log/control.log
 * @param    {int} max_file_size: 单个日志文件的最大大小（单位：字节）
 * @param    {int} max_files: 最大保留的日志文件数量
 * @return   {int} 0：成功，非0：失败
 */
int initLogBySize(char *logname, char *logfile, int max_file_size, int max_files);

/**
 * @brief    : 初始化对应等级的滚动日志文件 (按大小和数量分割)
 * @note     : 使用前一定得先调用 initLogBySize()初始化日志环境
 * @param    {char*} logname: 日志名称，不能跟其他日志名重复，否则会导致段错误
 * @param    {char*} logfile: 日志的完整路径跟名称：如：/opt/course/log/control.log
 * @param    {int} level: 日志等级，目前单独的日志只支持warn，error，user
 * @param    {int} max_file_size: 单个日志文件的最大大小（单位：字节）
 * @param    {int} max_files: 最大保留的日志文件数量
 * @return   {int} 0：成功，非0：失败
 */
int initLogLevelBySize(char *logname, char *logfile, int level, int max_file_size, int max_files);

/* 设置日志输出等级
 * level:日志等级
 * */
int setLogLevel(int level);

/* 反初始化日志 */
int uninitLog();

/*设置同步输出控制台*/
int syncPrintf(bool bStatus);

/*获取当前时间*/
int getCurrTime(char *outputBuf, char *timeFormat);

/*设置立即刷新等级*/
int setFlushLevel(int nLevel);

/*查找并跳过ANSI转义序列 */
void skip_ansi_escape_sequences(char *str);

/* 打印 */
int dlog_printf(int level, const char *filename_in, int line_in, const char *funcname_in, const char *format, ...);

#define dlog_trace(format, ...) dlog_printf(LOG_TRACE, __FILE__, __LINE__, __FUNCTION__, LOG_LIGHT_PURPLE format LOG_NONE "\r", ##__VA_ARGS__);
#define dlog_debug(format, ...) dlog_printf(LOG_DEBUG, __FILE__, __LINE__, __FUNCTION__, LOG_BLUE format LOG_NONE "\r", ##__VA_ARGS__);
#define dlog_info(format, ...) dlog_printf(LOG_INFO, __FILE__, __LINE__, __FUNCTION__, LOG_GREEN format LOG_NONE "\r", ##__VA_ARGS__);
#define dlog_warn(format, ...) dlog_printf(LOG_WARN, __FILE__, __LINE__, __FUNCTION__, LOG_YELLOW format LOG_NONE "\r", ##__VA_ARGS__);
#define dlog_error(format, ...) dlog_printf(LOG_ERROR, __FILE__, __LINE__, __FUNCTION__, LOG_RED format LOG_NONE "\r", ##__VA_ARGS__);
#define dlog_user(format, ...) dlog_printf(LOG_USER, __FILE__, __LINE__, __FUNCTION__, format "\r", ##__VA_ARGS__);
#define dlog_fault(format, ...) dlog_printf(LOG_FAULT, __FILE__, __LINE__, __FUNCTION__, format "\r", ##__VA_ARGS__);

#define dlog(nLevel, format, ...) \
    do{\
        switch(nLevel) {\
            case LOG_TRACE:\
            dlog_printf(LOG_TRACE,__FILE__,__LINE__,__FUNCTION__,LOG_LIGHT_PURPLE format LOG_NONE "\r", ##__VA_ARGS__);break; \
            case LOG_DEBUG:\
            dlog_printf(LOG_DEBUG,__FILE__,__LINE__,__FUNCTION__,LOG_BLUE format LOG_NONE "\r", ##__VA_ARGS__);break; \
            case LOG_INFO:\
            dlog_printf(LOG_INFO,__FILE__,__LINE__,__FUNCTION__,LOG_GREEN format LOG_NONE "\r", ##__VA_ARGS__);break; \
            case LOG_WARN:\
            dlog_printf(LOG_WARN,__FILE__,__LINE__,__FUNCTION__,LOG_YELLOW format LOG_NONE "\r", ##__VA_ARGS__);break; \
            case LOG_ERROR:\
            dlog_printf(LOG_ERROR,__FILE__,__LINE__,__FUNCTION__,LOG_RED format LOG_NONE "\r", ##__VA_ARGS__);break; \
            case LOG_USER:\
            dlog_printf(LOG_USER,__FILE__,__LINE__,__FUNCTION__, format "\r", ##__VA_ARGS__);break; \
            default:\
            printf(format, ##__VA_ARGS__);break;\
        }\
    }while(0)\

/*检查API的返回值，需传入用于判断的返回码，失败时打印错误信息与返回码*/
#define dlog_check_return(func, ret)                                  \
    do                                                                \
    {                                                                 \
        int __result = func;                                          \
        if (__result != ret)                                          \
        {                                                             \
            dlog_error("%s failed, error code: %d", #func, __result); \
            return __result;                                          \
        }                                                             \
    } while (0)

/*检查API的返回值，需传入用于判断的返回码、打印信息，失败时打印错误信息与返回码*/
#define dlog_check_return_print(func, ret, format, ...)           \
do                                                                \
{                                                                 \
    int __result = func;                                          \
    if (__result != ret)                                          \
    {                                                             \
        dlog_error("%s failed, error code: %d", #func, __result); \
        dlog_error(format, ##__VA_ARGS__);                        \
        return __result;                                          \
    }                                                             \
} while (0)

#ifdef __cplusplus
}
#endif

#endif //OS_SHARE_LOG_SPDLOG_BASE_INCLUDE_
