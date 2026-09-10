/*** 
 * @FilePath     : BlockThread.hpp
 * @Author       : cyc
 * @Date         : 2025-08-25 16:41:57
 * @LastEditors  : cyc
 * @LastEditTime : 2025-08-26 10:24:01
 * @Description  : 阻塞超时等待线程类，可通知线程立即唤醒
 */

#pragma once
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

#include "dlog.h"

class BlockThread
{
public:
    using Func = std::function<void()>;

    BlockThread(Func func, std::chrono::milliseconds timeout, bool bOnce = false) : _func(func), _timeout(timeout), _once(bOnce)
    {
    }

    ~BlockThread()
    {
        stop();
        if (_thread.joinable())
        {
            _thread.join();
        }
    }

    void start()
    {
        if (!_started.exchange(true))
        {
            _stop.store(false);
            _thread = std::thread(&BlockThread::threadFunc, this);
        }
    }

    void notify()
    {
        _notified.store(true);
    }

    void stop()
    {
        _stop.store(true);
    }

private:
    /* 线程等待结束原因。 */
    enum class WakeReason_E
    {
        STOPPED,
        NOTIFIED,
        TIMEOUT
    };

    /**
     * @brief  使用单调时钟等待停止、通知或超时事件
     * @param  [void]
     * @return [WakeReason_E] 本次线程唤醒原因
     * @note   采用短周期轮询，避免设备校时影响心跳和续注册定时任务
     */
    WakeReason_E wait_for_wakeup()
    {
        const auto tmDeadline = std::chrono::steady_clock::now() + _timeout;

        /* ! 板端系统校时可能导致condition_variable超时等待失效，必须使用steady_clock轮询。 */
        while (std::chrono::steady_clock::now() < tmDeadline)
        {
            if (_stop.load())
            {
                return WakeReason_E::STOPPED;
            }
            if (_notified.exchange(false))
            {
                return WakeReason_E::NOTIFIED;
            }

            /* perf: 100ms轮询兼顾停止响应速度与低CPU占用。 */
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (_stop.load())
        {
            return WakeReason_E::STOPPED;
        }
        if (_notified.exchange(false))
        {
            return WakeReason_E::NOTIFIED;
        }
        return WakeReason_E::TIMEOUT;
    }

    void threadFunc()
    {
        pthread_setname_np(pthread_self(), "SipBlockThread");

        while (!_stop.load())
        {
            const WakeReason_E enWakeReason = wait_for_wakeup();
            if (WakeReason_E::STOPPED == enWakeReason)
            {
                dlog_debug("收到停止信号");
                break;
            }

            if (WakeReason_E::NOTIFIED == enWakeReason)
            {
                dlog_debug("收到通知");
                std::cout << "收到通知，开始执行任务..." << std::endl;
            }
            else
            {
                dlog_debug("等待超时");
                std::cout << "等待超时，开始执行任务..." << std::endl;
            }

            if (_func)
            {
                _func();
            }

            if (_once)
            {
                break;
            }
        }
        dlog_debug("线程结束");
    }

    Func _func;
    std::chrono::milliseconds _timeout;
    std::thread _thread;
    std::atomic<bool> _stop{ false };
    std::atomic<bool> _started{ false };
    bool _once;
    std::atomic<bool> _notified{ false };
};

#if 0
// 示例类，ThreadClass作为其成员，传入this指针调用成员函数
class MyClass {
    public:
        // 成员函数，接受两个参数
        void memberFunc(int x, const std::string& msg) {
            std::cout << "MyClass::memberFunc 被调用, x = " << x << ", msg = " << msg << std::endl;
        }
    
        void startThread() {
            // 通过std::bind绑定成员函数、this指针以及其他参数
            _threadObj = new ThreadClass(
                std::bind(&MyClass::memberFunc, this, 42, "Hello"),
                std::chrono::milliseconds(5000)
            );
        }
    
        ~MyClass() {
            if (_threadObj) {
                delete _threadObj;
            }
        }
    
    private:
        ThreadClass* _threadObj = nullptr;
    };
    
    int main() {
        MyClass obj;
        obj.startThread();
    
        // 模拟2秒后发送通知（也可以注释以测试超时效果）
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        // 这里如果需要通知执行，可调用 _threadObj->notify()，示例中暂不调用
    
        // 主线程等待足够长的时间，以便观察任务执行
        std::this_thread::sleep_for(std::chrono::milliseconds(6000));
        return 0;
    }
#endif
