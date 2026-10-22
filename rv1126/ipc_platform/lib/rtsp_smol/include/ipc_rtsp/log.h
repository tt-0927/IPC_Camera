/**
 * @FilePath     : log.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 日志接入点
 */
/*
 * 库本身不写文件、不直接用 printf：由调用方注入 sink，把日志汇入产品 dlog。
 * 热路径建议只用 Error/Warn；默认输出级别为 Info（可用 SetLogLevel 调整），
 * 关键路径日志由库内限频。
 */
#pragma once

#include "ipc_rtsp/export.h"

namespace ipc_rtsp
{

/** 日志级别。 */
enum class LogLevel
{
    Error = 0, /* 错误：功能失败、必须关注 */
    Warn,      /* 警告：降级、拒绝等异常但可恢复 */
    Info,      /* 信息：生命周期等关键事件（默认输出级别） */
    Debug,     /* 调试：详细诊断信息，默认级别下被丢弃 */
};

/**
 * 日志回调。
 *
 * @param user  注册时传入的用户指针
 * @param level 级别
 * @param tag   模块标签（固定字符串字面量，生命周期不限）
 * @param msg   已格式化完成的消息（回调内可立即使用，返回后失效）
 */
using LogSink = void (*)(void *user, LogLevel level, const char *tag, const char *msg);

/**
 * 注册日志 sink；传 nullptr 表示关闭日志。
 *
 * @param sink 日志回调函数；nullptr 表示关闭日志
 * @param user 注册时透传给回调的用户指针
 */
IPC_RTSP_API void SetLogSink(LogSink sink, void *user);

/**
 * 设置最低输出级别。
 *
 * @param level 低于该级别的日志被丢弃；默认 Info
 */
IPC_RTSP_API void SetLogLevel(LogLevel level);

/**
 * 读取当前最低输出级别。
 *
 * @return 当前最低输出级别
 */
IPC_RTSP_API LogLevel GetLogLevel();

} // namespace ipc_rtsp
