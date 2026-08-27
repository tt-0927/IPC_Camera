/*
 * @Author       : EasonLu
 * @Date         : 2025-02-12 17:08:30
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-02-13 11:30:45
 * @FilePath     : ModuleLog.h
 * @Description  : 模块化日志
 */
#ifndef _MODULE_LOG_H_
#define _MODULE_LOG_H_
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#define MODULE_LOG_TRACE 0 /* 跟踪日志 */
#define MODULE_LOG_DEBUG 1 /* debug日志 */
#define MODULE_LOG_INFO 2  /* info日志 */
#define MODULE_LOG_WARN 3  /* 警告日志 */
#define MODULE_LOG_ERROR 4 /* 错误日志 */

    /**
     * @brief  日志回调函数
     * @param  [int] nLevel:日志级别
     * @param  [const char] *pLog:日志内容
     * @return [*]
     * @author EasonLu
     * @note   日志回调函数指针定义
     */
    typedef void (*ModuleLog)(int nLevel, const char *pLog);

    /**
     * @brief  设置日志回调
     * @param  [ModuleLog] pModuleLog 日志回调
     * @return [*]
     * @author EasonLu
     * @note
     */
    void set_module_log(ModuleLog pModuleLog);

    /**
     * @brief  设置日志的同步打印开关
     * @param  [int] bEnable
     * @return [*]
     * @author EasonLu
     * @note   
     */
    void enable_module_log_print(int bEnable);

    /**
     * @brief  日志调用函数
     * @param  [int] nLevel:日志级别
     * @param  [const char] *pFilename:文件名
     * @param  [int] nLine:行号
     * @param  [const char] *pFuncname:函数名
     * @param  [const char] *pFormat:格式化字符串
     * @param  [...]:可变参数
     * @return [*]
     * @author EasonLu
     * @note
     */
    void module_log(int nLevel,
                    const char *pFilename, int nLine, const char *pFuncname,
                    const char *pFormat, ...);

#define MLOG_TRACE(format, ...) module_log(MODULE_LOG_TRACE, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__);
#define MLOG_DEBUG(format, ...) module_log(MODULE_LOG_DEBUG, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__);
#define MLOG_INFO(format, ...) module_log(MODULE_LOG_INFO, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__);
#define MLOG_WARN(format, ...) module_log(MODULE_LOG_WARN, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__);
#define MLOG_ERROR(format, ...) module_log(MODULE_LOG_ERROR, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__);

#ifdef __cplusplus
}
#endif

#endif /* _MODULE_LOG_H_ */