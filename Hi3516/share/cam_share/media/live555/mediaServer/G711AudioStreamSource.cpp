#include "G711AudioStreamSource.h"

G711AudioStreamSource* G711AudioStreamSource::createNew(UsageEnvironment& env, Audio_Source_Info_t& stuG711SourecInfo)
{
    do
    {
        G711AudioStreamSource* newSource = new G711AudioStreamSource(env, stuG711SourecInfo);
        if (newSource != NULL && newSource->bitsPerSample() == 0)
        {
            // The WAV file header was apparently invalid.
            Medium::close(newSource);
            break;
        }
        return newSource;
    }
    while (0);

    return NULL;
}

G711AudioStreamSource::G711AudioStreamSource(UsageEnvironment& env, Audio_Source_Info_t& stuG711SourecInfo)
    : FramedSource(env), fNumChannels(0), fSamplingFrequency(0), fBitsPerSample(0), fLimitNumBytesToStream(False),
      fNumBytesToStream(0), fLastPlayTime(0), fPlayTimePerSample(0)
{
    fNumChannels = 1;
    fSamplingFrequency = 8000;
    fBitsPerSample = 8;

    fPlayTimePerSample = 1e6 / (double) fSamplingFrequency;
    // Although PCM is a sample-based format, we group samples into
    // 'frames' for efficient delivery to clients.  Set up our preferred
    // frame size to be close to 20 ms, if possible, but always no greater
    // than 1400 bytes (to ensure that it will fit in a single RTP packet)
    unsigned maxSamplesPerFrame = (1400 * 8) / (fNumChannels * fBitsPerSample);
    unsigned desiredSamplesPerFrame = (unsigned) (0.04 * fSamplingFrequency);
    unsigned samplesPerFrame = desiredSamplesPerFrame < maxSamplesPerFrame ? desiredSamplesPerFrame
                                                                           : maxSamplesPerFrame;
    fPreferredFrameSize = (samplesPerFrame * fNumChannels * fBitsPerSample) / 8;

    memset(&m_g711SouceInfo, 0, sizeof(Audio_Source_Info_t));
    memcpy(&m_g711SouceInfo, &stuG711SourecInfo, sizeof(Audio_Source_Info_t));
    gettimeofday(&m_g711prvTime, NULL);
    if (m_g711SouceInfo.clientFun)
    {
        m_status.param = m_g711SouceInfo.audioindex;
        m_status.status = RTSPCLIENT_START;
        m_g711SouceInfo.clientFun(&m_status);
    }
    m_toDelay = 10000;
}

G711AudioStreamSource::~G711AudioStreamSource()
{
    if (m_pToken)
    {
        envir().taskScheduler().unscheduleDelayedTask(m_pToken);
    }
    if (m_g711SouceInfo.clientFun)
    {
        m_status.param = m_g711SouceInfo.audioindex;
        m_status.status = RTSPCLIENT_STOP;
        m_g711SouceInfo.clientFun(&m_status);
    }
}
unsigned int G711AudioStreamSource::maxFrameSize() const
{
    return G711_MAX_FRAME_SIZE;
}

void G711AudioStreamSource::doGetNextFrame()
{
    m_pToken = envir().taskScheduler().scheduleDelayedTask(m_toDelay, getNextFrame, this);
}

void G711AudioStreamSource::getNextFrame(void* ptr)
{
    G711AudioStreamSource* pAudiosource = (G711AudioStreamSource*) ptr;
    if (pAudiosource == NULL)
    {
        printf("AAC getNextFrame is NULL\n");
        return;
    }
    pAudiosource->getNextFrame1();
}
void G711AudioStreamSource::getNextFrame1()
{
    if (fLimitNumBytesToStream && fNumBytesToStream < fMaxSize)
    {
        fMaxSize = fNumBytesToStream;
    }
    if (fPreferredFrameSize < fMaxSize)
    {
        fMaxSize = fPreferredFrameSize;
    }
    unsigned bytesPerSample = (fNumChannels * fBitsPerSample) / 8;
    if (bytesPerSample == 0)
    {
        bytesPerSample = 1; // because we can't read less than a byte at a time
        // unsigned bytesToRead = fMaxSize - fMaxSize%bytesPerSample;
    }

    if (m_g711SouceInfo.dataGetfun)
    {
        m_frame.frameSize = 0;
        m_frame.data = fTo;
        m_frame.param = m_g711SouceInfo.audioindex;
        m_frame.type = AUDIO_TYPE;
        m_g711SouceInfo.dataGetfun(&m_frame);
        if (m_frame.frameSize > 4)
        {
            fFrameSize = m_frame.frameSize;
            // Set the 'presentation time' and 'duration' of this frame:
            if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0)
            {
                // This is the first frame, so use the current time:
                gettimeofday(&fPresentationTime, NULL);
            }
            else
            {
                // Increment by the play time of the previous data:
                unsigned uSeconds = fPresentationTime.tv_usec + fLastPlayTime;
                fPresentationTime.tv_sec += uSeconds / 1000000;
                fPresentationTime.tv_usec = uSeconds % 1000000;
            }

            // Remember the play time of this data:
            fDurationInMicroseconds = fLastPlayTime = (unsigned) ((fPlayTimePerSample * fFrameSize) / bytesPerSample);
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

    /* 积压数据超过2帧时加速，快速发送 */
    if (m_frame.audiolistsize >= 2)
    {
        this->fDurationInMicroseconds = 0; // 告诉 Sink 立即处理下一帧，不要等待
    }

    afterGetting(this);
}