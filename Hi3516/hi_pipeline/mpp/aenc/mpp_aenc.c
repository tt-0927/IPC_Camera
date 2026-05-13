/**
 * @FilePath     : mpp_aenc.c
 * @Author       : zhouzirui
 * @Date         : 2025-03-31 19:12:07
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-17 16:41:32
 * @Description  : 海思aenc模块封装
 */

#include "mpp_aenc.h"
#include "mpi_common.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>

/**
 * @brief       : aenc初始化
 * @author      : zhouzirui
 * @param        {HiAenc_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int mppAenc_init(HiAenc_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    /*注册aac编码器*/
    ss_mpi_aenc_aac_init();
    /*注册mp3编码器*/
    // ss_mpi_aenc_mp3_init();
    /*注册opus编码器*/
    // ss_mpi_aenc_opus_init();

    /*aenc通道号*/
    ot_aenc_chn nAencChn = pHandle->stNeedParam.nChn;

    ot_aenc_attr_adpcm stAencAdpcm;
    ot_aenc_attr_g711 stAencG711;
    ot_aenc_attr_g726 stAencG726;
    ot_aenc_attr_lpcm stAencLpcm;
    ot_aenc_attr_aac stAencAac;
    // ot_aenc_attr_mp3 stAencMp3;
    // ot_aenc_attr_opus stAencOpus;

    ot_aenc_chn_attr stAencAttr;
    stAencAttr.type = pHandle->stNeedParam.enAencType;
    stAencAttr.point_num_per_frame = pHandle->stNeedParam.u32PointNumPerFrame;
    stAencAttr.buf_size = pHandle->stNeedParam.u32BufSize;

    switch (stAencAttr.type)
    {
    case OT_PT_ADPCMA:
        stAencAttr.value = &stAencAdpcm;
        stAencAdpcm.adpcm_type = OT_ADPCM_TYPE_DVI4; // AUDIO_ADPCM_TYPE
        break;
    case OT_PT_G711A:
    case OT_PT_G711U:
        stAencAttr.value = &stAencG711;
        break;
    case OT_PT_G726:
        stAencAttr.value = &stAencG726;
        stAencG726.g726bps = OT_G726_32K; // G726_BPS 小端 32K
        break;
    case OT_PT_LPCM:
        stAencAttr.value = &stAencLpcm;
        break;
    case OT_PT_AAC:
        stAencAttr.value = &stAencAac;
        stAencAac.aac_type = OT_AAC_TYPE_AACLC;
        stAencAac.bit_rate = pHandle->stNeedParam.u32BitRate; //OT_AAC_BPS_24K;
        stAencAac.bit_width = OT_AUDIO_BIT_WIDTH_16;
        stAencAac.sample_rate = pHandle->stNeedParam.enSampleRate;
        stAencAac.snd_mode = pHandle->stNeedParam.enSoundMode;
        stAencAac.transport_type = OT_AAC_TRANSPORT_TYPE_ADTS;
        stAencAac.band_width = 0;
        break;
    // case OT_PT_MP3:
    //     stAencAttr.value = &stAencMp3;
    //     stAencMp3.sample_rate = pHandle->stNeedParam.enSampleRate;
    //     stAencMp3.bit_width = OT_AUDIO_BIT_WIDTH_16;
    //     stAencMp3.sound_mode = pHandle->stNeedParam.enSoundMode;
    //     stAencMp3.bit_rate = pHandle->stNeedParam.u32BitRate; //OT_MP3_BPS_128K;
    //     stAencMp3.quality = 7; /* 7 : default quality */
    //     break;
    // case OT_PT_OPUS:
    //     stAencAttr.value = &stAencOpus;
    //     stAencOpus.bit_rate = OT_OPUS_BPS_96K;
    //     stAencOpus.app = OT_OPUS_APPLICATION_VOIP;
    //     stAencOpus.bit_width = OT_AUDIO_BIT_WIDTH_16;
    //     stAencOpus.sample_rate = pHandle->stNeedParam.enSampleRate;
    //     stAencOpus.snd_mode = pHandle->stNeedParam.enSoundMode;
    //     break;
    default:
        mpi_aenc_log("invalid aenc payload type:%d", stAencAttr.type);
        return TD_FAILURE;
    }

    /*创建音频编码通道*/
    CHECK_API_RETURN(ss_mpi_aenc_create_chn(nAencChn, &stAencAttr));

    return TD_SUCCESS;
}

/**
 * @brief       : aenc去初始化
 * @author      : zhouzirui
 * @param        {HiAenc_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int mppAenc_uninit(HiAenc_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    
    /*aenc通道号*/
    ot_aenc_chn nAencChn = pHandle->stNeedParam.nChn;

    /*销毁音频编码通道*/
    CHECK_API_RETURN(ss_mpi_aenc_destroy_chn(nAencChn));
    /*销毁aac编码器*/
    ss_mpi_aenc_aac_deinit();
    /*销毁mp3编码器*/
    // ss_mpi_aenc_mp3_deinit();
    /*销毁opus编码器*/
    // ss_mpi_aenc_opus_deinit();

    return TD_SUCCESS;
}

/**
 * @brief       : 发送数据帧
 * @author      : zhouzirui
 * @param        {HiAenc_S} *pHandle：句柄
 * @param        {ot_audio_frame} *pFrame：帧数据信息指针
 * @return       {*}成功返回0,失败返回-1
 */
static int mppAenc_sendFrame(HiAenc_S *pHandle, ot_audio_frame *pFrame)
{
    if (NULL == pHandle || NULL == pFrame)
    {
        return TD_FAILURE;
    }
    CHECK_API_RETURN(ss_mpi_aenc_send_frame(pHandle->stNeedParam.nChn, pFrame, NULL));

    return TD_SUCCESS;
}

/**
 * @brief       : 获取数据帧
 * @author      : zhouzirui
 * @param        {HiAenc_S} *pHandle：句柄
 * @param        {ot_audio_stream} *pFrame：帧数据信息指针
 * @param        {int} nTimeoutMs：等待时间
 * @return       {*}成功返回0,失败返回-1
 */
static int mppAenc_getFrame(HiAenc_S *pHandle, ot_audio_stream *pFrame, int nTimeoutMs)
{
    if (NULL == pHandle || NULL == pFrame || nTimeoutMs < -1)
    {
        return TD_FAILURE;
    }
    CHECK_API_RETURN(ss_mpi_aenc_get_stream(pHandle->stNeedParam.nChn, pFrame, nTimeoutMs));

    return TD_SUCCESS;
}

/**
 * @brief       : 销毁数据帧
 * @author      : zhouzirui
 * @param        {HiAenc_S} *pHandle：句柄
 * @param        {ot_audio_stream} *pFrame：帧数据信息指针
 * @return       {*}成功返回0,失败返回-1
 */
static int mppAenc_releaseFrame(HiAenc_S *pHandle, ot_audio_stream *pFrame)
{
    if (NULL == pHandle || NULL == pFrame)
    {
        return TD_FAILURE;
    }
    CHECK_API_RETURN(ss_mpi_aenc_release_stream(pHandle->stNeedParam.nChn, pFrame));

    return TD_SUCCESS;
}

HiAenc_S *mppAenc_alloc(HiAencNeedParam_S stNeedParam)
{
    HiAenc_S *pHandle = (HiAenc_S *)malloc(sizeof(HiAenc_S));
    memset(pHandle, 0, sizeof(HiAenc_S));

    //info /**********************必需参数***************************/
    pHandle->stNeedParam.nChn                   = stNeedParam.nChn;
    pHandle->stNeedParam.enAencType             = stNeedParam.enAencType;
    pHandle->stNeedParam.enSampleRate           = stNeedParam.enSampleRate;
    pHandle->stNeedParam.enSoundMode            = stNeedParam.enSoundMode;
    pHandle->stNeedParam.u32PointNumPerFrame    = stNeedParam.u32PointNumPerFrame;
    pHandle->stNeedParam.u32BufSize             = stNeedParam.u32BufSize;
    pHandle->stNeedParam.u32BitRate             = stNeedParam.u32BitRate;
    //info /**********************功能参数***************************/

    //info /**********************函数列表***************************/
    pHandle->mppAenc_init               = mppAenc_init;
    pHandle->mppAenc_uninit             = mppAenc_uninit;
    pHandle->mppAenc_sendFrame          = mppAenc_sendFrame;
    pHandle->mppAenc_getFrame           = mppAenc_getFrame;
    pHandle->mppAenc_releaseFrame       = mppAenc_releaseFrame;

    return pHandle;
}

void mppAenc_release(HiAenc_S *pHandle)
{
    if (pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
}