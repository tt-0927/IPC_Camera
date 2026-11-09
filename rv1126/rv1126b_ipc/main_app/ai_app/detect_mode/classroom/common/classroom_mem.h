/**
 * @FilePath     : classroom_mem.h
 * @Author       : leiyy
 * @Date         : 2026-09-22 10:07:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-22 10:07:11
 * @Description  : 内存观测与准入判定
 */

#pragma once

#include <cstdint>

namespace Classroom
{
/**
 * @brief 一次内存采样的结果
 */
typedef struct _MemInfo_
{
    bool    bValid = false;          /* 采样失败时为 false，上层不得据此放行加载 */
    int64_t llMemAvailableKb = 0;    /* 整机可用内存 */
    int64_t llVmRssKb = 0;           /* 本进程驻留 */
    int64_t llMmzUsedKb = 0;         /* 媒体 DMA 已用（含 NPU 权重） */
    int64_t llThreadNum = 0;         /* 本进程线程数 */
} CMemInfo_S;

/**
 * @brief   : 采样一次内存
 * @return   {CMemInfo_S} 失败时 bValid=false 且数值全 0
 */
CMemInfo_S classroomQueryMem();

/**
 * @brief   : 纯准入判定（无副作用，可单测）
 * @param    {CMemInfo_S} stMem：采样值
 * @param    {int64_t} llNeedKb：本次要新增的常驻内存（含 dma-buf 部分）
 * @param    {int64_t} llReserveKb：必须给系统与其它模块留出的底线余量
 * @return   {bool} 允许准入返回 true
 */
bool classroomCanAfford(const CMemInfo_S &stMem, int64_t llNeedKb, int64_t llReserveKb);
}
