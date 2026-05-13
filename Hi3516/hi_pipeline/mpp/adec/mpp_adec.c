/**
 * @FilePath     : mpp_adec.c
 * @Author       : zhouzirui
 * @Date         : 2025-05-13 15:10:33
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-17 19:38:08
 * @Description  : 海思adec模块封装
 */

#include "mpp_adec.h"
#include "mpi_common.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>

/**
 * @brief       : adec初始化
 * @author      : zhouzirui
 * @param        {HiAdec_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int mppAdec_init(HiAdec_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    /*注册aac解码器*/
    ss_mpi_adec_aac_init();
    /*注册mp3解码器*/
    // ss_mpi_adec_mp3_init();
    /*注册opus解码器*/
    // ss_mpi_adec_opus_init();

    /*adec通道号*/
    ot_adec_chn nAdecChn = pHandle->stNeedParam.nChn;

    ot_adec_attr_adpcm stAdecAdpcm;
    ot_adec_attr_g711 stAdecG711;
    ot_adec_attr_g726 stAdecG726;
    ot_adec_attr_lpcm stAdecLpcm;
    ot_adec_attr_aac stAdecAac;
    // ot_adec_attr_mp3 stAdecMp3;
    // ot_adec_attr_opus stAdecOpus;

    ot_adec_chn_attr stAdecAttr;
    stAdecAttr.type = pHandle->stNeedParam.enAdecType;
    stAdecAttr.mode = pHandle->stNeedParam.enAdecMode;
    stAdecAttr.buf_size = pHandle->stNeedParam.u32BufSize;

    switch (stAdecAttr.type)
    {
    case OT_PT_ADPCMA:
        stAdecAttr.value = &stAdecAdpcm;
        stAdecAdpcm.adpcm_type = OT_ADPCM_TYPE_DVI4; // AUDIO_ADPCM_TYPE
        break;
    case OT_PT_G711A:
    case OT_PT_G711U:
        stAdecAttr.value = &stAdecG711;
        break;
    case OT_PT_G726:
        stAdecAttr.value = &stAdecG726;
        stAdecG726.g726bps = OT_MEDIA_G726_40K; // G726_BPS 小端 32K
        break;
    case OT_PT_LPCM:
        stAdecAttr.value = &stAdecLpcm;
        stAdecAttr.mode = OT_ADEC_MODE_PACK; /* lpcm must use pack mode */
        break;
    case OT_PT_AAC:
        stAdecAttr.value = &stAdecAac;
        stAdecAac.transport_type = OT_AAC_TRANSPORT_TYPE_ADTS;
        break;
    // case OT_PT_MP3:
    //     stAdecAttr.value = &stAdecMp3;
    //     break;
    // case OT_PT_OPUS:
    //     stAdecAttr.value = &stAdecOpus;
    //     stAdecOpus.sample_rate = pHandle->stNeedParam.enSampleRate;
    //     stAdecOpus.snd_mode = pHandle->stNeedParam.enSoundMode;
    //     break;
    default:
        mpi_adec_log("invalid adec payload type:%d", stAdecAttr.type);
        return TD_FAILURE;
    }

    /*创建音频解码通道*/
    CHECK_API_RETURN(ss_mpi_adec_create_chn(nAdecChn, &stAdecAttr));

    return TD_SUCCESS;
}

/**
 * @brief       : adec去初始化
 * @author      : zhouzirui
 * @param        {HiAdec_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int mppAdec_uninit(HiAdec_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    
    /*adec通道号*/
    ot_adec_chn nAdecChn = pHandle->stNeedParam.nChn;

    /*销毁音频解码通道*/
    CHECK_API_RETURN(ss_mpi_adec_destroy_chn(nAdecChn));
    /*销毁aac解码器*/
    ss_mpi_adec_aac_deinit();
    /*销毁mp3解码器*/
    // ss_mpi_adec_mp3_deinit();
    /*销毁opus解码器*/
    // ss_mpi_adec_opus_deinit();

    return TD_SUCCESS;
}

/**
 * @brief       : 发送数据帧
 * @author      : zhouzirui
 * @param        {HiAdec_S} *pHandle：句柄
 * @param        {ot_audio_stream} *pStream：音频码流数据指针
 * @param        {td_bool} bBlock：阻塞标识
 * @return       {*}成功返回0,失败返回-1 
 */
static int mppAdec_sendStream(HiAdec_S *pHandle, ot_audio_stream *pStream, td_bool bBlock)
{
    if (NULL == pHandle || NULL == pStream)
    {
        return TD_FAILURE;
    }
    /*向音频解码通道发送码流*/
    CHECK_API_RETURN(ss_mpi_adec_send_stream(pHandle->stNeedParam.nChn, pStream, bBlock));

    return TD_SUCCESS;
}

/**
 * @brief       : 发送码流结束标识符
 * @author      : zhouzirui
 * @param        {HiAdec_S} *pHandle：句柄
 * @param        {td_bool} bInstant：是否立即清除解码器内部的缓存数据
 * @return       {*}成功返回0,失败返回-1  
 */
static int mppAdec_send_end_of_stream(HiAdec_S *pHandle, td_bool bInstant)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    /*向解码器发送码流结束标识符，并清除码流buffer*/
    CHECK_API_RETURN(ss_mpi_adec_send_end_of_stream(pHandle->stNeedParam.nChn, bInstant));

    return TD_SUCCESS;
}

/**
 * @brief       : 获取数据帧
 * @author      : zhouzirui
 * @param        {HiAdec_S} *pHandle：句柄
 * @param        {ot_audio_frame_info} *pFrameInfo：帧数据信息指针
 * @param        {td_bool} bBlock：阻塞标识
 * @return       {*}成功返回0,失败返回-1
 */
static int mppAdec_getFrame(HiAdec_S *pHandle, ot_audio_frame_info *pFrameInfo, td_bool bBlock)
{
    if (NULL == pHandle || NULL == pFrameInfo)
    {
        return TD_FAILURE;
    }
    /*获取音频解码帧数据*/
    CHECK_API_RETURN(ss_mpi_adec_get_frame(pHandle->stNeedParam.nChn, pFrameInfo, bBlock));

    return TD_SUCCESS;
}

/**
 * @brief       : 销毁数据帧
 * @author      : zhouzirui
 * @param        {HiAdec_S} *pHandle：句柄
 * @param        {ot_audio_stream} *pFrame：帧数据信息指针
 * @return       {*}成功返回0,失败返回-1
 */
static int mppAdec_releaseFrame(HiAdec_S *pHandle, ot_audio_frame_info *pFrameInfo)
{
    if (NULL == pHandle || NULL == pFrameInfo)
    {
        return TD_FAILURE;
    }
    /*释放获取到的音频解码帧数据*/
    CHECK_API_RETURN(ss_mpi_adec_release_frame(pHandle->stNeedParam.nChn, pFrameInfo));

    return TD_SUCCESS;
}

HiAdec_S *mppAdec_alloc(HiAdecNeedParam_S stNeedParam)
{
    HiAdec_S *pHandle = (HiAdec_S *)malloc(sizeof(HiAdec_S));
    memset(pHandle, 0, sizeof(HiAdec_S));

    //info /**********************必需参数***************************/
    pHandle->stNeedParam.nChn                   = stNeedParam.nChn;
    pHandle->stNeedParam.enAdecType             = stNeedParam.enAdecType;
    pHandle->stNeedParam.enSampleRate           = stNeedParam.enSampleRate;
    pHandle->stNeedParam.enSoundMode            = stNeedParam.enSoundMode;
    pHandle->stNeedParam.enAdecMode             = stNeedParam.enAdecMode;
    pHandle->stNeedParam.u32BufSize             = stNeedParam.u32BufSize;
    //info /**********************功能参数***************************/

    //info /**********************函数列表***************************/
    pHandle->mppAdec_init               = mppAdec_init;
    pHandle->mppAdec_uninit             = mppAdec_uninit;
    pHandle->mppAdec_sendStream         = mppAdec_sendStream;
    pHandle->mppAdec_send_end_of_stream = mppAdec_send_end_of_stream;
    pHandle->mppAdec_getFrame           = mppAdec_getFrame;
    pHandle->mppAdec_releaseFrame       = mppAdec_releaseFrame;

    return pHandle;
}

void mppAdec_release(HiAdec_S *pHandle)
{
    if (pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
}