#if CAP_NETWORK_4G
#ifndef _4G_MANAGE_H
#define _4G_MANAGE_H

#include <string>
#include "network_define.h"
#include "Singleton.h"
#include <thread> 
#include "platform_manager.h"
// 运营商类型枚举
typedef enum {
    OPERATOR_UNKNOWN = 0,
    OPERATOR_CHINA_MOBILE = 1, // 中国移动
    OPERATOR_CHINA_UNICOM = 2, // 中国联通
    OPERATOR_CHINA_TELECOM = 3 // 中国电信
} Operator_Type;

// 返回结果的状态码
typedef enum {
    RET_OK = 0,
    RET_ERR_TIMEOUT,
    RET_ERR_FAILURE,
    RET_ERR_OPEN_PORT,
    RET_ERR_CONFIG
} RetCode;

struct SIM_Info_t {
    std::string status;
    std::string ip_address;
    std::string subnet_mask;
    std::string gateway;
    std::string dns_address;
    std::string signal_quality;
    std::string operator_name; // 运营商名称（如 "China Telecom"）
    bool is_registered;
};

// 鉴权方式枚举 (对应网页上的 "网络切换方式" 下拉框)
typedef enum {
    AUTH_NONE = 0,  // 无鉴权
    AUTH_PAP = 1,   // PAP
    AUTH_CHAP = 2,   // CHAP
    AUTH_PAP_CHAP = 3 //3=PAP&CHAP
} Auth_Mode_t;

// 网络制式枚举 (对应网页上的 "网络切换方式" 下拉框)
typedef enum {
    NET_AUTO = 0,       // 自动
    NET_4G_ONLY = 1,    // 仅4G
    NET_3G_ONLY = 2,    // 仅3G
    NET_2G_ONLY = 3     // 仅2G
} Network_Mode_t;

// 拨号方式枚举 (对应网页上的 "拨号方式" 下拉框)
typedef enum {
    DIAL_AUTO = 0,      // 自动拨号 (模块内部处理)
    DIAL_MANUAL = 1     // 手动拨号 (主机发送ATD指令)
} Dial_Mode_t;

/**
 * @brief 4G模块网络配置结构体
 * @details 此结构体与网页表单字段一一对应
 */
struct Network_4G_Config_t {
    std::string apn;          // APN (例如: ctnet, cmnet)
    std::string username;     // 用户名 (专网参数)
    std::string password;     // 密码 (专网参数)
    std::string call_number;  // 拨号号码 (通常为 *99# 或 *99***1#)

    int mtu;                  // MTU大小 (例如: 1500, 1400)
    
    // 以下使用int存储枚举值，比string更高效
    int auth_mode;            // 鉴权方式 (0:None, 1:PAP, 2:CHAP,3=PAP&CHAP)
    int network_mode;         // 网络模式 (0:Auto, 1:4G, 2:3G...)
    int dial_mode;            // 拨号方式 (0:Auto, 1:Manual)

    // 构造函数：提供默认初始化值，防止空数据导致崩溃
    Network_4G_Config_t() : 
        apn(""), 
        username(""), 
        password(""), 
        call_number("*99#"), 
        mtu(1500), 
        auth_mode(AUTH_NONE), 
        network_mode(NET_AUTO), 
        dial_mode(DIAL_AUTO) {}
};

class FourGManager  : public CSingleton<FourGManager> {
public:
    FourGManager();
    ~FourGManager();

    RetCode init();

    void deinit();

    RetCode getStatus() const { return init_status; }
    
    // 获取当前配置（包含自动识别后的 APN）
    ::Network::Network_4G_Config_t getConfig() const { return m_config; }

    RetCode getSimInfo(::Network::SIM_Info_t &info);
    RetCode setConfig(const ::Network::Network_4G_Config_t &config);
    RetCode connect();
    bool isWiredConnected();
    void updateRouteIfNeeded(bool bWiredDisconnected);
    bool waitModuleReady();

private:
    std::string port_name;
    int fd = -1;
    RetCode init_status;
    bool is_initialized; 
    std::thread m_routeMonitorThread; 
    ::Network::Network_4G_Config_t m_config; // 保存当前配置
    std::string m_net_interface;// 用于存储网卡名称
    ::Network::SIM_Info_t last_cached_info; // 缓存上一次成功获取的信息
    std::chrono::steady_clock::time_point last_query_time; // 记录上一次查询的时间
    static constexpr int UPDATE_INTERVAL_MS = 2000; // 更新间隔：2000毫秒 (2秒)
    std::mutex port_mutex;
    bool m_lastWiredDisconnected = false;

    // 自动识别相关
    void autoDetectOperator(); 
    std::string getApnForOperator(Operator_Type op);
    Operator_Type parseOperatorFromImsi(const std::string& imsi);
    bool parseNetworkConfig(std::string& outIp, std::string& outGateway);

    // 串口操作
    RetCode openPort();
    void closePort();
    RetCode sendCommand(const std::string cmd, std::string &response, int timeout_ms);
    static void* routeMonitorThread(void* arg);
};

#endif
#endif