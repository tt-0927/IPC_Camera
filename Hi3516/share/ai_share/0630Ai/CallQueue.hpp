#pragma once
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <functional>
#include <mutex>
#include <queue>
#include <set>
#include <tuple>
#include <type_traits>

class CallQueue
{
public:

    CallQueue()                            = default;
    ~CallQueue()                           = default;
    CallQueue(const CallQueue&)            = delete;
    CallQueue& operator=(const CallQueue&) = delete;

    template<typename F, typename... Args>
    void push(F&& f, Args&&... args)
    {
        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        {
            std::lock_guard<std::mutex> lg(m_);
            q_.push(std::move(task));
        }
        cv_.notify_one();
    }

    // ---- 去重 push：若“函数实体 + 参数”完全一致，则拒绝入队 ----
    template<typename F, typename... Args>
    bool pushUnique(F&& f, Args&&... args)
    {
        using Fn              = std::decay_t<F>;
        const std::size_t key = makeKey<Fn>(f, args...);

        {
            std::lock_guard<std::mutex> lg(m_);
            if (activeKeys_.count(key))
            {
                return false;
            }

            // 捕获参数和 key，执行后移除 key
            auto task = [this, key, func = std::forward<F>(f),
                         tpl = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                std::apply(func, tpl);
                std::lock_guard<std::mutex> g(m_);
                activeKeys_.erase(key);
            };

            q_.push(std::move(task));
            activeKeys_.insert(key);
        }

        cv_.notify_one();
        return true;
    }

    bool tryPopAndRun()
    {
        std::function<void()> task;
        {
            std::lock_guard<std::mutex> lg(m_);
            if (q_.empty())
            {
                return false;
            }
            task = std::move(q_.front());
            q_.pop();
        }
        task();
        return true;
    }

    template<class Rep, class Period>
    bool waitPopAndRun(const std::chrono::duration<Rep, Period>& timeout)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> ul(m_);
            if (!cv_.wait_for(ul, timeout, [this] {
                return !q_.empty();
            }))
            {
                return false;
            }
            task = std::move(q_.front());
            q_.pop();
        }
        task();
        return true;
    }

    void runAll()
    {
        std::queue<std::function<void()>> local;
        {
            std::lock_guard<std::mutex> lg(m_);
            local.swap(q_);
        }
        while (!local.empty())
        {
            local.front()();
            local.pop();
        }
    }

    std::size_t size() const
    {
        std::lock_guard<std::mutex> lg(m_);
        return q_.size();
    }

private:

    mutable std::mutex                m_;
    std::condition_variable           cv_;
    std::queue<std::function<void()>> q_;
    std::set<std::size_t>             activeKeys_;

    // ---- hash combine ----
    static void hashCombine(std::size_t& seed, std::size_t v)
    {
        seed ^= v + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
    }

    // 成员函数指针按“字节序列”哈希（可移植做法）
    template<typename M>
    static std::size_t hashMemberPtr(const M& mptr)
    {
        std::size_t          seed = 0;
        const unsigned char* p    = reinterpret_cast<const unsigned char*>(&mptr);
        for (std::size_t i = 0; i < sizeof(M); ++i)
        {
            hashCombine(seed, p[i]);
        }
        return seed;
    }

    // 生成唯一 key：函数实体 + 参数值
    // 1) 成员函数指针：哈希(成员函数指针值) + 对象指针 + 参数
    // 2) 普通函数指针：哈希(函数指针地址) + 参数
    // 3) 其他 callable（lambda/functor）：typeid(F) + 参数
    template<typename Fn, typename... Args>
    static std::size_t makeKey(const Fn& f, const Args&... args)
    {
        std::size_t seed = 0;
        if constexpr (std::is_member_function_pointer_v<Fn>)
        {
            // 需要额外传入 this 指针作为首参；从参数包里取出来做哈希
            static_assert(sizeof...(Args) >= 1, "member function needs object pointer as first arg");
            seed = hashMemberPtr(f);
            // 展开参数：第一个是对象指针
            addArgsHash(seed, args...);
        }
        else if constexpr (std::is_pointer_v<Fn> && std::is_function_v<std::remove_pointer_t<Fn>>)
        {
            // 普通函数指针
            seed = std::hash<const void*> {}(reinterpret_cast<const void*>(f));
            addArgsHash(seed, args...);
        }
        else
        {
            // lambda / functor：不同 lambda 类型 typeid 不同
            seed = typeid(Fn).hash_code();
            addArgsHash(seed, args...);
        }
        return seed;
    }

    template<typename T>
    static void addOneArgHash(std::size_t& seed, const T& v)
    {
        if constexpr (std::is_pointer_v<T>)
        {
            hashCombine(seed, std::hash<const void*> {}(reinterpret_cast<const void*>(v)));
        }
        else
        {
            hashCombine(seed, std::hash<std::decay_t<T>> {}(v));
        }
    }

    template<typename... Ts>
    static void addArgsHash(std::size_t& seed, const Ts&... vs)
    {
        (addOneArgHash(seed, vs), ...);
    }
};
