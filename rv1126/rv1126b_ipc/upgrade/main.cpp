/*
 * @FilePath     : main.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-05-16 10:02:32
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-03-31 10:40:50
 * @Description  :
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
#define MAX_LOG_SIZE  (1 * 1024 * 1024) // 1MB
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
    /* 设置日志输出同步输出控制台 */
	syncPrintf(true);
    /* 设置日志等级 */
	setLogLevel(LOG_ERROR);
#else
    /* 设置日志输出同步输出控制台 */
	syncPrintf(true);
    /* 设置日志等级 */
	setLogLevel(LOG_TRACE);
#endif
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
