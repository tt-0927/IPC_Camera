
#include "aac_server_subsession.h"
#include "ADTSAudioFileSource.hh"
#include "MPEG4GenericRTPSink.hh"

aacAudioServerMediaSubsession*
aacAudioServerMediaSubsession::createNew(UsageEnvironment& env,
					     Boolean reuseFirstSource, Audio_Source_Info_t& aac_sourec_info)
{
  return new aacAudioServerMediaSubsession(env, reuseFirstSource, aac_sourec_info);
}

aacAudioServerMediaSubsession
::aacAudioServerMediaSubsession(UsageEnvironment& env,Boolean reuseFirstSource, Audio_Source_Info_t& aac_sourec_info)
: OnDemandServerMediaSubsession(env, True)
{
	memcpy(&m_aacSouceInfo, &aac_sourec_info, sizeof(Audio_Source_Info_t));
}

aacAudioServerMediaSubsession
::~aacAudioServerMediaSubsession() {
}

FramedSource* aacAudioServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  estBitrate = 128;
  envir() << "[RTSP] AAC bitrate: " << estBitrate << "\n"; // kbps
  return aacAudioSource::createNew(envir(), m_aacSouceInfo);
}

RTPSink* aacAudioServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* inputSource) {
	aacAudioSource* adtsSource = (aacAudioSource*)inputSource;
  return MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock,
					rtpPayloadTypeIfDynamic,
					adtsSource->samplingFrequency(),
					"audio", "AAC-hbr", adtsSource->configStr(),
					adtsSource->numChannels(),
					m_aacSouceInfo.outPacketBufferSize);
}
