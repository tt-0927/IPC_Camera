/*
 * @FilePath: mjpeg_video_source.h
 * @Author: yangwenyao
 * @Date: 2023-01-03 14:04:51
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2023-07-19 19:54:02
 * @Descripttion: mpeg rtp 数据封装
 */
#ifndef __MJPEG_VIDEO_SOURCE_H__
#define __MJPEG_VIDEO_SOURCE_H__
#pragma once
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "FramedSource.hh"
#include "JPEGVideoSource.hh"

#include "custom_define.h"

/*!*****************************************************************************
 * \brief MJPEG Video Source (RFC 2345)
*************************************************************************************/
class MJPEG_Video_Source : public JPEGVideoSource
{
public:
    static MJPEG_Video_Source* createNew (UsageEnvironment& env, FramedSource* source)
    {
        return new MJPEG_Video_Source(env,source);
    }
    
public:
    virtual void doGetNextFrame();

    // static void getNextFrame(void * ptr);
    // void getNextFrame1();
    // int set_videostate_callback();

    virtual u_int8_t type() { return fType; };
    virtual u_int8_t qFactor() { return fLastQFactor; };
    virtual u_int8_t width() { return fWidth; };
    virtual u_int8_t height() { return fHeight; };
    virtual u_int16_t widthPixels()
    {
        return fWidthPixels;
    };
    virtual u_int16_t heightPixels()
    {
        return fHeightPixels;
    };
    virtual u_int8_t const* quantizationTables( u_int8_t& precision, u_int16_t& length );	
private:
    static void afterGettingFrame(void* clientData, unsigned frameSize,
                                  unsigned numTruncatedBytes,
                                  struct timeval presentationTime,
                                  unsigned durationInMicroseconds);
    void afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes,
                            struct timeval presentationTime,
                            unsigned durationInMicroseconds);
protected:
    MJPEG_Video_Source(UsageEnvironment& env, FramedSource* source);
    virtual ~MJPEG_Video_Source();

private:    
    FramedSource* fSource;
	struct timeval fLastCaptureTime;
    u_int8_t      fQuantizationTable[128];
    bool          m_qTable0Init;
    bool          m_qTable1Init;
    u_int16_t fQtableLength;
    u_int16_t fWidthPixels, fHeightPixels;// actual pixels, support width/height more than 2048
	u_int8_t fType, fLastQFactor, fWidth, fHeight;
    u_int8_t fPrecision;
    // JpegFrameParser *fJpegFrameParser;
    // unsigned char fJPEGHeader[JPEG_HEADER_SIZE];
public:
    // void *m_pToken;
    // int m_toDelay;
};
 



#endif//__MJPEG_VIDEO_SOURCE_H__