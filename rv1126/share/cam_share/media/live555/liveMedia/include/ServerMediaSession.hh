/**
 * @FilePath     : ServerMediaSession.hh
 * @Author       : 17343431340@163.com
 * @Date         : 2026-02-27 13:40:43
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-19 15:58:15
 * @Description  : live555媒体会话及客户端准入接口
 */
/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2022 Live Networks, Inc.  All rights reserved.
// A data structure that represents a session that consists of
// potentially multiple (audio and/or video) sub-sessions
// (This data structure is used for media *streamers* - i.e., servers.
//  For media receivers, use "MediaSession" instead.)
// C++ header

#ifndef _SERVER_MEDIA_SESSION_HH
#define _SERVER_MEDIA_SESSION_HH

#ifndef _RTCP_HH
#include "RTCP.hh"
#endif

/* fClientInfo 的容量必须覆盖 rtsp_setclient_maxNum() 支持的连接数。 */
#ifndef CLIENTMAX
#define CLIENTMAX 32
#endif

/* 容量扩展不能改变未显式配置会话的兼容默认上限。 */
#ifndef CLIENT_DEFAULT_MAX
#define CLIENT_DEFAULT_MAX 4
#endif

/*
 * 通用 RTSP 会话准入回调。liveMedia 只传递不透明上下文，不依赖业务层头文件；
 * 返回 0 表示允许首个 SETUP，非 0 表示拒绝。
 */
typedef int (*ServerMediaSessionAdmissionCallback)(void* opaque);
typedef struct
{
	char ip[16];
	int socket;
	int port;

}Connect_Client_Info_t;

class ServerMediaSubsession; // forward

class ServerMediaSession: public Medium {
public:
  static ServerMediaSession* createNew(UsageEnvironment& env,
				       char const* streamName = NULL,
				       char const* info = NULL,
				       char const* description = NULL,
				       Boolean isSSM = False,
				       char const* miscSDPLines = NULL);

  static Boolean lookupByName(UsageEnvironment& env,
                              char const* mediumName,
                              ServerMediaSession*& resultSession);

  char* generateSDPDescription(int addressFamily); // based on the entire session
      // Note: The caller is responsible for freeing the returned string

  char const* streamName() const { return fStreamName; }

  Boolean addSubsession(ServerMediaSubsession* subsession);
  unsigned numSubsessions() const { return fSubsessionCounter; }

  void testScaleFactor(float& scale); // sets "scale" to the actual supported scale
  float duration() const;
    // a result == 0 means an unbounded session (the default)
    // a result < 0 means: subsession durations differ; the result is -(the largest).
    // a result > 0 means: this is the duration of a bounded session

  virtual void noteLiveness();
    // called whenever a client - accessing this media - notes liveness.
    // The default implementation does nothing, but subclasses can redefine this - e.g., if you
    // want to remove long-unused "ServerMediaSession"s from the server.

  unsigned referenceCount() const { return fReferenceCount; }
  void incrementReferenceCount() { ++fReferenceCount; }
  void decrementReferenceCount() 
  { 
    	  if (fReferenceCount > 0)
		  {
		  	  if(fReferenceCount >=0 && fReferenceCount < CLIENTMAX)
		  	  {
		  		 memset(&(fClientInfo[fReferenceCount]), 0, sizeof(Connect_Client_Info_t));
		  	  }

			  --fReferenceCount;
		  }
   }
  Boolean& deleteWhenUnreferenced() { return fDeleteWhenUnreferenced; }

  void deleteAllSubsessions();
    // Removes and deletes all subsessions added by "addSubsession()", returning us to an 'empty' state
    // Note: If you have already added this "ServerMediaSession" to a server then, before calling this function,
    //   you must first close any client connections that use it,
    //   by calling "GenericMediaServer::closeAllClientSessionsForServerMediaSession()".

  Boolean streamingUsesSRTP; // by default, False
  Boolean streamingIsEncrypted; // by default, False

  void incrementReferenceCount(int socket)
  {
	  if(getpeername(socket,(struct sockaddr*)&flocal, (socklen_t *)&faddr_len) < 0)
	  {
		  ++fReferenceCount;
		  printf("getsockname is fail\n");
		  return;
	  }
	  if(fReferenceCount >=0 && fReferenceCount < CLIENTMAX)
	  {
		  snprintf(fClientInfo[fReferenceCount].ip, sizeof(fClientInfo[fReferenceCount].ip), "%s", (char*)inet_ntoa(flocal.sin_addr));
		  fClientInfo[fReferenceCount].socket = socket;
		  printf("\033[33m""fReferenceCount: %d ip: %s socket : %d\n""\033[0m",
				  fReferenceCount, fClientInfo[fReferenceCount].ip, socket);
	  }

	  ++fReferenceCount;

  }

  /**
   * @brief   : 注册首个 SETUP 前执行的通用会话准入回调
   * @param   {ServerMediaSessionAdmissionCallback} callback：准入回调，返回0表示允许
   * @param   {void*} opaque：传递给准入回调的不透明上下文
   * @return  {void} 无
   * @note    : 回调由 live555 事件线程调用，执行时尚未增加引用计数。
   */
  void setClientAdmissionCallback(ServerMediaSessionAdmissionCallback callback, void* opaque);

  /**
   * @brief   : 检查当前媒体会话是否允许新增一个 RTSP 客户端
   * @return  {int} 0表示允许，非0表示拒绝
   * @note    : 业务回调负责跨会话策略，fReferenceMax 始终作为本媒体会话硬上限。
   */
  int checkClientAdmission() const;

protected:
  ServerMediaSession(UsageEnvironment& env, char const* streamName,
		     char const* info, char const* description,
		     Boolean isSSM, char const* miscSDPLines);
  // called only by "createNew()"

  virtual ~ServerMediaSession();

private: // redefined virtual functions
  virtual Boolean isServerMediaSession() const;

private:
  Boolean fIsSSM;

  // Linkage fields:
  friend class ServerMediaSubsessionIterator;
  ServerMediaSubsession* fSubsessionsHead;
  ServerMediaSubsession* fSubsessionsTail;
  unsigned fSubsessionCounter;

  char* fStreamName;
  char* fInfoSDPString;
  char* fDescriptionSDPString;
  char* fMiscSDPLines;
  struct timeval fCreationTime;
  unsigned fReferenceCount;
  Boolean fDeleteWhenUnreferenced;

/* custom add */
public:
  Connect_Client_Info_t fClientInfo[CLIENTMAX];
  unsigned fReferenceMax;
private:
  struct sockaddr_in flocal;
  int faddr_len;
  /* 首个 SETUP 前由 mediaServer 封装层提供的准入判断。 */
  ServerMediaSessionAdmissionCallback fClientAdmissionCallback;
  /* 准入回调使用的长期有效业务上下文，不由 liveMedia 释放。 */
  void* fClientAdmissionOpaque;
};


class ServerMediaSubsessionIterator {
public:
  ServerMediaSubsessionIterator(ServerMediaSession& session);
  virtual ~ServerMediaSubsessionIterator();

  ServerMediaSubsession* next(); // NULL if none
  void reset();

private:
  ServerMediaSession& fOurSession;
  ServerMediaSubsession* fNextPtr;
};


class ServerMediaSubsession: public Medium {
public:
  unsigned trackNumber() const { return fTrackNumber; }
  char const* trackId();
  virtual char const* sdpLines(int addressFamily) = 0;
  virtual void getStreamParameters(unsigned clientSessionId, // in
				   struct sockaddr_storage const& clientAddress, // in
				   Port const& clientRTPPort, // in
				   Port const& clientRTCPPort, // in
				   int tcpSocketNum, // in (-1 means use UDP, not TCP)
				   unsigned char rtpChannelId, // in (used if TCP)
				   unsigned char rtcpChannelId, // in (used if TCP)
				   TLSState* tlsState, // in (used if TCP)
				   struct sockaddr_storage& destinationAddress, // in out
				   u_int8_t& destinationTTL, // in out
				   Boolean& isMulticast, // out
				   Port& serverRTPPort, // out
				   Port& serverRTCPPort, // out
				   void*& streamToken // out
				   ) = 0;
  virtual void startStream(unsigned clientSessionId, void* streamToken,
			   TaskFunc* rtcpRRHandler,
			   void* rtcpRRHandlerClientData,
			   unsigned short& rtpSeqNum,
			   unsigned& rtpTimestamp,
			   ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
			   void* serverRequestAlternativeByteHandlerClientData) = 0;
  virtual void pauseStream(unsigned clientSessionId, void* streamToken);
  virtual void seekStream(unsigned clientSessionId, void* streamToken, double& seekNPT,
			  double streamDuration, u_int64_t& numBytes);
     // This routine is used to seek by relative (i.e., NPT) time.
     // "streamDuration", if >0.0, specifies how much data to stream, past "seekNPT".  (If <=0.0, all remaining data is streamed.)
     // "numBytes" returns the size (in bytes) of the data to be streamed, or 0 if unknown or unlimited.
  virtual void seekStream(unsigned clientSessionId, void* streamToken, char*& absStart, char*& absEnd);
     // This routine is used to seek by 'absolute' time.
     // "absStart" should be a string of the form "YYYYMMDDTHHMMSSZ" or "YYYYMMDDTHHMMSS.<frac>Z".
     // "absEnd" should be either NULL (for no end time), or a string of the same form as "absStart".
     // These strings may be modified in-place, or can be reassigned to a newly-allocated value (after delete[]ing the original).
  virtual void nullSeekStream(unsigned clientSessionId, void* streamToken,
			      double streamEndTime, u_int64_t& numBytes);
     // Called whenever we're handling a "PLAY" command without a specified start time.
  virtual void setStreamScale(unsigned clientSessionId, void* streamToken, float scale);
  virtual float getCurrentNPT(void* streamToken);
  virtual FramedSource* getStreamSource(void* streamToken);
  virtual void getRTPSinkandRTCP(void* streamToken,
				 RTPSink const*& rtpSink, RTCPInstance const*& rtcp) = 0;
     // Returns pointers to the "RTPSink" and "RTCPInstance" objects for "streamToken".
     // (This can be useful if you want to get the associated 'Groupsock' objects, for example.)
     // You must not delete these objects, or start/stop playing them; instead, that is done
     // using the "startStream()" and "deleteStream()" functions.
  virtual void deleteStream(unsigned clientSessionId, void*& streamToken);

  virtual void testScaleFactor(float& scale); // sets "scale" to the actual supported scale
  virtual float duration() const;
    // returns 0 for an unbounded session (the default)
    // returns > 0 for a bounded session
  virtual void getAbsoluteTimeRange(char*& absStartTime, char*& absEndTime) const;
    // Subclasses can reimplement this iff they support seeking by 'absolute' time.

protected: // we're a virtual base class
  ServerMediaSubsession(UsageEnvironment& env);
  virtual ~ServerMediaSubsession();

  char const* rangeSDPLine() const;
      // returns a string to be delete[]d

  ServerMediaSession* fParentSession;

private:
  friend class ServerMediaSession;
  friend class ServerMediaSubsessionIterator;
  ServerMediaSubsession* fNext;

  unsigned fTrackNumber; // within an enclosing ServerMediaSession
  char const* fTrackId;
};

#endif
