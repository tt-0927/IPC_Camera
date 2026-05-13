/*
 * @FilePath: mjpeg_server_subsession.h
 * @Author: yangwenyao
 * @Date: 2023-06-03 13:46:45
 * @LastEditors: yangwenyao
 * @LastEditTime: 2023-06-21 14:38:39
 * @Descripttion: mjpeg_server_subsession 
 */
#ifndef __MJPEG_SERVER_SUBSESSION_H__
#define __MJPEG_SERVER_SUBSESSION_H__

#pragma once

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "OnDemandServerMediaSubsession.hh"
#include "mjpeg_video_source.h"

class Mjpeg_Server_Subsession : public OnDemandServerMediaSubsession
{
public:
	Mjpeg_Server_Subsession(UsageEnvironment & env, Video_Source_Info_t& sourceInfo);
	~Mjpeg_Server_Subsession(void);
	static Mjpeg_Server_Subsession*	createNew(UsageEnvironment & env, Video_Source_Info_t& sourceInfo);
protected:
	  virtual char const * getAuxSDPLine(RTPSink * rtpSink, FramedSource * inputSource);
	  virtual FramedSource * createNewStreamSource(unsigned clientSessionId, unsigned & estBitrate); // "estBitrate" is the stream's estimated bitrate, in kbps
	  virtual RTPSink * createNewRTPSink(Groupsock * rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource * inputSource);
public:
	  void setDoneFlag() { fDoneFlag = ~0; };
	  void afterPlayingDummy1();
	  void checkForAuxSDPLine1();
private:
  Video_Source_Info_t m_pSouceInfo;
  char* fAuxSDPLine;
  char fDoneFlag; // used when setting up "fAuxSDPLine"
  RTPSink* fDummyRTPSink; //
};

#endif //__MJPEG_SERVER_SUBSESSION_H__