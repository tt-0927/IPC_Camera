/**
 * @FilePath     : event_loop.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 单线程事件循环（libevent 后端）
 */

/*
 * 设计要点（对应 ADR D2-R1 / D3）：
 * - 事件层用 vendored 的 OpenIPC/libevent（静态链接，仅取 core）；对外接口与自研
 *   epoll 版本一致（Add/Modify/Remove/Post/AddTimer），上层不感知后端；
 * - 全部连接/会话/定时器状态归 I/O 线程独占，其他线程只能经 Post/邮箱交互；
 * - 跨线程唤醒用 eventfd（注册为常驻 EV_READ|EV_PERSIST 事件），单次唤醒不分配内存；
 * - 定时器自管 multimap（支持取消），用一个 libevent 定时事件按最近截止时间武装；
 * - 事件位约定：对外用 EPOLLIN/EPOLLOUT 语义，实现内翻译为 EV_READ/EV_WRITE。
 */

#pragma once

#include "ipc_rtsp/result.h"

#include <event2/util.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

/* libevent 的 C 类型必须在全局作用域前置声明。 */
struct event_base;
struct event;

namespace ipc_rtsp
{
namespace detail
{

/** 单线程事件循环。 */
class EventLoop
{
public:
    /** fd 就绪回调；events 为 EPOLLIN/EPOLLOUT 语义（含 EPOLLHUP/EPOLLERR 映射）。 */
    using Handler = std::function<void(std::uint32_t events)>;
    /** 定时器句柄；0 保留表示"无定时器"。 */
    using TimerId = std::uint64_t;
    /** 跨线程投递/定时器执行的任务。 */
    using Task = std::function<void()>;

    /** @brief 默认构造；须经 Init() 初始化。 @return {void} */
    EventLoop() = default;

    /** @brief 析构：请求退出并释放全部 libevent 资源。 @return {void} */
    ~EventLoop();

    EventLoop(const EventLoop &) = delete;
    EventLoop &operator=(const EventLoop &) = delete;

    /**
     * @brief 创建 libevent base 与唤醒用 eventfd。
     *
     * @return 成功返回 Ok；重复初始化或资源创建失败返回对应错误
     * @note 允许同一实例 Shutdown 后再次 Init（内部先 Cleanup）。
     */
    Result Init();

    /**
     * @brief 注册 fd 及其事件回调（常驻注册）。
     *
     * @param fd 目标 fd（非负）
     * @param events 关注事件（EPOLLIN/EPOLLOUT）
     * @param handler 就绪回调（在 I/O 线程执行）
     * @return 成功返回 Ok；fd 非法或事件创建/注册失败返回对应错误
     * @note 仅 I/O 线程调用；同 fd 重复注册会先释放旧事件再重建。
     */
    Result Add(int fd, std::uint32_t events, Handler handler);

    /**
     * @brief 修改关注事件。
     *
     * @param fd 已注册的 fd
     * @param events 新的关注事件（EPOLLIN/EPOLLOUT）
     * @return 成功返回 Ok；fd 未注册或 libevent 调用失败返回对应错误
     * @note 仅 I/O 线程调用；实现为 del→assign→add，原因见 event_loop.cpp。
     */
    Result Modify(int fd, std::uint32_t events);

    /**
     * @brief 撤销注册并释放事件（不关闭 fd）。
     *
     * @param fd 目标 fd
     * @return {void}
     * @note 仅 I/O 线程调用；可在事件回调内调用（PERSIST 事件此时已出 active 队列）。
     */
    void Remove(int fd);

    /**
     * @brief 在循环线程中执行任务。
     *
     * @param task 待执行任务（空任务忽略）
     * @return {void}
     * @note 线程安全且不分配 per-frame 结构；任务在循环线程的唤醒事件回调中执行。
     */
    void Post(Task task);

    /**
     * @brief 仅唤醒循环（用于邮箱有新数据时）。
     *
     * @return {void}
     * @note 线程安全；写 eventfd 计数，EAGAIN（已饱和）说明必然被唤醒。
     */
    void Wake();

    /**
     * @brief 添加定时器。
     *
     * @param deadline_ms 绝对截止时间（单调钟毫秒）
     * @param task 到期任务（在循环线程执行）
     * @return 可用于取消的 id
     * @note 线程安全；重新武装由循环线程完成（event_add 非线程安全）。
     */
    TimerId AddTimer(std::int64_t deadline_ms, Task task);

    /**
     * @brief 取消定时器。
     *
     * @param id AddTimer 返回的 id（0 忽略）
     * @return {void}
     * @note 线程安全；只记账延迟删除，到期未到的定时器将被跳过并释放任务。
     */
    void CancelTimer(TimerId id);

    /**
     * @brief 运行直到 Stop()。
     *
     * @param ready 循环已进入可调度状态时在 I/O 线程回调，用于 Start ready 门禁
     * @return {void}
     */
    void Run(Task ready = Task());

    /**
     * @brief 请求退出（线程安全）。
     *
     * @return {void}
     * @note 直接置 running_ 为 false，Run() 在本轮事件处理完后退出循环。
     */
    void Stop();

    /**
     * 是否仍在运行；注意 Stop() 会立即将其置 false，为 true 不代表循环线程仍在
     * 调度（仅表示未请求退出）。
     */
    bool running() const
    {
        return running_.load(std::memory_order_relaxed);
    }

    /**
     * @brief 当前线程是否是循环线程。
     *
     * @return true 表示 Run() 已在本线程进入循环
     */
    bool InLoopThread() const;

    /**
     * @brief 注册每轮事件处理后的回调。
     *
     * @param hook 回调（在循环线程、每轮定时器任务之后执行）
     * @return {void}
     * @note 用于回收本轮被关闭的连接对象，避免在自身回调里析构。
     */
    void SetPostDispatchHook(Task hook)
    {
        post_dispatch_hook_ = std::move(hook);
    }

    /**
     * @brief 取底层 `event_base`（供后续把 evhttp/evws 挂到同一 loop）。
     *
     * @return libevent 事件循环本体；未初始化时为 nullptr
     * @note 仅限 I/O 线程使用。
     */
    event_base *base() const
    {
        return base_;
    }

private:
    static void OnWake(evutil_socket_t fd, short what, void *arg);
    static void OnFdEvent(evutil_socket_t fd, short what, void *arg);
    static void OnTimerTick(evutil_socket_t fd, short what, void *arg);

    void DrainWakeFd();
    void Cleanup();
    void RunExpiredTimers();
    void ArmTimerEvent();
    std::int64_t NextTimerDeadlineMs() const;

    /** libevent 事件循环本体；Init() 创建，Cleanup() event_base_free。 */
    event_base *base_ = nullptr;
    /** 唤醒事件（wake_fd_ 上的 EV_READ|EV_PERSIST）；libevent 拥有，Cleanup() event_free。 */
    event *wake_event_ = nullptr;
    /** 纯定时事件（无 fd）；libevent 拥有，Cleanup() event_free。 */
    event *timer_event_ = nullptr;
    /** 跨线程唤醒用 eventfd；Init() 申请，Cleanup() close。 */
    int wake_fd_ = -1;
    /** 循环线程 id，Run() 进入时记录（InLoopThread 判断用）。 */
    std::thread::id loop_thread_id_;
    /** 是否仍在运行；Stop() 会直接置 false（语义见 running()）。 */
    std::atomic<bool> running_{ false };
    /** 退出请求标志；Stop() 置位，Run() 每轮开始检查。 */
    std::atomic<bool> stop_requested_{ false };

    /* fd_events_ 与 handlers_ 为平行 map：键集合必须一致（同增同删）。
     * fd_events_ 持有 libevent event 的生命周期，handlers_ 存回调供派发查找。 */
    std::map<int, event *> fd_events_;
    std::map<int, Handler> handlers_;

    /** 保护 pending_tasks_ 的互斥量（Post/DrainWakeFd 短临界区）。 */
    std::mutex task_mutex_;
    /** 跨线程任务邮箱；仅循环线程在唤醒回调中交换并执行。 */
    std::vector<Task> pending_tasks_;

    /* 定时器结构仅 I/O 线程访问：deadline→id 与 id→task 平行映射；
     * cancelled_timers_ 只记账，延迟到 RunExpiredTimers 统一清理。 */
    std::multimap<std::int64_t, TimerId> timer_deadlines_;
    std::map<TimerId, Task> timer_tasks_;
    std::set<TimerId> cancelled_timers_;
    /** 下一个定时器 id（单调递增；0 保留表示无定时器）。 */
    TimerId next_timer_id_ = 1;
    /** 每轮事件处理后的回调（仅循环线程执行），用于回收本轮关闭的连接。 */
    Task post_dispatch_hook_;
};

} // namespace detail
} // namespace ipc_rtsp
