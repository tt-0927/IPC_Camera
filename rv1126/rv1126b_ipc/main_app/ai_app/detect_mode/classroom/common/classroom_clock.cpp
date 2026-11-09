/**
 * @FilePath     : classroom_clock.cpp
 * @Author       : leiyy
 * @Date         : 2026-09-22 10:05:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-22 10:05:11
 * @Description  : 单调时钟实现（无锁、无堆分配，可在任意线程调用）
 */

#include "classroom_clock.h"

#include <ctime>

namespace Classroom
{
int64_t classroomNowMs()
{
    struct timespec stTs;

    clock_gettime(CLOCK_MONOTONIC, &stTs);
    return static_cast<int64_t>(stTs.tv_sec) * 1000 + stTs.tv_nsec / 1000000;
}

uint64_t classroomNowUs()
{
    struct timespec stTs;

    clock_gettime(CLOCK_MONOTONIC, &stTs);
    return static_cast<uint64_t>(stTs.tv_sec) * 1000000 + stTs.tv_nsec / 1000;
}
}
