#ifndef __RTSPCLIENT_SUBSESSION_H
#define __RTSPCLIENT_SUBSESSION_H
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "rtspServer_base.h"
#include "TS_process.h"
#include "h264_parse.h"
// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e., each audio or video 'substream').
// In practice, this might be a class (or a chain of classes) that decodes and then renders the incoming audio or video.
// Or it might be a "FileSink", for outputting the received data into a file (as is done by the "openRTSP" application).
// In this example code, however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.

typedef struct rtsp_ourregister
{
	ClientStreamStatus  stateCallback;
	RtspClientStreamStatus  rtspStateCallback;
	FrameCallBack frameCall;
	Rtsp_Inparam Inparm;
	void* rtsp_param;
	void* param;

}rtsp_ourregister_t;

class DummySink: public MediaSink {
public:
  static DummySink* createNew(UsageEnvironment& env,
			      MediaSubsession& subsession, // identifies the kind of data that's being received
				  rtsp_ourregister_t ourRegiserInfo,
			      char const* streamId = NULL); // identifies the stream itself (optional)

private:
  DummySink(UsageEnvironment& env, MediaSubsession& subsession, rtsp_ourregister_t ourRegiserInfo, char const* streamId);
    // called only by "createNew()"
  virtual ~DummySink();

  static void afterGettingFrame(void* clientData, unsigned frameSize,
                                unsigned numTruncatedBytes,
				struct timeval presentationTime,
                                unsigned durationInMicroseconds);
  void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
			 struct timeval presentationTime, unsigned durationInMicroseconds);
  int PutBits(unsigned char *buf, unsigned int bits, unsigned int numBits, int pos);

  int selectsocket();
private:
  // redefined virtual functions:
  virtual Boolean continuePlaying();

private:
  u_int8_t* fReceiveBuffer;
  MediaSubsession& fSubsession;
  char* fStreamId;

  rtsp_ourregister_t m_ourRegiserInfo;
  Rtsp_ClientStream_State_t m_ClientState;
  Fream_Info_t m_frameInfo;
  Fream_Info_t m_audioframeInfo;
  struct timeval m_prevTime;
  bool m_getVideoInfo;
  int count;
  struct timeval m_firstTime;
  int callBack_flag;
   bool m_getAudioInfo;
  int sample_rate;
  int audio_type;
public:
  MediaSession* session;
  bool m_showdown;
  int 	m_rtpsocketnum;
  int headsocketnum;
  int tailsocketnum;
  fd_set m_RcvSet;
  struct timeval m_Time;
  int m_is_ts;
  unsigned int m_prev_recv_length;
  int m_recv_count;
  int m_same_length;
  TsParam m_tsparam;
  TsLostPkt m_tslostpkt;
  long m_lDstDataLen;
  long m_lDemuxRet;
  long m_tshandle;
  u_int8_t* m_bakTsData;
  unsigned long long m_prevDts;
  unsigned int m_packetNum;
  int m_firstFrame;
  char m_tmp_Buffer[256];
};


// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:

class StreamClientState {
public:
  StreamClientState();
  virtual ~StreamClientState();

public:
  MediaSubsessionIterator* iter;
  MediaSession* session;
  MediaSubsession* subsession;
  TaskToken streamTimerTask;
  double duration;
};

// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState" field to the subclass:

class ourRTSPClient: public RTSPClient {
public:
  static ourRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
		  	  	  rtsp_ourregister_t* registerInfo,
				  int verbosityLevel = 0,
				  char const* applicationName = NULL,
				  portNumBits tunnelOverHTTPPortNum = 0);

protected:
  ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
		  rtsp_ourregister_t* registerInfo,
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
    // called only by createNew();
  virtual ~ourRTSPClient();

public:
  rtsp_ourregister_t m_ourRegiserInfo;
  Rtsp_ClientStream_State_t m_ClientState;
  StreamClientState scs;
};



// Forward function definitions:

// RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);

// Other event handler functions:
void subsessionAfterPlaying(void* clientData); // called when a stream's subsession (e.g., audio or video substream) ends
void subsessionByeHandler(void* clientData); // called when a RTCP "BYE" is received for a subsession
void streamTimerHandler(void* clientData);
  // called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")

// The main streaming routine (for each "rtsp://" URL):
RTSPClient* openURL(UsageEnvironment& env, char const* progName, char const* rtspURL, rtsp_ourregister_t* registerInfo);

// Used to iterate through each stream's 'subsessions', setting up each one:
void setupNextSubsession(RTSPClient* rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):
void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);

#endif
