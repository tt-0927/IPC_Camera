/**
 * @FilePath     : main.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-05 15:19:56
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-05 15:54:36
 * @Description  : 守护进程 - 监控和管理关键业务进程
 */

#include "process_manager.h"
#include "dlog.h"
#include "timezone_runtime.h"
#include "path_define.h"

#include <iostream>
#include <unistd.h>
#include <fcntl.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <csignal>
#include <thread>
#include <chrono>

#if CAP_DAEMON_LOG_BY_SIZE // 守护进程按大小滚动日志

/* 日志记录单个日志文件的最大大小 */
#define MAX_LOG_SIZE  (1 * 512 * 1024) // 512KB
/* 日志记录最大保留的日志文件数量 */
#define MAX_LOG_FILES (1)
#endif

// info --- 全局配置 ---
char *LOG_FILE_PATH = "/opt/course/log/daemon/daemon.log";
char *DAEMON_NAME = "daemon";
const int CHECK_INTERVAL_SECONDS = 7;

/* 退出的全局标志 */
volatile sig_atomic_t g_running = 1;

void signal_handler(int signum)
{
    if (signum == SIGTERM || signum == SIGINT)
    {
        g_running = 0;
    }
    /* 处理子进程退出信号，回收资源，防止僵尸进程 */
    if (signum == SIGCHLD)
    {
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;
    }
}

void daemonize()
{
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    if (setsid() < 0) exit(EXIT_FAILURE);

    /* daemon化阶段临时忽略SIGHUP，主流程中由时区运行时恢复并通过sigwait处理 */
    signal(SIGHUP, SIG_IGN);

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    chdir("/");

    for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
        close(x);
    }

    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);
}

int main(int argc, char *argv[])
{
    /* 后台运行 */
    daemonize();

    /* 初始化你的日志系统 */
#if CAP_DAEMON_LOG_BY_SIZE // 守护进程按大小滚动日志
    if (initLogBySize(DAEMON_NAME, LOG_FILE_PATH, MAX_LOG_SIZE, MAX_LOG_FILES) != 0)
#else
    if (initLog(DAEMON_NAME, LOG_FILE_PATH) != 0)
#endif    
    {
        /* 如果日志初始化失败，守护进程无法记录任何信息，直接退出 */
        return 1;
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
    startLogLevelMonitor(DAEMON_LOG_LEVEL_PATH);

    dlog_info("守护进程已启动. PID: %d", getpid());

    /* 初始化进程时区运行时，并恢复 daemonize 中被忽略的 SIGHUP */
    TimezoneRuntime_NS::init_timezone_runtime("daemon");

    /* 设置信号处理器 */
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGCHLD, signal_handler);

    CProcessManager manager;
    if (!manager.init_processes())
    {
        dlog_error("进程管理器初始化失败,退出!");
        return 1;
    }

    /* 首次初始启动所有进程 */
    // manager.initial_startup_sequence();

    dlog_info("进入主监控循环...");

    while (g_running)
    {
        manager.check_and_manage_processes();
        std::this_thread::sleep_for(std::chrono::seconds(CHECK_INTERVAL_SECONDS));
    }

    dlog_info("守护进程停止");
    return 0;
}
