#include "ble_gatt_server.h"

#include "dlog.h"
#include "share_define.h"
#include <gio/gio.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <fstream>
#include <sstream>

namespace BleProvisioning
{
namespace
{
/*
 * operation 不直接操作 HCI。它把下列对象导出到 system D-Bus，
 * bluetoothd 读取这些对象后负责真正的 GATT 服务和 BLE 广播。
 */
const char *kBluezBus = "org.bluez";
const char *kAdapterPath = "/org/bluez/hci0";
const char *kApplicationPath = "/com/tv/provision";
const char *kAdvertisementPath = "/com/tv/provision/advertisement0";
const char *kServicePath = "/com/tv/provision/service0";
const char *kCommandPath = "/com/tv/provision/service0/command0";
const char *kResponsePath = "/com/tv/provision/service0/response0";

const char *kServiceUuid = "7a1e0001-7d8b-4f31-9e45-545650524f56";
const char *kCommandUuid = "7a1e0002-7d8b-4f31-9e45-545650524f56";
const char *kResponseUuid = "7a1e0003-7d8b-4f31-9e45-545650524f56";

const unsigned int kDefaultAttMtu = 23;
const unsigned int kAttValueOverhead = 3;

const char *kRootXml =
    "<node><interface name='org.freedesktop.DBus.ObjectManager'>"
    "<method name='GetManagedObjects'><arg type='a{oa{sa{sv}}}' name='objects' direction='out'/>"
    "</method></interface></node>";

const char *kAdvertisementXml =
    "<node><interface name='org.bluez.LEAdvertisement1'>"
    "<method name='Release'/><property name='Type' type='s' access='read'/>"
    "<property name='ServiceUUIDs' type='as' access='read'/>"
    "<property name='LocalName' type='s' access='read'/>"
    "</interface></node>";

const char *kServiceXml =
    "<node><interface name='org.bluez.GattService1'>"
    "<property name='UUID' type='s' access='read'/>"
    "<property name='Primary' type='b' access='read'/>"
    "</interface></node>";

const char *kCharacteristicXml =
    "<node><interface name='org.bluez.GattCharacteristic1'>"
    "<method name='ReadValue'><arg type='a{sv}' name='options' direction='in'/>"
    "<arg type='ay' name='value' direction='out'/></method>"
    "<method name='WriteValue'><arg type='ay' name='value' direction='in'/>"
    "<arg type='a{sv}' name='options' direction='in'/></method>"
    "<method name='StartNotify'/><method name='StopNotify'/>"
    "<property name='UUID' type='s' access='read'/>"
    "<property name='Service' type='o' access='read'/>"
    "<property name='Flags' type='as' access='read'/>"
    "<property name='Value' type='ay' access='read'/>"
    "<property name='Notifying' type='b' access='read'/>"
    "</interface></node>";

/* 将 vector 字节数组包装成 D-Bus ay Variant，供属性和方法返回使用。 */
GVariant *byteArrayVariant(const std::vector<uint8_t> &value)
{
    return g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE,
                                     value.empty() ? NULL : value.data(),
                                     value.size(), sizeof(uint8_t));
}

/* 将指定内存片段包装成 D-Bus ay Variant，主要用于响应 Notify分片。 */
GVariant *byteArrayVariant(const uint8_t *value, std::size_t size)
{
    return g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE,
                                     size == 0 ? NULL : value,
                                     size, sizeof(uint8_t));
}

/* 构造 BlueZ注册方法要求的空 a{sv} 选项字典。 */
GVariant *emptyOptions()
{
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
    return g_variant_builder_end(&builder);
}

/* 把一个接口及其属性字典加入 ObjectManager的接口集合。 */
void addInterface(GVariantBuilder *interfaces, const char *name,
                  GVariantBuilder *properties)
{
    g_variant_builder_add(interfaces, "{s@a{sv}}", name,
                          g_variant_builder_end(properties));
}

/* 把对象路径和接口集合加入 GetManagedObjects返回树。 */
void addManagedObject(GVariantBuilder *objects, const char *path,
                      GVariantBuilder *interfaces)
{
    g_variant_builder_add(objects, "{o@a{sa{sv}}}", path,
                          g_variant_builder_end(interfaces));
}

/* 构造 BlueZ注册 GATT Application时读取的完整服务和特征对象树。 */
GVariant *buildManagedObjects(bool notifying, const std::vector<uint8_t> &lastResponse)
{
    /*
     * BlueZ 注册 GATT Application 时会调用 GetManagedObjects，要求一次返回
     * 完整对象树：一个主服务、一个写特征和一个读/通知特征。
     */
    GVariantBuilder objects;
    g_variant_builder_init(&objects, G_VARIANT_TYPE("a{oa{sa{sv}}}"));

    GVariantBuilder interfaces;
    GVariantBuilder properties;

    g_variant_builder_init(&interfaces, G_VARIANT_TYPE("a{sa{sv}}"));
    g_variant_builder_init(&properties, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&properties, "{sv}", "UUID", g_variant_new_string(kServiceUuid));
    g_variant_builder_add(&properties, "{sv}", "Primary", g_variant_new_boolean(TRUE));
    addInterface(&interfaces, "org.bluez.GattService1", &properties);
    addManagedObject(&objects, kServicePath, &interfaces);

    const char *writeFlags[] = {"write", "write-without-response", NULL};
    g_variant_builder_init(&interfaces, G_VARIANT_TYPE("a{sa{sv}}"));
    g_variant_builder_init(&properties, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&properties, "{sv}", "UUID", g_variant_new_string(kCommandUuid));
    g_variant_builder_add(&properties, "{sv}", "Service", g_variant_new_object_path(kServicePath));
    g_variant_builder_add(&properties, "{sv}", "Flags", g_variant_new_strv(writeFlags, -1));
    addInterface(&interfaces, "org.bluez.GattCharacteristic1", &properties);
    addManagedObject(&objects, kCommandPath, &interfaces);

    const char *responseFlags[] = {"read", "notify", NULL};
    g_variant_builder_init(&interfaces, G_VARIANT_TYPE("a{sa{sv}}"));
    g_variant_builder_init(&properties, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&properties, "{sv}", "UUID", g_variant_new_string(kResponseUuid));
    g_variant_builder_add(&properties, "{sv}", "Service", g_variant_new_object_path(kServicePath));
    g_variant_builder_add(&properties, "{sv}", "Flags", g_variant_new_strv(responseFlags, -1));
    g_variant_builder_add(&properties, "{sv}", "Value", byteArrayVariant(lastResponse));
    g_variant_builder_add(&properties, "{sv}", "Notifying", g_variant_new_boolean(notifying));
    addInterface(&interfaces, "org.bluez.GattCharacteristic1", &properties);
    addManagedObject(&objects, kResponsePath, &interfaces);

    return g_variant_builder_end(&objects);
}

/* 统一记录 GError内容并释放错误对象，防止失败路径内存泄漏。 */
void logGError(const char *what, GError *error)
{
    dlog_error("[BLE GATT] %s: %s", what,
               error && error->message ? error->message : "unknown error");
    if (error)
        g_error_free(error);
}

struct PendingDBusCall
{
    GMainLoop *loop;
    GVariant *reply;
    GError *error;
};

/*
 * BlueZ在RegisterApplication/RegisterAdvertisement期间会反向读取本进程
 * 导出的ObjectManager和Advertisement属性。如果在拥有GMainContext的线程
 * 里使用call_sync，反向调用无法被分发，最终双方等待到超时。这里使用异步
 * 调用并运行嵌套主循环，使注册应答和BlueZ反向调用都能正常处理。
 */
GVariant *callBluezWithMainLoop(GDBusConnection *connection,
                                GMainContext *context,
                                const char *objectPath,
                                const char *interfaceName,
                                const char *methodName,
                                GVariant *parameters,
                                int timeoutMs,
                                GError **outError)
{
    PendingDBusCall pending;
    pending.loop = g_main_loop_new(context, FALSE);
    pending.reply = NULL;
    pending.error = NULL;

    g_dbus_connection_call(
        connection, kBluezBus, objectPath, interfaceName, methodName,
        parameters, NULL, G_DBUS_CALL_FLAGS_NONE, timeoutMs, NULL,
        [](GObject *source, GAsyncResult *result, gpointer userData) {
            PendingDBusCall *call = static_cast<PendingDBusCall *>(userData);
            call->reply = g_dbus_connection_call_finish(
                G_DBUS_CONNECTION(source), result, &call->error);
            g_main_loop_quit(call->loop);
        },
        &pending);

    g_main_loop_run(pending.loop);
    g_main_loop_unref(pending.loop);

    if (outError)
        *outError = pending.error;
    else if (pending.error)
        g_error_free(pending.error);
    return pending.reply;
}

/* 判断 GError是否为指定名称的远端 D-Bus错误，例如 AlreadyExists。 */
bool isRemoteError(GError *error, const char *expectedName)
{
    if (!error || !g_dbus_error_is_remote_error(error))
        return false;
    char *remoteName = g_dbus_error_get_remote_error(error);
    const bool matched = remoteName && std::strcmp(remoteName, expectedName) == 0;
    g_free(remoteName);
    return matched;
}
} // namespace

BleGattServer &BleGattServer::instance()
{
    static BleGattServer server;
    return server;
}

BleGattServer::BleGattServer()
    : m_running(false), m_stopRequested(false), m_context(NULL), m_loop(NULL),
      m_connection(NULL), m_rootInfo(NULL), m_advertisementInfo(NULL),
      m_serviceInfo(NULL), m_characteristicInfo(NULL),
      m_applicationRegistered(false), m_advertisementRegistered(false),
      m_notifying(false), m_attMtu(kDefaultAttMtu),
      m_powerOffOnStop(false), m_shutdownScheduled(false),
      m_wifiState(0),
      m_platformState(0), m_hasPendingWifiRequest(false),
      m_hasPendingWifiScanRequest(false),
      m_hasPendingPlatformRequest(false)
{
    std::memset(m_registrationIds, 0, sizeof(m_registrationIds));
    /* 广播名固定为“设备型号-芯片序列号”，方便 App 与二维码信息对应。 */
    m_advertisementName = buildAdvertisementName();
    /*
     * Handler 只描述“某个 BLE 指令由谁处理”。基础协议指令已由
     * CommandDispatcher 构造函数注册，这里补充认证和配网业务指令。
     */
    m_dispatcher.registerHandler(CMD_AUTH_BEGIN,
        [this](const CommandFrame &f) { return handleAuthBegin(f); });
    m_dispatcher.registerHandler(CMD_AUTH_PROVE,
        [this](const CommandFrame &f) { return handleAuthProve(f); });
    m_dispatcher.registerHandler(CMD_AUTH_GET_STATUS,
        [this](const CommandFrame &f) { return handleAuthStatus(f); });
    m_dispatcher.registerHandler(CMD_WIFI_SET_CREDENTIALS,
        [this](const CommandFrame &f) { return handleWifiCredentials(f); });
    m_dispatcher.registerHandler(CMD_WIFI_CONNECT,
        [this](const CommandFrame &f) { return handleWifiConnect(f); });
    m_dispatcher.registerHandler(CMD_WIFI_GET_STATUS,
        [this](const CommandFrame &f) { return handleWifiStatus(f); });
    m_dispatcher.registerHandler(CMD_WIFI_SCAN,
        [this](const CommandFrame &f) { return handleWifiScan(f); });
    m_dispatcher.registerHandler(CMD_PLATFORM_CONNECT,
        [this](const CommandFrame &f) { return handlePlatformConnect(f); });
    m_dispatcher.registerHandler(CMD_PLATFORM_GET_STATUS,
        [this](const CommandFrame &f) { return handlePlatformStatus(f); });
}

BleGattServer::~BleGattServer()
{
    stop();
}

bool BleGattServer::start()
{
    if (m_thread.joinable())
        return true;
    m_stopRequested.store(false);
    m_thread = std::thread(&BleGattServer::threadMain, this);
    return true;
}

void BleGattServer::stop()
{
    m_stopRequested.store(true);
    if (m_context && m_loop)
        g_main_context_invoke(m_context, [](gpointer data) -> gboolean {
            g_main_loop_quit(static_cast<GMainLoop *>(data));
            return G_SOURCE_REMOVE;
        }, m_loop);
    if (m_thread.joinable())
        m_thread.join();
}

bool BleGattServer::running() const
{
    return m_running.load();
}

void BleGattServer::registerCommandHandler(uint16_t command,
                                           const CommandDispatcher::Handler &handler)
{
    m_dispatcher.registerHandler(command, handler);
}

void BleGattServer::setExternalCommandSender(const ExternalCommandSender &sender)
{
    std::lock_guard<std::mutex> lock(m_flowMutex);
    m_externalSender = sender;
}

std::vector<uint8_t> BleGattServer::makeFlowStatusPayload(int state,
                                                           const std::string &detail) const
{
    std::ostringstream stream;
    stream << "state=" << state << ";detail=" << detail;
    const std::string text = stream.str();
    return std::vector<uint8_t>(text.begin(), text.end());
}

CommandResult BleGattServer::handleWifiCredentials(const CommandFrame &frame)
{
    CommandResult result;
    const std::string json(frame.payload.begin(), frame.payload.end());
    if (json.empty() || json.front() != '{' || json.back() != '}')
    {
        result.status = STATUS_INVALID_PAYLOAD;
        return result;
    }
    /* 此步骤只暂存凭据；真正连接由后续 CMD_WIFI_CONNECT 触发。 */
    std::lock_guard<std::mutex> lock(m_flowMutex);
    m_wifiCredentials = json;
    m_wifiState = 1; /* credentials ready */
    m_wifiDetail = "credentials_ready";
    return result;
}

CommandResult BleGattServer::handleWifiConnect(const CommandFrame &frame)
{
    CommandResult result;
    ExternalCommandSender sender;
    std::string credentials;
    {
        std::lock_guard<std::mutex> lock(m_flowMutex);
        if (m_wifiCredentials.empty() || !m_externalSender)
        {
            result.status = STATUS_INVALID_PAYLOAD;
            return result;
        }
        sender = m_externalSender;
        credentials = m_wifiCredentials;
        m_wifiState = 2; /* connecting */
        m_wifiDetail = "connecting";
        /* 保存原请求的 command/sequence，异步完成时据此生成对应响应。 */
        m_pendingWifiRequest = frame;
        m_hasPendingWifiRequest = true;
    }
    if (sender(CMD_WIFI_CONNECT, credentials) < 0)
    {
        std::lock_guard<std::mutex> lock(m_flowMutex);
        m_wifiState = 4;
        m_wifiDetail = "dispatch_failed";
        m_hasPendingWifiRequest = false;
        result.status = STATUS_INTERNAL_ERROR;
    }
    return result;
}

CommandResult BleGattServer::handleWifiStatus(const CommandFrame &)
{
    CommandResult result;
    std::lock_guard<std::mutex> lock(m_flowMutex);
    result.payload = makeFlowStatusPayload(m_wifiState, m_wifiDetail);
    return result;
}

CommandResult BleGattServer::handleWifiScan(const CommandFrame &frame)
{
    CommandResult result;
    ExternalCommandSender sender;
    {
        std::lock_guard<std::mutex> lock(m_flowMutex);
        if (!m_externalSender)
        {
            result.status = STATUS_INTERNAL_ERROR;
            return result;
        }
        /* 扫描耗时且底层通常不能并发执行，未完成前拒绝第二次扫描。 */
        if (m_hasPendingWifiScanRequest)
        {
            result.status = STATUS_BUSY;
            return result;
        }
        sender = m_externalSender;
        m_pendingWifiScanRequest = frame;
        m_hasPendingWifiScanRequest = true;
    }
    if (sender(CMD_WIFI_SCAN, "{}") < 0)
    {
        std::lock_guard<std::mutex> lock(m_flowMutex);
        m_hasPendingWifiScanRequest = false;
        result.status = STATUS_INTERNAL_ERROR;
    }
    return result;
}

CommandResult BleGattServer::handlePlatformConnect(const CommandFrame &frame)
{
    CommandResult result;
    ExternalCommandSender sender;
    std::string platformJson(frame.payload.begin(), frame.payload.end());
    {
        std::lock_guard<std::mutex> lock(m_flowMutex);
        if (m_wifiState != 3)
        {
            result.status = STATUS_BUSY;
            result.payload = makeFlowStatusPayload(m_wifiState, "wifi_not_connected");
            return result;
        }
        if (platformJson.empty() || platformJson.front() != '{' ||
            platformJson.back() != '}' || !m_externalSender)
        {
            result.status = STATUS_INVALID_PAYLOAD;
            return result;
        }
        sender = m_externalSender;
        m_platformState = 1; /* connecting */
        m_platformDetail = "connecting";
        m_pendingPlatformRequest = frame;
        m_hasPendingPlatformRequest = true;
    }
    if (sender(CMD_PLATFORM_CONNECT, platformJson) < 0)
    {
        std::lock_guard<std::mutex> lock(m_flowMutex);
        m_platformState = 3;
        m_platformDetail = "dispatch_failed";
        m_hasPendingPlatformRequest = false;
        result.status = STATUS_INTERNAL_ERROR;
    }
    return result;
}

CommandResult BleGattServer::handlePlatformStatus(const CommandFrame &)
{
    CommandResult result;
    std::lock_guard<std::mutex> lock(m_flowMutex);
    result.payload = makeFlowStatusPayload(m_platformState, m_platformDetail);
    return result;
}

CommandResult BleGattServer::handleAuthBegin(const CommandFrame &frame)
{
    return m_security.beginAuthentication(frame);
}

CommandResult BleGattServer::handleAuthProve(const CommandFrame &frame)
{
    return m_security.proveAuthentication(frame);
}

CommandResult BleGattServer::handleAuthStatus(const CommandFrame &)
{
    return m_security.authenticationStatus();
}

void BleGattServer::onExternalCommandResult(uint16_t command, bool success,
                                             const std::string &detail)
{
    /*
     * 此函数可能由业务线程调用。锁内只更新状态和取走 pending 请求，
     * publishResponse 放在锁外执行，避免 D-Bus 回调与业务锁相互等待。
     */
    CommandFrame pending;
    bool hasPending = false;
    bool rawPayload = false;
    {
        std::lock_guard<std::mutex> lock(m_flowMutex);
        if (command == CMD_WIFI_CONNECT)
        {
            m_wifiState = success ? 3 : 4;
            m_wifiDetail = detail;
            pending = m_pendingWifiRequest;
            hasPending = m_hasPendingWifiRequest;
            m_hasPendingWifiRequest = false;
        }
        else if (command == CMD_PLATFORM_CONNECT)
        {
            m_platformState = success ? 2 : 3;
            m_platformDetail = detail;
            pending = m_pendingPlatformRequest;
            hasPending = m_hasPendingPlatformRequest;
            m_hasPendingPlatformRequest = false;
        }
        else if (command == CMD_PLATFORM_GET_STATUS)
        {
            m_platformState = success ? 2 : 0;
            m_platformDetail = detail;
        }
        else if (command == CMD_WIFI_SCAN)
        {
            pending = m_pendingWifiScanRequest;
            hasPending = m_hasPendingWifiScanRequest;
            m_hasPendingWifiScanRequest = false;
            rawPayload = true;
        }
    }

    if (hasPending)
    {
        CommandResult result;
        result.status = success ? STATUS_OK : STATUS_INTERNAL_ERROR;
        /* Wi-Fi 扫描 detail 已经是约定的 JSON，直接返回；流程类结果统一封装。 */
        if (rawPayload)
            result.payload.assign(detail.begin(), detail.end());
        else
            result.payload = makeFlowStatusPayload(success ? 1 : 0, detail);
        publishResponse(pending, result);
    }
    if ((command == CMD_PLATFORM_CONNECT || command == CMD_PLATFORM_GET_STATUS) &&
        success)
        scheduleBluetoothShutdown();
}

void BleGattServer::scheduleBluetoothShutdown()
{
    bool expected = false;
    if (!m_shutdownScheduled.compare_exchange_strong(expected, true))
        return;
    if (!m_context)
    {
        m_shutdownScheduled.store(false);
        return;
    }
    dlog_info("[BLE GATT] WiFi和平台均已连接，3秒后关闭蓝牙");
    g_main_context_invoke(m_context, [](gpointer data) -> gboolean {
        BleGattServer *self = static_cast<BleGattServer *>(data);
        GSource *source = g_timeout_source_new_seconds(3);
        g_source_set_callback(source, [](gpointer timeoutData) -> gboolean {
            BleGattServer *server = static_cast<BleGattServer *>(timeoutData);
            server->m_powerOffOnStop = true;
            if (server->m_loop)
                g_main_loop_quit(server->m_loop);
            return G_SOURCE_REMOVE;
        }, self, NULL);
        g_source_attach(source, self->m_context);
        g_source_unref(source);
        return G_SOURCE_REMOVE;
    }, this);
}

void BleGattServer::queryPlatformStatus()
{
    ExternalCommandSender sender;
    {
        std::lock_guard<std::mutex> lock(m_flowMutex);
        /* 必须先完成WiFi连接，防止仅有线联网时提前结束蓝牙配网。 */
        if (m_wifiState != 3 || m_shutdownScheduled.load())
            return;
        sender = m_externalSender;
    }
    if (sender && sender(CMD_PLATFORM_GET_STATUS, "{}") < 0)
        dlog_warn("[BLE GATT] 查询平台连接状态发送失败");
}

void BleGattServer::startPlatformStatusMonitor()
{
    GSource *initial = g_timeout_source_new_seconds(5);
    g_source_set_callback(initial, [](gpointer data) -> gboolean {
        static_cast<BleGattServer *>(data)->queryPlatformStatus();
        return G_SOURCE_REMOVE;
    }, this, NULL);
    g_source_attach(initial, m_context);
    g_source_unref(initial);

    GSource *periodic = g_timeout_source_new_seconds(10);
    g_source_set_callback(periodic, [](gpointer data) -> gboolean {
        BleGattServer *server = static_cast<BleGattServer *>(data);
        if (server->m_stopRequested.load() || server->m_shutdownScheduled.load())
            return G_SOURCE_REMOVE;
        server->queryPlatformStatus();
        return G_SOURCE_CONTINUE;
    }, this, NULL);
    g_source_attach(periodic, m_context);
    g_source_unref(periodic);
}

std::string BleGattServer::buildAdvertisementName() const
{
    /* /proc/cpuinfo 的 Serial 在设备生命周期内稳定，可作为广播名后缀。 */
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    std::string serial;
    while (std::getline(cpuinfo, line))
    {
        if (line.find("Serial") == std::string::npos)
            continue;
        const std::size_t pos = line.find(':');
        if (pos != std::string::npos)
            serial = line.substr(pos + 1);
        break;
    }
    serial.erase(std::remove_if(serial.begin(), serial.end(),
        [](char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; }), serial.end());
    if (serial.empty())
        serial = "UNKNOWN";
    return std::string(MQTT_MODEL) + "-" + serial;
}

void BleGattServer::threadMain()
{
    /*
     * GDBus 对象和主循环放在独立线程中，避免 BLE 调用阻塞 operation 主线程。
     * 该线程的 context 同时负责 BlueZ 的反向属性读取和手机 GATT 请求。
     */
    m_context = g_main_context_new();
    g_main_context_push_thread_default(m_context);
    m_loop = g_main_loop_new(m_context, FALSE);

    unsigned int retryCount = 0;
    while (!m_stopRequested.load())
    {
        if (initializeBluez())
        {
            m_running.store(true);
            dlog_info("[BLE GATT] 配网服务已启动, name=%s service=%s",
                      m_advertisementName.c_str(), kServiceUuid);
            startPlatformStatusMonitor();
            g_main_loop_run(m_loop);
            break;
        }

        cleanupBluez();
        ++retryCount;
        dlog_warn("[BLE GATT] 初始化失败，2秒后重试，第%u次", retryCount);

        /* 分段等待，使进程退出时不必额外等待完整的重试间隔。 */
        for (int i = 0; i < 20 && !m_stopRequested.load(); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    m_running.store(false);
    cleanupBluez();
    g_main_loop_unref(m_loop);
    m_loop = NULL;
    g_main_context_pop_thread_default(m_context);
    g_main_context_unref(m_context);
    m_context = NULL;
}

bool BleGattServer::initializeBluez()
{
    if (!m_security.initialize())
    {
        dlog_error("[BLE安全] 加载或生成设备配网密钥失败");
        return false;
    }
    GError *error = NULL;
    m_connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (!m_connection)
    {
        dlog_warn("[BLE GATT] 使用默认地址连接 system D-Bus 失败: %s",
                  error && error->message ? error->message : "unknown error");
        if (error)
        {
            g_error_free(error);
            error = NULL;
        }

        /*
         * 目标系统的 dbus-daemon 监听 /run/dbus/system_bus_socket，部分
         * GLib 交叉编译配置仍默认使用 /var/run。显式地址作为兼容回退。
         */
        m_connection = g_dbus_connection_new_for_address_sync(
            "unix:path=/run/dbus/system_bus_socket",
            static_cast<GDBusConnectionFlags>(
                G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
                G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION),
            NULL, NULL, &error);
        if (!m_connection)
        {
            logGError("连接 /run/dbus/system_bus_socket 失败", error);
            return false;
        }
    }
    if (!registerObjects())
        return false;
    return registerWithBluez();
}

bool BleGattServer::registerObjects()
{
    /* 先将对象导出到 D-Bus，随后才能把 Application 路径注册给 BlueZ。 */
    GError *error = NULL;
    m_rootInfo = g_dbus_node_info_new_for_xml(kRootXml, &error);
    if (!m_rootInfo) { logGError("解析 ObjectManager XML 失败", error); return false; }
    m_advertisementInfo = g_dbus_node_info_new_for_xml(kAdvertisementXml, &error);
    if (!m_advertisementInfo) { logGError("解析广播 XML 失败", error); return false; }
    m_serviceInfo = g_dbus_node_info_new_for_xml(kServiceXml, &error);
    if (!m_serviceInfo) { logGError("解析服务 XML 失败", error); return false; }
    m_characteristicInfo = g_dbus_node_info_new_for_xml(kCharacteristicXml, &error);
    if (!m_characteristicInfo) { logGError("解析特征值 XML 失败", error); return false; }

    static const GDBusInterfaceVTable vtable = {
        &BleGattServer::handleMethodCall,
        &BleGattServer::handleGetProperty,
        NULL, {0}};

    const char *paths[] = {kApplicationPath, kAdvertisementPath, kServicePath,
                           kCommandPath, kResponsePath};
    GDBusInterfaceInfo *interfaces[] = {
        m_rootInfo->interfaces[0], m_advertisementInfo->interfaces[0],
        m_serviceInfo->interfaces[0], m_characteristicInfo->interfaces[0],
        m_characteristicInfo->interfaces[0]};
    for (int i = 0; i < 5; ++i)
    {
        m_registrationIds[i] = g_dbus_connection_register_object(
            m_connection, paths[i], interfaces[i], &vtable, this, NULL, &error);
        if (!m_registrationIds[i])
        {
            logGError("注册 D-Bus 对象失败", error);
            return false;
        }
    }
    return true;
}

bool BleGattServer::registerWithBluez()
{
    GError *error = NULL;

    /*
     * RegisterApplication/RegisterAdvertisement 可能已经在 BlueZ 端完成，
     * 但客户端因超时或连接切换没有收到成功应答。此时本地标志仍为 false，
     * 下一次重试就会收到 AlreadyExists。注册前按同一连接和对象路径做一次
     * 幂等清理；对象不存在时 BlueZ 返回错误，直接忽略即可。
     */
    GVariant *reply = g_dbus_connection_call_sync(
        m_connection, kBluezBus, kAdapterPath,
        "org.bluez.LEAdvertisingManager1", "UnregisterAdvertisement",
        g_variant_new("(o)", kAdvertisementPath), NULL,
        G_DBUS_CALL_FLAGS_NONE, 3000, NULL, &error);
    if (reply)
        g_variant_unref(reply);
    if (error)
    {
        g_error_free(error);
        error = NULL;
    }

    reply = g_dbus_connection_call_sync(
        m_connection, kBluezBus, kAdapterPath,
        "org.bluez.GattManager1", "UnregisterApplication",
        g_variant_new("(o)", kApplicationPath), NULL,
        G_DBUS_CALL_FLAGS_NONE, 3000, NULL, &error);
    if (reply)
        g_variant_unref(reply);
    if (error)
    {
        g_error_free(error);
        error = NULL;
    }
    m_advertisementRegistered = false;
    m_applicationRegistered = false;

    /* bluetoothd可能恢复为Powered=false；应用启动时显式开启适配器。 */
    reply = g_dbus_connection_call_sync(
        m_connection, kBluezBus, kAdapterPath,
        "org.freedesktop.DBus.Properties", "Set",
        g_variant_new("(ssv)", "org.bluez.Adapter1", "Powered",
                      g_variant_new_boolean(TRUE)),
        NULL, G_DBUS_CALL_FLAGS_NONE, 10000, NULL, &error);
    if (!reply)
    {
        logGError("开启蓝牙适配器失败", error);
        return false;
    }
    g_variant_unref(reply);

    reply = callBluezWithMainLoop(
        m_connection, m_context, kAdapterPath, "org.bluez.GattManager1",
        "RegisterApplication",
        g_variant_new("(o@a{sv})", kApplicationPath, emptyOptions()),
        10000, &error);
    if (!reply)
    {
        if (!isRemoteError(error, "org.bluez.Error.AlreadyExists"))
        {
            logGError("RegisterApplication 失败", error);
            return false;
        }
        dlog_warn("[BLE GATT] GATT Application已存在，复用现有注册");
        g_error_free(error);
        error = NULL;
    }
    else
    {
        g_variant_unref(reply);
    }
    m_applicationRegistered = true;

    reply = callBluezWithMainLoop(
        m_connection, m_context, kAdapterPath, "org.bluez.LEAdvertisingManager1",
        "RegisterAdvertisement",
        g_variant_new("(o@a{sv})", kAdvertisementPath, emptyOptions()),
        10000, &error);
    if (!reply)
    {
        if (!isRemoteError(error, "org.bluez.Error.AlreadyExists"))
        {
            logGError("RegisterAdvertisement 失败", error);
            return false;
        }
        dlog_warn("[BLE GATT] Advertisement已存在，复用现有注册");
        g_error_free(error);
        error = NULL;
    }
    else
    {
        g_variant_unref(reply);
    }
    m_advertisementRegistered = true;
    return true;
}

void BleGattServer::cleanupBluez()
{
    /* 新连接不能继承上一连接尚未组完的半包、MTU 或通知状态。 */
    m_decoder.reset();
    m_security.resetSession();
    m_notifying.store(false);
    m_attMtu.store(kDefaultAttMtu);
    {
        std::lock_guard<std::mutex> lock(m_responseMutex);
        m_lastResponse.clear();
    }
    {
        std::lock_guard<std::mutex> lock(m_flowMutex);
        m_wifiCredentials.clear();
        m_hasPendingWifiRequest = false;
        m_hasPendingWifiScanRequest = false;
        m_hasPendingPlatformRequest = false;
    }
    if (m_connection && m_advertisementRegistered)
    {
        GError *error = NULL;
        GVariant *reply = g_dbus_connection_call_sync(
            m_connection, kBluezBus, kAdapterPath,
            "org.bluez.LEAdvertisingManager1", "UnregisterAdvertisement",
            g_variant_new("(o)", kAdvertisementPath), NULL,
            G_DBUS_CALL_FLAGS_NONE, 3000, NULL, &error);
        if (reply)
            g_variant_unref(reply);
        else
            logGError("UnregisterAdvertisement 失败", error);
    }
    if (m_connection && m_applicationRegistered)
    {
        GError *error = NULL;
        GVariant *reply = g_dbus_connection_call_sync(
            m_connection, kBluezBus, kAdapterPath,
            "org.bluez.GattManager1", "UnregisterApplication",
            g_variant_new("(o)", kApplicationPath), NULL,
            G_DBUS_CALL_FLAGS_NONE, 3000, NULL, &error);
        if (reply)
            g_variant_unref(reply);
        else
            logGError("UnregisterApplication 失败", error);
    }
    m_advertisementRegistered = false;
    m_applicationRegistered = false;

    if (m_connection && m_powerOffOnStop)
    {
        GVariant *reply = g_dbus_connection_call_sync(
            m_connection, kBluezBus, kAdapterPath,
            "org.freedesktop.DBus.Properties", "Set",
            g_variant_new("(ssv)", "org.bluez.Adapter1", "Powered",
                          g_variant_new_boolean(FALSE)),
            NULL, G_DBUS_CALL_FLAGS_NONE, 3000, NULL, NULL);
        if (reply)
            g_variant_unref(reply);
        dlog_info("[BLE GATT] 配网完成，蓝牙适配器已关闭");
    }

    if (m_connection)
    {
        for (int i = 0; i < 5; ++i)
            if (m_registrationIds[i])
                g_dbus_connection_unregister_object(m_connection, m_registrationIds[i]);
        g_object_unref(m_connection);
        m_connection = NULL;
    }
    std::memset(m_registrationIds, 0, sizeof(m_registrationIds));
    if (m_rootInfo) g_dbus_node_info_unref(m_rootInfo);
    if (m_advertisementInfo) g_dbus_node_info_unref(m_advertisementInfo);
    if (m_serviceInfo) g_dbus_node_info_unref(m_serviceInfo);
    if (m_characteristicInfo) g_dbus_node_info_unref(m_characteristicInfo);
    m_rootInfo = m_advertisementInfo = m_serviceInfo = m_characteristicInfo = NULL;
}

void BleGattServer::handleFrame(const CommandFrame &frame)
{
    dlog_info("[BLE GATT] 收到指令 code=0x%04x seq=%u payload=%zu",
              static_cast<unsigned int>(frame.command),
              static_cast<unsigned int>(frame.sequence), frame.payload.size());
    /* 业务配网指令先通过安全层解密；认证/心跳等基础指令直接分发。 */
    if (BleSecurityManager::requiresSecureSession(frame.command))
    {
        CommandFrame decrypted = frame;
        StatusCode status = m_security.decryptProvisioningPayload(
            frame, decrypted.payload);
        if (status != STATUS_OK)
        {
            CommandResult result;
            result.status = status;
            publishResponse(frame, result);
            return;
        }
        publishResponse(frame, m_dispatcher.dispatch(decrypted));
        return;
    }
    publishResponse(frame, m_dispatcher.dispatch(frame));
}

void BleGattServer::publishResponse(const CommandFrame &request,
                                    const CommandResult &result)
{
    /* 一条响应的所有分片必须连续发送，不能与另一条异步响应交叉。 */
    std::lock_guard<std::mutex> publishLock(m_publishMutex);
    CommandFrame response;
    response.flags = 0x01;
    response.command = request.command;
    /* command 和 sequence 原样回显，App 可在并发请求中准确关联响应。 */
    response.sequence = request.sequence;
    response.payload = makeResponsePayload(result.status, result.payload);
    std::vector<uint8_t> encoded = FrameCodec::encode(response);
    if (encoded.empty())
    {
        dlog_error("[BLE GATT] 响应编码失败 code=0x%04x seq=%u payload=%zu",
                   static_cast<unsigned int>(request.command),
                   static_cast<unsigned int>(request.sequence),
                   response.payload.size());
        return;
    }
    {
        std::lock_guard<std::mutex> lock(m_responseMutex);
        m_lastResponse = encoded;
    }

    /*
     * 无论是否订阅 Notify 都保存最后响应；未订阅时 App 仍可 ReadValue。
     * 已订阅时通过 PropertiesChanged(Value) 让 bluetoothd 发送通知。
     */
    if (m_connection && m_notifying.load())
    {
        const unsigned int mtu = std::max(kDefaultAttMtu, m_attMtu.load());
        const std::size_t chunkLimit = mtu - kAttValueOverhead;
        const std::size_t chunkCount =
            (encoded.size() + chunkLimit - 1) / chunkLimit;
        dlog_info("[BLE GATT] 发送响应 code=0x%04x seq=%u frame=%zu mtu=%u chunks=%zu",
                  static_cast<unsigned int>(request.command),
                  static_cast<unsigned int>(request.sequence), encoded.size(),
                  mtu, chunkCount);

        /*
         * 每个通知只携带协议帧的一段原始字节，不额外嵌套分片头。
         * App 按协议头中的 payloadLength 计算整帧长度并连续累积即可。
         */
        for (std::size_t offset = 0; offset < encoded.size(); offset += chunkLimit)
        {
            const std::size_t length =
                std::min(chunkLimit, encoded.size() - offset);
            GVariantBuilder changed;
            GVariantBuilder invalidated;
            g_variant_builder_init(&changed, G_VARIANT_TYPE("a{sv}"));
            g_variant_builder_init(&invalidated, G_VARIANT_TYPE("as"));
            g_variant_builder_add(&changed, "{sv}", "Value",
                                  byteArrayVariant(encoded.data() + offset, length));
            GError *error = NULL;
            if (!g_dbus_connection_emit_signal(
                    m_connection, NULL, kResponsePath,
                    "org.freedesktop.DBus.Properties", "PropertiesChanged",
                    g_variant_new("(sa{sv}as)", "org.bluez.GattCharacteristic1",
                                  &changed, &invalidated), &error))
            {
                logGError("发送响应分片失败", error);
                break;
            }
        }
    }
}

void BleGattServer::handleMethodCall(GDBusConnection *, const char *,
                                     const char *objectPath, const char *interfaceName,
                                     const char *methodName, GVariant *parameters,
                                     GDBusMethodInvocation *invocation, void *userData)
{
    BleGattServer *self = static_cast<BleGattServer *>(userData);

    if (std::strcmp(interfaceName, "org.freedesktop.DBus.ObjectManager") == 0 &&
        std::strcmp(methodName, "GetManagedObjects") == 0)
    {
        std::lock_guard<std::mutex> lock(self->m_responseMutex);
        g_dbus_method_invocation_return_value(invocation,
            g_variant_new("(@a{oa{sa{sv}}})",
                          buildManagedObjects(self->m_notifying.load(), self->m_lastResponse)));
        return;
    }
    if (std::strcmp(interfaceName, "org.bluez.LEAdvertisement1") == 0 &&
        std::strcmp(methodName, "Release") == 0)
    {
        g_dbus_method_invocation_return_value(invocation, NULL);
        return;
    }
    if (std::strcmp(interfaceName, "org.bluez.GattCharacteristic1") != 0)
        goto unsupported;

    if (std::strcmp(methodName, "WriteValue") == 0 &&
        std::strcmp(objectPath, kCommandPath) == 0)
    {
        /* 一次 WriteValue 可能只是协议帧分片，由 FrameDecoder 负责缓存组帧。 */
        GVariant *value = NULL;
        GVariant *options = NULL;
        g_variant_get(parameters, "(@ay@a{sv})", &value, &options);
        guint16 mtu = 0;
        guint16 offset = 0;
        const gchar *writeType = "unknown";
        if (g_variant_lookup(options, "mtu", "q", &mtu) && mtu >= kDefaultAttMtu)
            self->m_attMtu.store(mtu);
        g_variant_lookup(options, "offset", "q", &offset);
        g_variant_lookup(options, "type", "&s", &writeType);

        /*
         * 本协议的分包由多个独立 WriteValue(offset=0) 构成。Prepare Write
         * 的非零 offset 不能和流式组帧混用，遇到时清空半包并明确拒绝。
         */
        if (offset != 0)
        {
            self->m_decoder.reset();
            g_variant_unref(value);
            g_variant_unref(options);
            dlog_warn("[BLE GATT] 拒绝非零写偏移 offset=%u，已清空接收缓存",
                      static_cast<unsigned int>(offset));
            g_dbus_method_invocation_return_dbus_error(
                invocation, "org.bluez.Error.InvalidOffset",
                "Protocol fragments must use independent writes with offset zero");
            return;
        }
        gsize size = 0;
        const uint8_t *bytes = static_cast<const uint8_t *>(
            g_variant_get_fixed_array(value, &size, sizeof(uint8_t)));
        const std::size_t bufferedBefore = self->m_decoder.bufferedSize();
        self->m_decoder.append(bytes, size,
            [self](const CommandFrame &frame) { self->handleFrame(frame); },
            [](StatusCode status) {
                dlog_warn("[BLE GATT] 指令帧解析失败 status=%u",
                          static_cast<unsigned int>(status));
            });
        dlog_info("[BLE GATT] 接收分片 type=%s size=%zu mtu=%u buffered=%zu->%zu",
                  writeType ? writeType : "unknown", static_cast<std::size_t>(size),
                  self->m_attMtu.load(), bufferedBefore,
                  self->m_decoder.bufferedSize());
        g_variant_unref(value);
        g_variant_unref(options);
        g_dbus_method_invocation_return_value(invocation, NULL);
        return;
    }
    if (std::strcmp(methodName, "ReadValue") == 0 &&
        std::strcmp(objectPath, kResponsePath) == 0)
    {
        GVariant *options = NULL;
        g_variant_get(parameters, "(@a{sv})", &options);
        guint16 offset = 0;
        guint16 mtu = 0;
        g_variant_lookup(options, "offset", "q", &offset);
        if (g_variant_lookup(options, "mtu", "q", &mtu) && mtu >= kDefaultAttMtu)
            self->m_attMtu.store(mtu);
        g_variant_unref(options);

        std::lock_guard<std::mutex> lock(self->m_responseMutex);
        if (offset > self->m_lastResponse.size())
        {
            g_dbus_method_invocation_return_dbus_error(
                invocation, "org.bluez.Error.InvalidOffset",
                "Read offset exceeds response length");
            return;
        }
        /* ATT Read Response 可承载 MTU-1 字节；客户端可用 offset 继续长读。 */
        const unsigned int effectiveMtu =
            std::max(kDefaultAttMtu, self->m_attMtu.load());
        const std::size_t available = self->m_lastResponse.size() - offset;
        const std::size_t length = std::min<std::size_t>(effectiveMtu - 1, available);
        const uint8_t *readData = length == 0
                                      ? NULL
                                      : self->m_lastResponse.data() + offset;
        g_dbus_method_invocation_return_value(invocation,
            g_variant_new("(@ay)",
                          byteArrayVariant(readData, length)));
        return;
    }
    if (std::strcmp(methodName, "StartNotify") == 0 &&
        std::strcmp(objectPath, kResponsePath) == 0)
    {
        /* App 应先订阅响应特征，再发送可能异步完成的配网指令。 */
        self->m_decoder.reset();
        {
            std::lock_guard<std::mutex> lock(self->m_responseMutex);
            self->m_lastResponse.clear();
        }
        self->m_notifying.store(true);
        g_dbus_method_invocation_return_value(invocation, NULL);
        return;
    }
    if (std::strcmp(methodName, "StopNotify") == 0 &&
        std::strcmp(objectPath, kResponsePath) == 0)
    {
        self->m_notifying.store(false);
        self->m_attMtu.store(kDefaultAttMtu);
        self->m_decoder.reset();
        /* 手机断开或停止通知后立即销毁认证会话、会话密钥及接收半包。 */
        self->m_security.resetSession();
        {
            std::lock_guard<std::mutex> lock(self->m_responseMutex);
            self->m_lastResponse.clear();
        }
        {
            std::lock_guard<std::mutex> lock(self->m_flowMutex);
            self->m_wifiCredentials.clear();
            self->m_hasPendingWifiRequest = false;
            self->m_hasPendingWifiScanRequest = false;
            self->m_hasPendingPlatformRequest = false;
        }
        g_dbus_method_invocation_return_value(invocation, NULL);
        return;
    }

unsupported:
    g_dbus_method_invocation_return_error(invocation, G_IO_ERROR,
                                          G_IO_ERROR_NOT_SUPPORTED,
                                          "Unsupported BLE method");
}

GVariant *BleGattServer::handleGetProperty(GDBusConnection *, const char *,
                                           const char *objectPath, const char *interfaceName,
                                           const char *propertyName, GError **, void *userData)
{
    BleGattServer *self = static_cast<BleGattServer *>(userData);
    if (std::strcmp(interfaceName, "org.bluez.LEAdvertisement1") == 0)
    {
        if (std::strcmp(propertyName, "Type") == 0)
            return g_variant_new_string("peripheral");
        if (std::strcmp(propertyName, "ServiceUUIDs") == 0)
        {
            const char *uuids[] = {kServiceUuid, NULL};
            return g_variant_new_strv(uuids, -1);
        }
        if (std::strcmp(propertyName, "LocalName") == 0)
            return g_variant_new_string(self->m_advertisementName.c_str());
    }
    if (std::strcmp(interfaceName, "org.bluez.GattService1") == 0)
    {
        if (std::strcmp(propertyName, "UUID") == 0)
            return g_variant_new_string(kServiceUuid);
        if (std::strcmp(propertyName, "Primary") == 0)
            return g_variant_new_boolean(TRUE);
    }
    if (std::strcmp(interfaceName, "org.bluez.GattCharacteristic1") == 0)
    {
        if (std::strcmp(propertyName, "UUID") == 0)
            return g_variant_new_string(std::strcmp(objectPath, kCommandPath) == 0
                                            ? kCommandUuid : kResponseUuid);
        if (std::strcmp(propertyName, "Service") == 0)
            return g_variant_new_object_path(kServicePath);
        if (std::strcmp(propertyName, "Flags") == 0)
        {
            const char *writeFlags[] = {"write", "write-without-response", NULL};
            const char *responseFlags[] = {"read", "notify", NULL};
            return g_variant_new_strv(std::strcmp(objectPath, kCommandPath) == 0
                                          ? writeFlags : responseFlags, -1);
        }
        if (std::strcmp(propertyName, "Value") == 0)
        {
            std::lock_guard<std::mutex> lock(self->m_responseMutex);
            return byteArrayVariant(self->m_lastResponse);
        }
        if (std::strcmp(propertyName, "Notifying") == 0)
            return g_variant_new_boolean(self->m_notifying.load());
    }
    return NULL;
}

} // namespace BleProvisioning
