/*************************************************************************
	> File Name: rockit_aenc.h
	> Author:luoyk 
	> Mail: 
	> Created Time: 2022年06月23日 星期四 10时23分30秒
 ************************************************************************/

#ifndef _ROCKIT_AENC_H
#define _ROCKIT_AENC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/poll.h>
#include "rk_debug.h"
#include "rk_mpi_aenc.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_sys.h"

/*送编码的pcm的数据结构*/
typedef struct _AUDIODATA_S{

    /*pcm数据*/
    uint8_t* pData;
    
    /*数据大小*/
    int nSize;
    
    /*内部释放数据*/
    int (*pFreeCB) (void* pData);
    
    /*超时时间ms*/
    int nTimeOut;
    
    /*计数*/
    uint32_t unSeq;
    
    /*时间戳us*/
    uint64_t unTimeStamp;
    
}PcmInfo_S;



typedef struct _RKAENCNEEDPARAM_S{

    /*通道号*/
    int nChn;
    /*编码格式*/
    RK_CODEC_ID_E enType;

    /*编码采样精度*/
    AUDIO_BIT_WIDTH_E enBitWidth;
    /*编码通道数*/
    int nChannels;
    /*编码采样率*/
    int nSampleRate;
    /*比特率*/
    int nBitrate;

}RkAencNeedParam_S;

typedef struct _RKAENCEXPARAM_S{
    /*缓冲大小 默认4*/
    int nBuffCount;
}RkAencExParam_S;


typedef struct _RKAENC_S RkAenc_S;
struct _RKAENC_S{

    int (*rockitAenc_init) ( RkAenc_S* pHandle );
    int (*rockitAenc_uninit) ( RkAenc_S* pHandle );
    /*送数据到编码通道*/
    int (*rockitAenc_send_data) ( RkAenc_S* pHandle, PcmInfo_S* stPcmInfo);
    /*获取编码帧*/
    int (*rockitAenc_get_frame) ( RkAenc_S* pHandle, AUDIO_STREAM_S* pstStream, int nTimeOut);
    /*获取编码帧的虚拟地址*/
    uint8_t* (*rockitAenc_get_virData) ( AUDIO_STREAM_S* pstStream );
    /*释放编码帧*/
    int (*rockitAenc_release_frame) ( RkAenc_S* pHandle, AUDIO_STREAM_S* pstStream );

    RkAencNeedParam_S stNeedParam;
    RkAencExParam_S stExParam;

    /*辅助参数*/
    int nFd;

};


/*分配一个音频编码句柄*/
RkAenc_S* rockitAenc_alloc( RkAencNeedParam_S stNeedParam);

/*释放编码句柄*/
int rockitAenc_release( RkAenc_S* pHandle );

#ifdef __cplusplus
}
#endif
#endif
