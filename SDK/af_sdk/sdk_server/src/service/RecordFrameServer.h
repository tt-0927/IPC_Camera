/**
 * @file RecordFrameServer.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CRecordFrameServer 模块接口与类型定义
 * 功能说明：
 * 1. 声明 CRecordFrameServer 模块对外接口和数据类型
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
#include <vector>

#include "NetTVSDKCommon.h"
#include "PlatformCompat.h"

namespace tvsdk {

using RecordFrameStartCallback = std::function<NET_COMMON_ECODE_E(const NET_RECORD_FRAME_STREAM_COND_S& cond,
                                                                     NET_RECORD_FRAME_STREAM_INFO_S& info)>;
using RecordFrameReadCallback = std::function<int(const std::string& stream_id,
                                                  NET_RECORD_FRAME_INFO_S& frame_info,
                                                  char* buffer,
                                                  size_t buffer_size)>;
using RecordFrameStopCallback = std::function<NET_COMMON_ECODE_E(const std::string& stream_id)>;

class CRecordFrameServer {
public:
    static CRecordFrameServer* instance();

    bool start(int port = 9007);
    void stop();

    int port() const { return m_nPort; }
    bool is_running() const { return m_bRunning; }

    void set_start_callback(RecordFrameStartCallback cb);
    void set_read_callback(RecordFrameReadCallback cb);
    void set_stop_callback(RecordFrameStopCallback cb);

    NET_COMMON_ECODE_E open_stream(const NET_RECORD_FRAME_STREAM_COND_S& cond,
                                      NET_RECORD_FRAME_STREAM_INFO_S& info);
    NET_COMMON_ECODE_E close_stream(const std::string& stream_id);

private:
    CRecordFrameServer();
    ~CRecordFrameServer();

    void accept_loop();
    void client_send_loop(socket_fd_t client_fd);
    bool send_packet(socket_fd_t fd, const NET_RECORD_FRAME_INFO_S& frame_info, const char* payload);
    std::string current_stream_id() const;

    socket_fd_t m_nListenFd{INVALID_SOCKET_FD};
    std::atomic<bool> m_bRunning{false};
    std::thread m_stAcceptThread;
    std::thread m_stClientThread;
    mutable std::mutex m_stClientMutex;
    socket_fd_t m_nClientFd{INVALID_SOCKET_FD};
    int m_nPort{9007};

    mutable std::mutex m_stStreamMutex;
    std::string m_strStreamId;
    uint32_t m_uSsrc{1};

    mutable std::mutex m_stCallbackMutex;
    RecordFrameStartCallback m_fnStartCallback;
    RecordFrameReadCallback m_fnReadCallback;
    RecordFrameStopCallback m_fnStopCallback;
};

}  /* namespace tvsdk */
