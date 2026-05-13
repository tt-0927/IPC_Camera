/*
 * @FilePath     : SignalSlot.h
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-04-17 10:15:16
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2026-03-12 09:16:18
 * @Description  :
 */
#ifndef _SIGNAL_SLOT_H_
#define _SIGNAL_SLOT_H_

#include <memory>
#include <mutex>
#include <vector>

template<typename T, typename... Args>
using signal_function = void (T::*)(Args...);

template<typename... Args>
struct TanSlotBase
{
    virtual void run(Args...)                  = 0;
    virtual bool contextMatches(void* context) = 0;
};

template<typename T, typename... Args>
class TanSlot : public TanSlotBase<Args...>
{
public:

    TanSlot(T* class_ptr, signal_function<T, Args...> slot_func_ptr)
        : class_ptr(class_ptr), slot_func_ptr(slot_func_ptr) {}

    bool operator==(T* ptr)
    {
        return class_ptr == ptr;
    }

    bool operator==(signal_function<T, Args...> func)
    {
        return slot_func_ptr == func;
    }

    virtual bool contextMatches(void* context) final
    {
        return class_ptr == context;
    }

private:

    virtual void run(Args... args) final
    {
        (class_ptr->*slot_func_ptr)(args...);
    }

private:

    T*                          class_ptr;
    signal_function<T, Args...> slot_func_ptr;
};

template< typename... Args>
class TanSignal
{
public:

    using slot_func = void (*)(Args...);

    TanSignal()
    {
        m_slots.clear();
    }

private:
    std::mutex m_mutex; 

public:

    std::vector<std::shared_ptr<TanSlotBase<Args...>>> m_slots;

    void emit(Args... args)
    {
        std::vector<std::shared_ptr<TanSlotBase<Args...>>> local_slots;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            local_slots = m_slots; 
        } 

        for (auto slot : local_slots)
        {
            if (slot)
            {
                slot->run(args...);
            }
        }
    }

    int size()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_slots.size();
    }

    std::mutex& getMutex() 
    { 
        return m_mutex; 
    }
};

/**
 * @brief 连接信号
 * @param [SIGNAL*] signal: 信号函数指针
 * @param [CONTEXT*] context: 槽所在的指针
 * @param [signal_function<CONTEXT, Args...>] slot: 槽函数指针
 * @param [bool] allow_duplicates: 是否允许重复，默认true
 * @return [*] 无
 * @note
 */
template<typename SIGNAL, typename CONTEXT, typename... Args>
void connect(SIGNAL* signal, CONTEXT* context, signal_function<CONTEXT, Args...> slot, bool allow_duplicates = true)
{
    if (!signal || !context)
    {
        return;
    }

    /*锁定对应信号实例的锁*/
    std::unique_lock<std::mutex> lock(signal->getMutex());

    if (!allow_duplicates)
    {
        int nSlotsNum = signal->m_slots.size();
        if (nSlotsNum > 0)
        {
            for (const auto& existing_slot : signal->m_slots)
            {
                if (existing_slot)
                {
                    TanSlot<CONTEXT, Args...>* p_slot = dynamic_cast<TanSlot<CONTEXT, Args...>*>(existing_slot.get());
                    if (p_slot && *p_slot == context && *p_slot == slot)
                    {
                        return;    // Duplicate connection, return without adding
                    }
                }
            }
        }
    }
    signal->m_slots.push_back(std::make_shared<TanSlot<CONTEXT, Args...>>(context, slot));
}

template<typename SIGNAL, typename CONTEXT, typename... Args>
void disconnect(SIGNAL* signal, CONTEXT* context, signal_function<CONTEXT, Args...> slot)
{
    if (!signal || !context)
    {
        return;
    }

    std::unique_lock<std::mutex> lock(signal->getMutex());

    for (auto iter = signal->m_slots.begin(); iter != signal->m_slots.end();)
    {
        TanSlot<CONTEXT, Args...>* p_slot = dynamic_cast<TanSlot<CONTEXT, Args...>*>(iter->get());
        if (p_slot && *p_slot == context && *p_slot == slot)
        {
            iter = signal->m_slots.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

// Disconnect all slots from a specific context
template<typename SIGNAL, typename CONTEXT>
void disconnect(SIGNAL* signal, CONTEXT* context)
{
    if (!signal || !context)
    {
        return;
    }

    std::unique_lock<std::mutex> lock(signal->getMutex());

    for (auto iter = signal->m_slots.begin(); iter != signal->m_slots.end();)
    {
        if ((*iter)->contextMatches(context))
        {
            iter = signal->m_slots.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

// Disconnect all slots from a specific signal
template<typename SIGNAL>
void disconnect(SIGNAL* signal)
{
    if (!signal)
    {
        return;
    }

    std::unique_lock<std::mutex> lock(signal->getMutex());

    signal->m_slots.clear();
}

#endif