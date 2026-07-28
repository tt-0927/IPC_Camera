/**
 * @file RecordFrameClient.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CRecordFrameClient 模块接口与类型定义
 * 功能说明：
 * 1. 声明 CRecordFrameClient 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "NetTVSDKCommon.h"
#include "PlatformCompat.h"

namespace tvsdk {

using RecordFrameCallback = std::function<void(const NET_RECORD_FRAME_INFO_S& frame_info,
                                               const char* data,
                                               size_t size)>;

class CRecordFrameClient {
public:
    CRecordFrameClient();
    ~CRecordFrameClient();

    bool start(const std::string& host,
               int port,
               const std::string& stream_id,
               RecordFrameCallback callback);
    void stop();

    bool is_running() const { return m_bRunning; }
    const std::string& stream_id() const { return m_strStreamId; }

private:
    void recv_loop();

    socket_fd_t m_nSocket{INVALID_SOCKET_FD};
    std::atomic<bool> m_bRunning{false};
    std::string m_strStreamId;
    RecordFrameCallback m_fnCallback;
    std::thread m_stReceiveThread;
    std::mutex m_stStopMutex;
};

}  /* namespace tvsdk */
