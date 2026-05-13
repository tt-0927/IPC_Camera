/**
 * @FilePath     : stream_main.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-03-19 08:57:03
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-19 16:44:48
 * @Description  : 主程序入口 
 */

#include <atomic>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "dlog.h"
#include "stream_video.h"
#include "stream_audio.h"
#include "performance_monitor.h"
#include "gb28181.hpp"
#include "share_define.h"
#include "network_define.h"
#include "convert_interface.h"
#include "path_define.h"
#include "control_manage.h"
#include "config_manager.h"
// #include "event_manage.h"

/* 日志记录单个日志文件的最大大小 */
#define MAX_LOG_SIZE  (1 * 1024 * 1024) // 1MB
/* 日志记录最大保留的日志文件数量 */
#define MAX_LOG_FILES (4)

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

/* 初始化所有模块 */ 
bool initModules()
{
    int nRet = OK;

    dlog_trace("初始化所有模块");

    /* 初始化性能监控模块 */
    // nRet = perfMonitor_init();
    // if (nRet < OK)
    // {
    //     dlog_error("初始化性能监控模块失败：%d", nRet);
    //     goto exit_perf_monitor;
    // }
    /* 初始化全局配置 */
    nRet = CConfigManager::instance()->init();
    if (nRet < OK)
    {
        dlog_error("初始化全局配置失败：%d", nRet);
        goto exit_perf_monitor;
    }

    /* 初始化视频流 */
    nRet = CStreamVideo::instance()->init();
    if(nRet < OK)
    {
        dlog_error("视频模块初始化失败：%d", nRet);
        goto exit_stream_video;
    }

    /* 初始化音频流 */
    nRet = CStreamAudio::instance()->init();
    if(nRet < OK)
    {
        dlog_error("音频模块初始化失败：%d", nRet);
        goto exit_stream_audio;
    }

    /* 初始化推流模块 */
    nRet = CPushStream::instance()->init();
    if (nRet < OK)
    {
        dlog_error("推流模块初始化失败：%d", nRet);
        goto exit_push_stream;
    }

    /* 控制管理模块初始化 */
    nRet = ControlManage::instance()->init();
    if (nRet < OK)
    {
        dlog_error("控制管理模块初始化失败：%d", nRet);
        goto exit_control_manage;
    }

    /* 事件管理初始化 */
    // nRet = CEventManage::instance()->init();
    // if (nRet < OK)
    // {
    //     dlog_error("事件管理模块初始化失败：%d", nRet);
    //     goto exit_event_manage;
    // }

    dlog_trace("初始化所有模块成功");
    return true;

// exit_event_manage:
    /* 去初始化事件管理 */
    // nRet = CEventManage::instance()->deinit();
    // if (nRet < OK)
    // {
    //     dlog_error("事件管理模块去初始化失败：%d", nRet);
    // }
exit_control_manage:
    /* 控制管理模块去初始化 */
    nRet = ControlManage::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("控制管理模块去初始化失败：%d", nRet);
    }
exit_push_stream:
    /* 去初始化推流模块 */
    nRet = CPushStream::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("推流模块去初始化失败：%d", nRet);
    }
exit_stream_audio:
    /* 去初始化音频流 */
    nRet = CStreamAudio::instance()->deinit();
    if(nRet < OK)
    {
        dlog_error("音频模块去初始化失败：%d", nRet);
    }
exit_stream_video:
    /* 去初始化视频流 */
    nRet = CStreamVideo::instance()->deinit();
    if(nRet < OK)
    {
        dlog_error("视频模块去初始化失败：%d", nRet);
    }
exit_perf_monitor:
    /* 停止监控并输出平均结果 */
    // nRet = perfMonitor_uninit();
    // if (nRet < OK)
    // {
    //     dlog_error("性能监控模块去初始化失败：%d", nRet);
    // }

    dlog_trace("初始化所有模块失败");
    return false;
}

/* 去初始化所有模块(与初始化的顺序相反) */
void deinitModules()
{
    int nRet = OK;

    //note:需提前停止，否则会统计出现占用为0情况，影响平均结果
    /* 停止监控并输出平均结果 */
    // nRet = perfMonitor_uninit();
    // if (nRet < OK)
    // {
    //     dlog_error("性能监控模块去初始化失败：%d", nRet);
    // }

    /* 去初始化事件管理 */
    // nRet = CEventManage::instance()->deinit();
    // if (nRet < OK)
    // {
    //     dlog_error("事件管理模块去初始化失败：%d", nRet);
    // }

    /* 控制管理模块去初始化 */
    nRet = ControlManage::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("控制管理模块去初始化失败：%d", nRet);
    }

    //note:需提前音视频模块停止，否则会出现外部请求卡死问题
    /* 去初始化推流模块 */
    nRet = CPushStream::instance()->deinit();
    if(nRet < OK)
    {
        dlog_error("推流模块去初始化失败：%d", nRet);
    }

    /* 去初始化音频流 */
    nRet = CStreamAudio::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("音频模块去初始化失败：%d", nRet);
    }

    /* 去初始化视频流 */
    nRet = CStreamVideo::instance()->deinit();
    if(nRet < OK)
    {
        dlog_error("视频模块去初始化失败：%d", nRet);
    }
}

int main(int argc,char *argv[])
{
    /* 初始化信号处理 */ 
    initSignals();

    /* 初始化日志 */
    if (ERR == initLogBySize("stream", STREAM_LOG_PATH, MAX_LOG_SIZE, MAX_LOG_FILES))
    {
        printf("日志初始化失败\n");
    }
    /* 设置日志输出同步输出控制台 */
	syncPrintf(true);
    /* 设置日志等级 */
	setLogLevel(LOG_TRACE);

    dlog_trace("启动主程序");

    /* 初始化所有模块 */ 
    if (!initModules())
    {
        return ERR;
    }

    /* 主循环 */
    while(!g_bExit.load())
	{
		sleep(1);
	}

    /* 去初始化所有模块 */ 
    deinitModules();

    dlog_trace("主程序退出");
    
    /* 去初始化日志 */ 
    // uninitLog();

    return OK;
}