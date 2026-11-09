/**
 * @FilePath     : rga_pool.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-08 13:57:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-08 13:57:00
 * @Description  : 定尺寸缓冲复用池实现
 */

#include "rga_pool.h"

#include <new>

#include "dlog.h"

namespace Classroom
{
CRgaPool::~CRgaPool()
{
    deinit();
}

bool CRgaPool::init(uint32_t nBufNum, uint32_t nBufBytes)
{
    /* 至少 2 块：单块复用会踩中「同指针 + 同尺寸 → 输出疑似被缓存」 */
    if (nBufNum < 2 || nBufBytes < 4 || (nBufBytes % 4) != 0)
    {
        dlog_error("rga_pool init: 入参非法 num=%u bytes=%u（块数至少 2，字节数至少 4 且为 4 的倍数）", nBufNum, nBufBytes);
        return false;
    }

    std::vector<RgaBufPtr> apBuf;
    try
    {
        apBuf.reserve(nBufNum);
        for (uint32_t i = 0; i < nBufNum; ++i)
        {
            apBuf.push_back(std::make_shared<std::vector<uint8_t>>(nBufBytes));
        }
    }
    catch (const std::bad_alloc &)
    {
        dlog_error("rga_pool init: 申请 %u 块 × %u 字节失败（内存不足）", nBufNum, nBufBytes);
        return false;
    }

    std::lock_guard<std::mutex> stLock(m_mtx);
    m_apBuf = std::move(apBuf);
    m_stStat = CRgaPoolStat_S();
    m_stStat.u32BufNum = nBufNum;
    m_stStat.u32BufBytes = nBufBytes;
    m_nCursor = 0;
    m_bReady = true;

    dlog_info("rga_pool 就绪: %u 块 × %u 字节 = %u KB", nBufNum, nBufBytes, nBufNum * nBufBytes / 1024);
    return true;
}

void CRgaPool::deinit()
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_bReady)
    {
        return;
    }
    m_bReady = false;

    uint32_t nBusy = 0;
    for (const auto &spBuf : m_apBuf)
    {
        if (spBuf && spBuf.use_count() > 1)
        {
            ++nBusy;
        }
    }
    if (nBusy > 0)
    {
        dlog_warn("rga_pool deinit: 仍有 %u 块缓冲被借出未归还", nBusy);
    }
    m_apBuf.clear();
    m_nCursor = 0;
}

RgaBufPtr CRgaPool::acquire(uint32_t nNeedBytes)
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_bReady)
    {
        return RgaBufPtr();
    }
    if (nNeedBytes > m_stStat.u32BufBytes)
    {
        ++m_stStat.u64AcquireFail;
        dlog_error("rga_pool: 需要 %u 字节 > 单块容量 %u 字节", nNeedBytes, m_stStat.u32BufBytes);
        return RgaBufPtr();
    }

    const int nTotal = static_cast<int>(m_apBuf.size());
    for (int i = 0; i < nTotal; ++i)
    {
        /* 从上次给出位置之后开始：连续两次借出一定不是同一块地址 */
        const int nIdx = static_cast<int>((m_nCursor + static_cast<uint32_t>(i)) % nTotal);
        const RgaBufPtr &spBuf = m_apBuf[nIdx];
        if (spBuf && spBuf.use_count() <= 1)
        {
            m_nCursor = static_cast<uint32_t>((nIdx + 1) % nTotal);
            ++m_stStat.u64AcquireOk;
            updateBusyLocked();
            return spBuf;
        }
    }

    ++m_stStat.u64AcquireFail;
    return RgaBufPtr();
}

CRgaPoolStat_S CRgaPool::stat() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_stStat;
}

bool CRgaPool::isReady() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_bReady;
}

int CRgaPool::nIdle() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    if (!m_bReady)
    {
        return -1;
    }
    int nIdleNum = 0;
    for (const auto &spBuf : m_apBuf)
    {
        if (spBuf && spBuf.use_count() <= 1)
        {
            ++nIdleNum;
        }
    }
    return nIdleNum;
}

void CRgaPool::updateBusyLocked()
{
    uint32_t nBusy = 0;
    for (const auto &spBuf : m_apBuf)
    {
        if (spBuf && spBuf.use_count() > 1)
        {
            ++nBusy;
        }
    }
    if (nBusy > m_stStat.u32BusyMax)
    {
        m_stStat.u32BusyMax = nBusy;
    }
}
}
