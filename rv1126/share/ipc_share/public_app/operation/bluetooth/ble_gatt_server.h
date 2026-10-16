#pragma once

#include "ble_command_protocol.h"
#include "ble_security.h"

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

typedef struct _GDBusConnection GDBusConnection;
typedef struct _GDBusNodeInfo GDBusNodeInfo;
typedef struct _GDBusMethodInvocation GDBusMethodInvocation;
typedef struct _GMainContext GMainContext;
typedef struct _GMainLoop GMainLoop;
typedef struct _GVariant GVariant;
typedef struct _GError GError;

namespace BleProvisioning
{

/*
 * BlueZ GATT 配网服务。
 *
 * 手机向 Command 特征写入协议帧，设备通过 Response 特征 Read/Notify 返回结果。
 * 本类只负责 BLE/D-Bus 适配、会话校验和流程编排；真正的 Wi-Fi/平台操作
 * 通过 ExternalCommandSender 转交给 operation 中已有的业务任务执行。
 */
class BleGattServer
{
public:
    /* command 为 BLE 指令码，JSON 为转交给业务层的参数。返回负数表示投递失败。 */
    typedef std::function<int(uint16_t, const std::string &)> ExternalCommandSender;

    /** 获取进程内唯一的 BLE GATT 配网服务实例。 */
    static BleGattServer &instance();

    /** 创建后台 GMainLoop 线程并异步启动 BlueZ GATT 服务。 */
    bool start();
    /** 请求主循环退出、回收 D-Bus/BlueZ资源并等待后台线程结束。 */
    void stop();
    /** 返回 GATT Application 和广播是否已经成功启动。 */
    bool running() const;

    /** 注册或替换指定 BLE 指令码的协议处理函数。 */
    void registerCommandHandler(uint16_t command,
                                const CommandDispatcher::Handler &handler);
    /** 设置向 operation/control 业务层投递 Wi-Fi、平台任务的回调。 */
    void setExternalCommandSender(const ExternalCommandSender &sender);
    /** 业务任务完成后调用，把异步结果映射为原 command、sequence 的 BLE 响应。 */
    void onExternalCommandResult(uint16_t command, bool success,
                                 const std::string &detail);

private:
    /** 初始化成员、广播名称以及内置认证和配网指令处理器。 */
    BleGattServer();
    /** 确保服务停止并回收后台线程。 */
    ~BleGattServer();
    BleGattServer(const BleGattServer &) = delete;
    BleGattServer &operator=(const BleGattServer &) = delete;

    /** BLE 后台线程入口：创建 GLib上下文，初始化失败时循环重试并运行主循环。 */
    void threadMain();
    /** 初始化安全模块、连接 system D-Bus、导出对象并向 BlueZ 注册服务。 */
    bool initializeBluez();
    /** 注销广播和 GATT Application，清理连接状态、残包、敏感数据及 D-Bus对象。 */
    void cleanupBluez();
    /** 根据 XML描述把 ObjectManager、广播、服务和两个特征导出到 D-Bus。 */
    bool registerObjects();
    /** 开启 hci0，并把已导出的 GATT Application 和广播注册到 bluetoothd。 */
    bool registerWithBluez();
    /** 处理已经完成组帧和 CRC校验的请求，并根据安全要求解密、分发。 */
    void handleFrame(const CommandFrame &frame);
    /** 编码并缓存完整响应；已订阅 Notify 时根据 ATT MTU分片发送。 */
    void publishResponse(const CommandFrame &request, const CommandResult &result);
    /** 校验并暂存0x0101携带的 Wi-Fi UTF-8 JSON凭据。 */
    CommandResult handleWifiCredentials(const CommandFrame &frame);
    /** 处理0x0102，将已暂存凭据异步投递给 Wi-Fi连接业务。 */
    CommandResult handleWifiConnect(const CommandFrame &frame);
    /** 处理0x0103，返回当前 Wi-Fi连接流程状态和描述。 */
    CommandResult handleWifiStatus(const CommandFrame &frame);
    /** 处理0x0105，启动异步 Wi-Fi扫描并阻止扫描任务并发。 */
    CommandResult handleWifiScan(const CommandFrame &frame);
    /** 处理0x0201，在 Wi-Fi成功后异步投递平台连接参数。 */
    CommandResult handlePlatformConnect(const CommandFrame &frame);
    /** 处理0x0202，返回当前平台连接流程状态和描述。 */
    CommandResult handlePlatformStatus(const CommandFrame &frame);
    /** 把0x0010开始认证请求转交给安全会话管理器。 */
    CommandResult handleAuthBegin(const CommandFrame &frame);
    /** 把0x0011认证证明请求转交给安全会话管理器。 */
    CommandResult handleAuthProve(const CommandFrame &frame);
    /** 把0x0012认证状态查询转交给安全会话管理器。 */
    CommandResult handleAuthStatus(const CommandFrame &frame);
    /** 将内部整数状态和文本详情编码为 state=N;detail=... 响应 Data。 */
    std::vector<uint8_t> makeFlowStatusPayload(int state, const std::string &detail) const;
    /** Wi-Fi和平台均成功后安排3秒定时器，随后关闭 BLE配网服务。 */
    void scheduleBluetoothShutdown();
    /** 启动平台状态初次查询和10秒周期查询定时器。 */
    void startPlatformStatusMonitor();
    /** 在 Wi-Fi已连接时向业务层发送一次平台在线状态查询。 */
    void queryPlatformStatus();
    /** 从设备型号和 CPU Serial生成“型号-序列号”广播名称。 */
    std::string buildAdvertisementName() const;

    /** 统一处理 BlueZ 对导出对象发起的 GetManagedObjects、读写和订阅方法调用。 */
    static void handleMethodCall(GDBusConnection *connection,
                                 const char *sender,
                                 const char *objectPath,
                                 const char *interfaceName,
                                 const char *methodName,
                                 GVariant *parameters,
                                 GDBusMethodInvocation *invocation,
                                 void *userData);
    /** 根据对象路径和属性名返回广播、服务及特征的 D-Bus属性值。 */
    static GVariant *handleGetProperty(GDBusConnection *connection,
                                       const char *sender,
                                       const char *objectPath,
                                       const char *interfaceName,
                                       const char *propertyName,
                                       GError **error,
                                       void *userData);

private:
    std::atomic<bool> m_running;
    std::atomic<bool> m_stopRequested;
    std::thread m_thread;
    GMainContext *m_context;
    GMainLoop *m_loop;
    GDBusConnection *m_connection;
    GDBusNodeInfo *m_rootInfo;
    GDBusNodeInfo *m_advertisementInfo;
    GDBusNodeInfo *m_serviceInfo;
    GDBusNodeInfo *m_characteristicInfo;
    unsigned int m_registrationIds[5];
    bool m_applicationRegistered;
    bool m_advertisementRegistered;
    std::atomic<bool> m_notifying;
    /* BlueZ 从 WriteValue/ReadValue options 上报当前连接协商出的 ATT MTU。 */
    std::atomic<unsigned int> m_attMtu;
    bool m_powerOffOnStop;
    std::atomic<bool> m_shutdownScheduled;
    /* 防止不同业务线程的响应分片交叉发送。 */
    mutable std::mutex m_publishMutex;
    mutable std::mutex m_responseMutex;
    std::vector<uint8_t> m_lastResponse;
    mutable std::mutex m_flowMutex;
    ExternalCommandSender m_externalSender;
    std::string m_wifiCredentials;
    std::string m_wifiDetail;
    std::string m_platformDetail;
    int m_wifiState;
    int m_platformState;
    CommandFrame m_pendingWifiRequest;
    CommandFrame m_pendingWifiScanRequest;
    CommandFrame m_pendingPlatformRequest;
    bool m_hasPendingWifiRequest;
    bool m_hasPendingWifiScanRequest;
    bool m_hasPendingPlatformRequest;
    std::string m_advertisementName;
    FrameDecoder m_decoder;
    CommandDispatcher m_dispatcher;
    BleSecurityManager m_security;
};

} // namespace BleProvisioning
