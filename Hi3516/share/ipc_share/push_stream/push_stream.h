/**
 * @FilePath     : push_stream.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-27 17:42:05
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-03 19:35:15
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

extern "C"
{
#include <unistd.h>
}

class CPushStream
{
private:
    CPushStream();
    static CPushStream* m_self;
    static std::mutex m_mutex;
public:
    static CPushStream* instance()
    {
        if (m_self == nullptr) // 第一层检查
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_self == nullptr) // 第二层检查
            {
                m_self = new CPushStream();
            }
        }
        return m_self;
    }

    ~CPushStream();

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
     * @brief       : 外部送音频数据
     * @author      : zhouzirui
     * @param        {AudioFrame_S} *pAudioFrame：音频帧数据指针
     * @param        {bool} bIsMain：是否为主码流
     * @param        {bool} bIsRtsp：是否为RTSP流 
     * @return       {*}非0：失败
     */
    int sendAudioData(Audio_NS::AudioFrame_S *pAudioFrame, bool bIsMain, bool bIsRtsp);

    /**
     * @brief   : 外部送音频数据
     * @param    {AudioFrame_S} *pAudioFrame：音频帧数据指针
     * @param    {bool} bIsRtsp：是否为RTSP流 
     * @return   {*}零：成功 小于零：失败
     */
    int sendAudioData(Audio_NS::AudioFrame_S *pAudioFrame, bool bIsRtsp = true);

private:
    //info /*----------------------- 模块句柄 -----------------------*/

    //info /*----------------------- 参数变量 -----------------------*/
    /* 是否初始化 */
    bool m_bInitFlag = false;
    /* https配置文件 */
    std:: string m_strHttpsConfigFile;
};
