/**
 * @FilePath     : mpp_adec.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-31 19:12:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-17 19:36:51
 * @Description  : 海思adec模块封装
 */

#ifndef _MPP_ADEC_H_
#define _MPP_ADEC_H_
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
#include "ot_common_adec.h"
#include "ot_audio_aac_adp.h"
// #include "ot_audio_mp3_adp.h"
// #include "ot_audio_opus_adp.h"

/*海思 adec必需参数*/
typedef struct _HiAdecNeedParam_S
{
    /*adec通道号*/
    int nChn;
    /*编码类型*/
    ot_payload_type enAdecType;
    /*采样率*/
    ot_audio_sample_rate enSampleRate;
    /*声道*/
    ot_audio_snd_mode enSoundMode;
    /*解码方式*/
    ot_adec_mode enAdecMode;
    /*音频编码缓存大小*/
    uint32_t u32BufSize;
} HiAdecNeedParam_S;

typedef struct _HiAdecExParam_S
{
    
} HiAdecExParam_S;

typedef struct _HiAdec_S HiAdec_S;
struct _HiAdec_S
{
    //info /**********************必需参数***************************/
    HiAdecNeedParam_S stNeedParam;
    // HiAdecExParam_S stExParam;
    //info /**********************辅助参数***************************/
    
    //info /**********************功能列表***************************/
    /*发送数据帧*/
    int (*mppAdec_sendStream)(HiAdec_S *pHandle, ot_audio_stream *pStream, td_bool bBlock);
    
    /*发送码流结束标识符*/
    int (*mppAdec_send_end_of_stream)(HiAdec_S *pHandle, td_bool bInstant);

    /*获取数据帧*/
    int (*mppAdec_getFrame)(HiAdec_S *pHandle, ot_audio_frame_info *pFrameInfo, td_bool bBlock);
    
    /*销毁数据帧*/
    int (*mppAdec_releaseFrame)(HiAdec_S *pHandle, ot_audio_frame_info *pFrameInfo);

    /*初始化adec*/
    int (*mppAdec_init)(HiAdec_S *pHandle);

    /*反初始化adec*/
    int (*mppAdec_uninit)(HiAdec_S *pHandle);
};

/**
 * @brief       : 分配adec句柄
 * @author      : zhouzirui
 * @param        {HiAdecNeedParam_S} stNeedParam：adec必须参数
 * @return       {*}成功返回句柄，失败返回NULL
 */
HiAdec_S *mppAdec_alloc(HiAdecNeedParam_S stNeedParam);

/**
 * @brief       : 释放adec句柄
 * @author      : zhouzirui
 * @param        {HiAdec_S} *pHandle：句柄
 * @return       {*}
 */
void mppAdec_release(HiAdec_S *pHandle);

#ifdef __cplusplus
}
#endif
#endif
