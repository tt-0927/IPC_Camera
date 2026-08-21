#include "G711uAudioStreamServerMediaSubsession.h"
#include "G711AudioStreamSource.h"
#include "SimpleRTPSink.hh"

G711uAudioStreamServerMediaSubsession* G711uAudioStreamServerMediaSubsession ::createNew(UsageEnvironment& env,
                                                                                         Boolean reuseFirstSource,
                                                                                         Audio_Source_Info_t& stuG711)
{
    return new G711uAudioStreamServerMediaSubsession(env, reuseFirstSource, stuG711);
}

G711uAudioStreamServerMediaSubsession ::G711uAudioStreamServerMediaSubsession(UsageEnvironment& env,
                                                                              Boolean reuseFirstSource,
                                                                              Audio_Source_Info_t& stuG711)
    : OnDemandServerMediaSubsession(env, reuseFirstSource)
{

    memcpy(&m_stuG711SourceInfo, &stuG711, sizeof(Audio_Source_Info_t));
}

G711uAudioStreamServerMediaSubsession ::~G711uAudioStreamServerMediaSubsession()
{
}

FramedSource* G711uAudioStreamServerMediaSubsession ::createNewStreamSource(unsigned /*clientSessionId*/,
                                                                            unsigned& estBitrate)
{
    FramedSource* resultSource = NULL;
    estBitrate = 64; // kbps, estimate
    do
    {
        G711AudioStreamSource* g711Source = G711AudioStreamSource::createNew(envir(), m_stuG711SourceInfo);
        if (g711Source == NULL)
            break;

        // Get attributes of the audio source:

        fBitsPerSample = g711Source->bitsPerSample();
        if (!(fBitsPerSample == 4 || fBitsPerSample == 8 || fBitsPerSample == 16))
        {
            envir() << "The input file contains " << fBitsPerSample << " bit-per-sample audio, which we don't handle\n";
            break;
        }
        fSamplingFrequency = g711Source->samplingFrequency();
        fNumChannels = g711Source->numChannels();
        unsigned bitsPerSecond = fSamplingFrequency * fBitsPerSample * fNumChannels;

        resultSource = g711Source;

        estBitrate = (bitsPerSecond + 500) / 1000; // kbps
        return resultSource;
    }
    while (0);

    // An error occurred:
    Medium::close(resultSource);
    return NULL;
}

RTPSink* G711uAudioStreamServerMediaSubsession ::createNewRTPSink(Groupsock* rtpGroupsock,
                                                                  unsigned char rtpPayloadTypeIfDynamic,
                                                                  FramedSource* inputSource)
{
    const char* mimeType = "PCMU";
    unsigned char payloadFormatCode;
    // G711AudioStreamSource* adtsSource = (G711AudioStreamSource*)inputSource;
    if (fSamplingFrequency == 8000 && fNumChannels == 1)
    {
        payloadFormatCode = 0; // a static RTP payload type
    }
    else
    {
        payloadFormatCode = rtpPayloadTypeIfDynamic;
    }
    return SimpleRTPSink::createNew(envir(),
                                    rtpGroupsock,
                                    payloadFormatCode,
                                    fSamplingFrequency,
                                    "audio",
                                    mimeType,
                                    fNumChannels,
                                    False, // 禁用多帧打包
                                    True,
                                    m_stuG711SourceInfo.outPacketBufferSize);
}
