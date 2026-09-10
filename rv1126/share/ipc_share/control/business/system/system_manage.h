/**
 * @FilePath     : system_manage.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-01-21 11:12:16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-16 14:28:25
 * @Description  : 系统管理
 */

#pragma once

#include <thread>
#include <vector>
#include <atomic>
#include <functional>
#include "Singleton.h"
#include "system_define.h"
#include "IpcRet.h"
#include "ssh_account.h"
#include "ssh_service.h"
// #include "PoE.h"
// #include "gpio_ctrl.h"
#include "IoInput.h"

#define UPTIME_FILE "/opt/cam/.runtime" /* 设备运行时长文件 */

#define CRON_DIR  "/var/spool/cron/crontabs"                /* 定时任务配置文件目录 */
#define CROND_FIND_PID "pgrep crond"                        /* 检查服务是否运行 */
#define CRON_CHECK  "crontab -l | grep -q '%s'"             /* 检查任务 */
#define CRON_DELETE "crontab -l | grep -v '%s' | crontab -" /* 删除任务 */
#define CRON_ADD   "(crontab -l; echo '%s') | crontab -"    /* 添加任务 */

#define REBOOT_CMD "/sbin/reboot"                           /* 自动维护重启任务 */

class SystemManage : public CSingleton<SystemManage>
{
    SystemManage();

public:
    ~SystemManage() = default;
    friend class CSingleton<SystemManage>;

    IpcRet_E init();
    IpcRet_E deinit();

    /**
     * @brief 设置设备配置
     * @param stDeviceConfig 设备配置
     * @return int
     */
    int set_device_config(System::DeviceConfig_S stDeviceConfig);
    /**
     * @brief 获取设备配置
     * @param stDeviceConfig 设备配置
     * @return int
     */
    int get_device_config(System::DeviceConfig_S &stDeviceConfig);
    
    /**
     * @brief 获取设备信息
     * @param stDeviceInfo 设备信息
     * @return int
     */
    int set_device_info(System::DeviceInfo_S stDeviceInfo);
    /**
     * @brief 获取设备信息
     * @param stDeviceInfo 设备信息
     * @return int
     */
    int get_device_info(System::DeviceInfo_S &stDeviceInfo);

    /**
     * @brief 设备重启
     * @param result
     * @return int
     */
    int system_reboot(std::function<void(int)> result);

    /**
     * @brief 设备简单恢复
     * @param result
     * @return int
     */
    int system_reset_simple(std::function<void(int)> result);

    /**
     * @brief 设备完全恢复
     * @param result
     * @return int
     */
    int system_reset_complete(std::function<void(int)> result);

    bool cron_check(const char *pTask);
    int cron_add(const char *pTask);
    int cron_delete(const char *pTask);

    /**
     * @brief 获取自动维护信息
     * @param stInfo 自动维护信息
     * @return int
     */
     void get_automatic_maintain_info(System::UpgradeMaintain_S &stInfo);

    /**
     * @brief 设置自动维护信息
     * @param stInfo 自动维护信息
     * @return int
     */
    int set_automatic_maintain_info(System::UpgradeMaintain_S stInfo);

    /***
     * @description : 获取安全服务信息
     * @author      : huangjunda
     * @param        {SecurityServices_S} &stInfo
     * @return       {*}
     */
    void get_security_services_info(System::SecurityServices_S &stInfo);

    /**
     * @brief   : 初始化 SSH 用户组
     * @return   {void}
     */
    void init_ssh_admin_group();

    /**
     * @brief   : 设置 SSH 用户密码
     * @param    {const char *} username：SSH 账号
     * @param    {const char *} password：SSH 密码
     * @return   {int} 0：成功，非0：失败
     */
    int set_ssh_password(const char* username, const char* password);

    /**
     * @brief   : 添加或更新 SSH 账号
     * @param    {const char *} username：SSH 账号
     * @param    {const char *} password：SSH 密码
     * @return   {int} 0：成功，非0：失败
     */
    int add_ssh_admin(const char* username, const char* password);

    /**
     * @brief   : 删除 SSH 账号
     * @param    {const char *} username：SSH 账号
     * @return   {int} 0：成功，非0：失败
     */
    int del_ssh_admin(const char* username);

    /**
     * @brief   : 设置安全服务信息
     * @param    {System::SecurityServices_S} stInfo：安全服务配置
     * @return   {int} 0：成功，非0：失败
     */
    int set_security_services_info(System::SecurityServices_S stInfo);

    /**
     * @brief   : 获取 SSH 剩余运行时间
     * @param    {std::string &} strCountdown：剩余时间字符串
     * @return   {int} 0：成功，非0：失败
     * @note    : 当 SSH 已关闭时，返回默认的 08:00:00
     */
    int get_countdown(std::string &strCountdown);

    /*** 
     * @description : 导出设备参数
     * @author      : huangjunda
     * @param        {AllDeviceParam_S} &stAllDeviceParam
     * @return       {*}
     */    
    int export_device_param(System::AllDeviceParam_S &stAllDeviceParam);

    /*** 
     * @description : 导入设备参数
     * @author      : huangjunda
     * @param        {AllDeviceParam_S} &stAllDeviceParam
     * @return       {*}
     */    
    int import_device_param(System::AllDeviceParam_S &stAllDeviceParam);

    /**
     * @brief 开始记录运行时长日志
     */
    void start_logUptime();

    /**
     * @brief 结束记录运行时长日志
     */
     void stop_logUptime();

    int enable_ioInputNumber(std::map<int, bool> numberSet);
    void callback_ioInputEvent(int nNumber);

    /**
     * @brief 获取设备序列号
     * @return std::string 设备序列号
     */
    std::string get_cpu_serialNumber();

private:

    void update_upTime();
    long get_process_startTime(const std::string& strProcessName);
    long getSystemUptime();

    /**
     * @brief io输入
     */
    IoInput m_ioInput;
    /**
     * @brief 设备信息文件
     */
    std::string m_deviceInfoFile;
    /**
     * @brief 设备配置文件
     */
    std::string m_deviceConfigFile;
    /**
     * @brief 安全服务配置文件
     */
    std::string m_securityServicesFile;
    /**
     * @brief 自动维护配置文件
     */
    std::string m_upgradeMaintainFile;

    /**
     * @brief 程序信息
     */
    System::ProgramInfo_S m_stProgramInfo;
    /* 设备使用时长文件 */
    std::string m_UptimeFile;
    /* SSH 账号管理 */
    SshAccountManager m_sshAccountManager;
    /* SSH 服务管理 */
    SshServiceManager m_sshServiceManager;
    /* 日志记录线程 */
    std::thread m_logThread;
    /* 日志记录线程退出标志 */
    std::atomic<bool> m_bLogThreadExitFlag;
    /* 第一次开机累计的总运行时长 */
    std::atomic<long> m_lgPersistedTotal{0};    
    /* 上一次保存时的系统运行时间 */
    std::atomic<long> m_lgLastsaveduptime{0};

    time_t program_start_time_;
};

