/**
 * @FilePath     : h265_video_source.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-01-16 17:30:24
 *
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-29 16:05:56
 * @Description  : h265 视频流源
 */

#include "h265_video_source.h"

H265_Video_Source::H265_Video_Source(UsageEnvironment& env, Video_Source_Info_t& souceInfo)
    : FramedSource(env), m_bIsFirstFrame(true), m_fFps(DEFAULT_VIDEO_FPS), m_fUsecPerFrame(40000.0f)
{
    memcpy(&m_stSourceInfo, &souceInfo, sizeof(Video_Source_Info_t));

    m_toDelay = 10000;
    fFrameSize = 0;
    m_stStatus.param = NULL;

    m_fUsecPerFrame =  1000000.0f / m_fFps;
}

H265_Video_Source::~H265_Video_Source(void)
{
    envir().taskScheduler().unscheduleDelayedTask(m_pToken);
    if (m_stSourceInfo.clientFun)
    {
        m_stStatus.param = m_stSourceInfo.videoindex;
        m_stStatus.status = RTSPCLIENT_STOP;
        m_stSourceInfo.clientFun(&m_stStatus);
    }
}

int H265_Video_Source::set_videostate_callback()
{
    if (m_stStatus.param == NULL)
    {
        if (m_stSourceInfo.clientFun)
        {
            m_stStatus.param = m_stSourceInfo.videoindex;
            m_stStatus.status = RTSPCLIENT_START;
            m_stSourceInfo.clientFun(&m_stStatus);
        }
    }
    return 0;
}

int H265_Video_Source::server_inputData(unsigned char* pData, int nDataLen)
{
    return 1;
}

void H265_Video_Source::doGetNextFrame()
{

    m_pToken = envir().taskScheduler().scheduleDelayedTask(m_toDelay, getNextFrame, this);
}

unsigned int H265_Video_Source::maxFrameSize() const
{
    return m_stSourceInfo.outPacketBufferSize == 0
               ? REV_BUF_SIZE
               : m_stSourceInfo.outPacketBufferSize;
}

void H265_Video_Source::getNextFrame(void* ptr)
{
    H265_Video_Source* pVideosource = (H265_Video_Source*) ptr;
    if (pVideosource == NULL)
    {
        printf("getNextFrame is NULL\n");
        return;
    }
    pVideosource->getNextFrame1();
}

void H265_Video_Source::getNextFrame1()
{
    if (m_stSourceInfo.dataGetfun)
    {
        set_videostate_callback();
        m_stFrame.frameSize = 0;
        m_stFrame.data = fTo;
        m_stFrame.type = VIDEO_TYPE;
        m_stFrame.param = m_stSourceInfo.videoindex;
        m_stSourceInfo.dataGetfun(&m_stFrame);
        if (m_stFrame.frameSize > 4)
        {
            fFrameSize = m_stFrame.frameSize;

            // live_log("================");
            // live_log("================");
            // live_log("m_stFrame.fFps:%f", m_stFrame.fFps);
            // live_log("================");
            // live_log("================");

            if (m_stFrame.fFps > 0.0f && m_stFrame.fFps != m_fFps)
            {
                m_fFps = m_stFrame.fFps;
                m_fUsecPerFrame = 1000000.0f / m_fFps;
            }
        }
        else
        {
            m_toDelay = 10000;
            fFrameSize = 0;
            doGetNextFrame();
            return;
        }
    }

    m_toDelay = 0;

    fDurationInMicroseconds = (unsigned) m_fUsecPerFrame;

    if (m_stFrame.videolistsize >= 2)
    {
        fDurationInMicroseconds = 0;
    }

    /**
     * 分数帧率修复：
     * 对于低帧率（< 1 fps），不在数据源层设置大的 duration。
     * 原因：H264VideoStreamFramer 会将一个完整帧拆分成多个 NAL 单元（SPS/PPS/SEI/IDR），
     * 每个 NAL 都会继承这个 duration，导致 MultiFramedRTPSink 的 fNextSendTime 被错误累加。
     * 例如：4 个 NAL × 2秒 = 8秒延迟，导致 RTP 发送被严重推迟。
     *
     * 解决方案：低帧率时设置 duration=0，让 H264or5VideoStreamFramer 的 parse() 函数
     * 在 Access Unit 结束时正确计算下一帧的 PresentationTime。
     */
    if (m_fFps < 1.0f && m_fFps > 0.0f)
    {
        fDurationInMicroseconds = 0;
    }

    FramedSource::afterGetting(this);
}
