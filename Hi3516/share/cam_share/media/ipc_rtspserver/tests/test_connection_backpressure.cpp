/**
 * @FilePath     : test_connection_backpressure.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 14:38:37
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : TCP 慢客户端的发送进展计时测试
 */

#include "net/congestion_tracker.h"

#include "test_support.h"

using namespace ipc_rtsp::detail;

RTSP_TEST_CASE(congestion_timeout_tracks_last_send_progress)
{
    CongestionTracker tracker;
    tracker.Enter(1000);

    /* 进入拥塞但尚无进展：未到阈值不判超时，无进展时长从进入时刻起算。 */
    RTSP_CHECK(tracker.active());
    RTSP_CHECK(!tracker.TimedOut(3000, 2000));
    RTSP_CHECK_EQ(tracker.NoProgressMs(3000), static_cast<std::uint64_t>(2000));

    /* 缓冲仍在下降时刷新进展时间，不能按首次进入拥塞的时间误断开。 */
    tracker.NoteProgress(2500, 4096);
    RTSP_CHECK(!tracker.TimedOut(4001, 2000));
    RTSP_CHECK_EQ(tracker.NoProgressMs(4001), static_cast<std::uint64_t>(1501));

    tracker.NoteProgress(4200, 1);
    RTSP_CHECK(!tracker.TimedOut(6200, 2000));
    RTSP_CHECK(tracker.TimedOut(6201, 2000));
    RTSP_CHECK_EQ(tracker.CongestedMs(6201), static_cast<std::uint64_t>(5201));
}

RTSP_TEST_CASE(congestion_timeout_fires_without_progress)
{
    CongestionTracker tracker;
    tracker.Enter(500);

    /* 零字节不算发送进展。 */
    tracker.NoteProgress(1000, 0);
    RTSP_CHECK(!tracker.TimedOut(2500, 2000));
    RTSP_CHECK(tracker.TimedOut(2501, 2000));
}

RTSP_TEST_CASE(congestion_leave_returns_duration_and_resets_state)
{
    /* 场景：拥塞结束后主动退出；预期 Leave 返回本次拥塞总时长，并把状态复位到非活跃。 */
    CongestionTracker tracker;
    tracker.Enter(100);
    tracker.NoteProgress(300, 10);

    RTSP_CHECK_EQ(tracker.Leave(650), static_cast<std::uint64_t>(550));
    RTSP_CHECK(!tracker.active());
    RTSP_CHECK(!tracker.TimedOut(10000, 1));
    RTSP_CHECK_EQ(tracker.CongestedMs(10000), static_cast<std::uint64_t>(0));
    RTSP_CHECK_EQ(tracker.NoProgressMs(10000), static_cast<std::uint64_t>(0));
}
