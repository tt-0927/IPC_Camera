/**
 * @FilePath     : result.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 统一错误码与返回类型
 */
#pragma once

#include <cerrno>

namespace ipc_rtsp
{

/**
 * 库返回状态。
 *
 * 约定：库内不抛异常，所有 API 都用 Result 表达失败；`sys_errno` 只在
 * `Status::IoError` 等由系统调用直接导致的失败时填充有效值，其余场景为 0。
 */
enum class Status
{
    Ok = 0,             /**< 成功 */
    InvalidArgument,    /**< 参数非法（长度、通道、空指针） */
    NotInitialized,     /**< 服务未初始化或已停止 */
    AlreadyInitialized, /**< 重复初始化 */
    Busy,               /**< 资源正被占用（如正在关停） */
    NoCapacity,         /**< 容量上限（队列满、连接数满、端口耗尽） */
    NotFound,           /**< 对象不存在（通道、会话） */
    Unsupported,        /**< 当前配置不支持（codec、传输方式） */
    IoError,            /**< 系统调用失败，见 sys_errno */
    AuthFailed,         /**< 鉴权失败 */
    Timeout,            /**< 操作超时 */
    ShuttingDown,       /**< 正在关停，拒绝新请求 */
    Internal,           /**< 内部错误（不变量被破坏） */
};

/** API 返回值：状态 + 可选 errno。 */
struct Result
{
    /* 业务状态码。 */
    Status code = Status::Ok;
    /* 系统调用 errno；仅 IoError 等系统调用失败场景有效，其余为 0。 */
    int sys_errno = 0;

    /**
     * 构造成功结果。
     *
     * @return code=Ok 且 sys_errno=0 的 Result
     */
    static Result Ok()
    {
        return Result{ Status::Ok, 0 };
    }

    /**
     * 构造失败结果。
     *
     * @param status   失败状态码
     * @param error_no 可选的系统 errno；0 表示不携带
     * @return 携带给定状态与 errno 的 Result
     */
    static Result Fail(Status status, int error_no = 0)
    {
        Result r;
        r.code = status;
        r.sys_errno = error_no;
        return r;
    }

    /**
     * 构造带 errno 的 I/O 失败结果。
     *
     * @param error_no 系统调用返回的 errno
     * @return code=IoError 且携带 error_no 的 Result
     */
    static Result Io(int error_no)
    {
        return Fail(Status::IoError, error_no);
    }

    /**
     * 是否成功。
     *
     * @return code 为 Ok 时返回 true
     */
    bool ok() const
    {
        return code == Status::Ok;
    }

    /**
     * 显式转 bool，便于 `if (result)` 写法。
     *
     * @return 等价于 ok()
     */
    explicit operator bool() const
    {
        return ok();
    }
};

/**
 * 状态码对应的中文短描述，用于日志。
 *
 * @param status 状态码
 * @return 静态字符串，生命周期不限；未知值返回 "未知状态"
 */
inline const char *StatusToString(Status status)
{
    switch (status)
    {
    case Status::Ok:
        return "成功";
    case Status::InvalidArgument:
        return "参数非法";
    case Status::NotInitialized:
        return "未初始化";
    case Status::AlreadyInitialized:
        return "重复初始化";
    case Status::Busy:
        return "资源忙";
    case Status::NoCapacity:
        return "容量上限";
    case Status::NotFound:
        return "对象不存在";
    case Status::Unsupported:
        return "不支持";
    case Status::IoError:
        return "系统调用失败";
    case Status::AuthFailed:
        return "鉴权失败";
    case Status::Timeout:
        return "超时";
    case Status::ShuttingDown:
        return "正在关停";
    case Status::Internal:
        return "内部错误";
    }
    return "未知状态";
}

} // namespace ipc_rtsp
