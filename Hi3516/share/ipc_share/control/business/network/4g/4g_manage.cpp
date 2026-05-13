#ifdef CAP_NETWORK_4G
#include "4g_manage.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sstream>
#include <cstdlib>  // 用于 system()
#include <iostream>
#include <thread>   // 用于 sleep
#include <chrono>
#include <fstream>      // 用于读取文件
#include <sstream>      // 用于字符串流
#include <arpa/inet.h>


#define DEFAULT_PORT "/dev/ttyAMA2"
#define BAUDRATE 115200



void runSystemCommand(const std::string& cmd) {
    std::cout << "[SYS] >>> " << cmd << std::endl;
    int ret = system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "[SYS] Command failed with code: " << ret << std::endl;
    }
}
// ==========================================
// 自动识别逻辑实现
// ==========================================

// 解析 IMSI 识别运营商
Operator_Type FourGManager::parseOperatorFromImsi(const std::string& imsi) {
    if (imsi.length() < 6) return OPERATOR_UNKNOWN;
    
    // 提取前6位 MCC+MNC
    std::string code = imsi.substr(0, 6);
    
    // 中国电信: 46003, 46011, 46005
    if (code == "46003" || code == "46011" || code == "46005") {
        return OPERATOR_CHINA_TELECOM;
    }
    // 中国移动: 46000, 46002, 46007, 46008
    else if (code == "46000" || code == "46002" || code == "46007" || code == "46008") {
        return OPERATOR_CHINA_MOBILE;
    }
    // 中国联通: 46001, 46006, 46009
    else if (code == "46001" || code == "46006" || code == "46009") {
        return OPERATOR_CHINA_UNICOM;
    }
    
    return OPERATOR_UNKNOWN;
}

// 获取对应运营商的 APN
std::string FourGManager::getApnForOperator(Operator_Type op) {
    switch (op) {
        case OPERATOR_CHINA_MOBILE: return "cmnet";
        case OPERATOR_CHINA_UNICOM: return "3gnet";
        case OPERATOR_CHINA_TELECOM: return "ctnet";
        default: return "cmnet"; // 默认 fallback 到移动
    }
}

// 自动检测并配置
void FourGManager::autoDetectOperator() {
    std::string resp;
    // 1. 发送 AT+CIMI 获取 IMSI
    if (sendCommand("AT+CIMI", resp, 1000) == RET_OK) {
        
        // 简单提取数字串
        std::string imsi;
        for(char c : resp) {
            if(isdigit(c)) imsi += c;
        }

        if (imsi.length() >= 6) {
            Operator_Type op = parseOperatorFromImsi(imsi);
            std::string apn = getApnForOperator(op);
            
            m_config.apn = apn;
            std::cout << "[4G] Auto-detected Operator: " << op << ", Setting APN: " << apn << std::endl;
        }
    }
}

// ==========================================
// 构造函数
// ==========================================

FourGManager::FourGManager() : port_name(DEFAULT_PORT), fd(-1), init_status(RET_ERR_FAILURE),is_initialized(false) {
    // 1. 默认配置初始化
    // m_config.apn = ""; 
    // m_config.username = "";
    // m_config.password = "";
    // m_config.network_mode = 1;
    // m_net_interface = "usb0"; 

    // std::cout << "[4G] Initializing module..." << std::endl;

    // if (openPort() != RET_OK) {
    //     std::cerr << "[4G] Failed to open port." << std::endl;
    //     init_status = RET_ERR_OPEN_PORT;
    //     return;
    // }

    // std::string resp;
    // if (sendCommand("AT", resp, 1000) != RET_OK || resp.find("OK") == std::string::npos) {
    //     std::cerr << "[4G] Module not responding." << std::endl;
    //     init_status = RET_ERR_FAILURE;
    //     return;
    // }

    // sendCommand("AT+CMEE=1", resp, 1000);
    

    // // 2. 自动识别运营商并设置 APN
    // autoDetectOperator();

    // runSystemCommand("ifconfig usb0 up");
    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // runSystemCommand("udhcpc -i usb0 -q");
    // init_status = RET_OK;
    std::cout << "[4G] Module Initialized with APN: " << m_config.apn << std::endl;
}

FourGManager::~FourGManager() {
    deinit();
}

RetCode FourGManager::init() {
    // --- 关键：防重入判断 ---
    if (is_initialized) {
        std::cout << "[4G] Already initialized. Skipping..." << std::endl;
        return RET_OK;
    }

    std::cout << "[4G] Starting initialization..." << std::endl;
    
    // 1. 打开端口
    if (openPort() != RET_OK) {
        std::cerr << "[4G] Failed to open port." << std::endl;
        init_status = RET_ERR_OPEN_PORT;
        return init_status;
    }

    // 2. 基础通信测试
    std::string resp;
    if (sendCommand("AT", resp, 1000) != RET_OK || resp.find("OK") == std::string::npos) {
        std::cerr << "[4G] Module not responding." << std::endl;
        init_status = RET_ERR_FAILURE;
        return init_status;
    }
    sendCommand("AT+CMEE=1", resp, 1000);

    // 3. 自动识别运营商并设置 APN
    autoDetectOperator();

    // 4. 系统网络配置 (对应原构造函数逻辑)
    runSystemCommand("ifconfig usb0 up");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    runSystemCommand("udhcpc -i usb0 -q");

    // --- 标记初始化成功 ---
    init_status = RET_OK;
    is_initialized = true; // 关键：设置标记
    std::cout << "[4G] Module Initialized with APN: " << m_config.apn << std::endl;
    
    return RET_OK;
}

// ==========================================
// 3. 新增 deinit 函数 (反初始化)
// ==========================================
void FourGManager::deinit() {
    // 加锁防止多线程同时操作（如果适用）
    // std::lock_guard<std::mutex> lock(mutex_);
    
    if (!is_initialized) {
        return; // 如果未初始化，直接返回
    }

    std::cout << "[4G] Deinitializing module....." << std::endl;

    // 1. 关闭串口
    closePort(); 

    // 2. (可选) 关闭网卡
    // runSystemCommand("ifconfig usb0 down");

    // 3. 重置状态变量
    is_initialized = false;
    init_status = RET_ERR_FAILURE;
    
    std::cout << "[4G] Module deinitialized." << std::endl;
}


// ==========================================
// 串口操作
// ==========================================

RetCode FourGManager::openPort() {
    fd = open(port_name.c_str(), O_RDWR | O_NOCTTY); 
    if (fd < 0) {
        perror("Open port error");
        return RET_ERR_OPEN_PORT;
    }

    struct termios options;
    // 1. 获取当前属性
    if (tcgetattr(fd, &options) != 0) {
        perror("Get attr error");
        return RET_ERR_FAILURE;
    }

    // 2. 【关键】设置波特率
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    // 3. 【关键】设置控制模式 (8N1)
    options.c_cflag |= (CLOCAL | CREAD); // 忽略调制解调器控制线，启用接收
    options.c_cflag &= ~CSIZE;           // 清除数据位掩码
    options.c_cflag |= CS8;              // 8位数据位
    options.c_cflag &= ~PARENB;          // 无校验位
    options.c_cflag &= ~CSTOPB;          // 1位停止位

    // 4. 【关键】关闭硬件流控 (CRTSCTS) - 这是最可能的凶手！
    options.c_cflag &= ~CRTSCTS;

    // 5. 【关键】设置原始模式 (Raw Mode) - 模拟 hiserial_demo
    // 关闭规范模式、回显、信号处理等
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    // 关闭输出处理
    options.c_oflag &= ~OPOST;
    // 关闭软件流控 (IXON/IXOFF) 和 特殊字符处理
    options.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INLCR);

    // 6. 【关键】设置 VMIN/VTIME
    options.c_cc[VMIN] = 0;   // 不等待最小字符数
    options.c_cc[VTIME] = 10; // 1秒超时 (单位0.1秒)

    // 7. 应用设置并清空缓冲区
    tcflush(fd, TCIFLUSH);
    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        perror("Set attr error");
        return RET_ERR_FAILURE;
    }
    
    std::cout << "[4G] Port configured successfully." << std::endl;
    return RET_OK;
}
void FourGManager::closePort() {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}
RetCode FourGManager::sendCommand(const std::string cmd, std::string &response, int timeout_ms) {
    if (fd < 0) return RET_ERR_OPEN_PORT;
    
    response.clear();
    
    // 1. 发送指令 (必须带 \r\n)
    std::string full_cmd = cmd + "\r\n";
    int len = write(fd, full_cmd.c_str(), full_cmd.length());
    if (len < 0) {
        std::cerr << "[4G] Write error!" << std::endl;
        return RET_ERR_FAILURE;
    }
    std::cout << "[TX] >>> " << cmd << std::endl; 
    // 2. 核心修正：增加初始化延时
    // 给模组一点反应时间，不要立刻去读，避免读到空或者干扰
    usleep(50000); // 先睡 50ms

    char buffer[256];
    int total_elapsed = 0;
    
    // 3. 循环读取
    while (total_elapsed < timeout_ms) {
        memset(buffer, 0, sizeof(buffer));
        
        int bytes = read(fd, buffer, sizeof(buffer) - 1);
        
        if (bytes > 0) {
            response += buffer;
            // 调试打印
             std::cout << "[RX] <<< " << buffer << std::endl; 
            
            // 【关键修正】检查是否收到结束符
            if (response.find("OK") != std::string::npos || 
                response.find("ERROR") != std::string::npos ||
                response.find("FAIL") != std::string::npos) {
                return RET_OK; // 收到结果，立即返回成功
            }
        }
        
        // 4. 等待一小段时间再轮询
        usleep(20000); // 20ms
        total_elapsed += 20;
    }
    
    // 5. 超时处理
    std::cout << "[4G] Timeout! Received: " << response << std::endl;
    return RET_ERR_FAILURE;
}

// ==========================================
// 业务逻辑
// ==========================================

RetCode FourGManager::getSimInfo(::Network::SIM_Info_t &info) {
    if (init_status != RET_OK) return init_status;
    std::string resp;
    
    auto now = std::chrono::steady_clock::now();
    // 3. 计算距离上次查询的时间差
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_query_time).count();
    // 4. 防抖逻辑：如果间隔时间未到，直接返回缓存数据
    if (duration < UPDATE_INTERVAL_MS) {
        info = last_cached_info; 
        return RET_OK; 
    }
    last_query_time = now;


    info.is_registered = false;
    info.status = "未连接";
    info.subnet_mask = "255.255.255.0";
    info.gateway = "0.0.0.0";
    info.dns_address = "0.0.0.0";

    // 信号
    if (sendCommand("AT+CSQ", resp, 1000) == RET_OK) {
        size_t pos = resp.find("+CSQ:");
        if (pos != std::string::npos) info.signal_quality = std::to_string(atoi(resp.substr(pos + 5).c_str()));
    }

    // 注册状态
    if (sendCommand("AT+CEREG?", resp, 1000) == RET_OK) {
        if (resp.find("+CEREG: 0,1") != std::string::npos || resp.find("+CEREG: 0,5") != std::string::npos) {
            info.is_registered = true;
        }
    }

    // 运营商名字 (AT+COPS?)
    if (sendCommand("AT+COPS?", resp, 2000) == RET_OK) {
        size_t start = resp.find("\"");
        size_t end = resp.rfind("\"");
        if (start != std::string::npos && end != std::string::npos && start != end) {
            info.operator_name = resp.substr(start + 1, end - start - 1);
        }
    }

    // IP
    if (sendCommand("AT+CGPADDR=1", resp, 1000) == RET_OK) {
        size_t pos = resp.find("+CGPADDR:");
        if (pos != std::string::npos) {
            size_t ip_start = resp.find("\"", pos);
            size_t ip_end = resp.find("\"", ip_start + 1);
            if (ip_start != std::string::npos && ip_end != std::string::npos) {
                info.ip_address = resp.substr(ip_start + 1, ip_end - ip_start - 1);
                if (!info.ip_address.empty()) info.status = "已连接";
            }
        }
    }
     // 获取子网掩码
    if (sendCommand("AT+CGNETMASK?", resp, 1000) == RET_OK) {
        // 假设返回格式类似 +CGNETMASK: 1,"255.255.255.0"
        size_t mask_pos = resp.find("+CGNETMASK:");
        if (mask_pos != std::string::npos) {
            size_t mask_start = resp.find("\"", mask_pos);
            size_t mask_end = resp.find("\"", mask_start + 1);
            if (mask_start != std::string::npos && mask_end != std::string::npos) {
                info.subnet_mask = resp.substr(mask_start + 1, mask_end - mask_start - 1);
            }
        }
    }


    std::ifstream routeFile("/proc/net/route");
    if (routeFile.is_open()) {
        std::string line;
        while (std::getline(routeFile, line)) {
            // 跳过表头
            if (line.find("Iface") != std::string::npos) continue;
            
            // 查找默认路由 (00000000)
            if (line.find("00000000") != std::string::npos) {
                std::istringstream iss(line);
                std::string iface, dest, gateway_hex;
                iss >> iface >> dest >> gateway_hex;
            
                unsigned long gw_ulong = std::stoul(gateway_hex, nullptr, 16);
                struct in_addr gw_addr;
                gw_addr.s_addr = gw_ulong;
                info.gateway = inet_ntoa(gw_addr);
                break; // 找到第一个默认路由即可
            }
        }
        routeFile.close();
    }

    std::ifstream resolvFile("/etc/resolv.conf");
    if (resolvFile.is_open()) {
        std::string line;
        while (std::getline(resolvFile, line)) {
            if (line.find("nameserver") != std::string::npos) {
                std::istringstream iss(line);
                std::string type, dns_ip;
                iss >> type >> dns_ip;
                if (!dns_ip.empty()) {
                    
                    info.dns_address = dns_ip; 
                    break; 
                }
            }
        }
        resolvFile.close();
    }

    last_cached_info = info;
    return RET_OK;
}




RetCode FourGManager::setConfig(const ::Network::Network_4G_Config_t &config) {
    if (init_status != RET_OK) {
        std::cerr << "[4G] Error: Module not initialized!" << std::endl;
        return RET_ERR_CONFIG;
    }
    
    bool need_close = false;
    if (fd < 0) {
        if (openPort() != RET_OK) {
            return RET_ERR_OPEN_PORT;
        }
        need_close = true;
    }
    
    std::string resp;
    RetCode ret;
    m_config = config;

    // ==========================================
    // 2. 设置网络模式 (根据网页上的 "网络切换方式")
    // ==========================================
    // 
    // if (config.network_mode == 1) {
    //     ret = sendCommand("AT+QNWPREFCFG=\"mode_pref\",LTE", resp, 2000);
    //     if (ret != RET_OK) {
    //         std::cerr << "[4G] Failed to set Network Mode to 4G." << std::endl;
    //     }
    // } else if (config.network_mode == 0) {
    //     ret = sendCommand("AT+QNWPREFCFG=\"mode_pref\",AUTO", resp, 2000);
    //     if (ret != RET_OK) {
    //         std::cerr << "[4G] Failed to set Network Mode to Auto." << std::endl;
    //     }
    // }
    // 上面的代码注释掉，因为模组不支持，会报错，但不影响基本功能
    
    // ==========================================
    // 3. 设置 APN (AT+CGDCONT)
    // ==========================================
    std::string cmd_apn = "AT+CGDCONT=1,\"IP\",\"" + config.apn + "\"";
    ret = sendCommand(cmd_apn.c_str(), resp, 2000);
    if (ret != RET_OK) {
        std::cerr << "[4G] Failed to set APN." << std::endl;
        return RET_ERR_FAILURE;
    }

    // ==========================================
    // 4. 设置鉴权方式 (AT+CGAUTH) - 修复 ERROR
    // ==========================================
    int auth_type = 0;
    if (config.auth_mode == AUTH_PAP) auth_type = 1;
    else if (config.auth_mode == AUTH_CHAP) auth_type = 2;
    else if (config.auth_mode == AUTH_PAP_CHAP) auth_type = 3;

    // 【修改】优化鉴权设置逻辑：如果不需要用户名密码，只发送前两个参数
    // 避免发送 AT+CGAUTH=1,0,"","" 这种非法格式
    if(auth_type != 0) {
        std::string cmd_auth = "AT+CGAUTH=1," + std::to_string(auth_type) + ",\"" + config.username + "\",\"" + config.password + "\"";
        ret = sendCommand(cmd_auth.c_str(), resp, 2000);
        if (ret != RET_OK) {
            std::cerr << "[4G] Failed to set Authentication." << std::endl;
            return RET_ERR_FAILURE;
        }
    } else {
        // 只发送 AT+CGAUTH=1,0
        // 很多模组不接受带空引号的参数，这样发最安全
        std::string cmd_auth = "AT+CGAUTH=1," + std::to_string(auth_type);
        ret = sendCommand(cmd_auth.c_str(), resp, 2000);
        if (ret != RET_OK) {
            std::cerr << "[4G] Failed to set Authentication (None)." << std::endl;
            // 这里即使报错也继续，因为很多模组不支持这个指令（默认就是None）
            // return RET_ERR_FAILURE;
        }
    }

    // ==========================================
    // 5. 设置 MTU (系统层面)
    // ==========================================
    if (config.mtu > 0) {
        std::string cmd_mtu = "ip link set dev " + m_net_interface + " mtu " + std::to_string(config.mtu);
        int sys_ret = std::system(cmd_mtu.c_str());
        if (sys_ret == 0) {
            std::cout << "[4G] MTU set to " << config.mtu << " on " << m_net_interface << std::endl;
        } else {
            std::cout << "[4G] Warning: Failed to set MTU via shell." << std::endl;
        }
    }

    // ==========================================
    // 6. 保存配置并重启生效
    // ==========================================
    sendCommand("AT&W", resp, 1000); // 保存设置到模组

    // 【关键修改】注释掉重启指令！
    // sendCommand("AT+CFUN=1,1", resp, 1000); // 注释这行！防止 USB 掉线重连
    // std::this_thread::sleep_for(std::chrono::seconds(10)); // 注释这行！

    // 不重启，直接激活
    // 稍微等一下保存完成
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 尝试激活 PDP，这会触发模组分配 IP
    // 如果之前已经激活过，这步会很快返回 OK
    sendCommand("AT+CGACT=1,1", resp, 5000); 

    if (need_close) {
        closePort();
    }
    return RET_OK;
}

RetCode FourGManager::connect() {
    if (init_status != RET_OK) return init_status;
    std::string resp;
    
    // 确保 APN 已设置（双重保险）
    if (m_config.apn.empty()) {
        m_config.apn = "cmnet"; 
    }
    
    // 如果还没设置进模块，先设置一次
    std::string check_cmd = "AT+CGDCONT=1,\"IP\",\"" + m_config.apn + "\"";
    sendCommand(check_cmd, resp, 2000);

    RetCode ret = sendCommand("AT+CGACT=1,1", resp, 5000);
    if (ret == RET_OK) {
        if (resp.find("CONNECT") != std::string::npos) {
            std::cout << "[4G] Connected." << std::endl;
            closePort();
            return RET_OK;
        } else if (resp.find("OK") != std::string::npos) {
            return RET_OK;
        }
    }
    return RET_ERR_FAILURE;
}
#endif