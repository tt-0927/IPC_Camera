/**
 * @FilePath     : record_file.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-28 10:36:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-08 17:28:30
 * @Description  : 通道通讯录制
 */
#pragma once

#include <mutex>

#include <atomic>
#include <thread>
#include <vector>
#include "ffmpeg_record.h"
#include "record_define.h"
#include "video_define.h"
#include "audio_define.h"
#include "data_tools.h"
#include "safe_queue.h"
#include "m3u8_file.h"

class CRecordFile
{
public:
    CRecordFile(int nChnId);

    ~CRecordFile();

    void stop();
    void start(Record_NS::Info_S &stRecordInfos);
    void pause();

    void set_videoInfo(Record_NS::VideoConfigInfo_S stVideoConfigInfo);
    void set_audioInfo(Record_NS::AudioConfigInfo_S stAudioConfigInfo);
    void push(const void *pData, int nLen, Record_NS::MediaDataType_E enType);

    /**
     * @brief 录制线程
     * @return void*
     */
    void thread_record();

    /*初始化录制头,成功返回0*/
    int init_record();
    /*写录制信息*/
    void write_record();

public:
    bool is_record();
    /**
     * @brief 插入数据队列
     * @param [RecordMediaData_S] stMediaData: 媒体数据
     * @return [*] 无
     * @note
     */
    void push_mediaDataQueue(Record_NS::MediaData_S stMediaData)
    {
        int nRet = m_mediaDataQueue.push(stMediaData, SafeQueue<Record_NS::MediaData_S>::TIMEOUT_NONE);
        if (nRet < 0)
        {
            stMediaData.pData.reset();
        }
    }

    /**
     * @brief 获取数据
     * @param [RecordMediaData_S&] stMediaData: 媒体数据
     * @return [*]  BlError_E::0 成功  其他失败
     * @note
     */
    int pop_mediaDataQueue(Record_NS::MediaData_S &stMediaData)
    {
        return m_mediaDataQueue.pop(stMediaData, SafeQueue<Record_NS::MediaData_S>::TIMEOUT_FOREVER);
    }

    /**
     * @brief 判断队列是否为空
     * @return [*] 是否为空
     * @note
     */
    bool isEmpty_mediaDataQueue()
    {
        return m_mediaDataQueue.empty();
    }

    /**
     * @brief 清空队列
     * @return [*]
     * @note
     */
    void clear_mediaDataQueue()
    {
        /* 清空队列 */
        while (!m_mediaDataQueue.empty())
        {
            Record_NS::MediaData_S stMediaData;
            m_mediaDataQueue.pop(stMediaData, SafeQueue<Record_NS::MediaData_S>::TIMEOUT_NONE);
        }
    }

    /* 发送录制完成的文件信息给到control */
    int send_tsFileInfo();
    /* 发送录制的m3u8信息给到control */
    int send_m3u8Info(std::string strEventM3u8FileName);

    /**
     * 将Record::MediaData_S结构体转换为FFmpeg使用的RecordData_S结构体
     * @param stMediaData 输入的媒体数据结构体
     * @return 转换后的FFmpeg媒体数据结构体
     */
    RecordData_S to_ffmpegData(Record_NS::MediaData_S &stMediaData);

    /**
     * 更新记录的日期信息
     */
    void update_recordDate();

    /**
     * 检查是否是新的一天
     * @return 如果是新的一天返回true，否则返回false
     */
    bool is_newDay();

    /**
     * 对记录文件进行切片处理
     */
    void slice();

    /**
     * 冗余备份文件
     */
    void redun_backup();

    /**
     * @brief   : 获取录制ID
     * @return   {int}录制ID
     */
    int get_chnId();

private:
    /* 录制的句柄 */
    FfmpegRecord m_ffmpegRecord;
    /* m3u8文件 */
    std::string m_m3u8Path;
    std::string m_redunPath;
    /* 录制信息 */
    SliceInfo_S m_stSliceInfo;
    std::atomic<bool> m_bResetSliceIndex = {false};
    /* 录制id */
    int m_nChnId = 0;

    /* 录制状态 */
    std::atomic<Record_NS::Status_E> m_nRecordStatus;

    /*录制时间线程*/
    std::thread m_recordTd;
    std::atomic<bool> m_bRunning = {true};

    /* 分片线程相关 */
    std::atomic<bool> m_bSliceActRunning = {false};

    /* 设置录制信息 */
    Record_NS::Info_S m_stRecordInfo;

    /* 记录接收到启动录制的时间 */
    std::string m_strRecvStratTime = std::string();

    /* 数据链表 */
    SafeQueue<Record_NS::MediaData_S> m_mediaDataQueue;

    /* 是否初始化 录制句柄 */
    std::atomic<bool> m_bFirstInit = {false};

    /*是否断开数据接收*/
    std::atomic<bool> m_bDisconnect = {false};

    /*记录视频时间戳*/
    int64_t nVptsMs = 0;
    /*记录音频时间戳*/
    int64_t nAptsMs = 0;

    /*用于判断日期变更*/
    std::string m_curRecordDate;

    CM3U8File m_m3u8;

    /*视频配置*/
    std::vector<Video_NS::VideoConfig_S> m_vstVideoConfig;

    /*是否需要配置变化切片*/
    std::atomic<bool> m_bHandleSlice = {false};
};
