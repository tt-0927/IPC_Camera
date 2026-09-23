/**
 * @FilePath     : test_output_buffer.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 有界输出缓冲测试（时长制背压）
 */

#include "net/output_buffer.h"

#include "test_support.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <vector>

using namespace ipc_rtsp;
using namespace ipc_rtsp::detail;

namespace
{

/**
 * 创建已连接且非阻塞的 socket 对。
 *
 * 生产环境里连接 socket 必须是非阻塞的（OutputBuffer::FlushTo 依赖 EAGAIN
 * 返回而不是阻塞在 send 上），测试沿用同样的语义。
 *
 * @param reader 输出读端 fd（接收侧）。
 * @param writer 输出写端 fd（发送侧）。
 * @return 创建成功返回 true。
 */
bool MakeSocketPair(int *reader, int *writer)
{
    int fds[2] = { -1, -1 };
    if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fds) != 0)
    {
        return false;
    }
    *reader = fds[0];
    *writer = fds[1];
    return true;
}

} // namespace

RTSP_TEST_CASE(output_buffer_append_and_flush)
{
    /* 场景：控制数据入队后一次性刷出；预期读端收到的字节与写入完全一致。 */
    int reader = -1;
    int writer = -1;
    RTSP_CHECK(MakeSocketPair(&reader, &writer));

    OutputBuffer buffer;
    buffer.Configure(1024, 64);

    const char payload[] = "OPTIONS * RTSP/1.0\r\n\r\n";
    RTSP_CHECK(buffer.AppendControl(payload, sizeof payload - 1));
    RTSP_CHECK_EQ(buffer.size(), sizeof payload - 1);
    RTSP_CHECK_EQ(buffer.FlushTo(writer), static_cast<ssize_t>(sizeof payload - 1));
    RTSP_CHECK(buffer.empty());

    char received[64] = { 0 };
    const ssize_t read_bytes = ::read(reader, received, sizeof received);
    RTSP_CHECK_EQ(read_bytes, static_cast<ssize_t>(sizeof payload - 1));
    RTSP_CHECK_EQ(std::strcmp(received, payload), 0);

    ::close(reader);
    ::close(writer);
}

RTSP_TEST_CASE(output_buffer_rejects_overflow)
{
    OutputBuffer buffer;
    buffer.Configure(128, 16);

    const std::array<std::uint8_t, 128> chunk = {};
    RTSP_CHECK(buffer.AppendControl(chunk.data(), 100));
    /* 再加 100 字节会超过容量：必须失败且不改变内容。 */
    RTSP_CHECK(!buffer.AppendControl(chunk.data(), 100));
    RTSP_CHECK_EQ(buffer.size(), static_cast<std::size_t>(100));
}

RTSP_TEST_CASE(output_buffer_reserves_capacity_for_control)
{
    OutputBuffer buffer;
    buffer.Configure(1000, 100);

    const std::array<std::uint8_t, 1000> chunk = {};
    RTSP_CHECK_EQ(buffer.media_capacity(), static_cast<std::size_t>(900));
    RTSP_CHECK(buffer.AppendMedia(chunk.data(), 400, 1000));
    RTSP_CHECK(buffer.AppendMedia(chunk.data(), 200, 1030));
    RTSP_CHECK(buffer.AppendMedia(chunk.data(), 300, 1060));
    RTSP_CHECK_EQ(buffer.size(), buffer.media_capacity());
    RTSP_CHECK(!buffer.has_media_room(1));

    /* RTP 已占满媒体区后不能再写，但 RTSP 控制响应仍可使用预留的 100 字节。 */
    RTSP_CHECK(!buffer.AppendMedia(chunk.data(), 1, 1100));
    RTSP_CHECK(buffer.AppendControl(chunk.data(), 100));
    RTSP_CHECK_EQ(buffer.size(), buffer.capacity());
    RTSP_CHECK(!buffer.has_control_room(1));
    RTSP_CHECK(!buffer.AppendControl(chunk.data(), 1));
}

RTSP_TEST_CASE(output_buffer_backlog_duration_tracks_oldest_media)
{
    int reader = -1;
    int writer = -1;
    RTSP_CHECK(MakeSocketPair(&reader, &writer));

    OutputBuffer buffer;
    buffer.Configure(64u * 1024u, 1024);

    const std::array<std::uint8_t, 1024> chunk = {};
    RTSP_CHECK_EQ(buffer.BacklogMs(1000), static_cast<std::uint64_t>(0));

    /* 三个媒体帧在不同时刻入队：积压时长由最老的一帧决定。 */
    RTSP_CHECK(buffer.AppendMedia(chunk.data(), 400, 1000));
    RTSP_CHECK(buffer.AppendMedia(chunk.data(), 300, 1200));
    RTSP_CHECK(buffer.AppendMedia(chunk.data(), 200, 1400));
    RTSP_CHECK_EQ(buffer.BacklogMs(2000), static_cast<std::uint64_t>(1000));

    /* 单个任意大的帧都只是"一个时刻"的戳记：积压时长与字节数无关。 */
    RTSP_CHECK(buffer.AppendMedia(chunk.data(), 800, 1900));
    RTSP_CHECK_EQ(buffer.BacklogMs(2000), static_cast<std::uint64_t>(1000));

    /* 排空最老一帧后，积压时长推进到下一帧。 */
    RTSP_CHECK(buffer.FlushTo(writer) > 0);
    ::close(reader);
    ::close(writer);

    /* socketpair 缓冲远大于 1.7KiB，这里已全部发出。 */
    RTSP_CHECK(buffer.empty());
    RTSP_CHECK_EQ(buffer.BacklogMs(2000), static_cast<std::uint64_t>(0));
}

RTSP_TEST_CASE(output_buffer_backlog_survives_partial_flush)
{
    int reader = -1;
    int writer = -1;
    RTSP_CHECK(MakeSocketPair(&reader, &writer));

    /* 把发送缓冲压小，制造部分写。 */
    int send_buffer = 4096;
    ::setsockopt(writer, SOL_SOCKET, SO_SNDBUF, &send_buffer, sizeof send_buffer);

    OutputBuffer buffer;
    buffer.Configure(256u * 1024u, 4096);

    std::vector<std::uint8_t> first_frame(64u * 1024u, 0x5A);
    std::vector<std::uint8_t> second_frame(64u * 1024u, 0xA5);
    RTSP_CHECK(buffer.AppendMedia(first_frame.data(), first_frame.size(), 1000));
    RTSP_CHECK(buffer.AppendMedia(second_frame.data(), second_frame.size(), 2000));

    const ssize_t flushed = buffer.FlushTo(writer);
    RTSP_CHECK(flushed > 0);
    RTSP_CHECK(!buffer.empty());
    /* 部分排空后仍应有积压；剩余数据属于两帧的交界或第二帧。 */
    const std::uint64_t backlog = buffer.BacklogMs(3000);
    RTSP_CHECK(backlog > 0);
    RTSP_CHECK(backlog <= 2000);

    /* 全部读走后排空，积压归零：内核发送缓冲远小于 128KiB，需多轮"读空-再刷"。 */
    std::vector<std::uint8_t> received(256u * 1024u);
    for (int round = 0; round < 64 && !buffer.empty(); ++round)
    {
        for (;;)
        {
            if (::read(reader, received.data(), received.size()) <= 0)
            {
                break;
            }
        }
        (void) buffer.FlushTo(writer);
    }
    RTSP_CHECK(buffer.empty());
    RTSP_CHECK_EQ(buffer.BacklogMs(5000), static_cast<std::uint64_t>(0));

    ::close(reader);
    ::close(writer);
}

RTSP_TEST_CASE(output_buffer_control_transaction_rollback)
{
    OutputBuffer buffer;
    buffer.Configure(256, 64);

    const std::array<std::uint8_t, 256> chunk = {};
    RTSP_CHECK(buffer.AppendMedia(chunk.data(), 80, 1000));
    const std::size_t checkpoint = buffer.Checkpoint();

    /* 模拟 RTSP 响应分多次写入后失败：回滚不能破坏之前已经排队的 RTP 数据。 */
    RTSP_CHECK(buffer.AppendControl(chunk.data(), 100));
    RTSP_CHECK(buffer.AppendControl(chunk.data(), 70));
    RTSP_CHECK(!buffer.AppendControl(chunk.data(), 7));
    buffer.Rollback(checkpoint);

    RTSP_CHECK_EQ(buffer.size(), static_cast<std::size_t>(80));
    /* 回滚只针对控制数据：媒体戳记不受影响。 */
    RTSP_CHECK_EQ(buffer.BacklogMs(1200), static_cast<std::uint64_t>(200));
    RTSP_CHECK(buffer.AppendMedia(chunk.data(), 112, 1300));
    RTSP_CHECK_EQ(buffer.size(), buffer.media_capacity());
}

RTSP_TEST_CASE(output_buffer_partial_flush_and_compaction)
{
    int reader = -1;
    int writer = -1;
    RTSP_CHECK(MakeSocketPair(&reader, &writer));

    /* 把发送缓冲压小，制造部分写。 */
    int send_buffer = 4096;
    ::setsockopt(writer, SOL_SOCKET, SO_SNDBUF, &send_buffer, sizeof send_buffer);

    OutputBuffer buffer;
    buffer.Configure(256u * 1024u, 4096);

    std::vector<std::uint8_t> payload(200u * 1024u, 0x5A);
    RTSP_CHECK(buffer.AppendMedia(payload.data(), payload.size(), 1000));

    const ssize_t first = buffer.FlushTo(writer);
    RTSP_CHECK(first > 0);
    RTSP_CHECK(buffer.size() < payload.size()); /* 内核缓冲有限：确实发生了部分写 */
    const std::size_t remaining = buffer.size();

    /* 读走全部可用数据后再刷，验证压缩/前移后的数据仍然连续正确。 */
    std::vector<std::uint8_t> received(256u * 1024u);
    std::size_t total_read = 0;
    for (;;)
    {
        const ssize_t read_bytes = ::read(reader, received.data(), received.size());
        if (read_bytes <= 0)
        {
            break;
        }
        total_read += static_cast<std::size_t>(read_bytes);
    }
    RTSP_CHECK(total_read > 0);

    const ssize_t second = buffer.FlushTo(writer);
    RTSP_CHECK(second >= 0);
    RTSP_CHECK(buffer.size() <= remaining);

    /* 全部内容最终必须一致：读出的字节都是填充值 0x5A。 */
    for (std::size_t i = 0; i < total_read; ++i)
    {
        RTSP_CHECK_EQ(static_cast<int>(received[i]), 0x5A);
    }

    ::close(reader);
    ::close(writer);
}

RTSP_TEST_CASE(output_buffer_flush_reports_closed_peer)
{
    int reader = -1;
    int writer = -1;
    RTSP_CHECK(MakeSocketPair(&reader, &writer));
    ::close(reader);

    OutputBuffer buffer;
    buffer.Configure(4096, 256);
    const std::uint8_t chunk[4096] = { 0 };
    RTSP_CHECK(buffer.AppendControl(chunk, sizeof chunk));

    /* 对端已关闭：第一次可能仍成功写入内核缓冲，但最终必须报错。 */
    ssize_t result = 0;
    for (int i = 0; i < 64; ++i)
    {
        result = buffer.FlushTo(writer);
        if (result < 0)
        {
            break;
        }
    }
    RTSP_CHECK(result < 0);

    ::close(writer);
}
