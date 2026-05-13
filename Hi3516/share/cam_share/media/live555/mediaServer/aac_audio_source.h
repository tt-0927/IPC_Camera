#ifndef _AAC_AUDIO_SOURCE_HH
#define _AAC_AUDIO_SOURCE_HH

#include "FramedFileSource.hh"
#include "rtspServer_base.h"
#include "custom_define.h"

class aacAudioSource: public FramedSource {
public:
  static aacAudioSource* createNew(UsageEnvironment& env, Audio_Source_Info_t& aac_sourec_info);

  unsigned samplingFrequency() const { return fSamplingFrequency; }
  unsigned numChannels() const { return fNumChannels; }
  char const* configStr() const { return fConfigStr; }
  static void getNextFrame(void * ptr);
  void getNextFrame1();
      // returns the 'AudioSpecificConfig' for this stream (in ASCII form)
  virtual unsigned int maxFrameSize() const;
private:
  aacAudioSource(UsageEnvironment& env, u_int8_t profile,
		      u_int8_t samplingFrequencyIndex, u_int8_t channelConfiguration, Audio_Source_Info_t& pAAC_sourec_info);
	// called only by createNew()

  int PutBits(unsigned char *buf, unsigned int bits, unsigned int numBits, int pos);
  virtual ~aacAudioSource();
  int set_audiostate_callback();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();

private:
  unsigned fSamplingFrequency;
  unsigned fNumChannels;
  unsigned fuSecsPerFrame;
  char fConfigStr[5];

  Audio_Source_Info_t m_aacSouceInfo;
  Rtsp_ClientStream_State_t m_status;
  Fream_Info_t m_frame;
  void *m_pToken;

  struct timeval m_aacCurTime;
  struct timeval m_aacprvTime;
  int m_aacRate;
  int m_aacframeCount;
  int m_lost;
  int m_toDelay;
  int continue_count_lost;
  int count_aac;
};

#endif
