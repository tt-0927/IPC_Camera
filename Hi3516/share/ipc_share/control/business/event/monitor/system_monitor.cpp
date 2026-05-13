/**
 * @FilePath     : system_monitor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-10-31 9:38:25
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-16 15:32:20
 * @Description  : 事件异常检测使用的系统状态采集实现
 */

#include "system_monitor.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <mntent.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <syslog.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "dlog.h"

namespace
{
/* 默认检测的以太网carrier节点，节点值为1表示链路连接。 */
constexpr const char *SYSFS_NET_CARRIER = "/sys/class/net/eth0/carrier";

/**
 * @brief   : 获取指定网卡的本机IPv4地址
 * @param   {const std::string&} interface：网卡名称
 * @return  {std::string} 成功返回IPv4地址字符串，失败返回空字符串
 */
std::string get_local_ip(const std::string &interface)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
    {
        return "";
    }

    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_addr.sa_family = AF_INET;
    std::strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1);

    std::string ip_addr;
    if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
    {
        ip_addr = inet_ntoa(reinterpret_cast<struct sockaddr_in *>(&ifr.ifr_addr)->sin_addr);
    }

    close(fd);
    return ip_addr;
}
}

CSystemMonitor::CSystemMonitor()
{
}

CSystemMonitor::SystemStatus CSystemMonitor::get_current_status()
{
    SystemStatus status;

    /* SD卡挂载、读写状态和空间阈值统一折算为磁盘异常标志。 */
    status.diskErrorFlag = check_sd_error("/mnt");
    if (status.diskErrorFlag == false)
    {
        status.diskErrorFlag = check_disk_usage("/mnt", status.diskUsage);
    }

    /* 事件异常检测只需要链路状态，避免主动ping外网带来的阻塞和误报。 */
    status.networkConnected = check_link_by_carrier(SYSFS_NET_CARRIER);

    /* IP冲突通过arping探测本机地址是否收到其他设备应答。 */
    status.ipConflictFlag = detect_ip_conflict();

    /* 非法访问统计按日志增量读取，避免重复累计历史记录。 */
    status.unauthorizedAccessAttempts = detect_unauthorized_access();

    return status;
}

bool CSystemMonitor::check_sd_error(const std::string &mount_path)
{
    bool is_mounted = false;
    FILE *fp = setmntent("/proc/mounts", "r");
    if (fp == nullptr)
    {
        return true;
    }

    /* 通过/proc/mounts确认目标路径是否存在有效挂载记录。 */
    struct mntent *mnt = nullptr;
    while ((mnt = getmntent(fp)) != nullptr)
    {
        if (std::string(mnt->mnt_dir) == mount_path)
        {
            is_mounted = true;
            break;
        }
    }
    endmntent(fp);

    if (!is_mounted)
    {
        return true;
    }

    /* 只读挂载通常意味着SD卡异常或文件系统自保护。 */
    struct statvfs val;
    if (statvfs(mount_path.c_str(), &val) != 0)
    {
        return true;
    }

    if (val.f_flag & ST_RDONLY)
    {
        return true;
    }

    return false;
}

bool CSystemMonitor::check_disk_usage(const std::string &path, double &usage)
{
    struct statvfs stat;
    if (statvfs(path.c_str(), &stat) != 0)
    {
        return false;
    }

    if (stat.f_blocks > 0)
    {
        usage = 100.0 - (static_cast<double>(stat.f_bavail) / static_cast<double>(stat.f_blocks)) * 100.0;
        if (usage >= 95.00)
        {
            return true;
        }
    }

    return false;
}

bool CSystemMonitor::check_network_connection()
{
    struct ifaddrs *ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1)
    {
        return false;
    }

    bool connected = false;
    for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr)
        {
            continue;
        }

        /* 非回环IPv4接口存在时，再通过公共DNS探测外部连通性。 */
        if (ifa->ifa_addr->sa_family == AF_INET && std::strncmp(ifa->ifa_name, "lo", 2) != 0)
        {
            connected = true;
            break;
        }
    }
    freeifaddrs(ifaddr);

    if (connected)
    {
        int result = system("ping -c 1 114.114.114.114 > /dev/null 2>&1");
        if (result != 0)
        {
            result = system("ping -c 1 8.8.8.8 > /dev/null 2>&1");
            connected = (result == 0);
        }
    }

    return connected;
}

int CSystemMonitor::check_link_by_carrier(const std::string &strCarrierPath)
{
    std::ifstream fs(strCarrierPath);
    if (!fs.is_open())
    {
        dlog_error("open %s failed: %s", strCarrierPath.c_str(), std::strerror(errno));
        return 0;
    }

    int isConnected = 0;
    fs >> isConnected;

    if (!fs.good())
    {
        dlog_error("read %s failed", strCarrierPath.c_str());
        return 0;
    }

    return isConnected;
}

bool CSystemMonitor::detect_ip_conflict(const std::string &interface)
{
    std::string local_ip = get_local_ip(interface);
    if (local_ip.empty() || local_ip == "0.0.0.0" || local_ip == "127.0.0.1")
    {
        return false;
    }

    /* arping的DAD模式收到单播或接收计数时，说明同网段存在重复IP。 */
    std::stringstream cmd;
    cmd << "arping -D -f -c 2 -I " << interface << " " << local_ip << " 2>&1";

    FILE *pipe = popen(cmd.str().c_str(), "r");
    if (pipe == nullptr)
    {
        return false;
    }

    char buffer[256];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        output += buffer;
    }
    pclose(pipe);

    if (output.find("Unicast reply") != std::string::npos || output.find("Received 1") != std::string::npos ||
        output.find("Received 2") != std::string::npos)
    {
        return true;
    }

    return false;
}

int CSystemMonitor::detect_unauthorized_access()
{
    const std::vector<std::string> logPaths = {
        "/opt/course/log/operation/operation.log",
        "/opt/course/log/stream/stream.log",
        "/opt/course/log/record/record.log",
    };

    int totalAttempts = 0;
    for (const auto &path : logPaths)
    {
        struct stat buffer;
        if (stat(path.c_str(), &buffer) == 0)
        {
            totalAttempts += analyze_auth_log(path);
        }
    }

    /* 保留syslog审计痕迹，便于确认非法访问检测任务已被触发。 */
    setlogmask(LOG_UPTO(LOG_NOTICE));
    openlog("CSystemMonitor", LOG_CONS, LOG_AUTH);
    syslog(LOG_NOTICE, "Unauthorized access check performed");
    closelog();

    return totalAttempts;
}

int CSystemMonitor::analyze_auth_log(const std::string &logPath)
{
    int attempts = 0;

    /* 记录每个日志文件上次读取位置，日志轮转后自动回退到文件起始位置。 */
    static std::map<std::string, std::streampos> filePosMap;

    std::ifstream logFile(logPath);
    if (!logFile.is_open())
    {
        return 0;
    }

    logFile.seekg(0, std::ios::end);
    std::streampos currentFileSize = logFile.tellg();

    if (filePosMap.find(logPath) == filePosMap.end())
    {
        filePosMap[logPath] = currentFileSize;
        logFile.close();
        return 0;
    }

    std::streampos &lastPos = filePosMap[logPath];
    if (currentFileSize < lastPos)
    {
        lastPos = 0;
    }
    else if (currentFileSize == lastPos)
    {
        logFile.close();
        return 0;
    }

    logFile.seekg(lastPos);

    /* 关键字同时覆盖系统英文日志和业务中文日志。 */
    const std::vector<std::string> patterns = {
        "authentication failure", "failed password", "invalid user", "break-in attempt",
        "unauthorized access",    "用户不存在",      "密码错误",
    };

    std::string line;
    while (std::getline(logFile, line))
    {
        for (const auto &pattern : patterns)
        {
            if (line.find(pattern) != std::string::npos)
            {
                attempts++;
                break;
            }
        }
    }

    if (logFile.eof())
    {
        logFile.clear();
    }

    lastPos = logFile.tellg();
    if (lastPos == std::streampos(-1))
    {
        lastPos = currentFileSize;
    }

    return attempts;
}
