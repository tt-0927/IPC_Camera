/*
 * @FilePath: h265_server_subsession.h
 * @Author: yangwenyao
 * @Date: 2023-01-03 13:46:45
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2023-01-03 16:06:25
 * @Descripttion: 265server session
 */
#ifndef __H265_SERVER_SUBSESSION_H__
#define __H265_SERVER_SUBSESSION_H__

#pragma once

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "OnDemandServerMediaSubsession.hh"
#include "h265_video_source.h"

class H265_Server_Subsession : public OnDemandServerMediaSubsession
{
public:
	H265_Server_Subsession(UsageEnvironment & env, Video_Source_Info_t& sourceInfo);
	~H265_Server_Subsession(void);
	static H265_Server_Subsession*	createNew(UsageEnvironment & env, Video_Source_Info_t& sourceInfo);
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
  H265_Video_Source* m_video_source;
};

#endif //__H265_SERVER_SUBSESSION_H__