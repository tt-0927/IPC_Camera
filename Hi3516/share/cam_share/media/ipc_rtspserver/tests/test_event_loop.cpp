/**
 * @FilePath     : test_event_loop.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-11 09:35:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : EventLoop（libevent 后端）单元测试
 */

/*
 * 覆盖点：
 * - `Add` + fd 就绪回调；
 * - `Modify` 改关注集后旧事件不再投递、新事件恢复投递（回归测试：
 *   libevent 的 `event_assign` 不会更新已注册事件的 evmap，必须 del→assign→add，
 *   否则该 fd 之后彻底收不到事件——2026-09-11 实机踩过）；
 * - `Post` 跨线程投递、`AddTimer`/`CancelTimer`；
 * - `Stop` 能让 `Run` 退出。
 */

#include "net/event_loop.h"
#include "support/time.h"
#include "test_support.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <unistd.h>

#include <sys/epoll.h>
#include <sys/socket.h>

namespace
{

using ipc_rtsp::detail::EventLoop;

/** 自旋等待条件成立，超时返回 false。
 *
 * @param predicate 轮询谓词。
 * @param timeout_ms 等待上限（毫秒），默认 2000。
 * @return 截止时间内条件成立返回 true，否则返回 false。
 */
template <typename Predicate>
bool WaitFor(Predicate predicate, int timeout_ms = 2000)
{
    const std::int64_t deadline = ipc_rtsp::detail::NowMonotonicMs() + timeout_ms;
    while (ipc_rtsp::detail::NowMonotonicMs() < deadline)
    {
        if (predicate())
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return predicate();
}

/** 在独立线程中运行 EventLoop 的测试夹具。 */
struct LoopThread
{
    /* 被测事件循环。 */
    EventLoop loop;

    /** 初始化并起线程运行循环；返回时循环已就绪。
     *
     * @return 启动成功返回 true。
     */
    bool Start()
    {
        if (!loop.Init().ok())
        {
            return false;
        }
        thread = std::thread(
            [this]
            {
                loop.Run();
            });
        return WaitFor(
            [this]
            {
                return loop.running();
            });
    }

    /** 通知循环退出并 join 线程（未启动时也安全）。 */
    void Stop()
    {
        loop.Stop();
        if (thread.joinable())
        {
            thread.join();
        }
    }

    ~LoopThread()
    {
        Stop();
    }

    /* 运行循环的后台线程。 */
    std::thread thread;
};

} // namespace

RTSP_TEST_CASE(event_loop_fd_events_and_modify)
{
    /* 场景：fd 事件投递与 Modify 切换关注集；预期旧事件停止投递、恢复关注后继续投递。 */
    LoopThread ctx;
    RTSP_CHECK(ctx.Start());

    int fds[2] = { -1, -1 };
    RTSP_CHECK_EQ(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fds), 0);

    std::atomic<int> reads{ 0 };
    const auto added = ctx.loop.Add(fds[0],
                                    EPOLLIN,
                                    [&](std::uint32_t events)
                                    {
                                        if ((events & EPOLLIN) == 0u)
                                        {
                                            return;
                                        }
                                        char buf[64];
                                        while (::read(fds[0], buf, sizeof buf) > 0)
                                        {
                                        }
                                        reads.fetch_add(1);
                                    });
    RTSP_CHECK(added.ok());

    const char payload = 'a';
    RTSP_CHECK_EQ(::write(fds[1], &payload, 1), 1);
    RTSP_CHECK(WaitFor(
        [&]
        {
            return reads.load() >= 1;
        }));

    /* 切到只关注可写：此时再写数据不应触发读回调（Modify 回归点）。 */
    std::atomic<bool> modified{ false };
    ctx.loop.Post(
        [&]
        {
            modified.store(ctx.loop.Modify(fds[0], EPOLLOUT).ok());
        });
    RTSP_CHECK(WaitFor(
        [&]
        {
            return modified.load();
        }));

    const int before = reads.load();
    RTSP_CHECK_EQ(::write(fds[1], &payload, 1), 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    RTSP_CHECK_EQ(reads.load(), before);

    /* 再切回可读：数据仍在内核缓冲里，应立刻恢复投递。 */
    std::atomic<bool> restored{ false };
    ctx.loop.Post(
        [&]
        {
            restored.store(ctx.loop.Modify(fds[0], EPOLLIN).ok());
        });
    RTSP_CHECK(WaitFor(
        [&]
        {
            return restored.load();
        }));
    RTSP_CHECK(WaitFor(
        [&]
        {
            return reads.load() > before;
        }));

    ctx.loop.Remove(fds[0]);
    ::close(fds[0]);
    ::close(fds[1]);
    ctx.Stop();
}

RTSP_TEST_CASE(event_loop_post_and_timers)
{
    /* 场景：Post 跨线程投递与定时器；预期按时触发、可取消、回调内可再挂定时器。 */
    LoopThread ctx;
    RTSP_CHECK(ctx.Start());

    /* Post：在循环线程执行，且能观察到循环线程身份。 */
    std::atomic<bool> ran{ false };
    std::atomic<bool> in_loop{ false };
    ctx.loop.Post(
        [&]
        {
            ran.store(true);
            in_loop.store(ctx.loop.InLoopThread());
        });
    RTSP_CHECK(WaitFor(
        [&]
        {
            return ran.load();
        }));
    RTSP_CHECK(in_loop.load());

    /* 定时器按截止时间触发。 */
    std::atomic<bool> fired{ false };
    const std::int64_t deadline = ipc_rtsp::detail::NowMonotonicMs() + 30;
    ctx.loop.AddTimer(deadline,
                      [&]
                      {
                          fired.store(true);
                      });
    RTSP_CHECK(WaitFor(
        [&]
        {
            return fired.load();
        }));
    RTSP_CHECK(ipc_rtsp::detail::NowMonotonicMs() >= deadline);

    /* 取消后不再触发。 */
    std::atomic<bool> cancelled_fired{ false };
    const auto id = ctx.loop.AddTimer(ipc_rtsp::detail::NowMonotonicMs() + 40,
                                      [&]
                                      {
                                          cancelled_fired.store(true);
                                      });
    ctx.loop.CancelTimer(id);
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    RTSP_CHECK(!cancelled_fired.load());

    /* 定时器回调里可以再挂定时器（链式）。 */
    std::atomic<int> chain{ 0 };
    ctx.loop.AddTimer(ipc_rtsp::detail::NowMonotonicMs() + 10,
                      [&]
                      {
                          chain.fetch_add(1);
                          ctx.loop.AddTimer(ipc_rtsp::detail::NowMonotonicMs() + 10,
                                            [&]
                                            {
                                                chain.fetch_add(1);
                                            });
                      });
    RTSP_CHECK(WaitFor(
        [&]
        {
            return chain.load() == 2;
        }));

    ctx.Stop();
}

RTSP_TEST_CASE(event_loop_stop_exits_run)
{
    /* 场景：Run 运行中调用 Stop；预期 Run 返回、等待线程随之退出。 */
    EventLoop loop;
    RTSP_CHECK(loop.Init().ok());

    std::atomic<bool> returned{ false };
    std::thread thread(
        [&]
        {
            loop.Run();
            returned.store(true);
        });

    RTSP_CHECK(WaitFor(
        [&]
        {
            return loop.running();
        }));
    loop.Stop();
    RTSP_CHECK(WaitFor(
        [&]
        {
            return returned.load();
        }));
    thread.join();
}
