/**
 * @FilePath     : main.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-10-13 10:52:02
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-07 17:50:36
 * @Description  : 升级包升级进程
 */

#include <iostream>
#include <chrono>
#include <atomic>
#include <signal.h>
#include <thread>
#include <string.h>
#include "path_define.h"
#include "upgrade_communicate.h"
#include "data_manage.h"
#include "dlog.h"
#include "timezone_runtime.h"

/* 日志记录单个日志文件的最大大小 */
#define MAX_LOG_SIZE  (1 * 512 * 1024) // 512KB
/* 日志记录最大保留的日志文件数量 */
#define MAX_LOG_FILES (1)

/* 进程是否捕获信号而退出标志 */
static std::atomic<bool> g_bExit(false);

/* 退出信号回调函数 */
static void exitHandler(int nSigno)
{
    if (SIGINT == nSigno || SIGTERM == nSigno)
    {
        g_bExit.store(true, std::memory_order_release);
    }
}

/* 初始化信号处理 */
void initSignals()
{
    /* 忽略管道破裂信号 */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, exitHandler);
    signal(SIGTERM, exitHandler);
    /* 不会因为管道关闭而收到SIGPIPE信号，避免程序异常终止 */
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);
}

int main(int argc, char *argv[])
{
    /* 初始化信号处理 */
    initSignals();

    /* 初始化日志 */
    if (ERR == initLogBySize("upgrade", UPGRADE_LOG_PATH, MAX_LOG_SIZE, MAX_LOG_FILES))
    {
        printf("日志初始化失败\n");
    }
#if CAP_PROCESS_LOG_SWITCH
    /* 发布模式：关闭控制台同步输出，日志级别设置为 WARN */
    syncPrintf(false);
    setLogLevel(LOG_WARN);
#else
    /* 开发模式：开启控制台同步输出，日志级别设置为 TRACE */
    syncPrintf(true);
    setLogLevel(LOG_TRACE);
#endif

    /* 设置日志限流：同一调用点至少间隔1000ms才允许再次输出，防止高频日志阻塞业务线程 */
    setLogThrottleInterval(1000);
    /* 启动日志级别文件监控，支持运行时通过文件切换日志级别 */
    startLogLevelMonitor(UPGRADE_LOG_LEVEL_PATH);

    dlog_trace("启动升级程序");

    /* 初始化进程时区运行时，收到 SIGHUP 后重新加载时区配置 */
    TimezoneRuntime_NS::init_timezone_runtime("upgrade");

    /* 初始化升级程序 */
    dataManage_init();
    
    /* 创建通讯 */
    UpgradeServer::instance()->init();
    
    std::chrono::seconds sleepDuration(100);
    
    /* 主循环 */
    while(!g_bExit.load())
	{
        // log_trace("升级程序运行中......");
        std::this_thread::sleep_for(sleepDuration);
	}

    dlog_trace("升级程序退出");

    /* 去初始化日志 */ 
    uninitLog();
    
    return 0;
}
