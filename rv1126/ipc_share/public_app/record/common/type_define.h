/**
 * @FilePath     : type_define.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-28 10:36:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-01 19:49:38
 * @Description  : 录制相关类型定义
 */

#pragma once

#include <string>

/*定义视频分片时长，秒*/
#define SLICING_TIME 60
 
typedef struct RecordData
{
    /* 数据类型 */
    int nType = 0;
    /* 流媒体数据 */
    unsigned char *pData = nullptr;
    /* 流媒体数据大小 */
    int nSize = 0;
    /* 判断是否是关键帧 I帧*/
    int nKey = 0;
} RecordData_S;

/*数据封装结构*/
typedef struct RecordFrame
{
    /* 数据 */
    unsigned char *pData = nullptr;
    /* 数据大小 */
    int nSize = 0;
    /* 数据是否是关键值 */
    int nKey = 0;
    /* 流索引值 */
    int nStreamIndex = 0;
    /* 显示时间戳 */
    int64_t nPts = 0;
    /* 解码时间戳 */
    int64_t nDts = 0;
} RecordFrame_S;

typedef struct _NVR_RECORD_M3U8_
{
    /* 存储m3u8文件名 */
    std::string m3u8Path;
    /* 存储冗余文件路径 */
    std::string redunPath;
} RecordM3u8_S;

/* 来保存每个视频文件分片数据 */
typedef struct SliceInfo
{
    /* 带路径文件名 */
    std::string filename;
    /* 开始时间 */
    std::string startTime;
    /* 开始时间,从00:00:00开始经过的毫秒数 */
    int64_t nStartTimeMs = 0;
    /* 结束时间,从00:00:00开始经过的毫秒数 */
    int64_t nEndTimeMs = 0;
    /*开始分片时的实际时间戳,毫秒*/
    int64_t nStartTimestampMs = 0;
    /*结束分片时的实际时间戳,毫秒*/
    int64_t nEndTimestampMs = 0;
    /* 事件标志位 */
    int nEventFlag = 0;

    /* 视频录制标志位 */
    int nVideoFlag = 0;
    /* 宽 */
    int nVencWidth = 0;
    /* 高 */
    int nVencHeight = 0;
    /* 帧率 */
    int nFps = 0;
    /* 真实帧率 */
    int nRealFrameRate = 0;
    /* ffmpeg-编码器ID */
    int nVideoCodeID = 0;

    /* 音频录制标志位 */
    int nAudioFlag = 0;
    /* 采样率 */
    int nSampleRate = 0;
    /* ffmpeg-编码器ID AV_CODEC_ID_AAC*/
    int nAudioCodeID = 0;
    /*采样格式 AV_SAMPLE_FMT_S16*/
    int nSampleFmt = 0;
    /*采样通道 1单通道，2是双通道*/
    int nChannel = 0;
    /* 分片大小 */
    int nSize = 0;
    /* 分片序号 */
    int nIndex = 0;

    /*视频帧计数值，当前片段视频帧数，每个片段重新计数*/
    int nVideoIndex = 0;

} SliceInfo_S;
