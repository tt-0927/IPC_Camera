/**
 * @FilePath     : stream_process_ext.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-09 08:55:19
 * @Description  : 流处理拓展
 */

#pragma once

#include <iostream>
#include <memory>

#include "share_data.h"
#include "stream_video.h"
#include "stream_audio.h"

/* 媒体数据类型 */
typedef enum _MediaDataType_
{
    VIDEO_DATA = 0, /* 视频 */
    AUDIO_DATA,     /* 音频 */
    SUB_DATA,       /* 字幕 */
} MediaDataType_E;

/* 媒体参数信息 */
typedef struct _MediaParam_
{
    int nVideoWidth;               /* 视频宽度 */
    int nVideoHeight;              /* 视频高度 */
    double dFrameRate;             /* 帧率 */
    PIXEL_FORMAT_E enPixelFormat; /*RK-图像像素格式 */
    int nSampleRate;               /* 采样率 */
    int nChannel;                  /* 采样通道 */
    AUDIO_BIT_WIDTH_E enBitWidth; /*音频采样位深*/
    void clear()
    {
        nVideoWidth    = 0;
        nVideoHeight   = 0;
        dFrameRate     = 0.0;
        enPixelFormat  = RK_FMT_BUTT;
        nSampleRate    = 0;
        nChannel       = 0;
        enBitWidth = AUDIO_BIT_WIDTH_BUTT;
    }

    _MediaParam_()
    {
        clear();
    }

    void print() const
    {
        std::cout << "\n编码信息:=============" << std::endl;
        std::cout << "视频宽度:" << nVideoWidth << std::endl;
        std::cout << "视频高度:" << nVideoHeight << std::endl;
        std::cout << "帧率:" << dFrameRate << std::endl;
        std::cout << "rk-图像像素格式:" << std::to_string(enPixelFormat) << std::endl;
        std::cout << "采样率:" << nSampleRate << std::endl;
        std::cout << "采样通道:" << nChannel << std::endl;
        std::cout << "音频采样位深:" << std::to_string(enBitWidth) << std::endl;
        std::cout << "end:=============" << std::endl;
    }

} MediaParam_S;

/* 媒体信息 */
typedef struct _MediaData_
{
    MediaDataType_E enType;        /* 数据类型 */
    int64_t         nSize;         /* 数据大小 */
    bool            bIFrame;       /* 是否为I帧 */

    std::shared_ptr<char[]> pData; /* 媒体数据智能指针 */

    /* 全分辨率帧*/
    std::shared_ptr<char[]> pFullData;
    int nFullWidth  = 0;
    int nFullHeight = 0;

    // std::string strFileName;       /* 文件名 */
    MediaParam_S stMediaParam;   /* 媒体编码信息 */

    void clear()
    {
        enType  = VIDEO_DATA;
        nSize   = 0;
        bIFrame = false;
        
        pFullData.reset();
        nFullWidth  = 0;
        nFullHeight = 0;

        // strFileName.clear();
        stMediaParam.clear();
    }

    _MediaData_()
    {
        clear();
    }

} MediaData_S;
