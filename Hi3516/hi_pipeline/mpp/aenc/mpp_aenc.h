/**
 * @FilePath     : mpp_aenc.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-31 19:12:10
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-05-19 11:37:03
 * @Description  : 海思aenc模块封装
 */

#ifndef _MPP_AENC_H_
#define _MPP_AENC_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "ss_mpi_audio.h"
#include "ot_common_aenc.h"
#include "ot_audio_aac_adp.h"
#include "ot_audio_mp3_adp.h"
#include "ot_audio_opus_adp.h"

/*海思 aenc必需参数*/
typedef struct _HiAencNeedParam_S
{
    /*aenc通道号*/
    int nChn;
    /*编码类型*/
    ot_payload_type enAencType;
    /*采样率*/
    ot_audio_sample_rate enSampleRate;
    /*声道*/
    ot_audio_snd_mode enSoundMode;
    /*每帧的采样点个数*/
    uint32_t u32PointNumPerFrame;
    /*音频编码缓存大小*/
    uint32_t u32BufSize;
    /*码率*/
    uint32_t u32BitRate;
} HiAencNeedParam_S;

typedef struct _HiAencExParam_S
{
    
} HiAencExParam_S;

typedef struct _HiAenc_S HiAenc_S;
struct _HiAenc_S
{
    //info /**********************必需参数***************************/
    HiAencNeedParam_S stNeedParam;
    // HiAencExParam_S stExParam;
    //info /**********************辅助参数***************************/
    
    //info /**********************功能列表***************************/
    /*发送数据帧*/
    int (*mppAenc_sendFrame)(HiAenc_S *pHandle, ot_audio_frame *pFrame);
    
    /*获取数据帧*/
    int (*mppAenc_getFrame)(HiAenc_S *pHandle, ot_audio_stream *pFrame, int nTimeoutMs);
    
    /*销毁数据帧*/
    int (*mppAenc_releaseFrame)(HiAenc_S *pHandle, ot_audio_stream *pFrame);

    /*初始化aenc*/
    int (*mppAenc_init)(HiAenc_S *pHandle);

    /*反初始化aenc*/
    int (*mppAenc_uninit)(HiAenc_S *pHandle);
};

/**
 * @brief       : 分配aenc句柄
 * @author      : zhouzirui
 * @param        {HiAencNeedParam_S} stNeedParam：aenc必须参数
 * @return       {*}成功返回句柄，失败返回NULL
 */
HiAenc_S *mppAenc_alloc(HiAencNeedParam_S stNeedParam);

/**
 * @brief       : 释放aenc句柄
 * @author      : zhouzirui
 * @param        {HiAenc_S} *pHandle：句柄
 * @return       {*}
 */
void mppAenc_release(HiAenc_S *pHandle);

#ifdef __cplusplus
}
#endif
#endif
