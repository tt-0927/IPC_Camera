/**
 * @FilePath     : av_handler.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-28 17:49:39
 * @Description  : 音视频媒体数据转发
 */

#pragma once

#include <iostream>

#include "IpcRet.h"
#include "SignalSlot.h"
#include "common_process.h"
// #include "media_ffmpeg.h"
#include "stream_process_ext.hpp"
#include "video_define.h"

class CAVHandler
{
public:

    CAVHandler()
    {
    }

    virtual ~CAVHandler()
    {
        disconnect(&m_sendVideoDataSig);
        disconnect(&m_sendAudioDataSig);
    }

    /**
     * @brief 开始获取数据
     * @return [*]
     * @note
     */
    virtual bool start() = 0;

    /**
     * @brief 停止获取数据
     * @return [*]
     * @note
     */
    virtual bool stop() = 0;

    /**
     * @brief 接受媒体数据
     * @param pData 数据结构体
     */
    virtual void recvDataProcess(const ot_video_frame_info *pFrameInfo) = 0;

    /**
     * @brief 接受媒体数据
     * @param pData 数据结构体
     */
    virtual void recvDataProcess(const ot_audio_frame *pFrame) = 0;

    /**
     * @brief   : 接受视频媒体数据
     * @param    {void} *pData 视频图像帧数据指针
     * @param    {int} nLength 字节数
     * @param    {int} nWidth 图像宽度
     * @param    {int} nHeight 图像高度
     */
    virtual void recvDataProcess(const void *pData, int nLength, int nWidth, int nHeight) = 0;

    /**
     * @brief   : 接受音频媒体数据
     * @param    {void} *pData 音频帧数据指针
     * @param    {int} nLength 字节数
     */
    virtual void recvDataProcess(const void *pData, int nLength) = 0;

protected:
    /**
     * @description: 发送视频数据
     * @param [MediaData_S] stMediaData: 媒体数据
     * @return [*]
     * @others:
     */
    void send_videoData(MediaData_S stMediaData)
    {
        m_sendVideoDataSig.emit(stMediaData);
    }

    /**
     * @description: 发送音频数据
     * @param [MediaData_S] stMediaData: 媒体数据
     * @return [*]
     * @others:
     */
    void send_audioData(MediaData_S stMediaData)
    {
        m_sendAudioDataSig.emit(stMediaData);
    }

public:
    TanSignal<MediaData_S> m_sendVideoDataSig;
    TanSignal<MediaData_S> m_sendAudioDataSig;

protected:

};
