/**
 * @FilePath     : stream_handler.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-04 11:28:41
 * @Description  : 处理AiAppStreamClient的数据
 */

#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <cstring>
#include <iostream>
#include <vector>

// #include "audio_dec.h"
#include "av_handler.hpp"
#include "dlog.h"

class CStreamHandler : public CAVHandler
{
public:
    CStreamHandler() = default;
    ~CStreamHandler() = default;

    /**
     * @brief 开始获取数据
     * @return [*]
     * @note
     */
    bool start() override;

    /**
     * @brief 停止获取数据
     * @return [*]
     * @note
     */
    bool stop() override;

private:
    /**
     * @brief 接受媒体数据
     * @param pData 数据结构体
     */
    void recvDataProcess(const void *pData, int nLen, int nH, int nW) override;
    void recvDataProcess(const Video_NS::VideoFrame_S *pFrame, int nH, int nW) override;
    void recvDataProcess(const void *pData, int nLength) override;

    /**
     * @brief 从 4K 缓冲池分配一帧 (引用计数, 有引用时绝不复用该槽, 全忙则临时分配)
     * @param pSrc 源 4K NV12 帧
     * @param nLen 帧字节数
     * @return 持有池槽或临时缓冲的智能指针
     */
    std::shared_ptr<char[]> allocFullFrame(const char *pSrc, int nLen);

private:
    /* NV12 缩小临时缓冲 (4K 帧缩小 1080p 时复用) */
    std::vector<char> m_scaleBuf;

    /* 4K 帧缓冲池: 预分配 4 块 12MB, 引用计数复用, 消除每帧 new/delete 分配抖动 */
    static constexpr int kFullPoolSlots = 4;
    char                *m_fullPoolBuf[kFullPoolSlots] = {};
    std::atomic<int>     m_fullPoolRefs[kFullPoolSlots] = {};
    int                  m_fullSlot = 0;
    size_t               m_fullFrameSize = 0;
};
