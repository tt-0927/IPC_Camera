/**
 * @FilePath     : stream_handler.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-28 17:49:59
 * @Description  : 处理AiAppStream的数据
 */

#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <cstring>
#include <iostream>

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
     * @brief   : 接受媒体数据
     * @param    {ot_video_frame_info} *pFrameInfo 视频图像帧信息
     */
    void recvDataProcess(const ot_video_frame_info *pFrameInfo) override;

    /**
     * @brief   : 接受媒体数据
     * @param    {ot_audio_frame} *pFrame 音频帧信息
     */
    void recvDataProcess(const ot_audio_frame *pFrame) override;

    /**
     * @brief   : 接受视频媒体数据
     * @param    {void} *pData 视频图像帧数据指针
     * @param    {int} nLength 字节数
     * @param    {int} nWidth 图像宽度
     * @param    {int} nHeight 图像高度
     */
    void recvDataProcess(const void *pData, int nLength, int nWidth, int nHeight) override;

    /**
     * @brief   : 接受音频媒体数据
     * @param    {void} *pData 音频帧数据指针
     * @param    {int} nLength 字节数
     */
    void recvDataProcess(const void *pData, int nLength) override;
};
