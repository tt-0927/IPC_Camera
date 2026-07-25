#pragma once

/**
 * @file NetTVSDKTransport.h
 * @brief SDK 协议无关通信抽象。
 *
 * 该头文件只定义 HTTP/WebSocket/MQTT 等传输协议共同使用的请求、响应、
 * 路由和传输接口，不依赖 httplib 或其他具体协议库，供客户端、服务端以及
 * sdk_share 共同引用。
 */

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace nettv
{

/**
 * @brief 传输协议类型。
 */
enum class TransportType
{
    HTTP = 0,
    WEBSOCKET = 1,
    MQTT = 2,
    CUSTOM = 100
};

/**
 * @brief 协议无关请求对象。
 *
 * HTTP 适配器可将 method/path/header/query/body 映射到该结构；
 * WebSocket/MQTT 可将消息体中的 action/path/requestId/sessionId 映射到该结构。
 */
struct RpcRequest
{
    std::string requestId;                         ///< 请求唯一标识，用于 WebSocket/MQTT 等异步协议匹配响应
    std::string method;                            ///< 逻辑动作，例如 GET/POST/PUT/DELETE/query/set/event
    std::string path;                              ///< 资源路径或命令名，例如 /ISAPI/xxx 或 device.config.get
    std::string sessionId;                         ///< 会话标识
    std::string contentType;                       ///< 内容类型，例如 application/json、application/octet-stream
    std::map<std::string, std::string> headers;    ///< 统一头字段
    std::map<std::string, std::string> query;      ///< 统一查询参数
    std::string body;                              ///< 文本请求体，通常为 JSON
    std::vector<unsigned char> binary;             ///< 二进制请求体，例如文件上传、音频帧
};

/**
 * @brief 协议无关响应对象。
 */
struct RpcResponse
{
    int code = 200;                                ///< 业务/协议状态码，HTTP 可直接映射到 status
    std::string message;                           ///< 状态描述或错误描述
    std::string contentType = "application/json";  ///< 内容类型
    std::map<std::string, std::string> headers;    ///< 统一响应头字段
    std::string body;                              ///< 文本响应体，通常为 JSON
    std::vector<unsigned char> binary;             ///< 二进制响应体
};

using RpcHandler = std::function<void(const RpcRequest&, RpcResponse&)>;

/**
 * @brief 协议无关路由描述。
 *
 * 业务层只注册 RouteDescriptor。HTTP/WebSocket/MQTT 适配器负责把各自协议
 * 的请求转换为 RpcRequest 后分发到 handler。
 */
struct RouteDescriptor
{
    std::string path;                              ///< 资源路径或命令名
    std::string method;                            ///< 逻辑动作，例如 GET/POST/PUT/DELETE/query/set
    RpcHandler handler;                            ///< 业务处理函数
    bool supportBinary = false;                    ///< 是否支持二进制输入/输出
    bool longConnection = false;                   ///< 是否为长连接/订阅类路由，例如告警监听
};

/**
 * @brief 客户端传输通道抽象。
 */
class IClientTransport
{
public:
    virtual ~IClientTransport() = default;

    virtual bool connect(const std::string& host, std::uint16_t port) = 0;
    virtual void close() = 0;
    virtual bool send(const RpcRequest& request, RpcResponse& response) = 0;
    virtual TransportType type() const = 0;
    virtual const char* name() const = 0;
};

/**
 * @brief 服务端传输通道抽象。
 */
class IServerTransport
{
public:
    virtual ~IServerTransport() = default;

    virtual int start(const std::string& host, std::uint16_t port) = 0;
    virtual int stop() = 0;
    virtual TransportType type() const = 0;
    virtual const char* name() const = 0;
};

/**
 * @brief 事件通道抽象，用于告警、状态变化、语音等推送/订阅场景。
 */
class IEventChannel
{
public:
    virtual ~IEventChannel() = default;

    virtual bool subscribe(const std::string& topic) = 0;
    virtual bool unsubscribe(const std::string& topic) = 0;
    virtual bool publish(const std::string& topic, const std::string& payload) = 0;
};

} // namespace nettv
