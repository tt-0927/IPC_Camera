/***
 * @FilePath     : base_define.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-12-11 10:35:46
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-28 10:36:15
 * @Description  : 基本类型定义
 */

#pragma once

#include <iostream>
#include <cstdio>
#include <vector>

// 针对容器的 size 实现
template <typename T>
typename std::enable_if<std::is_same<T, std::string>::value, size_t>::type
size_impl(const T &v)
{
    return v.size(); // 对于 std::string，返回字符串长度
}

// 针对基本类型的 size 实现
template <typename T>
typename std::enable_if<std::is_fundamental<T>::value, size_t>::type
size_impl(const T &)
{
    return sizeof(T); // 对于基本类型，返回类型的大小
}

// 针对容器和基本类型的 c_str 实现
template <typename T>
typename std::enable_if<std::is_same<T, std::string>::value, const char *>::type
c_str_impl(const T &v)
{
    return v.c_str(); // 对于 std::string，返回字符串
}

// 针对其他类型的 c_str 实现
template <typename T>
typename std::enable_if<!std::is_same<T, std::string>::value, const char *>::type
c_str_impl(const T &)
{
    return nullptr; // 对于其他类型，返回nullptr
}

// 主宏：用于定义类型
#define BACE_DEFINE_TYPE(TypeName, T)                           \
    struct TypeName                                             \
    {                                                           \
        T value;                                                \
        TypeName() : value(T()) {}                              \
        TypeName(T v) : value(v) {}                             \
        operator T() const { return value; }                    \
        operator T &() { return value; }                        \
        TypeName &operator=(T v)                                \
        {                                                       \
            value = v;                                          \
            return *this;                                       \
        }                                                       \
        size_t size() const { return size_impl(value); }        \
        size_t lenght() const { return size(); }                \
        const char *c_str() const { return c_str_impl(value); } \
    };

/* 使用宏定义生成不同类型的封装 */
/* 通道id类型 */
BACE_DEFINE_TYPE(ChnId, int)

/* 路径类型 */
BACE_DEFINE_TYPE(Path, std::string)
/* 密码类型 */
BACE_DEFINE_TYPE(Password, std::string)