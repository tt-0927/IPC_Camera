

/*
 * @FilePath: MJPEG_FRAME_SOURCE.cpp
 * @Author: yangwenyao
 * @Date: 2023-01-03 14:08:38
 * @LastEditors: yangwenyao
 * @LastEditTime: 2023-01-05 11:04:45
 * @Descripttion: h265 rtp source
 */
#include "mpeg_frame_source.h"
MJPEG_FRAME_SOURCE::MJPEG_FRAME_SOURCE(UsageEnvironment & env, Video_Source_Info_t& souceInfo):
FramedSource(env)
{
	memcpy(&m_stSourceInfo, &souceInfo, sizeof(Video_Source_Info_t));

	m_toDelay = 100;
	fFrameSize = 0;
	m_stStatus.param = NULL;
}
MJPEG_FRAME_SOURCE::~MJPEG_FRAME_SOURCE(void)
{
	envir().taskScheduler().unscheduleDelayedTask(m_pToken);
	if(m_stSourceInfo.clientFun)
	{
		m_stStatus.param = m_stSourceInfo.videoindex;
		m_stStatus.status = RTSPCLIENT_STOP;
		m_stSourceInfo.clientFun(&m_stStatus);
	}
}

int MJPEG_FRAME_SOURCE::set_videostate_callback()
{
	if(m_stStatus.param == NULL)
	{
		if(m_stSourceInfo.clientFun)
		{
			m_stStatus.param = m_stSourceInfo.videoindex;
			m_stStatus.status = RTSPCLIENT_START;
			m_stSourceInfo.clientFun(&m_stStatus);
		}
	}
	return 0;
}
int MJPEG_FRAME_SOURCE::server_inputData(unsigned char* pData, int nDataLen)
{
	return 1;
}

void MJPEG_FRAME_SOURCE::doGetNextFrame()
{

	m_pToken = envir().taskScheduler().scheduleDelayedTask(m_toDelay, getNextFrame, this);
}

unsigned int MJPEG_FRAME_SOURCE::maxFrameSize() const
{
  return REV_BUF_SIZE;
}

void MJPEG_FRAME_SOURCE::getNextFrame(void * ptr)
{
	MJPEG_FRAME_SOURCE * pVideosource = (MJPEG_FRAME_SOURCE *)ptr;
	if(pVideosource == NULL)
	{
		printf("getNextFrame is NULL\n");
		return;
	}
	pVideosource->getNextFrame1();
}

void MJPEG_FRAME_SOURCE::getNextFrame1()
{
	struct timeval oldPresentationTime=fPresentationTime;
	if(m_stSourceInfo.dataGetfun)
	{
		/*第一次连接上需要通知上层，通知一次即可*/
		set_videostate_callback();
		m_stFrame.frameSize = 0;
		m_stFrame.data = fTo;
		m_stFrame.type = VIDEO_TYPE;
		m_stFrame.param = m_stSourceInfo.videoindex;
		m_stSourceInfo.dataGetfun(&m_stFrame);
		if(m_stFrame.frameSize > 4)
		{
			fFrameSize = m_stFrame.frameSize;
		}
		else
		{
			m_toDelay = 100;
			fFrameSize = 0;
			doGetNextFrame();
			return;
		}
	}
	
	m_toDelay = -1;
	if ( ((fPresentationTime.tv_sec==0)&&(fPresentationTime.tv_usec==0))
			||((oldPresentationTime.tv_sec==fPresentationTime.tv_sec)
				&&(oldPresentationTime.tv_usec==fPresentationTime.tv_usec)) ) {
		gettimeofday(&fPresentationTime, NULL);
	}
	//printf("m_stFrame.videolistsize:%d\n", m_stFrame.videolistsize);

	FramedSource::afterGetting(this);
}