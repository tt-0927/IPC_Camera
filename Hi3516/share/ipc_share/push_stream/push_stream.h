/**
 * @FilePath     : push_stream.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-27 17:42:05
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-13 10:00:12
 * @Description  : 推流模块
 */
#pragma once

#include <iostream>
#include <mutex>
#include <thread>
#include <atomic>
#include "dlog.h"
#include "IpcRet.h"
#include "rtsp_server.h"
#include "network_define.h"
#include "rtmp_pusher.h"

extern "C"
{
#include <unistd.h>
}

/* 推流协议枚举 */
enum PushStreamProtocol_E
{
    PROTOCOL_RTSP = 0, /* RTSP协议 */
    PROTOCOL_RTMP,     /* RTMP协议 */
};

class CPushStream : public CSingleton<CPushStream>
{
private:
    CPushStream();

public:
    ~CPushStream() = default;
    friend class CSingleton<CPushStream>;

    /**
     * @brief       : 初始化推流模块
     * @author      : zhouzirui
     * @return       {*} RETURN_ERROR：失败
     */
    IpcRet_E init();

    /**
     * @brief       : 去初始化推流模块
     * @author      : zhouzirui
     * @return       {*} RETURN_ERROR：失败
     */
    IpcRet_E deinit();

    /**
     * @brief       : 外部送视频数据
     * @author      : zhouzirui
     * @param        {VideoFrame_S} *pVideoFrame：视频帧数据指针
     * @param        {bool} bIsMain：是否为主码流
     * @param        {bool} bIsRtsp：是否为RTSP流
     * @return       {*}非0：失败
     */
    int sendVideoData(Video_NS::VideoFrame_S *pVideoFrame, bool bIsMain, bool bIsRtsp);

    /**
     * @brief       : 外部送视频数据（按协议）
     * @author      : zhouzirui
     * @param        {VideoFrame_S} *pVideoFrame：视频帧数据指针
     * @param        {bool} bIsMain：是否为主码流
     * @param        {PushStreamProtocol_E} enProtocol：推流协议
     * @return       {*}非0：失败
     */
    int sendVideoData(Video_NS::VideoFrame_S *pVideoFrame, bool bIsMain, PushStreamProtocol_E enProtocol);

    /**
     * @brief       : 外部送音频数据
     * @author      : zhouzirui
     * @param        {AudioFrame_S} *pAudioFrame：音频帧数据指针
     * @param        {bool} bIsMain：是否为主码流
     * @param        {bool} bIsRtsp：是否为RTSP流 
     * @return       {*}非0：失败
     */
    int sendAudioData(Audio_NS::AudioFrame_S *pAudioFrame, bool bIsMain, bool bIsRtsp);

    /**
     * @brief       : 外部送音频数据（按协议）
     * @author      : zhouzirui
     * @param        {AudioFrame_S} *pAudioFrame：音频帧数据指针
     * @param        {bool} bIsMain：是否为主码流
     * @param        {PushStreamProtocol_E} enProtocol：推流协议
     * @return       {*}非0：失败
     */
    int sendAudioData(Audio_NS::AudioFrame_S *pAudioFrame, bool bIsMain, PushStreamProtocol_E enProtocol);

    /**
     * @brief   : 外部送音频数据
     * @param    {AudioFrame_S} *pAudioFrame：音频帧数据指针
     * @param    {bool} bIsRtsp：是否为RTSP流 
     * @return   {*}零：成功 小于零：失败
     */
    int sendAudioData(Audio_NS::AudioFrame_S *pAudioFrame, bool bIsRtsp = true);

    /**
     * @brief   : 外部送音频数据（按协议）
     * @param    {AudioFrame_S} *pAudioFrame：音频帧数据指针
     * @param    {PushStreamProtocol_E} enProtocol：推流协议
     * @return   {*}零：成功 小于零：失败
     */
    int sendAudioData(Audio_NS::AudioFrame_S *pAudioFrame, PushStreamProtocol_E enProtocol);

#if CAP_GARBAGE_STATION_PLATFORM
    /**
     * @brief   : 重新启动 RTMP 推流（平台信息变更后热更新）
     * @param    {Platform_Info_t} stPlatformInfo：新的平台配置信息
     * @return   {int} 0：成功，非0：失败
     * @note    : 先停止当前 RTMP 推流，用新的平台地址重新生成 URL 并启动
     */
    int restart_rtmp_stream(const Network::Platform_Info_t &stPlatformInfo);

    /**
     * @brief   : 按通道重启 RTMP 推流（视频配置变更后调用）
     * @param    {int} nChannel：视频通道号（0=主码流，1=子码流）
     * @return   {int} 0：成功，非0：失败
     * @note    : 获取最新视频配置，停止并重新启动指定通道的 RTMP 推流
     */
    int restart_rtmp_by_channel(int nChannel);
#endif

private:
    /* 是否初始化 */
    bool m_bInitFlag = false;
    /* https配置文件 */
    std:: string m_strHttpsConfigFile;
};
