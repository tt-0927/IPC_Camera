/**
 * @FilePath     : log_gate.h
 * @Author       : leiyy
 * @Date         : 2026-10-29 09:31:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-29 09:31:00
 * @Description  : 异常路径的日志闸门
 */

#pragma once

#include <cstdint>

/* 异常路径的日志闸门：这些分支每拍都可能命中（模型没交付时更是每拍全中），
   逐拍打日志会把 stream.log 刷到不可用 */
inline bool logOpen(uint64_t u64Count, uint32_t u32ShiftBits = 5)
{
    return (0 == (u64Count & ((UINT64_C(1) << u32ShiftBits) - 1)));
}
