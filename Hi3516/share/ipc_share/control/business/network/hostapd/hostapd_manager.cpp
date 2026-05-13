#ifdef CAP_NETWORK_WIFI
#include "hostapd_manager.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include "dlog.h"
#include <cstring>
HostapdManager::HostapdManager() 
    : last_query_time_(std::chrono::steady_clock::now()) 
{
    // 构造函数初始化列表，初始化时间点
}

HostapdManager::~HostapdManager() {
    if (is_running_) {
        Deinit();
    }
}

bool HostapdManager::ExecuteCommand(const std::string& cmd) const {
    // 生产环境建议重定向输出以避免污染控制台
    std::cout << "[CMD] " << cmd << std::endl;
    int ret = std::system(cmd.c_str());
    return (ret == 0);
}

void HostapdManager::StopServices() {
    std::cout << "Stopping services..." << std::endl;
    // -q 参数让 killall 在未找到进程时不报错
    ExecuteCommand("killall -q hostapd");
    ExecuteCommand("killall -q udhcpd");
    usleep(500000); // 等待 0.5秒确保进程退出
}

/**
 * @brief 初始化：增加了防止重复初始化的检查
 */
InitResult HostapdManager::Init(const std::string& uplink_interface) {
     // [防止重复初始化检查]
     if (is_running_) {
         std::cerr << "Warning: Hotspot is already running. Init ignored." << std::endl;
         return InitResult::SUCCESS;
     }
 
     this->uplink_iface_ = uplink_interface;
 
     // 1. 基础校验
     if (ap_config_.ssid.empty()) {
         std::cerr << "Error: SSID is not set." << std::endl;
         return InitResult::ERR_SSID_EMPTY;
     }
     if (dhcp_config_.interface.empty()) {
         std::cerr << "Error: DHCP Interface is not set." << std::endl;
         return InitResult::ERR_DHCP_IFACE_EMPTY;
     }
 
     std::cout << "Initializing Hotspot System...." << std::endl;
 
     // ============================================================
     // 设置环境变量，强制指定 iptables 插件路径
     // ============================================================
     setenv("IPTABLES_LIB_DIR", "/lib/xtables", 1); 
     setenv("XTABLES_LIBDIR", "/lib/xtables", 1);
     // 同时确保核心动态库能被加载
     setenv("LD_LIBRARY_PATH", "/lib/xtables:/lib:/usr/lib", 1);
 
     // 2. 生成并保存配置文件
     try {
         std::ofstream hostapd_conf("/tmp/hostapd.conf");
         if (!hostapd_conf) throw std::runtime_error("Cannot create hostapd.conf");
         hostapd_conf << GenerateHostapdConfigStr();
         hostapd_conf.close();
 
         std::ofstream dhcp_conf("/tmp/udhcpd.conf");
         if (!dhcp_conf) throw std::runtime_error("Cannot create udhcpd.conf");
         dhcp_conf << GenerateDHCPConfigStr();
         dhcp_conf.close();
         ExecuteCommand("mkdir -p /var/lib/misc");
 
     } catch (const std::exception& e) {
         std::cerr << "Config File Error: " << e.what() << std::endl;
         return InitResult::ERR_CONFIG_FILE;
     }
 
     // 3. 配置网络接口 IP
     std::string ip_cmd = "ifconfig " + dhcp_config_.interface + " " + 
                         dhcp_config_.gateway + " netmask " + 
                         dhcp_config_.subnet_mask + " up";
     ExecuteCommand(ip_cmd);
 
     // 4. 开启内核 IP 转发
     ExecuteCommand("echo 1 > /proc/sys/net/ipv4/ip_forward");
 
     // 5. 配置 iptables
     // 注意：由于上面设置了环境变量，这里应该能自动找到 MASQUERADE 模块
     ExecuteCommand("iptables -F");
     ExecuteCommand("iptables -t nat -F");
     ExecuteCommand("iptables -X");
     ExecuteCommand("iptables -t nat -X");
     
     std::string subnet = dhcp_config_.gateway.substr(0, dhcp_config_.gateway.rfind('.') + 1) + "0/24";
     
     // NAT 转发规则
     ExecuteCommand("iptables -t nat -A POSTROUTING -s " + subnet + " -o " + uplink_iface_ + " -j MASQUERADE");
     
     // 转发规则：允许内网到外网
     ExecuteCommand("iptables -A FORWARD -i " + dhcp_config_.interface + " -o " + uplink_iface_ + " -j ACCEPT");
     
     // 转发规则：允许外网回包 (已移除 -m conntrack，直接 ACCEPT 以兼容缺失模块的系统)
     ExecuteCommand("iptables -A FORWARD -i " + uplink_iface_ + " -o " + dhcp_config_.interface + " -j ACCEPT");
 
     // 6. 启动守护进程
     std::string hostapd_cmd = "hostapd -B /tmp/hostapd.conf"; 
     std::string udhcpd_cmd = "udhcpd -S /tmp/udhcpd.conf &"; 
 
     ExecuteCommand(hostapd_cmd);
     ExecuteCommand(udhcpd_cmd);
 

     usleep(200000); // 等待 200ms 让进程尝试启动

     std::string check_cmd = "pgrep -f hostapd";
     FILE* pipe = popen(check_cmd.c_str(), "r");
     bool hostapd_running = false;
     if (pipe) {
         char buffer[16];
         if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
             hostapd_running = true;
         }
         pclose(pipe);
     }
 
     if (!hostapd_running) {
         std::cerr << "Error: hostapd failed to start (check password/encryption settings)." << std::endl;
         StopServices();
         return InitResult::ERR_STARTUP_FAILED; // 返回启动失败
     }

     // [更新状态]
     is_running_ = true;
     std::cout << "Hotspot Initialized Successfully!" << std::endl;
     return InitResult::SUCCESS;
 }

/**
 * @brief 反初始化：增加了状态检查，防止对未运行的服务执行关闭操作
 */
void HostapdManager::Deinit() {
    // [防止重复关闭/无效关闭检查]
    if (!is_running_) {
        std::cout << "Info: Hotspot is not running. Deinit ignored." << std::endl;
        return;
    }

    std::cout << "Deinitializing..." << std::endl;
    StopServices();
    
    // [更新状态]
    is_running_ = false;
}

bool HostapdManager::SetNetworkConfig(const APConfig& config) {
    if (config.ssid.empty() || config.ssid.length() > 32) {
        std::cerr << "SSID Error: Must be 1-32 characters." << std::endl;
        return false;
    }
    if (config.password != config.confirm_password) {
        std::cerr << "Password Error: Passwords do not match." << std::endl;
        return false;
    }
    if (config.password.length() < 8) {
        std::cerr << "Warning: Password length < 8 is weak for WPA2." << std::endl;
    }

    ap_config_ = config;
    return true;
}

void HostapdManager::SetDHCPConfig(const DHCPConfig& dhcp_cfg) {
    dhcp_config_ = dhcp_cfg;
}

/**
 * @brief 获取连接设备：实现了防抖动逻辑
 */
std::vector<ClientInfo> HostapdManager::GetConnectedDevices() {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_query_time_);

    // [防抖动检查]
    // 如果距离上次查询时间小于阈值，直接返回缓存结果
    if (duration.count() < DEBOUNCE_INTERVAL_MS) {
        // std::cout << "[Debounce] Skipping query, returning cached result." << std::endl;
        return last_known_clients_;
    }

    // 如果超过阈值，执行实际查询
    last_query_time_ = now;
    last_known_clients_ = QueryDevicesInternal();
    
    return last_known_clients_;
}

/**
 * @brief 实际执行 Shell 命令获取设备列表的函数
 */
// std::vector<ClientInfo> HostapdManager::QueryDevicesInternal() {
//     std::vector<ClientInfo> clients;
    
//     // 1. 获取 ARP 表 (MAC -> IP)
//     std::map<std::string, std::string> mac_to_ip;
//     std::string cmd_arp = "arp -n | grep " + dhcp_config_.interface;
//     dlog_debug("[ARP] 执行命令: %s", cmd_arp.c_str());
    
//     FILE* arp_pipe = popen(cmd_arp.c_str(), "r");
//     if (arp_pipe) {
//         char line[128];
//         int arp_count = 0;
//         while (fgets(line, sizeof(line), arp_pipe)) {
//             std::string l(line);
//             size_t pos_ip = l.find(" ");
//             size_t pos_mac = l.find(" ", pos_ip + 1);
//             if (pos_ip != std::string::npos && pos_mac != std::string::npos) {
//                 std::string ip = l.substr(0, pos_ip);
//                 std::string mac = l.substr(pos_ip + 1, pos_mac - pos_ip - 1);
//                 dlog_debug("[ARP] 解析到 -> IP: %s, MAC: %s", ip.c_str(), mac.c_str());
//                 mac_to_ip[mac] = ip;
//                 arp_count++;
//             }
//         }
//         pclose(arp_pipe);
//         dlog_debug("[ARP] 总共解析到 %d 条记录", arp_count);
//     }

//     // 2. 获取 hostapd 客户端信息
//     std::string cli_cmd = "hostapd_cli -i " + dhcp_config_.interface + " all_sta";
//     dlog_debug("[STA] 执行命令: %s", cli_cmd.c_str());
//     FILE* sta_pipe = popen(cli_cmd.c_str(), "r");
    
//     if (sta_pipe) {
//         char line[256];
//         int count = 0;
//         std::string current_mac = "";
//         std::string current_time = "0";

//         while (fgets(line, sizeof(line), sta_pipe)) {
//             std::string l(line);
//             if (!l.empty() && l[l.length()-1] == '\n') l.erase(l.length()-1);

//             // 检测到 MAC 地址行 (简单的启发式判断)
//             if (l.length() == 17 && l.find(":") != std::string::npos) {
//                 dlog_debug("[STA] 发现客户端 MAC: %s", l.c_str());
//                 if (!current_mac.empty()) {
//                     ClientInfo info;
//                     info.index = ++count;
//                     info.mac = current_mac;
//                     info.ip = mac_to_ip.count(current_mac) ? mac_to_ip[current_mac] : "Unknown";
//                     info.conn_time = current_time;
//                     clients.push_back(info);
//                 }
//                 current_mac = l;
//                 current_time = "0";
//             } 
//             else if (l.find("connected_time") != std::string::npos) {
//                 size_t pos = l.find("=");
//                 if (pos != std::string::npos) {
//                     current_time = l.substr(pos + 1);
//                 }
//             }
//         }
        
//         // 处理最后一个
//         if (!current_mac.empty()) {
//             ClientInfo info;
//             info.index = ++count;
//             info.mac = current_mac;
//             info.ip = mac_to_ip.count(current_mac) ? mac_to_ip[current_mac] : "Unknown";
//             info.conn_time = current_time;
//             clients.push_back(info);
//         }

//         pclose(sta_pipe);
//     }

//     return clients;
// }

std::vector<ClientInfo> HostapdManager::QueryDevicesInternal() {
    std::vector<ClientInfo> clients;
    std::map<std::string, std::string> mac_to_ip;

    // 1. 获取 ARP 表
    std::string cmd_arp = "arp -n | grep " + dhcp_config_.interface;
    dlog_debug("[ARP] 执行命令: %s", cmd_arp.c_str());

    FILE* arp_pipe = popen(cmd_arp.c_str(), "r");
    if (arp_pipe) {
        char line[128];
        int arp_count = 0;
        
        while (fgets(line, sizeof(line), arp_pipe)) {
            std::string l(line);
            char ip_buf[32] = {0};
            char mac_buf[32] = {0};

            // 【核心修复】使用 sscanf 进行更健壮的解析
            // 尝试匹配格式："? (IP地址) at MAC地址 ..."
            // 注意：不同系统格式可能略有不同，这里适配最常见的 BusyBox 格式
            if (sscanf(l.c_str(), "%s (%[^)]) at %s", ip_buf, ip_buf, mac_buf) >= 2) {
                // 成功解析到 IP 和 MAC
                // 过滤掉 "<incomplete>" 这种无效条目
                if (strcmp(mac_buf, "<incomplete>") != 0 && strlen(mac_buf) == 17) {
                    dlog_debug("[ARP] 解析成功 -> IP: %s, MAC: %s", ip_buf, mac_buf);
                    mac_to_ip[mac_buf] = ip_buf;
                    arp_count++;
                }
            } 
            // 兼容另一种格式：IP地址 HWaddress MAC地址
            else if (sscanf(l.c_str(), "%s HWaddress %s", ip_buf, mac_buf) == 2) {
                 dlog_debug("[ARP] 解析成功(格式2) -> IP: %s, MAC: %s", ip_buf, mac_buf);
                 mac_to_ip[mac_buf] = ip_buf;
                 arp_count++;
            }
            else {
                // 调试用：打印无法解析的行
                dlog_debug("[ARP] 无法解析的行: %s", l.c_str());
            }
        }
        pclose(arp_pipe);
        dlog_debug("[ARP] 总共有效记录: %d", arp_count);
    }

    // 2. 获取 hostapd 客户端信息 (保持不变)
    std::string cli_cmd = "hostapd_cli -i " + dhcp_config_.interface + " all_sta";
    FILE* sta_pipe = popen(cli_cmd.c_str(), "r");
    
    if (sta_pipe) {
        char line[256];
        int count = 0;
        std::string current_mac = "";
        std::string current_time = "0";

        while (fgets(line, sizeof(line), sta_pipe)) {
            std::string l(line);
            if (!l.empty() && l[l.length()-1] == '\n') l.erase(l.length()-1);

            if (l.length() == 17 && l.find(":") != std::string::npos) {
                if (!current_mac.empty()) {
                    ClientInfo info;
                    info.index = ++count;
                    info.mac = current_mac;
                    // 查找 IP
                    info.ip = mac_to_ip.count(current_mac) ? mac_to_ip[current_mac] : "Unknown";
                    info.conn_time = current_time;
                    clients.push_back(info);
                    
                    dlog_debug("[RESULT] 客户端 -> MAC: %s, IP: %s", current_mac.c_str(), info.ip.c_str());
                }
                current_mac = l;
                current_time = "0";
            } 
            else if (l.find("connected_time") != std::string::npos) {
                size_t pos = l.find("=");
                if (pos != std::string::npos) {
                    current_time = l.substr(pos + 1);
                }
            }
        }
        
        // 处理最后一个
        if (!current_mac.empty()) {
            ClientInfo info;
            info.index = ++count;
            info.mac = current_mac;
            info.ip = mac_to_ip.count(current_mac) ? mac_to_ip[current_mac] : "Unknown";
            info.conn_time = current_time;
            clients.push_back(info);
            dlog_debug("[RESULT] 客户端(最后) -> MAC: %s, IP: %s", current_mac.c_str(), info.ip.c_str());
        }

        pclose(sta_pipe);
    }

    return clients;
}

std::string HostapdManager::GenerateHostapdConfigStr() const {
    std::ostringstream oss;
    oss << "# Auto-generated by C++ Manager\n";
    oss << "interface=" << dhcp_config_.interface << "\n";
    oss << "driver=nl80211\n";
    oss << "ssid=" << ap_config_.ssid << "\n";
    oss << "hw_mode=g\n";
    oss << "channel=6\n";
    oss << "auth_algs=1\n";
    oss << "wmm_enabled=0\n";
    
    oss << "wpa=2\n";
    oss << "wpa_key_mgmt=WPA-PSK\n";
    
    std::string cipher = "TKIP";
    if (ap_config_.encryption == EncryptionType::AES) cipher = "CCMP";
    else if (ap_config_.encryption == EncryptionType::TKIP_AES) cipher = "TKIP CCMP";
    
    oss << "wpa_pairwise=" << cipher << "\n";
    oss << "rsn_pairwise=" << cipher << "\n";
    oss << "wpa_passphrase=" << ap_config_.password << "\n";
    
    oss << "ctrl_interface=/var/run/hostapd\n";
    oss << "ctrl_interface_group=0\n";

    return oss.str();
}

std::string HostapdManager::GenerateDHCPConfigStr() const {
    std::ostringstream oss;
    oss << "interface " << dhcp_config_.interface << "\n";
    oss << "start\t\t" << dhcp_config_.ip_range_start << "\n";
    oss << "end\t\t" << dhcp_config_.ip_range_end << "\n";
    oss << "max_leases\t" << dhcp_config_.max_leases << "\n";
    oss << "lease_file\t" << dhcp_config_.lease_file << "\n";
    oss << "option subnet " << dhcp_config_.subnet_mask << "\n";
    oss << "option router " << dhcp_config_.gateway << "\n";
    oss << "option dns " << dhcp_config_.dns1 << ", " << dhcp_config_.dns2 << "\n";
    return oss.str();
}
#endif