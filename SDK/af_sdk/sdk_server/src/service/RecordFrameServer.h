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

using RecordFrameStartCallback = std::function<NET_TV_COMMON_ECODE_E(const NET_TV_RECORD_FRAME_STREAM_COND_S& cond,
                                                                     NET_TV_RECORD_FRAME_STREAM_INFO_S& info)>;
using RecordFrameReadCallback = std::function<int(const std::string& stream_id,
                                                  NET_TV_RECORD_FRAME_INFO_S& frame_info,
                                                  char* buffer,
                                                  size_t buffer_size)>;
using RecordFrameStopCallback = std::function<NET_TV_COMMON_ECODE_E(const std::string& stream_id)>;

class RecordFrameServer {
public:
    static RecordFrameServer* instance();

    bool start(int port = 9007);
    void stop();

    int port() const { return m_port; }
    bool is_running() const { return m_running; }

    void set_start_callback(RecordFrameStartCallback cb);
    void set_read_callback(RecordFrameReadCallback cb);
    void set_stop_callback(RecordFrameStopCallback cb);

    NET_TV_COMMON_ECODE_E open_stream(const NET_TV_RECORD_FRAME_STREAM_COND_S& cond,
                                      NET_TV_RECORD_FRAME_STREAM_INFO_S& info);
    NET_TV_COMMON_ECODE_E close_stream(const std::string& stream_id);

private:
    RecordFrameServer();
    ~RecordFrameServer();

    void accept_loop();
    void client_send_loop(socket_fd_t client_fd);
    bool send_packet(socket_fd_t fd, const NET_TV_RECORD_FRAME_INFO_S& frame_info, const char* payload);
    std::string current_stream_id() const;

    socket_fd_t m_listen_fd{INVALID_SOCKET_FD};
    std::atomic<bool> m_running{false};
    std::thread m_accept_thread;
    std::thread m_client_thread;
    mutable std::mutex m_client_mutex;
    socket_fd_t m_client_fd{INVALID_SOCKET_FD};
    int m_port{9007};

    mutable std::mutex m_stream_mutex;
    std::string m_stream_id;
    uint32_t m_ssrc{1};

    mutable std::mutex m_callback_mutex;
    RecordFrameStartCallback m_start_cb;
    RecordFrameReadCallback m_read_cb;
    RecordFrameStopCallback m_stop_cb;
};

}  // namespace tvsdk
