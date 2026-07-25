/*************************************************************************
	> File Name: rockit_aenc.h
	> Author:luoyk 
	> Mail: 
	> Created Time: 2022年06月23日 星期四 10时23分30秒
 ************************************************************************/

#ifndef _ROCKIT_ADEC_H
#define _ROCKIT_ADEC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/poll.h>

#include "mpi_common.h"

#include "rk_debug.h"
#include "rk_mpi_adec.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_sys.h"

/*送编码的pcm的数据结构*/
typedef struct _AUDIOFRAMEDATA_S{

    /*要解码数据*/
    uint8_t* pData;
    
    /*数据大小*/
    int nSize;
    
    /*内部释放数据*/
    int (*pFreeCB) (void* pData);
    
    /*阻塞标志*/
    RK_BOOL bBlock;
    
    /*计数*/
    uint32_t unSeq;
    
    /*时间戳us*/
    uint64_t unTimeStamp;
    
}AudioFrameInfo_S;

typedef struct _RkAdecNeedParam_S_
{
    /*通道号*/
    int nChn;
    /*解码协议类型*/
    RK_CODEC_ID_E enType;
    /*解码模式：流模式和帧包模式*/
    ADEC_MODE_E enMode;
    /*解码通道数*/
    int nChannels;
    /*解码采样率*/
    int nSampleRate;
    /*解码采样深度*/
    int nBitPerCodedSample;
} RkAdecNeedParam_S;

typedef struct _RkAdecExParam_S_
{
    /*缓冲数量 默认4*/
    int nBuffCount;
    /*缓冲大小 默认4096*/
    int nBuffSize;
    /* 获取队列深度 */
    // int nDepth;
    /* 解码输入缓存数量 */
    // int nInBufCount;
} RkAdecExParam_S;

typedef struct _RKADEC_S RkAdec_S;
struct _RKADEC_S{

    // info /**********************必需参数***************************/
    RkAdecNeedParam_S stNeedParam;
    RkAdecExParam_S stExParam;

    // info /**********************辅助参数***************************/

    // info /**********************功能列表***************************/

    /* adec初始化 */
    int (*rockitAdec_init)(RkAdec_S *pHandle);

    /* adec去初始化 */
    int (*rockitAdec_uninit)(RkAdec_S *pHandle);

    /* 发送码流结束标识符 */
    int (*rockitAdec_send_endOfStream)(RkAdec_S *pHandle, RK_BOOL bInstant);

    /*送数据到解码通道*/
    int (*rockitAdec_send_data)(RkAdec_S *pHandle, AudioFrameInfo_S *pstFrameInfo);

    /*清空解码通道缓存*/
    int (*rockitAdec_clear_chnBuf)(RkAdec_S *pHandle);

    /*查询解码通道状态*/
    int (*rockitAdec_query_chnStat)(RkAdec_S *pHandle, ADEC_CHN_STATE_S *pstStat);

    /*获取解码帧*/
    int (*rockitAdec_get_frame)(RkAdec_S *pHandle, AUDIO_FRAME_INFO_S *pstStream, RK_BOOL bBlock);

    /*获取解码音频帧信息的虚拟地址*/
    uint8_t *(*rockitAdec_get_virData)(AUDIO_FRAME_INFO_S *pstStream);

    /*释放音频解码帧数据*/
    int (*rockitAdec_release_frame)(RkAdec_S *pHandle, AUDIO_FRAME_INFO_S *pstStream);

};

/**
 * @brief   : 分配adec句柄
 * @param    {RkAdecNeedParam_S} stNeedParam 必须参数
 * @return   {RkAdec_S *} 成功返回句柄，失败返回NULL
 */
RkAdec_S *rockitAdec_alloc(RkAdecNeedParam_S stNeedParam);

/**
 * @brief   : 释放adec句柄
 * @param    {RkAdec_S*} pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
int rockitAdec_release(RkAdec_S *pHandle);

#ifdef __cplusplus
}
#endif
#endif
