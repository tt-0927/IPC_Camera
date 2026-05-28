/**
 * @FilePath     : platform_manager.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-08 17:44:21
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-15 17:43:08
 * @Description  : 平台管理
 */

#pragma once

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <mutex>

#include "mqtt_manager.h"
#include "mqtt_topic_define.h"
#include "mqtt_message.h"

#include "Singleton.h"
#include "network_define.h"
#include "cJSON.h"

/* 前置声明，避免循环依赖 */
class CTaskManage;

#if CAP_GARBAGE_STATION_PLATFORM
class CPlatformManager : public CSingleton<CPlatformManager>
{
public:
    CPlatformManager() = default;
    ~CPlatformManager();

    /**
     * @brief   : 初始化平台管理模块
     * @return   {int} 0：成功，非0：失败
     * @note    : 读取持久化配置，若 enable=true 则启动自动登录重试线程
     */
    int init();

    /**
     * @brief   : 反初始化平台管理模块
     * @return   {int} 0：成功，非0：失败
     */
    int deinit();

    /**
     * @brief   : 重新登录平台并通知推流模块更新推流地址
     * @note    : 用于平台信息变更后，重新生成推流地址并热更新
     */
    void relogin_and_update_stream();

    struct LoginResponse
    {
        int status_code;
        std::string status;
        std::string message;

        // 嵌套的 data 数据
        struct Data
        {
            std::string access_token;
            std::string token_type;
            long expires_in;
            std::string phone;
            bool need_modify_password;

            // Data 结构体自己的构造函数
            Data() : access_token(""), token_type(""), expires_in(0), phone(""), need_modify_password(false)
            {
            }
        } data;

        // LoginResponse 的构造函数
        // 修改点：移除初始化列表，改为在函数体内赋值
        LoginResponse() : status_code(0), status(""), message("")
        {
        }
    };

    struct StoreDevice
    {
        std::string sn;          // 必须
        std::string name;        // 必须
        std::string version;     // 非必须
        std::string mac_address; // 非必须
        std::string ip;          // 必须
        // std::string port;            // 必须
        int port;
        std::string account;      // 必须
        std::string password;     // 必须
        std::string live_url;     // 非必须
        std::string playback_url; // 非必须
        std::string protocol;     // 必须
        std::string resolution;   // 必须
        std::string storage;      // 必须
        std::string use_storage;  // 必须

        std::string current_time; // 示例：2026-04-28
        std::string location;     // 示例：广东省 广州市

        // 构造函数
        StoreDevice() : version(""), mac_address(""), live_url(""), playback_url("")
        {
        }
    };

    // 返回数据结构
    struct StoreResponse
    {
        int status_code;
        std::string status;
        std::string message;

        struct Data
        {
            std::string sn;
            std::string name;
            std::string version;
            int status;
            std::string mac_address;
            std::string account;
            std::string password; // 注意：这是服务器返回的加密密码
            std::string live_url;
            std::string playback_url;
            std::string updated_at;
            std::string created_at;
            int id;
            std::string device_uuid;
        } data;

        StoreResponse() : status_code(0), data()
        {
        }
    };

    // 上报工单请求
    struct WorkOrderRequest
    {
        std::string device_sn; // 摄像头唯一标识
        int type;              // 事件类型
        std::string images;    // 抓拍照片
        std::string online;    // 在线状态 "0" 或 "1"

        WorkOrderRequest() : type(0)
        {
        }
    };

    // 上报工单响应
    struct WorkOrderResponse
    {
        int status_code;
        std::string status;
        std::string message;
        int data; // 返回的工单ID或状态码

        WorkOrderResponse() : status_code(0), data(0)
        {
        }
    };

    // 设备列表响应中的单个设备项
    struct DeviceItem
    {
        int id;
        std::string sn;
        std::string name;
        int area_id;
        std::string version;
        int status;
        std::string mac_address;
        std::string ip;
        std::string port;
        std::string account;
        std::string password;
        std::string live_url;
        std::string playback_url;
        std::string created_at;
        std::string updated_at;
        std::string deleted_at;
        std::string protocol;
        std::string resolution;
        int storage;
        int use_storage;
        std::string device_uuid;

        // 构造函数初始化
        DeviceItem() : id(0), area_id(0), status(0), storage(0), use_storage(0)
        {
        }
    };

    // 获取设备列表响应
    struct DeviceListResponse
    {
        int status_code;
        std::string status;
        std::string message;
        std::vector<DeviceItem> data; // 注意：这里是一个数组

        DeviceListResponse() : status_code(0)
        {
        }
    };

    // ================== 新增接口声明 ==================

    /**
     * @brief 上报工单信息
     * @param workOrder 工单信息
     * @param token 认证 Token
     * @param out_response 返回的响应
     * @return true 成功, false 失败
     */
    bool reportWorkOrder(const WorkOrderRequest &workOrder, const std::string &token, WorkOrderResponse &out_response);

    /**
     * @brief 获取设备列表
     * @param token 认证 Token
     * @param out_response 返回的设备列表响应
     * @return true 成功, false 失败
     */
    bool getDeviceList(const std::string &token, DeviceListResponse &out_response);

    /**
     * @brief 发起登录请求
     * @param user 用户名 (明文)
     * @param password 密码 (明文，函数内部会自动进行 Base64 编码)
     * @param out_response 返回的响应结构体
     * @return true 登录成功, false 失败
     */
    bool login(const std::string &host,
               int port,
               const std::string &user,
               const std::string &password,
               const bool &enable,
               const bool &Custom,
               LoginResponse &out_response);

    void getlogininfo(::Network::LoginInfo &retLoginInfo);

    /**
     * @brief   : 获取访问 token
     * @return   {std::string} access_token 字符串
     */
    std::string get_access_token() const;

    /**
     * @brief 上报设备信息
     * @param device 设备信息结构体
     * @param token 认证 Token (Authorization Bearer 后面的字符串)
     * @param out_response 返回的响应
     * @return true 成功, false 失败
     */
    bool storeDevice(const StoreDevice &device, const std::string &token, StoreResponse &out_response);

    /**
     * @brief   : 使用当前设备信息向平台注册摄像头
     * @param    {const std::string &} strToken：平台登录返回的 access_token
     * @param    {const std::string &} strAccount：注册到平台的设备账号
     * @param    {const std::string &} strPassword：注册到平台的设备密码
     * @return   {bool} true：注册成功，false：注册失败
     */
    bool register_current_device(const std::string &strToken,
                                 const std::string &strAccount,
                                 const std::string &strPassword);

    /**
     * @brief   : 将当前平台登录信息保存到配置文件
     * @return   {bool} true：成功，false：失败
     */
    bool save_config();

    /**
     * @brief   : 初始化 MQTT 连接
     * @return   {int} 0：成功，非0：失败
     * @note    : 在 init() 中调用，从配置文件读取 MQTT 参数
     */
    int init_mqtt();

    /**
     * @brief   : 反初始化 MQTT
     */
    void deinit_mqtt();

    /**
     * @brief   : 发布业务事件
     * @param    {const std::string &} strCommand：事件命令名
     * @param    {const std::string &} strData：事件数据（JSON）
     * @param    {const std::string &} strRequestId：请求ID（可选）
     * @return   {int} 0：成功，非0：失败
     */
    int publish_event(const std::string &strCommand,
                      const std::string &strData,
                      const std::string &strRequestId = "");

    /**
     * @brief   : 发布命令响应
     * @param    {const std::string &} strCommand：命令名
     * @param    {const std::string &} strRequestId：请求ID（用于关联请求）
     * @param    {int} nReturn：返回码（0=成功）
     * @param    {const std::string &} strData：响应数据（JSON）
     * @return   {int} 0：成功，非0：失败
     */
    int publish_response(const std::string &strCommand,
                         const std::string &strRequestId,
                         int nReturn,
                         const std::string &strData = "{}");

    /**
     * @brief   : 设置 CTaskManage 实例指针
     * @param    {CTaskManage *} pTaskManage：任务管理器指针
     * @note    : 在 ControlManage::init_server() 中调用，用于 MQTT SDK 网关执行命令
     */
    void set_taskManage(CTaskManage *pTaskManage);

private:
    // 辅助函数：Base64 编码
    std::string base64_encode(const std::string &input);

    /**
     * @brief   : 从配置文件加载平台登录信息
     * @return   {bool} true：成功，false：失败或文件不存在
     */
    bool load_config();

    /**
     * @brief   : 自动登录重试线程函数
     * @note    : 设备重启后若网络未就绪，定时重试登录，直到成功或达到最大重试次数
     */
    void auto_login_loop();

    /**
     * @brief   : 处理收到的 MQTT 消息
     * @param    {const std::string &} strTopic：消息 Topic
     * @param    {const std::string &} strPayload：消息内容（JSON）
     * @note    : 解析 JSON 并分发到对应处理器
     */
    void on_mqtt_message(const std::string &strTopic, const std::string &strPayload);

    /**
     * @brief   : MQTT 人脸命令预处理，确保平台 NV21 文件已下载到设备本地
     * @param    {const std::string &} strCommand：MQTT 命令名
     * @param    {std::string &} strData：命令 Data，成功时会回写规范后的 JSON
     * @param    {std::string &} strError：失败原因
     * @return   {bool} true：成功，false：失败
     */
    bool prepare_face_image_command(const std::string &strCommand,
                                    std::string &strData,
                                    std::string &strError);

    bool ensure_face_nv21_local(cJSON *pData, std::string &strError);
    bool download_file_to_path(const std::string &strUrl,
                               const std::string &strLocalPath,
                               std::string &strError);
    std::string resolve_platform_file_url(const std::string &strPathOrUrl) const;

    /**
     * @brief   : 注册 MQTT 命令处理器
     * @note    : 在 init_mqtt() 中调用，注册所有业务命令
     */
    void register_mqtt_handlers();

    // 服务器配置
    const std::string host_ = "183.129.224.253";
    const int port_ = 4910;
    std::string custom_host;
    int custom_post;
    std::string login_user = "";
    std::string login_password = "";
    bool g_enable = false;
    bool g_custom = false;
    const std::string login_path_ = "/api/auth/login";
    const std::string store_path_ = "/api/device/store_device";
    const std::string workorder_path_ = "/api/workorder/store";
    const std::string device_list_path_ = "/api/device/device_list";
    std::string access_token_ = "";
    std::string token_type_ = "bearer";

    /* 自动登录重试线程 */
    std::thread m_autoLoginThread;
    /* 停止自动登录标志 */
    std::atomic<bool> m_bStopAutoLogin{false};
    /* 自动登录重试间隔（秒） */
    static constexpr int AUTO_LOGIN_RETRY_INTERVAL_SEC = 30;
    /* 自动登录最大重试次数（0 表示无限重试） */
    static constexpr int AUTO_LOGIN_MAX_RETRIES = 0;
    /* 当前已重试次数 */
    int m_nRetryCount = 0;
    /* 初始化完成标志 */
    bool m_bInited = false;

    /* MQTT 相关 */
    CMqttManager *m_pstMqtt = nullptr;                      /* MQTT 管理器指针 */
    std::string m_strMqttBroker;                             /* MQTT Broker 地址 */
    int m_nMqttPort = MQTT_DEFAULT_PORT;                     /* MQTT Broker 端口 */
    std::string m_strMqttUsername;                           /* MQTT 用户名 */
    std::string m_strMqttPassword;                          /* MQTT 密码 */
    std::string m_strMqttClientId;                          /* MQTT 客户端标识（设备SN） */
    
    /* 命令处理器 */
    using CommandHandler = std::function<void(const std::string &strRequestId, const std::string &strData)>;
    std::unordered_map<std::string, CommandHandler> m_mapMqttHandlers;
    std::mutex m_mtxMqttHandlers;
};
#endif
