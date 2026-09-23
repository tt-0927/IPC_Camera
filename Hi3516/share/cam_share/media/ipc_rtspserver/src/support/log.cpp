/**
 * @FilePath     : log.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 日志 sink 实现
 */

#include "support/log.h"

#include <atomic>
#include <cstdarg>
#include <cstdio>

namespace ipc_rtsp
{
namespace
{
/** 单条日志的最大格式化长度；超长截断，避免污染栈。 */
constexpr std::size_t kLogBufferBytes = 512;

/* 当前日志 sink；nullptr 表示日志关闭。 */
std::atomic<LogSink> g_sink{ nullptr };
/* sink 回调的用户指针；与 g_sink 配套写入（顺序见 SetLogSink）。 */
std::atomic<void *> g_sink_user{ nullptr };
/* 最低输出级别（存 int 以便原子操作；数值越小越严重）。 */
std::atomic<int> g_level{ static_cast<int>(LogLevel::Info) };
} // namespace

void SetLogSink(LogSink sink, void *user)
{
    /* 先存 user 后存 sink：读到非空 sink 的调用方拿到的 user 只会是配套值或
     * 更新值，缩小“新 sink 配旧 user”的错配窗口。 */
    g_sink_user.store(user, std::memory_order_relaxed);
    g_sink.store(sink, std::memory_order_relaxed);
}

void SetLogLevel(LogLevel level)
{
    g_level.store(static_cast<int>(level), std::memory_order_relaxed);
}

LogLevel GetLogLevel()
{
    return static_cast<LogLevel>(g_level.load(std::memory_order_relaxed));
}

namespace detail
{

void LogMessage(LogLevel level, const char *tag, const char *fmt, ...)
{
    const LogSink sink = g_sink.load(std::memory_order_relaxed);
    if (sink == nullptr)
    {
        return;
    }
    if (static_cast<int>(level) > g_level.load(std::memory_order_relaxed))
    {
        return;
    }

    char buffer[kLogBufferBytes];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, sizeof buffer, fmt, ap);
    va_end(ap);

    sink(g_sink_user.load(std::memory_order_relaxed), level, tag, buffer);
}

} // namespace detail
} // namespace ipc_rtsp
