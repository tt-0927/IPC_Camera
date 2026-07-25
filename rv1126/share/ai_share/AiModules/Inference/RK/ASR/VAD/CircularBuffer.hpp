/**
 * @file CCircularBuffer.HPP
 * @author wucp (wucp@kfb.cn)
 * @date 2025-07-25
 *
 * @brief
 */
#pragma once

#include <cstdint>
#include <vector>

namespace Inference_NS
{

    /**
     * @brief 环形缓冲区类，用于高效管理音频流数据
     *
     * 提供缓冲区操作接口，适合流式音频处理场景
     */
    class CCircularBuffer
    {
    public:
        /**
         * @brief 构造函数 - 创建指定容量的环形缓冲区
         * @param nCapacity 缓冲区容量（元素数量）
         * @note 容量应该足够大，如果初始化失败将退出程序
         */
        explicit CCircularBuffer(int32_t nCapacity);

        /**
         * @brief 向缓冲区添加数据
         * @param p 待添加数据的起始地址指针
         * @param n 待添加数据的元素数量
         * @note 当剩余空间不足时，先扩容再添加数据
         */
        void push(const float *p, int32_t n);

        /**
         * @brief 从缓冲区获取数据
         * @param nStartIndex 起始位置索引（逻辑索引）
         * @param n 需要获取的元素数量
         * @return 包含请求元素的浮点数组向量
         */
        std::vector<float> get(int32_t nStartIndex, int32_t n) const;

        /**
         * @brief 从缓冲区移除数据
         * @param n 需要移除的元素数量
         * @note 只能移除开头部分数据（先进先出）
         */
        void pop(int32_t n);

        /**
         * @brief 获取缓冲区当前有效数据量
         * @return 缓冲区中存储的有效元素数量
         */
        int32_t size() const { return m_nTail - m_nHead; }

        /**
         * @brief 获取当前头指针位置
         * @return 头指针的线性索引值
         */
        int32_t head() const { return m_nHead; }

        /**
         * @brief 获取当前尾指针位置
         * @return 尾指针的线性索引值
         */
        int32_t tail() const { return m_nTail; }

        /**
         * @brief 重置缓冲区
         * @note 清空所有数据，初始化头尾指针
         */
        void reset()
        {
            m_nHead = 0;
            m_nTail = 0;
        }

        /**
         * @brief 调整缓冲区容量
         * @param nNewCapacity 新的缓冲区容量
         * @note 仅支持扩容操作（新容量必须大于当前容量）
         */
        void resize(int32_t nNewCapacity);

    private:
        std::vector<float> m_vBuffer; /* 底层数据存储容器 */

        // 线性索引指针（永不回绕）
        int32_t m_nHead = 0;  /* 缓冲区逻辑起始位置 */
        int32_t m_nTail = 0;  /* 缓冲区逻辑结束位置 */
    };

} // namespace Inference_NS
