/**
 * @FilePath     : event_loop.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 事件循环实现（libevent 后端）
 */

/*
 * 与自研 epoll 版本的差异只在实现层：
 * - fd 就绪事件改由 event_new(..., EV_PERSIST) 注册（libevent 的 epoll 后端仍是
 *   水平触发，语义与之前一致）；
 * - 定时器仍由本类用 multimap 管理，额外用一个 libevent 定时事件在最近截止时间
 *   醒来，避免每个定时器一个 event；
 * - 退出用「置位 + Wake」而不是 event_base_loopbreak：loopbreak 跨线程调用依赖
 *   libevent 的线程锁初始化，这里保持无锁语义。
 */

#include "net/event_loop.h"

#include "support/log.h"
#include "support/time.h"

#include <cerrno>
#include <cstring>
#include <unistd.h>

#include <event2/event.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

namespace ipc_rtsp
{
namespace detail
{
namespace
{
constexpr const char *kTag = "loop";

/** EPOLL 语义 → libevent 事件位。 */
short ToLibeventEvents(std::uint32_t events)
{
    short out = 0;
    if ((events & EPOLLIN) != 0u)
    {
        out = static_cast<short>(out | EV_READ);
    }
    if ((events & EPOLLOUT) != 0u)
    {
        out = static_cast<short>(out | EV_WRITE);
    }
    return out;
}

/*
 * libevent 事件位 → EPOLL 语义（回调参数，保持上层判断不变）。
 * epoll 后端的错误/挂起不一定有独立事件位：可能合并为 EV_READ|EV_WRITE 投递，
 * 由上层读/写拿到 0 或 errno 后自行关闭（Connection::OnReadable 已覆盖）。
 * review: EV_CLOSED/EV_EOF 的取舍随 libevent 版本而异，此处映射未逐一核实。
 */
std::uint32_t ToEpollEvents(short what)
{
    std::uint32_t out = 0;
    if ((what & EV_READ) != 0)
    {
        out |= EPOLLIN;
    }
    if ((what & EV_WRITE) != 0)
    {
        out |= EPOLLOUT;
    }
    if ((what & EV_CLOSED) != 0)
    {
        out |= EPOLLHUP;
    }
    return out;
}
} // namespace

EventLoop::~EventLoop()
{
    Stop();
    Cleanup();
}

void EventLoop::Cleanup()
{
    /* 释放顺序：先 event_free 全部事件（fd/timer/wake），再 event_base_free，
     * 最后 close(wake_fd_) —— event 必须先于其所属 base 释放。 */
    for (auto &entry : fd_events_)
    {
        event_free(entry.second);
    }
    fd_events_.clear();

    if (timer_event_ != nullptr)
    {
        event_free(timer_event_);
        timer_event_ = nullptr;
    }
    if (wake_event_ != nullptr)
    {
        event_free(wake_event_);
        wake_event_ = nullptr;
    }
    if (base_ != nullptr)
    {
        event_base_free(base_);
        base_ = nullptr;
    }
    if (wake_fd_ >= 0)
    {
        ::close(wake_fd_);
        wake_fd_ = -1;
    }

    handlers_.clear();
    timer_deadlines_.clear();
    timer_tasks_.clear();
    cancelled_timers_.clear();
    next_timer_id_ = 1;
    post_dispatch_hook_ = Task();
    loop_thread_id_ = std::thread::id();
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        pending_tasks_.clear();
    }
}

Result EventLoop::Init()
{
    if (running_.load(std::memory_order_acquire))
    {
        return Result::Fail(Status::AlreadyInitialized);
    }

    /* 允许同一 Server 实例 Shutdown 后再次 Start。 */
    Cleanup();
    stop_requested_.store(false, std::memory_order_release);

    base_ = event_base_new();
    if (base_ == nullptr)
    {
        return Result::Fail(Status::Internal);
    }

    wake_fd_ = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (wake_fd_ < 0)
    {
        const int saved = errno;
        Cleanup();
        return Result::Io(saved);
    }

    wake_event_ = event_new(base_, wake_fd_, EV_READ | EV_PERSIST, &EventLoop::OnWake, this);
    if (wake_event_ == nullptr)
    {
        Cleanup();
        return Result::Fail(Status::Internal);
    }
    if (event_add(wake_event_, nullptr) < 0)
    {
        const int saved = errno;
        Cleanup();
        return Result::Io(saved);
    }

    /* 纯定时事件：无 fd，按最近截止时间武装。 */
    timer_event_ = event_new(base_, -1, 0, &EventLoop::OnTimerTick, this);
    if (timer_event_ == nullptr)
    {
        Cleanup();
        return Result::Fail(Status::Internal);
    }

    return Result::Ok();
}

Result EventLoop::Add(int fd, std::uint32_t events, Handler handler)
{
    if (fd < 0)
    {
        return Result::Fail(Status::InvalidArgument);
    }

    const auto existing = fd_events_.find(fd);
    if (existing != fd_events_.end())
    {
        /* 同 fd 重复注册：直接 free 后重建 event（不走 event_assign），
         * 原因与 Modify() 的 del→assign→add 注释同源。 */
        event_free(existing->second);
        fd_events_.erase(existing);
    }

    handlers_[fd] = std::move(handler);

    event *ev = event_new(base_, fd, ToLibeventEvents(events) | EV_PERSIST, &EventLoop::OnFdEvent, this);
    if (ev == nullptr)
    {
        handlers_.erase(fd);
        return Result::Fail(Status::Internal);
    }
    if (event_add(ev, nullptr) < 0)
    {
        const int saved = errno;
        event_free(ev);
        handlers_.erase(fd);
        return Result::Io(saved);
    }

    fd_events_[fd] = ev;
    return Result::Ok();
}

Result EventLoop::Modify(int fd, std::uint32_t events)
{
    const auto it = fd_events_.find(fd);
    if (it == fd_events_.end())
    {
        return Result::Fail(Status::InvalidArgument);
    }

    event *ev = it->second;

    /* 必须 `del → assign → add`：
     * - `event_assign` 会把事件的注册标志清成「未注册」，而 base 的 io 映射里还留着
     *   它，直接 assign 会让两边状态不一致，表现为该 fd 之后不再投递事件（实测踩过）；
     * - `event_add_nolock_` 只在事件「未注册」时才调用 `evmap_io_add_` 更新关注集，
     *   所以必须先用 `event_del` 真正摘除，才能让新的 `EPOLLIN/EPOLLOUT` 生效。
     * 回调内调用是安全的：常驻事件在回调前已从 active 队列摘除。 */
    if (event_del(ev) < 0)
    {
        return Result::Io(errno);
    }
    if (event_assign(ev, base_, fd, ToLibeventEvents(events) | EV_PERSIST, &EventLoop::OnFdEvent, this) < 0)
    {
        return Result::Io(errno);
    }
    if (event_add(ev, nullptr) < 0)
    {
        return Result::Io(errno);
    }
    return Result::Ok();
}

void EventLoop::Remove(int fd)
{
    const auto it = fd_events_.find(fd);
    if (it != fd_events_.end())
    {
        /* libevent 对 PERSIST 事件会先把回调从 active 队列摘除，
         * 因此在回调内 event_free 是安全的（见 D2-R1 记录）。 */
        event_free(it->second);
        fd_events_.erase(it);
    }
    handlers_.erase(fd);
}

void EventLoop::Post(Task task)
{
    if (!task)
    {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        pending_tasks_.push_back(std::move(task));
    }
    Wake();
}

void EventLoop::Wake()
{
    if (wake_fd_ < 0)
    {
        return;
    }
    const std::uint64_t one = 1;
    const ssize_t written = ::write(wake_fd_, &one, sizeof one);
    /* EAGAIN 说明计数已饱和，循环一定会被唤醒。 */
    (void) written;
}

void EventLoop::DrainWakeFd()
{
    std::uint64_t value = 0;
    while (::read(wake_fd_, &value, sizeof value) > 0)
    {
        /* 读空 eventfd 计数。 */
    }

    std::vector<Task> tasks;
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        tasks.swap(pending_tasks_);
    }
    for (Task &task : tasks)
    {
        task();
    }
}

EventLoop::TimerId EventLoop::AddTimer(std::int64_t deadline_ms, Task task)
{
    const TimerId id = next_timer_id_++;
    timer_deadlines_.emplace(deadline_ms, id);
    timer_tasks_[id] = std::move(task);
    /* 只唤醒；重新武装定时事件由循环线程在每轮末尾完成（event_add 不是线程安全的）。 */
    Wake();
    return id;
}

void EventLoop::CancelTimer(TimerId id)
{
    if (id == 0)
    {
        return;
    }
    /* 只记账、不直接改定时器结构（本函数可能不在循环线程调用）：
     * 延迟删除交给循环线程的 RunExpiredTimers，未到期的定时器被跳过并释放任务。 */
    cancelled_timers_.insert(id);
}

std::int64_t EventLoop::NextTimerDeadlineMs() const
{
    for (const auto &entry : timer_deadlines_)
    {
        if (cancelled_timers_.find(entry.second) == cancelled_timers_.end())
        {
            return entry.first;
        }
    }
    return -1;
}

void EventLoop::ArmTimerEvent()
{
    if (timer_event_ == nullptr)
    {
        return;
    }

    (void) evtimer_del(timer_event_);

    const std::int64_t deadline = NextTimerDeadlineMs();
    if (deadline < 0)
    {
        return;
    }

    timeval tv{};
    const std::int64_t now = NowMonotonicMs();
    if (deadline > now)
    {
        const std::int64_t delta = deadline - now;
        tv.tv_sec = static_cast<decltype(tv.tv_sec)>(delta / 1000);
        tv.tv_usec = static_cast<decltype(tv.tv_usec)>((delta % 1000) * 1000);
    }
    (void) evtimer_add(timer_event_, &tv);
}

void EventLoop::RunExpiredTimers()
{
    /* 先 erase 再执行：task 内可能再 AddTimer/CancelTimer，
     * 避免迭代器失效与同一轮内的重入问题。 */
    const std::int64_t now = NowMonotonicMs();
    while (!timer_deadlines_.empty())
    {
        const auto it = timer_deadlines_.begin();
        if (it->first > now)
        {
            break;
        }

        const TimerId id = it->second;
        timer_deadlines_.erase(it);

        if (cancelled_timers_.erase(id) > 0)
        {
            timer_tasks_.erase(id);
            continue;
        }

        auto task_it = timer_tasks_.find(id);
        if (task_it == timer_tasks_.end())
        {
            continue;
        }
        Task task = std::move(task_it->second);
        timer_tasks_.erase(task_it);
        task();
    }
}

bool EventLoop::InLoopThread() const
{
    return std::this_thread::get_id() == loop_thread_id_;
}

void EventLoop::OnWake(evutil_socket_t fd, short what, void *arg)
{
    (void) fd;
    (void) what;
    static_cast<EventLoop *>(arg)->DrainWakeFd();
}

void EventLoop::OnFdEvent(evutil_socket_t fd, short what, void *arg)
{
    EventLoop *self = static_cast<EventLoop *>(arg);
    const auto it = self->handlers_.find(static_cast<int>(fd));
    if (it == self->handlers_.end())
    {
        return;
    }
    /* 回调内可能删除自身注册，先拷贝一份 handler。 */
    Handler handler = it->second;
    handler(ToEpollEvents(what));
}

void EventLoop::OnTimerTick(evutil_socket_t fd, short what, void *arg)
{
    /* 空实现：定时器任务统一在本轮结束后由 RunExpiredTimers() 执行，
     * 保证与旧实现相同的「fd 回调 → 定时器回调 → post_dispatch_hook」顺序。 */
    (void) fd;
    (void) what;
    (void) arg;
}

void EventLoop::Run(Task ready)
{
    loop_thread_id_ = std::this_thread::get_id();
    running_.store(true, std::memory_order_release);

    /* Stop 可能在新线程真正获得调度前到达。 */
    if (stop_requested_.load(std::memory_order_acquire))
    {
        running_.store(false, std::memory_order_release);
        return;
    }

    ArmTimerEvent();
    if (ready)
    {
        ready();
    }

    while (running_.load(std::memory_order_acquire))
    {
        /* EVLOOP_ONCE：阻塞到至少一个事件就绪，处理完本轮后返回。 */
        if (event_base_loop(base_, EVLOOP_ONCE) < 0)
        {
            IPC_RTSP_LOGE(kTag, "event_base_loop 失败");
            break;
        }

        RunExpiredTimers();

        if (post_dispatch_hook_)
        {
            post_dispatch_hook_();
        }

        /* 本轮内可能增删定时器，这里按最新截止时间重新武装。 */
        ArmTimerEvent();
    }

    running_.store(false, std::memory_order_release);
}

void EventLoop::Stop()
{
    /* 直接置 running_=false：Run() 的 while 条件在本轮事件处理完后判假退出，
     * 不依赖 event_base_loopbreak（其跨线程调用依赖 libevent 线程锁初始化）。
     * 副作用：Stop 一返回 running()==false，但循环线程可能仍在收尾本轮。 */
    stop_requested_.store(true, std::memory_order_release);
    running_.store(false, std::memory_order_release);
    Wake();
}

} // namespace detail
} // namespace ipc_rtsp
