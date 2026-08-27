/**
 * @FilePath     : process_manager.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-05 16:04:57
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-06 09:41:01
 * @Description  : 进程管理器
 */

#pragma once

#include <string>
#include <vector>

/* 进程信息结构体 */
typedef struct ProcessInfo
{
    std::string strName;               /* 进程的唯一标识名, 用于生成 a.pid */
    std::string strCommandPath;        /* 可执行文件的完整路径 */
    std::vector<std::string> aStrArgs; /* 启动参数列表 */
    long long llStartupDelayMs;        /* 启动前延时 (毫秒) */
    std::string strWorkingDir;         /* 进程的工作目录 */
} ProcessInfo_S;

/* 进程管理器 */
class CProcessManager
{
public:
    /* 构造函数，可以设置PID文件的存放目录 */
    CProcessManager(std::string strPidDir = "/opt/cam/run");

    /**
     * @brief   : 初始化进程
     * @return   {bool} true:成功 false:失败
     */
    bool init_processes();

    /**
     * @brief   : 首次初始启动序列
     */
    void initial_startup_sequence();

    /**
     * @brief   : 周期性地检查和管理所有已定义的进程
     */
    void check_and_manage_processes();

private:

    /**
     * @brief   : 根据名称查找进程pid
     * @param    {string&} strProcessName
     * @return   {pid_t} 大于0:进程pid -1:查找失败
     */
    pid_t find_process_by_name(const std::string& strProcessName);

    /**
     * @brief   : 检查单个进程是否正在运行 (通过PID文件)
     * @param    {string&} strProcessName 进程名
     * @return   {bool} true:进程正在运行 false:未运行
     */
    bool is_process_running(const std::string& strProcessName);

    /**
     * @brief   : 启动指定的进程
     * @param    {ProcessInfo_S&} stProc 进程信息
     */
    void start_process(const ProcessInfo_S& stProc);

    /**
     * @brief   : 根据进程名获取其PID文件的完整路径
     * @param    {string&} strProcessName
     * @return   {string} 获取的PID文件的完整路径
     */
    std::string get_pid_file_path(const std::string& strProcessName) const;

private:
    /* PID文件的存放目录 */
    std::string m_strPidDir;
    /* 进程信息 */
    std::vector<ProcessInfo_S> m_astProcesses;
};
