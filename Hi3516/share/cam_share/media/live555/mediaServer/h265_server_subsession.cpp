/*
 * @FilePath: h265_server_subsession.cpp
 * @Author: yangwenyao
 * @Date: 2023-01-03 13:55:20
 * @LastEditors: ywy
 * @LastEditTime: 2023-01-03 16:06:49
 * @Descripttion: h265 subsession server
 */
#include "h265_server_subsession.h"
 
H265_Server_Subsession* H265_Server_Subsession::createNew(UsageEnvironment & env, Video_Source_Info_t& sourceInfo)
{
  return new H265_Server_Subsession(env, sourceInfo);
}

H265_Server_Subsession::H265_Server_Subsession(UsageEnvironment & env, Video_Source_Info_t& sourceInfo)
: OnDemandServerMediaSubsession(env, True)
{
	printf("H265_Server_Subsession is create\n");
	memcpy(&m_pSouceInfo, &sourceInfo, sizeof(Video_Source_Info_t));
	fAuxSDPLine = NULL;
	fDoneFlag = 0;
	fDummyRTPSink = NULL;
	m_video_source = NULL;
}
H265_Server_Subsession::~H265_Server_Subsession() {
	printf("H265_Server_Subsession is delete\n");

	delete[] fAuxSDPLine;
}

static void afterPlayingDummy(void* clientData) {
  H265_Server_Subsession* subsess = (H265_Server_Subsession*)clientData;
  subsess->afterPlayingDummy1();
}

void H265_Server_Subsession::afterPlayingDummy1() {
  // Unschedule any pending 'checking' task:
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
  // Signal the event loop that we're done:
  setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData) {
  H265_Server_Subsession* subsess = (H265_Server_Subsession*)clientData;
  subsess->checkForAuxSDPLine1();
}

void H265_Server_Subsession::checkForAuxSDPLine1() {
  /* 清理上一次延时轮询任务句柄，避免旧任务状态影响本次 auxSDPLine 检查。 */
  nextTask() = NULL;
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
    /* 当 VPS/SPS/PPS 还未准备好时继续轮询等待，避免过早退出导致 SDP 缺少 fmtp 参数。 */
    // try again after a brief delay:
	int uSecsToDelay = 100000; // 100 ms
    nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
			      (TaskFunc*)checkForAuxSDPLine, this);
  }
}

char const* H265_Server_Subsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) {
  if (fAuxSDPLine != NULL) return fAuxSDPLine; // it's already been set up (for a previous client)

  if (fDummyRTPSink == NULL) { // we're not already setting it up for another, concurrent stream
    /* 新客户端触发 DESCRIBE 时重置完成标志，确保事件循环重新等待 auxSDPLine 就绪。 */
    fDoneFlag = 0;
		// Note: For H265 video files, the 'config' information (used for several payload-format
		// specific parameters in the SDP description) isn't known until we start reading the file.
		// This means that "rtpSink"s "auxSDPLine()" will be NULL initially,
		// and we need to start reading data from our file until this changes.
    fDummyRTPSink = rtpSink;

    // Start reading the file:
    fDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);

    // Check whether the sink's 'auxSDPLine()' is ready:
    checkForAuxSDPLine(this);
  }
  envir().taskScheduler().doEventLoop(&fDoneFlag);

  return fAuxSDPLine;
}

FramedSource* H265_Server_Subsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate)
{
	estBitrate = 20000;

	m_video_source = new H265_Video_Source(envir(),m_pSouceInfo);
	if(m_pSouceInfo.nAudio == 1)
	{
		m_video_source->m_nSynchronize = 1;
	}
	else
	{
		m_video_source->m_nSynchronize = 0;
	}

	return H265VideoStreamFramer::createNew(envir(), m_video_source);
}

RTPSink* H265_Server_Subsession::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* /*inputSource*/) {
  return H265VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}

// info ------------------ zhouzr ------------------
/**
 * @brief   : 重写父类方法，在客户端开始播放时调用
 * @note    : 这是触发 "客户端连接" 回调的正确位置
 */
void H265_Server_Subsession::startStream(unsigned clientSessionId,
  void *streamToken,
  TaskFunc *rtcpRRHandler,
  void *rtcpRRHandlerClientData,
  unsigned short &rtpSeqNum,
  unsigned &rtpTimestamp,
  ServerRequestAlternativeByteHandler *serverRequestAlternativeByteHandler,
  void *serverRequestAlternativeByteHandlerClientData)
{
/* 首先，调用父类的原始方法，确保 live555 内部逻辑正确执行 */
OnDemandServerMediaSubsession::startStream(clientSessionId,
      streamToken,
      rtcpRRHandler,
      rtcpRRHandlerClientData,
      rtpSeqNum,
      rtpTimestamp,
      serverRequestAlternativeByteHandler,
      serverRequestAlternativeByteHandlerClientData);

/* 然后，调用我们自己的状态回调函数，通知上层应用有新客户端连接 */
if (m_pSouceInfo.clientFun)
{
Rtsp_ClientStream_State_t clientState;
clientState.status = RTSPCLIENT_START;
/* m_pSouceInfo.videoindex 实际上是您在 rtsp_server.cpp 中传入的 Live_Stream_Info_t 指针 */
clientState.param = m_pSouceInfo.videoindex;
m_pSouceInfo.clientFun(&clientState);
}
}

/**
* @brief   : 重写父类方法，在客户端流被删除时调用
* @note    : 这是触发 "客户端断开" 回调的正确位置
*/
void H265_Server_Subsession::deleteStream(unsigned clientSessionId, void *&streamToken)
{
/**
* fParentSession 是 ServerMediaSubsession 的成员，指向父级的 ServerMediaSession。
* fParentSession->referenceCount() 返回当前连接的客户端数量。
* 在 deleteStream 被调用时，这个即将断开的客户端仍然被计数。
* 因此，如果计数为 1，说明这是最后一个客户端。
*/
unsigned int clientCount = fParentSession->referenceCount();
if (clientCount == 1)
{
/* 这是最后一个客户端，可以安全地通知上层停止数据请求和清理队列 */
if (m_pSouceInfo.clientFun)
{
live_log("最后一个客户端已断开连接");
Rtsp_ClientStream_State_t clientState;
clientState.status = RTSPCLIENT_STOP;
clientState.param = m_pSouceInfo.videoindex;
m_pSouceInfo.clientFun(&clientState);
}
}
else
{
live_log("一个客户端断开连接，还有 %u 个客户端", clientCount - 1);
}

/* 最后，必须调用父类的原始方法来完成内部清理 */
OnDemandServerMediaSubsession::deleteStream(clientSessionId, streamToken);
}
// info ------------------ zhouzr ------------------
