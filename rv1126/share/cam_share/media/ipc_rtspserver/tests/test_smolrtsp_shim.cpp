/**
 * @FilePath     : test_smolrtsp_shim.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 14:38:45
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : smolrtsp C 隔离层的产品约束测试
 */

#include "adapter/smolrtsp_shim.h"

#include "test_support.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace
{

/** 捕获 shim writer 全部写出的测试替身。 */
struct WriterCapture
{
    /* 写出缓冲（512 字节足够容纳 interleaved 头 + 最大 payload）。 */
    std::array<std::uint8_t, 512> bytes = {};
    /* 最近一次写出的字节数。 */
    std::size_t size = 0;
    /* write 回调被调用的次数。 */
    std::size_t calls = 0;
};

/** ipc_rtsp_shim_writer 的写回调：整段拷贝一次写入并计数；入参非法或超容量时返回 -1 模拟写失败。
 *
 * @param user WriterCapture 上下文。
 * @param data 待写数据。
 * @param len 待写字节数。
 * @return 实际写入字节数；失败返回 -1。
 */
ssize_t CaptureWrite(void *user, const void *data, std::size_t len)
{
    WriterCapture *capture = static_cast<WriterCapture *>(user);
    if (capture == nullptr || data == nullptr || len > capture->bytes.size())
    {
        return -1;
    }
    std::memcpy(capture->bytes.data(), data, len);
    capture->size = len;
    ++capture->calls;
    return static_cast<ssize_t>(len);
}

/** ipc_rtsp_shim_writer 的已缓冲量查询回调：返回当前已捕获的字节数。
 *
 * @param user WriterCapture 上下文。
 * @return 已捕获字节数；上下文为空返回 0。
 */
std::size_t CaptureFilled(void *user)
{
    const WriterCapture *capture = static_cast<const WriterCapture *>(user);
    return capture == nullptr ? 0 : capture->size;
}

} // namespace

RTSP_TEST_CASE(interleaved_rtcp_is_appended_by_one_writer_call)
{
    /* 场景：写一次 interleaved RTCP；预期 '$'+channel+2 字节长度头与 payload 由同一次写回调完整写出。 */
    WriterCapture capture;
    ipc_rtsp_shim_writer *writer = ipc_rtsp_shim_writer_new(&CaptureWrite, &CaptureFilled, &capture);
    RTSP_CHECK(writer != nullptr);

    const std::array<std::uint8_t, 8> payload = { 0x80, 0xc8, 0x00, 0x01, 0x11, 0x22, 0x33, 0x44 };
    RTSP_CHECK_EQ(ipc_rtsp_shim_write_interleaved(writer, 3, payload.data(), payload.size()), 0);
    RTSP_CHECK_EQ(capture.calls, static_cast<std::size_t>(1));
    RTSP_CHECK_EQ(capture.size, payload.size() + 4u);
    RTSP_CHECK_EQ(static_cast<unsigned>(capture.bytes[0]), static_cast<unsigned>('$'));
    RTSP_CHECK_EQ(static_cast<unsigned>(capture.bytes[1]), 3u);
    RTSP_CHECK_EQ(static_cast<unsigned>(capture.bytes[2]), 0u);
    RTSP_CHECK_EQ(static_cast<unsigned>(capture.bytes[3]), static_cast<unsigned>(payload.size()));
    RTSP_CHECK(std::memcmp(capture.bytes.data() + 4, payload.data(), payload.size()) == 0);

    ipc_rtsp_shim_writer_free(writer);
}

RTSP_TEST_CASE(interleaved_rtcp_rejects_oversize_without_partial_write)
{
    /* 场景：payload 超过 1 字节长度上限（257 字节）；预期直接拒绝，不产生任何部分写。 */
    WriterCapture capture;
    ipc_rtsp_shim_writer *writer = ipc_rtsp_shim_writer_new(&CaptureWrite, &CaptureFilled, &capture);
    RTSP_CHECK(writer != nullptr);

    const std::array<std::uint8_t, 257> payload = {};
    RTSP_CHECK(ipc_rtsp_shim_write_interleaved(writer, 1, payload.data(), payload.size()) < 0);
    RTSP_CHECK_EQ(capture.calls, static_cast<std::size_t>(0));
    RTSP_CHECK_EQ(capture.size, static_cast<std::size_t>(0));

    ipc_rtsp_shim_writer_free(writer);
}
