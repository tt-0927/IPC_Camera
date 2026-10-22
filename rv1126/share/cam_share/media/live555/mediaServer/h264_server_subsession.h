#pragma once

#include <vector>
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "OnDemandServerMediaSubsession.hh"
#include "h264_video_source.h"
class H264_Server_Subsession : public OnDemandServerMediaSubsession
{
public:
	H264_Server_Subsession(UsageEnvironment & env, Video_Source_Info_t& sourceInfo);
	~H264_Server_Subsession(void);
	static H264_Server_Subsession*	createNew(UsageEnvironment & env, Video_Source_Info_t& sourceInfo);
protected:
	  virtual char const * getAuxSDPLine(RTPSink * rtpSink, FramedSource * inputSource);
	  virtual FramedSource * createNewStreamSource(unsigned clientSessionId, unsigned & estBitrate); // "estBitrate" is the stream's estimated bitrate, in kbps
	  virtual RTPSink * createNewRTPSink(Groupsock * rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource * inputSource);

    // info ------------------ zhouzr ------------------
    /**
     * @brief   : 重写父类方法，在客户端开始播放时调用
     * @note    : 这是触发 "客户端连接" 回调的正确位置
     */
    virtual void startStream(unsigned clientSessionId,
                              void *streamToken,
                              TaskFunc *rtcpRRHandler,
                              void *rtcpRRHandlerClientData,
                              unsigned short &rtpSeqNum,
                              unsigned &rtpTimestamp,
                              ServerRequestAlternativeByteHandler *serverRequestAlternativeByteHandler,
                              void *serverRequestAlternativeByteHandlerClientData) override;

    /**
     * @brief   : 重写父类方法，在客户端流被删除时调用
     * @note    : 这是触发 "客户端断开" 回调的正确位置
     */
    virtual void deleteStream(unsigned clientSessionId, void *&streamToken) override;
    // info ------------------ zhouzr ------------------

public:
	  void setDoneFlag() { fDoneFlag = ~0; };
	  void afterPlayingDummy1();
	  void checkForAuxSDPLine1();
private:
  Video_Source_Info_t m_pSouceInfo;
  char* fAuxSDPLine;
  char fDoneFlag; // used when setting up "fAuxSDPLine"
  RTPSink* fDummyRTPSink; //
  H264_Video_Source* m_video_source;
	std::vector<uint8_t> fSPS;
  std::vector<uint8_t> fPPS;
};
