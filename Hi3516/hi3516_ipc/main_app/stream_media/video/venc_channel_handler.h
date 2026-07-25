/**
 * @FilePath     : venc_channel_handler.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-01-08 09:49:07
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-08 10:22:05
 * @Description  : VENC通道处理策略接口及实现类
 */

#pragma once

#include <memory>
#include "video_define.h"
#include "stream_video_config.h"

/* 前向声明 */
class CStreamVideo;

/**
 * @brief   : VENC通道处理策略接口
 * @note    : 使用策略模式分离不同通道的处理逻辑
 */
class CIVencChannelHandler
{
public:
    virtual ~CIVencChannelHandler() = default;

    /**
     * @brief   : 处理视频帧数据
     * @param   {uint8_t*} pData：帧数据指针
     * @param   {int} nDataLen：帧数据长度
     * @param   {Video_NS::VideoFrame_S*} pVideoFrame：视频帧结构体指针
     * @param   {CStreamVideoConfig&} configManager：配置管理器引用
     * @param   {int} nChannel：通道号
     */
    virtual void handleFrame(const uint8_t* pData,
                             int nDataLen,
                             Video_NS::VideoFrame_S* pVideoFrame,
                             CStreamVideoConfig& configManager,
                             int nChannel) = 0;
};

/**
 * @brief   : 主码流通道处理器
 * @note    : 处理主码流数据，发送到RTSP推流和GB28181
 */
class CMainChannelHandler : public CIVencChannelHandler
{
public:
    /**
     * @brief   : 构造函数
     * @param   {CStreamVideo*} pStreamVideo：流媒体视频对象指针，用于请求IDR帧
     */
    explicit CMainChannelHandler(CStreamVideo* pStreamVideo);

    /**
     * @brief   : 处理主码流帧数据
     * @param   {uint8_t*} pData：帧数据指针
     * @param   {int} nDataLen：帧数据长度
     * @param   {Video_NS::VideoFrame_S*} pVideoFrame：视频帧结构体指针
     * @param   {CStreamVideoConfig&} configManager：配置管理器引用
     * @param   {int} nChannel：通道号
     */
    void handleFrame(const uint8_t* pData,
                     int nDataLen,
                     Video_NS::VideoFrame_S* pVideoFrame,
                     CStreamVideoConfig& configManager,
                     int nChannel) override;

private:
    /* 流媒体视频对象指针 */
    CStreamVideo* m_pStreamVideo;

    /* 上次请求IDR帧的时间戳 */
    long long int m_llLastIdrTimestamp;

    /**
     * @brief   : 检查并请求IDR帧
     * @param   {CStreamVideoConfig&} configManager：配置管理器引用
     * @param   {int} nChannel：通道号
     */
    void checkAndRequestIdr(CStreamVideoConfig& configManager, int nChannel);
};

/**
 * @brief   : 子码流通道处理器
 * @note    : 处理子码流数据，发送到RTSP推流和录制模块
 */
class CSubChannelHandler : public CIVencChannelHandler
{
public:
    /**
     * @brief   : 构造函数
     * @param   {CStreamVideo*} pStreamVideo：流媒体视频对象指针，用于请求IDR帧
     */
    explicit CSubChannelHandler(CStreamVideo* pStreamVideo);

    /**
     * @brief   : 处理子码流帧数据
     * @param   {uint8_t*} pData：帧数据指针
     * @param   {int} nDataLen：帧数据长度
     * @param   {Video_NS::VideoFrame_S*} pVideoFrame：视频帧结构体指针
     * @param   {CStreamVideoConfig&} configManager：配置管理器引用
     * @param   {int} nChannel：通道号
     */
    void handleFrame(const uint8_t* pData,
                     int nDataLen,
                     Video_NS::VideoFrame_S* pVideoFrame,
                     CStreamVideoConfig& configManager,
                     int nChannel) override;

private:
    /* 流媒体视频对象指针 */
    CStreamVideo* m_pStreamVideo;

    /* 上次请求IDR帧的时间戳 */
    long long int m_llLastIdrTimestamp;

    /**
     * @brief   : 检查并请求IDR帧
     * @param   {CStreamVideoConfig&} configManager：配置管理器引用
     * @param   {int} nChannel：通道号
     */
    void checkAndRequestIdr(CStreamVideoConfig& configManager, int nChannel);
};

/**
 * @brief   : JPEG通道处理器
 * @note    : 处理JPEG抓图数据，发送到抓图模块
 */
class CJpegChannelHandler : public CIVencChannelHandler
{
public:
    /**
     * @brief   : 构造函数
     */
    CJpegChannelHandler();

    /**
     * @brief   : 处理JPEG帧数据
     * @param   {uint8_t*} pData：帧数据指针
     * @param   {int} nDataLen：帧数据长度
     * @param   {Video_NS::VideoFrame_S*} pVideoFrame：视频帧结构体指针（JPEG不使用）
     * @param   {CStreamVideoConfig&} configManager：配置管理器引用（JPEG不使用）
     * @param   {int} nChannel：通道号
     */
    void handleFrame(const uint8_t* pData,
                     int nDataLen,
                     Video_NS::VideoFrame_S* pVideoFrame,
                     CStreamVideoConfig& configManager,
                     int nChannel) override;

private:
    /**
     * @brief   : 发送JPEG帧数据到抓图模块
     * @param   {uint8_t*} pData：帧数据指针
     * @param   {int} nDataLen：帧数据长度
     * @return  {int} 0：成功，非0：失败
     */
    int sendFrameData(const uint8_t* pData, int nDataLen);

    /* 帧计数器 */
    int m_nFrameCount;

    /* 前一帧数据缓存 */
    std::vector<uint8_t> m_prevFrame;
};
