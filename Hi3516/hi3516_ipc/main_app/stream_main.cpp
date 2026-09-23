/**
 * @FilePath     : stream_main.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-03-19 08:57:03
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-20 16:07:17
 * @Description  : 主程序入口 
 */

#include <atomic>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ucontext.h>
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
#include "timezone_runtime.h"
#include "crypto_init.h"
// #include "stream_performance_service.h"
// #include "event_manage.h"

/* 日志记录单个日志文件的最大大小 */
#define MAX_LOG_SIZE  (1 * 1024 * 1024) // 1MB
/* 日志记录最大保留的日志文件数量 */
#define MAX_LOG_FILES (4)

/* 进程是否捕获信号而退出标志 */
static std::atomic<bool> g_bExit(false);
#if ENABLE_CRASH_DIAGNOSTICS
static int g_nCrashLogFd = -1;
static int g_nProcMapsFd = -1;

/* 信号处理函数中不能调用日志库/堆分配函数，使用固定缓冲区直接 write。 */
static char *appendLiteral(char *pOut, const char *pEnd, const char *pText)
{
    while (pOut < pEnd && *pText != '\0')
    {
        *pOut++ = *pText++;
    }
    return pOut;
}

static char *appendHex(char *pOut, const char *pEnd, uintptr_t nValue)
{
    static const char kHex[] = "0123456789abcdef";
    pOut = appendLiteral(pOut, pEnd, "0x");
    bool bStarted = false;
    for (int nShift = static_cast<int>(sizeof(nValue) * 8) - 4; nShift >= 0 && pOut < pEnd; nShift -= 4)
    {
        const unsigned int nDigit = static_cast<unsigned int>((nValue >> nShift) & 0x0fU);
        if (nDigit != 0 || bStarted || nShift == 0)
        {
            *pOut++ = kHex[nDigit];
            bStarted = true;
        }
    }
    return pOut;
}

static char *appendDec(char *pOut, const char *pEnd, unsigned int nValue)
{
    char szDigits[16];
    unsigned int nCount = 0;
    do
    {
        szDigits[nCount++] = static_cast<char>('0' + nValue % 10U);
        nValue /= 10U;
    } while (nValue != 0 && nCount < sizeof(szDigits));

    while (nCount > 0 && pOut < pEnd)
    {
        *pOut++ = szDigits[--nCount];
    }
    return pOut;
}

static void crashHandler(int nSigno, siginfo_t *pInfo, void *pContext)
{
    uintptr_t nPc = 0;
    uintptr_t nLr = 0;
    uintptr_t nSp = 0;
#if defined(__arm__)
    /*
     * arm-v01c02 musl 的 mcontext_t 没有 glibc 风格的 arm_pc/arm_lr/arm_sp
     * 命名成员，但其内存布局仍遵循 Linux ARM sigcontext ABI。使用本地 ABI
     * 视图读取寄存器，避免依赖不同 libc 对 mcontext_t 成员的命名方式。
     */
    struct ArmSignalContext
    {
        unsigned long trap_no;
        unsigned long error_code;
        unsigned long oldmask;
        unsigned long arm_r0;
        unsigned long arm_r1;
        unsigned long arm_r2;
        unsigned long arm_r3;
        unsigned long arm_r4;
        unsigned long arm_r5;
        unsigned long arm_r6;
        unsigned long arm_r7;
        unsigned long arm_r8;
        unsigned long arm_r9;
        unsigned long arm_r10;
        unsigned long arm_fp;
        unsigned long arm_ip;
        unsigned long arm_sp;
        unsigned long arm_lr;
        unsigned long arm_pc;
        unsigned long arm_cpsr;
        unsigned long fault_address;
    };
    const ucontext_t *pUc = static_cast<const ucontext_t *>(pContext);
    if (pUc != nullptr)
    {
        const ArmSignalContext *pArmContext =
            reinterpret_cast<const ArmSignalContext *>(&pUc->uc_mcontext);
        nPc = static_cast<uintptr_t>(pArmContext->arm_pc);
        nLr = static_cast<uintptr_t>(pArmContext->arm_lr);
        nSp = static_cast<uintptr_t>(pArmContext->arm_sp);
    }
#elif defined(__aarch64__)
    const ucontext_t *pUc = static_cast<const ucontext_t *>(pContext);
    if (pUc != nullptr)
    {
        nPc = static_cast<uintptr_t>(pUc->uc_mcontext.pc);
        nLr = static_cast<uintptr_t>(pUc->uc_mcontext.regs[30]);
        nSp = static_cast<uintptr_t>(pUc->uc_mcontext.sp);
    }
#endif

    char szLine[256];
    char *pOut = szLine;
    const char *pEnd = szLine + sizeof(szLine);
    pOut = appendLiteral(pOut, pEnd, "STREAM_CRASH signal=");
    pOut = appendDec(pOut, pEnd, static_cast<unsigned int>(nSigno));
    pOut = appendLiteral(pOut, pEnd, " fault=");
    pOut = appendHex(pOut, pEnd, reinterpret_cast<uintptr_t>(pInfo != nullptr ? pInfo->si_addr : nullptr));
    pOut = appendLiteral(pOut, pEnd, " pc=");
    pOut = appendHex(pOut, pEnd, nPc);
    pOut = appendLiteral(pOut, pEnd, " lr=");
    pOut = appendHex(pOut, pEnd, nLr);
    pOut = appendLiteral(pOut, pEnd, " sp=");
    pOut = appendHex(pOut, pEnd, nSp);
    pOut = appendLiteral(pOut, pEnd, "\n");

    const size_t nLength = static_cast<size_t>(pOut - szLine);
    char szRawContext[512];
    char *pRawOut = szRawContext;
    const char *pRawEnd = szRawContext + sizeof(szRawContext);
    pRawOut = appendLiteral(pRawOut, pRawEnd, "STREAM_MCONTEXT");
    if (pContext != nullptr)
    {
        const ucontext_t *pUc = static_cast<const ucontext_t *>(pContext);
        const uintptr_t *pWords = reinterpret_cast<const uintptr_t *>(&pUc->uc_mcontext);
        size_t nWordCount = sizeof(pUc->uc_mcontext) / sizeof(uintptr_t);
        if (nWordCount > 24U)
        {
            nWordCount = 24U;
        }
        for (size_t i = 0; i < nWordCount; ++i)
        {
            pRawOut = appendLiteral(pRawOut, pRawEnd, " ");
            pRawOut = appendHex(pRawOut, pRawEnd, pWords[i]);
        }
    }
    pRawOut = appendLiteral(pRawOut, pRawEnd, "\n");
    const size_t nRawLength = static_cast<size_t>(pRawOut - szRawContext);

    if (g_nCrashLogFd >= 0)
    {
        (void)write(g_nCrashLogFd, szLine, nLength);
        (void)write(g_nCrashLogFd, szRawContext, nRawLength);
        /*
         * PC/LR 可能落在启用 ASLR 的共享库中。崩溃时把本进程映射一并写入，
         * 后续才能由“绝对地址 - 映射起始地址 + 文件偏移”得到库内地址。
         */
        if (g_nProcMapsFd >= 0 && lseek(g_nProcMapsFd, 0, SEEK_SET) >= 0)
        {
            static const char kMapsBegin[] = "STREAM_MAPS_BEGIN\n";
            static const char kMapsEnd[] = "STREAM_MAPS_END\n";
            (void)write(g_nCrashLogFd, kMapsBegin, sizeof(kMapsBegin) - 1U);
            char szMaps[512];
            ssize_t nRead = 0;
            while ((nRead = read(g_nProcMapsFd, szMaps, sizeof(szMaps))) > 0)
            {
                (void)write(g_nCrashLogFd, szMaps, static_cast<size_t>(nRead));
            }
            (void)write(g_nCrashLogFd, kMapsEnd, sizeof(kMapsEnd) - 1U);
        }
        (void)fsync(g_nCrashLogFd);
    }
    (void)write(STDERR_FILENO, szLine, nLength);
    (void)write(STDERR_FILENO, szRawContext, nRawLength);

    /* SA_RESETHAND 已恢复默认动作；重新发送同一信号，仍可按系统配置生成 core。 */
    (void)kill(getpid(), nSigno);
}
#endif
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
#if ENABLE_CRASH_DIAGNOSTICS
    /*
     * memory: 崩溃日志写入 /run（tmpfs 内存盘），掉电丢失但零存储介质磨损；
     * 日志带完整 /proc/self/maps（约几十KB/次），SD 卡上反复追加会磨损且依赖插卡。
     * 崩溃诊断只需保留到本次运行结束，RAM 存放最合理。
     */
    g_nCrashLogFd = open("/run/stream_crash.log", O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
    g_nProcMapsFd = open("/proc/self/maps", O_RDONLY | O_CLOEXEC);

    struct sigaction stCrashAction;
    memset(&stCrashAction, 0, sizeof(stCrashAction));
    sigemptyset(&stCrashAction.sa_mask);
    stCrashAction.sa_sigaction = crashHandler;
    stCrashAction.sa_flags = SA_SIGINFO | SA_RESETHAND;
    sigaction(SIGSEGV, &stCrashAction, nullptr);
    sigaction(SIGBUS, &stCrashAction, nullptr);
    sigaction(SIGABRT, &stCrashAction, nullptr);
#endif
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

    /* 初始化线程性能观测服务，默认关闭；启用后必须早于业务线程启动。 */
    // nRet = CStreamPerformanceService::instance()->init();
    // if (nRet < OK)
    // {
    //     dlog_error("初始化线程性能观测服务失败：%d", nRet);
    //     goto exit_stream_performance;
    // }

    /* 初始化密码学模块（硬件加速 + OpenSSL Provider） */
    nRet = CCryptoInit::instance()->init();
    if (nRet < OK)
    {
        dlog_error("密码学模块初始化失败：%d", nRet);
        goto exit_crypto_init;
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
exit_crypto_init:
    /* 去初始化密码学模块 */
    nRet = CCryptoInit::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("密码学模块去初始化失败：%d", nRet);
    }
// exit_stream_performance:
    /* 停止观测线程，避免退出期间继续读取已释放的模块状态。 */
    // nRet = CStreamPerformanceService::instance()->deinit();
    // if (nRet < OK)
    // {
    //     dlog_error("去初始化线程性能观测服务失败：%d", nRet);
    // }
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

    /* 先停止汇总线程，避免模块退出过程中继续采样已经释放的业务线程。 */
    // nRet = CStreamPerformanceService::instance()->deinit();
    // if (nRet < OK)
    // {
    //     dlog_error("去初始化线程性能观测服务失败：%d", nRet);
    // }

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

    /* 去初始化密码学模块 */
    nRet = CCryptoInit::instance()->deinit();
    if (nRet < OK)
    {
        dlog_error("密码学模块去初始化失败：%d", nRet);
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
    /* 写入 info/debug/trace/warn/error 到该文件即可动态切换，setLogLevel 也会同步写入该文件 */
    startLogLevelMonitor(STREAM_LOG_LEVEL_PATH);

    dlog_trace("启动主程序");

    /* 初始化进程时区运行时，保证后续线程继承正确的 SIGHUP 屏蔽状态 */
    TimezoneRuntime_NS::init_timezone_runtime("stream");

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
