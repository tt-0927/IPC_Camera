#include "dlog.h"
#if CAP_NETWORK_4G
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
#include <algorithm> 
#include "network_convert.h"
#include "convert_interface.h"
#include "path_define.h"
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
    #if CAP_IO_EXTERNAL_DDR_00S
    system("bspmm 0x11130010 0x1102");/*io复用*/
    system("bspmm 0x11130014 0x1102");
    #else
    system("bspmm 0x11130048 0x1105");/*io复用*/
    system("bspmm 0x1113004c 0x1105");
    #endif

    if (Convert::read_file(NETWORK_4G_CONFIG_FILE, m_config) != OK)
    {
        dlog_info("[4G] No saved configuration, using defaults (enabled=false)");
    }
    else
    {
        dlog_info("[4G] Configuration loaded, enabled=%d", m_config.enabled ? 1 : 0);
    }
    std::cout << "[4G] Module Initialized with APN: " << m_config.apn << std::endl;
}

FourGManager::~FourGManager() {
    deinit();
}

bool FourGManager::waitModuleReady()
{
    for (int i = 0; i < 30; i++)
    {
        std::string resp;

        if (sendCommand("AT", resp, 100) == RET_OK)
        {
            return true;
        }
        std::cout << "sleep sec :" << i << std::endl;
        sleep(1);
    }

    return false;
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

    if (!waitModuleReady())//等待4G模块启动完成
    {
        return RET_ERR_FAILURE;
    }

    // 2. 基础通信测试
    std::string resp;
    if (sendCommand("AT", resp, 700) != RET_OK || resp.find("OK") == std::string::npos) {
        std::cerr << "[4G] Module not responding." << std::endl;
        init_status = RET_ERR_FAILURE;
        return init_status;
    }
    sendCommand("AT+CMEE=1", resp, 700);

    std::string cpinResp;
    if (sendCommand("AT+CPIN?", cpinResp, 700) == RET_OK) {
        // 如果返回了 READY，说明已插卡且正常
        if (cpinResp.find("+CPIN: READY") != std::string::npos) {
            std::cout << "[4G] SIM card is inserted and ready." << std::endl;
        } 
        // 如果返回了 10 号错误码，说明没插卡
        else if (cpinResp.find("+CME ERROR: 10") != std::string::npos) {
            std::cerr << "[4G] Error: SIM card not inserted!" << std::endl;
            init_status = RET_ERR_FAILURE;
            return init_status;//确认卡正常才算初始化成功
        }
    }
    // 3. 自动识别运营商并设置 APN
    autoDetectOperator();

    // 4. 系统网络配置 (对应原构造函数逻辑)
    runSystemCommand("ifconfig usb0 up");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // runSystemCommand("udhcpc -i usb0 -q");
    runSystemCommand("udhcpc -i usb0 -R -n -t 5");

    int eth0_link_up =
    system("grep -q 1 /sys/class/net/eth0/carrier");//查看有线是否存在
    if (eth0_link_up == 0)
    {
        system("ip route del default dev usb0");
    }
    // --- 标记初始化成功 ---
    init_status = RET_OK;
    is_initialized = true; // 关键：设置标记

    
    // m_routeMonitorThread = std::thread(&FourGManager::routeMonitorThread, this);


    // m_routeMonitorThread.detach(); 


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
    if (fd >= 0) {
        dlog_debug("Port is open")
        return RET_OK;
    }
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
    std::lock_guard<std::mutex> lock(port_mutex); 
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
// --- 获取 IMEI (设备识别码) ---
// AT+GSN 或 AT+CGSN=1
    if (sendCommand("AT+CGSN", resp, 1000) == RET_OK) { 
        // 提取数字部分
        std::string temp_imei;
        for(char c : resp) {
            if(isdigit(c)) temp_imei += c;
        }
        if (temp_imei.length() >= 14 && temp_imei.length() <= 16) { // IMEI通常为15位
            info.imei = temp_imei;
        }
    }

    // --- 获取 ICCID (SIM卡识别码) ---
    // AT+CCID 或 AT+ICCID
    if (sendCommand("AT+ICCID", resp, 1000) == RET_OK) { 
        // 不同模组返回格式不同，这里做简单的字符串提取逻辑
        // 通常格式为: +CCID: "898604A..."
        size_t start = resp.find("\"");
        size_t end = resp.rfind("\"");
        if (start != std::string::npos && end != std::string::npos && start != end) {
            info.iccid = resp.substr(start + 1, end - start - 1);
        } else {
            // 如果没有引号，直接提取连续的数字串（ICCID通常很长，超过19位）
            std::string temp_iccid;
            for(char c : resp) {
                if(isdigit(c)) temp_iccid += c;
            }
            if (temp_iccid.length() >= 19) { // ICCID通常为19或20位
                info.iccid = temp_iccid;
            }
        }
    }
    info.network_type = "Unknown"; // 默认值
    
    //类型
    if (sendCommand("AT+COPS?", resp, 1000) == RET_OK) {
        // 1. 找到最后一个逗号的位置
        size_t last_comma_pos = resp.rfind(',');
        if (last_comma_pos != std::string::npos) {
            // 2. 提取逗号后面的子字符串，即 "7"
            std::string act_str = resp.substr(last_comma_pos + 1);
            // 3. 将字符串转换为整数
            int act = std::stoi(act_str);
            
            // 4. 根据 3GPP 标准判断网络类型
            if (act == 7 || act == 9) { 
                info.network_type = "4G";
            } else if (act == 5 || act == 2 || act == 6) { 
                info.network_type = "3G";
            } else if (act == 0 || act == 1 || act == 3) { 
                info.network_type = "2G";
            }
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

    info.set_config_ret = m_config; 

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

    // 保存配置
    m_config = config;

    // ==========================================
    // 1. 设置网络模式 (修复：先断网再设置)
    // ==========================================
    int at_mode = 2; // 默认 Auto
    if (config.network_mode == 1) {
        at_mode = 38; // 4G优先
    } else if (config.network_mode == 2) {
        at_mode = 14;  // 仅3G (WCDMA)
    }

    // 【关键修复】：在修改网络模式前，必须先断开数据连接 (CGACT=0)
    // 否则模组会返回 ERROR 或 +CME ERROR: operation not allowed
    std::cout << "[4G] Deactivating PDP before changing network mode..." << std::endl;
    sendCommand("AT+CGACT=0,1", resp, 3000); 
    // 稍微等待一下让网络层释放
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    ret = sendCommand("AT+CNMP=" + std::to_string(at_mode), resp, 2000);
    if (ret != RET_OK) {
        std::cerr << "[4G] CNMP=14 failed, trying Auto (2)..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        sendCommand("AT+CNMP=2", resp, 3000);
        // 这里不直接 return，因为网络模式设置失败不应阻断 APN 配置
    } else {
        std::cout << "[4G] Network Mode set to: " << at_mode << std::endl;
    }

    // ==========================================
    // 2. 设置 APN (AT+CGDCONT)
    // ==========================================
    std::string apn = config.apn;
    if (apn.empty()) {
        apn = "cmnet"; // 默认 APN
    }
    
    std::string cmd_apn = "AT+CGDCONT=1,\"IP\",\"" + apn + "\"";
    ret = sendCommand(cmd_apn.c_str(), resp, 2000);
    if (ret != RET_OK) {
        std::cerr << "[4G] Failed to set APN: " << apn << std::endl;
        // APN 设置失败是严重错误，但我们可以尝试继续
    }

    // ==========================================
    // 3. 设置鉴权方式 (修复：严格判断参数)
    // ==========================================
    // 需求: 0=自动/无, 1=PAP, 2=CHAP
    int auth_type = 0; 
    if (config.auth_mode == 1) auth_type = 1; // PAP
    else if (config.auth_mode == 2) auth_type = 2; // CHAP

    sendCommand("AT+CGAUTH=1,0", resp, 1000); 
    // 【关键修复】：只有当 auth_type 不为 0 时，才发送用户名和密码
    // 如果 auth_type 为 0，必须只发送 "AT+CGAUTH=1,0"，严禁带后续参数
    if (auth_type != 0) {
        std::string cmd_auth = "AT+CGAUTH=1," + std::to_string(auth_type) + 
                               ",\"" + config.username + "\",\"" + config.password + "\"";
        ret = sendCommand(cmd_auth.c_str(), resp, 2000);
        if (ret != RET_OK) {
            std::cerr << "[4G] Failed to set Authentication." << std::endl;
        }
    } else {
        // 显式设置为无鉴权，确保清除之前的配置
        sendCommand("AT+CGAUTH=1,0", resp, 1000);
    }

    // ==========================================
    // 4. 设置 MTU (系统层面)
    // ==========================================
    int mtu = config.mtu;
    if (mtu < 500) mtu = 500;
    if (mtu > 1500) mtu = 1500;
    
    // 注意：这里需要确保 m_net_interface 不为空，例如 "usb0"
    if (!m_net_interface.empty()) {
        std::string cmd_mtu = "ip link set dev " + m_net_interface + " mtu " + std::to_string(mtu);
        std::cout << "[SYS] Executing: " << cmd_mtu << std::endl;
        // 只有在有 root 权限且接口存在时才会成功
        // system(cmd_mtu.c_str()); 
    }

    // ==========================================
    // 5. 保存配置与激活
    // ==========================================
    sendCommand("AT&W", resp, 1000); // 保存配置
    
    // 如果是“自动拨号”模式，立即激活 PDP 上下文
    if (config.dial_mode == 0) { // 0 = Auto
        std::cout << "[4G] Auto-Dial mode detected, activating PDP..." << std::endl;
        
        // 先确保去激活，防止状态异常
        sendCommand("AT+CGACT=0,1", resp, 3000);
        
        // 重新激活
        ret = sendCommand("AT+CGACT=1,1", resp, 15000); // 拨号可能需要较长时间
        if (ret == RET_OK && resp.find("OK") != std::string::npos) {
            std::cout << "[4G] Auto-Dial: Network activated successfully." << std::endl;
        } else {
            std::cerr << "[4G] Auto-Dial: Failed to activate PDP. Resp: " << resp << std::endl;
        }
    }

    if (need_close) {
        closePort();
    }

    std::cout << "[4G] Configuration applied." << std::endl;
    if (Convert::write_file(NETWORK_4G_CONFIG_FILE, m_config) != OK)
    {
        dlog_error("[4G] Failed to save configuration, enabled=%d", m_config.enabled ? 1 : 0);
        return RET_ERR_CONFIG;
    }

    /* getSimInfo() 有短时缓存，设置成功后必须同步配置，避免立即获取时返回旧 enabled。 */
    last_cached_info.set_config_ret = m_config;

    std::cout << "[4G] Configuration applied and saved. enabled=" << m_config.enabled << std::endl;
    return RET_OK;
}

// RetCode FourGManager::setConfig(const ::Network::Network_4G_Config_t &config) {
//     if (init_status != RET_OK) {
//         std::cerr << "[4G] Error: Module not initialized  !" << std::endl;
//         return RET_ERR_CONFIG;
//     }
    
//     bool need_close = false;
//     if (fd < 0) {
//         if (openPort() != RET_OK) {
//             return RET_ERR_OPEN_PORT;
//         }
//         need_close = true;
//     }
    
//     std::string resp;
//     RetCode ret;
//     m_config = config;

    
//     // ==========================================
//     // 3. 设置 APN (AT+CGDCONT)
//     // ==========================================
//     std::string cmd_apn = "AT+CGDCONT=1,\"IP\",\"" + config.apn + "\"";
//     ret = sendCommand(cmd_apn.c_str(), resp, 2000);
//     if (ret != RET_OK) {
//         std::cerr << "[4G] Failed to set APN." << std::endl;
//         return RET_ERR_FAILURE;
//     }

//     // ==========================================
//     // 4. 设置鉴权方式 (AT+CGAUTH) - 修复 ERROR
//     // ==========================================
//     int auth_type = 0;
//     if (config.auth_mode == AUTH_PAP) auth_type = 1;
//     else if (config.auth_mode == AUTH_CHAP) auth_type = 2;
//     else if (config.auth_mode == AUTH_PAP_CHAP) auth_type = 3;

//     // 【修改】优化鉴权设置逻辑：如果不需要用户名密码，只发送前两个参数
//     // 避免发送 AT+CGAUTH=1,0,"","" 这种非法格式
//     if(auth_type != 0) {
//         std::string cmd_auth = "AT+CGAUTH=1," + std::to_string(auth_type) + ",\"" + config.username + "\",\"" + config.password + "\"";
//         ret = sendCommand(cmd_auth.c_str(), resp, 2000);
//         if (ret != RET_OK) {
//             std::cerr << "[4G] Failed to set Authentication." << std::endl;
//             return RET_ERR_FAILURE;
//         }
//     } else {
//         // 只发送 AT+CGAUTH=1,0
//         // 很多模组不接受带空引号的参数，这样发最安全
//         std::string cmd_auth = "AT+CGAUTH=1," + std::to_string(auth_type);
//         ret = sendCommand(cmd_auth.c_str(), resp, 2000);
//         if (ret != RET_OK) {
//             std::cerr << "[4G] Failed to set Authentication (None)." << std::endl;
//             // 这里即使报错也继续，因为很多模组不支持这个指令（默认就是None）
//             // return RET_ERR_FAILURE;
//         }
//     }

//     // ==========================================
//     // 5. 设置 MTU (系统层面)
//     // ==========================================
//     if (config.mtu > 0) {
//         std::string cmd_mtu = "ip link set dev " + m_net_interface + " mtu " + std::to_string(config.mtu);
//         int sys_ret = std::system(cmd_mtu.c_str());
//         if (sys_ret == 0) {
//             std::cout << "[4G] MTU set to " << config.mtu << " on " << m_net_interface << std::endl;
//         } else {
//             std::cout << "[4G] Warning: Failed to set MTU via shell." << std::endl;
//         }
//     }
//     // 设置为 13 (仅 4G) 后，如果你所在的区域没有 4G 信号覆盖，模块将无法联网。
//     std::string cmd_mode = "AT+CNMP=13"; 
//     if(config.network_mode == 1)
//     {
//         cmd_mode = "AT+CNMP=13";
//     }else {
//         cmd_mode = "AT+CNMP=2"; 
//     }
//     ret = sendCommand(cmd_mode, resp, 2000);  
//     if (ret != RET_OK) {
//         std::cerr << "[4G] Failed to set network mode." << std::endl;
//         // 注意：这里不直接返回，继续尝试重启，防止模块处于未知状态
//     } else {
//         std::cout << "[4G] Network mode set to LTE Only." << std::endl;
//     }

//     sendCommand("AT&W", resp, 1000); // 保存设置到模组


//     std::this_thread::sleep_for(std::chrono::milliseconds(500));
//     sendCommand("AT+CGACT=1,1", resp, 5000); 

//     if (need_close) {
//         closePort();
//     }
//     return RET_OK;
// }

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

// 检查 eth0 是否有 IP 地址且链路通
bool FourGManager::isWiredConnected() {
    std::ifstream stateFile("/sys/class/net/eth0/operstate");
    if (stateFile.is_open()) {
        std::string state;
        std::getline(stateFile, state);
        stateFile.close();
        // 如果状态是 "up"，说明物理链路连上了
        if (state.find("up") != std::string::npos) {
            std::ifstream addrFile("/proc/net/fib_trie");
            if (addrFile.is_open()) {
                std::string line;
                // 简单检查是否存在 eth0 的 IP 路由条目 (这里可以简单化，检查 eth0 关键词)
                while (std::getline(addrFile, line)) {
                    if (line.find("eth0") != std::string::npos) {
                        addrFile.close();
                        std::cout << "[4G] Wired network (eth0) is UP and has IP." << std::endl;
                        return true;
                    }
                }
                addrFile.close();
            }
            // 如果只有链路通但没 IP，也可以认为是有线连接（正在获取 IP）
            std::cout << "[4G] Wired network (eth0) Link is UP (maybe getting IP)." << std::endl;
            return true;
        }
    }
    return false;
}

/**
 * 解析 /etc/init.d/S80network 脚本以获取有线网络配置
 * @param outIp 输出参数：存储提取的 IP 地址
 * @param outGateway 输出参数：存储提取的网关地址
 * @return 成功返回 true，失败返回 false
 */
 bool FourGManager::parseNetworkConfig(std::string& outIp, std::string& outGateway) {
    std::ifstream file("/etc/init.d/S80network");
    if (!file.is_open()) {
        std::cerr << "[Config] Failed to open /etc/init.d/S80network" << std::endl;
        return false;
    }

    std::string line;
    bool ipFound = false;
    bool gwFound = false;

    while (std::getline(file, line)) {
        // 1. 查找 IP 地址 (匹配: ifconfig eth0 172.16.25.83 netmask ...)
        if (!ipFound && line.find("ifconfig eth0") != std::string::npos && line.find("netmask") != std::string::npos) {
            std::istringstream iss(line);
            std::string token;
            // 简单的令牌解析，假设格式固定
            // ifconfig(0) eth0(1) IP_ADDR(2) netmask(3) MASK(4)
            int count = 0;
            while (iss >> token) {
                if (count == 2) { // IP 地址通常在 eth0 后面
                    outIp = token;
                    ipFound = true;
                    break;
                }
                count++;
            }
        }

        // 2. 查找网关 (匹配: route add default gw 172.16.25.254 eth0)
        if (!gwFound && line.find("route add default gw") != std::string::npos) {
            std::istringstream iss(line);
            std::string token;
            // 查找 "gw" 后面的那个字符串
            while (iss >> token) {
                if (token == "gw") {
                    if (iss >> token) {
                        outGateway = token;
                        gwFound = true;
                    }
                    break;
                }
            }
        }
        
        // 如果都找到了，提前退出循环
        if (ipFound && gwFound) break;
    }

    file.close();
    return (ipFound && gwFound);
}

std::string getUsb0Gateway()
{
    FILE *fp = popen("ifconfig usb0 | grep 'inet addr' "
                     "| awk -F: '{print $2}' "
                     "| awk '{print $1}'",
                     "r");

    if (!fp)
    {
        return "";
    }

    char buf[64] = { 0 };

    if (!fgets(buf, sizeof(buf), fp))
    {
        pclose(fp);
        return "";
    }

    pclose(fp);

    std::string ip(buf);

    ip.erase(std::remove(ip.begin(), ip.end(), '\n'), ip.end());

    size_t pos = ip.rfind('.');

    if (pos == std::string::npos)
    {
        return "";
    }

    return ip.substr(0, pos) + ".1";
}
void FourGManager::updateRouteIfNeeded(bool bWiredDisconnected) {
    
    if (m_lastWiredDisconnected == bWiredDisconnected)
    {
        dlog_info("bWiredDisconnected = %d",bWiredDisconnected);
        return;
    }
    dlog_info("bWiredDisconnected = %d",bWiredDisconnected);
    m_lastWiredDisconnected = bWiredDisconnected;

    std::string ipResp;
    bool fourgReady =
        (sendCommand("AT+CGPADDR=1", ipResp, 1000) == RET_OK) &&
        (ipResp.find("0.0.0.0") == std::string::npos);

        if (!bWiredDisconnected)
        {
            // =========================
            // 网线恢复
            // =========================
    
            std::string wiredIp;
            std::string wiredGateway;
    
            if (!parseNetworkConfig(wiredIp, wiredGateway))
            {
                dlog_error("parseNetworkConfig failed");
                return;
            }
    
            dlog_info("switch route to eth0, gateway=%s",
                      wiredGateway.c_str());
    
            // 删除4G默认路由
            system("ip route del default dev usb0 2>/dev/null");
    
            // 删除旧有线路由
            system("ip route del default dev eth0 2>/dev/null");
    
            // 添加有线默认路由
            std::string cmd =
                "ip route add default via " +
                wiredGateway +
                " dev eth0";
    
            system(cmd.c_str());
    
            system("ip route flush cache");
    
            system("ip route");
    
            dlog_info("switch route to eth0");
            usleep(50000);
            const int max_retries = 3; 
            for (int i = 0; i < max_retries; ++i) {
                int ret = CPlatformManager::instance()->change_net_relogin();
                if (ret == 0) {
                    break; 
                } else {
                    if (i < max_retries - 1) {
                        dlog_error("，准备进行第 (%d)  次重试..." ,(i+1));
                    } else {
                        dlog_error("，已达最大重试次数，放弃执行。");
                    }
                }
            }
        }
        else
        {
            // =========================
            // 网线断开
            // =========================
            if (!fourgReady)
            {
                dlog_warn("4G not ready");
                return;
            }
            dlog_info("switch route to 4G");
            std::string gateway = getUsb0Gateway();

            if (gateway.empty())
            {
                dlog_error("usb0 gateway not found");
                return;
            }

            system("ip route del default dev eth0 2>/dev/null");
            system("ip route del default dev usb0 2>/dev/null");

            std::string cmd = "ip route add default via " + gateway + " dev usb0";

            dlog_info("%s", cmd.c_str());

            system(cmd.c_str());

            system("ip route flush cache");
            usleep(500000);
            // CPlatformManager::instance()->change_net_relogin();
            const int max_retries = 3; 
            for (int i = 0; i < max_retries; ++i) {
                int ret = CPlatformManager::instance()->change_net_relogin();
                if (ret == 0) {
                    break; 
                } else {
                    if (i < max_retries - 1) {
                        dlog_error("，准备进行第 (%d)  次重试..." ,(i+1));
                    } else {
                        dlog_error("，已达最大重试次数，放弃执行。");
                    }
                }
            }
        }
    
}
void* FourGManager::routeMonitorThread(void* arg) {
    // FourGManager* pThis = static_cast<FourGManager*>(arg);
    // while (pThis->is_initialized) {
    //     pThis->updateRouteIfNeeded();
    //     // 每隔 5 秒检测一次
    //     std::this_thread::sleep_for(std::chrono::seconds(5));
    // }
    return nullptr;
}
#endif