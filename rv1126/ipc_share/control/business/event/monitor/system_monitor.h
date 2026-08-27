/**
 * @FilePath     : system_monitor.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-10-31 9:38:25
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-05 17:09:29
 * @Description  : 事件异常检测使用的系统状态监控接口，负责采集存储、网络、IP冲突和非法访问状态
 */

#pragma once

#include <string>

#include "Singleton.h"

class CSystemMonitor : public CSingleton<CSystemMonitor>
{
public:
    CSystemMonitor();
    ~CSystemMonitor() = default;
    friend class CSingleton<CSystemMonitor>;

    struct SystemStatus
    {
        /* 磁盘使用率，单位为百分比。 */
        double diskUsage = 0.0;

        /* 磁盘错误标志位，包含未挂载、只读挂载和空间异常等状态。 */
        bool diskErrorFlag = false;

        /* 网络链路连接状态。 */
        bool networkConnected = false;

        /* IP地址冲突标志位。 */
        bool ipConflictFlag = false;

        /* 本轮检测到的非法访问尝试次数。 */
        int unauthorizedAccessAttempts = 0;
    };

    /**
     * @brief   : 获取当前系统状态快照
     * @return  {SystemStatus} 当前磁盘、网络、IP冲突和非法访问检测结果
     */
    SystemStatus get_current_status();

private:
    /**
     * @brief   : 检测指定路径所在文件系统的磁盘使用率是否超过阈值
     * @param   {const std::string&} path：待检测的挂载路径
     * @param   {double&} usage：输出磁盘使用率，单位为百分比
     * @return  {bool} true：磁盘使用率超过阈值，false：磁盘使用率正常或状态读取失败
     */
    bool check_disk_usage(const std::string &path, double &usage);

    /**
     * @brief   : 检查SD卡设备是否存在
     * @return  {bool} true：SD卡设备存在，false：SD卡设备不存在
     */
    bool is_sd_card_exist();

    /**
     * @brief   : 检测指定挂载点对应的SD卡是否处于异常状态
     * @param   {const std::string&} mount_path：SD卡挂载路径
     * @return  {bool} true：SD卡异常，false：SD卡正常或SD卡不存在
     */
    bool check_sd_error(const std::string &mount_path);

    /**
     * @brief   : 通过网卡列表和外部连通性检测网络是否可用
     * @return  {bool} true：网络可用，false：网络不可用
     */
    bool check_network_connection();

    /**
     * @brief   : 通过sysfs carrier节点检测指定网口的以太网链路状态
     * @param   {const std::string&} strCarrierPath：网口carrier节点路径
     * @return  {int} 1：链路已连接，0：链路未连接或读取失败
     */
    int check_link_by_carrier(const std::string &strCarrierPath);

    /**
     * @brief   : 检测指定网卡是否存在IP地址冲突
     * @param   {const std::string&} interface：网卡名称
     * @return  {bool} true：检测到IP冲突，false：未检测到IP冲突或检测失败
     */
    bool detect_ip_conflict(const std::string &interface = "eth0");

    /**
     * @brief   : 统计业务日志中的非法访问尝试次数
     * @return  {int} 本轮检测新增的非法访问尝试次数
     */
    int detect_unauthorized_access();

    /**
     * @brief   : 增量分析指定认证日志中的非法访问关键字
     * @param   {const std::string&} logPath：待分析的日志文件路径
     * @return  {int} 当前日志新增的非法访问尝试次数
     */
    int analyze_auth_log(const std::string &logPath);
};
