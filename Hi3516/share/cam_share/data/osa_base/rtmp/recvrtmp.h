

#ifndef RECV_CODE_RTMP_STREAM_DATA_H_
#define RECV_CODE_RTMP_STREAM_DATA_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rtmp.h"
#include "os_thr.h"


//数据上抛
//datatype 0-video,1-audio
typedef int(*rtmp_DealbitFunc)(char *buff,int buffLen,int datatype,void* user);


typedef void* recv_rtmpHandle_t;


typedef struct rtmp_fream_Info
{
	int frameSize;
	unsigned int width;
	unsigned int height;
	unsigned int fps;
	unsigned char *data;
	int iFrame;
	int audio_type;
	unsigned int sample_rate;
	int type;

}rtmp_fream_t;



typedef struct _RTMP_INPARAM_INFO_
{
	char rtmpUrl[256];
	int isconversionAudioToAac;		//是否将音频转为aac
	int isconversionAudioSampleRate;	//将音频转为固定的采样率，转换后的采样率
	rtmp_DealbitFunc dealData;			//上抛数据
	void *user;							//上层用户句柄

}rtmp_inparam_t;



typedef struct _RECV_RTMP_INFO_
{
	rtmp_inparam_t inparam;
	RTMP *rtmp;
	int bLiveStream;	//是否是直播流
	OS_ThrHndl tid;
	char *spsbuf;
	int spsLen;
	char *ppsbuf;
	int ppsLen;


}recv_rtmpInfo_t;

recv_rtmpHandle_t recv_init_rtmp(rtmp_inparam_t inparam);

int recv_unInit_rtmp(recv_rtmpHandle_t *Handle);

int free_frameMessage_buff(rtmp_fream_t *buff);

#endif //RECV_CODE_RTMP_STREAM_DATA_H_


