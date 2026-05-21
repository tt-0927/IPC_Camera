#if CAP_NETWORK_WIFI
#ifndef HOSTAPD_MANAGER_H
#define HOSTAPD_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <chrono> // 用于防抖动的时间计算
#include "network_define.h"
#include "Singleton.h"
// --- 配置结构体定义 ---

enum class EncryptionType {
    TKIP,
    AES,
    TKIP_AES
};

struct APConfig {
    std::string ssid;           // 网络名称
    std::string password;       // 密码
    std::string confirm_password;// 确认密码
    EncryptionType encryption = EncryptionType::TKIP;
};

struct DHCPConfig {
    std::string interface = "wlan0";
    std::string ip_range_start = "192.168.4.10";
    std::string ip_range_end = "192.168.4.100";
    std::string subnet_mask = "255.255.255.0";
    std::string gateway = "192.168.4.1";
    std::string dns1 = "8.8.8.8";
    std::string dns2 = "114.114.114.114";
    int max_leases = 90;
    std::string lease_file = "/var/lib/misc/udhcpd.leases";
};

struct ClientInfo {
    int index;
    std::string ip;
    std::string mac;
    std::string conn_time;
};

enum class InitResult {
    SUCCESS = 0,            // 成功
    ERR_SSID_EMPTY,         // SSID 为空
    ERR_DHCP_IFACE_EMPTY,   // DHCP 接口未设置
    ERR_CONFIG_FILE,        // 配置文件生成/写入失败
    ERR_STARTUP_FAILED,     // 进程启动失败 (如密码错误导致秒退)
    ERR_UNKNOWN             // 未知错误
};
// --- 核心管理类 ---

class HostapdManager: public CSingleton<HostapdManager>  {
public:
    HostapdManager();
    ~HostapdManager();

    // 生命周期管理
    InitResult  Init(const std::string& uplink_interface);
    void Deinit();

    // 配置管理
    bool SetNetworkConfig(const APConfig& config);
    void SetDHCPConfig(const DHCPConfig& dhcp_cfg);

    // 状态查询 (带防抖动)
    std::vector<ClientInfo> GetConnectedDevices();

private:
    // 配置数据
    APConfig ap_config_;
    DHCPConfig dhcp_config_;
    std::string uplink_iface_;

    // 状态控制变量
    bool is_running_ = false;

    // 防抖动相关变量
    std::vector<ClientInfo> last_known_clients_; // 缓存上一次的结果
    std::chrono::steady_clock::time_point last_query_time_; // 上次查询时间点
    const int DEBOUNCE_INTERVAL_MS = 500; // 防抖动阈值：500毫秒

    // 内部工具函数
    bool ExecuteCommand(const std::string& cmd) const;
    std::string GenerateHostapdConfigStr() const;
    std::string GenerateDHCPConfigStr() const;
    void StopServices();
    
    // 实际执行查询的私有函数
    std::vector<ClientInfo> QueryDevicesInternal();
};

#endif // HOSTAPD_MANAGER_H
#endif
