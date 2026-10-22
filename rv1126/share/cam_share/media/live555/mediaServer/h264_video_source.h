/**
 * @FilePath     : h264_video_source.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-12-29 13:45:19
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-28 14:23:53
 * @Description  : h264 视频流源
 */

#pragma once

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "FramedSource.hh"
#include "rtspServer_base.h"
#include "custom_define.h"

class H264_Video_Source : public FramedSource
{

public:
    H264_Video_Source(UsageEnvironment& env, Video_Source_Info_t& pSouceInfo);
    ~H264_Video_Source(void);
    virtual void doGetNextFrame();
    virtual unsigned int maxFrameSize() const;

    static void getNextFrame(void* ptr);
    void getNextFrame1();
    int set_videostate_callback();

public:
    int server_inputData(unsigned char* pData, int nDataLen);

public:
    Video_Source_Info_t m_stSourceInfo;
    Rtsp_ClientStream_State_t m_stStatus;
    Fream_Info_t m_stFrame;
    void* m_pToken;
    int m_toDelay;
    bool m_bIsFirstFrame;
    float m_fFps;
    float m_fUsecPerFrame;
};
