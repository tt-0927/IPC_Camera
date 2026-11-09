/**
 * @FilePath     : classroom_clock.h
 * @Author       : leiyy
 * @Date         : 2026-09-22 10:05:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-22 10:05:11
 * @Description  : 课堂子系统统一时钟出口
 */

#pragma once

#include <cstdint>

namespace Classroom
{
/**
 * @brief   : 单调时钟当前时刻
 * @return   {int64_t} 毫秒，起算点无绝对意义，只可用作差值
 */
int64_t classroomNowMs();

/**
 * @brief   : 单调时钟当前时刻（微秒精度，供推理耗时统计用）
 * @return   {uint64_t} 微秒
 */
uint64_t classroomNowUs();
}
