/**
 * @FilePath     : process_manager.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-05 16:05:04
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-02 16:33:13
 * @Description  : 进程管理器
 */

#include "process_manager.h"
#include "dlog.h"

#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <cerrno>
#include <cstring>

namespace fs = std::filesystem;

CProcessManager::CProcessManager(std::string strPidDir) :
    m_strPidDir(std::move(strPidDir))
{
}

bool CProcessManager::init_processes()
{
    dlog_info("初始化进程管理器...");

    // note --- 添加所有需要守护的进程 ---
    m_astProcesses = {
        {
            "upgrade",
            "/opt/cam/bin/upgrade",
            {}, /* 无参数 */
            0,  /* 启动无延迟 */
            "/opt/cam/bin"
        },
        {
            "stream",
            "/opt/cam/bin/stream",
            {},
            200,
            "/opt/cam/bin"
        },
        {
            "operation",
            "/opt/cam/bin/operation",
            {},
            4000,
            "/opt/cam/bin"
        },
        {
            "record",
            "/opt/cam/bin/record",
            {},
            100,
            "/opt/cam/bin"
        },
        {
            "nginx",
            "/opt/cam/third-party/nginx/sbin/nginx",
            {"-p", "/opt/cam/third-party/nginx/"}, /* nginx 的 -p 参数指定路径前缀 */
            500,  /* 启动前延迟0.5秒 */
            "/opt/cam/third-party/nginx/"
        }
        // note --- 如果有更多进程，请在这里添加 ---
    };

    dlog_info("进程管理器已初始化，将监控 %zu 个进程", m_astProcesses.size());
    return true;
}

void CProcessManager::initial_startup_sequence()
{
    dlog_info("开始首次启动进程...");
    for (const auto &proc : m_astProcesses)
    {
        /* 检查进程是否已在运行 (例如守护进程被重启，但业务程序还在) */
        if (is_process_running(proc.strName))
        {
            dlog_info("进程 [%s] 已经正在运行，跳过初始启动步骤.", proc.strName.c_str());
            continue;
        }

        /* 应用首次启动延迟 */
        if (proc.llStartupDelayMs > 0)
        {
            dlog_info("等待 %lld 毫秒后再启动 %s", proc.llStartupDelayMs, proc.strName.c_str());
            std::this_thread::sleep_for(std::chrono::milliseconds(proc.llStartupDelayMs));
        }

        /* 启动进程 */
        start_process(proc);
    }
    dlog_info("首次启动进程完毕.");
}

std::string CProcessManager::get_pid_file_path(const std::string &strProcessName) const
{
    return fs::path(m_strPidDir) / (strProcessName + ".pid");
}

pid_t CProcessManager::find_process_by_name(const std::string &strProcessName)
{
#if CAP_PROCESS_USE_PS_GREP // 进程探测 ps|grep
    std::string command = "ps | grep '" + strProcessName + "' | grep -v grep | awk '{print $1}'";
#else
    std::string command = "pidof -s " + strProcessName;
#endif
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe)
    {
        return -1;
    }

    char buffer[128];
    std::string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        result += buffer;
    }
    pclose(pipe);

#if CAP_PROCESS_USE_PS_GREP // 进程探测 ps|grep
    /* 清理结果*/
    result.erase(0, result.find_first_not_of(" \n\r\t"));
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
#endif
    if (result.empty())
    {
        return -1;
    }

    try
    {
        return std::stoi(result);
    }
    catch (...)
    {
        return -1;
    }
}

bool CProcessManager::is_process_running(const std::string &strProcessName)
{

    pid_t pid = find_process_by_name(strProcessName);
    if (pid <= 0)
    {
        return false;
    }

    /* 使用 kill(pid, 0) 来精确检查进程是否存在 */
    if (kill(pid, 0) == 0)
    {
        return true; /* 进程存在 */
    }
    else
    {
        if (errno == ESRCH)
        {
            dlog_warn("进程 %s (PID %d) 未找到.", strProcessName.c_str(), pid);
        }
        else
        {
            dlog_error("错误检查 进程 %s (PID %d): %s", strProcessName.c_str(), pid, strerror(errno));
        }
        return false;
    }
}

void CProcessManager::start_process(const ProcessInfo_S &stProc)
{
    dlog_info("尝试启动进程: %s", stProc.strName.c_str());

#if CAP_PROCESS_KILL_CROND_BEFORE_STREAM // 拉流前处理 crond
    // 特殊处理：启动stream前需要检查并结束crond进程
    if (stProc.strName == "stream")
    {
        dlog_info("启动stream进程，检查crond进程...");
        
        // 检查crond进程是否在运行
        if (is_process_running("crond"))
        {
            dlog_warn("检测到crond进程正在运行，尝试结束...");
            
            // 找到crond的PID
            pid_t crond_pid = find_process_by_name("crond");
            if (crond_pid > 0)
            {
                if (kill(crond_pid, SIGTERM) == 0)
                {
                    dlog_info("已发送SIGTERM信号给crond进程(PID: %d)", crond_pid);
                    
                    // 等待最多3秒让进程结束
                    for (int i = 0; i < 30; i++)
                    {
                        usleep(100000); // 100ms
                        if (!is_process_running("crond"))
                        {
                            dlog_info("crond进程已结束");
                            break;
                        }
                        
                        if (i == 15) // 1.5秒后如果还没结束，尝试强制结束
                        {
                            dlog_warn("crond进程未响应，尝试强制结束");
                            kill(crond_pid, SIGKILL);
                        }
                    }
                }
                else
                {
                    dlog_error("无法发送信号给crond进程(PID: %d): %s", 
                              crond_pid, strerror(errno));
                }
            }
        }
        else
        {
            dlog_info("crond进程未运行，无需处理");
        }
    }
#endif
    pid_t pid = fork();

    if (pid < 0)
    {
        dlog_error("fork() 执行失败 %s: %s", stProc.strName.c_str(), strerror(errno));
        return;
    }

    if (pid == 0)
    { /* 子进程 */
        /* 切换工作目录 */
        if (chdir(stProc.strWorkingDir.c_str()) != 0)
        {
            dlog_error("切换到 %s 目录失败 %s: %s", stProc.strWorkingDir.c_str(), stProc.strName.c_str(), strerror(errno));
            _exit(127);
        }

        /* 准备 execvp 的参数列表 */
        std::vector<const char *> argv_list;
        argv_list.push_back(stProc.strCommandPath.c_str());
        for (const auto &arg : stProc.aStrArgs)
        {
            argv_list.push_back(arg.c_str());
        }
        argv_list.push_back(nullptr); /* 参数列表以NULL结尾 */

        /* 设置你的脚本中需要的环境变量 */
        setenv("LD_LIBRARY_PATH", "/lib:/opt/cam/lib", 1);

        /* 执行新程序 */
        execv(stProc.strCommandPath.c_str(), const_cast<char *const *>(argv_list.data()));

        /* 如果 execv 返回，说明出错了 */
        dlog_error("execv 执行失败 %s: %s", stProc.strCommandPath.c_str(), strerror(errno));
        _exit(127);
    }
    else
    { /* 父进程 */
    }
}

void CProcessManager::check_and_manage_processes()
{
    for (const auto &proc : m_astProcesses)
    {
        if (!is_process_running(proc.strName))
        {
            dlog_error("进程[%s]未运行.正在启动...", proc.strName.c_str());
            start_process(proc);
        }
    }
}
