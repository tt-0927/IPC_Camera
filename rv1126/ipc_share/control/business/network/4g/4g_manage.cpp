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
#include <cstdio>
#include <cctype>
#include <vector>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <cerrno>
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

static bool isSafeAtArgument(const std::string& value, size_t max_length) {
    if (value.length() > max_length) return false;
    for (char c : value) {
        if (c == '"' || c == '\r' || c == '\n' ||
            static_cast<unsigned char>(c) < 0x20) {
            return false;
        }
    }
    return true;
}

static std::string longestDigitRun(const std::string& text) {
    std::string longest;
    std::string current;
    for (char c : text) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            current += c;
        } else {
            if (current.length() > longest.length()) longest = current;
            current.clear();
        }
    }
    if (current.length() > longest.length()) longest = current;
    return longest;
}

static bool getInterfaceIpv4(const std::string& interface_name,
                             std::string& address,
                             std::string& netmask) {
    struct ifaddrs* addresses = nullptr;
    if (getifaddrs(&addresses) != 0) return false;
    bool found = false;
    for (struct ifaddrs* item = addresses; item != nullptr; item = item->ifa_next) {
        if (!item->ifa_addr || !item->ifa_name || interface_name != item->ifa_name ||
            item->ifa_addr->sa_family != AF_INET) continue;
        char ip_text[INET_ADDRSTRLEN] = {0};
        char mask_text[INET_ADDRSTRLEN] = {0};
        const struct sockaddr_in* ip = reinterpret_cast<const struct sockaddr_in*>(item->ifa_addr);
        const struct sockaddr_in* mask = reinterpret_cast<const struct sockaddr_in*>(item->ifa_netmask);
        if (inet_ntop(AF_INET, &ip->sin_addr, ip_text, sizeof(ip_text))) {
            address = ip_text;
            found = true;
        }
        if (mask && inet_ntop(AF_INET, &mask->sin_addr, mask_text, sizeof(mask_text)))
            netmask = mask_text;
        break;
    }
    freeifaddrs(addresses);
    return found;
}

static bool isValidIpv4(const std::string& address) {
    struct in_addr parsed;
    return !address.empty() && inet_pton(AF_INET, address.c_str(), &parsed) == 1;
}

static bool connectByInterface(const std::string& interface_name,
                               const char* target_ip,
                               unsigned short target_port,
                               int timeout_ms) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) return false;

    if (setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE,
                   interface_name.c_str(), interface_name.length() + 1) != 0) {
        close(socket_fd);
        return false;
    }

    int old_flags = fcntl(socket_fd, F_GETFL, 0);
    if (old_flags < 0 || fcntl(socket_fd, F_SETFL, old_flags | O_NONBLOCK) != 0) {
        close(socket_fd);
        return false;
    }

    struct sockaddr_in target;
    std::memset(&target, 0, sizeof(target));
    target.sin_family = AF_INET;
    target.sin_port = htons(target_port);
    if (inet_pton(AF_INET, target_ip, &target.sin_addr) != 1) {
        close(socket_fd);
        return false;
    }

    int result = connect(socket_fd, reinterpret_cast<struct sockaddr*>(&target), sizeof(target));
    if (result != 0 && errno != EINPROGRESS) {
        close(socket_fd);
        return false;
    }

    struct pollfd poll_fd;
    poll_fd.fd = socket_fd;
    poll_fd.events = POLLOUT;
    poll_fd.revents = 0;
    result = poll(&poll_fd, 1, timeout_ms);

    int socket_error = 0;
    socklen_t error_length = sizeof(socket_error);
    bool connected = result > 0 &&
        getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &socket_error, &error_length) == 0 &&
        socket_error == 0;
    close(socket_fd);
    return connected;
}

static bool checkInternetByInterface(const std::string& interface_name) {
    // Each interface uses different /32 probe routes. This allows health
    // checks without keeping two default routes in the main routing table.
    if (interface_name == "eth0") {
        return connectByInterface(interface_name, "223.5.5.5", 53, 1200) ||
               connectByInterface(interface_name, "114.114.114.114", 53, 1200);
    }
    return connectByInterface(interface_name, "1.1.1.1", 53, 1200) ||
           connectByInterface(interface_name, "8.8.8.8", 53, 1200);
}

static bool applyDefaultRoutes(const std::string& eth_gateway,
                               const std::string& usb_gateway,
                               bool prefer_4g) {
    const bool has_eth = isValidIpv4(eth_gateway);
    const bool has_4g = isValidIpv4(usb_gateway);
    if (!has_eth && !has_4g) return false;

    runSystemCommand("ip route del default dev eth0 2>/dev/null");
    runSystemCommand("ip route del default dev usb0 2>/dev/null");

    if (prefer_4g && has_4g)
        runSystemCommand("ip route add default via " + usb_gateway + " dev usb0");
    else if (has_eth)
        runSystemCommand("ip route add default via " + eth_gateway + " dev eth0");
    else if (has_4g)
        runSystemCommand("ip route add default via " + usb_gateway + " dev usb0");

    // Dedicated host routes keep both health checks usable while the main
    // table contains only one default route.
    runSystemCommand("ip route del 223.5.5.5/32 2>/dev/null");
    runSystemCommand("ip route del 114.114.114.114/32 2>/dev/null");
    runSystemCommand("ip route del 1.1.1.1/32 2>/dev/null");
    runSystemCommand("ip route del 8.8.8.8/32 2>/dev/null");
    if (has_eth) {
        runSystemCommand("ip route add 223.5.5.5/32 via " + eth_gateway + " dev eth0");
        runSystemCommand("ip route add 114.114.114.114/32 via " + eth_gateway + " dev eth0");
    }
    if (has_4g) {
        runSystemCommand("ip route add 1.1.1.1/32 via " + usb_gateway + " dev usb0");
        runSystemCommand("ip route add 8.8.8.8/32 via " + usb_gateway + " dev usb0");
    }
    runSystemCommand("ip route flush cache");
    return true;
}

static bool hasDefaultRoute(const std::string& interface_name) {
    std::ifstream route("/proc/net/route");
    std::string line;
    while (std::getline(route, line)) {
        std::istringstream fields(line);
        std::string iface, destination;
        fields >> iface >> destination;
        if (iface == interface_name && destination == "00000000") return true;
    }
    return false;
}
// ==========================================
// 自动识别逻辑实现
// ==========================================

// 解析 IMSI 识别运营商
Operator_Type FourGManager::parseOperatorFromImsi(const std::string& imsi) {
    if (imsi.length() < 5) return OPERATOR_UNKNOWN;
    
    // 提取前6位 MCC+MNC
    std::string code = imsi.substr(0, 5);
    
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
    // Never replace a private-network APN entered on the web page.
    if (!m_config.apn.empty()) return;

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

FourGManager::FourGManager() : port_name(DEFAULT_PORT), fd(-1), init_status(RET_ERR_FAILURE),is_initialized(false), m_net_interface("usb0") {
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
        if (m_config.enabled) connectAsync();
        return init_status;
    }

    if (!waitModuleReady())//等待4G模块启动完成
    {
        init_status = RET_ERR_FAILURE;
        if (m_config.enabled) connectAsync();
        return RET_ERR_FAILURE;
    }

    // 2. 基础通信测试
    std::string resp;
    if (sendCommand("AT", resp, 700) != RET_OK || resp.find("OK") == std::string::npos) {
        std::cerr << "[4G] Module not responding." << std::endl;
        init_status = RET_ERR_FAILURE;
        if (m_config.enabled) connectAsync();
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
            if (m_config.enabled) connectAsync();
            return init_status;//确认卡正常才算初始化成功
        }
    }
    // 3. 自动识别运营商并设置 APN
    // Only infer a public APN when no APN has been configured. Private APNs
    // must remain exactly as supplied by the user.
    autoDetectOperator();

    // 4. 系统网络配置 (对应原构造函数逻辑)
    if (m_config.enabled) {
        runSystemCommand("ifconfig usb0 up");
    } else {
        runSystemCommand("ifconfig usb0 down");
    }

    int eth0_link_up =
    system("grep -q 1 /sys/class/net/eth0/carrier");//查看有线是否存在
    if (eth0_link_up == 0)
    {
        system("ip route del default dev usb0");
    }
    // --- 标记初始化成功 ---
    init_status = RET_OK;
    is_initialized = true; // 关键：设置标记

    if (m_config.enabled) {
        RetCode dial_ret = connect();
        if (dial_ret != RET_OK) {
            dlog_error("[4G] Automatic ECM dial failed during initialization, ret=%d", dial_ret);
            connectAsync();
        }
    }

    
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
    m_4gDataConnected.store(false);
    
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
    tcflush(fd, TCIFLUSH);
    std::string full_cmd = cmd + "\r\n";
    size_t written = 0;
    while (written < full_cmd.length()) {
        ssize_t len = write(fd, full_cmd.data() + written, full_cmd.length() - written);
        if (len <= 0) {
            std::cerr << "[4G] Write error!" << std::endl;
            return RET_ERR_FAILURE;
        }
        written += static_cast<size_t>(len);
    }
    if (cmd.find("AT+ECMDUP=") == 0 || cmd.find("AT^AUTHDATA=") == 0) {
        std::cout << "[TX] >>> " << cmd.substr(0, cmd.find('='))
                  << "=<credentials hidden>" << std::endl;
    } else {
        std::cout << "[TX] >>> " << cmd << std::endl;
    }
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
            if (response.find("ERROR") != std::string::npos ||
                response.find("FAIL") != std::string::npos ||
                response.find("NO CARRIER") != std::string::npos) {
                return RET_ERR_FAILURE;
            }
            if (response.find("\r\nOK\r\n") != std::string::npos ||
                response.find("\nOK\r\n") != std::string::npos ||
                response.find("CONNECT") != std::string::npos) {
                return RET_OK;
            }
        }
        
        // 4. 等待一小段时间再轮询
        usleep(20000); // 20ms
        total_elapsed += 20;
    }
    
    // 5. 超时处理
    std::cout << "[4G] Timeout! Received: " << response << std::endl;
    return RET_ERR_TIMEOUT;
}

// ==========================================
// 业务逻辑
// ==========================================

#if 0
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
    // SLM770A reports address, mask, gateway and DNS through CGCONTRDP.
    if (sendCommand("AT+CGCONTRDP=1", resp, 3000) == RET_OK) {
        size_t contrdp = resp.find("+CGCONTRDP:");
        size_t first_quote = contrdp == std::string::npos ? std::string::npos : resp.find('"', contrdp);
        size_t first_end = first_quote == std::string::npos ? std::string::npos : resp.find('"', first_quote + 1);
        if (first_quote != std::string::npos && first_end != std::string::npos) {
            std::string local_and_mask = resp.substr(first_quote + 1, first_end - first_quote - 1);
            size_t split = 0;
            for (int dots = 0; dots < 4 && split != std::string::npos; ++dots) {
                split = local_and_mask.find('.', split + 1);
            }
            if (split != std::string::npos) {
                info.ip_address = local_and_mask.substr(0, split);
                info.subnet_mask = local_and_mask.substr(split + 1);
            }

            size_t gateway_start = resp.find('"', first_end + 1);
            size_t gateway_end = gateway_start == std::string::npos ? std::string::npos : resp.find('"', gateway_start + 1);
            if (gateway_start != std::string::npos && gateway_end != std::string::npos)
                info.gateway = resp.substr(gateway_start + 1, gateway_end - gateway_start - 1);

            size_t dns_start = gateway_end == std::string::npos ? std::string::npos : resp.find('"', gateway_end + 1);
            size_t dns_end = dns_start == std::string::npos ? std::string::npos : resp.find('"', dns_start + 1);
            if (dns_start != std::string::npos && dns_end != std::string::npos)
                info.dns_address = resp.substr(dns_start + 1, dns_end - dns_start - 1);
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


#endif

RetCode FourGManager::getSimInfo(::Network::SIM_Info_t &info) {
    if (init_status != RET_OK) return init_status;

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_query_time).count();
    if (elapsed < UPDATE_INTERVAL_MS) {
        info = last_cached_info;
        return RET_OK;
    }
    last_query_time = now;

    info = ::Network::SIM_Info_t();
    info.set_config_ret = m_config;
    info.status = m_config.enabled ? "未注册" : "未启用";
    info.ip_address.clear();
    info.subnet_mask = "0.0.0.0";
    info.gateway = "0.0.0.0";
    info.dns_address = "0.0.0.0";
    info.signal_quality = "0";
    info.network_type = "Unknown";
    info.is_registered = false;

    std::string resp;
    bool sim_ready = false;
    if (sendCommand("AT+CPIN?", resp, 1500) == RET_OK) {
        sim_ready = resp.find("+CPIN: READY") != std::string::npos;
    }

    int rssi = 99;
    if (sendCommand("AT+CSQ", resp, 1500) == RET_OK) {
        size_t pos = resp.find("+CSQ:");
        if (pos != std::string::npos) {
            rssi = std::atoi(resp.c_str() + pos + 5);
            info.signal_quality = rssi == 99 ? "0" : std::to_string(rssi);
        }
    }

    int registration = -1;
    if (sendCommand("AT+CEREG?", resp, 1500) == RET_OK) {
        size_t pos = resp.find("+CEREG:");
        if (pos != std::string::npos) {
            int report_mode = 0;
            if (std::sscanf(resp.c_str() + pos + 7, " %d,%d", &report_mode, &registration) != 2) {
                std::sscanf(resp.c_str() + pos + 7, " %d", &registration);
            }
            info.is_registered = registration == 1 || registration == 5;
        }
    }

    int access_technology = -1;
    if (sendCommand("AT+COPS?", resp, 2500) == RET_OK) {
        size_t first_quote = resp.find('"');
        size_t second_quote = first_quote == std::string::npos ? std::string::npos : resp.find('"', first_quote + 1);
        if (first_quote != std::string::npos && second_quote != std::string::npos) {
            info.operator_name = resp.substr(first_quote + 1, second_quote - first_quote - 1);
            if (info.operator_name.length() >= 5 &&
                std::all_of(info.operator_name.begin(), info.operator_name.end(),
                            [](char c) { return std::isdigit(static_cast<unsigned char>(c)); })) {
                switch (parseOperatorFromImsi(info.operator_name)) {
                    case OPERATOR_CHINA_MOBILE: info.operator_name = "中国移动"; break;
                    case OPERATOR_CHINA_UNICOM: info.operator_name = "中国联通"; break;
                    case OPERATOR_CHINA_TELECOM: info.operator_name = "中国电信"; break;
                    default: break;
                }
            }
            size_t comma = resp.find(',', second_quote + 1);
            if (comma != std::string::npos) access_technology = std::atoi(resp.c_str() + comma + 1);
        }
    }

    if (info.operator_name.empty() && sendCommand("AT+CIMI", resp, 1500) == RET_OK) {
        std::string imsi;
        for (char c : resp) {
            if (std::isdigit(static_cast<unsigned char>(c))) imsi += c;
        }
        switch (parseOperatorFromImsi(imsi)) {
            case OPERATOR_CHINA_MOBILE: info.operator_name = "中国移动"; break;
            case OPERATOR_CHINA_UNICOM: info.operator_name = "中国联通"; break;
            case OPERATOR_CHINA_TELECOM: info.operator_name = "中国电信"; break;
            default: break;
        }
    }

    if (access_technology == 7 || access_technology == 9) info.network_type = "4G";
    else if (access_technology == 2 || access_technology == 5 || access_technology == 6) info.network_type = "3G";
    else if (access_technology == 0 || access_technology == 1 || access_technology == 3) info.network_type = "2G";
    else if (m_config.network_mode == ::Network::NET_4G_ONLY) info.network_type = "4G";
    else if (m_config.network_mode == ::Network::NET_3G_ONLY) info.network_type = "3G";
    else if (m_config.network_mode == ::Network::NET_2G_ONLY) info.network_type = "2G";

    if (sendCommand("AT+CGSN", resp, 1500) == RET_OK) {
        std::string digits = longestDigitRun(resp);
        if (digits.length() >= 14 && digits.length() <= 16) info.imei = digits;
    }
    if (sendCommand("AT+ICCID", resp, 1500) == RET_OK) {
        std::string digits = longestDigitRun(resp);
        if (digits.length() >= 19 && digits.length() <= 20) info.iccid = digits;
    }

    bool ecm_active = false;
    if (sendCommand("AT+ECMDUP?", resp, 3000) == RET_OK) {
        ecm_active = resp.find("+ECMDUP: 1,1") != std::string::npos;
    }


    // In ECM mode the Linux interface is the authoritative source for the
    // address actually usable by the application.
    struct ifaddrs* addresses = nullptr;
    if (getifaddrs(&addresses) == 0) {
        for (struct ifaddrs* item = addresses; item != nullptr; item = item->ifa_next) {
            if (!item->ifa_addr || !item->ifa_name || m_net_interface != item->ifa_name ||
                item->ifa_addr->sa_family != AF_INET) continue;

            char address[INET_ADDRSTRLEN] = {0};
            char netmask[INET_ADDRSTRLEN] = {0};
            const struct sockaddr_in* ip = reinterpret_cast<const struct sockaddr_in*>(item->ifa_addr);
            const struct sockaddr_in* mask = reinterpret_cast<const struct sockaddr_in*>(item->ifa_netmask);
            if (inet_ntop(AF_INET, &ip->sin_addr, address, sizeof(address))) info.ip_address = address;
            if (mask && inet_ntop(AF_INET, &mask->sin_addr, netmask, sizeof(netmask))) info.subnet_mask = netmask;
            break;
        }
        freeifaddrs(addresses);
    }

    bool module_has_ip = false;
    if (ecm_active && info.is_registered) {
        if (sendCommand("AT+CGCONTRDP=1", resp, 3000) == RET_OK) {
            size_t line = resp.find("+CGCONTRDP:");
            std::vector<std::string> quoted_fields;
            size_t cursor = line;
            while (cursor != std::string::npos) {
                size_t begin = resp.find('"', cursor);
                if (begin == std::string::npos) break;
                size_t end = resp.find('"', begin + 1);
                if (end == std::string::npos) break;
                quoted_fields.push_back(resp.substr(begin + 1, end - begin - 1));
                cursor = end + 1;
            }
            // SLM770A returns: APN, local address, gateway, primary DNS,
            // secondary DNS. Gateway can legitimately be empty in ECM mode.

            if (quoted_fields.size() >= 2 && quoted_fields[1] != "0.0.0.0" &&
                isValidIpv4(quoted_fields[1])) {
                module_has_ip = true;
                // Prefer the Linux ECM address. The cellular PDP address is
                // only a fallback while usb0 is still obtaining its lease.
                if (info.ip_address.empty()) info.ip_address = quoted_fields[1];
            }
            if (quoted_fields.size() >= 3 && !quoted_fields[2].empty())
                info.gateway = quoted_fields[2];
            if (quoted_fields.size() >= 4 && !quoted_fields[3].empty())
                info.dns_address = quoted_fields[3];
            if (quoted_fields.size() >= 5 && info.dns_address == "0.0.0.0" &&
                !quoted_fields[4].empty()) {
                info.dns_address = quoted_fields[4];
            }
        }
    }
    // ECMDUP may stay active for the modem management LAN even without a
    // usable cellular context.  Only CGCONTRDP proves that a PDP address was
    // assigned by the mobile network.
    m_4gDataConnected.store(ecm_active && info.is_registered && module_has_ip);
    // A 6072 query can race with the background DHCP step immediately after
    // ^DCONN. Give usb0 a short window to obtain its host-side lease so the
    // first response can already contain its mask and gateway.
    if (ecm_active && module_has_ip && info.subnet_mask == "0.0.0.0") {
        if (!m_dial_in_progress.load()) {
            runSystemCommand("ifconfig usb0 up");
            runSystemCommand("udhcpc -i usb0 -q -n -t 5");
        }
        for (int retry = 0; retry < 80; ++retry) {
            std::string host_ip;
            std::string host_mask;
            if (getInterfaceIpv4(m_net_interface, host_ip, host_mask)) {
                info.ip_address = host_ip;
                info.subnet_mask = host_mask;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::ifstream route("/proc/net/route");
    std::string route_line;
    while (std::getline(route, route_line)) {
        std::istringstream fields(route_line);
        std::string iface, destination, gateway_hex;
        fields >> iface >> destination >> gateway_hex;
        if (iface != m_net_interface || destination != "00000000") continue;
        try {
            struct in_addr gateway;
            gateway.s_addr = std::stoul(gateway_hex, nullptr, 16);
            info.gateway = inet_ntoa(gateway);
        } catch (...) {}
        break;
    }

    if (!m_config.enabled) info.status = "未启用";
    else if (!sim_ready) info.status = "SIM卡异常";
    else if (ecm_active && (module_has_ip || !info.ip_address.empty())) info.status = "已连接";
    else if (info.is_registered) info.status = "已注册未拨号";
    else if (registration == 2) info.status = "正在注册";
    else if (registration == 3) info.status = "注册被拒绝";
    else if (rssi == 99) info.status = "无信号";
    else info.status = "未注册";

    last_cached_info = info;
    return RET_OK;
}

#if 0
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

#endif

RetCode FourGManager::setConfig(const ::Network::Network_4G_Config_t &config) {
    if (init_status != RET_OK) return RET_ERR_CONFIG;

    if (!isSafeAtArgument(config.apn, 99) ||
        !isSafeAtArgument(config.username, 200) ||
        !isSafeAtArgument(config.password, 200) ||
        config.network_mode < ::Network::NET_AUTO ||
        config.network_mode > ::Network::NET_2G_ONLY ||
        config.dial_mode < ::Network::DIAL_AUTO ||
        config.dial_mode > ::Network::DIAL_MANUAL ||
        config.auth_mode < ::Network::AUTH_NONE ||
        config.auth_mode > ::Network::AUTH_PAP_CHAP) {
        dlog_error("[4G] Invalid web configuration");
        return RET_ERR_CONFIG;
    }

    // The web page sends a second 6073 request after 6072. In that request
    // only enabled is meaningful; all other fields contain UI defaults.
    // Do not let this handshake overwrite the saved APN/private-network
    // configuration or trigger a reconnect on every page refresh.
    // 6073 echoes values returned by 6072. Treat a field as a user change only
    // when it is non-default and differs from the currently saved value.
    const bool has_explicit_parameters =
        (!config.apn.empty() && config.apn != m_config.apn) ||
        (!config.username.empty() && config.username != m_config.username) ||
        (!config.password.empty() && config.password != m_config.password) ||
        (!config.phone_number.empty() && config.phone_number != m_config.phone_number) ||
        (!config.call_number.empty() && config.call_number != "*99#" &&
         config.call_number != m_config.call_number) ||
        (config.mtu != 1500 && config.mtu != m_config.mtu) ||
        (config.auth_mode != ::Network::AUTH_NONE && config.auth_mode != m_config.auth_mode) ||
        (config.network_mode != ::Network::NET_AUTO && config.network_mode != m_config.network_mode) ||
        (config.dial_mode != ::Network::DIAL_AUTO && config.dial_mode != m_config.dial_mode);

    if (!has_explicit_parameters) {
        const bool enabled_changed = m_config.enabled != config.enabled;
        m_config.enabled = config.enabled;

        if (Convert::write_file(NETWORK_4G_CONFIG_FILE, m_config) != OK) {
            dlog_error("[4G] Failed to save enable state");
            return RET_ERR_CONFIG;
        }

        last_query_time = std::chrono::steady_clock::time_point();
        last_cached_info.set_config_ret = m_config;

        if (!enabled_changed) {
            dlog_info("[4G] Enable-only request ignored because state is unchanged: enabled=%d",
                      m_config.enabled ? 1 : 0);
            if (m_config.enabled && last_cached_info.status != "已连接") connectAsync();
            return RET_OK;
        }

        std::string response;
        if (!m_config.enabled) {
            m_4gDataConnected.store(false);
            sendCommand("AT+ECMDUP=1,0", response, 5000);
            runSystemCommand("ifconfig usb0 down");
            dlog_info("[4G] 4G disabled by enable-only request");
            return RET_OK;
        }

        dlog_info("[4G] 4G enabled by enable-only request; saved parameters are preserved");
        connectAsync();
        return RET_OK;
    }

    bool need_close = false;
    if (fd < 0) {
        if (openPort() != RET_OK) return RET_ERR_OPEN_PORT;
        need_close = true;
    }

    const auto finish = [this, need_close]() {
        if (need_close) closePort();
    };
    std::string resp;

    // SLM770A Linux ECM calls are controlled by AT+ECMDUP. ATD*99# is
    // PPP mode and would turn the AT serial port into a data channel.
    sendCommand("AT+ECMDUP=1,0", resp, 5000);
    m_4gDataConnected.store(false);

    const char* rat = "00";
    if (config.network_mode == ::Network::NET_4G_ONLY) rat = "03";
    else if (config.network_mode == ::Network::NET_3G_ONLY) rat = "02";
    else if (config.network_mode == ::Network::NET_2G_ONLY) rat = "01";
    if (sendCommand(std::string("AT^SYSCFGEX=\"") + rat + "\"", resp, 10000) != RET_OK) {
        dlog_error("[4G] AT^SYSCFGEX failed: %s", resp.c_str());
        finish();
        return RET_ERR_CONFIG;
    }

    ::Network::Network_4G_Config_t applied = config;
    if (applied.apn.empty()) {
        m_config = applied;
        autoDetectOperator();
        applied.apn = m_config.apn;
    }
    if (applied.apn.empty()) applied.apn = "cmnet";

    if (sendCommand("AT+CGDCONT=1,\"IP\",\"" + applied.apn + "\"", resp, 3000) != RET_OK) {
        dlog_error("[4G] AT+CGDCONT failed: %s", resp.c_str());
        finish();
        return RET_ERR_CONFIG;
    }

    applied.mtu = std::max(500, std::min(1500, applied.mtu));
    runSystemCommand("ip link set dev usb0 mtu " + std::to_string(applied.mtu));

    m_config = applied;
    if (!applied.enabled) {
        runSystemCommand("ifconfig usb0 down");
    }
    finish();

    if (Convert::write_file(NETWORK_4G_CONFIG_FILE, m_config) != OK) {
        dlog_error("[4G] Failed to save configuration");
        return RET_ERR_CONFIG;
    }
    last_query_time = std::chrono::steady_clock::time_point();
    last_cached_info.set_config_ret = m_config;
    if (m_config.enabled) connectAsync();
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

#if 0
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
#endif

void FourGManager::connectAsync() {
    bool expected = false;
    if (!m_dial_in_progress.compare_exchange_strong(expected, true)) {
        dlog_info("[4G] Background dial already in progress");
        return;
    }

    std::thread([this]() {
                // Keep recovering independently of the web page and abnormal-event
        // monitor. This also covers boot-time USB enumeration and delayed SIM
        // registration. The saved enable switch remains authoritative.
        while (m_config.enabled && !m_4gDataConnected.load()) {
            RetCode ret = is_initialized ? connect() : init();
            if (ret == RET_OK && m_4gDataConnected.load()) {
                dlog_info("[4G] Background ECM dial succeeded");
                // Do not wait for another subsystem's periodic callback to
                // install the recovered default route.
                updateRouteIfNeeded(!isWiredConnected());
                break;
            }

            dlog_error("[4G] Background ECM recovery failed, ret=%d; retrying in 15 seconds",
                       ret);
            const long long now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            m_nextAutoDialTimeMs.store(now_ms + 15000);
            for (int second = 0; second < 15 && m_config.enabled; ++second)
                std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        m_dial_in_progress.store(false);
    }).detach();
}

RetCode FourGManager::connect() {
    // A new dial attempt is not usable for routing until registration, ECM
    // activation and host-side DHCP have all completed successfully.
    m_4gDataConnected.store(false);
    if (init_status != RET_OK) return init_status;
    if (!m_config.enabled) return RET_ERR_CONFIG;

    bool need_close = false;
    if (fd < 0) {
        if (openPort() != RET_OK) return RET_ERR_OPEN_PORT;
        need_close = true;
    }

    std::string apn = m_config.apn.empty() ? "cmnet" : m_config.apn;
    int auth_type = m_config.auth_mode;
    if (auth_type == ::Network::AUTH_PAP_CHAP) {
        // AT+ECMDUP supports PAP(1) or CHAP(2), not a combined value.
        auth_type = ::Network::AUTH_CHAP;
    }

    std::string resp;
    if (sendCommand("AT+CFUN?", resp, 2000) != RET_OK ||
        resp.find("+CFUN: 1") == std::string::npos) {
        if (sendCommand("AT+CFUN=1", resp, 10000) != RET_OK) {
            if (need_close) closePort();
            return RET_ERR_FAILURE;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    sendCommand("AT+CEREG=2", resp, 2000);
    if (sendCommand("AT+CEREG?", resp, 2000) == RET_OK) {
        size_t pos = resp.find("+CEREG:");
        int report_mode = 0;
        int state = -1;
        if (pos != std::string::npos &&
            std::sscanf(resp.c_str() + pos + 7, " %d,%d", &report_mode, &state) == 2 &&
            state == 0) {
            // COPS=0 returns CME ERROR 100 on this firmware. Restart RF once
            // and let the modem perform automatic registration itself.
            sendCommand("AT+CFUN=0", resp, 10000);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (sendCommand("AT+CFUN=1", resp, 10000) != RET_OK) {
                if (need_close) closePort();
                return RET_ERR_FAILURE;
            }
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }

    bool registered = false;
    for (int retry = 0; retry < 5; ++retry) {
        if (sendCommand("AT+CEREG?", resp, 2000) == RET_OK) {
            size_t pos = resp.find("+CEREG:");
            int report_mode = 0;
            int state = -1;
            if (pos != std::string::npos &&
                std::sscanf(resp.c_str() + pos + 7, " %d,%d", &report_mode, &state) == 2 &&
                (state == 1 || state == 5)) {
                registered = true;
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    if (!registered) {
        dlog_error("[4G] Network registration timeout, ECM dial cancelled");
        if (need_close) closePort();
        return RET_ERR_TIMEOUT;
    }

    // The short form is the most compatible form for a context already set
    // by CGDCONT. Private APNs with authentication use the full form.
    std::string cmd = "AT+ECMDUP=1,1";
    if (auth_type == ::Network::AUTH_PAP || auth_type == ::Network::AUTH_CHAP) {
        cmd = "AT+ECMDUP=1,1,0,\"" + apn + "\",\"" +
               m_config.username + "\",\"" + m_config.password +
               "\"," + std::to_string(auth_type);
    }

    const auto query_ecm_active = [this](std::string& response) {
        return sendCommand("AT+ECMDUP?", response, 3000) == RET_OK &&
               response.find("+ECMDUP: 1,1") != std::string::npos;
    };

    // ECMDUP start is not idempotent on SLM770A: when a session already
    // exists (or is still coming up), another start command may return ERROR.
    // The query result, not the start command's immediate return, is the
    // authoritative state.
    bool ecm_active = query_ecm_active(resp);
    if (!ecm_active) {
        RetCode start_ret = sendCommand(cmd, resp, 30000);
        if (start_ret != RET_OK)
            dlog_warn("[4G] ECMDUP start returned %d; checking actual session state", start_ret);

        for (int retry = 0; retry < 10 && !ecm_active; ++retry) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            ecm_active = query_ecm_active(resp);
        }
    } else {
        dlog_info("[4G] Existing ECM data session detected; reusing it");
    }
    RetCode ret = ecm_active ? RET_OK : RET_ERR_FAILURE;

    bool pdp_active = false;
    for (int retry = 0; ret == RET_OK && retry < 10 && !pdp_active; ++retry) {
        if (sendCommand("AT+CGCONTRDP=1", resp, 3000) == RET_OK) {
            const size_t line = resp.find("+CGCONTRDP:");
            std::vector<std::string> quoted_fields;
            size_t cursor = line;
            while (cursor != std::string::npos) {
                const size_t begin = resp.find('"', cursor);
                if (begin == std::string::npos) break;
                const size_t end = resp.find('"', begin + 1);
                if (end == std::string::npos) break;
                quoted_fields.push_back(resp.substr(begin + 1, end - begin - 1));
                cursor = end + 1;
            }
            pdp_active = quoted_fields.size() >= 2 &&
                         quoted_fields[1] != "0.0.0.0" &&
                         isValidIpv4(quoted_fields[1]);
        }
        if (!pdp_active) std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    if (ret == RET_OK && !pdp_active) {
        dlog_error("[4G] ECMDUP is active but no cellular PDP address is available");
        ret = RET_ERR_FAILURE;
    }

    if (need_close) closePort();
    if (ret != RET_OK) return ret;

    runSystemCommand("ifconfig usb0 up");
    // BusyBox -R releases the lease when udhcpc exits. Combined with -q it
    // obtained an address and immediately cleared it, so never use -R here.
    runSystemCommand("udhcpc -i usb0 -q -n -t 5");
    runSystemCommand("ip link set dev usb0 mtu " + std::to_string(m_config.mtu));

    std::string host_ip;
    std::string host_mask;
    if (!getInterfaceIpv4("usb0", host_ip, host_mask)) {
        dlog_error("[4G] ECM is active but usb0 has no usable host address");
        return RET_ERR_FAILURE;
    }

    m_4gDataConnected.store(true);
    return RET_OK;
}

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
    std::ifstream route("/proc/net/route");
    std::string line;
    while (std::getline(route, line)) {
        std::istringstream fields(line);
        std::string iface, destination, gateway_hex;
        fields >> iface >> destination >> gateway_hex;
        if (iface != "usb0" || destination != "00000000") continue;

        try {
            unsigned long gateway = std::stoul(gateway_hex, nullptr, 16);
            struct in_addr addr;
            addr.s_addr = gateway;
            return inet_ntoa(addr);
        } catch (...) {
            return "";
        }
    }
    // The DHCP default may have been removed by an older route switch. The
    // SLM770A ECM DHCP server uses the first address of the usb0 /24 subnet.
    std::string usb_ip;
    std::string usb_mask;
    if (getInterfaceIpv4("usb0", usb_ip, usb_mask)) {
        size_t last_dot = usb_ip.rfind('.');
        if (last_dot != std::string::npos) return usb_ip.substr(0, last_dot) + ".1";
    }
    return "";
}
#if 0
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
            /*
             * 4G 默认路由生效后重新读取网页持久化的平台参数。
             * 与 WiFi 切换保持一致，enable=false 时也按重新上电的临时规则使用最新配置连接。
             */
            if (CPlatformManager::instance()->reconnect_from_persisted_config() != OK)
            {
                /* 平台管理模块会在有效配置的立即重连失败后启动后台重试。 */
                dlog_warn("4G 路由已切换，平台配置重载或立即重登失败");
            }
        }
    
}
#endif

void FourGManager::updateRouteIfNeeded(bool bWiredDisconnected) {
    if (m_config.enabled && !m_4gDataConnected.load() &&
    !m_dial_in_progress.load()) {
    const long long now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    if (now_ms >= m_nextAutoDialTimeMs.load()) {
        dlog_info("[4G] Data link is down; scheduling automatic dial recovery");
        connectAsync();
    }
}
    std::string wired_ip;
    std::string wired_gateway;
    parseNetworkConfig(wired_ip, wired_gateway);

    std::string usb_ip;
    std::string usb_mask;
    const bool usb_has_ip = getInterfaceIpv4("usb0", usb_ip, usb_mask);
    const std::string usb_gateway = usb_has_ip ? getUsb0Gateway() : "";

    const bool wired_available = !bWiredDisconnected && isValidIpv4(wired_gateway);
    // usb0 can expose the modem management LAN even without a SIM/PDP
    // connection.  It is eligible as a default route only after connect()
    // confirms cellular registration, ECMDUP activation and host DHCP.
    const bool fourg_available = m_config.enabled && m_4gDataConnected.load() &&
                                 usb_has_ip && isValidIpv4(usb_gateway);
    const bool availability_changed =
        wired_available != m_lastWiredAvailable || fourg_available != m_last4gAvailable;
    m_lastWiredAvailable = wired_available;
    m_last4gAvailable = fourg_available;

    // With 4G disabled/unavailable, always repair the wired default route.
    // This fixes the startup case where carrier was already up and the old
    // state-only comparison skipped route initialization.
    if (!fourg_available) {
        if (wired_available &&
            (m_routePreference != ROUTE_ETH0 || !hasDefaultRoute("eth0"))) {
            applyDefaultRoutes(wired_gateway, "", false);
            m_routePreference = ROUTE_ETH0;
            dlog_info("[4G-ROUTE] 4G unavailable, using eth0 gateway=%s",
                      wired_gateway.c_str());
        }
        return;
    }

    // No wired carrier: 4G is the only default route, while any eth0 direct
    // subnet route remains untouched for local access if the link returns.
    if (!wired_available) {
        if (availability_changed || m_routePreference != ROUTE_4G || !hasDefaultRoute("usb0")) {
            applyDefaultRoutes("", usb_gateway, true);
            const bool changed = m_routePreference != ROUTE_4G;
            m_routePreference = ROUTE_4G;
            dlog_info("[4G-ROUTE] wired unavailable, using 4G gateway=%s",
                      usb_gateway.c_str());
            if (changed) CPlatformManager::instance()->change_net_relogin();
        }
        return;
    }

    // 4G is the normal default. eth0 keeps only its connected/static subnet
    // route and becomes the default only while 4G Internet is unhealthy.
    if (m_routePreference == ROUTE_UNKNOWN) m_routePreference = ROUTE_4G;
    const bool expected_default_exists = m_routePreference == ROUTE_4G
        ? hasDefaultRoute("usb0") : hasDefaultRoute("eth0");
    if (availability_changed || !expected_default_exists) {
        const bool prefer_4g = m_routePreference == ROUTE_4G;
        applyDefaultRoutes(wired_gateway, usb_gateway, prefer_4g);
    }

    const bool eth_internet = checkInternetByInterface("eth0");
    const bool fourg_internet = checkInternetByInterface("usb0");

    const auto update_counter = [](bool healthy, int& successes, int& failures) {
        if (healthy) {
            successes = std::min(successes + 1, 3);
            failures = 0;
        } else {
            failures = std::min(failures + 1, 3);
            successes = 0;
        }
    };
    update_counter(eth_internet, m_ethInternetSuccess, m_ethInternetFailure);
    update_counter(fourg_internet, m_4gInternetSuccess, m_4gInternetFailure);

    RoutePreference desired = m_routePreference;
    if (m_routePreference == ROUTE_4G) {
        if (m_4gInternetFailure >= 3 && m_ethInternetSuccess >= 2)
            desired = ROUTE_ETH0;
    } else if (m_routePreference == ROUTE_ETH0) {
        // Return to the normal 4G default after it has recovered stably.
        if (m_4gInternetSuccess >= 3)
            desired = ROUTE_4G;
    }

    dlog_info("[4G-ROUTE] health eth0=%d(success=%d fail=%d), usb0=%d(success=%d fail=%d), preferred=%d",
              eth_internet ? 1 : 0, m_ethInternetSuccess, m_ethInternetFailure,
              fourg_internet ? 1 : 0, m_4gInternetSuccess, m_4gInternetFailure,
              static_cast<int>(m_routePreference));

    if (desired == m_routePreference) return;

    const bool prefer_4g = desired == ROUTE_4G;
    if (!applyDefaultRoutes(wired_gateway, usb_gateway, prefer_4g)) return;

    m_routePreference = desired;
    dlog_info("[4G-ROUTE] default route switched to %s",
              prefer_4g ? "usb0" : "eth0");
    CPlatformManager::instance()->change_net_relogin();
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
