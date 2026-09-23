/**
 * @FilePath     : log_sink.cpp
 * @Author       : zhouzr@kfb.cn
 * @Description  : ipc_rtsp 库日志 -> 产品 dlog 转发实现（见 log_sink.h）
 */
#include "log_sink.h"

#include <atomic>

#include "dlog.h"

namespace
{

/*
 * dlog 按调用点（文件+行号）限流 1 秒 1 条；库日志若共用单个 dlog 调用点，
 * 启动阶段的生命周期/产物身份等突发会被整秒丢弃（板端实测 build-id 日志缺失）。
 * 下面按级别轮转 4 个物理调用点：突发可观测，总量仍受每秒 4 条/级别约束。
 * 注意：4 个 case 内容相同是刻意的，不能合并或用函数封装（会退化回单调用点）。
 */
std::atomic<unsigned> g_log_rotate_warn{ 0 };
std::atomic<unsigned> g_log_rotate_info{ 0 };

} // namespace

namespace rtsp_smol
{

void log_sink(void *user, ipc_rtsp::LogLevel level, const char *tag, const char *message)
{
    (void) user;
    switch (level)
    {
    case ipc_rtsp::LogLevel::Error:
        dlog_error("[rtsp:%s] %s", tag, message);
        break;
    case ipc_rtsp::LogLevel::Warn:
        switch (g_log_rotate_warn.fetch_add(1, std::memory_order_relaxed) % 4u)
        {
        case 0:
            dlog_warn("[rtsp:%s] %s", tag, message);
            break;
        case 1:
            dlog_warn("[rtsp:%s] %s", tag, message);
            break;
        case 2:
            dlog_warn("[rtsp:%s] %s", tag, message);
            break;
        default:
            dlog_warn("[rtsp:%s] %s", tag, message);
            break;
        }
        break;
    case ipc_rtsp::LogLevel::Info:
        switch (g_log_rotate_info.fetch_add(1, std::memory_order_relaxed) % 4u)
        {
        case 0:
            dlog_info("[rtsp:%s] %s", tag, message);
            break;
        case 1:
            dlog_info("[rtsp:%s] %s", tag, message);
            break;
        case 2:
            dlog_info("[rtsp:%s] %s", tag, message);
            break;
        default:
            dlog_info("[rtsp:%s] %s", tag, message);
            break;
        }
        break;
    default:
        dlog_debug("[rtsp:%s] %s", tag, message);
        break;
    }
}

} // namespace rtsp_smol
