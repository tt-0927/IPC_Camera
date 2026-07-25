/**
 * @FilePath     : timezone_runtime.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-06-05 10:08:32
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-05 15:51:28
 * @Description  : 进程时区运行时管理
 */

#include "timezone_runtime.h"

#include <atomic>
#include <cerrno>
#include <cstdio>
#include <csignal>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <thread>
#include <pthread.h>
#include <unistd.h>

#include "dlog.h"
#include "path_define.h"

namespace
{
/* 时区配置文件路径，和启动脚本保持一致 */
const char *TIMEZONE_CONFIG_FILE = CAM_PATH "shell/timezone.conf";

/* 默认 POSIX 时区，CST-8 表示 UTC+8 */
const char *DEFAULT_POSIX_TIMEZONE = "CST-8";

/* 需要即时刷新时区的业务进程列表 */
const char *TIMEZONE_RELOAD_PROCESS_LIST[] = {"record", "operation", "daemon", "upgrade"};

/* SIGHUP 监听线程启动状态，避免同一进程重复创建监听线程 */
std::atomic<bool> g_bListenerStarted(false);

/**
 * @brief   : 去掉字符串首尾空白字符
 * @param   {std::string} strValue：原始字符串
 * @return  {std::string} 去掉首尾空白后的字符串
 */
std::string trim_string(const std::string &strValue)
{
    const std::string strBlank = " \t\r\n";
    const size_t nBegin = strValue.find_first_not_of(strBlank);
    if (nBegin == std::string::npos)
    {
        return "";
    }

    const size_t nEnd = strValue.find_last_not_of(strBlank);
    return strValue.substr(nBegin, nEnd - nBegin + 1);
}

/**
 * @brief   : 根据进程名查找进程 PID
 * @param   {std::string} strProcessName：进程名称
 * @param   {pid_t &} nPid：输出 PID
 * @return  {IpcRet_E} OK：找到进程，非 OK：未找到或命令失败
 */
IpcRet_E find_process_pid(const std::string &strProcessName, pid_t &nPid)
{
    const std::string strCommand = "pidof -s " + strProcessName;
    FILE *pPipe = popen(strCommand.c_str(), "r");
    if (pPipe == nullptr)
    {
        dlog_error("查找进程[%s]失败, popen错误:%s", strProcessName.c_str(), strerror(errno));
        return ERR;
    }

    char achBuffer[64] = {0};
    std::string strResult;
    if (fgets(achBuffer, sizeof(achBuffer), pPipe) != nullptr)
    {
        strResult = achBuffer;
    }

    const int nCloseRet = pclose(pPipe);
    if (nCloseRet != OK)
    {
        dlog_debug("查找进程[%s]命令返回异常:%d", strProcessName.c_str(), nCloseRet);
    }

    strResult = trim_string(strResult);
    if (strResult.empty())
    {
        dlog_debug("进程[%s]未运行, 跳过时区刷新通知", strProcessName.c_str());
        return ERR;
    }

    try
    {
        nPid = static_cast<pid_t>(std::stoi(strResult));
    }
    catch (...)
    {
        dlog_error("解析进程[%s] PID失败:%s", strProcessName.c_str(), strResult.c_str());
        return ERR;
    }

    dlog_debug("查找进程[%s] PID成功:%d", strProcessName.c_str(), nPid);
    return OK;
}

/**
 * @brief   : SIGHUP 监听线程入口
 * @param   {std::string} strProcessName：当前进程名称
 * @return  {void}
 * @note    : sigwait 在线程上下文中处理 SIGHUP，避免在 signal handler 中调用非异步信号安全函数
 */
void timezone_reload_thread(std::string strProcessName)
{
    sigset_t stSigSet;
    sigemptyset(&stSigSet);
    sigaddset(&stSigSet, SIGHUP);

    dlog_debug("进程[%s]时区SIGHUP监听线程启动", strProcessName.c_str());
    while (true)
    {
        int nSignal = 0;
        const int nRet = sigwait(&stSigSet, &nSignal);
        if (nRet != OK)
        {
            dlog_error("进程[%s]等待SIGHUP失败:%s", strProcessName.c_str(), strerror(nRet));
            continue;
        }

        dlog_debug("进程[%s]收到SIGHUP, 开始重新加载时区", strProcessName.c_str());
        TimezoneRuntime_NS::reload_timezone(strProcessName, "SIGHUP");
    }
}
} // namespace

namespace TimezoneRuntime_NS
{
const char *get_default_timezone()
{
    return DEFAULT_POSIX_TIMEZONE;
}

const char *to_posix_timezone(int nTimeZone)
{
    switch (nTimeZone)
    {
    case -12:
        return "GMT+12";
    case -11:
        return "GMT+11";
    case -10:
        return "GMT+10";
    case -9:
        return "GMT+9";
    case -8:
        return "GMT+8";
    case -7:
        return "GMT+7";
    case -6:
        return "GMT+6";
    case -5:
        return "GMT+5";
    case -4:
        return "GMT+4";
    case -3:
        return "GMT+3";
    case -2:
        return "GMT+2";
    case -1:
        return "GMT+1";
    case 0:
        return "GMT0";
    case 1:
        return "GMT-1";
    case 2:
        return "GMT-2";
    case 3:
        return "GMT-3";
    case 4:
        return "GMT-4";
    case 5:
        return "GMT-5";
    case 6:
        return "GMT-6";
    case 7:
        return "GMT-7";
    case 8:
        return "CST-8";
    case 9:
        return "JST-9";
    case 10:
        return "GMT-10";
    case 11:
        return "GMT-11";
    case 12:
        return "GMT-12";
    case 13:
        return "GMT-13";
    default:
        dlog_warn("未知时区枚举:%d, 使用默认时区:%s", nTimeZone, DEFAULT_POSIX_TIMEZONE);
        return DEFAULT_POSIX_TIMEZONE;
    }
}

IpcRet_E read_timezone_config(std::string &strTimezone)
{
    std::ifstream stFile(TIMEZONE_CONFIG_FILE);
    if (!stFile.is_open())
    {
        strTimezone = DEFAULT_POSIX_TIMEZONE;
        dlog_debug("时区配置文件[%s]不存在, 使用默认时区:%s", TIMEZONE_CONFIG_FILE, strTimezone.c_str());
        return ERR;
    }

    std::ostringstream stBuffer;
    stBuffer << stFile.rdbuf();
    strTimezone = trim_string(stBuffer.str());
    if (strTimezone.empty())
    {
        strTimezone = DEFAULT_POSIX_TIMEZONE;
        dlog_debug("时区配置文件[%s]内容为空, 使用默认时区:%s", TIMEZONE_CONFIG_FILE, strTimezone.c_str());
        return ERR;
    }

    dlog_debug("读取时区配置文件[%s]成功, 时区:%s", TIMEZONE_CONFIG_FILE, strTimezone.c_str());
    return OK;
}

IpcRet_E write_timezone_config(const std::string &strTimezone)
{
    const std::string strCommand = "mkdir -p " CAM_PATH "shell";
    const int nMkdirRet = system(strCommand.c_str());
    if (nMkdirRet != OK)
    {
        dlog_error("创建时区配置目录失败, 命令返回:%d", nMkdirRet);
        return ERR;
    }

    std::ofstream stFile(TIMEZONE_CONFIG_FILE, std::ios::trunc);
    if (!stFile.is_open())
    {
        dlog_error("打开时区配置文件[%s]失败:%s", TIMEZONE_CONFIG_FILE, strerror(errno));
        return ERR;
    }

    stFile << strTimezone << std::endl;
    if (!stFile.good())
    {
        dlog_error("写入时区配置文件[%s]失败", TIMEZONE_CONFIG_FILE);
        return ERR;
    }

    dlog_debug("写入时区配置文件[%s]成功, 时区:%s", TIMEZONE_CONFIG_FILE, strTimezone.c_str());
    return OK;
}

IpcRet_E reload_timezone(const std::string &strProcessName, const std::string &strReason)
{
    std::string strTimezone;
    IpcRet_E enRet = read_timezone_config(strTimezone);
    if (enRet != OK)
    {
        dlog_debug("进程[%s]读取时区配置失败, 原因[%s], 使用默认时区:%s",
                   strProcessName.c_str(), strReason.c_str(), strTimezone.c_str());
    }

    const char *pOldTimezone = getenv("TZ");
    dlog_debug("进程[%s]刷新时区, 原因[%s], 旧时区:%s, 新时区:%s",
               strProcessName.c_str(), strReason.c_str(), pOldTimezone ? pOldTimezone : "", strTimezone.c_str());

    if (setenv("TZ", strTimezone.c_str(), 1) != OK)
    {
        dlog_error("进程[%s]设置TZ失败:%s", strProcessName.c_str(), strerror(errno));
        return ERR;
    }

    tzset();
    const char *pCurrentTimezone = getenv("TZ");
    dlog_debug("进程[%s]刷新时区完成, 当前TZ:%s", strProcessName.c_str(),
               pCurrentTimezone ? pCurrentTimezone : "");
    return OK;
}

IpcRet_E init_timezone_runtime(const std::string &strProcessName)
{
    /* daemonize 中可能忽略 SIGHUP，这里恢复默认并阻塞，统一交给 sigwait 线程处理 */
    signal(SIGHUP, SIG_DFL);

    sigset_t stSigSet;
    sigemptyset(&stSigSet);
    sigaddset(&stSigSet, SIGHUP);
    const int nMaskRet = pthread_sigmask(SIG_BLOCK, &stSigSet, nullptr);
    if (nMaskRet != OK)
    {
        dlog_error("进程[%s]阻塞SIGHUP失败:%s", strProcessName.c_str(), strerror(nMaskRet));
        return ERR;
    }

    IpcRet_E enRet = reload_timezone(strProcessName, "init");
    if (g_bListenerStarted.exchange(true))
    {
        dlog_debug("进程[%s]时区SIGHUP监听线程已启动, 跳过重复启动", strProcessName.c_str());
        return enRet;
    }

    try
    {
        std::thread stThread(timezone_reload_thread, strProcessName);
        stThread.detach();
    }
    catch (...)
    {
        g_bListenerStarted.store(false);
        dlog_error("进程[%s]创建时区SIGHUP监听线程失败", strProcessName.c_str());
        return ERR;
    }

    dlog_debug("进程[%s]时区运行时初始化完成", strProcessName.c_str());
    return enRet;
}

IpcRet_E notify_timezone_reload(const std::string &strSourceProcess)
{
    IpcRet_E enFinalRet = OK;
    for (const char *pProcessName : TIMEZONE_RELOAD_PROCESS_LIST)
    {
        pid_t nPid = -1;
        if (find_process_pid(pProcessName, nPid) != OK)
        {
            enFinalRet = ERR;
            continue;
        }

        if (kill(nPid, SIGHUP) != OK)
        {
            dlog_error("进程[%s]通知进程[%s](PID:%d)重新加载时区失败:%s",
                       strSourceProcess.c_str(), pProcessName, nPid, strerror(errno));
            enFinalRet = ERR;
            continue;
        }

        dlog_debug("进程[%s]已通知进程[%s](PID:%d)重新加载时区", strSourceProcess.c_str(), pProcessName, nPid);
    }

    return enFinalRet;
}
} // namespace TimezoneRuntime_NS
