/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2015, Live Networks, Inc.  All rights reserved
// A demo application, showing how to create and run a RTSP client (that can potentially receive multiple streams concurrently).
//
// NOTE: This code - although it builds a running application - is intended only to illustrate how to develop your own RTSP
// client application.  For a full-featured RTSP client application - with much more functionality, and many options - see
// "openRTSP": http://www.live555.com/openRTSP/

#include "rtspClient_subsession.h"


// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient) {
  return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) {
  return env << subsession.mediumName() << "/" << subsession.codecName();
}

void usage(UsageEnvironment& env, char const* progName) {
  env << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
  env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
}


#define RTSP_CLIENT_VERBOSITY_LEVEL 1 // by default, print verbose output from each "RTSPClient"

static unsigned rtspClientCount = 0; // Counts how many streams (i.e., "RTSPClient"s) are currently in use.

RTSPClient* openURL(UsageEnvironment& env, char const* progName, char const* rtspURL, rtsp_ourregister_t* registerInfo) {
  // Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
  // to receive (even if more than stream uses the same "rtsp://" URL).
  RTSPClient* rtspClient = ourRTSPClient::createNew(env, rtspURL, registerInfo,
		  RTSP_CLIENT_VERBOSITY_LEVEL,
		  progName);
  if (rtspClient == NULL) {
    env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
    return NULL;
  }

  ++rtspClientCount;

  // Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
  // Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
  // Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
  rtspClient->sendDescribeCommand(continueAfterDESCRIBE);
  return rtspClient;
}


// Implementation of the RTSP 'response handlers':

void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    //cwsadd
    ((ourRTSPClient*)rtspClient)->m_ClientState.status = RTSPCLIENT_FINISH;
    ((ourRTSPClient*)rtspClient)->m_ourRegiserInfo.rtspStateCallback((&((ourRTSPClient*)rtspClient)->m_ClientState),
    		(ourRTSPClient*)rtspClient);

    if (resultCode != 0) {
      env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
      delete[] resultString;
      break;
    }

    char* const sdpDescription = resultString;
    env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

    // Create a media session object from this SDP description:
    scs.session = MediaSession::createNew(env, sdpDescription,
    		((ourRTSPClient*)rtspClient)->m_ourRegiserInfo.Inparm.turnAudio);
    delete[] sdpDescription; // because we don't need it anymore
    if (scs.session == NULL) {
      env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
      break;
    } else if (!scs.session->hasSubsessions()) {
      env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
      break;
    }

    // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
    // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
    // (Each 'subsession' will have its own data source.)
    scs.iter = new MediaSubsessionIterator(*scs.session);
    setupNextSubsession(rtspClient);
    return;
  } while (0);
//  return;
  // An unrecoverable error occurred with this stream.
  return;
  shutdownStream(rtspClient);
}

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
#define REQUEST_STREAMING_OVER_TCP True

void setupNextSubsession(RTSPClient* rtspClient) {
  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

  scs.subsession = scs.iter->next();
  if (scs.subsession != NULL) {
    if (!scs.subsession->initiate()) {
      env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
      setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
    } else {
      env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
      if (scs.subsession->rtcpIsMuxed()) {
	env << "client port " << scs.subsession->clientPortNum();
      } else {
	env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
      }
      env << ")\n";

      // Continue setting up this subsession, by sending a RTSP "SETUP" command:
      rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
    }
    return;
  }

  // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
  if (scs.session->absStartTime() != NULL) {
    // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
  } else {
    scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
  }
}

void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
      break;
    }

    env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
    if (scs.subsession->rtcpIsMuxed()) {
      env << "client port " << scs.subsession->clientPortNum();
    } else {
      env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
    }
    env << ")\n";

    // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
    // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
    // after we've sent a RTSP "PLAY" command.)




    scs.subsession->sink = DummySink::createNew(env, *scs.subsession,((ourRTSPClient*)rtspClient)->m_ourRegiserInfo,
    		rtspClient->url());
      // perhaps use your own custom "MediaSink" subclass instead
    if (scs.subsession->sink == NULL) {
      env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
	  << "\" subsession: " << env.getResultMsg() << "\n";
      break;
    }
    ((DummySink*)(scs.subsession->sink))->session = scs.session;

    env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
    scs.subsession->miscPtr = rtspClient; // a hack to let subsession handle functions get the "RTSPClient" from the subsession
    scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
				       subsessionAfterPlaying, scs.subsession);
    // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
    if (scs.subsession->rtcpInstance() != NULL) {
      scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
    }
  } while (0);
  delete[] resultString;

  // Set up the next subsession, if any:
  setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
  Boolean success = False;

  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
      break;
    }

    // Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
    // using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
    // 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
    // (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
    if (scs.duration > 0) {
      unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
      scs.duration += delaySlop;
      unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
      scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
    }

    env << *rtspClient << "Started playing session";
    if (scs.duration > 0) {
      env << " (for up to " << scs.duration << " seconds)";
    }
    env << "...\n";

    success = True;
  } while (0);
  delete[] resultString;

  if (!success) {
    // An unrecoverable error occurred with this stream.
    shutdownStream(rtspClient);
  }
}


// Implementation of the other event handlers:

void subsessionAfterPlaying(void* clientData) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

  // Begin by closing this subsession's stream:
  Medium::close(subsession->sink);
  subsession->sink = NULL;

  // Next, check whether *all* subsessions' streams have now been closed:
  MediaSession& session = subsession->parentSession();
  MediaSubsessionIterator iter(session);
  while ((subsession = iter.next()) != NULL) {
    if (subsession->sink != NULL) return; // this subsession is still active
  }

  // All subsessions' streams have now been closed, so shutdown the client:
  shutdownStream(rtspClient);
}

void subsessionByeHandler(void* clientData) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
  UsageEnvironment& env = rtspClient->envir(); // alias

  env << *rtspClient << "Received RTCP \"BYE\" on \"" << *subsession << "\" subsession\n";

  // Now act as if the subsession had closed:
  subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) {
  ourRTSPClient* rtspClient = (ourRTSPClient*)clientData;
  StreamClientState& scs = rtspClient->scs; // alias

  scs.streamTimerTask = NULL;

  // Shut down the stream:
  shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient* rtspClient, int exitCode) {
  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
  // First, check whether any subsessions have still to be closed:
  if (scs.session != NULL) {
    Boolean someSubsessionsWereActive = False;
    MediaSubsessionIterator iter(*scs.session);
    MediaSubsession* subsession;

    while ((subsession = iter.next()) != NULL) {
      if (subsession->sink != NULL) {
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	if (subsession->rtcpInstance() != NULL) {
	  subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
	}

	someSubsessionsWereActive = True;
      }
    }

    if (someSubsessionsWereActive) {
      // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
      // Don't bother handling the response to the "TEARDOWN".
      rtspClient->sendTeardownCommand(*scs.session, NULL);
    }
  }

  env << *rtspClient << "Closing the stream.\n";
  Medium::close(rtspClient);
    // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

  if (--rtspClientCount == 0) {
    // The final stream has ended, so exit the application now.
    // (Of course, if you're embedding this code into your own application, you might want to comment this out,
    // and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
    return ;
  }
}


// Implementation of "ourRTSPClient":

ourRTSPClient* ourRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL,
					rtsp_ourregister_t* registerInfo,
					int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) {
  return new ourRTSPClient(env, rtspURL, registerInfo, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
					rtsp_ourregister_t* registerInfo,
			     int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
  : RTSPClient(env,rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) {
	if(registerInfo == NULL)
	{
		printf("\033[33m""ourRTSPClient is fail\n""\033[0m");
	}
	else
	{
		memcpy(&m_ourRegiserInfo, registerInfo, sizeof(rtsp_ourregister_t));
	}
	 m_ClientState.param = m_ourRegiserInfo.rtsp_param;
	 m_ourRegiserInfo.rtspStateCallback(&m_ClientState, this);
}

ourRTSPClient::~ourRTSPClient() {
	  m_ClientState.param = m_ourRegiserInfo.param;
	  m_ClientState.status = RTSPCLIENT_STOP;
	  m_ourRegiserInfo.stateCallback(&m_ClientState);

	  m_ClientState.param = m_ourRegiserInfo.rtsp_param;
	  m_ourRegiserInfo.rtspStateCallback(&m_ClientState, NULL);
}


// Implementation of "StreamClientState":

StreamClientState::StreamClientState()
  : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) {
}

StreamClientState::~StreamClientState() {
  delete iter;
  if (session != NULL) {
    // We also need to delete "session", and unschedule "streamTimerTask" (if set)
    UsageEnvironment& env = session->envir(); // alias

    env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
    Medium::close(session);
  }
}


// Implementation of "DummySink":

// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:

//cws===============================================
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 2000000

DummySink* DummySink::createNew(UsageEnvironment& env, MediaSubsession& subsession, rtsp_ourregister_t ourRegiserInfo,
		char const* streamId)
{
  return new DummySink(env, subsession, ourRegiserInfo, streamId);
}

#define DUMMY_SINK_AUDFIO_BUFFER_SIZE 1000000
DummySink::DummySink(UsageEnvironment& env, MediaSubsession& subsession, rtsp_ourregister_t ourRegiserInfo, char const* streamId)
  : MediaSink(env),
    fSubsession(subsession) {
  fStreamId = strDup(streamId);
  fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
  m_frameInfo.data = NULL;
  m_tshandle = 0;
  m_bakTsData = NULL;
  m_ourRegiserInfo = ourRegiserInfo;
  m_frameInfo.param = ourRegiserInfo.param;
  m_frameInfo.height = 0;
  m_frameInfo.width = 0;
  if(subsession.sample_rate < 7350 || subsession.sample_rate > 96000)
  {
	  subsession.sample_rate = 48000;
  }
  m_frameInfo.audio_type = subsession.audio_type;
  m_frameInfo.sample_rate = subsession.sample_rate;
  m_audioframeInfo.audio_type = subsession.audio_type;
  m_audioframeInfo.sample_rate = subsession.sample_rate;
  m_frameInfo.fps = subsession.videoFPS();
  m_frameInfo.frameSize = 0;
  m_audioframeInfo.param = ourRegiserInfo.param;
  m_ClientState.param = ourRegiserInfo.param;
  m_ClientState.status = RTSPCLIENT_START;
  m_ourRegiserInfo.stateCallback(&m_ClientState);
  m_getVideoInfo = 0;
  m_getAudioInfo = 0;
  m_prevTime.tv_sec = m_prevTime.tv_usec = 0;
  m_showdown = 0;
  callBack_flag = 0;
  m_firstTime.tv_sec = m_firstTime.tv_usec = 0;
  count = 0;
  audio_type = 0;
  sample_rate = 0;
  m_is_ts = 0;
  m_prev_recv_length = 0;
  m_recv_count = 0;
  m_same_length = 0;
  m_prevDts = 0;
  m_firstFrame = 0;

}

DummySink::~DummySink() {
  DestroyOneTsStreamDemux(m_tshandle);
  delete[] fReceiveBuffer;
  delete[] fStreamId;
  delete [] m_frameInfo.data;
  delete [] m_bakTsData;

}

void DummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned durationInMicroseconds) {
  DummySink* sink = (DummySink*)clientData;
  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1
int DummySink::PutBits(unsigned char *buf, unsigned int bits, unsigned int numBits, int pos)
{
	for (int i = numBits - 1; i >= 0; i--)
	{
		buf[pos >> 3] |= ((bits >> i) & 1) << (7 - (pos & 7));
		pos++;
	}

	return pos;
}
int AAC_sample_rate[12] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000};

int DummySink::selectsocket()
{
	FD_ZERO(&m_RcvSet);
	if(session->fSubsessionsHead)
	{
		headsocketnum = session->fSubsessionsHead->rtpSource()->RTPgs()->socketNum();
		FD_SET(headsocketnum, &m_RcvSet);
	}
	if(session->fSubsessionsTail && session->fSubsessionsHead != session->fSubsessionsTail)
	{
		tailsocketnum = session->fSubsessionsTail->rtpSource()->RTPgs()->socketNum();
		FD_SET(tailsocketnum, &m_RcvSet);
	}
	if(tailsocketnum < headsocketnum)
	{
		m_rtpsocketnum = headsocketnum;
	}
	else
	{
		m_rtpsocketnum = tailsocketnum;
	}

	m_Time.tv_sec = 0;
	m_Time.tv_usec = 1000 * 30;
	return select(m_rtpsocketnum + 1, &m_RcvSet, NULL, NULL, &m_Time);

}
void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned durationInMicroseconds) {
  // We've just received a frame of data.  (Optionally) print out information about it:
#if 0
  if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
  envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
  if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
  char uSecsStr[6+1]; // used to output the 'microseconds' part of the presentation time
  sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
  envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
  if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
    envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
  }
#ifdef DEBUG_PRINT_NPT
  envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
  envir() << "\n";
#endif

  m_rtpsocketnum = fSubsession.rtpSource()->RTPgs()->socketNum();
  if(m_showdown == 1)
  {
	  return;
  }
  if(m_recv_count < 5)
  {
	  if (0 == strcmp(fSubsession.mediumName(), "video"))
	  {
		 // if(m_prev_recv_length != 0)
		  {
			  if(fReceiveBuffer[0] == 0x47)
			  {
				  m_same_length++;
			  }
			  else
			  {
				  m_same_length = 0;
			  }
		  }
		//  m_prev_recv_length = frameSize;
		  m_recv_count++;
		  if(m_recv_count == 5)
		  {
			  if(m_same_length == 5)
			  {
				  m_is_ts = 1;
				  m_tshandle = CreateOneTsStreamDemux();
				  m_bakTsData = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
			  }
			  printf("m_same_length:%d m_is_ts:%d\n", m_same_length, m_is_ts);

		  }
	  }
	  else
	  {
		  m_recv_count = 5;
	  }

  }
  else
  {
	  if(m_is_ts == 1)
	  {
		  if(m_frameInfo.data == NULL)
		  {
			  m_frameInfo.data = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
		  }
		  m_packetNum = 0;

		  while(1)
		  {
			  if(m_packetNum * 188 < frameSize)
			  {
				  m_lDstDataLen = DUMMY_SINK_RECEIVE_BUFFER_SIZE;
				  m_lDemuxRet =  DemuxTsStream(m_tshandle,(char*)fReceiveBuffer + m_packetNum * 188,(char*)m_bakTsData,&m_lDstDataLen,&m_tsparam,&m_tslostpkt);
				  m_packetNum++;
				  if(m_lDstDataLen > 0 && (m_tsparam.m_lDstDataType == 0x1b))
				  {
//				 	  printf("fReceiveBuffer[0] & 0xf:%0x %0x %0x %0x %0x\n", m_frameInfo.data[0], m_frameInfo.data[1], m_frameInfo.data[2],
//				 			 m_frameInfo.data[3], m_frameInfo.data[4]);
					  //printf("m_tsparam.m_lDts:%llu\n", m_tsparam.m_lDts);

					  if((m_tsparam.m_lDts != m_prevDts) && (m_frameInfo.frameSize != 0))
					  {
						  if(m_frameInfo.fps == 0)
						  {
							  if((m_prevDts != 0) && (m_tsparam.m_lDts != 0))
							  {
								  int timecur = m_tsparam.m_lDts - m_prevDts;
								  if(timecur != 0)
								  {
									  m_frameInfo.fps  = 90000 /timecur;
								  }

							  }
						  }
				 		  m_frameInfo.type = VIDEO_TYPE;
				 		  m_frameInfo.presentationTime = m_prevTime;
					 	  //通过分析sps获取视频的宽高，本身从rtp头获取宽高有些厂家错误或者不支持
					 	  if(((m_frameInfo.data[4] & 0xf) == 0x7) )
					 	  {
					 		 h264_decode_sps((BYTE*)(char*)(m_frameInfo.data +4), frameSize,m_frameInfo.width,m_frameInfo.height, m_frameInfo.fps);
					 		  m_getVideoInfo = 1;
					 	  }
					 	  else if( ((m_frameInfo.data[4] & 0xf) == 0x9) && ((m_frameInfo.data[10] & 0xf) == 0x7))
					 	  {
						 		// h264_decode_sps((BYTE*)(char*)(m_frameInfo.data +10), frameSize,m_frameInfo.width,m_frameInfo.height, m_frameInfo.fps);
						 		  m_getVideoInfo = 1;
					 	  }
//					 	  printf("0:%0x 1:%0x 2:%0x 3:%0x 4:%0x  5:%0x 6:%0x 7:%0x 8:%0x 9:%0x 10:%0x rameSize:%d fps:%d\n",
//					 			  m_frameInfo.data[0], m_frameInfo.data[1],
//					 			 m_frameInfo.data[2], m_frameInfo.data[3], m_frameInfo.data[4],m_frameInfo.data[5],
//								 m_frameInfo.data[6], m_frameInfo.data[7],  m_frameInfo.data[8],  m_frameInfo.data[9],
//								 m_frameInfo.data[10], m_frameInfo.frameSize, m_frameInfo.fps);

					 	if((m_frameInfo.data[4] & 0xf) == 0x9)
						{
							m_firstFrame = 1;
							if((m_frameInfo.data[10] & 0xf) == 0x7)
							{
								m_frameInfo.iFrame = 1;
							}
						}
						else if(((m_frameInfo.data[4] & 0xf) == 0x5) || ((m_frameInfo.data[4] & 0xf) == 0x7))
					 	 {
					 		m_firstFrame = 1;
					 		m_frameInfo.iFrame = 1;
					 	 }
					 	 if(m_firstFrame)
					 	 {
					 		m_ourRegiserInfo.frameCall(&m_frameInfo);
					 	 }

				 		  m_frameInfo.frameSize = 0;
				 		  m_frameInfo.iFrame = 0;
					  }
					  {

			 			  memcpy(m_frameInfo.data + m_frameInfo.frameSize, m_bakTsData, m_lDstDataLen);
			 			  m_frameInfo.frameSize += m_lDstDataLen;
					  }
					  m_prevDts = m_tsparam.m_lDts;
				  }
			  }
			  else
			  {
				  break;
			  }
		}

	  }
	  else
	  {
		  if (0 == strcmp(fSubsession.mediumName(), "video"))
		   {

			  if(m_frameInfo.data == NULL)
			  {
				  m_frameInfo.data = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
			  }
		 	  //通过分析sps获取视频的宽高，本身从rtp头获取宽高有些厂家错误或者不支持
		 	  if((fReceiveBuffer[0] & 0xf) == 0x7 && m_getVideoInfo == 0)
		 	  {
				//  memcpy(m_tmp_Buffer, fReceiveBuffer, frameSize);
				  h264_decode_sps((BYTE*)fReceiveBuffer, frameSize,m_frameInfo.width,m_frameInfo.height, m_frameInfo.fps);
				  m_getVideoInfo = 1;
		 	  }


		 		//printf("th channel=%d\n", );
		 	  if(!(m_prevTime.tv_sec == presentationTime.tv_sec && m_prevTime.tv_usec == presentationTime.tv_usec)
		 			  && m_prevTime.tv_sec != 0)
		 	  {
		 		 // printf("fReceiveBuffer[0] & 0xf:%0x'n", fReceiveBuffer[0] & 0xf);
		 		 if (count == 0)
		 		 {
		 			m_firstTime = m_prevTime;
		 		 }
		 		 count++;

		 		if(m_frameInfo.fps <= 0 || m_frameInfo.fps > 60)
		 		 {
		 			int timecur = 0;
		 			if(m_frameInfo.bakfps <= 0 || m_frameInfo.bakfps > 60)
		 			{
						     timecur = (presentationTime.tv_sec - m_prevTime.tv_sec) * 1000000 +
									  (presentationTime.tv_usec - m_prevTime.tv_usec)  ;
							  if(timecur != 0)
							  {
								  m_frameInfo.bakfps  = 1000000 /timecur;
								  if(m_frameInfo.bakfps == 29)
								  {
									  m_frameInfo.bakfps = 30;
								  }
								  if(m_frameInfo.bakfps == 59)
								  {
									  m_frameInfo.bakfps = 60;
								  }
							  }
		 			}


					 if ((m_prevTime.tv_sec - m_firstTime.tv_sec) * 1000000 + (m_prevTime.tv_usec - m_firstTime.tv_usec)
							 >= 5000000)
					 {
						m_frameInfo.fps =  count / (m_prevTime.tv_sec - m_firstTime.tv_sec);

						if((m_frameInfo.fps - m_frameInfo.bakfps <= 3) ||
								(m_frameInfo.bakfps - m_frameInfo.fps <= 3))
						{
							//证明时间戳是对的
							m_frameInfo.fps = m_frameInfo.bakfps;
						}
						else
						{
							int remainder = m_frameInfo.fps % 5;
							if((remainder < 3) && (remainder > 0))
							{
								m_frameInfo.fps = m_frameInfo.fps / 5 * 5;
							}
							else if(remainder >= 3)
							{
								m_frameInfo.fps = m_frameInfo.fps / 5 * 5 + 5;
							}

						}
						//printf("/033[35m""the fps =%d\n""\033[0m", m_frameInfo.fps);
					 }
					 if(m_frameInfo.fps != 0)
					 {
						  printf("/033[35m""++the bakfps =%d timecur:%d this:%p durationInMicroseconds:%d fps:%u\n""\033[0m", m_frameInfo.bakfps, timecur,
								  this, durationInMicroseconds, m_frameInfo.fps);
					 }


		 		 }



		 		  m_frameInfo.type = VIDEO_TYPE;
		 		  m_frameInfo.presentationTime = m_prevTime;
		 		  //代表I帧已经到来
		 		  if(m_firstFrame == 1)
		 		  {
		 			 m_ourRegiserInfo.frameCall(&m_frameInfo);
		 		  }


		 		 // memset(m_frameInfo.data, 0, DUMMY_SINK_RECEIVE_BUFFER_SIZE);
		 		  m_frameInfo.frameSize = 0;
		 		  m_frameInfo.iFrame = 0;
		 	  }
		 	  if(m_frameInfo.frameSize + 4 < DUMMY_SINK_RECEIVE_BUFFER_SIZE)
		 	  {

		 		  if(((fReceiveBuffer[0] & 0xf) == 0x5 || ((fReceiveBuffer[0] & 0xf) == 0x7))
		 				  && ((m_frameInfo.height != 0) && (m_frameInfo.width !=0 )))
		 		  {
		 			 m_firstFrame = 1;
		 			  m_frameInfo.iFrame = 1;
		 		  }
		 		  m_frameInfo.data[m_frameInfo.frameSize] = 0x00;
		 		  m_frameInfo.data[m_frameInfo.frameSize + 1] = 0x00;
		 		  m_frameInfo.data[m_frameInfo.frameSize + 2] = 0x00;
		 		  m_frameInfo.data[m_frameInfo.frameSize + 3] = 0x01;

		 		  m_frameInfo.frameSize += 4;

		 		  if( m_frameInfo.frameSize + frameSize <  DUMMY_SINK_RECEIVE_BUFFER_SIZE)
		 		  {
		 			  memcpy(m_frameInfo.data + m_frameInfo.frameSize, fReceiveBuffer, frameSize);
		 			  m_frameInfo.frameSize += frameSize;
		 		  }
		 		  else
		 		  {
		 			  m_frameInfo.frameSize = 0;
		 			  m_frameInfo.iFrame = 0;
		 			  printf("\033[32m""frameSize is over %d\n""\033[0m", DUMMY_SINK_RECEIVE_BUFFER_SIZE);
		 		  }

		 	  }
		 	  else
		 	  {
		 		  m_frameInfo.frameSize = 0;
		 		  m_frameInfo.iFrame = 0;
		 	  }
		 	  m_prevTime = presentationTime;


		   }
		   else if (0 == strcmp(fSubsession.mediumName(), "audio"))
		    {
		 	 	 m_audioframeInfo.type = AUDIO_TYPE;
		 	 	 m_audioframeInfo.data = fReceiveBuffer;
		  	 	  m_audioframeInfo.frameSize = frameSize;
		  	 	  m_audioframeInfo.presentationTime = presentationTime;
		  	 	  m_ourRegiserInfo.frameCall(&m_audioframeInfo);
		  	 }

	  }



  }


  if(m_rtpsocketnum > 2)
  {
#if 0
	  int ret = 0;
	  struct timeval BeginTime;
	  struct timeval EndTime;
	  gettimeofday (&BeginTime, NULL);
	  ret = selectsocket();
	  gettimeofday (&EndTime, NULL);
	  //调试
	  {
		  printf("fSubsession.mediumName():%s headsocketnum :%d tailsocketnum:%d socektnum:===%d=== ret:%d time:%ld\n",
				  fSubsession.mediumName(), headsocketnum, tailsocketnum, m_rtpsocketnum,
				  ret, (EndTime.tv_sec - BeginTime.tv_sec) * 1000000 + (EndTime.tv_usec - BeginTime.tv_usec));
	  }
#else
	  selectsocket();
#endif

  }
  // Then continue, to request the next frame of data:
  continuePlaying();
}

Boolean DummySink::continuePlaying() {
  if (fSource == NULL) return False; // sanity check (should not happen)

  // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
  fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                        afterGettingFrame, this,
                        onSourceClosure, this);
  return True;
}
