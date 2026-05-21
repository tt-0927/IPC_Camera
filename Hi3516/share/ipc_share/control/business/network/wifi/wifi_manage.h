#if CAP_NETWORK_WIFI
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include "network_define.h"
#include "path_define.h"
#include "convert_interface.h"
#include <map>


// --- 全局配置结构体 ---
struct GlobalConfig {
    std::string config_file_path;
    std::string ctrl_interface;
    std::string interface_name;
    std::string driver;
    
    // 当前连接状态缓存
    std::string current_ssid;
    std::string current_psk;
    
    

    // 构造函数初始化默认值
    GlobalConfig();
    
};

// --- WiFi 信息结构体 ---
struct WifiInfo {
    std::string bssid;// MAC 地址
    int frequency;
    int signal_level;//信号强度(RSSI)
    std::string ssid;
    std::string flags;//安全能力标志
    bool is_current;//当前连接状态
    
    std::string security_type;//安全类型 
    std::string band;//频段名称
};


// 安全模式枚举
enum class WifiSecurityMode {
    WPA_PERSONAL,   // WPA-个人版
    OPEN,           // 开放网络
    WEP,            // WEP加密
    EAP_PEAP,       // WPA企业版 (PEAP)
    EAP_TLS         // WPA企业版 (TLS)
};

// WEP 密钥配置结构
struct WepKeyConfig {
    int index;          // 密钥索引 1-4
    std::string value;  // 密钥内容
};

// WiFi 连接配置结构体 (通用)
struct WifiConnectConfig {
    std::string ssid;           // SSID
    WifiSecurityMode mode;      // 安全模式
    
    // --- 通用配置 ---
    std::string password;       // 密码 (用于 WPA-Personal, WEP, EAP-PEAP)
    
    // --- WPA-Personal 特有 ---
    std::string pairwise;       // 加密类型 (TKIP/CCMP), 默认 CCMP
    
    // --- WEP 特有 ---
    int wep_key_len;            // 64 or 128
    bool wep_is_hex;            // true=16进制, false=ASCII
    std::string auth_alg;       // "OPEN" or "SHARED"
    std::vector<WepKeyConfig> wep_keys; // 密钥列表
    
    // --- EAP-PEAP 特有 ---
    std::string eap_identity;   // 用户名
    std::string eap_password;   // 密码
    std::string peap_version;   // "0" or "1"
    std::string phase2;         // 内部认证, 如 "auth=GTC"
    std::string anonymous_identity; // 匿名身份
    std::string ca_cert_path;   // CA证书路径
    
    // --- EAP-TLS 特有 ---
    std::string tls_identity;   // 身份
    std::string private_key_passwd; // 私钥密码
    std::string eapol_version;  // EAPOL版本
    std::string client_cert_path; // 用户证书路径
    std::string private_key_path; // 私钥路径

    std::string ctrl_interface;

    std::string interface_name;

    // 构造函数初始化默认值
    WifiConnectConfig() 
        : mode(WifiSecurityMode::OPEN), 
          pairwise("CCMP"),
          wep_key_len(128),
          wep_is_hex(false),
          auth_alg("OPEN"),
          peap_version("0"),
          eapol_version("2"),
          ctrl_interface("/var/run/wpa_supplicant"),
          interface_name("wlan0") {}
};


// --- WiFi 管理器类 ---
class CWifiManager : public CSingleton<CWifiManager> 
{
private:
    GlobalConfig config;
    
    std::atomic<bool> is_running;
    std::atomic<bool> is_connected;
    std::thread monitor_thread;
    int reconnect_attempts;
    static const int MAX_RECONNECT_ATTEMPTS = 3;
    std::chrono::steady_clock::time_point last_scan_time_; // 记录上次扫描时间
    std::vector<WifiInfo> last_scan_results_;              // 缓存上次扫描结果
    std::string m_configFile;                               /* 配置文件 */
    ::Network::WifiStaConncet_S m_lastConnectConfig; 
    bool m_hasLastConfig; // 标记是否有有效的配置
    const std::string PERSISTENT_CONFIG_PATH = "/etc/wifi_last_config.conf"; 
    std::mutex m_configMutex;

    // 私有辅助函数
    void execShell(const std::string& cmd);
    std::string sendCommand(const std::string& cmd);
    bool startDaemon();
    bool connectSocket();
    void monitorLoop();
    void handleDisconnect();
    void connectToWifiSimple(const std::string& ssid, const std::string& psk);
    // void set_wifi_config(Network::WifiStaInfo_S stWifiConfigInfo);
    // Network::WifiStaInfo_S load_wifi_config();
    
    
    void restoreConnection(); // 尝试从文件恢复并连接
    bool saveConfigToFile(const ::Network::WifiStaConncet_S& config); // 保存配置
    bool loadConfigFromFile(::Network::WifiStaConncet_S& config);     // 读取配置

    // 异步执行，防止 init 阻塞 ---
    void asyncRestoreConnection(); 
    // 线程函数
    void restoreConnectionThread();
    // 1. 声明新函数：强制锁定当前 IP 并持久化
    void lockCurrentIp();

public:
    CWifiManager();
    ~CWifiManager();

    // 公开接口 (外部可调用的功能)
    
    // 1. 初始化：启动进程并开始监听
    int init();

    void deinit(); 

    // 2. 扫描 WiFi
    std::vector<WifiInfo> scanWifi();

    // 3. 连接指定 WiFi (带同步结果检查)
    // bool connectToWifi(const std::string& ssid, const std::string& psk);
    ::Network::WifiConnectResult connectToWifi(::Network::WifiStaConncet_S& config);

    bool disconnectWifi();// 断开当前 WiFi 连接

    // 4. 开启增强模式
    bool setWifiEnhancedMode();
    
    // // 5. 获取当前配置（可选）
    // GlobalConfig getConfig() const { return config; }

    
    Network::WifiStaInfo_S load_wifi_config();
};

void set_wifi_config(Network::WifiStaInfo_S stWifiConfigInfo);

#endif // WIFI_MANAGER_H
#endif
