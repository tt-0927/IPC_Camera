/**
 * @FilePath     : export.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : ipc_rtspserver 版本与编译期导出宏
 */
#pragma once

#define IPC_RTSP_VERSION_MAJOR 0
#define IPC_RTSP_VERSION_MINOR 1
#define IPC_RTSP_VERSION_PATCH 0

#if defined(_WIN32)
#define IPC_RTSP_API
#else
#define IPC_RTSP_API __attribute__((visibility("default")))
#endif

/**
 * 本库默认以 `-fno-exceptions -fno-rtti` 编译，公共接口不抛异常、不依赖 RTTI：
 * 所有失败都通过 ipc_rtsp::Result 返回，调用方不应依赖异常做控制流。
 */
