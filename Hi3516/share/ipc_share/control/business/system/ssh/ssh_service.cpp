/**
 * @FilePath     : ssh_service.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-16 10:25:57
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-16 14:28:04
 * @Description  : SSH 服务管理实现
 */

#include "ssh_service.h"

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

#include <sys/stat.h>

#include "IpcRet.h"
#include "dlog.h"
#include "time_manage.h"

namespace
{
    /* 检查指定 SSH 定时任务是否已存在 */
    constexpr const char* kCronCheck = "crontab -l | grep -q '%s'";
    /* 删除指定 SSH 定时任务 */
    constexpr const char* kCronDelete = "crontab -l | grep -v '%s' | crontab -";
    /* 向 crontab 追加指定 SSH 定时任务 */
    constexpr const char* kCronAdd = "(crontab -l; echo '%s') | crontab -";
    /* 检查 crond 进程是否存在 */
    constexpr const char* kCrondFindPid = "pgrep crond";
    /* 检查 dropbear 进程是否存在 */
    constexpr const char* kDropbearFindPid = "pgrep dropbear";
    /* dropbear 主机密钥文件路径 */
    constexpr const char* kDropbearKeyPath = "/etc/dropbear/dropbear_ed25519_host_key";
    /* 生成 dropbear 主机密钥命令 */
    constexpr const char* kDropbearKeyAdd = "/bin/dropbearkey -t ed25519 -f /etc/dropbear/dropbear_ed25519_host_key";
    /* 设置主机密钥权限命令 */
    constexpr const char* kDropbearKeyChmod = "chmod 600 /etc/dropbear/dropbear_ed25519_host_key";
    /* dropbear 服务程序路径 */
    constexpr const char* kDropbearBin = "/sbin/dropbear";
    /* 正常关闭全部 dropbear 进程的命令 */
    constexpr const char* kDropbearStop = "killall dropbear >/dev/null 2>&1";
    /* SSH 超时自动关闭脚本路径 */
    constexpr const char* kStopScript = "/var/spool/cron/crontabs/stop_ssh_service.sh";
    /* SSH 启动时间戳文件路径 */
    constexpr const char* kStartTimeFile = "/var/run/ssh_service_start.time";
    /* SSH 单次启用后的默认有效时长，单位：秒 */
    constexpr int kSshExpireSeconds = 8 * 3600;
    /* 启动后检查监听状态的最大重试次数 */
    constexpr int kStartRetryTimes = 15;
    /* 启动后每次检查监听状态的间隔，单位：毫秒 */
    constexpr int kStartRetryIntervalMs = 200;
    /* 关闭后检查端口释放状态的最大重试次数 */
    constexpr int kStopRetryTimes = 10;
    /* 关闭后每次检查端口释放状态的间隔，单位：毫秒 */
    constexpr int kStopRetryIntervalMs = 100;

    /**
     * @brief   : 执行 shell 命令
     * @param    {const std::string &} cmd：待执行命令
     * @return   {int} 0：成功，非0：失败
     */
    int exec_cmd(const std::string& cmd)
    {
        return std::system(cmd.c_str());
    }

    /**
     * @brief   : 执行格式化 shell 命令
     * @param    {const char *} fmt：命令格式字符串
     * @return   {int} 0：成功，非0：失败
     */
    int run_cmd(const char* fmt, ...)
    {
        char cmd[512] = { 0 };
        va_list args;
        va_start(args, fmt);
        vsnprintf(cmd, sizeof(cmd), fmt, args);
        va_end(args);
        return exec_cmd(cmd);
    }

    /**
     * @brief   : 检查指定 crontab 任务是否存在
     * @param    {const char *} task：待检查任务内容
     * @return   {bool} true：存在，false：不存在
     */
    bool cron_check(const char* task)
    {
        return 0 == run_cmd(kCronCheck, task);
    }

    /**
     * @brief   : 添加指定 crontab 任务
     * @param    {const char *} task：待添加任务内容
     * @return   {int} 0：成功，非0：失败
     */
    int cron_add(const char* task)
    {
        return 0 == run_cmd(kCronAdd, task) ? OK : ERR;
    }

    /**
     * @brief   : 删除指定 crontab 任务
     * @param    {const char *} task：待删除任务内容
     * @return   {int} 0：成功，非0：失败
     */
    int cron_delete(const char* task)
    {
        return 0 == run_cmd(kCronDelete, task) ? OK : ERR;
    }

    /**
     * @brief   : 检查进程是否正在运行
     * @param    {const char *} find_cmd：进程检查命令
     * @return   {bool} true：正在运行，false：未运行
     */
    bool is_process_running(const char* find_cmd)
    {
        return 0 == exec_cmd(std::string(find_cmd) + " >/dev/null 2>&1");
    }

    /**
     * @brief   : 从指定 TCP 状态文件中检查端口是否处于监听状态
     * @param    {const char *} path：TCP 状态文件路径
     * @param    {int} port：待检查端口
     * @return   {bool} true：监听中，false：未监听
     */
    bool is_port_listening_in_file(const char* path, int port)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            return false;
        }
        std::string line;
        std::getline(file, line);
        while (std::getline(file, line))
        {
            unsigned int local_port = 0;
            char state[3] = { 0 };
            if (2 == std::sscanf(line.c_str(), "%*d: %*[^:]:%X %*[^:]:%*X %2s", &local_port, state) &&
                local_port == static_cast<unsigned int>(port) && 0 == std::strcmp(state, "0A"))
                return true;
        }
        return false;
    }

    /**
     * @brief   : 检查端口是否处于监听状态
     * @param    {int} port：待检查端口
     * @return   {bool} true：监听中，false：未监听
     */
    bool is_port_listening(int port)
    {
        return is_port_listening_in_file("/proc/net/tcp", port) || is_port_listening_in_file("/proc/net/tcp6", port);
    }

    /**
     * @brief   : 检查 SSH 服务是否可用
     * @param    {int} port：SSH 服务端口
     * @return   {bool} true：可用，false：不可用
     * @note    : 仅当 dropbear 进程存在且目标端口处于监听态时，才视为服务可用
     */
    bool is_service_active(int port)
    {
        return is_process_running(kDropbearFindPid) && is_port_listening(port);
    }

    /**
     * @brief   : 同步 crond 服务状态
     * @param    {bool} need_running：是否需要保持 crond 运行
     * @return   {int} 0：成功，非0：失败
     * @note    : SSH 自动关闭和自动维护都依赖 crond
     */
    int sync_crond_state(bool need_running)
    {
        const bool running = is_process_running(kCrondFindPid);
        if (need_running)
        {
            if (running)
                return OK;
            if (0 != exec_cmd("crond"))
            {
                dlog_error("启动 crond 服务失败");
                return ERR;
            }
            dlog_debug("启动 crond 服务成功");
            return OK;
        }
        if (!running)
            return OK;
        exec_cmd("killall crond >/dev/null 2>&1");
        dlog_debug("停止 crond 服务");
        return OK;
    }

    /**
     * @brief   : 确保 SSH 主机密钥存在
     * @return   {int} 0：成功，非0：失败
     * @note    : SSH 首次启用前必须先生成主机密钥，否则 dropbear 无法正常启动
     */
    int ensure_host_key()
    {
        if (std::filesystem::exists(kDropbearKeyPath))
            return OK;
        if (0 != exec_cmd(kDropbearKeyAdd) || 0 != exec_cmd(kDropbearKeyChmod))
        {
            dlog_error("生成 SSH 主机密钥失败");
            return ERR;
        }
        dlog_debug("生成 SSH 主机密钥成功");
        return OK;
    }

    /**
     * @brief   : 删除旧的 SSH 自动关闭任务
     * @return   {int} 0：成功，非0：失败
     * @note    : 每次启停前都要先清理旧任务，避免 crontab 中出现重复项
     */
    int remove_stop_task()
    {
        if (cron_check(kStopScript) && OK != cron_delete(kStopScript))
        {
            dlog_error("删除 SSH 自动关闭任务失败");
            return ERR;
        }
        return OK;
    }

    /**
     * @brief   : 生成 SSH 自动关闭脚本
     * @return   {int} 0：成功，非0：失败
     * @note    : 脚本根据启动时间戳判断是否到达 8 小时超时点
     */
    int write_stop_script()
    {
        std::ofstream script(kStopScript);
        if (!script.is_open())
        {
            dlog_error("写入 SSH 自动关闭脚本失败");
            return ERR;
        }
        script << "START_FILE=" << kStartTimeFile << "\n";
        script << "EXPIRY_SECONDS=" << kSshExpireSeconds << "\n";
        script << "CURRENT_TIME=$(date +%s)\n";
        script << "START_TIME=$(cat \"$START_FILE\" 2>/dev/null || echo 0)\n";
        script << "if [ \"$START_TIME\" -le 0 ]; then\n";
        script << "    exit 0\n";
        script << "fi\n";
        script << "ELAPSED=$((CURRENT_TIME - START_TIME))\n";
        script << "if [ \"$ELAPSED\" -ge \"$EXPIRY_SECONDS\" ]; then\n";
        script << "    " << kDropbearStop << "\n";
        script << "    crontab -l | grep -v '" << kStopScript << "' | crontab -\n";
        script << "    rm -f \"$START_FILE\"\n";
        script << "    rm -f \"$0\"\n";
        script << "fi\n";
        script.close();
        chmod(kStopScript, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        dlog_debug("刷新 SSH 自动关闭脚本成功");
        return OK;
    }

    /**
     * @brief   : 添加 SSH 自动关闭任务
     * @return   {int} 0：成功，非0：失败
     * @note    : 通过 crond 每分钟执行一次自动关闭脚本
     */
    int add_stop_task()
    {
        std::stringstream ss;
        ss << "* * * * * " << kStopScript;
        if (OK != cron_add(ss.str().c_str()))
        {
            dlog_error("添加 SSH 自动关闭任务失败");
            return ERR;
        }
        dlog_debug("添加 SSH 自动关闭任务成功");
        return OK;
    }

    /**
     * @brief   : 读取 SSH 启动时间戳
     * @param    {std::time_t &} start_time：启动时间戳
     * @return   {bool} true：读取成功，false：读取失败
     * @note    : 该时间戳是倒计时计算的唯一可信来源
     */
    bool read_start_timestamp(std::time_t& start_time)
    {
        std::ifstream file(kStartTimeFile);
        if (!file.is_open())
        {
            return false;
        }
        file >> start_time;
        return !file.fail() && start_time > 0;
    }

    /**
     * @brief   : 写入 SSH 启动时间戳
     * @param    {std::time_t} start_time：启动时间戳
     * @return   {int} 0：成功，非0：失败
     */
    int write_start_timestamp(std::time_t start_time)
    {
        std::ofstream file(kStartTimeFile);
        if (!file.is_open())
        {
            dlog_error("写入 SSH 启动时间戳失败");
            return ERR;
        }
        file << start_time;
        return OK;
    }

    /**
     * @brief   : 清理 SSH 启动时间戳文件
     * @return   {void}
     */
    void clear_start_timestamp()
    {
        std::filesystem::remove(kStartTimeFile);
    }

    /**
     * @brief   : 格式化倒计时字符串
     * @param    {int} remaining_seconds：剩余秒数
     * @return   {std::string} 格式化后的 HH:MM:SS 字符串
     */
    std::string format_countdown(int remaining_seconds)
    {
        const int hours = remaining_seconds / 3600;
        const int minutes = (remaining_seconds % 3600) / 60;
        const int seconds = remaining_seconds % 60;
        char buffer[16] = { 0 };
        std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hours, minutes, seconds);
        return buffer;
    }

    /**
     * @brief   : 获取默认倒计时字符串
     * @return   {std::string} 默认的 08:00:00
     */
    std::string default_countdown()
    {
        return format_countdown(kSshExpireSeconds);
    }

    /**
     * @brief   : 等待 SSH 服务进入监听状态
     * @param    {int} port：SSH 服务端口
     * @return   {int} 0：成功，非0：失败
     * @note    : 启动成功判定以目标端口进入监听态为准
     */
    int wait_service_ready(int port)
    {
        for (int i = 0; i < kStartRetryTimes; ++i)
        {
            if (is_service_active(port))
            {
                dlog_debug("SSH 服务监听确认成功，端口[%d]", port);
                return OK;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(kStartRetryIntervalMs));
        }
        dlog_error("SSH 服务启动失败，端口[%d]未进入监听状态", port);
        return ERR;
    }

    /**
     * @brief   : 启动 SSH 服务
     * @param    {int} port：SSH 服务端口
     * @return   {int} 0：成功，非0：失败
     * @note    : 启动 dropbear 后，会继续校验目标端口是否真正进入监听态
     */
    int start_service(int port)
    {
        char cmd[512] = { 0 };
        std::snprintf(cmd, sizeof(cmd), "%s -r %s -p %d >/dev/null 2>&1 &", kDropbearBin, kDropbearKeyPath, port);
        dlog_debug("启动 SSH 服务，端口[%d]", port);
        exec_cmd(cmd);
        return wait_service_ready(port);
    }

    /**
     * @brief   : 关闭 SSH 服务
     * @param    {int} port：SSH 服务端口
     * @return   {int} 0：成功，非0：失败
     * @note    : 关闭成功以目标端口不再监听为准
     */
    int stop_service(int port)
    {
        if (!is_port_listening(port))
        {
            dlog_debug("SSH 服务监听端口[%d]已关闭，无需重复关闭", port);
            return OK;
        }
        exec_cmd(kDropbearStop);
        for (int i = 0; i < kStopRetryTimes; ++i)
        {
            if (!is_port_listening(port))
            {
                if (is_process_running(kDropbearFindPid))
                {
                    dlog_warn("SSH 监听端口[%d]已关闭，但仍存在残留 dropbear 进程，视为关闭成功", port);
                }
                else
                {
                    dlog_debug("关闭 SSH 服务成功，端口[%d]已停止监听", port);
                }
                return OK;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(kStopRetryIntervalMs));
        }
        exec_cmd("killall -9 dropbear >/dev/null 2>&1");
        for (int i = 0; i < kStopRetryTimes; ++i)
        {
            if (!is_port_listening(port))
            {
                dlog_warn("强制关闭 SSH 服务后，端口[%d]已停止监听", port);
                return OK;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(kStopRetryIntervalMs));
        }
        dlog_error("关闭 SSH 服务失败，端口[%d]仍处于监听状态", port);
        return ERR;
    }

    /**
     * @brief   : 清理 SSH 运行期信息
     * @param    {System::SshAdmin_S &} stInfo：SSH 管理信息
     * @return   {void}
     * @note    : 关闭 SSH 后会清空启动时间，并把倒计时重置为默认展示值
     */
    void clear_runtime_info(System::SshAdmin_S& stInfo)
    {
        stInfo.strSshStartTime.clear();
        stInfo.strSshCountdown = default_countdown();
        clear_start_timestamp();
    }
} // namespace

int SshServiceManager::apply(System::SecurityServices_S& stInfo, bool bAutoMaintain)
{
    if (OK != remove_stop_task())
    {
        return ERR;
    }
    if (!stInfo.stSshAdmin.bSshEnable)
    {
        if (OK != stop_service(stInfo.stSshAdmin.nSshPort))
        {
            return ERR;
        }
        clear_runtime_info(stInfo.stSshAdmin);
        return sync_crond_state(bAutoMaintain);
    }
    if (OK != ensure_host_key() || OK != sync_crond_state(true))
    {
        return ERR;
    }

    bool started_now = false;
    if (!is_service_active(stInfo.stSshAdmin.nSshPort))
    {
        if (OK != start_service(stInfo.stSshAdmin.nSshPort))
        {
            return ERR;
        }
        stInfo.stSshAdmin.strSshStartTime = CTimeManage::instance()->get_device_time();
        if (OK != write_start_timestamp(std::time(nullptr)))
        {
            return ERR;
        }
        started_now = true;
    }
    if (!started_now)
    {
        std::time_t start_time = 0;
        if (!read_start_timestamp(start_time))
        {
            stInfo.stSshAdmin.strSshStartTime = CTimeManage::instance()->get_device_time();
            if (OK != write_start_timestamp(std::time(nullptr)))
            {
                return ERR;
            }
        }
    }
    if (OK != write_stop_script() || OK != add_stop_task())
    {
        return ERR;
    }

    if (OK != get_countdown(stInfo.stSshAdmin.nSshPort, stInfo.stSshAdmin.strSshCountdown))
    {
        stInfo.stSshAdmin.strSshCountdown = default_countdown();
    }
    dlog_debug("SSH 服务启用成功，端口[%d]，倒计时[%s]",
               stInfo.stSshAdmin.nSshPort,
               stInfo.stSshAdmin.strSshCountdown.c_str());
    return OK;
}

int SshServiceManager::get_countdown(int port, std::string& strCountdown) const
{
    std::time_t start_time = 0;
    if (!is_service_active(port) || !read_start_timestamp(start_time))
    {
        strCountdown = default_countdown();
        dlog_debug("SSH 服务未运行，返回默认倒计时[%s]", strCountdown.c_str());
        return OK;
    }
    const int elapsed = static_cast<int>(std::difftime(std::time(nullptr), start_time));
    const int remaining = std::max(0, kSshExpireSeconds - elapsed);
    if (remaining <= 0)
    {
        strCountdown = default_countdown();
        dlog_debug("SSH 服务已超时，返回默认倒计时[%s]", strCountdown.c_str());
        return OK;
    }
    strCountdown = format_countdown(remaining);
    return OK;
}
