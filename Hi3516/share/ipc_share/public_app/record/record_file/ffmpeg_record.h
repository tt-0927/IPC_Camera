/**
 * @FilePath     : ffmpeg_record.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-28 10:36:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-22 09:47:08
 * @Description  : ffmpeg录制类
 */
#pragma once

#include <mutex>

extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/time.h"
#include "libavutil/opt.h"
}
#include "dlog.h"
#include "type_define.h"

/* 输出流配置 */
typedef struct _OUTPUTSTREAM_
{
    /* 流数据 */
    AVStream *pAvStream = nullptr;
    /* 流索引值 */
    int nIndex = 0;
} OutputStream_S;

class FfmpegRecord
{
public:
    FfmpegRecord() = default;
    ~FfmpegRecord() = default;

    /**
     * @brief 检查是否已经初始化
     * @return 如果初始化返回true，否则返回false
     */
    bool is_init();

    /**
     * @brief 初始化记录模块
     * @param stSliceInfo 分片信息结构体
     * @return 成功返回0，失败返回错误码
     */
    int init(SliceInfo_S stSliceInfo);

    /**
     * @brief 反初始化记录模块
     */
    void deinit();

    /**
     * @brief 重置记录模块的状态
     */
    void reset();

    /**
     * @brief 重置上一次音视频pts
     */
    int reset_lastPts();

    /**
     * @brief 设置媒体信息
     * @param stSliceInfo 分片信息结构体
     */
    void set_mediaInfo(SliceInfo_S stSliceInfo);

    /**
     * @brief 获取媒体信息
     * @return 分片信息结构体
     */
    SliceInfo_S get_mediaInfo();

    /**
     * @brief 写入记录数据
     * @param stRecordDate 记录数据结构体引用
     * @return 成功返回0，失败返回错误码
     */
    int write(RecordData_S &stRecordDate);

    /**
     * @brief 获取已写入的视频帧数
     * @return 视频帧数
     */
    int64_t get_videoCount();

    /**
     * @brief 获取已写入的音频帧数
     * @return 音频帧数
     */
    int64_t get_audioCount();

    /**
     * @brief 清除已写入的帧数计数
     */
    void clear_count();

    /**
     * @brief 获取当前录制的时长（毫秒）
     * @return 时长（毫秒）
     */
    int get_durationMs();

    /**
     * @brief 获取录制的开始时间
     * @return 开始时间字符串
     */
    std::string get_startTime();

    /**
     * @brief 设置音频时间戳
     * @param nPts 时间戳值
     */
    void set_audioPts(int64_t nPts);

    /**
     * @brief 获取音频时间戳
     * @return 音频时间戳值
     */
    int64_t get_audioPts();

    /**
     * @brief 设置视频时间戳
     * @param nPts 时间戳值
     */
    void set_videoPts(int64_t nPts);

    /**
     * @brief 获取视频时间戳
     * @return 视频时间戳值
     */
    int64_t get_videoPts();

    /**
     * @brief 设置视频帧率
     * @param nFrameRate 帧率值
     */
    void set_frameRate(int nFrameRate);

    /**
     * @brief 获取录制的开始时间戳（毫秒）
     * @return 开始时间戳（毫秒）
     */
    int64_t get_startTimeStampMs();

    /**
     * @brief 获取录制的结束时间戳（毫秒）
     * @return 结束时间戳（毫秒）
     */
    int64_t get_endTimestampMs();

private:
    /**
     * @brief 初始化开始时间
     * @param nCurrentTimeMs 当前时间戳（毫秒）
     * @return 成功返回0，失败返回错误码
     */
    int init_startTime(uint64_t nCurrentTimeMs);

    /**
     * @brief 处理音视频同步
     * @param nType 类型标识（视频或音频）
     * @param stFrameData 帧数据结构体引用
     * @param stSrcTimebase 源时间基
     * @param stDstTimebase 目标时间基
     * @return 成功返回0，失败返回错误码
     */
    int av_sync(int nType, RecordFrame_S &stFrameData, AVRational &stSrcTimebase, AVRational &stDstTimebase);

    /**
     * @brief 初始化上下文
     * @return 成功返回0，失败返回错误码
     */
    int init_context();

    /**
     * @brief 初始化视频流
     */
    void init_videoStream();

    /**
     * @brief 初始化音频流
     */
    void init_audioStream();

    /**
     * @brief 打开文件以进行写入
     * @return 成功返回0，失败返回错误码
     */
    int open_file();

    /**
     * @brief 写入文件头
     * @param pData 文件头数据指针
     * @param nSize 文件头数据大小
     * @return 成功返回0，失败返回错误码
     */
    int write_head(unsigned char *pData, int nSize);

private:
    /* 流信息指针 */
    AVFormatContext *m_pFormatContext = nullptr;

    /* 参数传参 */
    SliceInfo_S m_stSliceInfo;

    /* 视频流配置 */
    OutputStream_S m_stVideoStream;
    /* 音频流配置 */
    OutputStream_S m_stAudioStream;

    /*视频首帧*/
    int m_nFirstFrame = 0;

    /*开始录制时,当天00:00:00到现在经过的毫秒数*/
    int64_t m_nRecStartTimeMs = 0;

    /*开始录制时的实际时间戳，毫秒*/
    int64_t m_nStartTimeStampMs = 0;
    /* 音频帧数 */
    int64_t m_nAudioCount = 0;
    /* 视频帧数 */
    int64_t m_nVideoCount = 0;
    /*记录视频时间戳*/
    int64_t m_nVptsMs = 0;
    /*记录音频时间戳*/
    int64_t m_nAptsMs = 0;
    /* 记录上一帧的视频pts */
    int64_t m_lastVideoPts = -1;
    /* 记录上一帧的音频pts */
    int64_t m_lastAudioPts = -1;

    std::mutex m_mutex;
};
