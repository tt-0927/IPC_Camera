// 包含头文件
// #define CAP_NETWORK_WIFI 1
#if CAP_NETWORK_WIFI
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <chrono>
#include <arpa/inet.h>
#include <algorithm>
#include "wifi_manage.h" 
#include "platform_manager.h"
#include "rtsp_server.h"
#include "record_ctrl.h"
#include "capture_ctrl.h"
namespace {
const char* WIFI_SUPPLICANT_LOG_PATH = "/tmp/wpa_supplicant_wifi.log";
const std::streamoff WIFI_SUPPLICANT_LOG_MAX_SIZE = 256 * 1024;

std::streamoff getFileSize(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return 0;
    }
    return static_cast<std::streamoff>(file.tellg());
}

void clearSupplicantLog() {
    std::ofstream logFile(WIFI_SUPPLICANT_LOG_PATH, std::ios::trunc);
    logFile.close();
}

void trimSupplicantLogIfNeeded() {
    if (getFileSize(WIFI_SUPPLICANT_LOG_PATH) > WIFI_SUPPLICANT_LOG_MAX_SIZE) {
        clearSupplicantLog();
    }
}

std::string readFileFromOffset(const char* path, std::streamoff offset) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    file.seekg(0, std::ios::end);
    const std::streamoff fileSize = static_cast<std::streamoff>(file.tellg());
    if (offset < 0 || offset > fileSize) {
        offset = 0;
    }

    file.seekg(offset, std::ios::beg);
    std::stringstream content;
    content << file.rdbuf();
    return content.str();
}

int detectSupplicantError(const std::string& log, ::Network::WifiSecurityMode mode) {
    const bool isPersonalMode =
        mode == ::Network::WifiSecurityMode::WPA_PERSONAL ||
        mode == ::Network::WifiSecurityMode::WPA3_PERSONAL;

    if (log.find("Invalid passphrase length") != std::string::npos ||
        log.find("Invalid PSK") != std::string::npos ||
        log.find("failed to parse psk") != std::string::npos) {
        return ::Network::WIFI_CONNECT_INVALID_CREDENTIALS;
    }

    if (log.find("reason=WRONG_KEY") != std::string::npos ||
        log.find("pre-shared key may be incorrect") != std::string::npos ||
        log.find("SAE: Authentication failed") != std::string::npos ||
        (isPersonalMode && log.find("reason=AUTH_FAILED") != std::string::npos)) {
        return ::Network::WIFI_CONNECT_WRONG_PASSWORD;
    }

    if (log.find("CTRL-EVENT-NETWORK-NOT-FOUND") != std::string::npos) {
        return ::Network::WIFI_CONNECT_NETWORK_NOT_FOUND;
    }

    if (log.find("CTRL-EVENT-EAP-FAILURE") != std::string::npos ||
        log.find("EAP authentication failed") != std::string::npos ||
        log.find("CTRL-EVENT-AUTH-REJECT") != std::string::npos ||
        log.find("reason=AUTH_FAILED") != std::string::npos) {
        return ::Network::WIFI_CONNECT_AUTHENTICATION_FAILED;
    }

    return ::Network::WIFI_CONNECT_UNKNOWN_ERROR;
}
bool isIpv4RoutePrefix(const std::string& routePrefix) {
    const std::size_t slashPos = routePrefix.find('/');
    const std::string address = routePrefix.substr(0, slashPos);
    struct in_addr ipv4Address;

    if (inet_pton(AF_INET, address.c_str(), &ipv4Address) != 1) {
        return false;
    }

    if (slashPos == std::string::npos) {
        return true;
    }

    const std::string prefixLength = routePrefix.substr(slashPos + 1);
    if (prefixLength.empty() ||
        !std::all_of(prefixLength.begin(), prefixLength.end(), [](unsigned char ch) {
            return std::isdigit(ch) != 0;
        })) {
        return false;
    }

    const long parsedLength = std::strtol(prefixLength.c_str(), nullptr, 10);
    return parsedLength >= 0 && parsedLength <= 32;
}

bool executeRouteCommand(const std::string& command) {
    const int status = std::system(command.c_str());
    if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        dlog_error("[路由] 命令执行失败: %s, status=%d", command.c_str(), status);
        return false;
    }
    return true;
}
}

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

CWifiManager::CWifiManager() :  is_running(false),is_connected(false), m_hasConnectedOnce(false), m_isConnecting(false), reconnect_attempts(0), m_configFile(WIFI_CONFIG_FILE) {
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
    m_monitorCondition.notify_all();
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

    // 每次启动时清空旧日志，避免日志无限增长和旧错误干扰本次判断。
    clearSupplicantLog();
    
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

    std::string cmd = "wpa_supplicant -D" + config.driver + " -i" + config.interface_name +
                      " -c" + config.config_file_path + " -f" + WIFI_SUPPLICANT_LOG_PATH + " -B";

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
    std::unique_lock<std::mutex> monitorLock(m_monitorMutex);

    while (is_running.load()) {
        // 从未成功连接过 WiFi 时一直休眠，避免无意义地轮询 STATUS。
        // 首次连接成功或线程准备退出时，会通过 notify_all() 唤醒这里。
        m_monitorCondition.wait(monitorLock, [this] {
            return !is_running.load() || m_hasConnectedOnce.load();
        });

        if (!is_running.load()) {
            break;
        }

        monitorLock.unlock();

        std::string status = sendCommand("STATUS");
        if (status.find("wpa_state=DISCONNECTED") != std::string::npos ||
            status.find("wpa_state=SCANNING") != std::string::npos) {
            std::cout << "[后台监听] Wi-Fi 意外断开" << std::endl;
            is_connected.store(false);

            if (m_hasConnectedOnce.load() && !m_isConnecting.load()) {
                handleDisconnect();
            }

            if (is_connected.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                lockCurrentIp(); // 重连后立即重新锁定 IP

                const int max_retries = 3;
                for (int i = 0; i < max_retries; ++i)
                {
                    int ret = CPlatformManager::instance()->change_net_relogin();
                    if (ret == 0)
                    {
                        break;
                    }
                    else
                    {
                        if (i < max_retries - 1)
                        {
                            dlog_error("，准备进行第 (%d)  次重试...", (i + 1));
                        }
                        else
                        {
                            dlog_error("，已达最大重试次数，放弃执行。");
                        }
                    }
                }
            }
        }
        else if (status.find("wpa_state=COMPLETED") != std::string::npos) {
            is_connected.store(true);
            trimSupplicantLogIfNeeded();
            // 每 3 分钟检查一次 IP 状态，防止静默变更
            static auto last_ip_check = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            if (now - last_ip_check > std::chrono::minutes(3)) {
                lockCurrentIp(); // 强制刷新并锁定
                last_ip_check = now;
            }
        }


        // int eth0_link_up = system("cat /sys/class/net/eth0/carrier 2>/dev/null | grep 1 > /dev/null 2>&1");
        // if (eth0_link_up == 0) {
        //     // 情况 A: 网线插好了
        //     // 只要网线插着，就强制走有线
            
        //     Network::Info_S stNetInfo;
        //     CNetworkManage::instance()->get_system_networkInfo(stNetInfo);
        //     if (stNetInfo.stIp.ipv4Ip.length() >= 4 && stNetInfo.stIp.ipv4Ip.substr(0, 4) == "192.") {
        //         system("ip route replace default via 192.168.1.254 dev eth0");
        //     }
        //     else {
        //         system("ip route replace default via 172.16.25.254 dev eth0");
        //     }
            
        // } else {
        //     // 情况 B: 网线没插 
        //     // 只有当 WiFi 连接时，才切换路由
        //     if (is_connected) {
        //         std::cout << "[监控] 检测到 eth0 网线断开，切换至 WiFi..." << std::endl;
        //         system("ip route replace default dev wlan0");
        //     }
        // }

        monitorLock.lock();
        // 正常情况下最多等待 30 秒，然后继续检查一次 WiFi 状态。
        // 程序退出或用户主动断开 WiFi 时，可通过 notify_all() 提前结束等待。
        m_monitorCondition.wait_for(
            monitorLock,
            std::chrono::seconds(30),
            [this] {
                return !is_running.load() || !m_hasConnectedOnce.load();
            });
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
        m_hasConnectedOnce.store(false);
        m_monitorCondition.notify_all();
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
    file << "ip_address=" << config.ip_address << "\n"; //
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
    m_hasConnectedOnce.store(false);
    m_monitorCondition.notify_all();
    if (monitor_thread.joinable()) {
        monitor_thread.join();
        std::cout << "[反初始化] 监控线程已停止。" << std::endl;
    }

    is_connected.store(false);

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

std::string decodeEscapedUtf8(const std::string& input)
{
    std::string output;

    for(size_t i=0;i<input.size();)
    {
        // 匹配 \xHH
        if(i+3<input.size() &&
           input[i]=='\\' &&
           input[i+1]=='x' &&
           isxdigit(input[i+2]) &&
           isxdigit(input[i+3]))
        {
            std::string hex=input.substr(i+2,2);

            unsigned char value=
                static_cast<unsigned char>(
                    strtoul(
                        hex.c_str(),
                        nullptr,
                        16));

            output.push_back(
                static_cast<char>(value));

            i+=4;
        }
        else
        {
            output.push_back(input[i]);
            i++;
        }
    }

    return output;
}


std::map<std::string,std::string>
parseStatus(const std::string& status)
{
    std::map<std::string,std::string> result;

    std::stringstream ss(status);

    std::string line;

    while(std::getline(ss,line))
    {
        size_t pos=line.find('=');

        if(pos==std::string::npos)
        {
            continue;
        }

        std::string key=
            line.substr(0,pos);

        std::string value=
            line.substr(pos+1);

        // 中文SSID自动解码
        if(key=="ssid")
        {
            value=
                decodeEscapedUtf8(
                    value);
        }

        result[key]=value;
    }

    return result;
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
    std::this_thread::sleep_for(std::chrono::seconds(3)); 

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
            // info.ssid.erase(info.ssid.find_last_not_of(" \t") + 1);
            
            info.ssid = decodeEscapedUtf8(info.ssid);
            // 检查是否当前连接
            // if (info.flags.find("[CURRENT]") != std::string::npos) {
            //     info.is_current = true;
            //     config.current_ssid = info.ssid;
            // }
            if (config.current_ssid == info.ssid) {
                std::string status = sendCommand("STATUS");
                // if (status.find("ssid=" + info.ssid) != std::string::npos)
                // {
                //     std::cout << "已经连接的wifi " << line << std::endl;
                //     info.is_current = true;
                // }

                auto statusInfo=
                    parseStatus(status);

                std::string current_bssid=
                    statusInfo["bssid"];

                std::string current_ssid=
                    statusInfo["ssid"];

                dlog_info(
                    "STATUS BSSID:[%s] 扫描BSSID:[%s]",
                    current_bssid.c_str(),
                    info.bssid.c_str());

                dlog_info(
                    "STATUS SSID:[%s] 扫描SSID:[%s]",
                    current_ssid.c_str(),
                    info.ssid.c_str());

                if(current_bssid==info.bssid)
                {
                    info.is_current=true;

                    std::cout
                        << "已经连接:"
                        << info.ssid
                        << std::endl;
                }
            }

            // 判断频段
            if (info.frequency > 3000) {
                info.band = "5GHz";
            } else {
                info.band = "2.4GHz";
            }

            // 判断加密方式
            if (info.flags.find("WPA3") != std::string::npos || info.flags.find("WPA2-SAE") != std::string::npos) {
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
    m_hasConnectedOnce.store(false);
    m_monitorCondition.notify_all();
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
            is_connected.store(false);
            m_hasLastConfig = false; 
        }
    
        std::cout << "[断开] 清理流程完成." << std::endl;
        return true; 
    
    // }
}

// --- 辅助函数：生成 wpa_supplicant 配置文件内容 ---
// 这个函数负责把 C++ 结构体转换成 wpa_supplicant 能读懂的文本格式
std::string utf8ToHex(const std::string& str)
{
    std::stringstream ss;

    for (unsigned char c : str)
    {
        ss << std::uppercase
           << std::hex
           << std::setw(2)
           << std::setfill('0')
           << (int)c;
    }

    return ss.str();
}

bool containsNonAscii(const std::string& str)
{
    for(unsigned char c : str)
    {
        if(c > 127)
        {
            return true;
        }
    }

    return false;
}
std::string generateWpaConfContent(const ::Network::WifiStaConncet_S& config) {
    std::stringstream ss;
    
    // 1. 全局配置头
    ss << "ctrl_interface=" << config.ctrl_interface << "\n";
    ss << "update_config=1\n";
    ss << "ap_scan=1\n"; // 强制由客户端决定 AP
    ss << "network={\n";
    
    // 2. 通用 SSID 设置
    // ss << "\tssid=\"" << config.ssid << "\"\n";

    if (containsNonAscii(config.ssid))
    {
        std::string hex_ssid = utf8ToHex(config.ssid);
    
        dlog_info(
            "[WiFi] 检测到中文SSID:[%s] -> HEX:[%s]",
            config.ssid.c_str(),
            hex_ssid.c_str());
    
        ss << "\tssid=" << hex_ssid << "\n";
    }
    else
    {
        ss << "\tssid=\"" << config.ssid << "\"\n";
    }

    // 3. 根据不同模式生成配置
    switch (config.mode) {
        case ::Network::WifiSecurityMode::OPEN:
            // --- 开放网络 ---
            ss << "\tkey_mgmt=NONE\n";
            break;

        case ::Network::WifiSecurityMode::WPA_PERSONAL:
            // --- WPA-个人版 ---
            ss << "\tkey_mgmt=WPA-PSK\n";
            // if (config.pairwise.empty()) {
            //     // 支持 WPA 和 WPA2，组合写法让 wpa_supplicant 尝试多种协议/加密
            //     ss << "\tproto=WPA RSN\n"; // 支持 WPA (TKIP) 和 RSN (CCMP)
            //     ss << "\tgroup=TKIP CCMP\n"; // 允许组播/广播同时使用 TKIP 或 CCMP
            // } else {
            //     ss << "\tproto=RSN\n"; // 默认使用 RSN (WPA2)
            //     ss << "\tpairwise=" << config.pairwise << "\n"; // TKIP 或 CCMP
            //     ss << "\tgroup=CCMP\n";
            // }
            if (config.pairwise.empty())
            {
                // 自动协商，不限制算法
            }
            else if (config.pairwise == "AUTO")
            {
                // 自动协商，不限制算法
            }
            else if (config.pairwise == "CCMP")
            {
                ss << "\tproto=RSN\n";
                ss << "\tpairwise=CCMP\n";
                ss << "\tgroup=CCMP\n";
            }
            else if (config.pairwise == "TKIP")
            {
                ss << "\tproto=WPA\n";
                ss << "\tpairwise=TKIP\n";
                ss << "\tgroup=TKIP CCMP\n";
            }
            else if (config.pairwise == "TKIP_RSN")
            {
                // WPA2 TKIP
                ss << "\tproto=RSN\n";
                ss << "\tpairwise=TKIP\n";
                ss << "\tgroup=TKIP\n";
            }
            ss << "\tpsk=\"" << config.password << "\"\n";
            break;

        case ::Network::WifiSecurityMode::WPA3_PERSONAL:
        {
            ss << "\tkey_mgmt=SAE\n";
            ss << "\tieee80211w=2\n";
            ss << "\tproto=RSN\n";
            if (config.pairwise.empty())
            {
            }
            else if (config.pairwise == "AUTO")
            {
                // 自动协商，不限制算法
            }
            else if (config.pairwise == "CCMP")
            {
                ss << "\tpairwise=CCMP\n";
                ss << "\tgroup=CCMP\n";
            }
            else
            {
                dlog_warn("WPA3 不支持 TKIP，忽略 pairwise=%s", config.pairwise.c_str());
            }

            ss << "\tpsk=\"" << config.password << "\"\n";

            break;
        }

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


              printf("SSID原始内容:[%s]\n",
                config.ssid.c_str());
         
         printf("SSID HEX:[%s]\n",
                utf8ToHex(config.ssid).c_str());

    ::Network::WifiConnectResult result; // 初始化返回结果
    result.success = false;
    result.error_code = ::Network::WIFI_CONNECT_UNKNOWN_ERROR;

    bool expectedConnecting = false;
    if (!m_isConnecting.compare_exchange_strong(expectedConnecting, true)) {
        result.error_code = ::Network::WIFI_CONNECT_BUSY;
        return result;
    }

    struct ConnectingGuard {
        std::atomic<bool>& connecting;
        ~ConnectingGuard() {
            connecting.store(false);
        }
    } connectingGuard{m_isConnecting};

    is_connected.store(false);
    printf("DEBUG: 解析到的 SSID 是: [%s]\n", config.ssid.c_str()); 

    if (config.ssid.empty()) {
            std::cerr << "[错误] SSID 不能为空" << std::endl;
            result.error_code = ::Network::WIFI_CONNECT_INVALID_SSID;
            return result;
        }
    if ((config.mode == ::Network::WifiSecurityMode::WPA_PERSONAL ||
         config.mode == ::Network::WifiSecurityMode::WPA3_PERSONAL) &&
        config.password.empty()) {
            std::cerr << "[错误] WPA/WPA3-Personal 模式下密码不能为空" << std::endl;
            result.error_code = ::Network::WIFI_CONNECT_INVALID_CREDENTIALS;
            return result;
        }
    if (config.mode == ::Network::WifiSecurityMode::WEP && config.wep_keys.empty()) {
            std::cerr << "[错误] WEP 模式下密钥不能为空" << std::endl;
            result.error_code = ::Network::WIFI_CONNECT_INVALID_CREDENTIALS;
            return result;
        }
    if ((config.mode == ::Network::WifiSecurityMode::EAP_PEAP ||
         config.mode == ::Network::WifiSecurityMode::EAP_TTLS) &&
        (config.eap_identity.empty() || config.eap_password.empty())) {
            std::cerr << "[错误] 企业认证模式下用户名和密码不能为空" << std::endl;
            result.error_code = ::Network::WIFI_CONNECT_INVALID_CREDENTIALS;
            return result;
        }
    if (config.mode == ::Network::WifiSecurityMode::EAP_TLS &&
        (config.tls_identity.empty() || config.client_cert_path.empty() ||
         config.private_key_path.empty())) {
            std::cerr << "[错误] EAP-TLS 模式下身份、用户证书和私钥不能为空" << std::endl;
            result.error_code = ::Network::WIFI_CONNECT_INVALID_CREDENTIALS;
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
        result.error_code = ::Network::WIFI_CONNECT_TEMP_CONFIG_FAILED;
        return result;
    }
    conf_file << conf_content;
    conf_file.close();

    // 3. 应用配置：覆盖正式配置文件
    std::string final_conf_path = this->config.config_file_path; 
    
    std::string cp_cmd = "cp " + tmp_conf_path + " " + final_conf_path;
    if (system(cp_cmd.c_str()) != 0) {
        std::cerr << "[错误] 无法复制配置文件到: " << final_conf_path << std::endl;
        result.error_code = ::Network::WIFI_CONNECT_APPLY_CONFIG_FAILED;
        return result;
    }


    // 清空历史日志，本次只保留当前连接产生的诊断事件。
    clearSupplicantLog();
    struct SupplicantLogCleanup {
        ~SupplicantLogCleanup() {
            clearSupplicantLog();
        }
    } supplicantLogCleanup;

    // 记录本次连接开始前的日志位置，只分析本次连接产生的事件。
    std::streamoff supplicantLogOffset = getFileSize(WIFI_SUPPLICANT_LOG_PATH);

    // 4. 先断开旧连接，清除上一轮错误密码留下的握手状态。
    sendCommand("DISCONNECT");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 重新加载配置文件。
    std::string res = sendCommand("RECONFIGURE");//更新配置
    if (res.find("FAIL") != std::string::npos) {
        std::cerr << "[警告] RECONFIGURE 失败，重启 wpa_supplicant 守护进程..." << std::endl;
        execShell("killall wpa_supplicant");
        if (!startDaemon()) {
            result.error_code = ::Network::WIFI_CONNECT_SUPPLICANT_START_FAILED;
            return result;
        }
        supplicantLogOffset = getFileSize(WIFI_SUPPLICANT_LOG_PATH);
        if (system(cp_cmd.c_str()) != 0) {
            result.error_code = ::Network::WIFI_CONNECT_APPLY_CONFIG_FAILED;
            return result;
        }
        res = sendCommand("RECONFIGURE");
        if (res.find("FAIL") != std::string::npos) {
            result.error_code = ::Network::WIFI_CONNECT_RECONFIGURE_FAILED;
            return result;
        }
    }

    // 连续密码错误后网络可能处于 TEMP-DISABLED，显式启用可清除禁用计时。
    std::string enableResult = sendCommand("ENABLE_NETWORK all");
    if (enableResult.find("FAIL") != std::string::npos) {
        result.error_code = ::Network::WIFI_CONNECT_ENABLE_NETWORK_FAILED;
        return result;
    }

    std::string rets = sendCommand("RECONNECT");//使用新配置重新连接 WiFi
    if (rets.find("FAIL") != std::string::npos) {
        std::cerr << "[警告] RECONNECT 失败，重启 wpa_supplicant 守护进程..." << std::endl;
        execShell("killall wpa_supplicant");
        if (!startDaemon()) {
            result.error_code = ::Network::WIFI_CONNECT_SUPPLICANT_START_FAILED;
            return result;
        }
        supplicantLogOffset = getFileSize(WIFI_SUPPLICANT_LOG_PATH);
        if (system(cp_cmd.c_str()) != 0) {
            result.error_code = ::Network::WIFI_CONNECT_APPLY_CONFIG_FAILED;
            return result;
        }
        res = sendCommand("RECONFIGURE");
        if (res.find("FAIL") != std::string::npos) {
            result.error_code = ::Network::WIFI_CONNECT_RECONFIGURE_FAILED;
            return result;
        }
        enableResult = sendCommand("ENABLE_NETWORK all");
        if (enableResult.find("FAIL") != std::string::npos) {
            result.error_code = ::Network::WIFI_CONNECT_ENABLE_NETWORK_FAILED;
            return result;
        }
        rets = sendCommand("RECONNECT");
        if (rets.find("FAIL") != std::string::npos) {
            result.error_code = ::Network::WIFI_CONNECT_RECONNECT_FAILED;
            return result;
        }
    }

 

    std::cout << "[连接] 请求已发送，正在等待连接结果..." << std::endl;
    
    
    // 这样调用者能立刻知道连接是否成功
    auto start_time = std::chrono::steady_clock::now();
    const std::chrono::seconds timeout(30);
    bool sawScanning = false;
    bool sawAssociating = false;
    bool sawFourWayHandshake = false;
    
    while (true) {
        auto now = std::chrono::steady_clock::now();
        if (now - start_time > timeout) {
            std::cout << "[连接] 超时！连接 " << config.ssid << " 失败。" << std::endl;
            const std::string supplicantLog =
                readFileFromOffset(WIFI_SUPPLICANT_LOG_PATH, supplicantLogOffset);
            const int detectedError = detectSupplicantError(supplicantLog, config.mode);

            if (detectedError != ::Network::WIFI_CONNECT_UNKNOWN_ERROR) {
                result.error_code = detectedError;
            } else if (sawFourWayHandshake &&
                       (config.mode == ::Network::WifiSecurityMode::WPA_PERSONAL ||
                        config.mode == ::Network::WifiSecurityMode::WPA3_PERSONAL)) {
                result.error_code = ::Network::WIFI_CONNECT_WRONG_PASSWORD;
            } else if (sawScanning && !sawAssociating) {
                result.error_code = ::Network::WIFI_CONNECT_NETWORK_NOT_FOUND;
            } else {
                result.error_code = ::Network::WIFI_CONNECT_TIMEOUT;
            }
// 如果连接失败，删除可能产生的默认网关，防止劫持有线网络
            // std::string clear_route_cmd = "route del default gw 0.0.0.0 dev " + config.interface_name + " 2>/dev/null || true";
            // system(clear_route_cmd.c_str());
            sendCommand("DISCONNECT");
            return result;
        }

        std::string status = sendCommand("STATUS");
        if (status.find("wpa_state=COMPLETED") != std::string::npos) {
            // 检查是否是当前 SSID
            // if (status.find("ssid=" + config.ssid) != std::string::npos) 
                {
                std::cout << "[成功] 已连接到 WiFi: " << config.ssid << std::endl;
                reconnect_attempts = 0;
                is_connected.store(true);
                m_hasConnectedOnce.store(true);
                m_monitorCondition.notify_all();
                this->config.current_ssid = config.ssid;

                // system(("udhcpc -i " + config.interface_name + " -n -q 5").c_str());
                system(("udhcpc -i " + config.interface_name + " -n -q").c_str());
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

                {
                    std::lock_guard<std::mutex> lock(m_configMutex);
                    this->m_lastConnectConfig = config;
                    this->m_hasLastConfig = true;
                }

                if (!saveConfigToFile(config)) {
                    std::cerr << "[警告] 配置已应用，但持久化保存失败 (可能磁盘只读)" << std::endl;
                }
            
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                lockCurrentIp(); 

                result.success = true;
                result.error_code = ::Network::WIFI_CONNECT_SUCCESS;
                

                int eth0_has_default_route = system("ip route | grep default | grep eth0 > /dev/null 2>&1");

                if (eth0_has_default_route == 0) {
                    // 情况 A: 有线网络正在使用中
                    std::cout << "[路由] 检测到有线网络已连接，WiFi 仅作为局域网使用。" << std::endl;
                    
                    // 确保删除 WiFi 的默认网关，防止双网关冲突
                    system("route del default dev wlan0 2>/dev/null || true");
                } else {
                //     // 情况 B: 有线网络未连接，WiFi 接管互联网
                //     std::cout << "[路由] 未检测到有线网络，WiFi 接管默认网关。" << std::endl;
                    
                //     // 添加 WiFi 默认网关
                //     system("route add default dev wlan0");
                }
                return result;
            }
        }
        if(status.find("wpa_state=DISCONNECTED")!=std::string::npos)
        {
            std::cout<<"断开"<<std::endl;
        }
        if(status.find("wpa_state=SCANNING")!=std::string::npos)
        {
            sawScanning = true;
        }
        if(status.find("wpa_state=ASSOCIATING")!=std::string::npos)
        {
            sawAssociating = true;
        }
        if(status.find("wpa_state=4WAY_HANDSHAKE")!=std::string::npos)
        {
            sawFourWayHandshake = true;
        }
        if(status.find("wpa_state=GROUP_HANDSHAKE")!=std::string::npos)
        {
        }

        const std::string supplicantLog =
            readFileFromOffset(WIFI_SUPPLICANT_LOG_PATH, supplicantLogOffset);
        const int detectedError = detectSupplicantError(supplicantLog, config.mode);
        if (detectedError != ::Network::WIFI_CONNECT_UNKNOWN_ERROR) {
            std::cerr << "[连接] wpa_supplicant 返回错误码: " << detectedError << std::endl;
            result.error_code = detectedError;
            sendCommand("DISCONNECT");
            return result;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}



bool CWifiManager::setWifiEnhancedMode(bool bEnableBoost) {
    bool nRet = true;
    ::Network::WifiStaInfo_S stInfo;
    get_wifi_config(stInfo);
    if(stInfo.bEnableBoost == bEnableBoost)
    {
        return nRet;
    }
    else {
        if(bEnableBoost)
        {
            stInfo.bEnableBoost = bEnableBoost;
            dlog_debug("[增强] 正在开启 WiFi 增强模式..." );
            execShell("cp /lib/firmware/aic8800D80/aic_userconfig_8800d80_plus2dB.txt /lib/firmware/aic8800D80/aic_userconfig_8800d80.txt");
        }
        else {
            stInfo.bEnableBoost = bEnableBoost;
            dlog_debug("[增强] 正在关闭 WiFi 增强模式..." );
            execShell("cp /lib/firmware/aic8800D80/aic_userconfig_8800d80_original.txt /lib/firmware/aic8800D80/aic_userconfig_8800d80.txt");
        }
    }
    set_wifi_config(stInfo);
    auto thrRun = []()
    {
        CCaptureCtrl::instance()->stop_capture();
        CRecordCtrl::instance()->stop_record();
        /* 延时2秒重启 */
        std::this_thread::sleep_for(std::chrono::seconds(2));  
        system("sync;reboot");
    };
    std::thread thr(thrRun);
    thr.detach();
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

void get_wifi_config(Network::WifiStaInfo_S &outWifiConfigInfo) {
    std::cout << "读取WiFi配置信息。" << std::endl;
    Convert::read_file(WIFI_CONFIG_FILE, outWifiConfigInfo);
}

static std::string getGatewayByInterface(const std::string &iface)
{
    char buf[128] = { 0 };

    std::string cmd = "ip route | grep default | grep " + iface + " | awk '{print $3}'";

    FILE *fp = popen(cmd.c_str(), "r");
    if (!fp)
    {
        return "";
    }

    if (fgets(buf, sizeof(buf), fp))
    {
        std::string gateway(buf);

        gateway.erase(std::remove(gateway.begin(), gateway.end(), '\n'), gateway.end());

        pclose(fp);
        return gateway;
    }

    pclose(fp);

    return "";
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
    // std::string gateway = "192.168.1.1"; // 默认回退值
    // size_t gw_pos = status.find("dhcp_router=");
    // if (gw_pos != std::string::npos) {
    //     size_t gw_end = status.find("\n", gw_pos);
    //     gateway = status.substr(gw_pos + 12, gw_end - gw_pos - 12);
    //     gateway.erase(std::remove(gateway.begin(), gateway.end(), '"'), gateway.end());
    //     gateway.erase(std::remove(gateway.begin(), gateway.end(), ' '), gateway.end());
    // }
    std::string gateway = getGatewayByInterface(config.interface_name);
    if (!gateway.empty())
    {
        m_wifiGateway = gateway;

        std::cout << "[IP锁定] 缓存WiFi网关:" << m_wifiGateway << std::endl;
    }

    std::cout << "[IP锁定] 检测到当前 IP: " << current_ip << " 网关: " << gateway << std::endl;

    // 4. 强制执行 ifconfig 覆盖 (核心：防止本次运行期间 IP 变动)
    // std::string cmd_ifconfig = "ifconfig " + config.interface_name + " " + current_ip + " netmask 255.255.255.0";
    // system(cmd_ifconfig.c_str());


    // int eth0_link_up = system("cat /sys/class/net/eth0/carrier 2>/dev/null | grep 1 > /dev/null 2>&1");
    // if (eth0_link_up != 0) {
    //     // std::string cmd_route = "route add default gw " + gateway + " " + config.interface_name;
    //     // system(cmd_route.c_str());
    //     system("route del default 2>/dev/null");
    //     std::string cmd_route = "ip route replace default via " + gateway + " dev " + config.interface_name;
    //     system(cmd_route.c_str());
    //     system("ip route flush cache");
    //     std::cout << "[IP锁定]  网线没插配置wlan网关 "  << std::endl;
    // }
    
       
    // std::cout << "[IP锁定] 已强制应用静态配置: " << cmd_ifconfig << std::endl;

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
bool CWifiManager::isWifiConnectedAndWiredDisconnected(bool bWiredDisconnected)
{
    if (m_lastWiredDisconnected == bWiredDisconnected)
    {
        return true;
    }

    //m_lastWiredDisconnected = bWiredDisconnected;

    if (bWiredDisconnected)
    {
        std::cout << "[路由] 检测到网线断开" << std::endl;

        if (!is_connected.load())
        {
            std::cout << "[路由] WiFi未连接" << std::endl;

            return false;
        }

        //return switchToWifi();
        if (!switchToWifi())
        {
            /* 保留旧状态，下一轮 5 秒检测会继续尝试切换。 */
            return false;
        }

        m_lastWiredDisconnected = true;
        return true;
    }
    else
    {
        std::cout << "[路由] 检测到网线恢复" << std::endl;

        //return switchToEth();
        if (!switchToEth())
        {
            /* 保留旧状态，避免有线恢复但路由未恢复后停止重试。 */
            return false;
        }

        m_lastWiredDisconnected = false;
        return true;
    }
}

std::string CWifiManager::getEthGateway()
{
    Network::Info_S stNetInfo;

    if (CNetworkManage::instance()->get_system_networkInfo(stNetInfo) != 0)
    {
        dlog_error("[路由] 获取系统网络配置失败");

        return "";
    }

    std::string gateway = stNetInfo.stIp.ipv4Gateway;

    dlog_info("[路由] eth0 gateway:[%s]",
              gateway.c_str());

    return gateway;
}

bool CWifiManager::remove_eth_direct_routes()
{
    /* 默认路由添加失败时会重试切换，不能覆盖首次移除前保存的路由快照。 */
    if (m_ethDirectRoutesRemoved)
    {
        return true;
    }

    m_ethDirectRoutes.clear();

    /*
     * eth0 即使 NO-CARRIER，内核仍会保留由 IPv4 地址自动生成的 scope link
     * 路由。例如 172.16.25.0/24 dev eth0 会优先于 wlan0 默认路由，导致访问
     * 同网段平台时仍从已断开的网线发包。
     */
    FILE* pipe = popen("ip -4 route show dev eth0 scope link", "r");
    if (pipe == nullptr)
    {
        dlog_error("[路由] 获取 eth0 直连路由失败");
        return false;
    }

    char line[256] = {0};
    while (fgets(line, sizeof(line), pipe) != nullptr)
    {
        std::istringstream lineStream(line);
        std::string routePrefix;
        lineStream >> routePrefix;

        if (isIpv4RoutePrefix(routePrefix))
        {
            m_ethDirectRoutes.push_back(routePrefix);
        }
    }

    const int pipeStatus = pclose(pipe);
    if (pipeStatus == -1 || !WIFEXITED(pipeStatus) || WEXITSTATUS(pipeStatus) != 0)
    {
        dlog_error("[路由] 读取 eth0 直连路由命令结束失败");
        return false;
    }

    for (const std::string& routePrefix : m_ethDirectRoutes)
    {
        const std::string command = "ip -4 route del " + routePrefix + " dev eth0";
        if (!executeRouteCommand(command))
        {
            return false;
        }
        dlog_info("[路由] 已移除失效 eth0 直连路由: %s", routePrefix.c_str());
    }

    m_ethDirectRoutesRemoved = true;
    return true;
}

bool CWifiManager::restore_eth_direct_routes()
{
    for (const std::string& routePrefix : m_ethDirectRoutes)
    {
        const std::string command = "ip -4 route replace " + routePrefix +
                                    " dev eth0 scope link";
        if (!executeRouteCommand(command))
        {
            return false;
        }
        dlog_info("[路由] 已恢复 eth0 直连路由: %s", routePrefix.c_str());
    }

    m_ethDirectRoutes.clear();
    m_ethDirectRoutesRemoved = false;
    return true;
}

bool CWifiManager::switchToWifi()
{
    if (m_wifiGateway.empty())
    {
        std::cout << "[路由] WiFi网关为空" << std::endl;

        return false;
    }

    if (!remove_eth_direct_routes())
    {
        return false;
    }

    /* 删除失效有线默认路由；无默认路由时返回非零属于正常情况。 */
    system("ip route del default dev eth0 2>/dev/null");
    system("ip route del default dev wlan0 2>/dev/null");

    std::string cmd = "ip route add default via " + m_wifiGateway + " dev wlan0";

    //int ret = system(cmd.c_str());
    if (!executeRouteCommand(cmd))
    {
        return false;
    }

    system("ip route flush cache");

    // std::cout << "[路由] 已切换到WiFi:" << m_wifiGateway << std::endl;
    // // CPlatformManager::instance()->change_net_relogin();
    // usleep(50000);
    // const int max_retries = 3;
    // for (int i = 0; i < max_retries; ++i)
    dlog_info("[路由] 已切换到 WiFi，gateway=%s", m_wifiGateway.c_str());

    /* 平台失败后由 PlatformManager 的 30 秒后台重试接管，避免阻塞路由检测线程。 */
    if (CPlatformManager::instance()->change_net_relogin() != OK)
    {
        // int ret = CPlatformManager::instance()->change_net_relogin();
        // if (ret == 0)
        // {
        //     break;
        // }
        // else
        // {
        //     if (i < max_retries - 1)
        //     {
        //         dlog_error("，准备进行第 (%d)  次重试...", (i + 1));
        //     }
        //     else
        //     {
        //         dlog_error("，已达最大重试次数，放弃执行。");
        //     }
        // }
        dlog_warn("[路由] WiFi 已切换，平台立即重登失败，等待后台重试");
    }
    //return (ret == 0);
    return true;
}
bool CWifiManager::switchToEth()
{
    std::string ethGateway = getEthGateway();

    if (ethGateway.empty())
    {
        dlog_error("[路由] eth0 gateway为空");

        return false;
    }

     if (!restore_eth_direct_routes())
    {
        return false;
    }

    system("ip route del default dev wlan0 2>/dev/null");
    system("ip route del default dev eth0 2>/dev/null");

    std::string cmd =
        "ip route add default via " +
        ethGateway +
        " dev eth0";

    dlog_info("[路由] %s", cmd.c_str());

    //int ret = system(cmd.c_str());
    if (!executeRouteCommand(cmd))
    {
        return false;
    }

    system("ip route flush cache");
    // CPlatformManager::instance()->change_net_relogin();
    // usleep(50000);
    // const int max_retries = 3;
    // for (int i = 0; i < max_retries; ++i)
    dlog_info("[路由] 已切换到 eth0，gateway=%s", ethGateway.c_str());

    if (CPlatformManager::instance()->change_net_relogin() != OK)
    {
        // int ret = CPlatformManager::instance()->change_net_relogin();
        // if (ret == 0)
        // {
        //     break;
        // }
        // else
        // {
        //     if (i < max_retries - 1)
        //     {
        //         dlog_error("，准备进行第 (%d)  次重试...", (i + 1));
        //     }
        //     else
        //     {
        //         dlog_error("，已达最大重试次数，放弃执行。");
        //     }
        // }
        dlog_warn("[路由] eth0 已切换，平台立即重登失败，等待后台重试");
    }
    //return (ret == 0);
    return true;
}
#endif
