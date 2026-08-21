/**
 * @FilePath     : platform_manager.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-08 17:44:21
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-29 14:24:22
 * @Description  : 平台管理
 */

#pragma once

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
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
    int relogin_and_update_stream();

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

    struct EventImageUploadRequest
    {
        std::string device_sn;   /* 设备SN，为空时自动从MQTT ClientId或设备信息中取值 */
        int event_type = 0;      /* 事件类型，用于平台归类和文件命名 */
        std::string event_name;  /* 事件名称，方便平台直接展示 */
        int channel = 0;         /* 事件触发通道号 */
        long long timestamp = 0; /* 事件时间戳，毫秒；为空时文件名使用当前时间戳 */
        std::string time;        /* 平台上传接口要求的时间字段，空时使用毫秒时间戳并与文件名时间保持一致 */
        std::string request_id;  /* 关联报警事件的RequestId，便于平台串联事件和图片 */
        std::string image_path;  /* 设备本地抓拍图片路径 */
        std::string file_name;   /* 平台侧保存文件名，空时自动生成：SN_事件类型_时间_序号.jpg */
    };

    struct EventImageUploadResponse
    {
        int status_code = 0;     /* HTTP状态码或平台业务状态码 */
        std::string status;      /* 平台返回状态 */
        std::string message;     /* 平台返回说明 */
        std::string image_url;   /* 平台返回的图片访问地址 */
        std::string image_path;  /* 设备本地图片路径，失败时也会带回便于排查 */
        std::string file_name;   /* 实际上传使用的文件名 */
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
     * @brief 主动上传事件抓拍图片到平台
     * @param request 事件图片上传请求
     * @param out_response 平台响应
     * @return true 上传成功, false 上传失败
     */
    bool upload_event_image(const EventImageUploadRequest &request, EventImageUploadResponse &out_response);

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
    /* bIgnoreEnable 仅供启动或网络切换配置重载后的自动重连使用，网页保存仍遵循 enable。 */
    bool login(const std::string &host,
               int port,
               const std::string &user,
               const std::string &password,
               const bool &enable,
               const bool &Custom,
               LoginResponse &out_response,
               bool bIgnoreEnable = false);
    
               /**
     * @brief 将网页平台参数应用到运行时配置
     * @note 自定义平台时，HTTP 与 MQTT 共用 user/password。
     */
    bool apply_platform_config(const ::Network::Platform_Info_t &stInfo);

    void getlogininfo(::Network::LoginInfo &retLoginInfo);
    void getplatforminfo(::Network::Platform_Info_t &retPlatformIfo);

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
     * @brief 按当前平台参数重建 MQTT 会话
     * @note 先停止旧 Broker 的连接线程，再连接新 Broker。
     */
    int restart_mqtt();

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

    /**
     * @brief   : 发布设备在线/离线状态到平台
     * @param    {bool} bOnline：true=在线, false=离线
     * @param    {const std::string &} strReason：状态原因（"connect"/"shutdown"/"disconnect"）
     * @return   {int} OK=成功, 非OK=失败
     * @note    : 消息发布到 device/{SN}/status Topic，QoS=1
     */
    int publish_device_status(bool bOnline, const std::string &strReason);

    /**
     * @brief   :网页关闭平台接入时释放MQTT连接
     * @note    ：先发布 offline/disabled 状态，再停止 MQTT 重连线程并断开连
     */
    void disable_mqtt_for_platform();

    int change_net_relogin();//切换无线重新登录

    /**
     * @brief   : 获取平台配置与重连操作锁
     * @note    : 网页保存必须持有该锁覆盖“应用配置、登录、MQTT 重建、RTMP 更新”的完整序列，
     *            避免与 WiFi/4G 网络切换使用不同平台参数并发执行。
     */
    std::unique_lock<std::recursive_mutex> lock_platform_operation();

    /**
     * @brief   : 网络切换完成后，从持久化配置重新加载平台参数并重连
     * @return  : OK 登录、注册和 RTMP 更新成功；其他值表示读取、校验或重连失败
     * @note    : enable=false 时按本次启动的临时自动连接规则处理，网页保存的 enable 值不被改写
     */
    int reconnect_from_persisted_config();
    
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

    /* 确保自动登录线程存在；线程常驻以承接后续网络切换重连。 */
    void ensure_auto_login_thread();

    /**
     * @brief   : 处理收到的 MQTT 消息
     * @param    {const std::string &} strTopic：消息 Topic
     * @param    {const std::string &} strPayload：消息内容（JSON）
     * @return   {void}
     * @note    : 此函数由 MQTT SDK 回调触发，只复制 Topic/Payload 并投入命令队列
     */
    void on_mqtt_message(const std::string &strTopic, const std::string &strPayload);

    /**
     * @brief   : 启动 MQTT 平台命令工作线程
     * @return   {void}
     * @note    : MQTT SDK 回调线程只负责复制并入队，HTTP、JSON 和任务执行均在该线程处理
     */
    void start_mqtt_command_worker();

    /**
     * @brief   : 停止 MQTT 平台命令工作线程
     * @return   {void}
     * @note    : 必须在释放 MQTT 管理器前完成，避免命令线程访问失效发布句柄
     */
    void stop_mqtt_command_worker();

    /**
     * @brief   : 将 MQTT 原始消息放入有界命令队列
     * @param    {const std::string &} strTopic：消息 Topic
     * @param    {const std::string &} strPayload：消息内容
     * @return   {bool} true：入队成功，false：工作线程停止或队列已满
     * @note    : 队列满时直接拒绝新命令，保护 MQTT SDK 回调线程和设备内存
     */
    bool enqueue_mqtt_command(const std::string &strTopic, const std::string &strPayload);

    /**
     * @brief   : MQTT 平台命令工作线程函数
     * @return   {void}
     * @note    : 单线程串行执行，保持同设备 MQTT 控制命令顺序
     */
    void mqtt_command_loop();

    /**
     * @brief   : 在平台命令线程中执行 MQTT 业务逻辑
     * @param    {const std::string &} strTopic：消息 Topic
     * @param    {const std::string &} strPayload：消息内容
     * @return   {void}
     * @note    : 仅可在 m_mqttCommandThread 中调用；同步任务和 HTTP 下载不得回到 MQTT SDK 回调上下文
     */
    void process_mqtt_command(const std::string &strTopic, const std::string &strPayload);

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

    /* 临时启动覆盖：网页保存新配置后立即清除。 */
    std::atomic<bool> m_bBootConnectOverride{false};
    /**
     * @brief   : 注册 MQTT 命令处理器
     * @note    : 在 init_mqtt() 中调用，注册所有业务命令
     */
    void register_mqtt_handlers();

    /**
     * @brief   : MQTT 连接状态变化回调
     * @param    {bool} bConnected：true=已连接, false=已断开
     * @param    {const std::string &} strReason：状态原因
     * @return   {void}
     * @note    : 由 CMqttManager 重连线程调用，连接成功时主动发布在线状态
     */
    void on_mqtt_connection_changed(bool bConnected, const std::string &strReason);

    /**
     * @brief MQTT 连接成功后向平台上报设备注册及取流信息
     * @return OK=已提交发送，非OK=未连接或参数不完整
     * @note 发布到 device/{SN}/register Topic，QoS=1
     */
    int publish_device_register();
    
    /**
     * @brief 启动设备在线状态心跳线程
     * @note  MQTT 连接成功后的首个在线状态仍由连接回调发送；该线程负责周期性续报。
     */
    void start_status_heartbeat();

    /**
     * @brief 停止设备在线状态心跳线程
     * @note  必须在释放 MQTT 管理器前调用，避免线程访问已释放的 MQTT 句柄。
     */
    void stop_status_heartbeat();

    /**
     * @brief 设备在线状态心跳线程函数
     */
    void status_heartbeat_loop();

    // 服务器配置
    const std::string host_ = "183.129.224.253";
    const int port_ = 4910;
    // const std::string host_ = "172.16.25.125";
    // const int port_ = 388;
    std::string custom_host;
    int custom_post;
    std::string login_user = "";
    std::string login_password = "";
    bool g_enable = false;
    bool g_custom = false;
    /* 串行化网页保存、配置重载与网络切换重连，递归锁允许重连入口内部复用 change_net_relogin。 */
    std::recursive_mutex m_mtxPlatformOperation;
    /* 临时自动连接覆盖：网页保存新配置后立即清除；WiFi 重读配置时会按启动规则重新设置。 */
    //std::atomic<bool> m_bBootConnectOverride{false};
    const std::string login_path_ = "/api/auth/login";
    const std::string store_path_ = "/api/device/store_device";
    const std::string workorder_path_ = "/api/workorder/store";
    const std::string event_image_upload_path_ = "/api/device/upload_screen";
    const std::string device_list_path_ = "/api/device/device_list";
    std::string access_token_ = "";
    std::string token_type_ = "bearer";

    /* 自动登录重试线程 */
    std::thread m_autoLoginThread;
    /* 停止自动登录标志 */
    std::atomic<bool> m_bStopAutoLogin{false};
    /* 网络已切换但平台登录未恢复时，即使旧 token 存在也必须继续重试。 */
    std::atomic<bool> m_bNetworkReloginPending{false};
    /* 串行化自动重试与网络切换触发的重登，避免并发 HTTP 登录。 */
    std::atomic<bool> m_bReloginInProgress{false};
    std::mutex m_mtxAutoLoginLifecycle;
    /* 自动登录重试间隔（秒） */
    static constexpr int AUTO_LOGIN_RETRY_INTERVAL_SEC = 30;
    /* 自动登录最大重试次数（0 表示无限重试） */
    static constexpr int AUTO_LOGIN_MAX_RETRIES = 0;
    /* 当前已重试次数 */
    //int m_nRetryCount = 0;
    std::atomic<int> m_nRetryCount{0};
    /* 初始化完成标志 */
    bool m_bInited = false;

    /* 平台在线状态心跳线程：使用独立生命周期锁，避免并发启停产生重复线程。 */
    std::thread m_statusHeartbeatThread;
    std::atomic<bool> m_bStopStatusHeartbeat{true};
    std::mutex m_mtxStatusHeartbeatLifecycle;
    static constexpr int STATUS_HEARTBEAT_INTERVAL_SEC = 30;

    // #define RTMP_DEFAULT_PORT   1935
    #define RTMP_DEFAULT_PORT   4920
    /* MQTT 相关 */
    CMqttManager *m_pstMqtt = nullptr;                      /* MQTT 管理器指针 */
    std::string m_strMqttBroker;                            /* MQTT Broker 地址 */
    int m_nMqttPort = MQTT_DEFAULT_PORT;                    /* MQTT Broker 端口 */
    int m_nRtmpPort = RTMP_DEFAULT_PORT;                    /* RTMP Broker 端口 */
    std::string m_strMqttUsername;                          /* MQTT 用户名 */
    std::string m_strMqttPassword;                          /* MQTT 密码 */
    std::string m_strMqttClientId;                          /* MQTT 客户端标识（设备SN） */

    /**
     * @brief   : MQTT SDK 回调与平台工作线程之间传递的原始命令
     * @note    : 数据在 SDK 回调返回前完成深拷贝，避免底层 MQTT 缓冲区释放后被异步访问
     */
    struct MqttCommand_S
    {
        /* 原始 MQTT Topic；当前用于日志和后续按 Topic 扩展路由。 */
        std::string strTopic;
        /* 原始 MQTT Payload；由工作线程负责 JSON 解析与业务处理。 */
        std::string strPayload;
    };

    /* memory: 有界队列中的命令由 m_mqttCommandThread 取走或 stop 时丢弃，避免命令风暴耗尽内存。 */
    std::deque<MqttCommand_S> m_deqMqttCommands;
    /* lock: 保护 m_deqMqttCommands、m_nLastMqttCommandDropLogMs 与停止前的清队列操作。 */
    std::mutex m_mtxMqttCommandQueue;
    /* worker 在队列为空时等待；入队和停止路径必须 notify，避免固定周期轮询。 */
    std::condition_variable m_cvMqttCommand;
    /* lock: 串行化命令线程的创建、停止和 join，防止平台重复启停创建多个消费者。 */
    std::mutex m_mtxMqttCommandLifecycle;
    /* worker 生命周期；true 时停止接收新命令并在当前任务结束后退出。 */
    std::atomic<bool> m_bStopMqttCommand{ true };
    /* worker 线程；仅可由 start_mqtt_command_worker 创建，并由 stop_mqtt_command_worker join。 */
    std::thread m_mqttCommandThread;
    /* 队列满时累计拒绝的新命令数量；原子计数允许 SDK 回调线程无额外日志锁地更新。 */
    std::atomic<uint64_t> m_uMqttCommandDropCount{ 0 };
    /* lock: 使用 steady_clock 的毫秒时间基；仅在 m_mtxMqttCommandQueue 保护下读写，用于限频满队列日志。 */
    int64_t m_nLastMqttCommandDropLogMs = 0;
    /* 命令队列容量上限；超过上限时拒绝最新消息，以优先保护媒体和 MQTT SDK 线程。 */
    static constexpr size_t MQTT_COMMAND_QUEUE_MAX_SIZE = 32;
    /* 队列满告警最小输出间隔，单位毫秒。 */
    static constexpr int64_t MQTT_COMMAND_DROP_LOG_INTERVAL_MS = 5000;

    /* 命令处理器 */
    using CommandHandler = std::function<void(const std::string &strRequestId, const std::string &strData)>;
    std::unordered_map<std::string, CommandHandler> m_mapMqttHandlers;
    std::mutex m_mtxMqttHandlers;
};
#endif
