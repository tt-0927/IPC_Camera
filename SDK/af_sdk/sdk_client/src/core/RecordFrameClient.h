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

using RecordFrameCallback = std::function<void(const NET_TV_RECORD_FRAME_INFO_S& frame_info,
                                               const char* data,
                                               size_t size)>;

class RecordFrameClient {
public:
    RecordFrameClient();
    ~RecordFrameClient();

    bool start(const std::string& host,
               int port,
               const std::string& stream_id,
               RecordFrameCallback callback);
    void stop();

    bool is_running() const { return m_running; }
    const std::string& stream_id() const { return m_stream_id; }

private:
    void recv_loop();

    socket_fd_t m_socket{INVALID_SOCKET_FD};
    std::atomic<bool> m_running{false};
    std::string m_stream_id;
    RecordFrameCallback m_callback;
    std::thread m_recv_thread;
    std::mutex m_stop_mutex;
};

}  // namespace tvsdk
