/**
 * @FilePath     : mpp_vi.c
 * @Author       : zhouzirui
 * @Date         : 2025-03-19 14:59:29
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-17 19:38:13
 * @Description  : 海思vi模块封装
 */

#include "mpp_vi.h"
#include "mpi_common.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>

/**
 * @brief   : 将VI的PIPE绑定到用户私有视频缓存VB池中
 * @param    {HiVi_S} *pHiViHandle：句柄
 * @param    {ot_vi_pipe} nViPipe：管道号
 * @return   {*}成功返回0,失败返回-1
 */
static int mppVi_attach_pipe_vb_pool(HiVi_S *pHiViHandle,ot_vi_pipe nViPipe)
{
    ot_vb_pool_cfg vb_pool_cfg;
    ot_pic_buf_attr stBufAttr;
    stBufAttr.width = pHiViHandle->stNeedParam.nWidth;
    stBufAttr.height = pHiViHandle->stNeedParam.nHeight;
    stBufAttr.align = OT_DEFAULT_ALIGN;
    stBufAttr.bit_width = OT_DATA_BIT_WIDTH_8;
    stBufAttr.pixel_format = pHiViHandle->stNeedParam.enPixelFormat;
    stBufAttr.compress_mode = pHiViHandle->stNeedParam.enCompressMode;
    stBufAttr.video_format = OT_VIDEO_FORMAT_LINEAR;

    vb_pool_cfg.blk_size = ot_common_get_pic_buf_size(&stBufAttr);
    vb_pool_cfg.blk_cnt = pHiViHandle->stExParam.nDepth > 0 ? pHiViHandle->stExParam.nDepth : 2;
    vb_pool_cfg.blk_cnt = 1;
    memcpy(vb_pool_cfg.mmz_name, MMZ_NAME, sizeof(MMZ_NAME));
    vb_pool_cfg.remap_mode = OT_VB_REMAP_MODE_NONE;

    mpi_vi_log("vb_pool_cfg.blk_size:%lld,%d,%s,%d", vb_pool_cfg.blk_size, vb_pool_cfg.blk_cnt, vb_pool_cfg.mmz_name, vb_pool_cfg.remap_mode);
    pHiViHandle->aVbPoolId = ss_mpi_vb_create_pool(&vb_pool_cfg);
    // ot_vb_pool pHiVpssHandle->aVbPoolId[nChn] = ss_mpi_vb_create_ext_pool(&vb_pool_cfg);
    if (pHiViHandle->aVbPoolId == OT_VB_INVALID_POOL_ID)
    {
        mpi_vi_log("创建vi dev:%d 通道:%d 用户视频缓冲池失败:%d", pHiViHandle->stNeedParam.nDevId, pHiViHandle->stNeedParam.nChannel, pHiViHandle->aVbPoolId);
        return TD_FAILURE;
    }
    /*设置VI PIPE的VB来源*/
    CHECK_API_RETURN(ss_mpi_vi_set_pipe_vb_src(nViPipe, OT_VB_SRC_USER));
    // /*将VI的PIPE绑定到某个视频缓存VB池中*/
    CHECK_API_RETURN(ss_mpi_vi_attach_pipe_vb_pool(nViPipe, pHiViHandle->aVbPoolId));

    return TD_SUCCESS;
}

/**
 * @brief       : vi初始化
 * @author      : zhouzirui
 * @param        {HiVi_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVi_init(HiVi_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    ot_vi_dev nViDev = pHandle->stNeedParam.nDevId;
    ot_vi_pipe nViPipe = pHandle->stExParam.nPipeId;
    ot_vi_chn nViChn = pHandle->stNeedParam.nChannel;
    sample_vi_cfg *pViCfg = &pHandle->stExParam.vi_cfg;
    pViCfg->pipe_info[0].pipe_attr.isp_bypass = TD_FALSE;
    // pViCfg->pipe_info[0].chn_info[0].chn_attr.mirror_en = TD_TRUE;
    // pViCfg->pipe_info[0].chn_info[0].chn_attr.flip_en = TD_TRUE;
    // pViCfg->pipe_info[0].chn_info[0].chn_attr.depth = pHandle->stExParam.nDepth;
    // CHECK_API_RETURN(sample_comm_vi_start_vi(pViCfg));
    /*启动MIPI_RX*/
    sample_comm_vi_start_mipi_rx(&pViCfg->sns_info,&pViCfg->mipi_info);
    /*设置VI设备属性*/
    CHECK_API_RETURN(ss_mpi_vi_set_dev_attr(nViDev, &pViCfg->dev_info.dev_attr));
    /*启用VI设备*/
    CHECK_API_RETURN(ss_mpi_vi_enable_dev(nViDev));

    /*设置VI，VPSS工作模式*/
    ot_vi_vpss_mode stViVpssMode;
    ot_vi_vpss_mode_type mode_type = OT_VI_ONLINE_VPSS_ONLINE; //OT_VI_ONLINE_VPSS_OFFLINE
    ot_vi_vpss_mode_type other_pipe_mode_type;
    memset(&stViVpssMode, 0, sizeof(ot_vi_vpss_mode));
    if (mode_type == OT_VI_OFFLINE_VPSS_ONLINE) {
        other_pipe_mode_type = OT_VI_OFFLINE_VPSS_ONLINE;
    } else {
        other_pipe_mode_type = OT_VI_OFFLINE_VPSS_OFFLINE;
    }
    stViVpssMode.mode[0] = mode_type;
    for (int i = 1; i < OT_VI_MAX_PIPE_NUM; i++) {
        stViVpssMode.mode[i] = other_pipe_mode_type;
    }
    CHECK_API_RETURN(ss_mpi_sys_set_vi_vpss_mode(&stViVpssMode));
    /*设置VI AIISP模式*/
    // CHECK_API_RETURN(ss_mpi_sys_set_vi_aiisp_mode(nViPipe,OT_VI_AIISP_MODE_DEFAULT));
    /*一对一绑定Dev和Pipe*/
    CHECK_API_RETURN(ss_mpi_vi_bind(nViDev,nViPipe));
    /*设置wdr合成组的属性*/
    CHECK_API_RETURN(ss_mpi_vi_set_wdr_fusion_grp_attr(pViCfg->grp_info.fusion_grp[0],&pViCfg->grp_info.fusion_grp_attr[0]));
    /*设置VI PIPE bayernr buff er个数 AI3DNR场景需要根据缓存需要调大buff er个数，建议设置为3个以上*/
    // CHECK_API_RETURN(ss_mpi_vi_set_pipe_bnr_buf_num(nViPipe, 4));
    /*创建一个VI PIPE*/
    CHECK_API_RETURN(ss_mpi_vi_create_pipe(nViPipe,&pViCfg->pipe_info[0].pipe_attr));
    /*设置VI在线PIPE处理时钟*/
    if (mode_type == OT_VI_ONLINE_VPSS_ONLINE || mode_type == OT_VI_ONLINE_VPSS_OFFLINE)
        CHECK_API_RETURN(ss_mpi_vi_set_pipe_online_clock(nViPipe, pViCfg->pipe_info[0].pixel_rate)); //SC4336P_MIPI_4M_30FPS_10BIT: 126000000
    /*启用VI PIPE*/
    CHECK_API_RETURN(ss_mpi_vi_start_pipe(nViPipe));
    /*设置VI PIPE数据的来源*/
    // CHECK_API_RETURN(ss_mpi_vi_set_pipe_frame_source(nViPipe, OT_VI_PIPE_FRAME_SOURCE_FE));//数据来自于前端FE，默认模式。
    /*设置VI PIPE的VB来源*/
    CHECK_API_RETURN(ss_mpi_vi_set_pipe_vb_src(nViPipe, OT_VB_SRC_COMMON));
    // /*将VI的PIPE绑定到某个视频缓存VB池中*/
    // CHECK_API_RETURN(ss_mpi_vi_attach_pipe_vb_pool(nViPipe, 1));
    // CHECK_API_RETURN(mppVi_attach_pipe_vb_pool(pHandle, nViPipe));
    /*设置VI通道属性*/
    CHECK_API_RETURN(ss_mpi_vi_set_chn_attr(nViPipe, nViChn, &pViCfg->pipe_info[0].chn_info[0].chn_attr));
    /*设置VI PIPE的3DNR属性*/
    CHECK_API_RETURN(ss_mpi_vi_set_pipe_3dnr_attr(nViPipe, &pViCfg->pipe_info[0].nr_attr));
    /*设置VI通道使用VB的来源*/
    // CHECK_API_RETURN(ss_mpi_vi_set_chn_vb_src(nViPipe, nViChn, OT_VB_SRC_COMMON));
    // /*将VI PIPE的通道绑定到某个视频缓存VB池中*/
    // CHECK_API_RETURN(ss_mpi_vi_attach_chn_vb_pool(nViPipe, nViChn, 2));
    /*启用VI通道*/
    CHECK_API_RETURN(ss_mpi_vi_enable_chn(nViPipe, nViChn));

    return TD_SUCCESS;
}

/**
 * @brief       : vi去初始化
 * @author      : zhouzirui
 * @param        {HiVi_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVi_uninit(HiVi_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    sample_vi_cfg *pViCfg = &pHandle->stExParam.vi_cfg;
    // sample_comm_vi_stop_vi(pViCfg);
    ot_vi_dev nViDev = pHandle->stNeedParam.nDevId;
    ot_vi_pipe nViPipe = pHandle->stExParam.nPipeId;
    ot_vi_chn nViChn = pHandle->stNeedParam.nChannel;
    if (pHandle->aVbPoolId != OT_VB_INVALID_POOL_ID)
    {
        /*将VI PIPE的通道从某个视频缓存VB池中解绑定*/
        CHECK_API_RETURN(ss_mpi_vi_detach_chn_vb_pool(nViPipe, nViChn));
        /*销毁一个视频缓存池*/
        CHECK_API_RETURN(ss_mpi_vb_destroy_pool(pHandle->aVbPoolId));
    }
    /*禁用VI通道*/
    CHECK_API_RETURN(ss_mpi_vi_disable_chn(nViPipe, nViChn));
    /*禁用VI PIPE*/
    CHECK_API_RETURN(ss_mpi_vi_stop_pipe(nViPipe));
    /*销毁一个VI PIPE*/
    CHECK_API_RETURN(ss_mpi_vi_destroy_pipe(nViPipe));
    /*一对一解绑定Dev和Pipe*/
    CHECK_API_RETURN(ss_mpi_vi_unbind(nViDev, nViPipe));
    /*禁用VI设备*/
    CHECK_API_RETURN(ss_mpi_vi_disable_dev(nViDev));
    /*禁用MIPI_RX*/
    sample_comm_vi_stop_mipi_rx(&pViCfg->sns_info, &pViCfg->mipi_info);
    return TD_SUCCESS;
}

/**
 * @brief       : 获取管道采集帧
 * @author      : zhouzirui
 * @param        {HiVi_S} *pHandle：句柄
 * @param        {ot_video_frame_info} *pFrameInfo：帧数据信息指针
 * @param        {int} nTimeoutMs：等待时间
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVi_getPipeStream(HiVi_S *pHandle,  ot_video_frame_info *pFrameInfo, int nTimeoutMs)
{
    if (NULL == pHandle || NULL == pFrameInfo || nTimeoutMs < -1)
    {
        return TD_FAILURE;
    }
    CHECK_API_RETURN(ss_mpi_vi_get_pipe_frame(pHandle->stExParam.nPipeId, pFrameInfo, nTimeoutMs));
    return TD_SUCCESS;
}

/**
 * @brief       : 释放管道采集帧
 * @author      : zhouzirui
 * @param        {HiVi_S} *pHandle：句柄
 * @param        {ot_video_frame_info} *pFrameInfo：帧数据信息指针
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVi_releasePipeStream(HiVi_S *pHandle,  ot_video_frame_info *pFrameInfo)
{
    if (NULL == pHandle || NULL == pFrameInfo)
    {
        return TD_FAILURE;
    }
    CHECK_API_RETURN(ss_mpi_vi_release_pipe_frame(pHandle->stExParam.nPipeId, pFrameInfo));
    return TD_SUCCESS;
}

/**
 * @brief   : 设置上下左右翻转
 * @param    {HiVi_S} *pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
static int mppVi_set_mirror_flip(HiVi_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    ot_vi_pipe nViPipe = pHandle->stExParam.nPipeId;
    ot_vi_chn nViChn = pHandle->stNeedParam.nChannel;
    ot_vi_chn_attr stChnAttr;
    /* 获取VI通道属性 */
    CHECK_API_RETURN(ss_mpi_vi_get_chn_attr(nViPipe, nViChn, &stChnAttr));
    stChnAttr.mirror_en = pHandle->stExParam.bMirror;
    stChnAttr.flip_en = pHandle->stExParam.bFlip;
    /* 设置VI通道属性 */
    CHECK_API_RETURN(ss_mpi_vi_set_chn_attr(nViPipe, nViChn, &stChnAttr));

    return TD_SUCCESS;
}

/**
 * @brief   : 设置sensor上下左右翻转
 * @param    {HiVi_S} *pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
static int mppVi_set_sensor_mirror_flip(HiVi_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    ot_isp_sns_mirrorflip_type sns_mirror_flip;
    if (pHandle->stExParam.bMirror == TD_TRUE && pHandle->stExParam.bFlip == TD_TRUE)
    {
        sns_mirror_flip = ISP_SNS_MIRROR_FLIP;
    }
    else if (pHandle->stExParam.bMirror == TD_TRUE && pHandle->stExParam.bFlip == TD_FALSE)
    {
        sns_mirror_flip = ISP_SNS_MIRROR;
    }
    else if (pHandle->stExParam.bMirror == TD_FALSE && pHandle->stExParam.bFlip == TD_TRUE)
    {
        sns_mirror_flip = ISP_SNS_FLIP;
    }
    else
    {
        sns_mirror_flip = ISP_SNS_NORMAL;
    }

    /* 设置镜像、倒置翻转 */
    CHECK_API_RETURN(sample_comm_isp_set_mirror_flip(pHandle->stNeedParam.sns_type, pHandle->stExParam.nPipeId, sns_mirror_flip));

    return TD_SUCCESS;
}

/**
 * @brief       : 获取采集帧
 * @author      : zhouzirui
 * @param        {HiVi_S} *pHandle：句柄
 * @param        {int} nChn：通道号
 * @param        {ot_video_frame_info} *pFrameInfo：帧数据信息指针
 * @param        {int} nTimeoutMs：等待时间
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVi_getChnFrame(HiVi_S *pHandle, int nChn, ot_video_frame_info *pFrameInfo, int nTimeoutMs)
{
    if (NULL == pHandle || NULL == pFrameInfo || nTimeoutMs < -1)
    {
        return TD_FAILURE;
    }
    CHECK_API_RETURN(ss_mpi_vi_get_chn_frame(pHandle->stExParam.nPipeId, nChn, pFrameInfo, nTimeoutMs));
    return TD_SUCCESS;
}

/**
 * @brief       : 释放采集帧
 * @author      : zhouzirui
 * @param        {HiVi_S} *pHandle：句柄
 * @param        {int} nChn：通道号
 * @param        {ot_video_frame_info} *pFrameInfo：帧数据信息指针
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVi_releaseChnFrame(HiVi_S *pHandle, int nChn, ot_video_frame_info *pFrameInfo)
{
    if (NULL == pHandle || NULL == pFrameInfo)
    {
        return TD_FAILURE;
    }
    CHECK_API_RETURN(ss_mpi_vi_release_chn_frame(pHandle->stExParam.nPipeId, nChn, pFrameInfo));
    return TD_SUCCESS;
}

HiVi_S *mppVi_alloc(HiViNeedParam_S stNeedParam)
{
    HiVi_S *pHandle = (HiVi_S *)malloc(sizeof(HiVi_S));
    memset(pHandle, 0, sizeof(HiVi_S));

    //info /**********************必需参数***************************/
    pHandle->stNeedParam.nDevId         = stNeedParam.nDevId;
    pHandle->stNeedParam.nChannel       = stNeedParam.nChannel;
    snprintf( (char*)(pHandle->stNeedParam.aEnityName),sizeof(pHandle->stNeedParam.aEnityName), "%s", stNeedParam.aEnityName);
    pHandle->stNeedParam.nWidth         = stNeedParam.nWidth;
    pHandle->stNeedParam.nHeight        = stNeedParam.nHeight;
    pHandle->stNeedParam.enPixelFormat  = stNeedParam.enPixelFormat;
    pHandle->stNeedParam.enCompressMode = stNeedParam.enCompressMode;
    pHandle->stNeedParam.sns_type       = stNeedParam.sns_type;

    //info /**********************功能参数***************************/
    pHandle->stExParam.nPipeId                  = stNeedParam.nDevId;
    pHandle->stExParam.nSrcFrameRate            = -1;
    pHandle->stExParam.nDstFrameRate            = -1;
    pHandle->stExParam.nDepth                   = 0;
    pHandle->stExParam.bDumpEnable              = TD_FALSE;
    pHandle->stExParam.nScaWidth                = stNeedParam.nWidth;
    pHandle->stExParam.nScaHeight               = stNeedParam.nHeight;
    pHandle->stExParam.bMirror                  = TD_FALSE;
    pHandle->stExParam.bFlip                    = TD_FALSE;
    pHandle->stExParam.bUseSensorRollingOver    = TD_FALSE;

    pHandle->aVbPoolId = OT_VB_INVALID_POOL_ID;
    /*设置VI模块vi_cfg初始值*/
    sample_comm_vi_get_default_vi_cfg(pHandle->stNeedParam.sns_type, &pHandle->stExParam.vi_cfg);

    //info /**********************函数列表***************************/
    pHandle->mppVi_init                     = mppVi_init;
    pHandle->mppVi_uninit                   = mppVi_uninit;
    pHandle->mppVi_getChnFrame              = mppVi_getChnFrame;
    pHandle->mppVi_releaseChnFrame          = mppVi_releaseChnFrame;
    pHandle->mppVi_getPipeStream            = mppVi_getPipeStream;
    pHandle->mppVi_releasePipeStream        = mppVi_releasePipeStream;
    pHandle->mppVi_set_mirror_flip          = mppVi_set_mirror_flip;
    pHandle->mppVi_set_sensor_mirror_flip   = mppVi_set_sensor_mirror_flip;
    return pHandle;
}

void mppVi_release(HiVi_S *pHandle)
{
    if (pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
}