/*
 * @Author       : EasonLu
 * @Date         : 2023-09-05 14:41:16
 * @LastEditors  : EasonLu
 * @LastEditTime : 2024-01-02 15:40:35
 * @FilePath     : logcommon.h
 * @Description  : 日志公共头文件
 */
#ifndef __LOG_COMMON_H__
#define __LOG_COMMON_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* 本机存放日志的路径 */
#define LOG_COMMON_PATH_LOCAL "/opt/course/log/"

    /**
     * @brief  获取日志路径前缀
     * @return [const char*] - [日志路径前缀]
     * @author EasonLu
     * @note   根据型号进行获取，后续可根据不同型号返回不同的日志存放路径
     */
    const char *get_logPathPrefix();

    /**
     * @brief  获取dlog日志输出到日志文件的最低级别
     * @return [int] dlog的日志输出级别，0-6，6最高，0最低，-1表示获取失败
     * @author EasonLu
     * @note   根据型号进行获取，返回输出到日志的最低级别
     */
    int get_dlogOutputLevel();

#ifdef __cplusplus
}
#endif

#endif // __LOG_COMMON_H__
