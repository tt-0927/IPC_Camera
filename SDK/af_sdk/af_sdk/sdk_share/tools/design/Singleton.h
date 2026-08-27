/**
 * @file Singleton.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief Singleton 模块接口与类型定义
 * 功能说明：
 * 1. 声明 Singleton 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#ifndef NETSDK_SINGLETON_H
#define NETSDK_SINGLETON_H

#include <mutex>
#include <memory>

template <class T>
class CSingleton
{
public:
    /* 获取实例 */
    static std::shared_ptr<T> instance()
    {
        /*
         * std::make_shared cannot access a derived class private constructor even
         * when CSingleton is a friend. The raw allocation is immediately owned by
         * std::shared_ptr, so the singleton lifetime remains RAII-managed.
         */
        std::call_once(s_stInstantiated, []()
                       { s_pInstance = std::shared_ptr<T>(new T); });
        return s_pInstance;
    }

    /**
     * @author tianl (tianl@kfb.cn)
     * @brief 释放当前单例实例持有的资源。
     * @return 无返回值。
     */
    static void DestroyInstance()
    {
        if (s_pInstance)
        {
            s_pInstance.reset();
        }
    }

    /* 禁用拷贝和移动操作 */
    CSingleton(const CSingleton &) = delete;
    CSingleton &operator=(const CSingleton &) = delete;
    CSingleton(CSingleton &&) = delete;
    CSingleton &operator=(CSingleton &&) = delete;

protected:
    /* 构造函数仅供子类访问 */
    CSingleton() = default;
    virtual ~CSingleton() = default;

private:
    static std::shared_ptr<T> s_pInstance;
    static std::once_flag s_stInstantiated;
};

/* 静态成员初始化 */
template <class T>
std::shared_ptr<T> CSingleton<T>::s_pInstance = nullptr;
template <class T>
std::once_flag CSingleton<T>::s_stInstantiated;

#endif
