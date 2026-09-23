/**
 * @FilePath     : log_sink.h
 * @Author       : zhouzr@kfb.cn
 * @Description  : ipc_rtsp 库日志 -> 产品 dlog 转发
 *
 * 提供给 ipc_rtsp::SetLogSink 的回调；转发限流策略见 log_sink.cpp 实现。
 */
#pragma once

#include "ipc_rtsp/log.h"

namespace rtsp_smol
{

/**
 * 库日志回调：把 ipc_rtsp 日志按级别转发到 dlog。
 * 通过 ipc_rtsp::SetLogSink(&log_sink, nullptr) 注册。
 */
void log_sink(void *user, ipc_rtsp::LogLevel level, const char *tag, const char *message);

} // namespace rtsp_smol
