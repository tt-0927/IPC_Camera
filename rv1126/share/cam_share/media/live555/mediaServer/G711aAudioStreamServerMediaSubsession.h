#ifndef _G711A_AUDIO_STREAM_SERVER_MEDIA_SUBSESSION_HH
#define _G711A_AUDIO_STREAM_SERVER_MEDIA_SUBSESSION_HH

#ifndef _ON_DEMAND_SERVER_MEDIA_SUBSESSION_HH
#include "OnDemandServerMediaSubsession.hh"
#endif
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "G711AudioStreamSource.h"

class G711aAudioStreamServerMediaSubsession : public OnDemandServerMediaSubsession
{
public:
    static G711aAudioStreamServerMediaSubsession* createNew(UsageEnvironment& env,
                                                            Boolean reuseFirstSource,
                                                            Audio_Source_Info_t& stuG711);

protected:
    G711aAudioStreamServerMediaSubsession(UsageEnvironment& env,
                                          Boolean reuseFirstSource,
                                          Audio_Source_Info_t& stuG711);
    // called only by createNew();
    virtual ~G711aAudioStreamServerMediaSubsession();

protected: // redefined virtual functions
    virtual FramedSource* createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate);
    virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                      unsigned char rtpPayloadTypeIfDynamic,
                                      FramedSource* inputSource);

protected:
    // The following parameters of the input stream are set after
    // "createNewStreamSource" is called:
    unsigned char fBitsPerSample;
    unsigned fSamplingFrequency;
    unsigned fNumChannels;
    Audio_Source_Info_t m_stuG711SourceInfo;
};

#endif