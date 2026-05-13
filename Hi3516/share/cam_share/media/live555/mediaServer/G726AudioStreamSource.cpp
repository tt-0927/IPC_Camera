/**
 * @FilePath     : G726AudioStreamSource.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-09 17:16:23
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-30 19:10:55
 * @Description  : G.726音频数据源类实现
 * 实现G.726音频数据的异步读取、时间戳生成和帧调度功能。通过回调机制获取外部音频数据，并适配为Live555可处理的帧格式。
 * 
 * 关键实现：
 * 1. 构造函数：初始化G.726音频参数，计算帧大小和播放时间
 * 2. doGetNextFrame()：启动异步帧获取流程
 * 3. getNextFrame1()：执行实际的数据获取和时间戳设置
 * 4. 时间戳算法：基于采样率和位深度精确计算帧持续时间
 * 5. 帧大小处理：考虑G.726非字节对齐特性的位操作
 * 
 * 数据流程：
 * 外部音频源 -> dataGetfun回调 -> 帧数据复制 -> 时间戳计算 -> Live555处理
 */

#include "G726AudioStreamSource.h"

G726AudioStreamSource *
G726AudioStreamSource::createNew(UsageEnvironment &env, Audio_Source_Info_t &stuG726SourecInfo)
{
    do
    {
        G726AudioStreamSource *newSource = new G726AudioStreamSource(env, stuG726SourecInfo);
        if (newSource != NULL && newSource->bitsPerSample() == 0)
        {
            // G.726音频源初始化失败
            Medium::close(newSource);
            break;
        }
        return newSource;
    } while (0);

    return NULL;
}

G726AudioStreamSource::G726AudioStreamSource(UsageEnvironment &env, Audio_Source_Info_t &stuG726SourecInfo)
    : FramedSource(env),
      fNumChannels(0), fSamplingFrequency(0),
      fBitsPerSample(0),
      fLimitNumBytesToStream(False),
      fNumBytesToStream(0),
      fLastPlayTime(0),
      fPlayTimePerSample(0)
{
    // G.726音频参数设置
    fNumChannels = 1;          // G.726通常是单声道
    fSamplingFrequency = 8000; // G.726采样率8kHz
    fBitsPerSample = 4;        // G.726-32的位深度为4位/样本，对应32kbps

    if (stuG726SourecInfo.bitWidth >= 2 && stuG726SourecInfo.bitWidth <= 5)
    {
        fBitsPerSample = (unsigned char)stuG726SourecInfo.bitWidth;
    }
    // 计算每个样本的播放时间（微秒）
    fPlayTimePerSample = 1e6 / (double)fSamplingFrequency;

    // 计算优选帧大小
    // G.726帧大小计算需要考虑位深度
    unsigned maxSamplesPerFrame = (1400 * 8) / (fNumChannels * fBitsPerSample);
    unsigned desiredSamplesPerFrame = (unsigned)(0.02 * fSamplingFrequency); // 20ms帧
    unsigned samplesPerFrame = desiredSamplesPerFrame < maxSamplesPerFrame ? desiredSamplesPerFrame : maxSamplesPerFrame;

    // G.726每个样本占用的位数不是8的倍数，需要特殊处理
    fPreferredFrameSize = (samplesPerFrame * fNumChannels * fBitsPerSample + 7) / 8; // 向上取整到字节

    memset(&m_g726SouceInfo, 0, sizeof(Audio_Source_Info_t));
    memcpy(&m_g726SouceInfo, &stuG726SourecInfo, sizeof(Audio_Source_Info_t));
    gettimeofday(&m_g726prvTime, NULL);

    if (m_g726SouceInfo.clientFun)
    {
        m_status.param = m_g726SouceInfo.audioindex;
        m_status.status = RTSPCLIENT_START;
        m_g726SouceInfo.clientFun(&m_status);
    }
    m_toDelay = 10;
}

G726AudioStreamSource::~G726AudioStreamSource()
{
    if (m_pToken)
    {
        envir().taskScheduler().unscheduleDelayedTask(m_pToken);
        m_pToken = NULL;
    }
    if (m_g726SouceInfo.clientFun)
    {
        m_status.param = m_g726SouceInfo.audioindex;
        m_status.status = RTSPCLIENT_STOP;
        m_g726SouceInfo.clientFun(&m_status);
    }
}
unsigned int G726AudioStreamSource::maxFrameSize() const
{
    return MAX_FRAME_SIZE;
}

void G726AudioStreamSource::doGetNextFrame()
{
    if (m_pToken)
    {
        envir().taskScheduler().unscheduleDelayedTask(m_pToken);
        m_pToken = NULL;
    }
    m_pToken = envir().taskScheduler().scheduleDelayedTask(m_toDelay, getNextFrame, this);
}

void G726AudioStreamSource::getNextFrame(void *ptr)
{
    G726AudioStreamSource *pAudiosource = (G726AudioStreamSource *)ptr;
    if (pAudiosource == NULL)
    {
        printf("G726 getNextFrame is NULL\n");
        return;
    }
    pAudiosource->getNextFrame1();
}
void G726AudioStreamSource::getNextFrame1()
{
    if (fLimitNumBytesToStream && fNumBytesToStream < fMaxSize)
    {
        fMaxSize = fNumBytesToStream;
    }
    if (fPreferredFrameSize < fMaxSize)
    {
        fMaxSize = fPreferredFrameSize;
    }

    // G.726每个样本的字节数计算（需要特殊处理，因为不是8的倍数）
    unsigned bitsPerSample = fNumChannels * fBitsPerSample;
    unsigned bytesPerSample = (bitsPerSample + 7) / 8; // 向上取整
    if (bytesPerSample == 0)
    {
        bytesPerSample = 1;
    }

    if (m_g726SouceInfo.dataGetfun)
    {
        m_frame.frameSize = 0;
        m_frame.data = fTo;
        m_frame.param = m_g726SouceInfo.audioindex;
        m_frame.type = AUDIO_TYPE;
        m_g726SouceInfo.dataGetfun(&m_frame);
        if (m_frame.frameSize > 0)
        {
            fFrameSize = m_frame.frameSize;
            // 设置时间戳
            if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0)
            {
                // 第一帧使用当前时间
                gettimeofday(&fPresentationTime, NULL);
            }
            else
            {
                // 基于前一帧的时间戳递增
                unsigned uSeconds = fPresentationTime.tv_usec + fLastPlayTime;
                fPresentationTime.tv_sec += uSeconds / 1000000;
                fPresentationTime.tv_usec = uSeconds % 1000000;
            }

            // 计算帧持续时间（微秒）
            // G.726的每个字节包含 8/fBitsPerSample 个样本
            unsigned samplesInFrame = (fFrameSize * 8) / fBitsPerSample;
            fDurationInMicroseconds = fLastPlayTime = (unsigned)(samplesInFrame * fPlayTimePerSample);

            // 设置延迟
            m_toDelay = (m_frame.audiolistsize >= 1) ? 0 : 10;
        }
        else
        {
            // 没有数据，继续等待
            m_toDelay = 10;
            fFrameSize = 0;
            doGetNextFrame();
            return;
        }
    }

	m_toDelay = 0;

    /* 积压数据超过2帧时加速，快速发送 */
    if (m_frame.audiolistsize >= 2)
    {
        this->fDurationInMicroseconds = 0; // 告诉 Sink 立即处理下一帧，不要等待
    }

    afterGetting(this);
}