#ifndef __MJPEG_FRAME_SOURCE_H__
#define __MJPEG_FRAME_SOURCE_H__

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "FramedSource.hh"
#include "custom_define.h"

class MJPEG_FRAME_SOURCE : public FramedSource
{
public:
    static MJPEG_FRAME_SOURCE* createNew (UsageEnvironment& env, Video_Source_Info_t& souceInfo)
    {
        return new MJPEG_FRAME_SOURCE(env,souceInfo);
    }
    
public:
    virtual void doGetNextFrame();
    virtual unsigned int maxFrameSize() const;

    static void getNextFrame(void * ptr);
    void getNextFrame1();
    int set_videostate_callback();
    
    int server_inputData(unsigned char* pData, int nDataLen);
protected:
    MJPEG_FRAME_SOURCE(UsageEnvironment& env, Video_Source_Info_t& souceInfo);
    virtual ~MJPEG_FRAME_SOURCE();
    
public:
  Video_Source_Info_t m_stSourceInfo;
  Rtsp_ClientStream_State_t m_stStatus;
  Fream_Info_t m_stFrame;
  void *m_pToken;
  int m_toDelay;

};

#endif//__MJPEG_FRAME_SOURCE_H__