/**
 * @FilePath     : mpp_handle_wrapper.h
 * @Author       : Claude Code
 * @Date         : 2026-01-07
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-08 10:30:11
 * @Description  : MPP模块句柄RAII封装，提供自动资源管理
 */

#pragma once

#include <utility>

extern "C"
{
#include "mpp_vi.h"
#include "mpp_vpss.h"
#include "mpp_venc.h"
}

/**
 * @brief   : MPP句柄RAII包装器模板类
 * @note    : 提供自动资源管理，禁止拷贝，支持移动语义
 * @tparam  {T} 句柄类型（如HiVi_S, HiVpss_S, HiVenc_S）
 * @tparam  {Deleter} 释放函数指针类型
 */
template <typename T, void (*Deleter)(T*)>
class CMppHandle
{
public:
    /**
     * @brief   : 默认构造函数
     * @param   {T*} handle：句柄指针，默认为nullptr
     */
    explicit CMppHandle(T* handle = nullptr) : handle_(handle)
    {
    }

    /**
     * @brief   : 析构函数，自动释放资源
     */
    ~CMppHandle()
    {
        reset();
    }

    /* 禁止拷贝构造 */
    CMppHandle(const CMppHandle&) = delete;

    /* 禁止拷贝赋值 */
    CMppHandle& operator=(const CMppHandle&) = delete;

    /**
     * @brief   : 移动构造函数
     * @param   {CMppHandle&&} other：右值引用
     */
    CMppHandle(CMppHandle&& other) noexcept : handle_(other.handle_)
    {
        other.handle_ = nullptr;
    }

    /**
     * @brief   : 移动赋值运算符
     * @param   {CMppHandle&&} other：右值引用
     * @return  {CMppHandle&} 当前对象引用
     */
    CMppHandle& operator=(CMppHandle&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    /**
     * @brief   : 获取原始句柄指针
     * @return  {T*} 句柄指针
     */
    T* get() const
    {
        return handle_;
    }

    /**
     * @brief   : 获取原始句柄指针的引用
     * @return  {T*&} 句柄指针引用
     */
    T*& get_ref()
    {
        return handle_;
    }

    /**
     * @brief   : 箭头运算符重载，提供智能指针语义
     * @return  {T*} 句柄指针
     */
    T* operator->() const
    {
        return handle_;
    }

    /**
     * @brief   : 解引用运算符重载
     * @return  {T&} 句柄引用
     */
    T& operator*() const
    {
        return *handle_;
    }

    /**
     * @brief   : 布尔转换运算符，判断句柄是否有效
     * @return  {bool} true：有效，false：无效
     */
    explicit operator bool() const
    {
        return handle_ != nullptr;
    }

    /**
     * @brief   : 重置句柄，释放旧资源并接管新句柄
     * @param   {T*} handle：新句柄指针，默认为nullptr
     */
    void reset(T* handle = nullptr)
    {
        if (handle_ && Deleter)
        {
            Deleter(handle_);
        }
        handle_ = handle;
    }

    /**
     * @brief   : 释放句柄所有权，不释放资源
     * @return  {T*} 原句柄指针
     */
    T* release()
    {
        T* temp = handle_;
        handle_ = nullptr;
        return temp;
    }

private:
    /* 原始句柄指针 */
    T* handle_;
};

/* VI句柄类型别名 */
using ViHandle = CMppHandle<HiVi_S, mppVi_release>;

/* VPSS句柄类型别名 */
using VpssHandle = CMppHandle<HiVpss_S, mppVpss_release>;

/* VENC句柄类型别名 */
using VencHandle = CMppHandle<HiVenc_S, mppVenc_release>;
