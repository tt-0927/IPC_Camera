/*************************************************************************
	> File Name: audio_enc.h
	> Author:luoyk 
	> Mail: 
	> Created Time: 2022年09月01日 星期四 11时25分38秒
 ************************************************************************/

#ifndef _AUDIO_ENC_H
#define _AUDIO_ENC_H

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus
#include "libavcodec/avcodec.h"
#include "libavutil/channel_layout.h"
#include "libavutil/common.h"
#include "libavutil/frame.h"
#include "libavutil/samplefmt.h"
#include "libavutil/opt.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"

#include "os_ringBuf.h"
#include "os_thr.h"
#include "os_que.h"
#include "os_sem.h"
#include "dlog.h"



typedef struct _FFAUDIOENCNEEDPARAM{

    /*编码器ID*/
    enum AVCodecID enCodecId;
    /*采样率*/
    int nSampleRate;
    /*采样格式*/
    enum AVSampleFormat enSampleFormat;
    /*编码缓存帧数*/
    int nPacketCnt;

    /*是否要重采样*/
    uint8_t bSwr;
    int nChannel;

}ff_AudioEncNeedParam_S;

typedef struct _FFAUDIOENCEXPARAM{

    /*编码器名*/
    char *pCodecName;
    /*码流*/
    int nBite;
    /*编码等级 如FF_PROFILE_AAC_LOW*/
    int nProfile;
    /*延时帧数*/
    int nDelaySum;
    /*adts for aac*/
    int nAdts;

}ff_AudioEncExParam_S;


typedef struct _FF_AUDIOENC_S ff_AudioEnc_S;

struct _FF_AUDIOENC_S{

/***************功能***********************************/

    /*送数据到编码*/
    int (*send_frame)( ff_AudioEnc_S* pHandle, char* pPcmData, int nSize );
    /*接受编码帧*/
    AVPacket* (*receive_packet)( ff_AudioEnc_S* pHandle, int nTimeOut );
    /*释放编码帧*/
    int (*release_packet)( AVPacket* pPacket );
    /*初始化*/
    int (*ff_audioEnc_init)( ff_AudioEnc_S* pHandle );
    /*反初始化*/
    int (*ff_audioEnc_uninit)( ff_AudioEnc_S* pHandle );

/***************属性***********************************/
    ff_AudioEncNeedParam_S stNeedParam;
    ff_AudioEncExParam_S stExParam;
    /*编码器*/
    AVCodec *pCodec;
    /*编码器上下文*/
    AVCodecContext *pCodecCtx;

    /* 重采样上下文 */
    SwrContext *pSwrCtx;
    
    /*送一次pcm数据的大小*/
    int nFrameByte;
    AVFrame *pFrame;
    AVPacket* pPacket;
    /* 重采样帧 */
    AVFrame* pAudioResampleFrame;
    
    /*存放编码后缓存帧队列*/
    OS_QueHndl stPacketQue;

    /*存放pcm原始数据流*/
    OS_streamRingBufHndl stFrameRing;
    
    OS_SemHndl stFrameSem;
    OS_ThrHndl stEncode_thrId;
    
    uint8_t nExit;

};

/*分配编码句柄*/
ff_AudioEnc_S* ff_audioEnc_alloc( ff_AudioEncNeedParam_S stNeedParam, char* pCodecName);
/*释放编码句柄*/
int ff_audioEnc_release( ff_AudioEnc_S* pHandle);


#ifdef __cplusplus
}
#endif	// __cplusplus
#endif
