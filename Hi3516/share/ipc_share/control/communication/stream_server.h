/**
 * @FilePath     : stream_server.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-30 13:57:22
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-20 17:30:00
 * @Description  : 录制送流、配置服务端
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <utility>

#include "IpcRet.h"
#include "Singleton.h"
#include "video_define.h"
#include "audio_define.h"
#include "IOBase.h"
#include "UDSServer.h"


class CStreamServer : public CSingleton<CStreamServer>
{
    CStreamServer() = default;

public:
    ~CStreamServer();
    friend class CSingleton<CStreamServer>;

    /**
     * @brief   : 初始化录制送流、配置服务端
     * @return   {int} 0：成功 小于零：失败
     */
    int init();

    /**
     * @brief   : 去初始化录制送流、配置服务端
     */
    void deinit();

    /**
     * @brief   : 发送数据、命令码
     * @param    {string} data：数据
     * @param    {int} nActionCode：命令码
     * @param    {void} *pHandle：通讯句柄
     * @return   {int} 0：成功 小于零：失败
     */
    int send(std::string data, int nActionCode, void *pHandle = nullptr);

    /**
     * @brief   : 发送数据、命令码
     * @param    {void} *pData：数据
     * @param    {int} nLen：数据长度
     * @param    {int} nActionCode：命令码
     * @param    {void} *pHandle：通讯句柄
     * @return   {int} 0：成功 小于零：失败
     */
    int send(void *pData, int nLen, int nActionCode, void *pHandle = nullptr);

    /**
     * @brief   : 处理心跳
     * @param    {Message_S&} stMessage：数据回调参数
     * @param    {UserParam_S} &stUserParam：用户参数
     */
    void deal_heartbeat(Net::Message_S &stMessage, Net::UserParam_S &stUserParam);

    /**
     * @brief   : 处理连接状态
     * @param    {Message_S&} stMessage：数据回调参数
     * @param    {UserParam_S} &stUserParam：用户参数
     */
    void deal_status(Net::Message_S &stMessage, Net::UserParam_S &stUserParam);

    /**
     * @brief   : 处理数据
     * @param    {Message_S&} stMessage：数据回调参数
     * @param    {UserParam_S} &stUserParam：用户参数
     */
    void deal_message(Net::Message_S &stMessage, Net::UserParam_S &stUserParam);

    /**
     * @brief   : 填充不包含Return返回码的消息体头部
     * @param    {string&} data：要填充到消息体的data数据
     * @param    {int} nActionCode：命令码
     */
    void fill_head(std::string &strData, int nActionCode);

    /**
     * @brief   : 填充包含Return返回码的消息体头部
     * @param    {string&} data：要填充到消息体的data数据
     * @param    {int} nActionCode：命令码
     * @param    {int} nReturn：返回码
     */
    void fill_returnHead(std::string &strData, int nActionCode, int nRetCode);

    /**
     * @brief   : 外部送视频数据
     * @param    {VideoFrame_S} *pVideoFrame：视频帧数据指针
     * @return   {int}非0：失败
     */
    int sendVideoData(Video_NS::VideoFrame_S *pVideoFrame);

    /**
     * @brief   : 发送 VENC 只读视频帧到录制进程
     * @param   {uint8_t*} pData：VENC pack 数据地址，仅在本次调用期间有效
     * @param   {int} nDataLen：视频数据长度
     * @return  {int} 0：成功，非0：失败
     * @note    : 录制异步队列入队时复制一次，不保存 VENC 原始指针。
     */
    int sendVideoData(const uint8_t *pData, int nDataLen);

    /**
     * @brief   : 发送共享媒体帧到录制进程（引用计数零拷贝入队）
     * @param   {const Video_NS::SharedMediaFrame_S&} stSharedFrame：共享帧
     * @return  {int} 0：成功，非0：失败
     * @note    : 入队不复制数据，仅增加 shared_ptr 引用计数，
     *            与 RTSP/RTMP 共享同一份 buffer，降低多消费者总内存。
     */
    int sendVideoData(const Video_NS::SharedMediaFrame_S &stSharedFrame);

    /**
     * @brief   : 外部送视频配置
     * @param    {VideoConfig_S} &stVideoConfig：视频配置
     * @return   {int}非0：失败
     */
    int sendVideoConfig(const Video_NS::VideoConfig_S &stVideoConfig);

    /**
     * @brief   : 外部送音频数据
     * @param    {AudioFrame_S} *pAudioFrame：音频帧数据指针
     * @return   {int}非0：失败
     */
    int sendAudioData(Audio_NS::AudioFrame_S *pAudioFrame);

    /**
     * @brief   : 向录制进程同步固定录制音频配置
     * @return   {int} 0：成功，非0：失败
     * @note    : 录制独立使用16 kHz AAC编码通道，不能复用网页/RTSP当前音频格式。
     */
    int send_record_audio_config();

    /**
     * @brief   : 通知录制进程音频编码链路将重启
     * @return   {int} 0：成功，非0：失败
     * @note    : 录制端在下一个视频I帧切片，隔离重启前后的音频时间线。
     */
    int notify_record_audio_restart();

    /**
     * @brief   : 设置录制进程接入后的状态同步回调
     * @param    {std::function<void()>} fnCallback：录制进程连接成功后执行的同步回调
     * @return   {void}
     * @note    : 回调在通讯线程中执行，调用方不得执行耗时或阻塞操作
     */
    void set_record_connected_callback(std::function<void()> fnCallback);

private:
    /**
     * @brief   : 将编码媒体帧复制到有界队列
     * @param   {void*} pData：编码数据指针
     * @param   {int} nLen：编码数据长度
     * @param   {int} nActionCode：录制协议命令码
     * @return  : 0 表示入队成功，非0表示未入队
     * @note    : 入队前完成一次拷贝，确保 VENC/AENC 释放原始缓冲区后工作线程仍拥有有效数据。
     */
    int enqueue_media_data(const void *pData, int nLen, int nActionCode);

    /**
     * @brief   : 将共享媒体帧引用入队（零拷贝）
     * @param   {shared_ptr<uint8_t[]>} &pSharedData：共享数据 buffer
     * @param   {int} nLen：编码数据长度
     * @param   {int} nActionCode：录制协议命令码
     * @return  : 0 表示入队成功，非0表示未入队
     * @note    : 入队不复制数据，仅增加 shared_ptr 引用计数。
     */
    int enqueue_media_data(const std::shared_ptr<std::uint8_t[]> &pSharedData, int nLen, int nActionCode);

    /**
     * @brief   : 录制媒体入队公共实现（拷贝路径与共享路径二选一）
     * @param   {void*} pData：拷贝路径数据指针，共享路径传 nullptr
     * @param   {int} nLen：拷贝路径数据长度，共享路径传 0
     * @param   {int} nActionCode：录制协议命令码
     * @param   {shared_ptr<uint8_t[]>} &pSharedData：共享路径数据 buffer，拷贝路径传 nullptr
     * @param   {int} nSharedLen：共享路径数据长度，拷贝路径传 0
     * @return  : 0 表示入队成功，非0表示未入队
     */
    int enqueue_media_data_impl(const void *pData,
                                int nLen,
                                int nActionCode,
                                const std::shared_ptr<std::uint8_t[]> &pSharedData,
                                int nSharedLen);

    /**
     * @brief   : 录制媒体发送线程主循环
     */
    void media_send_loop();

    /**
     * @brief   : 清理尚未发送的录制媒体帧
     */
    void clear_media_queue();

    /**
     * @brief   : 录制媒体队列元素
     * @note    : vecData 或 pSharedData 持有数据所有权（二选一）：
     *            - vecData：独立拷贝路径（旧接口，Audio 等）
     *            - pSharedData：共享副本路径（新接口，与 RTSP/RTMP 共享）
     *            禁止将外部 VENC/AENC 指针直接跨线程传递。
     */
    struct RecordMediaTask_S
    {
        int nActionCode = 0;
        std::vector<std::uint8_t> vecData;
        std::shared_ptr<std::uint8_t[]> pSharedData; /* 共享帧数据 */
        int nSharedLen = 0;                           /* 共享帧长度 */
    };

    /*网络基础智能指针句柄*/
    std::shared_ptr<Net::IOBase> m_pHandler = nullptr;
    /*心跳字符串*/
    std::string m_heartbeat;
    /*客户端是否连接*/
    std::atomic_bool m_bConnect = false;
    /* lock: 保护录制进程接入回调的注册和读取 */
    std::mutex m_mtxRecordConnectedCallback;
    /* 录制进程重连后补发当前媒体配置的回调 */
    std::function<void()> m_fnRecordConnectedCallback;
    /* lock: 串行化控制消息与媒体消息，避免同一 UDS 会话的头/体交叉 */
    std::mutex m_mtxIoSend;
    /* 录制进程已完成连接后的配置同步，可以接收媒体帧 */
    std::atomic_bool m_bRecordReady = false;
    /* 录制媒体发送线程运行标志 */
    std::atomic_bool m_bMediaWorkerRunning = false;
    /* lock: 保护录制媒体队列和字节计数 */
    std::mutex m_mtxMediaQueue;
    std::condition_variable m_cvMediaQueue;
    std::deque<RecordMediaTask_S> m_mediaQueue;
    std::thread m_mediaSendThread;
    /* 录制媒体单帧最大字节上限，按平均码率动态计算，避免高码率大I帧被固定上限击穿。 */
    std::atomic<std::size_t> m_nMediaMaxFrameBytes{512U * 1024U};
    /* 录制媒体队列总字节上限，按单帧上限放大，容纳多帧积压。 */
    std::atomic<std::size_t> m_nMediaQueueMaxBytes{512U * 1024U};
    /* 队列当前占用的媒体字节数，避免仅按帧数估算内存 */
    std::size_t m_nMediaQueueBytes = 0;
    /* 正在同步写入UDS的媒体副本，队列字节数不包含这部分内存。 */
    std::size_t m_nMediaInFlightBytes = 0;
    /* 统计周期内队列的峰值，用于识别积压而不是只看瞬时值。 */
    std::size_t m_nMediaHighWaterFrames = 0;
    std::size_t m_nMediaHighWaterBytes = 0;
    /* 丢帧统计与限频日志时间戳，仅在队列锁内更新 */
    std::uint64_t m_ullDroppedMediaFrames = 0;
    std::uint64_t m_ullDroppedMediaBytes = 0;
    long long m_llLastMediaDropLogMs = 0;
    /* 录制UDS慢写日志时间戳，仅在队列锁内更新 */
    long long m_llLastMediaSendWarnMs = 0;
};
