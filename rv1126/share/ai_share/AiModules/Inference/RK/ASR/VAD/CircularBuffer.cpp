/**
 * @file CCircularBuffer.CPP
 * @author wucp (wucp@kfb.cn)
 * @date 2025-07-25
 *
 * @brief
 */

#include <cstdio>
#include <algorithm>
#include "CircularBuffer.hpp"

// 构造函数：初始化环形缓冲区
Inference_NS::CCircularBuffer::CCircularBuffer(int32_t nCapacity)
{
    if (nCapacity <= 0)
    {
        /* 容量必须为正 */
        printf("Please specify a positive nCapacity. Given: %d\n", nCapacity);
        exit(0);
    }
    /* 分配存储空间 */
    m_vBuffer.resize(nCapacity);
}

// 扩容缓冲区（仅支持增大容量）
void Inference_NS::CCircularBuffer::resize(int32_t nNewCapacity)
{
    int32_t nCapacity = static_cast<int32_t>(m_vBuffer.size());
    /* 检查新容量是否有效 */
    if (nNewCapacity <= nCapacity)
    {
        printf("nNewCapacity (%d) <= original nCapacity (%d). Skip it.\n", nNewCapacity, nCapacity);
        return;
    }

    int32_t nSize = size();
    /* 空缓冲区直接扩容 */
    if (nSize == 0)
    {
        m_vBuffer.resize(nNewCapacity);
        return;
    }

    std::vector<float> vNewBuffer(nNewCapacity);
    int32_t nStart = m_nHead % nCapacity;   /* 原缓冲区数据起始物理位置 */
    int32_t nDest = m_nHead % nNewCapacity; //* 新缓冲区目标起始位置

    /* 情况1：数据在物理内存中连续（未跨越原缓冲区尾部） */
    if (nStart + nSize <= nCapacity)
    {
        /* 子情况1.1：新缓冲区有连续空间 */
        if (nDest + nSize <= nNewCapacity)
        {
            std::copy(m_vBuffer.begin() + nStart,
                      m_vBuffer.begin() + nStart + nSize,
                      vNewBuffer.begin() + nDest);
        }
        /* 子情况1.2：新缓冲区空间不连续（需分段拷贝） */
        else
        {
            int32_t part1_size = nNewCapacity - nDest;
            /* 拷贝第一部分到新缓冲区尾部 */
            std::copy(m_vBuffer.begin() + nStart,
                      m_vBuffer.begin() + nStart + part1_size,
                      vNewBuffer.begin() + nDest);
            /* 拷贝剩余部分到新缓冲区头部 */
            std::copy(m_vBuffer.begin() + nStart + part1_size,
                      m_vBuffer.begin() + nStart + nSize,
                      vNewBuffer.begin());
        }
    }
    /* 情况2：数据跨越原缓冲区尾部（物理内存不连续） */
    else
    {
        int32_t part1_size = nCapacity - nStart; /* 尾部数据长度 */
        int32_t part2_size = nSize - part1_size; /* 头部数据长度 */

        /* 拷贝尾部数据（原缓冲区start至末尾）*/
        if (nDest + part1_size <= nNewCapacity)
        {
            std::copy(m_vBuffer.begin() + nStart,
                      m_vBuffer.begin() + nStart + part1_size,
                      vNewBuffer.begin() + nDest);
        }
        else
        {
            /* 新缓冲区尾部空间不足时分段拷贝 */
            int32_t first_part = nNewCapacity - nDest;
            std::copy(m_vBuffer.begin() + nStart,
                      m_vBuffer.begin() + nStart + first_part,
                      vNewBuffer.begin() + nDest);
            std::copy(m_vBuffer.begin() + nStart + first_part,
                      m_vBuffer.begin() + nStart + part1_size,
                      vNewBuffer.begin());
        }

        int32_t new_dest = (nDest + part1_size) % nNewCapacity;
        /* 拷贝头部数据（原缓冲区0至part2_size） */
        if (new_dest + part2_size <= nNewCapacity)
        {
            std::copy(m_vBuffer.begin(),
                      m_vBuffer.begin() + part2_size,
                      vNewBuffer.begin() + new_dest);
        }
        else
        {
            /* 新缓冲区空间不连续时分段拷贝 */
            int32_t first_part = nNewCapacity - new_dest;
            std::copy(m_vBuffer.begin(),
                      m_vBuffer.begin() + first_part,
                      vNewBuffer.begin() + new_dest);
            std::copy(m_vBuffer.begin() + first_part,
                      m_vBuffer.begin() + part2_size,
                      vNewBuffer.begin());
        }
    }
    /* 交换新旧缓冲区（原子操作） */
    m_vBuffer.swap(vNewBuffer);
}

/* 向缓冲区尾部添加数据 */
void Inference_NS::CCircularBuffer::push(const float *p, int32_t n)
{
    int32_t nCapacity = static_cast<int32_t>(m_vBuffer.size());
    int32_t nSize = size();
    /* 空间不足时触发扩容 */
    if (n + nSize > nCapacity)
    {
        /* 计算新容量（翻倍或按需） */
        int32_t nNewCapacity = std::max(nCapacity * 2, n + nSize);

        printf("Overflow! n: %d, nSize: %d, n+nSize: %d, nCapacity: %d. Increase nCapacity to: %d.\n",
               n, nSize, n + nSize, nCapacity, nNewCapacity);
        resize(nNewCapacity);
        nCapacity = nNewCapacity;
    }

    int32_t nStart = m_nTail % nCapacity; /* 写入起始位置 */
    m_nTail += n;                         /* 更新逻辑尾部指针 */

    /* 情况1：数据可连续写入 */
    if (nStart + n < nCapacity)
    {
        std::copy(p, p + n, m_vBuffer.begin() + nStart);
        return;
    }

    /* 情况2：数据需分段写入（跨越缓冲区尾部） */
    int32_t part1_size = nCapacity - nStart;
    /* 先填满缓冲区尾部剩余空间 */
    std::copy(p, p + part1_size, m_vBuffer.begin() + nStart);
    /* 剩余数据写入缓冲区头部 */
    std::copy(p + part1_size, p + n, m_vBuffer.begin());
}

/* 从缓冲区读取数据 */
std::vector<float> Inference_NS::CCircularBuffer::get(int32_t nStartIndex, int32_t n) const
{
    /* 校验读取位置合法性 */
    if (nStartIndex < m_nHead || nStartIndex >= m_nTail)
    {
        printf("Invalid nStartIndex: %d. m_nHead: %d, m_nTail: %d\n", nStartIndex, m_nHead, m_nTail);
        return {};
    }

    int32_t nSize = size();
    /* 校验读取长度合法性 */
    if (n < 0 || n > nSize)
    {
        printf("Invalid n: %d. nSize: %d\n", n, nSize);
        return {};
    }

    int32_t nCapacity = static_cast<int32_t>(m_vBuffer.size());
    /* 校验读取范围是否越界 */
    if (nStartIndex - m_nHead + n > nSize)
    {
        printf("Invalid nStartIndex: %d and n: %d. m_nHead: %d, nSize: %d\n", nStartIndex, n, m_nHead, nSize);
        return {};
    }

    int32_t nStart = nStartIndex % nCapacity; /* 物理起始位置 */
    /* 情况1：数据物理连续 */
    if (nStart + n < nCapacity)
    {
        return {m_vBuffer.begin() + nStart, m_vBuffer.begin() + nStart + n};
    }

    /* 情况2：数据跨越缓冲区尾部（需分段拷贝） */
    std::vector<float> ans(n);
    int32_t part1_size = nCapacity - nStart; // 尾部数据长度
    /* 拷贝尾部数据 */
    std::copy(m_vBuffer.begin() + nStart, m_vBuffer.end(), ans.begin());
    /* 拷贝头部剩余数据 */
    int32_t part2_size = n - part1_size;
    std::copy(m_vBuffer.begin(), m_vBuffer.begin() + part2_size, ans.begin() + part1_size);
    return ans;
}

/* 从缓冲区头部移除数据 */
void Inference_NS::CCircularBuffer::pop(int32_t n)
{
    int32_t nSize = size();
    /* 校验移除长度 */
    if (n < 0 || n > nSize)
    {
        printf("Invalid n: %d. nSize: %d\n", n, nSize);
        return;
    }
    m_nHead += n; /* 直接前移逻辑头部指针（惰性物理删除） */
}
