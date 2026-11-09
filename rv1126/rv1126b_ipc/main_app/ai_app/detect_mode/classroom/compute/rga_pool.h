/**
 * @FilePath     : rga_pool.h
 * @Author       : leiyy
 * @Date         : 2026-10-08 13:57:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-08 13:57:00
 * @Description  : RGA / 模型输入缓冲复用池
 */

#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace Classroom
{
typedef std::shared_ptr<std::vector<uint8_t>> RgaBufPtr;

/**
 * @brief 池计数
 */
typedef struct _RgaPoolStat_
{
    uint64_t u64AcquireOk = 0;
    uint64_t u64AcquireFail = 0;   /* 全部在借出时被拒 —— 该降采样或加缓冲 */
    uint32_t u32BusyMax = 0;       /* 同时在借出的峰值，逼近容量说明池偏小 */
    uint32_t u32BufNum = 0;
    uint32_t u32BufBytes = 0;
} CRgaPoolStat_S;

/**
 * @brief   : 定尺寸缓冲复用池
 * @note    : 线程安全。一个尺寸档一个实例（整帧档 / 裁剪档分开建，避免小任务占着大块内存）
 */
class CRgaPool
{
public:
    CRgaPool() = default;
    ~CRgaPool();

    CRgaPool(const CRgaPool &) = delete;
    CRgaPool &operator=(const CRgaPool &) = delete;

    /**
     * @brief   : 一次性分配 nBufNum 块 nBufBytes 缓冲
     * @param    {uint32_t} nBufNum：缓冲块数，至少 2
     * @param    {uint32_t} nBufBytes：单块字节数，必须 4 对齐
     * @return   {bool} 参数非法或分配失败返回 false
     */
    bool init(uint32_t nBufNum, uint32_t nBufBytes);
    void deinit();

    /**
     * @brief   : 借出一块不小于 nNeedBytes 的缓冲
     * @param    {uint32_t} nNeedBytes：需要的字节数
     * @return   {RgaBufPtr} 无空闲或容量不足返回 nullptr
     */
    RgaBufPtr acquire(uint32_t nNeedBytes);

    CRgaPoolStat_S stat() const;
    bool isReady() const;

    /**
     * @brief   : 空闲块数（此刻 `use_count()<=1` 的块，与 acquire 的判据同一个）
     * @return   {int} 未就绪返回 -1——与"就绪但全借空(0)"分开：前者是装配问题，后者是背压
     */
    int nIdle() const;

private:
    void updateBusyLocked();

private:
    mutable std::mutex m_mtx;
    std::vector<RgaBufPtr> m_apBuf;    /* 永久持有：块地址稳定，满足 RGA 对 dst 指针连续性的要求 */
    uint32_t m_nCursor = 0;            /* 环形游标，规避「同指针+同尺寸」输入被后端缓存的实测风险 */
    bool m_bReady = false;
    CRgaPoolStat_S m_stStat;
};
}
