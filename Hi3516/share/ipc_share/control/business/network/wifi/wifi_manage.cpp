// 包含头文件
#if CAP_NETWORK_WIFI
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <chrono>
#include <algorithm>
#include "wifi_manage.h" 
#include "rtsp_server.h"
// --- 全局配置实现 ---
GlobalConfig::GlobalConfig() 
    : config_file_path("/etc/wpa_supplicant.conf"),
      ctrl_interface("/var/run/wpa_supplicant"),
      interface_name("wlan0"),
      driver("nl80211"),
      current_ssid(""),
      current_psk("")
      {}

// --- CWifiManager 类实现 ---

CWifiManager::CWifiManager() :  is_running(false),is_connected(false), reconnect_attempts(0), m_configFile(WIFI_CONFIG_FILE) {
    last_scan_time_ = std::chrono::steady_clock::now() - std::chrono::seconds(10);

    // startDaemon();
    // if (connectSocket()) {
    //     is_running = true; 
    //     monitor_thread = std::thread(&CWifiManager::monitorLoop, this);
    //     std::cout << "[初始化] WiFi 管理器已启动 (自动初始化)" << std::endl;
    // } else {
    //     std::cerr << "[错误] 无法连接 wpa_supplicant (自动初始化失败)" << std::endl;
    // }
}

CWifiManager::~CWifiManager() {
    is_running = false;
    if (monitor_thread.joinable()) monitor_thread.join();
}

void CWifiManager::execShell(const std::string& cmd) {
    system(cmd.c_str());
}

std::string CWifiManager::sendCommand(const std::string& cmd) {

    std::string cli_cmd = "/bin/wpa_cli -i " + config.interface_name + " " + cmd;

    char buffer[4096];
    std::string result;

    FILE* pipe = popen(cli_cmd.c_str(), "r");
    if (!pipe) {
        return "FAIL: popen error";
    }

    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    pclose(pipe);

    // 去掉末尾换行符
    if (!result.empty() && result.back() == '\n') result.pop_back();

    dlog_error("[TX] 执行 wpa_cli : %s", cli_cmd.c_str());
    dlog_error("[RX] 响应: %s", result.c_str());

    return result;

}

bool CWifiManager::startDaemon() {
    execShell("killall wpa_supplicant 2>/dev/null");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::string ctrl_dir = config.ctrl_interface;
    execShell("mkdir -p " + ctrl_dir + " && chmod 777 " + ctrl_dir);
   
    std::ofstream file(config.config_file_path);
    if (file.is_open()) {
        file << "ctrl_interface=" << config.ctrl_interface << "\n";
        file << "update_config=1\n"; 
        file << "ap_scan=1\n";
        file.close();
    }else {
        std::cerr << "[错误] 无法创建配置文件:" << config.config_file_path << std::endl;
        return false;
    }

    std::string cmd = "wpa_supplicant -D" + config.driver + " -i" + config.interface_name + " -c" + config.config_file_path + " -B";

    execShell(cmd);
    
    const int max_wait = 5; // 最多 5 秒
    bool socket_ready = false;
    for (int i = 0; i < max_wait * 2; i++) { // 每 0.5 秒检查一次
        if (access((config.ctrl_interface + "/" + config.interface_name).c_str(), F_OK) == 0) {
            socket_ready = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (!socket_ready) {
        std::cerr << "[错误] wpa_supplicant 控制 socket 未创建" << std::endl;
        return false;
    }

    std::cout << "[守护] wpa_supplicant 已启动，控制接口就绪" << std::endl;

    // std::this_thread::sleep_for(std::chrono::seconds(1)); 
    return true;
}

bool CWifiManager::connectSocket() {
    return true;
}

void CWifiManager::monitorLoop() {
    while (is_running) {
        if (!is_connected) {
            std::this_thread::sleep_for(std::chrono::seconds(60));
            continue; 
        }

        std::string status = sendCommand("STATUS");
        if (status.find("wpa_state=DISCONNECTED") != std::string::npos ||
            status.find("wpa_state=SCANNING") != std::string::npos) {
            std::cout << "[后台监听] Wi-Fi 意外断开" << std::endl;
            handleDisconnect();

            if (is_connected) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                lockCurrentIp(); // 重连后立即重新锁定 IP
            }
        }
        else if (status.find("wpa_state=COMPLETED") != std::string::npos) {
            // 每 3 分钟检查一次 IP 状态，防止静默变更
            static auto last_ip_check = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            if (now - last_ip_check > std::chrono::minutes(3)) {
                lockCurrentIp(); // 强制刷新并锁定
                last_ip_check = now;
            }
        }


        int eth0_link_up = system("cat /sys/class/net/eth0/carrier 2>/dev/null | grep 1 > /dev/null 2>&1");
        if (eth0_link_up == 0) {
            // 情况 A: 网线插好了
            // 只要网线插着，我们就强制走有线
            // 注意：这里不需要 Ping 网关，物理连接在我们就信任它
            Network::Info_S stNetInfo;
            CNetworkManage::instance()->get_system_networkInfo(stNetInfo);
            if (stNetInfo.stIp.ipv4Ip.length() >= 4 && stNetInfo.stIp.ipv4Ip.substr(0, 4) == "192.") {
                system("ip route replace default via 192.168.1.254 dev eth0");
            }
            else {
                system("ip route replace default via 172.16.25.254 dev eth0");
            }
            
            is_rebootrtsp_wlan0 = true;
            if(is_rebootrtsp_eth0)
            {
                CRtspServer::instance()->reboot();
                is_rebootrtsp_eth0 = false;
            }
        } else {
            // 情况 B: 网线没插 (或者 carrier 检测失败)
            // 只有当 WiFi 连接时，才切换路由
            if (is_connected) {
                std::cout << "[监控] 检测到 eth0 网线断开，切换至 WiFi..." << std::endl;
                system("ip route replace default dev wlan0");
                if(is_rebootrtsp_wlan0)
                {
                    CRtspServer::instance()->reboot();
                    is_rebootrtsp_wlan0 = false;

                    is_rebootrtsp_eth0 =true;
                }
                
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

void CWifiManager::handleDisconnect() {
    ::Network::WifiStaConncet_S localConfig; 
    {
        std::lock_guard<std::mutex> lock(m_configMutex);

        // 如果内存中没有配置，尝试从文件加载一次
        if (!m_hasLastConfig) {
            if (loadConfigFromFile(m_lastConnectConfig)) {
                m_hasLastConfig = true;
                std::cout << "[重连] 内存无配置，从文件恢复了配置。" << std::endl;
            } else {
                return; // 既没有内存配置也没有文件配置，无法重连
            }
        }

        localConfig = m_lastConnectConfig;

    }
    

    if (reconnect_attempts < MAX_RECONNECT_ATTEMPTS) {
        reconnect_attempts++;
        std::cout << "[重连] 正在尝试重连 (" << reconnect_attempts << ")..." << std::endl;
        
        // 使用保存的完整配置重连
        connectToWifi(localConfig);
    } else {
        std::cout << "[重连] 达到最大重连次数。" << std::endl;
    }
}


// 保存配置到文件
bool CWifiManager::saveConfigToFile(const ::Network::WifiStaConncet_S& config) {
    std::ofstream file(PERSISTENT_CONFIG_PATH);
    if (!file.is_open()) {
        std::cerr << "[错误] 无法创建持久化配置文件: " << PERSISTENT_CONFIG_PATH << std::endl;
        return false;
    }

    std::cout << "----------------------------------------" << std::endl;
    std::cout << "[持久化调试] 开始保存配置到文件: " << PERSISTENT_CONFIG_PATH << std::endl;
    std::cout << "[持久化调试] SSID: " << config.ssid << std::endl;
    std::cout << "[持久化调试] IP Address: " << config.ip_address << std::endl;
    std::cout << "[持久化调试] Security Mode: " << static_cast<int>(config.mode) << std::endl;
    
    std::cout << "[持久化调试] Password: " << config.password << std::endl; 

    // 1. 写入基础信息
    file << "ssid=" << config.ssid << "\n";
    file << "ip_address=" << config.ip_address << "\n"; // <--- 加上这一行
    file << "mode=" << static_cast<int>(config.mode) << "\n";

    // 2. 根据模式写入特定信息
    switch (config.mode) {
        case ::Network::WifiSecurityMode::WPA_PERSONAL:
            file << "password=" << config.password << "\n";
            file << "pairwise=" << config.pairwise << "\n";
            break;

        case ::Network::WifiSecurityMode::WEP:
            file << "wep_auth=" << config.auth_alg << "\n";
            file << "wep_is_hex=" << (config.wep_is_hex ? 1 : 0) << "\n"; // 转成 0 或 1 存储
            // 保存 WEP 密钥列表
            file << "wep_keys_count=" << config.wep_keys.size() << "\n";
            for (size_t i = 0; i < config.wep_keys.size(); ++i) {
                file << "wep_key_idx_" << i << "=" << config.wep_keys[i].index << "\n";
                file << "wep_key_val_" << i << "=" << config.wep_keys[i].value << "\n";
            }
            break;

        case ::Network::WifiSecurityMode::EAP_PEAP:
            file << "eap_identity=" << config.eap_identity << "\n";
            file << "eap_password=" << config.eap_password << "\n";
            file << "peap_version=" << config.peap_version << "\n";
            file << "phase2=" << config.phase2 << "\n";
            file << "anonymous_identity=" << config.anonymous_identity << "\n";
            if (!config.ca_cert_path.empty()) {
                file << "ca_cert_path=" << config.ca_cert_path << "\n";
            }
            if (!config.peap_label.empty()) {
                file << "peap_label=" << config.peap_label << "\n";
            }
            
            break;

        case ::Network::WifiSecurityMode::EAP_TLS:
            file << "tls_identity=" << config.tls_identity << "\n";
            file << "private_key_passwd=" << config.private_key_passwd << "\n";
            file << "eapol_version=" << config.eapol_version << "\n";
            file << "ca_cert_path=" << config.ca_cert_path << "\n";
            file << "client_cert_path=" << config.client_cert_path << "\n";
            file << "private_key_path=" << config.private_key_path << "\n";
            break;

        case ::Network::WifiSecurityMode::EAP_TTLS: // <--- 找到或添加此 case
            // 必填项
            file << "eap_identity=" << config.eap_identity << "\n";
            file << "eap_password=" << config.eap_password << "\n";
            file << "eap_anonymous_identity=" << config.eap_anonymous_identity << "\n"; // 匿名身份
            file << "eap_ttls_phase2=" << config.eap_ttls_phase2 << "\n"; // 内部认证
            // 证书路径
            if (!config.ca_cert_path.empty()) {
                file << "ca_cert_path=" << config.ca_cert_path << "\n";
            }
            break;
            

        case ::Network::WifiSecurityMode::OPEN:
        default:
            // 开放网络不需要额外信息
            break;
    }

    file.close();
    std::cout << "[持久化] 配置已保存到: " << PERSISTENT_CONFIG_PATH << std::endl;
    return true;
}

// 从文件读取配置
bool CWifiManager::loadConfigFromFile(::Network::WifiStaConncet_S& config) {
    std::cout << "[调试] lockCurrentIp 尝试读取文件: " << PERSISTENT_CONFIG_PATH << std::endl;
    std::ifstream file(PERSISTENT_CONFIG_PATH);
    if (!file.is_open()) {
        std::cout << "[调试] lockCurrentIp 文件不存在: " << PERSISTENT_CONFIG_PATH << std::endl;
        return false; // 文件不存在，说明是首次运行或已被清除
    }

    // if (config.ssid.empty()) {
    //     return false;
    // }


    std::string line;
    // 用于临时存储解析出的键值对
    std::string key, value;

    while (std::getline(file, line)) {
        // 1. 简单的解析：找到第一个 '=' 分割键值
        size_t pos = line.find('=');
        if (pos == std::string::npos) continue;

        key = line.substr(0, pos);
        value = line.substr(pos + 1);

        // 去除末尾可能存在的回车符 (Windows 换行符兼容)
        if (!value.empty() && value.back() == '\r') {
            value.pop_back();
        }

        // 2. 匹配键名并赋值
        if (key == "ssid") {
            config.ssid = value;
            std::cout << "  [解析] SSID: " << config.ssid << std::endl;
        } else if (key == "ip_address") { 
            config.ip_address = value;
            std::cout << "  [解析] 静态IP: " << config.ip_address  << std::endl;
        }else if (key == "mode") {
            config.mode = static_cast<::Network::WifiSecurityMode>(std::stoi(value));
            std::cout << "  [解析] 模式ID: " << static_cast<int>(config.mode) << std::endl;
        } else if (key == "password") {
            config.password = value;
            std::cout << "  [解析] 密码: " << config.password << std::endl;
        } else if (key == "pairwise") {
            config.pairwise = value;
        } 
        // --- WEP 相关 ---
        else if (key == "wep_auth") {
            config.auth_alg = value;
        } else if (key == "wep_is_hex") {
            config.wep_is_hex = (std::stoi(value) == 1);
        } else if (key.find("wep_key_idx_") == 0) {
            // 动态处理 WEP 密钥索引
            int idx = std::stoi(key.substr(12)); // 获取 "wep_key_idx_" 后面的数字
            if (idx >= 0 && idx < 4) {
                // 确保向量有足够空间
                if (config.wep_keys.size() <= static_cast<size_t>(idx)) {
                    config.wep_keys.resize(idx + 1);
                }
                config.wep_keys[idx].index = std::stoi(value);
            }
        } else if (key.find("wep_key_val_") == 0) {
            // 动态处理 WEP 密钥值
            int idx = std::stoi(key.substr(12));
            if (idx >= 0 && idx < 4) {
                if (config.wep_keys.size() <= static_cast<size_t>(idx)) {
                    config.wep_keys.resize(idx + 1);
                }
                config.wep_keys[idx].value = value;
            }
        }
        // --- EAP-PEAP 相关 ---
        else if (key == "eap_identity") {
            config.eap_identity = value;
        } else if (key == "eap_password") {
            config.eap_password = value;
        } else if (key == "peap_version") {
            config.peap_version = value;
        } else if (key == "phase2") {
            config.phase2 = value;
        } else if (key == "anonymous_identity") {
            config.anonymous_identity = value;
        }
        else if (key == "peap_label") {
            config.peap_label = value;
        }
        // --- EAP-TLS 相关 ---
        else if (key == "tls_identity") {
            config.tls_identity = value;
        } else if (key == "private_key_passwd") {
            config.private_key_passwd = value;
        } else if (key == "eapol_version") {
            config.eapol_version = value;
        }
        // --- EAP-TTLS 相关 ---
        else if (key == "eap_anonymous_identity") { // 匿名身份
            config.eap_anonymous_identity = value;
        } 
        else if (key == "eap_ttls_phase2") { // 内部认证
            config.eap_ttls_phase2 = value;
        } 
        // --- 证书路径 (通用) ---
        else if (key == "ca_cert_path") {
            config.ca_cert_path = value;
        } else if (key == "client_cert_path") {
            config.client_cert_path = value;
        } else if (key == "private_key_path") {
            config.private_key_path = value;
        }
    }

    file.close();

    // 3. 验证有效性：如果 SSID 不为空，认为配置是有效的
    return !config.ssid.empty();
}

void CWifiManager::restoreConnection() {
    ::Network::WifiStaConncet_S tempConfig;
    if (loadConfigFromFile(tempConfig)) {
        std::cout << "[恢复] 检测到历史配置 SSID: " << tempConfig.ssid << "，正在尝试自动连接..." << std::endl;
        
        // 直接调用主连接函数
        // 注意：这里不需要再次保存文件，因为 connectToWifi 会处理
        connectToWifi(tempConfig);
    } else {
        std::cout << "[恢复] 未找到历史配置文件，等待用户指令。" << std::endl;
    }
}
// --- 异步恢复连接入口 ---
void CWifiManager::asyncRestoreConnection() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "[恢复线程] asyncRestoreConnection 启动" << std::endl;
    restoreConnectionThread();
}

void CWifiManager::restoreConnectionThread() {
    ::Network::WifiStaConncet_S tempConfig;
    std::cout << "[恢复线程] restoreConnectionThread 启动" << std::endl;
    if (loadConfigFromFile(tempConfig)) {
        std::cout << "[恢复] 检测到历史配置 SSID: " << tempConfig.ssid << "，正在尝试自动连接..." << std::endl;
        connectToWifi(tempConfig);
    } else {
        std::cout << "[恢复] 未找到有效历史配置，等待用户指令。" << std::endl;
    }
}

int CWifiManager::init() {
    if (is_running.load()) {
        std::cout << "[初始化] 检测到已初始化，忽略重复调用。" << std::endl;
        return 0;
    }

    std::cout << "[初始化] 启动 WiFi 守护进程..." << std::endl;
    if(!startDaemon())
    {
        return -1;
    }
    if (connectSocket()) {
        is_running = true;
        monitor_thread = std::thread(&CWifiManager::monitorLoop, this);
        std::cout << "[初始化] 守护进程连接成功。" << std::endl;

    Network::WifiStaInfo_S cfg = load_wifi_config();
    if(cfg.bEnableWifi)
    {
        std::cout << "[初始化] 重连wifi..." << std::endl;
        std::thread(&CWifiManager::asyncRestoreConnection, this).join();
    }
   
        
    } else {
        std::cerr << "[错误] 无法连接 wpa_supplicant" << std::endl;
    }
    return 0;
}

// --- 反初始化函数实现 ---
void CWifiManager::deinit() {
    // 1. 检查是否已经关闭
    if (!is_running.load()) {
        std::cout << "[反初始化] WiFi 管理器未运行，无需操作。" << std::endl;
        return;
    }

    std::cout << "[反初始化] 开始关闭 WiFi 管理器..." << std::endl;

    // 2. 停止监控线程
    is_running = false;
    if (monitor_thread.joinable()) {
        monitor_thread.join();
        std::cout << "[反初始化] 监控线程已停止。" << std::endl;
    }

    // 3. 断开 WiFi 连接 (发送 DISCONNECT 命令)
    // 注意：sendCommand 可能依赖 socket，需在 kill 进程前调用
    sendCommand("DISCONNECT");
    sendCommand("TERMINATE"); // 告诉 wpa_supplicant 优雅退出
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 等待命令处理

    // 4. 强制关闭 wpa_supplicant 进程
    execShell("killall wpa_supplicant");
    std::cout << "[反初始化] wpa_supplicant 进程已终止。" << std::endl;

    // 5. 清理状态
    is_connected = false;

    std::cout << "[反初始化] WiFi 管理器已完全关闭。" << std::endl;
}

std::vector<WifiInfo> CWifiManager::scanWifi() {
    // 1. 防抖检查
    auto now = std::chrono::steady_clock::now();
    // 设定最小扫描间隔为 5 秒
    auto scan_interval = std::chrono::seconds(5);

    if (now - last_scan_time_ < scan_interval) {
        std::cout << "[扫描] 距离上次扫描时间过短，返回缓存结果。" << std::endl;
        return last_scan_results_;
    }

    // 2. 执行实际扫描
    std::vector<WifiInfo> results;
    std::cout << "[扫描] 开始新一轮扫描..." << std::endl;
    
    // 发送扫描命令
    std::string scan_res = sendCommand("SCAN");
    if (scan_res.find("OK") != std::string::npos) {
         std::cerr << "[扫描] 扫描命令发送成功" << std::endl;
    }

    // 等待扫描完成 (通常 3-5 秒)
    std::this_thread::sleep_for(std::chrono::seconds(5)); 

    std::string res = sendCommand("SCAN_RESULTS");
    std::cout << "[调试] SCAN_RESULTS 原始返回长度: " << res.length() << std::endl;
    if(res.empty()) {
        std::cerr << "[错误] 返回结果为空！请检查 sendCommand 中的 Socket 连接状态。" << std::endl;
        return results; 
    }
    std::cout << "[调试] 原始数据预览: " << res.substr(0, 100) << "..." << std::endl;

    std::istringstream iss(res);
    std::string line;
     if (!std::getline(iss, line)) {
        std::cerr << "[错误] 无法读取表头行。" << std::endl;
        return results;
    }
    std::cout << "[调试] 表头内容: " << line << std::endl;

    while (std::getline(iss, line)) {
        std::istringstream lineStream(line);
        WifiInfo info;
        info.is_current = false;
        
        if (lineStream >> info.bssid >> info.frequency >> info.signal_level >> info.flags) {
            std::getline(lineStream, info.ssid);
            info.ssid.erase(0, info.ssid.find_first_not_of(" \t"));
            info.ssid.erase(info.ssid.find_last_not_of(" \t") + 1);
            
            // 检查是否当前连接
            // if (info.flags.find("[CURRENT]") != std::string::npos) {
            //     info.is_current = true;
            //     config.current_ssid = info.ssid;
            // }
            if (config.current_ssid == info.ssid) {
                std::string status = sendCommand("STATUS");
                if (status.find("ssid=" + info.ssid) != std::string::npos)
                {
                    std::cout << "已经连接的wifi " << line << std::endl;
                    info.is_current = true;
                }
            }

            // 判断频段
            if (info.frequency > 3000) {
                info.band = "5GHz";
            } else {
                info.band = "2.4GHz";
            }

            // 判断加密方式
            if (info.flags.find("WPA3") != std::string::npos) {
                info.security_type = "WPA3";
            } else if (info.flags.find("WPA2") != std::string::npos) {
                info.security_type = "WPA2";
            } else if (info.flags.find("WPA") != std::string::npos) {
                info.security_type = "WPA";
            } else if (info.flags.find("WEP") != std::string::npos) {
                info.security_type = "WEP";
            } else if (info.flags.find("ESS") != std::string::npos) {
                info.security_type = "Open";
            } else {
                info.security_type = "Unknown";
            }

            if (!info.ssid.empty()) {
                results.push_back(info);
            }
        }
    }

    // 3. 更新缓存和时间戳
    // 只有当扫描结果不为空时，才更新缓存（防止扫描失败导致缓存被清空）
    if (!results.empty()) {
        last_scan_time_ = now;
        last_scan_results_ = results;
    }

    return results;
}
bool CWifiManager::disconnectWifi() {
    std::cout << "[断开] 正在断开当前 WiFi 连接..." << std::endl;

    // 我们需要先知道当前是哪个 network id
    // std::string status = sendCommand("STATUS");
    // int net_id = -1;

    // // 简单解析 status 获取 id (格式通常包含 id=xx)
    // size_t pos = status.find("id=");
    // if (pos != std::string::npos) {
    //     // 假设 id 是数字，且后面紧跟换行或空格
    //     std::string id_str = status.substr(pos + 3);
    //     try {
    //         net_id = std::stoi(id_str);
    //     } catch (...) {
    //         net_id = -1;
    //     }
    // }

    // if (net_id >= 0) {
    //     std::string res = sendCommand("DISABLE_NETWORK " + std::to_string(net_id));
    //     if (res.find("OK") != std::string::npos || res.find("FAIL") == std::string::npos) {
    //         std::cout << "[断开] 网络 ID " << net_id << " 已禁用" << std::endl;
            
            
    //         // 3. 更新内部状态
    //         config.current_ssid = "";
    //         config.current_psk = "";
    //         is_connected = false;
    //         return true;
    //     } else {
    //         std::cerr << "[断开] 禁用网络失败: " << res << std::endl;
    //         return false;
    //     }
    // } else {
        // 如果没有获取到 ID，尝试直接发送 DISCONNECT 命令
        // std::cout << "[断开] 未获取到网络ID，尝试通用断开命令..." << std::endl;
        std::cout << "[断开] 正在断开当前 WiFi 连接并清除配置..." << std::endl;

        std::string res = sendCommand("DISCONNECT");
        if (res.find("OK") != std::string::npos || res.empty()) {
            std::cout << "[断开] WiFi 断开命令执行成功" << std::endl;
        } else {
            std::cerr << "[断开] 警告: 断开命令响应异常，但仍将继续清理配置: " << res << std::endl;
        }
        if (std::remove(PERSISTENT_CONFIG_PATH.c_str()) == 0) {
            std::cout << "[清理] 已成功删除持久化配置文件: " << PERSISTENT_CONFIG_PATH << std::endl;
        } else {
            std::cerr << "[清理] 警告: 无法删除配置文件 (可能文件不存在或无权限): " << PERSISTENT_CONFIG_PATH << std::endl;
           
        }

        // 2. 清理路由
        system("route del default dev wlan0 2>/dev/null || true");
        system("ip route replace default via 172.16.25.254 dev eth0");

        {
            std::lock_guard<std::mutex> lock(m_configMutex);
            config.current_ssid = "";
            config.current_psk = "";
            is_connected = false;
            m_hasLastConfig = false; 
        }
    
        std::cout << "[断开] 清理流程完成." << std::endl;
        return true; 
    
    // }
}

// --- 辅助函数：生成 wpa_supplicant 配置文件内容 ---
// 这个函数负责把 C++ 结构体转换成 wpa_supplicant 能读懂的文本格式
std::string generateWpaConfContent(const ::Network::WifiStaConncet_S& config) {
    std::stringstream ss;
    
    // 1. 全局配置头
    ss << "ctrl_interface=" << config.ctrl_interface << "\n";
    ss << "update_config=1\n";
    ss << "ap_scan=1\n"; // 强制由客户端决定 AP
    ss << "network={\n";
    
    // 2. 通用 SSID 设置
    ss << "\tssid=\"" << config.ssid << "\"\n";

    // 3. 根据不同模式生成配置
    switch (config.mode) {
        case ::Network::WifiSecurityMode::OPEN:
            // --- 开放网络 ---
            ss << "\tkey_mgmt=NONE\n";
            break;

        case ::Network::WifiSecurityMode::WPA_PERSONAL:
            // --- WPA-个人版 ---
            ss << "\tkey_mgmt=WPA-PSK\n";
            ss << "\tproto=RSN\n"; // 默认使用 RSN (WPA2)
            ss << "\tpairwise=" << config.pairwise << "\n"; // TKIP 或 CCMP
            ss << "\tgroup=CCMP\n";
            ss << "\tpsk=\"" << config.password << "\"\n";
            break;

        case ::Network::WifiSecurityMode::WEP:
            // --- WEP 加密 ---
            ss << "\tkey_mgmt=NONE\n";
            ss << "\tauth_alg=" << config.auth_alg << "\n"; // OPEN 或 SHARED
            
            // 设置 WEP 密钥 (wep_key0 到 wep_key3)
            // 初始化所有密钥为空，防止残留
            ss << "\twep_key0=\"\"\n";
            ss << "\twep_key1=\"\"\n";
            ss << "\twep_key2=\"\"\n";
            ss << "\twep_key3=\"\"\n";

            if (!config.wep_keys.empty()) {
                // 设置默认传输密钥索引 (0-3)
                // 这里假设用户传入的 index 是 1-4，需要减 1
                ss << "\twep_tx_keyidx=" << (config.wep_keys[0].index - 1) << "\n";

                for (const auto& key : config.wep_keys) {
                    int idx = key.index - 1;
                    if (idx >= 0 && idx <= 3) {
                        if (config.wep_is_hex) {
                            // 16进制不需要引号
                            ss << "\twep_key" << idx << "=" << key.value << "\n";
                        } else {
                            // ASCII 需要引号
                            ss << "\twep_key" << idx << "=\"" << key.value << "\"\n";
                        }
                    }
                }
            }
            break;

        case ::Network::WifiSecurityMode::EAP_PEAP:
            // --- EAP-PEAP (企业版) ---
            ss << "\tkey_mgmt=WPA-EAP\n";
            ss << "\teap=PEAP\n";
            ss << "\tidentity=\"" << config.eap_identity << "\"\n";
            ss << "\tpassword=\"" << config.eap_password << "\"\n";
            ss << "\tpeapver=" << config.peap_version << "\n"; // 0 或 1
            
            if (!config.phase2.empty()) {
                ss << "\tphase2=\"" << config.phase2 << "\"\n"; // 如 "auth=GTC"
            }
            if (!config.anonymous_identity.empty()) {
                ss << "\tanonymous_identity=\"" << config.anonymous_identity << "\"\n";
            }
            if (!config.ca_cert_path.empty()) {
                ss << "\tca_cert=\"" << config.ca_cert_path << "\"\n";
            }
            if (config.peap_label == "old") {
                ss << "\tpeap_label=1\n"; // 告诉 wpa_supplicant 使用 "peap" 标签
            }else {
                ss << "\tpeap_label=0\n";
            }
            break;

        case ::Network::WifiSecurityMode::EAP_TLS:
            // --- EAP-TLS (企业版-证书) ---
            ss << "\tkey_mgmt=WPA-EAP\n";
            ss << "\teap=TLS\n";
            ss << "\tidentity=\"" << config.tls_identity << "\"\n";
            
            if (!config.private_key_passwd.empty()) {
                ss << "\tprivate_key_passwd=\"" << config.private_key_passwd << "\"\n";
            }
            // eapol_flags 通常是整数，这里假设传入的是字符串形式的数字
            ss << "\teapol_flags=" << config.eapol_version << "\n"; 

            if (!config.ca_cert_path.empty()) {
                ss << "\tca_cert=\"" << config.ca_cert_path << "\"\n";
            }
            if (!config.client_cert_path.empty()) {
                ss << "\tclient_cert=\"" << config.client_cert_path << "\"\n";
            }
            if (!config.private_key_path.empty()) {
                ss << "\tprivate_key=\"" << config.private_key_path << "\"\n";
            }
            break;
        case ::Network::WifiSecurityMode::EAP_TTLS: // <--- 新增分支
            ss << "\tkey_mgmt=WPA-EAP\n";
            ss << "\teap=TTLS\n"; // 核心：指定 EAP 方法为 TTLS
            
            // 1. 匿名身份 (必填)
            ss << "\tanonymous_identity=\"" << config.eap_anonymous_identity << "\"\n";
            
            // 2. 用户名和密码
            ss << "\tidentity=\"" << config.eap_identity << "\"\n"; // 用户名
            ss << "\tpassword=\"" << config.eap_password << "\"\n"; // 密码
            
            // 3. 内部认证 (Phase 2)
            // 注意：wpa_supplicant 中 PAP 和 MSCHAPV2 的写法
            if (config.eap_ttls_phase2 == "MSCHAPV2") {
                ss << "\tphase2=\"auth=MSCHAPV2\"\n";
            } else { // 默认 PAP
                ss << "\tphase2=\"auth=PAP\"\n";
            }
            
            // 4. CA 证书
            if (!config.ca_cert_path.empty()) {
                ss << "\tca_cert=\"" << config.ca_cert_path << "\"\n";
            }
            
            // 5. 其他建议配置 (提高兼容性)
            ss << "\tclient_cert=NONE\n"; // TTLS 通常不需要客户端证书
            ss << "\tprivate_key=NONE\n";
            break;
    }

    ss << "}\n";
    return ss.str();
}

::Network::WifiConnectResult  CWifiManager::connectToWifi(::Network::WifiStaConncet_S& config) {
    std::cout << "[连接] 正在配置连接: " << config.ssid 
              << " (模式: " << (int)config.mode << ")" << std::endl;

    ::Network::WifiConnectResult result; // 初始化返回结果
    result.success = false;
    printf("DEBUG: 解析到的 SSID 是: [%s]\n", config.ssid.c_str()); 

    if (config.ssid.empty()) {
            std::cerr << "[错误] SSID 不能为空" << std::endl;
            return result;
        }
    if (config.mode == ::Network::WifiSecurityMode::WPA_PERSONAL && config.password.empty()) {
            std::cerr << "[错误] WPA-Personal 模式下密码不能为空" << std::endl;
            return result;
        }   
    // 1. 生成配置文件内容
    std::string conf_content = generateWpaConfContent(config);
    printf("DEBUG: conf_content 是: [%s]\n", conf_content.c_str()); 
    // 2. 写入临时配置文件
    std::string tmp_conf_path = "/tmp/wpa_supplicant_" + config.interface_name + ".conf";
    
    std::ofstream conf_file(tmp_conf_path);
    if (!conf_file.is_open()) {
        std::cerr << "[错误] 无法创建临时配置文件: " << tmp_conf_path << std::endl;
        return result;
    }
    conf_file << conf_content;
    conf_file.close();

    // 3. 应用配置：覆盖正式配置文件
    std::string final_conf_path = this->config.config_file_path; 
    
    std::string cp_cmd = "cp " + tmp_conf_path + " " + final_conf_path;
    if (system(cp_cmd.c_str()) != 0) {
        std::cerr << "[错误] 无法复制配置文件到: " << final_conf_path << std::endl;
        return result;
    }


    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        this->m_lastConnectConfig = config;
        this->m_hasLastConfig = true;
    }
    if (!saveConfigToFile(config)) {
        std::cerr << "[警告] 配置已应用，但持久化保存失败 (可能磁盘只读)" << std::endl;
    }

    // 4. 触发重连
    std::string res = sendCommand("RECONFIGURE");//更新配置
    if (res.find("FAIL") != std::string::npos) {
        std::cerr << "[警告] RECONFIGURE 失败，重启 wpa_supplicant 守护进程..." << std::endl;
        execShell("killall wpa_supplicant");
        startDaemon(); // 重启
    }

    std::string rets = sendCommand("REASSOCIATE");//连接WiFi
    if (rets.find("FAIL") != std::string::npos) {
        std::cerr << "[警告] REASSOCIATE 失败，重启 wpa_supplicant 守护进程..." << std::endl;
        execShell("killall wpa_supplicant");
        startDaemon(); // 重启
    }

 

    std::cout << "[连接] 请求已发送，正在等待连接结果..." << std::endl;
    
    
    // 这样调用者能立刻知道连接是否成功
    auto start_time = std::chrono::steady_clock::now();
    const std::chrono::seconds timeout(60);
    
    while (true) {
        auto now = std::chrono::steady_clock::now();
        if (now - start_time > timeout) {
            std::cout << "[连接] 超时！连接 " << config.ssid << " 失败。" << std::endl;
            result.error_code=1;
// 如果连接失败，删除可能产生的默认网关，防止劫持有线网络
            std::string clear_route_cmd = "route del default gw 0.0.0.0 dev " + config.interface_name + " 2>/dev/null || true";
            system(clear_route_cmd.c_str());
            return result;
        }

        std::string status = sendCommand("STATUS");
        if (status.find("wpa_state=COMPLETED") != std::string::npos) {
            // 检查是否是当前 SSID
            if (status.find("ssid=" + config.ssid) != std::string::npos) {
                std::cout << "[成功] 已连接到 WiFi: " << config.ssid << std::endl;
                reconnect_attempts = 0;
                is_connected = true;
                this->config.current_ssid = config.ssid;

                system(("udhcpc -i " + config.interface_name + " -n -q 5").c_str()); 
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); 

                std::string ip_info = sendCommand("STATUS");
                size_t ip_pos = ip_info.find("ip_address=");
                
                if (ip_pos != std::string::npos) {
                    size_t ip_end = ip_info.find("\n", ip_pos);
                    // 提取 IP 字符串 (去掉 "ip_address=" 前缀)
                    std::string ip = ip_info.substr(ip_pos + 11, ip_end - ip_pos - 11);
                    // 去除可能的引号或空格
                    ip.erase(std::remove(ip.begin(), ip.end(), '"'), ip.end());
                    ip.erase(std::remove(ip.begin(), ip.end(), ' '), ip.end());
                    
                    result.ip_address = ip;
                    std::cout << "[信息] 已获取 IP 地址: " << result.ip_address << std::endl;
                } else {
                    std::cout << "[警告] 连接成功但未获取到 IP 地址。" << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                lockCurrentIp(); 

                result.success = true;
                result.error_code = 0;
                

                int eth0_has_default_route = system("ip route | grep default | grep eth0 > /dev/null 2>&1");

                if (eth0_has_default_route == 0) {
                    // 情况 A: 有线网络正在使用中
                    std::cout << "[路由] 检测到有线网络已连接，WiFi 仅作为局域网使用。" << std::endl;
                    
                    // 确保删除 WiFi 的默认网关，防止双网关冲突
                    system("route del default dev wlan0 2>/dev/null || true");
                } else {
                    // 情况 B: 有线网络未连接，WiFi 接管互联网
                    std::cout << "[路由] 未检测到有线网络，WiFi 接管默认网关。" << std::endl;
                    
                    // 添加 WiFi 默认网关
                    system("route add default dev wlan0");
                }
                return result;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}



bool CWifiManager::setWifiEnhancedMode() {
    bool nRet = true;
    std::cout << "[增强] 正在开启 WiFi 增强模式..." << std::endl;
    execShell("ip link set " + config.interface_name + " up");
    execShell("iw dev " + config.interface_name + " set txpower fixed 3000"); 
    std::cout << "[增强] 增强模式命令已发送" << std::endl;
    return nRet;
}

Network::WifiStaInfo_S CWifiManager::load_wifi_config()
{
    Network::WifiStaInfo_S cfg;
    memset(&cfg, 0, sizeof(cfg));

    cfg.bEnableWifi = false;

    if (access(m_configFile.c_str(), F_OK) != 0) {
        std::cout << "[Config] 文件不存在，使用默认配置" << std::endl;
        return cfg;
    }

    // 读取文件
    int ret = Convert::read_file(m_configFile, cfg);
    if (ret != 0) {
        std::cerr << "[Config] 读取失败，使用默认配置" << std::endl;
        return cfg;
    }

    std::cout << "[Config] 读取成功: bEnableWifi="
              << cfg.bEnableWifi << std::endl;

    return cfg;
}
void set_wifi_config(Network::WifiStaInfo_S stWifiConfigInfo)
{
    std::cout << "写入WiFi是否开启。" << std::endl;
    Convert::write_file(WIFI_CONFIG_FILE, stWifiConfigInfo);
    
}

// --- 新增函数：强制锁定当前 IP 并持久化 ---
void CWifiManager::lockCurrentIp() {
    // 1. 获取当前 wpa_cli 状态
    std::string status = sendCommand("STATUS");
    
    // 2. 提取 IP 地址
    std::string current_ip = "";
    size_t ip_pos = status.find("ip_address=");
    if (ip_pos != std::string::npos) {
        size_t ip_end = status.find("\n", ip_pos);
        current_ip = status.substr(ip_pos + 11, ip_end - ip_pos - 11);
        // 去除可能的引号或空格
        current_ip.erase(std::remove(current_ip.begin(), current_ip.end(), '"'), current_ip.end());
        current_ip.erase(std::remove(current_ip.begin(), current_ip.end(), ' '), current_ip.end());
    } else {
        std::cout << "[IP锁定] 警告: 当前未获取到 IP 地址，跳过锁定。" << std::endl;
        return; // 没有 IP 无法锁定
    }

    // 3. 提取网关 (Gateway)
    std::string gateway = "192.168.1.1"; // 默认回退值
    size_t gw_pos = status.find("dhcp_router=");
    if (gw_pos != std::string::npos) {
        size_t gw_end = status.find("\n", gw_pos);
        gateway = status.substr(gw_pos + 12, gw_end - gw_pos - 12);
        gateway.erase(std::remove(gateway.begin(), gateway.end(), '"'), gateway.end());
        gateway.erase(std::remove(gateway.begin(), gateway.end(), ' '), gateway.end());
    }

    std::cout << "[IP锁定] 检测到当前 IP: " << current_ip << " 网关: " << gateway << std::endl;

    // 4. 强制执行 ifconfig 覆盖 (核心：防止本次运行期间 IP 变动)
    std::string cmd_ifconfig = "ifconfig " + config.interface_name + " " + current_ip + " netmask 255.255.255.0";
    system(cmd_ifconfig.c_str());


    int eth0_link_up = system("cat /sys/class/net/eth0/carrier 2>/dev/null | grep 1 > /dev/null 2>&1");
    if (eth0_link_up != 0) {
        std::string cmd_route = "route add default gw " + gateway + " " + config.interface_name;
        system(cmd_route.c_str());
        std::cout << "[IP锁定]  网线没插配置wlan网关 "  << std::endl;
    }
    
       
    std::cout << "[IP锁定] 已强制应用静态配置: " << cmd_ifconfig << std::endl;

    // 5. 持久化：如果 IP 发生了变化，更新配置文件
    // 加载旧配置进行对比
    ::Network::WifiStaConncet_S tempConfig;
    bool needSave = false;
    
    if (loadConfigFromFile(tempConfig)) {
        if (tempConfig.ip_address != current_ip) {
            needSave = true; // IP 变了，需要更新文件
        }
    }
    // } else {
    //     needSave = true; // 首次运行，文件不存在
    // }

    if (needSave) {
        tempConfig.ip_address = current_ip;
        if (saveConfigToFile(tempConfig)) {
            std::cout << "[IP锁定] 配置已更新: IP 持久化保存成功。" << std::endl;
        } else {
            std::cerr << "[IP锁定] 错误: 无法保存配置文件!" << std::endl;
        }

        struct stat buffer;
	    Network::Info_S stConfigNetInfo;
        if (stat(NETWORK_CONFIG_FILE, &buffer) == 0)
        {
        
            Convert::read_file(NETWORK_CONFIG_FILE, stConfigNetInfo);
            stConfigNetInfo.stIp.ipv4Ip = current_ip;
            stConfigNetInfo.stIp.netName = "wlan0";
            stConfigNetInfo.stIp.ipv4Gateway = gateway;
            Convert::write_file(NETWORK_WIFI_CONFIG_FILE, stConfigNetInfo);
        
        }
    }
}

// --- 检测 WiFi 连接且有线断开 ---
bool CWifiManager::isWifiConnectedAndWiredDisconnected() {

    std::string status = sendCommand("STATUS");
    
    bool isWifiConnected = (status.find("wpa_state=COMPLETED") != std::string::npos);
    
    int eth0_link_up = system("cat /sys/class/net/eth0/carrier 2>/dev/null | grep 1 > /dev/null 2>&1");
    bool isWiredDisconnected = (eth0_link_up != 0); 
    std::cout << "[检测] WiFi和线网络。" << std::endl;
    if (isWifiConnected && isWiredDisconnected) {
        std::cout << "[检测] WiFi已连接，且有线网络已断开。" << std::endl;
        return true;
    } else {
        if (!isWifiConnected) {
            std::cout << "[检测] WiFi未连接 (状态: " << (status.empty() ? "空" : status.substr(0, 20)) << "...)" << std::endl;
        }
        if (!isWiredDisconnected) {
            std::cout << "[检测] 有线网络仍连接。" << std::endl;
        }
        return false;
    }
}
#endif