/**
 * @FilePath     : controller.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : RTSP 控制面：方法分发与各方法语义
 */

/*
 * smolrtsp 默认 dispatch 只覆盖 OPTIONS/DESCRIBE/SETUP/PLAY/TEARDOWN，
 * GET_PARAMETER/PAUSE/SET_PARAMETER 会落到 unknown()；本控制器直接按方法分派，
 * 把这几个 IPC 常用方法作为一等公民实现。
 */
#pragma once

#include "ipc_rtsp/result.h"
#include "ipc_rtsp/types.h"

#include <string>

struct ipc_rtsp_shim_request;

namespace ipc_rtsp
{
namespace detail
{

class Connection;

/** 请求控制器。 */
class Controller
{
public:
    /**
     * @brief 构造控制器。
     *
     * @param connection 所属连接，非拥有；Connection 以 unique_ptr 持有本对象，
     *                   生命周期必然覆盖本对象
     * @return {void}
     */
    explicit Controller(Connection *connection);

    /**
     * @brief 处理一个已解析完成的请求，按方法分发到各 Handle* 分支。
     *
     * @param request 已解析的请求（shim 层句柄），仅本次调用内有效
     * @return 仅当 request 或 connection 为空时返回 false；其余情况一律返回 true，
     *         业务错误（401/404/453 等）已在内部应答，需要断连时内部自行关闭，
     *         调用方不需要为此关闭连接
     */
    bool Handle(const ipc_rtsp_shim_request *request);

    /**
     * @brief 生成并发送 DESCRIBE 响应（调用方需先准备好响应上下文）。
     *
     * 参数集未就绪时挂起请求；供正常 DESCRIBE 与延迟补发共用。
     *
     * @param stream 目标码流
     * @return true 表示已回 200；false 分三种（均已完成应答或挂起）：
     *         码流不存在（已回 404）、参数集未就绪（已挂起延迟补发）、
     *         SDP 生成失败（已回 500）
     */
    bool SendDescribe(StreamId stream);

private:
    /** @brief 处理 DESCRIBE：解析码流后交 SendDescribe 生成 SDP 应答。 */
    void HandleDescribe();
    /** @brief 处理 OPTIONS：回 Public 方法列表。 */
    void HandleOptions();

    /** @brief 处理 SETUP：解析 Transport、准入校验并建立传输。 */
    void HandleSetup();
    /** @brief 处理 PLAY：attach 各 track 订阅并回 RTP-Info。 */
    void HandlePlay();
    /** @brief 处理 PAUSE：暂停全部已 SETUP 的 track。 */
    void HandlePause();
    /** @brief 处理 TEARDOWN：释放全部 track 并复位会话。 */
    void HandleTeardown();
    /** @brief 处理 GET_PARAMETER：作为 keepalive，回 200。 */
    void HandleGetParameter();
    /** @brief 处理 SET_PARAMETER：接受并忽略参数，回 200。 */
    void HandleSetParameter();
    /** @brief 处理未支持的方法：回 405 与 Allow 列表。 */
    void HandleUnknown(const std::string &method);

    /**
     * @brief 统一鉴权：失败时回 401，返回 false。
     *
     * @param anonymous_allowed 该方法是否允许匿名；且仅当配置
     *        auth.allow_anonymous_options 为 true 时匿名才实际放行
     * @return 鉴权通过返回 true；失败已回 401（必要时附带新 challenge）返回 false
     */
    bool CheckAuth(bool anonymous_allowed);

    /**
     * @brief 读取当前请求的指定头。
     *
     * @param name 头名称
     * @param value 输出头值；可为 nullptr（仅探测存在性）
     * @return 头存在返回 true，不存在（或无待处理请求）返回 false
     */
    bool Header(const char *name, std::string *value) const;

    /* 所属连接（宿主对象，持有本对象的 unique_ptr），非拥有；生命周期覆盖本对象。 */
    Connection *connection_ = nullptr;
};

} // namespace detail
} // namespace ipc_rtsp
