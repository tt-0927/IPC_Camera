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
};
