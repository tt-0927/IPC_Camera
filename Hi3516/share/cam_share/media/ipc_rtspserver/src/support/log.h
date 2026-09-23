/**
 * @FilePath     : log.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 库内日志转发
 */
/*
 * 库不直接写 stdout/文件：统一转发给调用方注入的 sink。热路径应避免调用，
 * 或由调用点自行限频。
 */
#pragma once

#include "ipc_rtsp/log.h"

namespace ipc_rtsp
{
namespace detail
{

/**
 * 按 printf 格式格式化并转发到当前 sink。
 *
 * @param level 日志级别
 * @param tag   模块标签（静态字符串，生命周期不限）
 * @param fmt   printf 风格格式串，后接可变参数
 * @return {void} 无返回值
 * @note sink 未注册或级别低于当前阈值时直接丢弃，不做任何格式化。
 */
void LogMessage(LogLevel level, const char *tag, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

} // namespace detail
} // namespace ipc_rtsp

/** 记录一条 Error 日志；致命失败必须可见，热路径可少量使用。 */
#define IPC_RTSP_LOGE(tag, ...) ::ipc_rtsp::detail::LogMessage(::ipc_rtsp::LogLevel::Error, tag, __VA_ARGS__)

/** 记录一条 Warn 日志；热路径使用须自行限频（如 static 时间戳节流）。 */
#define IPC_RTSP_LOGW(tag, ...) ::ipc_rtsp::detail::LogMessage(::ipc_rtsp::LogLevel::Warn, tag, __VA_ARGS__)

/** 记录一条 Info 日志；仅限低频生命周期事件，热路径禁止使用。 */
#define IPC_RTSP_LOGI(tag, ...) ::ipc_rtsp::detail::LogMessage(::ipc_rtsp::LogLevel::Info, tag, __VA_ARGS__)

/** 记录一条 Debug 日志；默认级别 Info 下被丢弃，仅调试期临时使用。 */
#define IPC_RTSP_LOGD(tag, ...) ::ipc_rtsp::detail::LogMessage(::ipc_rtsp::LogLevel::Debug, tag, __VA_ARGS__)
