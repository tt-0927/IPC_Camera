/*
 * @FilePath: mjpeg_server_subsession.cpp
 * @Author: yangwenyao
 * @Date: 2023-06-13 13:55:20
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2023-07-19 15:09:22
 * @Descripttion: mjpeg subsession server
 */
#include "mjpeg_server_subsession.h"
#include "JPEGVideoRTPSink.hh"
#include "JPEGVideoRTPSource.hh"
#include "mpeg_frame_source.h"
 
Mjpeg_Server_Subsession* Mjpeg_Server_Subsession::createNew(UsageEnvironment & env, Video_Source_Info_t& sourceInfo)
{
  return new Mjpeg_Server_Subsession(env, sourceInfo);
}

Mjpeg_Server_Subsession::Mjpeg_Server_Subsession(UsageEnvironment & env, Video_Source_Info_t& sourceInfo)
: OnDemandServerMediaSubsession(env, True)
{
	printf("Mjpeg_Server_Subsession is create\n");
	memcpy(&m_pSouceInfo, &sourceInfo, sizeof(Video_Source_Info_t));
	fAuxSDPLine = NULL;
	fDoneFlag = 0;
	fDummyRTPSink = NULL;
}
Mjpeg_Server_Subsession::~Mjpeg_Server_Subsession() {
	printf("Mjpeg_Server_Subsession is delete\n");

	delete[] fAuxSDPLine;
}

static void afterPlayingDummy(void* clientData) {
  Mjpeg_Server_Subsession* subsess = (Mjpeg_Server_Subsession*)clientData;
  subsess->afterPlayingDummy1();
}

void Mjpeg_Server_Subsession::afterPlayingDummy1() {
  // Unschedule any pending 'checking' task:
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
  // Signal the event loop that we're done:
  setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData) {
  Mjpeg_Server_Subsession* subsess = (Mjpeg_Server_Subsession*)clientData;
  subsess->checkForAuxSDPLine1();
}

void Mjpeg_Server_Subsession::checkForAuxSDPLine1() {
  char const* dasl;

  if (fAuxSDPLine != NULL) {
    // Signal the event loop that we're done:
    setDoneFlag();
  } else if (fDummyRTPSink != NULL && (dasl = fDummyRTPSink->auxSDPLine()) != NULL) {
    fAuxSDPLine = strDup(dasl);
    fDummyRTPSink = NULL;

    // Signal the event loop that we're done:
    setDoneFlag();
  } else if (!fDoneFlag) {
    // try again after a brief delay:
	#if 1
	  setDoneFlag();//cwsaddm for play quickly
	#else
	int uSecsToDelay = 100000;//100000; // 100 ms
    nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
			      (TaskFunc*)checkForAuxSDPLine, this);
	#endif
  }
}

char const* Mjpeg_Server_Subsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) 
{
  MJPEG_Video_Source *fDummySource = (MJPEG_Video_Source *)inputSource;

  if (fAuxSDPLine != NULL) return fAuxSDPLine; // it's already been set up (for a previous client)

  if (fDummyRTPSink == NULL) 
  { 
    fDummyRTPSink = rtpSink;

    // Start reading the file:
    fDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);
    // Check whether the sink's 'auxSDPLine()' is ready:
    checkForAuxSDPLine(this);
  }
  char const* addSDPFormat =
          "b=AS:%d\r\n"
          "a=framerate:%d\r\n"
          "a=framesize:%d %d-%d\r\n"
          ;

	unsigned addSDPFormatSize = strlen(addSDPFormat)
                            + 2 /* max char len */
                            + 4 /* max char len */
                            + 2 + 4 + 4
                            + 1;

	char* fmtp = new char[addSDPFormatSize];
	sprintf(fmtp,
		    addSDPFormat,
		    4096,
		    30,
		    rtpSink->rtpPayloadType(),
		    fDummySource->widthPixels(),
		    fDummySource->heightPixels()
	    );

	fAuxSDPLine = strDup(fmtp);
	delete[] fmtp;

  envir().taskScheduler().doEventLoop(&fDoneFlag);
	return fAuxSDPLine;

}

FramedSource* Mjpeg_Server_Subsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate)
{
	estBitrate = 2000000;

	MJPEG_FRAME_SOURCE* pVideoSource = MJPEG_FRAME_SOURCE::createNew(envir(), m_pSouceInfo);
  pVideoSource->m_nSynchronize = 1;
  
  return MJPEG_Video_Source::createNew(envir(), pVideoSource);
}

RTPSink* Mjpeg_Server_Subsession::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* /*inputSource*/) {
  return JPEGVideoRTPSink::createNew(envir(), rtpGroupsock);//MPEG4ESVideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
