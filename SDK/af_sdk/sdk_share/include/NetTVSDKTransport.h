/**
 * @file NetTVSDKTransport.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief NetTVSDKTransport 模块接口与类型定义
 * 功能说明：
 * 1. 声明 NetTVSDKTransport 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#pragma once



#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace nettv
{

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 传输协议类型。
 */
enum class TransportType_EN
{
    HTTP = 0,
    WEBSOCKET = 1,
    MQTT = 2,
    CUSTOM = 100
};

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 协议无关请求对象。
 *
 * HTTP 适配器可将 method/path/header/query/body 映射到该结构；
 * WebSocket/MQTT 可将消息体中的 action/path/requestId/sessionId 映射到该结构。
 */
struct RpcRequest_S
{
    std::string strRequestId;
    std::string strMethod;
    std::string strPath;
    std::string strSessionId;
    std::string strContentType;
    std::map<std::string, std::string> stHeaders;
    std::map<std::string, std::string> stQuery;
    std::string strBody;
    std::vector<unsigned char> aBinary;
};

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 协议无关响应对象。
 */
struct RpcResponse_S
{
    int nCode{200};
    std::string strMessage;
    std::string strContentType{"application/json"};
    std::map<std::string, std::string> stHeaders;
    std::string strBody;
    std::vector<unsigned char> aBinary;
};

using RpcHandler_FN = std::function<void(const RpcRequest_S&, RpcResponse_S&)>;

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 协议无关路由描述。
 *
 * 业务层只注册 RouteDescriptor_S。HTTP/WebSocket/MQTT 适配器负责把各自协议
 * 的请求转换为 RpcRequest_S 后分发到回调函数。
 */
struct RouteDescriptor_S
{
    std::string strPath;
    std::string strMethod;
    RpcHandler_FN fnHandler;
    bool bSupportBinary{false};
    bool bLongConnection{false};
};

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 客户端传输通道抽象。
 */
class CClientTransport
{
public:
    virtual ~CClientTransport() = default;

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 连接远端传输端点。
     * @param [in] strHost 远端主机地址。
     * @param [in] uPort 远端端口。
     * @return 连接成功返回 true，否则返回 false。
     */
    virtual bool Connect(const std::string& strHost, std::uint16_t uPort) = 0;

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 关闭当前传输连接。
     * @return 无。
     */
    virtual void Close() = 0;

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 发送统一请求并接收统一响应。
     * @param [in] stRequest 请求数据。
     * @param [out] stResponse 响应数据。
     * @return 发送和响应解析成功返回 true，否则返回 false。
     */
    virtual bool Send(const RpcRequest_S& stRequest, RpcResponse_S& stResponse) = 0;
    virtual TransportType_EN GetType() const = 0;
    virtual const char* GetName() const = 0;
};

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 服务端传输通道抽象。
 */
class CServerTransport
{
public:
    virtual ~CServerTransport() = default;

    virtual int Start(const std::string& strHost, std::uint16_t uPort) = 0;
    virtual int Stop() = 0;
    virtual TransportType_EN GetType() const = 0;
    virtual const char* GetName() const = 0;
};

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 事件通道抽象，用于告警、状态变化、语音等推送/订阅场景。
 */
class CEventChannel
{
public:
    virtual ~CEventChannel() = default;

    virtual bool Subscribe(const std::string& strTopic) = 0;
    virtual bool Unsubscribe(const std::string& strTopic) = 0;
    virtual bool Publish(const std::string& strTopic, const std::string& strPayload) = 0;
};

} /* namespace nettv */
