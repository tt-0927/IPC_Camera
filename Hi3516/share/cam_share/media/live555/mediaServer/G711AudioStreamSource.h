/**
 * @FilePath     : G711AudioStreamSource.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-12-29 13:45:19
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-16 13:41:49
 * @Description  : G711 音频流源
 */

#ifndef _G711_AUDIO_STREAM_SOURCE_HH
#define _G711_AUDIO_STREAM_SOURCE_HH

#ifndef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif
#include "custom_define.h"

// 定义 G.711 音频参数常量
#define G711_SAMPLING_FREQUENCY (8000)   /* 采样率 8000Hz */
#define G711_NUM_CHANNELS       (1)      /* 单声道 */
#define G711_BITS_PER_SAMPLE    (8)      /* 位深 8bit */
#define G711_MAX_FRAME_SIZE     (4096) /* 最大帧缓冲区大小 */

class G711AudioStreamSource : public FramedSource
{
public:
    static G711AudioStreamSource* createNew(UsageEnvironment& env, Audio_Source_Info_t& stuG711SourecInfo);
    unsigned char bitsPerSample() const
    {
        return fBitsPerSample;
    }
    unsigned char numChannels() const
    {
        return fNumChannels;
    }
    unsigned samplingFrequency() const
    {
        return fSamplingFrequency;
    }

    static void getNextFrame(void* ptr);
    void getNextFrame1();
    // returns the 'AudioSpecificConfig' for this stream (in ASCII form)
    virtual unsigned int maxFrameSize() const;

protected:
    G711AudioStreamSource(UsageEnvironment& envm, Audio_Source_Info_t& stuG711SourecInfo);
    // called only by createNew()

    virtual ~G711AudioStreamSource();

private:
    // redefined virtual functions:
    virtual void doGetNextFrame();

private:
    unsigned char fNumChannels;
    unsigned fSamplingFrequency;
    unsigned char fBitsPerSample;
    unsigned fPreferredFrameSize;
    Boolean fLimitNumBytesToStream;
    unsigned fNumBytesToStream;
    unsigned fLastPlayTime;
    double fPlayTimePerSample; // useconds

    int m_toDelay;
    Audio_Source_Info_t m_g711SouceInfo;
    Rtsp_ClientStream_State_t m_status;
    Fream_Info_t m_frame;
    void* m_pToken;

    struct timeval m_g711CurTime;
    struct timeval m_g711prvTime;
};

#endif