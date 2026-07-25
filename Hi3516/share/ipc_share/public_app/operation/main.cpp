/**
 * @FilePath     : main.cpp
 * @Author       : huangjunda
 * @Date         : 2025-07-15 15:41:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-05 15:54:46
 * @Description  : 运维记录程序
 * 
 */

#include <stdlib.h>
#include <signal.h>
#include <thread>
#include "dlog.h"
#include "operation_communicate.h"
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
    if(SIGINT == nSigno || SIGTERM == nSigno)
    {
        g_bExit.store(true);
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
void communicate_init()
{
    OperationServer::instance()->init();
}

int main(int argc, char const *argv[])
{
    /* 初始化信号处理 */ 
    initSignals();

    /* 初始化日志 */
	if(ERR == initLogBySize("operation", OPERATION_LOG_PATH, MAX_LOG_SIZE, MAX_LOG_FILES))
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


    dlog_trace("启动运维程序");

    /* 初始化进程时区运行时，收到 SIGHUP 后重新加载时区配置 */
    TimezoneRuntime_NS::init_timezone_runtime("operation");

    communicate_init();

    while (!g_bExit)
    {
        /* 休眠1秒 */
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    dlog_trace("运维程序退出");

    return 0;
}
