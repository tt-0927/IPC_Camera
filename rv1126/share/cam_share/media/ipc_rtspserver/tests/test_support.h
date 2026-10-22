/**
 * @FilePath     : test_support.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 11:35:00
 * @Description  : 极简测试支撑：断言、用例注册与统计
 */

/*
 * 不引入 gtest/catch2：主机 CI 与交叉环境下都能直接编译，避免额外依赖。
 */
#pragma once

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

namespace rtsp_test
{

/** 用例。 */
struct Case
{
    /* 用例名（注册时的宏名）。 */
    const char *name;
    /* 用例体；断言失败记录后继续执行，不会中途退出。 */
    std::function<void()> body;
};

/** 用例注册表。 */
std::vector<Case> &Registry();

/** 注册器：在静态初始化期把用例加入注册表。 */
struct Registrar
{
    Registrar(const char *name, std::function<void()> body);
};

/** 断言失败：打印位置、计入失败统计后返回，当前用例继续执行。 */
void Fail(const char *file, int line, const std::string &message);

/** 依次运行全部用例；断言失败不中断，跑完全部用例后按失败断言总数返回退出码。 */
int RunAll();

} // namespace rtsp_test

/* 定义一个用例：静态注册用例体，经 Registrar 在初始化期加入注册表。 */
#define RTSP_TEST_CASE(name)                                                                                                               \
    static void name##_body();                                                                                                             \
    static const ::rtsp_test::Registrar name##_registrar(#name, &name##_body);                                                             \
    static void name##_body()

/* 断言条件成立；失败打印文件与行号并计入失败统计，继续执行后续语句。 */
#define RTSP_CHECK(cond)                                                                                                                   \
    do                                                                                                                                     \
    {                                                                                                                                      \
        if (!(cond))                                                                                                                       \
        {                                                                                                                                  \
            ::rtsp_test::Fail(__FILE__, __LINE__, "断言失败: " #cond);                                                                     \
        }                                                                                                                                  \
    }                                                                                                                                      \
    while (0)

/* 断言两值相等（两侧各求值一次）；失败打印文件与行号并计入失败统计，继续执行。 */
#define RTSP_CHECK_EQ(lhs, rhs)                                                                                                            \
    do                                                                                                                                     \
    {                                                                                                                                      \
        const auto lhs_value = (lhs);                                                                                                      \
        const auto rhs_value = (rhs);                                                                                                      \
        if (!(lhs_value == rhs_value))                                                                                                     \
        {                                                                                                                                  \
            ::rtsp_test::Fail(__FILE__, __LINE__, "断言失败: " #lhs " == " #rhs);                                                          \
        }                                                                                                                                  \
    }                                                                                                                                      \
    while (0)
